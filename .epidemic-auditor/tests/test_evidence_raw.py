import sys
import tempfile
import unittest
import uuid
import json
import tracemalloc
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, RawEvidence, atomic_json, digest
from ep_auditor.core.storage import RunStorage


class RawEvidenceTests(unittest.TestCase):
    def test_large_atomic_json_keeps_canonical_bytes_and_bounded_serialization_memory(self):
        value = {"records": [{"index": index, "payload": "é/raw;" * 40} for index in range(50000)]}
        expected = (json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False) + "\n").encode()
        path = self.root / "derived.json"
        tracemalloc.start()
        try:
            atomic_json(path, value)
            _, peak = tracemalloc.get_traced_memory()
        finally:
            tracemalloc.stop()
        self.assertEqual(path.read_bytes(), expected)
        self.assertLess(peak, 4 * 1024 * 1024, "serialization must not duplicate the complete evidence payload")
        with self.assertRaises(ValueError):
            atomic_json(path, {"records": ["prefix", float("nan")]})
        self.assertEqual(path.read_bytes(), expected)
        self.assertEqual(list(self.root.glob("derived.json.*.tmp")), [])

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.capture = RawEvidence(self.root)
        self.health = {"capacity": 64, "observed": 1, "dropped": 0, "incomplete_reason": None}
        self.context = {"run_id": "RunId:" + str(uuid.uuid4()), "instrumentation_profile": "audit",
                        "source_fingerprints": {"fixture": digest(Path(__file__))},
                        "binary_fingerprints": {"fixture_interpreter": digest(Path(sys.executable))},
                        "analyzer_version": "fixture-1"}

    def tearDown(self):
        self.temporary.cleanup()

    def append(self, data=b"known", **overrides):
        self.capture.append("collector/events.bin", data, collector="controlled_fixture", version="1",
                            start_ns=10, end_ns=20, health={**self.health, **overrides})

    def test_manifest_complete_and_read_only_reanalysis_hashes(self):
        self.append()
        manifest = self.capture.seal(self.context)
        row = manifest["raw_files"][0]
        self.assertTrue(manifest["capture_complete"])
        self.assertEqual((row["size"], row["collector"], row["collector_version"], row["start_ns"], row["end_ns"]),
                         (5, "controlled_fixture", "1", 10, 20))
        before = {path.relative_to(self.root).as_posix(): digest(path) for path in self.capture.raw.rglob("*") if path.is_file()}
        self.assertEqual(self.capture.read_bytes("collector/events.bin"), b"known")
        self.assertEqual(RawEvidence(self.root).read_bytes("collector/events.bin"), b"known")
        self.assertEqual(before, {path.relative_to(self.root).as_posix(): digest(path)
                                for path in self.capture.raw.rglob("*") if path.is_file()})
        with self.assertRaises(EvidenceError):
            self.append(b"forbidden")

    def test_one_byte_corruption_same_size_rejected(self):
        self.append()
        self.capture.seal(self.context)
        path = self.capture.raw / "collector/events.bin"
        path.chmod(0o600)
        path.write_bytes(b"Known")
        with self.assertRaises(EvidenceError):
            self.capture.verify()

    def test_generic_writers_cannot_add_to_sealed_raw(self):
        self.append()
        self.capture.seal(self.context)
        for writer in (RunStorage.write_json, RunStorage.append_jsonl, atomic_json):
            with self.subTest(writer=writer.__name__), self.assertRaises(EvidenceError):
                writer(self.capture.raw / "unregistered.json", {"invalid": "post-seal-write"})

    def test_required_raw_deleted_and_unregistered_file_rejected(self):
        self.append()
        self.capture.seal(self.context)
        path = self.capture.raw / "collector/events.bin"
        path.chmod(0o600)
        path.unlink()
        with self.assertRaises(EvidenceError):
            self.capture.verify()
        path.write_bytes(b"known")
        (self.capture.raw / "unregistered.bin").write_bytes(b"surprise")
        with self.assertRaises(EvidenceError):
            self.capture.verify()

    def test_dropped_unknown_and_zero_capacity_cannot_claim_complete(self):
        for changes in ({"dropped": 1, "complete": True}, {"observed": None}, {"capacity": 0}):
            with self.subTest(changes=changes), tempfile.TemporaryDirectory() as directory:
                capture = RawEvidence(Path(directory))
                capture.append("events.bin", b"known", collector="fixture", version="1", start_ns=1, end_ns=2,
                               health={**self.health, **changes})
                manifest = capture.seal(self.context)
                self.assertFalse(manifest["capture_complete"])
                self.assertTrue(manifest["raw_files"][0]["health"]["incomplete_reason"])

    def test_path_escape_and_capture_identity_changes_rejected(self):
        for name in ("../outside", "/absolute", "C:/absolute", "a\\b", "a/./b"):
            with self.subTest(name=name), self.assertRaises(EvidenceError):
                self.capture.append(name, b"x", collector="fixture", version="1", start_ns=1, end_ns=2, health=self.health)
        self.append()
        with self.assertRaises(EvidenceError):
            self.capture.append("collector/events.bin", b"x", collector="other", version="2",
                                start_ns=10, end_ns=30, health=self.health)

    def test_interrupted_or_concurrent_writer_rejected(self):
        (self.root / "capture_writer.lock").write_text("interrupted owner")
        with self.assertRaises(EvidenceError):
            self.append()
        self.assertFalse(self.capture.manifest_path.exists())

    def test_manifest_missing_collector_and_invalid_profile_rejected(self):
        self.append()
        with self.assertRaises(EvidenceError):
            self.capture.seal({**self.context, "instrumentation_profile": "normal-debug"})
        manifest = self.capture.seal(self.context)
        self.capture.manifest_path.chmod(0o600)
        del manifest["raw_files"][0]["collector_version"]
        atomic_json(self.capture.manifest_path, manifest)
        with self.assertRaises(EvidenceError):
            self.capture.verify()
