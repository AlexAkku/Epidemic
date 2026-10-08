from __future__ import annotations

import json
import sqlite3
import threading
import uuid
from copy import deepcopy
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable, Iterator

from ep_auditor.config import AuditorConfig
from ep_auditor.core.environment import capture_environment
from ep_auditor.core.evidence import EvidenceError, atomic_json, guard_sealed_raw
from ep_auditor.core.lifecycle import authoritative_run, finalize_run, initialize_run, mode_selection
from ep_auditor.core.evidence_index import EvidenceIndex
from ep_auditor.models import RunRecord


class RunStorage:
    def __init__(self, config: AuditorConfig) -> None:
        self.config = config
        self._lock = threading.RLock()
        self._counter = 0
        self._init_db()
        self.evidence_index = EvidenceIndex(self._connect, self.config.runs_dir)

    @contextmanager
    def _connect(self) -> Iterator[sqlite3.Connection]:
        conn = sqlite3.connect(self.config.database_path)
        conn.row_factory = sqlite3.Row
        try:
            with conn:
                yield conn
        finally:
            conn.close()

    def _init_db(self) -> None:
        with self._connect() as conn:
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS runs (
                    run_id TEXT PRIMARY KEY,
                    kind TEXT NOT NULL,
                    title TEXT NOT NULL,
                    capture_mode TEXT NOT NULL,
                    started_utc TEXT NOT NULL,
                    finished_utc TEXT,
                    status TEXT NOT NULL,
                    run_dir TEXT NOT NULL,
                    summary_json TEXT
                )
                """
            )
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS test_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    run_id TEXT NOT NULL,
                    test_name TEXT NOT NULL,
                    status TEXT NOT NULL,
                    wall_seconds REAL NOT NULL,
                    peak_rss_bytes INTEGER NOT NULL DEFAULT 0,
                    recorded_utc TEXT NOT NULL
                )
                """
            )
            conn.commit()

    def new_run(self, kind: str, title: str, capture_mode: str, *, target_scope: list[str] | None = None) -> RunRecord:
        with self._lock:
            now = datetime.now(timezone.utc)
            selection = mode_selection(capture_mode, target_scope if target_scope is not None else
                                       self.config.data.get("capture", {}).get("target_scope"))
            profile = selection["instrumentation_profile"]
            identity = uuid.uuid4()
            run_id = "RunId:" + str(identity)
            run_dir = self.config.runs_dir / identity.hex
            run_dir.mkdir(parents=True, exist_ok=False)
            record = RunRecord(
                run_id=run_id,
                kind=kind,
                title=title,
                capture_mode=capture_mode,
                run_dir=run_dir,
                started_utc=now.isoformat(),
            )
            record.metadata["mode_selection"] = selection
            if self.config.backend_operation is not None:
                record.metadata["backend_operation"] = deepcopy(self.config.backend_operation)
            if self.config.data.get("project_selection"):
                record.metadata["project_selection"] = deepcopy(self.config.data["project_selection"])
            initialize_run(record, capture_environment(self.config.for_mode(capture_mode), instrumentation_profile=profile))
            with self._connect() as conn:
                conn.execute(
                    "INSERT INTO runs(run_id,kind,title,capture_mode,started_utc,status,run_dir) VALUES(?,?,?,?,?,?,?)",
                    (run_id, kind, title, capture_mode, record.started_utc, record.status, str(run_dir)),
                )
                conn.commit()
            return record

    def finish_run(self, record: RunRecord, status: str, summary: dict[str, Any] | None = None) -> None:
        finalize_run(record, status, summary or {})
        for path in sorted((record.run_dir / 'metrics/exact').glob('*-functions.json')):
            self.evidence_index.index(record.run_dir, 'functions', 'metrics', 'exact/' + path.name)
        if (record.run_dir / 'metrics/coverage.json').is_file():
            self.evidence_index.index(record.run_dir, 'coverage_history', 'metrics', 'coverage.json')
        with self._connect() as conn:
            conn.execute(
                "UPDATE runs SET finished_utc=?, status=?, summary_json=? WHERE run_id=?",
                (record.finished_utc, record.status, json.dumps(summary or {}, ensure_ascii=False), record.run_id),
            )
            conn.commit()

    def add_test_history(self, run_id: str, test_name: str, status: str, wall_seconds: float, peak_rss_bytes: int) -> None:
        with self._connect() as conn:
            conn.execute(
                "INSERT INTO test_history(run_id,test_name,status,wall_seconds,peak_rss_bytes,recorded_utc) VALUES(?,?,?,?,?,?)",
                (run_id, test_name, status, wall_seconds, peak_rss_bytes, datetime.now(timezone.utc).isoformat()),
            )
            conn.commit()

    def list_runs(self, limit: int = 200) -> list[dict[str, Any]]:
        with self._connect() as conn:
            rows = conn.execute(
                "SELECT run_id,kind,title,capture_mode,started_utc,finished_utc,status,run_dir,summary_json FROM runs ORDER BY started_utc DESC LIMIT ?",
                (limit,),
            ).fetchall()
        result = []
        for row in rows:
            item = dict(row)
            self._refresh_status(item)
            if item.get("summary_json"):
                try:
                    item["summary"] = json.loads(item["summary_json"])
                except json.JSONDecodeError:
                    item["summary"] = {}
            result.append(item)
        return result

    def history_for_function(self, function_id: str) -> dict:
        """History from verified existing function facts, scoped to a binary-bound FunctionId."""
        samples, tests, seen = [], set(), set()
        for directory in sorted(self.config.runs_dir.iterdir()):
            if not (directory / 'run.json').is_file():
                continue
            header = json.loads((directory / 'run.json').read_text(encoding='utf-8'))
            if header.get('metadata', {}).get('finalization', {}).get('state') != 'committed':
                continue
            run = authoritative_run(directory)
            if run['kind'] == 'tests':
                continue  # Group/suite projections must not double-count child execution.
            for item in self.evidence_index.query(directory, 'functions'):
                row = item['fact']
                if not item['complete'] or row.get('functionId') != function_id:
                    continue
                capture_key = row['trace_sha256']
                if capture_key in seen:
                    continue
                seen.add(capture_key)
                samples.append({'run_id': run['run_id'], 'started_utc': run['started_utc'],
                    'calls': row['calls'], 'mean_ns': row['mean_ns'], 'p95_ns': row['p95_ns'],
                    'cpu_self_100ns': row['cpu_self_100ns'], 'evidence': {
                        'layer': item['layer'], 'path': item['artifact_path'], 'sha256': item['artifact_sha256']}})
                if run['kind'] == 'test':
                    tests.add(run['title'])
        samples.sort(key=lambda row: row['started_utc'])
        total = sum(row['calls'] for row in samples)
        return {'functionId': function_id, 'observed_runs': len({row['run_id'] for row in samples}),
            'observed_tests': len(tests), 'total_calls': total, 'history': samples,
            'once_candidate': total == 1,
            'performance_regression': (samples[-1]['mean_ns'] > 1.2 * samples[-2]['mean_ns']) if len(samples) >= 2 else None,
            'comparison_availability': 'observed' if len(samples) >= 2 else 'insufficient_samples',
            'comparison_policy': 'heuristic >20% mean increase for the same binary-bound function; not causal proof',
            'scope': 'verified complete captures only; absence is not proof of never executing outside these runs'}

    def history_for_test(self, test_id: str, compatibility_id: str) -> dict:
        from .evidence import RawEvidence
        from .test_observation import junit_result, observed_cancellation
        import hashlib
        results = []
        # Enumerate filesystem authority, then verify the disposable SQLite facts.
        for directory in sorted(self.config.runs_dir.iterdir()):
            if not (directory / "run.json").is_file():
                continue
            header = json.loads((directory / "run.json").read_text(encoding="utf-8"))
            if header.get("kind") != "test":
                continue
            run = authoritative_run(directory)
            if run.get("metadata", {}).get("finalization", {}).get("state") != "committed":
                raise EvidenceError("test history contains an interrupted uncommitted observation")
            facts = self.evidence_index.query(directory, "tests")
            if len(facts) != 1:
                raise EvidenceError("test history requires exactly one indexed result")
            for entry in facts:
                fact = entry["fact"]
                if fact.get("eventType") != "ctest.result":
                    continue
                raw = RawEvidence(directory)
                descriptor = raw.read_bytes("ctest/compatibility.json")
                build_id = "BuildId:" + hashlib.sha256(descriptor).hexdigest()
                if (fact["compatibility_id"] != build_id or fact["test_run_id"] != run["run_id"]
                        or fact["test_id"] != json.loads(descriptor)["test_id"]):
                    raise EvidenceError("test result identity disagrees with capture")
                if fact["test_id"] != test_id or build_id != compatibility_id:
                    continue
                lifecycle = json.loads(raw.read_bytes("process/lifecycle.json"))
                try:
                    observed, duration = junit_result(raw.read_bytes("ctest/junit.xml"), fact["test_name"],
                                                     lifecycle["exit_code"], observed_cancellation(lifecycle))
                except EvidenceError:
                    observed, duration = "INCOMPLETE", None
                if observed != fact["status"] or duration != fact["test_seconds"]:
                    raise EvidenceError("test result disagrees with original CTest observation")
                results.append({**fact, "status": run["status"]})
        passed = sum(row["status"] == "PASS" for row in results)
        failed = sum(row["status"] == "FAIL" for row in results)
        return {"results": results, "stability": {"passed": passed, "failed": failed,
                "incomplete": len(results) - passed - failed,
                "pass_rate": passed / (passed + failed) if passed + failed else None}}

    def get_run(self, run_id: str) -> dict[str, Any] | None:
        with self._connect() as conn:
            row = conn.execute("SELECT * FROM runs WHERE run_id=?", (run_id,)).fetchone()
        if row is None:
            return None
        item = dict(row)
        self._refresh_status(item)
        return item

    def _refresh_status(self, item: dict) -> None:
        try:
            if not Path(item["run_dir"]).resolve().is_relative_to(self.config.runs_dir.resolve()):
                raise EvidenceError("index run path outside configured runs directory")
            verified = authoritative_run(Path(item["run_dir"]))
            if verified["run_id"] != item["run_id"]:
                raise EvidenceError("index run identity mismatch")
            item["status"] = verified["status"]
        except (EvidenceError, OSError, ValueError, KeyError) as error:
            item["status"] = "INCOMPLETE"
            item["evidence_error"] = str(error)

    @staticmethod
    def write_json(path: Path, value: Any) -> None:
        atomic_json(path, value)

    @staticmethod
    def append_jsonl(path: Path, value: Any) -> None:
        guard_sealed_raw(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("a", encoding="utf-8", newline="\n") as f:
            f.write(json.dumps(value, ensure_ascii=False, separators=(",", ":")) + "\n")

    @staticmethod
    def read_json(path: Path, default: Any = None) -> Any:
        if not path.exists():
            return default
        return json.loads(path.read_text(encoding="utf-8"))
