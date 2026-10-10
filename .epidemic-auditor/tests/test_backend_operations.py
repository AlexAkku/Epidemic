import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import uuid

from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.lifecycle import authoritative_run
import test_core


class BackendOperationTests(unittest.TestCase):
    def test_headless_required_operations_share_ids_and_capture_real_effects(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("backend_operations_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["cmake"]["configure_args"] = ["-A", "x64"]
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/operations" / uuid.uuid4().hex / "audit")
            script = project / "game.py"
            script.write_text("import pathlib,sys; pathlib.Path(sys.argv[1]).write_text('game effect'); print('game observed')\n")
            config.data["run"]["command"] = [sys.executable, str(script), str(project / "game.marker")]
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Operations NONE)\nenable_testing()\n'
                'add_test(NAME Alpha COMMAND "${CMAKE_COMMAND}" -E echo alpha)\n'
                'set_tests_properties(Alpha PROPERTIES LABELS "alpha;subsystem")\n'
                'add_test(NAME Beta COMMAND "${CMAKE_COMMAND}" -E echo beta)\n'
                'set_tests_properties(Beta PROPERTIES LABELS "beta")\n')
            storage = RunStorage(config)
            manager = RunManager(config, storage, EventBus())
            calls = []
            try:
                for operation in ("configure", "build", "discover-tests"):
                    calls.append(manager.execute(operation, "Fast"))
                tests = manager.catalog.tests
                self.assertEqual(len(tests), 2)
                calls.append(manager.execute("run-one-test", "Fast", test_ids=[tests[0].test_id]))
                calls.append(manager.execute("run-selected-tests", "Fast", test_ids=[test.test_id for test in tests]))
                calls.append(manager.execute("run-group", "Fast", labels=["alpha"]))
                calls.append(manager.execute("run-all-tests", "Fast"))
                calls.append(manager.execute("start-game", "Fast"))
                calls.append(manager.execute("build-and-run", "Fast"))
                expected = {"configure", "build", "discover-tests", "run-one-test", "run-selected-tests", "run-group",
                            "run-all-tests", "start-game", "build-and-run"}
                self.assertEqual({call["operation_id"] for call in calls}, expected)
                self.assertEqual(len({call["invocation_id"] for call in calls}), len(calls))
                self.assertEqual((project / "game.marker").read_text(), "game effect")
                observed = set()
                for run in storage.list_runs():
                    header = authoritative_run(Path(run["run_dir"]))
                    backend = header["metadata"].get("backend_operation")
                    if backend:
                        observed.add(backend["operation_id"])
                        self.assertEqual(header["status"], "PASS")
                        self.assertEqual(backend["entry_point"], "headless")
                self.assertEqual(observed, expected)
                with self.assertRaises(ValueError):
                    manager.execute("run-one-test", "Fast", test_ids=[])
                with self.assertRaises(ValueError):
                    manager.execute("run-group", "Fast", labels=[])
                with self.assertRaises(ValueError):
                    manager.execute("unknown-operation", "Fast")
                (base / "operations-oracle.json").write_text(json.dumps({"calls": calls, "runs_dir": str(config.runs_dir)}))
            finally:
                manager.shutdown()
