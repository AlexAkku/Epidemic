import json
import os
from pathlib import Path
import tempfile
import sys
import unittest
from unittest.mock import patch
import uuid

from ep_auditor.core.build import BuildController
from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.lifecycle import authoritative_run
import test_core


class StopAdmissionTests(unittest.TestCase):
    def test_timeout_only_cancels_current_child_not_next_child(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("timeout_scope_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            storage = RunStorage(config)
            manager = RunManager(config, storage, EventBus())
            try:
                first = storage.new_run("fixture", "timeout current child", "Fast")
                timed_out = manager.runner.run([sys.executable, "-c", "import time; time.sleep(30)"],
                                               project, first.run_dir, "Fast", timeout=0.2)
                self.assertTrue(timed_out.cancelled)
                storage.finish_run(first, "CANCELLED", {})
                second = storage.new_run("fixture", "next child after timeout", "Fast")
                next_result = manager.runner.run([sys.executable, "-c", "print('next child observed')"],
                                                 project, second.run_dir, "Fast", timeout=5)
                self.assertEqual(next_result.exit_code, 0, "timeout poisoned the next child in the same operation")
                self.assertFalse(next_result.cancelled)
                storage.finish_run(second, "PASS", {})
                (base / "timeout-scope-oracle.json").write_text(json.dumps({"first": str(first.run_dir), "second": str(second.run_dir)}))
            finally:
                manager.shutdown()

    def test_stop_before_sdk_creation_is_latched_and_next_operation_can_run(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("stop_admission_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["cmake"]["configure_args"] = ["-A", "x64"]
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/stop-admission" / uuid.uuid4().hex / "audit")
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(StopAdmission NONE)\n')
            storage = RunStorage(config)
            manager = RunManager(config, storage, EventBus())
            capture = BuildController._capture_project

            def stop_before_creation(run_dir, selection):
                capture(run_dir, selection)
                manager.stop()

            try:
                with patch.object(BuildController, "_capture_project", side_effect=stop_before_creation):
                    cancelled = manager.execute("configure", "Fast")
                rows = storage.list_runs()
                self.assertEqual(len(rows), 1)
                run = authoritative_run(Path(rows[0]["run_dir"]))
                self.assertEqual(run["status"], "CANCELLED", "Stop disappeared because no active process existed yet")
                self.assertFalse((config.build_dir / "CMakeCache.txt").exists(), "SDK code executed after prelaunch Stop")
                positive = manager.execute("configure", "Fast")
                self.assertTrue((config.build_dir / "CMakeCache.txt").is_file())
                self.assertNotEqual(cancelled["invocation_id"], positive["invocation_id"])
                (base / "stop-admission-oracle.json").write_text(json.dumps({"cancelled": cancelled, "positive": positive,
                    "cancelled_run_dir": rows[0]["run_dir"], "runs_dir": str(config.runs_dir),
                    "cache": str(config.build_dir / "CMakeCache.txt"), "fault": "Stop immediately before SDK process creation"}))
            finally:
                manager.shutdown()
