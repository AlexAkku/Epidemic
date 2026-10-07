import os
import sys
import subprocess
import tempfile
import threading
import time
import unittest
from pathlib import Path

import psutil

from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import load_json
import test_core


class OwnedProcessStopTests(unittest.TestCase):
    def test_stop_after_root_exit_terminates_live_owned_child(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "owned child", "Fast")
            child_file = record.run_dir / "child_pid.txt"
            script = ("import subprocess,sys,pathlib; p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'],"
                      "stdout=sys.stdout,stderr=sys.stderr); pathlib.Path(sys.argv[1]).write_text(str(p.pid)); print('ready',flush=True)")
            runner = ManagedProcessRunner(config)
            results = []
            errors = []

            def run():
                try:
                    results.append(runner.run([sys.executable, "-c", script, str(child_file)], project, record.run_dir, "Fast"))
                except BaseException as error:
                    errors.append(error)

            worker = threading.Thread(target=run, daemon=True)
            worker.start()
            deadline = time.monotonic() + 10
            root = None
            while time.monotonic() < deadline:
                with runner._active_lock:
                    root = runner._active
                if child_file.is_file() and root is not None and root.poll() is not None:
                    break
                time.sleep(0.01)
            self.assertIsNotNone(root)
            self.assertIsNotNone(root.poll(), "controlled root did not exit")
            child = psutil.Process(int(child_file.read_text()))
            self.assertTrue(child.is_running())
            runner.cancel()
            worker.join(timeout=3)
            self.assertFalse(worker.is_alive(), "stop returned while inherited owned child kept capture pipes live")
            self.assertFalse(child.is_running())
            self.assertFalse(errors)
            self.assertTrue(results[0].cancelled)

    def test_stop_leaves_unrelated_sibling_alive_and_records_kernel_reason(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "managed scope", "Fast")
            sentinel = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(30)"],
                                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            runner = ManagedProcessRunner(config)
            ready = threading.Event()
            results = []
            script = "import time; print('ready',flush=True); time.sleep(30)"
            worker = threading.Thread(target=lambda: results.append(runner.run(
                [sys.executable, "-c", script], project, record.run_dir, "Fast",
                log_callback=lambda channel, line: ready.set())), daemon=True)
            try:
                worker.start()
                self.assertTrue(ready.wait(10))
                runner.cancel()
                worker.join(timeout=3)
                self.assertFalse(worker.is_alive())
                self.assertIsNone(sentinel.poll(), "managed Stop affected unrelated sibling")
                self.assertTrue(results[0].cancelled)
                evidence = load_json(record.run_dir / "termination.json")
                self.assertEqual(evidence["method"], "TerminateJobObject")
                self.assertEqual(evidence["reason"], "user_stop")
                self.assertEqual(evidence["provenance"], "external")
                self.assertGreaterEqual(evidence["active_processes_before"], 1)
            finally:
                runner.cancel()
                worker.join(timeout=3)
                sentinel.terminate()
                sentinel.wait(timeout=5)

    def test_callback_failure_closes_owned_job_and_preserves_reason(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "callback failure", "Fast")
            runner = ManagedProcessRunner(config)
            target = []

            def broken_callback(channel, line):
                target.append(psutil.Process(runner._active.pid))
                raise RuntimeError("controlled callback failure")

            with self.assertRaisesRegex(RuntimeError, "controlled callback failure"):
                runner.run([sys.executable, "-c", "import time; print('ready',flush=True); time.sleep(30)"],
                           project, record.run_dir, "Fast", log_callback=broken_callback)
            self.assertFalse(target[0].is_running())
            self.assertIsNone(runner._active)
            self.assertEqual(load_json(record.run_dir / "termination.json")["reason"], "observer_operation_failure")
