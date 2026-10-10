import sys
import os
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, RawEvidence
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
import test_core


class ProcessRawCaptureTests(unittest.TestCase):
    def test_original_stdio_bytes_are_registered_without_decode_or_newline_loss(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("process_bytes_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            storage = RunStorage(config)
            record = storage.new_run("fixture", "binary stdio", "Fast")
            command = [sys.executable, "-c", "import sys; sys.stdout.buffer.write(bytes([0,255,13,10])); sys.stderr.buffer.write(bytes([254,13,10]))"]
            result = ManagedProcessRunner(config).run(command, project, record.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            self.assertEqual((record.run_dir / "stdout.log").read_bytes(), bytes([0, 255, 13, 10]))
            storage.finish_run(record, "PASS", {"fixture": "original-bytes"})
            self.assertEqual(RawEvidence(record.run_dir).read_bytes("process/stdout.bin"), bytes([0, 255, 13, 10]))
            self.assertEqual(RawEvidence(record.run_dir).read_bytes("process/stderr.bin"), bytes([254, 13, 10]))

    def test_over_capacity_long_line_records_exact_loss_and_incomplete(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            config.data["capture"]["stdio_raw_capacity_bytes"] = 32
            storage = RunStorage(config)
            record = storage.new_run("fixture", "overflow", "Fast")
            command = [sys.executable, "-c", "import sys; sys.stdout.buffer.write(b'x'*(2*1024*1024))"]
            result = ManagedProcessRunner(config).run(command, project, record.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            self.assertEqual(result.stdout_lines, 1)
            storage.finish_run(record, "PASS", {"fixture": "overflow"})
            self.assertEqual(storage.get_run(record.run_id)["status"], "INCOMPLETE")
            capture = RawEvidence(record.run_dir)
            self.assertEqual(capture.read_bytes("process/stdout.bin"), b"x" * 32)
            row = next(row for row in capture.verify()["raw_files"] if row["path"] == "raw/process/stdout.bin")
            self.assertEqual(row["health"]["capacity"], 32)
            self.assertEqual(row["health"]["observed"], 2 * 1024 * 1024)
            self.assertEqual(row["health"]["dropped"], 2 * 1024 * 1024 - 32)
            self.assertFalse(row["complete"])

    def test_invalid_capacity_never_launches_and_empty_stdio_is_observed(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "empty", "Fast")
            config.data["capture"]["stdio_raw_capacity_bytes"] = 0
            with self.assertRaises(EvidenceError):
                ManagedProcessRunner(config).run([sys.executable, "-c", "pass"], project, record.run_dir, "Fast")
            config.data["capture"]["stdio_raw_capacity_bytes"] = 32
            result = ManagedProcessRunner(config).run([sys.executable, "-c", "pass"], project, record.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            self.assertEqual(result.stdout_lines, 0)
            storage.finish_run(record, "PASS", {})
            capture = RawEvidence(record.run_dir)
            self.assertEqual(capture.read_bytes("process/stdout.bin"), b"")
            self.assertEqual(capture.read_bytes("process/stderr.bin"), b"")
