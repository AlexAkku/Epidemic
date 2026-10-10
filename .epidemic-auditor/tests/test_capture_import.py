import hashlib
from pathlib import Path
import tempfile
import unittest

from ep_auditor.core.evidence import RawEvidence, EvidenceError, load_json


class CaptureImportTests(unittest.TestCase):
    def test_streamed_capture_exact_hash_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = directory / "trace.etl"
            payload = b"captured original trace" * 100000
            source.write_bytes(payload)
            raw = RawEvidence(directory / "run")
            sha = hashlib.sha256(payload).hexdigest()
            arguments = {"collector": "controlled_import", "version": "1", "start_ns": 1, "end_ns": 2,
                         "health": {"capacity": len(payload), "observed": len(payload), "dropped": 0, "incomplete_reason": None}}
            row = raw.import_file("etw/trace.etl", source, sha, **arguments)
            self.assertEqual(row["sha256"], sha)
            self.assertEqual((raw.raw / "etw/trace.etl").read_bytes(), payload)
            self.assertEqual(raw.read_capture("etw/trace.etl")[0], payload)
            with self.assertRaises(EvidenceError):
                raw.import_file("etw/trace.etl", source, sha, **arguments)
            self.assertEqual(load_json(raw.catalog_path)["files"]["etw/trace.etl"]["size"], len(payload))

    def test_wrong_hash_capacity_and_counts_leave_no_fake_catalog_entry(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = directory / "source"
            source.write_bytes(b"observed bytes")
            for variant in ("hash", "capacity", "count", "unknown_capacity", "unknown_count"):
                with self.subTest(variant=variant):
                    raw = RawEvidence(directory / variant)
                    health = {"capacity": 14 if variant != "capacity" else 1,
                              "observed": 14 if variant != "count" else 13, "dropped": 0, "incomplete_reason": None}
                    if variant == "unknown_capacity":
                        health["capacity"] = None
                    if variant == "unknown_count":
                        health["observed"] = None
                    with self.assertRaises(EvidenceError):
                        raw.import_file("etl/trace", source, "0" * 64 if variant == "hash" else hashlib.sha256(source.read_bytes()).hexdigest(),
                                        collector="fixture", version="1", start_ns=1, end_ns=2, health=health)
                    self.assertFalse((raw.raw / "etl/trace").exists())
                    self.assertFalse(raw.catalog_path.exists())
