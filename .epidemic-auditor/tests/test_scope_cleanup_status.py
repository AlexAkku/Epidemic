import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import uuid

from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
import test_core


class ScopeCleanupStatusTests(unittest.TestCase):
    def test_successful_root_with_owned_helper_cleanup_is_not_user_cancellation(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("scope_cleanup_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            storage = RunStorage(config)
            record = storage.new_run("fixture", "successful root with helper", "Fast")
            command = ("import subprocess,sys; subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'],"
                       "stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL); print('root completed')")
            result = ManagedProcessRunner(config).run([sys.executable, "-c", command], project, record.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            self.assertFalse(result.cancelled, "normal scope cleanup falsely cancelled a completed operation")
            lifecycle = json.loads((record.run_dir / "raw/process/lifecycle.json").read_bytes())
            self.assertEqual(lifecycle["termination"]["reason"], "root_exit_live_descendants")
            self.assertFalse(lifecycle["cancelled"])
            self.assertGreaterEqual(lifecycle["termination"]["active_processes_before"], 1)
            self.assertEqual(lifecycle["termination"]["purpose"], "scope_cleanup")
            storage.finish_run(record, "PASS", {})
            (base / "scope-cleanup-oracle.json").write_text(json.dumps({"run_dir": str(record.run_dir),
                "expected_exit": 0, "expected_cancelled": False, "root_output": "root completed"}))
