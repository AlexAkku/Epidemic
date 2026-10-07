import csv
import html
import json
import os
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.lineage import EvidenceLineage, normalized_record
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.exports import export_evidence, verify_export
from ep_auditor.core.evidence_index import DOMAINS
import test_core


class EvidenceIndexExportTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        base = Path(self.temporary.name)
        if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
            base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / (self._testMethodName + "_" + uuid.uuid4().hex)
            base.mkdir(parents=True)
        self.config, _ = test_core.AuditorCoreTests._config(self, base)
        self.storage = RunStorage(self.config)
        self.run = self.storage.new_run("fixture", "index and exports", "Fast")
        self.raw = RawEvidence(self.run.run_dir)
        self.raw.append("events.jsonl", b'{"value":7}\n', collector="fixture", version="1", start_ns=10, end_ns=20,
                        health={"capacity": 64, "observed": 1, "dropped": 0, "incomplete_reason": None})
        self.storage.finish_run(self.run, "PASS", {"fixture": True})
        self.lineage = EvidenceLineage(self.run.run_dir)
        reference = self.lineage.reference("raw", "events.jsonl")
        record = normalized_record({"schemaVersion": 1, "eventType": "fixture.observation", "required": True,
                                    "provenance": "external", "confidence": "observed", "value": 7,
                                    "label": '<script>x</script>|,"\n'}, {"fixture.observation"}, reference, 0)
        self.envelope = self.lineage.write("normalized", "fixture.json", producer_version="fixture-1",
                                           producer_source=Path(__file__), inputs=[reference], records=[record])

    def tearDown(self):
        self.temporary.cleanup()

    def test_sqlite_domains_and_raw_replay_after_db_delete(self):
        for domain in sorted(DOMAINS):
            self.assertEqual(self.storage.evidence_index.index(self.run.run_dir, domain, "normalized", "fixture.json"), 1)
            facts = self.storage.evidence_index.query(self.run.run_dir, domain)
            self.assertEqual(facts[0]["fact"], self.envelope["records"][0])
        before = digest(self.run.run_dir / "raw/events.jsonl")
        self.config.database_path.unlink()
        # Replay requires no SQLite connection, schema or index.
        self.assertEqual(RawEvidence(self.run.run_dir).read_bytes("events.jsonl"), b'{"value":7}\n')
        self.assertEqual(EvidenceLineage(self.run.run_dir).read_normalized("fixture.json", {"fixture.observation"}),
                         self.envelope["records"])
        rebuilt = RunStorage(self.config)
        with self.assertRaisesRegex(EvidenceError, "rebuild required"):
            rebuilt.evidence_index.query(self.run.run_dir, "tests")
        self.assertEqual(rebuilt.evidence_index.rebuild(), len(DOMAINS))
        self.assertEqual(rebuilt.get_run(self.run.run_id)["status"], "PASS")
        self.assertEqual(rebuilt.evidence_index.query(self.run.run_dir, "tests")[0]["fact"], self.envelope["records"][0])
        self.assertEqual(before, digest(self.run.run_dir / "raw/events.jsonl"))

    def test_sqlite_forgery_deletion_and_missing_raw_rejected(self):
        index = self.storage.evidence_index
        index.index(self.run.run_dir, "tests", "normalized", "fixture.json")
        with self.storage._connect() as connection:
            connection.execute("UPDATE evidence_facts SET fact_json='{}'")
        with self.assertRaisesRegex(EvidenceError, "disagrees"):
            index.query(self.run.run_dir, "tests")
        index.rebuild()
        with self.storage._connect() as connection:
            connection.execute("DELETE FROM evidence_facts")
        with self.assertRaisesRegex(EvidenceError, "rebuild required"):
            index.query(self.run.run_dir, "tests")
        index.rebuild()
        path = self.run.run_dir / "raw/events.jsonl"
        path.chmod(0o600)
        path.unlink()
        with self.assertRaises(EvidenceError):
            index.query(self.run.run_dir, "tests")
        self.assertEqual(self.storage.get_run(self.run.run_id)["status"], "INCOMPLETE")

    def test_four_formats_are_lossless_structured_exports(self):
        output = export_evidence(self.lineage, "normalized", "fixture.json", "fixture-1")
        manifest = verify_export(self.lineage, "fixture-1")
        self.assertEqual(len(manifest["files"]), 4)
        self.assertEqual(json.loads((output / "evidence.json").read_text()), self.envelope)
        jsonl = [json.loads(line) for line in (output / "evidence.jsonl").read_text().splitlines()]
        self.assertEqual(jsonl[0]["record"], self.envelope["records"][0])
        with (output / "evidence.csv").open(encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
        self.assertEqual(json.loads(rows[0]["record_json"]), self.envelope["records"][0])
        metadata = {key: value for key, value in self.envelope.items() if key != "records"}
        self.assertEqual(json.loads(rows[0]["metadata_json"]), metadata)
        markdown = (output / "evidence.md").read_text()
        self.assertNotIn("<script>", markdown)
        record_json = markdown.split("<pre>")[2].split("</pre>")[0]
        self.assertEqual(json.loads(html.unescape(record_json)), self.envelope["records"][0])

    def test_export_corruption_and_incomplete_manifest_rejected(self):
        output = export_evidence(self.lineage, "normalized", "fixture.json", "fixture-1")
        with self.assertRaises(EvidenceError):
            export_evidence(self.lineage, "normalized", "fixture.json", "fixture-1")
        (output / "evidence.csv").write_text("damaged")
        with self.assertRaisesRegex(EvidenceError, "hash mismatch"):
            verify_export(self.lineage, "fixture-1")
        (output / "manifest.json").unlink()
        with self.assertRaises(FileNotFoundError):
            verify_export(self.lineage, "fixture-1")

    def test_index_writer_collision_and_unknown_domain_rejected(self):
        lock = self.run.run_dir / "index_writer.lock"
        lock.write_text("active owner")
        with self.assertRaisesRegex(EvidenceError, "writer active or interrupted"):
            self.storage.evidence_index.index(self.run.run_dir, "tests", "normalized", "fixture.json")
        lock.unlink()
        with self.assertRaisesRegex(EvidenceError, "unknown evidence index domain"):
            self.storage.evidence_index.index(self.run.run_dir, "fake", "normalized", "fixture.json")
