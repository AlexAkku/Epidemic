import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import uuid

import psutil
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
import test_core


class StopCompletionTests(unittest.TestCase):
    def test_root_exit_live_detached_stdio_child_has_termination_provenance(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("stop_completion_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            record = RunStorage(config).new_run("fixture", "root exits with live child", "Fast")
            script = ("import subprocess,sys; p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'],"
                      "stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL); print(p.pid,flush=True)")
            result = ManagedProcessRunner(config).run([sys.executable, "-c", script], project, record.run_dir, "Fast")
            child_pid = int((record.run_dir / "raw/process/stdout.bin").read_bytes())
            self.assertFalse(psutil.pid_exists(child_pid), "managed child survived completed scope")
            lifecycle = json.loads((record.run_dir / "raw/process/lifecycle.json").read_bytes())
            self.assertIsNotNone(lifecycle["termination"], "kill-on-close ended a live child without provenance")
            self.assertEqual(lifecycle["termination"]["reason"], "root_exit_live_descendants")
            self.assertEqual(lifecycle["termination"]["method"], "TerminateJobObject")
            self.assertGreaterEqual(lifecycle["termination"]["active_processes_before"], 1)
            self.assertEqual(result.exit_code, 0)
            self.assertFalse(result.cancelled)
            self.assertEqual(lifecycle["termination"]["purpose"], "scope_cleanup")
            self.assertFalse(lifecycle["cancelled"])
