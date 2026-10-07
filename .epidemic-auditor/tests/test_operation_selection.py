import json
import os
from pathlib import Path
import tempfile
import unittest
import uuid

from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.lifecycle import authoritative_run
import test_core


class Events:
    def __init__(self):
        self.callback = None

    def emit(self, kind, **value):
        if self.callback:
            self.callback(kind, value)


class OperationSelectionTests(unittest.TestCase):
    def fixture(self, base):
        config, project = test_core.AuditorCoreTests._config(self, base)
        config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/selection" / uuid.uuid4().hex / "audit")
        config.data["cmake"]["configure_args"] = ["-A", "x64"]
        (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Selection NONE)\n'
            'enable_testing()\nif(EP_AUDITOR_PROFILE STREQUAL "audit-memory")\n'
            'add_test(NAME MemoryOnly COMMAND "${CMAKE_COMMAND}" -E echo memory)\nelse()\n'
            'add_test(NAME FastOnly COMMAND "${CMAKE_COMMAND}" -E echo fast)\nendif()\n')
        events = Events()
        return config, project, events, RunManager(config, RunStorage(config), events)

    def case(self, kind):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / (kind + "_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project, events, manager = self.fixture(base)
            try:
                if kind == "profile":
                    manager.execute("configure", "Fast")
                    manager.execute("discover-tests", "Fast")
                    stale = manager.catalog.tests[0].test_id
                    self.assertEqual(manager.catalog.tests[0].name, "FastOnly")
                    manager.execute("configure", "Memory")
                    with self.assertRaisesRegex(ValueError, "absent from current profile"):
                        manager.execute("run-one-test", "Memory", test_ids=[stale])
                    self.assertFalse(manager.busy)
                    runs = [authoritative_run(path) for path in config.runs_dir.iterdir()]
                    self.assertFalse(any(row["kind"] in {"test", "tests"} for row in runs))
                    self.assertEqual([test.name for test in manager.catalog.tests], ["MemoryOnly"])
                    oracle = {"stale_test_id": stale, "current_test_id": manager.catalog.tests[0].test_id}
                    self.assertNotEqual(oracle["stale_test_id"], oracle["current_test_id"])
                else:
                    rejected = []
                    def callback(event, value):
                        if event == "busy" and value["busy"]:
                            config.data["project_root"] = str(base / "absent-mutated-root")
                            with self.assertRaisesRegex(RuntimeError, "busy"):
                                manager.execute("configure", "Fast")
                            rejected.append(True)
                    events.callback = callback
                    manager.execute("configure", "Fast")
                    runs = [authoritative_run(path) for path in config.runs_dir.iterdir()]
                    self.assertEqual(len(runs), 1)
                    self.assertEqual(runs[0]["status"], "PASS")
                    self.assertEqual(runs[0]["metadata"]["project_selection"]["selected_root"], str(project.resolve()))
                    self.assertEqual(rejected, [True])
                    oracle = {"admitted_root": str(project), "mutated_root": config.data["project_root"], "busy_rejections": 1}
                (base / "selection-oracle.json").write_text(json.dumps({"kind": kind, **oracle, "runs_dir": str(config.runs_dir)}))
            finally:
                manager.shutdown()

    def test_profile_switch_rejects_stale_test_id_before_execution(self):
        self.case("profile")

    def test_admitted_snapshot_survives_config_edit_and_rejects_concurrent_operation(self):
        self.case("snapshot")
