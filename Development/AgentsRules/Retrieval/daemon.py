"""Persistent Epidemic retrieval daemon.

Keeps SQLite, vectors and BGE reranker resident.
Refreshes changed indexed files synchronously when a search is requested.
Only loopback HTTP is allowed.
"""

from __future__ import annotations

import contextlib
import argparse
import hashlib
import io
import json
import math
import re
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

import numpy as np

import retrieve as core


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNTIME = ROOT / ".ai" / "retrieval"
CONFIG = json.loads((HERE / "config.json").read_text(encoding="utf-8-sig"))

HOST = CONFIG.get("daemon_host", "127.0.0.1")
_port_base = int(CONFIG.get("daemon_port_base", 8765))
_port_span = max(int(CONFIG.get("daemon_port_span", 1000)), 1)
_port_offset = int(hashlib.sha256(str(ROOT).lower().encode("utf-8")).hexdigest()[:8], 16) % _port_span
PORT = _port_base + _port_offset

if HOST not in {"127.0.0.1", "localhost", "::1"}:
    raise RuntimeError("Retrieval daemon must bind to loopback only")


def quiet_build(db) -> dict:
    buffer = io.StringIO()

    with contextlib.redirect_stdout(buffer):
        core.build(db)

    lines = [
        line.strip()
        for line in buffer.getvalue().splitlines()
        if line.strip()
    ]

    if not lines:
        return {"status": "PASS"}

    try:
        return json.loads(lines[-1])
    except json.JSONDecodeError:
        return {"status": "PASS", "detail": lines[-1]}


class Engine:
    def __init__(self, refresh: bool = True):
        import torch
        from transformers import AutoModelForSequenceClassification, AutoTokenizer

        torch.set_num_threads(int(CONFIG.get("torch_threads", 2)))
        self.lock = threading.RLock()
        self.stop_event = threading.Event()
        self.auto_refresh = refresh

        core.verify_model()

        self.db = core.database()

        # Bring copied/stale checkout to exact current state before serving.
        if refresh:
            quiet_build(self.db)
        core.check(self.db)

        model_path = RUNTIME / "models" / "bge-reranker-v2-m3"

        self.tokenizer = AutoTokenizer.from_pretrained(
            model_path,
            local_files_only=True,
            trust_remote_code=False,
        )

        self.reranker = AutoModelForSequenceClassification.from_pretrained(
            model_path,
            local_files_only=True,
            trust_remote_code=False,
            use_safetensors=True,
        ).to("cpu").eval()

        self.records = []
        self.matrix = None
        self.lookup = {}

        self.reload_memory_index()
        self.signatures = dict(self.db.execute('SELECT path,sha FROM files'))
        self.keep_embedding_model_alive()


    def capture_signatures(self) -> dict[str, str]:
        signatures = {}

        for path in core.files():
            try:
                content = path.read_bytes()
            except FileNotFoundError:
                continue

            relative = path.relative_to(ROOT).as_posix()
            signatures[relative] = core.digest(content)

        return signatures

    def source_tree_changed(self) -> bool:
        return self.capture_signatures() != self.signatures

    def keep_embedding_model_alive(self):
        if CONFIG.get('ollama_keep_alive') == 0:
            return
        payload = {
            "model": CONFIG["embedding_model"],
            "input": ["Epidemic retrieval warmup"],
            "truncate": False,
            "keep_alive": CONFIG.get("ollama_keep_alive", -1),
            "options": {"num_ctx": 4096, "num_batch": CONFIG.get("ollama_num_batch", 64)},
        }

        result = core.api("/api/embed", payload)

        if "error" in result:
            raise RuntimeError(result["error"])

    def embed_query(self, text: str) -> np.ndarray:
        prompt = (
            "Instruct: Retrieve relevant C++ engine source code "
            "for the given query.\nQuery: " + text
        )

        result = core.api(
            "/api/embed",
            {
                "model": CONFIG["embedding_model"],
                "input": [prompt],
                "truncate": False,
                "keep_alive": CONFIG.get("ollama_keep_alive", -1),
                "options": {"num_ctx": 4096, "num_batch": CONFIG.get("ollama_num_batch", 64)},
            },
        )

        if "error" in result:
            raise RuntimeError(result["error"])

        values = np.asarray(result["embeddings"], dtype=np.float32)

        expected_dimensions = int(
            CONFIG.get("embedding_dimensions", core.EMBEDDING_DIMENSIONS)
        )

        if values.shape != (1, expected_dimensions):
            raise RuntimeError(
                f"Unexpected embedding shape: {values.shape}"
            )

        if not np.isfinite(values).all():
            raise RuntimeError("Non-finite query embedding")

        norm = np.linalg.norm(values, axis=1, keepdims=True)

        if (norm == 0).any():
            raise RuntimeError("Zero query embedding")

        return values[0] / norm[0]

    def reload_memory_index(self):
        records = self.db.execute(
            """
            SELECT id,path,start,end,text,vector
            FROM chunks
            ORDER BY path,start,end
            """
        ).fetchall()

        if not records:
            raise RuntimeError("Semantic index is empty")

        matrix = np.vstack(
            [
                np.frombuffer(record[5], dtype=np.float32)
                for record in records
            ]
        )

        self.records = records
        self.matrix = matrix
        self.lookup = {record[0]: record for record in records}

    def refresh_if_needed(self, force: bool = False) -> dict:
        with self.lock:
            changed = force or self.source_tree_changed()

            if not changed:
                return {
                    "status": "PASS",
                    "changed": False,
                    "files": len(self.signatures),
                    "chunks": len(self.records),
                }

            if not getattr(self, 'auto_refresh', True):
                raise RuntimeError('Semantic index changed during query; refresh it manually')

            result = quiet_build(self.db)

            # build() unloads Ollama after generating
            # source embeddings. Immediately restore persistent residency.
            self.keep_embedding_model_alive()

            core.check(self.db)
            self.reload_memory_index()
            # Remember indexed content, not a later filesystem snapshot that
            # could hide an edit made between admission and this assignment.
            self.signatures = dict(self.db.execute('SELECT path,sha FROM files'))

            return {
                "status": "PASS",
                "changed": True,
                "files": len(self.signatures),
                "chunks": len(self.records),
                "updated_files": result.get("updated_files", 0),
                "new_embeddings": result.get("new_embeddings", 0),
            }

    def rerank(self, query: str, records: list) -> list[float]:
        import torch

        scores: list[float] = []

        with torch.inference_mode():
            for start in range(0, len(records), 4):
                batch = records[start : start + 4]

                inputs = self.tokenizer(
                    [
                        (query, record[1] + "\n" + record[4])
                        for record in batch
                    ],
                    padding=True,
                    truncation=True,
                    max_length=512,
                    return_tensors="pt",
                )

                logits = self.reranker(**inputs).logits
                scores.extend(logits.view(-1).float().tolist())

        if len(scores) != len(records):
            raise RuntimeError("Reranker result count mismatch")

        if any(not math.isfinite(score) for score in scores):
            raise RuntimeError("Invalid reranker output")

        return scores

    def search(self, text: str, top: int | None = None) -> dict:
        with self.lock:
            # Persistent mode synchronizes changes; bounded one-query mode
            # rejects them for explicit manual index refresh.
            refresh = self.refresh_if_needed()

            requested_top = (
                int(top)
                if top is not None
                else int(CONFIG.get("default_top", 3))
            )

            requested_top = max(
                1,
                min(requested_top, int(CONFIG.get("max_top", 5))),
            )

            vector = self.embed_query(text)

            dense_indexes = np.argsort(
                -(self.matrix @ vector),
                kind="stable",
            )[: int(CONFIG["candidate_count"])]

            tokens = re.findall(
                r"[A-Za-z_][A-Za-z_0-9]*",
                text,
            )
            tokens += re.findall(
                r"[A-Z]?[a-z]+|[A-Z]+(?![a-z])",
                text,
            )

            tokens = list(dict.fromkeys(tokens))

            expression = " OR ".join(
                '"' + token.replace('"', '""') + '"'
                for token in tokens
            )

            if expression:
                lexical = self.db.execute(
                    """
                    SELECT rowid
                    FROM lexical
                    WHERE lexical MATCH ?
                    ORDER BY bm25(lexical), path
                    LIMIT ?
                    """,
                    (
                        expression,
                        int(CONFIG["candidate_count"]),
                    ),
                ).fetchall()
            else:
                lexical = []

            fused: dict[int, float] = {}

            ranked_sets = [
                [self.records[index][0] for index in dense_indexes],
                [row[0] for row in lexical],
            ]

            for ranked in ranked_sets:
                for rank, identity in enumerate(ranked):
                    fused[identity] = (
                        fused.get(identity, 0.0)
                        + 1.0 / (60 + rank + 1)
                    )

            candidate_ids = sorted(
                fused,
                key=lambda identity: (
                    -fused[identity],
                    self.lookup[identity][1:4],
                ),
            )[: int(CONFIG["rerank_count"])]

            candidates = [
                self.lookup[identity]
                for identity in candidate_ids
            ]

            scores = self.rerank(text, candidates)

            best = sorted(
                zip(candidates, scores),
                key=lambda pair: (
                    -pair[1],
                    pair[0][1:4],
                ),
            )[:requested_top]

            snippet_chars = int(
                CONFIG.get("snippet_chars", 1200)
            )

            return {
                "status": "PASS",
                "root": str(ROOT),
                "query": text,
                "embedding_model": CONFIG["embedding_model"],
                "reranker": CONFIG["reranker_model"],
                "fusion": "semantic+fts5+rrf+bge",
                "refresh": refresh,
                "results": [
                    {
                        "path": record[1],
                        "start": record[2],
                        "end": record[3],
                        "score": score,
                        "snippet": record[4][:snippet_chars],
                    }
                    for record, score in best
                ],
            }

    def health(self) -> dict:
        with self.lock:
            return {
                "status": "READY",
                "root": str(ROOT),
                "files": len(self.signatures),
                "chunks": len(self.records),
                "embedding_model": CONFIG["embedding_model"],
                "reranker": CONFIG["reranker_model"],
                "reranker_device": CONFIG["reranker_device"],
                "vector_matrix_mb": round(
                    self.matrix.nbytes / 1024 / 1024,
                    1,
                ),
                "dirty": self.source_tree_changed(),
            }


ENGINE: Engine | None = None


class Handler(BaseHTTPRequestHandler):
    server_version = "EpidemicRetrieval/1"

    def log_message(self, *_):
        # Prevent one line of HTTP noise per Codex query.
        pass

    def send_json(self, status: int, payload: dict):
        encoded = json.dumps(
            payload,
            ensure_ascii=False,
        ).encode("utf-8")

        self.send_response(status)
        self.send_header(
            "Content-Type",
            "application/json; charset=utf-8",
        )
        self.send_header(
            "Content-Length",
            str(len(encoded)),
        )
        self.end_headers()
        self.wfile.write(encoded)

    def read_json(self) -> dict:
        length = int(
            self.headers.get("Content-Length", "0")
        )
        raw = self.rfile.read(length)

        if not raw:
            return {}

        return json.loads(raw.decode("utf-8"))

    def do_GET(self):
        try:
            if self.path == "/health":
                self.send_json(200, ENGINE.health())
                return

            self.send_json(
                404,
                {"status": "ERROR", "error": "not found"},
            )
        except Exception as exc:
            self.send_json(
                500,
                {"status": "ERROR", "error": str(exc)},
            )

    def do_POST(self):
        try:
            payload = self.read_json()

            if self.path == "/search":
                text = str(payload.get("query", "")).strip()

                if not text:
                    raise ValueError("query is required")

                self.send_json(
                    200,
                    ENGINE.search(
                        text,
                        payload.get("top"),
                    ),
                )
                return

            if self.path == "/refresh":
                self.send_json(
                    200,
                    ENGINE.refresh_if_needed(force=True),
                )
                return

            self.send_json(
                404,
                {"status": "ERROR", "error": "not found"},
            )

        except Exception as exc:
            self.send_json(
                500,
                {
                    "status": "ERROR",
                    "error": str(exc),
                },
            )


def main():
    global ENGINE

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--query')
    parser.add_argument('--top', type=int, choices=range(1, 6), default=3)
    args = parser.parse_args()
    if args.query is not None:
        # Run-Quiet owns this single-query lifetime. Never bootstrap an index,
        # download models or repair corruption. Refresh only source-content drift.
        ready = core.readiness()
        refreshed = None
        if (ready['index']['status'] == 'STALE'
                and ready['dependencies']['status'] == 'AVAILABLE'
                and ready['embedding']['status'] == 'AVAILABLE'
                and ready['reranker']['status'] == 'LOCAL_FILES_VALIDATED'):
            with contextlib.closing(core.database()) as db:
                refreshed = quiet_build(db)
            ready = core.readiness()
        if ready['status'] != 'PREREQUISITES_READY':
            print(json.dumps(ready, ensure_ascii=False), flush=True)
            raise SystemExit(2)
        CONFIG['ollama_keep_alive'] = 0
        ENGINE = Engine(refresh=False)
        try:
            result = ENGINE.search(args.query, args.top)
            if refreshed is not None:
                result['refresh'] = {**refreshed, 'changed': True}
            print(json.dumps(result, ensure_ascii=False), flush=True)
        finally:
            ENGINE.stop_event.set()
            ENGINE.db.close()
        return

    ENGINE = Engine()

    server = ThreadingHTTPServer(
        (HOST, PORT),
        Handler,
    )

    print(
        json.dumps(
            {
                "status": "READY",
                "host": HOST,
                "port": PORT,
                **ENGINE.health(),
            },
            ensure_ascii=False,
        ),
        flush=True,
    )

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        ENGINE.stop_event.set()
        server.server_close()
        ENGINE.db.close()


if __name__ == "__main__":
    main()
