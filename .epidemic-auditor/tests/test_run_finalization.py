import tempfile
import os
import subprocess
import sys
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.evidence import RawEvidence, load_json
from ep_auditor.core.lifecycle import RUN_DIRECTORIES, authoritative_run, profile_for_mode
from ep_auditor.config import load_config
from ep_auditor.core.storage import RunStorage
import test_core


class RunFinalizationTests(unittest.TestCase):
    def test_all_admitted_modes_preserve_profile_contract(self):
        expected = {"Fast": "audit", "Full": "audit", "Deep": "audit-deep", "Targeted Exact": "audit-deep",
                    "Memory": "audit-memory", "Coverage": "audit-coverage", "Stress": "audit-deep"}
        for mode, profile in expected.items():
            self.assertEqual(profile_for_mode(mode), profile)

    def test_missing_capture_never_becomes_clean_pass(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "no capture", "Fast")
            storage.finish_run(record, "PASS", {"claimed": "success"})
            self.assertEqual(storage.get_run(record.run_id)["status"], "INCOMPLETE")
            self.assertEqual(load_json(record.run_dir / "run.json")["status"], "INCOMPLETE")

    def test_new_run_is_incomplete_until_commit(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "interrupted", "Fast")
            self.assertEqual(load_json(record.run_dir / "run.json")["status"], "INCOMPLETE")
            self.assertEqual(storage.get_run(record.run_id)["status"], "INCOMPLETE")

    def test_complete_capture_uuid_layout_and_corruption(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "complete", "Fast")
            self.assertEqual(record.run_id, "RunId:" + str(uuid.UUID(record.run_id.removeprefix("RunId:"))))
            for name in RUN_DIRECTORIES:
                self.assertTrue((record.run_dir / name).is_dir())
            RawEvidence(record.run_dir).append("events.bin", b"observed", collector="fixture", version="1",
                                              start_ns=10, end_ns=20,
                                              health={"capacity": 64, "observed": 1, "dropped": 0, "incomplete_reason": None})
            storage.finish_run(record, "PASS", {"count": 1})
            self.assertEqual(authoritative_run(record.run_dir)["status"], "PASS")
            raw = record.run_dir / "raw/events.bin"
            raw.chmod(0o600)
            raw.write_bytes(b"Observed")
            indexed = storage.get_run(record.run_id)
            self.assertEqual(indexed["status"], "INCOMPLETE")
            self.assertTrue(indexed["evidence_error"])

    def test_actual_crash_before_commit_remains_incomplete(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("crash_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            child = Path(__file__).parent / "fixtures/crash_finalization.py"
            process = subprocess.run([sys.executable, str(child), str(base)], capture_output=True, timeout=90)
            self.assertEqual(process.returncode, 73, process.stderr.decode(errors="replace"))
            identity = load_json(base / "child_identity.json")
            effect = load_json(base / "fault_effect.json")
            self.assertEqual(effect, {"point": "before_atomic_run_commit", "candidate_status": "PASS"})
            run_dir = Path(identity["run_dir"])
            self.assertTrue((run_dir / "manifest.json").is_file())
            self.assertTrue((run_dir / "run_commit.lock").is_file())
            self.assertEqual(authoritative_run(run_dir)["status"], "INCOMPLETE")
            storage = RunStorage(load_config(Path(identity["config"])))
            self.assertEqual(storage.get_run(identity["run_id"])["status"], "INCOMPLETE")
