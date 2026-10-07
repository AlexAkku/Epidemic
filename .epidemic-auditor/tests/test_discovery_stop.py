import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import uuid

from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.tests import TestCatalog
from ep_auditor.core.evidence import load_json
import test_core


class DiscoveryStopTests(unittest.TestCase):
    def test_discovery_uses_cancellable_owned_transport(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("discovery_stop_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            ready = threading.Event()
            command = [sys.executable, "-c", 'import time; print(\'{"tests":[]}\',flush=True); time.sleep(3)']

            class ControlledTransport(ManagedProcessRunner):
                def run(self, original, cwd, run_dir, mode, **kwargs):
                    self.original_command = original
                    return super().run(command, cwd, run_dir, mode, timeout=60,
                                       log_callback=lambda channel, line: ready.set())

            runner = ControlledTransport(config)
            catalog = TestCatalog(config)
            catalog.runner = runner
            errors = []

            def old_transport(*args, **kwargs):
                # Before the fix, discovery bypasses the assigned managed runner.
                ready.set()
                return subprocess.run(command, cwd=project, capture_output=True, timeout=10)

            def discover():
                try:
                    catalog.discover()
                except Exception as error:
                    errors.append(str(error))

            with patch("ep_auditor.core.tests.subprocess", SimpleNamespace(run=old_transport)):
                worker = threading.Thread(target=discover)
                worker.start()
                self.assertTrue(ready.wait(10))
                runner.cancel("user_stop")
                worker.join(10)
                self.assertFalse(worker.is_alive())
            header = load_json(catalog.discovery_run.run_dir / "run.json")
            self.assertEqual(header["status"], "CANCELLED")
            self.assertTrue(errors)
            lifecycle = load_json(catalog.discovery_run.run_dir / "raw/process/lifecycle.json")
            self.assertEqual(lifecycle["termination"]["reason"], "user_stop")
            self.assertEqual(lifecycle["termination"]["method"], "TerminateJobObject")
            (base / "discovery-stop-oracle.json").write_text(json.dumps({"run_dir": str(catalog.discovery_run.run_dir),
                "transport": "controlled Python process emitting valid CTest catalog before blocking", "errors": errors}))
