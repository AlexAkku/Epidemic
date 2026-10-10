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

    def history_for_function(self, function_id: str, *, cursor=None, limit=200) -> dict:
        """History from verified existing function facts, scoped to a binary-bound FunctionId."""
        import base64
        import hashlib
        from .evidence import digest, load_json
        if not isinstance(function_id, str) or not function_id or len(function_id) > 512:
            raise EvidenceError('history requires a bounded FunctionId')
        if type(limit) is not int or not 1 <= limit <= 200:
            raise EvidenceError('history page limit must be between 1 and 200')
        offset, expected_snapshot = 0, None
        if cursor is not None:
            try:
                if not isinstance(cursor, str) or len(cursor) > 4096:
                    raise ValueError('cursor capacity exceeded')
                token = json.loads(base64.urlsafe_b64decode(cursor))
                if (token['function_id'] != function_id or token['limit'] != limit or
                        type(token['offset']) is not int or token['offset'] < 0):
                    raise ValueError('history scope/bounds changed')
                offset, expected_snapshot = token['offset'], token['snapshot']
            except (ValueError, TypeError, KeyError, UnicodeError) as error:
                raise EvidenceError('invalid history continuation') from error
        samples, tests, seen, runs, latest = [], set(), set(), set(), []
        total, count = 0, 0
        signature = hashlib.sha256()
        directories = []
        for directory in sorted(self.config.runs_dir.iterdir()):
            if not (directory / 'run.json').is_file():
                continue
            header = json.loads((directory / 'run.json').read_text(encoding='utf-8'))
            if header.get('metadata', {}).get('finalization', {}).get('state') != 'committed':
                continue
            run = authoritative_run(directory)
            if run['kind'] == 'tests':
                continue  # Group/suite projections must not double-count child execution.
            directories.append((run['started_utc'], run['run_id'], directory, run))
        for _, _, directory, run in sorted(directories):
            signature.update(json.dumps([run['run_id'], digest(directory / 'manifest.json')]).encode())
            environment = load_json(directory / 'environment.json')
            compatibility = {key: environment.get(key) for key in
                ('schemaVersion', 'instrumentation_profile', 'capture_configuration', 'metric_units',
                 'hostname', 'machine', 'platform', 'processor', 'logical_cpu_count', 'build_config', 'compiler_toolchain')}
            entries = self.evidence_index.query(directory, 'functions')
            if entries:
                from ep_auditor.collectors.exact_trace import publish_capture
                proof = publish_capture(directory, verify_only=True)
                if any(proof.get(item['layer'] + '/' + item['artifact_path']) != item['artifact_sha256']
                       for item in entries):
                    raise EvidenceError('function history differs from replay of original capture')
            for item in entries:
                row = item['fact']
                if not item['complete'] or row.get('functionId') != function_id:
                    continue
                capture_key = row['trace_sha256']
                if capture_key in seen:
                    continue
                seen.add(capture_key)
                sample = {'run_id': run['run_id'], 'started_utc': run['started_utc'],
                    'calls': row['calls'], 'mean_ns': row['mean_ns'], 'p95_ns': row['p95_ns'],
                    'cpu_self_100ns': row['cpu_self_100ns'], 'evidence': {
                        'layer': item['layer'], 'path': item['artifact_path'], 'sha256': item['artifact_sha256']},
                    'compatibility': compatibility, 'provenance': 'compiler_instrumentation'}
                previous = latest[-1] if latest else None
                comparable = (previous is not None and previous['compatibility'] == compatibility and
                    all(value is not None for value in compatibility.values()) and
                    compatibility['compiler_toolchain'].get('availability') == 'observed')
                sample['regression'] = {
                    'baseline_run_id': previous['run_id'] if previous else None,
                    'baseline_evidence': previous['evidence'] if previous else None,
                    'delta_ns': sample['mean_ns'] - previous['mean_ns'] if comparable else None,
                    'candidate': sample['mean_ns'] > 1.2 * previous['mean_ns'] if comparable else None,
                    'availability': 'observed' if comparable else 'UNKNOWN',
                    'confidence': 'heuristic', 'policy': '>20% mean increase; not causal proof'}
                if offset <= count < offset + limit:
                    samples.append(sample)
                count += 1
                total += row['calls']
                runs.add(run['run_id'])
                latest = (latest + [sample])[-2:]
                if run['kind'] == 'test':
                    tests.add(run['title'])
        snapshot = signature.hexdigest()
        if expected_snapshot is not None and expected_snapshot != snapshot:
            raise EvidenceError('history snapshot changed; restart the query')
        compatible = (len(latest) >= 2 and latest[0]['compatibility'] == latest[1]['compatibility'] and
            all(value is not None for value in latest[0]['compatibility'].values()) and
            latest[0]['compatibility']['compiler_toolchain'].get('availability') == 'observed')
        continuation = base64.urlsafe_b64encode(json.dumps({'function_id': function_id,
            'limit': limit, 'offset': offset + limit, 'snapshot': snapshot}).encode()).decode() if offset + limit < count else None
        return {'functionId': function_id, 'observed_runs': len(runs),
            'observed_tests': len(tests), 'total_calls': total, 'history': samples,
            'items': samples, 'snapshot': snapshot, 'continuation': continuation, 'truncated': continuation is not None,
            'availability': 'observed' if count else 'UNKNOWN', 'complete': count > 0,
            'once_candidate': total == 1,
            'performance_regression': (latest[-1]['mean_ns'] > 1.2 * latest[-2]['mean_ns']) if compatible else None,
            'comparison_availability': 'observed' if compatible else 'incompatible' if len(latest) >= 2 else 'insufficient_samples',
            'comparison_policy': 'heuristic >20% mean increase for the same binary-bound function; not causal proof',
            'scope': 'verified complete captures only; absence is not proof of never executing outside these runs'}

    def history_for_regression(self, function_id: str, *, cursor=None, limit=200) -> dict:
        """Measured candidate history, with each comparison bound to both captures."""
        result = self.history_for_function(function_id, cursor=cursor, limit=limit)
        result['history_kind'] = 'regression'
        return result

    def history_for_test(self, test_id: str, compatibility_id: str | None = None, *, cursor=None, limit=200) -> dict:
        from .evidence import RawEvidence
        from .test_observation import junit_result, observed_cancellation
        from .evidence import digest
        import base64
        import hashlib
        if not isinstance(test_id, str) or not test_id or len(test_id) > 512:
            raise EvidenceError('history requires a bounded TestId')
        if type(limit) is not int or not 1 <= limit <= 200:
            raise EvidenceError('history page limit must be between 1 and 200')
        offset, expected_snapshot = 0, None
        if cursor is not None:
            try:
                if not isinstance(cursor, str) or len(cursor) > 4096:
                    raise ValueError('cursor capacity exceeded')
                token = json.loads(base64.urlsafe_b64decode(cursor))
                if (token['test_id'] != test_id or token['compatibility_id'] != compatibility_id or
                        token['limit'] != limit or type(token['offset']) is not int or token['offset'] < 0):
                    raise ValueError('history scope/bounds changed')
                offset, expected_snapshot = token['offset'], token['snapshot']
            except (ValueError, TypeError, KeyError, UnicodeError) as error:
                raise EvidenceError('invalid history continuation') from error
        results = []
        count, passed, failed = 0, 0, 0
        signature = hashlib.sha256()
        # Enumerate filesystem authority, then verify the disposable SQLite facts.
        directories = []
        for directory in self.config.runs_dir.iterdir():
            if not (directory / "run.json").is_file():
                continue
            header = json.loads((directory / "run.json").read_text(encoding="utf-8"))
            if header.get("kind") != "test":
                continue
            directories.append((header['started_utc'], header['run_id'], directory))
        for _, _, directory in sorted(directories):
            run = authoritative_run(directory)
            if run.get("metadata", {}).get("finalization", {}).get("state") != "committed":
                raise EvidenceError("test history contains an interrupted uncommitted observation")
            signature.update(json.dumps([run['run_id'], digest(directory / 'manifest.json')]).encode())
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
                if fact["test_id"] != test_id or (compatibility_id is not None and build_id != compatibility_id):
                    continue
                lifecycle = json.loads(raw.read_bytes("process/lifecycle.json"))
                try:
                    observed, duration = junit_result(raw.read_bytes("ctest/junit.xml"), fact["test_name"],
                                                     lifecycle["exit_code"], observed_cancellation(lifecycle))
                except EvidenceError:
                    observed, duration = "INCOMPLETE", None
                if observed != fact["status"] or duration != fact["test_seconds"]:
                    raise EvidenceError("test result disagrees with original CTest observation")
                if offset <= count < offset + limit:
                    results.append({**fact, 'run_id': run['run_id'], 'started_utc': run['started_utc'],
                        'wall_seconds': duration, 'provenance': 'external_ctest_observation',
                        'evidence': {'path': 'raw/ctest/junit.xml', 'sha256': digest(directory / 'raw/ctest/junit.xml')}
                            if duration is not None else None,
                        "status": run["status"]})
                count += 1
                passed += run['status'] == 'PASS'
                failed += run['status'] == 'FAIL'
        snapshot = signature.hexdigest()
        if expected_snapshot is not None and expected_snapshot != snapshot:
            raise EvidenceError('history snapshot changed; restart the query')
        continuation = base64.urlsafe_b64encode(json.dumps({'test_id': test_id,
            'compatibility_id': compatibility_id, 'limit': limit, 'offset': offset + limit,
            'snapshot': snapshot}).encode()).decode() if offset + limit < count else None
        return {'test_id': test_id, 'results': results, 'items': results, 'history_kind': 'test',
                'snapshot': snapshot, 'continuation': continuation, 'truncated': continuation is not None,
                'availability': 'observed' if count else 'UNKNOWN', 'complete': count > 0 and count == passed + failed,
                'comparison_availability': 'not_compared',
                "stability": {"passed": passed, "failed": failed,
                "incomplete": count - passed - failed,
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
