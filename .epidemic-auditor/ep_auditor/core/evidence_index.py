"""Disposable SQLite indexing of independently verifiable filesystem evidence."""
import hashlib
import json
import os
from pathlib import Path

from .evidence import EvidenceError, atomic_json, load_json
from .lifecycle import authoritative_run
from .lineage import EvidenceLineage

DOMAINS = {"builds", "tests", "functions", "modules", "traces", "findings", "performance_history",
           "coverage_history", "exceptions", "waits_locks", "api_usage"}


class EvidenceIndex:
    def __init__(self, connect, runs_dir: Path):
        self.connect = connect
        self.runs_dir = runs_dir
        with connect() as connection:
            connection.execute("""CREATE TABLE IF NOT EXISTS evidence_facts (
                fact_id TEXT PRIMARY KEY, run_id TEXT NOT NULL, domain TEXT NOT NULL,
                layer TEXT NOT NULL, artifact_path TEXT NOT NULL, artifact_sha256 TEXT NOT NULL,
                ordinal INTEGER NOT NULL, producer_version TEXT NOT NULL, complete INTEGER NOT NULL,
                fact_json TEXT NOT NULL)""")
            connection.execute("CREATE INDEX IF NOT EXISTS evidence_domain_run ON evidence_facts(domain, run_id)")

    def _run(self, run_dir: Path) -> dict:
        if not run_dir.resolve().is_relative_to(self.runs_dir.resolve()):
            raise EvidenceError("evidence index path outside runs directory")
        return authoritative_run(run_dir)

    def index(self, run_dir: Path, domain: str, layer: str, path: str, *, record_catalog=True) -> int:
        self._run(run_dir)
        lock = run_dir / "index_writer.lock"
        try:
            descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        except FileExistsError as error:
            raise EvidenceError("evidence index writer active or interrupted") from error
        try:
            os.close(descriptor)
            return self._index(run_dir, domain, layer, path, record_catalog=record_catalog)
        finally:
            lock.unlink()

    def _index(self, run_dir: Path, domain: str, layer: str, path: str, *, record_catalog: bool) -> int:
        if domain not in DOMAINS:
            raise EvidenceError("unknown evidence index domain")
        run = self._run(run_dir)
        lineage = EvidenceLineage(run_dir)
        reference = lineage.reference(layer, path)
        envelope = lineage.verify(layer, path)
        facts = []
        for ordinal, record in enumerate(envelope["records"]):
            identity = json.dumps([run["run_id"], domain, reference, ordinal], sort_keys=True).encode()
            facts.append(("FactId:" + hashlib.sha256(identity).hexdigest(), run["run_id"], domain, layer, path,
                          reference["sha256"], ordinal, envelope["producerVersion"],
                          int(envelope["completeness"]["complete"]), json.dumps(record, ensure_ascii=False, allow_nan=False)))
        if reference != lineage.reference(layer, path):
            raise EvidenceError("index input changed during read")
        if record_catalog:
            catalog_path = run_dir / "index_catalog.json"
            catalog = load_json(catalog_path) if catalog_path.is_file() else {"schemaVersion": 1, "artifacts": []}
            if catalog.get("schemaVersion") != 1 or not isinstance(catalog.get("artifacts"), list):
                raise EvidenceError("unsupported index catalog")
            entry = {"domain": domain, "reference": reference}
            if entry not in catalog["artifacts"]:
                catalog["artifacts"].append(entry)
            # Filesystem authority is persisted before the disposable cache update.
            atomic_json(catalog_path, catalog)
        with self.connect() as connection:
            connection.execute("DELETE FROM evidence_facts WHERE run_id=? AND domain=? AND layer=? AND artifact_path=?",
                               (run["run_id"], domain, layer, path))
            connection.executemany("INSERT INTO evidence_facts VALUES(?,?,?,?,?,?,?,?,?,?)", facts)
        return len(facts)

    def query(self, run_dir: Path, domain: str) -> list[dict]:
        if domain not in DOMAINS:
            raise EvidenceError("unknown evidence index domain")
        run = self._run(run_dir)
        with self.connect() as connection:
            rows = connection.execute("SELECT * FROM evidence_facts WHERE run_id=? AND domain=? ORDER BY fact_id",
                                      (run["run_id"], domain)).fetchall()
        result = []
        lineage = EvidenceLineage(run_dir)
        catalog_path = run_dir / "index_catalog.json"
        expected_ids = set()
        if catalog_path.is_file():
            catalog = load_json(catalog_path)
            if catalog.get("schemaVersion") != 1:
                raise EvidenceError("unsupported index catalog")
            for entry in catalog["artifacts"]:
                if entry["domain"] != domain:
                    continue
                ref = entry["reference"]
                if ref != lineage.reference(ref["layer"], ref["path"]):
                    raise EvidenceError("index catalog input binding mismatch")
                envelope = lineage.verify(ref["layer"], ref["path"])
                for ordinal in range(len(envelope["records"])):
                    identity = json.dumps([run["run_id"], domain, ref, ordinal], sort_keys=True).encode()
                    expected_ids.add("FactId:" + hashlib.sha256(identity).hexdigest())
        if expected_ids != {row["fact_id"] for row in rows}:
            raise EvidenceError("SQLite index missing or unexpected facts; rebuild required")
        for row in rows:
            item = dict(row)
            reference = lineage.reference(item["layer"], item["artifact_path"])
            envelope = lineage.verify(item["layer"], item["artifact_path"])
            if (reference["sha256"] != item["artifact_sha256"]
                    or item["ordinal"] < 0 or item["ordinal"] >= len(envelope["records"])
                    or json.loads(item["fact_json"]) != envelope["records"][item["ordinal"]]
                    or item["producer_version"] != envelope["producerVersion"]
                    or item["complete"] != int(envelope["completeness"]["complete"])):
                raise EvidenceError("SQLite fact disagrees with independently verified artifact")
            item["fact"] = json.loads(item.pop("fact_json"))
            result.append(item)
        return result

    def rebuild(self) -> int:
        total = 0
        for directory in sorted(self.runs_dir.iterdir()):
            if not directory.is_dir() or not (directory / "run.json").is_file():
                continue
            run = self._run(directory)
            with self.connect() as connection:
                connection.execute("INSERT OR REPLACE INTO runs VALUES(?,?,?,?,?,?,?,?,?)",
                                   (run["run_id"], run["kind"], run["title"], run["capture_mode"],
                                    run["started_utc"], run.get("finished_utc"), run["status"], str(directory),
                                    json.dumps(run.get("metadata", {}).get("finalization", {}).get("summary", {}))))
            catalog_path = directory / "index_catalog.json"
            if not catalog_path.is_file():
                continue
            catalog = load_json(catalog_path)
            if catalog.get("schemaVersion") != 1:
                raise EvidenceError("unsupported index catalog")
            for entry in catalog["artifacts"]:
                ref = entry["reference"]
                if ref != EvidenceLineage(directory).reference(ref["layer"], ref["path"]):
                    raise EvidenceError("rebuild input binding mismatch")
                total += self.index(directory, entry["domain"], ref["layer"], ref["path"], record_catalog=False)
        return total
