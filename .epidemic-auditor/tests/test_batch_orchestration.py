import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import uuid

from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.tests import TestCatalog
from ep_auditor.core.lifecycle import authoritative_run
import test_core


class RecordingEvents:
    def __init__(self):
        self.calls = []
        self.callback = None

    def emit(self, kind, **value):
        self.calls.append((kind, value))
        if self.callback:
            self.callback(kind, value)


class BatchOrchestrationTests(unittest.TestCase):
    def execute_case(self, base, stop):
        config, project = test_core.AuditorCoreTests._config(self, base)
        config.data["cmake"]["configure_args"] = ["-A", "x64"]
        config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/batch" / uuid.uuid4().hex / "audit")
        script = project / "probe.py"
        script.write_text("import pathlib,sys; pathlib.Path(sys.argv[1]).write_text('executed'); print('known good')\n")
        (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Batch NONE)\nenable_testing()\n'
            + ''.join('add_test(NAME Test' + str(index) + ' COMMAND "' + Path(sys.executable).as_posix() + '" "'
                      + script.as_posix() + '" "' + (project / (str(index) + '.marker')).as_posix() + '")\n'
                      for index in (1, 2)))
        events = RecordingEvents()
        storage = RunStorage(config)
        manager = RunManager(config, storage, events)
        try:
            configure = storage.new_run("configure", "batch fixture configure", "Fast")
            self.assertEqual(manager.builder.configure(configure.run_dir, "Fast").exit_code, 0)
            storage.finish_run(configure, "PASS", {})
            tests = TestCatalog(config).discover()
            if stop:
                def callback(kind, value):
                    if kind == "test_progress" and value["current"] == 2:
                        manager.shutdown() if stop == "application_shutdown" else manager.stop()
                events.callback = callback
            manager._run_tests(tests, "Fast", "controlled batch")
            finished = [value for kind, value in events.calls if kind == "run_finished"]
            self.assertEqual(len(finished), 1)
            self.assertEqual(finished[0]["status"], "CANCELLED" if stop else "PASS")
            parent = storage.get_run(finished[0]["run_id"])
            self.assertEqual(authoritative_run(Path(parent["run_dir"]))["status"], finished[0]["status"])
            self.assertTrue((project / "1.marker").is_file())
            self.assertEqual((project / "2.marker").is_file(), not stop)
            if stop:
                batch = json.loads((Path(parent["run_dir"]) / "raw/batch/observation.json").read_bytes())
                self.assertEqual(batch["cancel_reason"], "application_shutdown" if stop == "application_shutdown" else "user_stop")
            (base / "batch-oracle.json").write_text(json.dumps({"parent_run_id": finished[0]["run_id"],
                "parent_run_dir": parent["run_dir"], "stop_between_tests": stop, "second_executed": not stop}))
        finally:
            manager.shutdown()

    def test_successful_batch_has_committed_parent_and_child_evidence(self):
        self.case(False)

    def test_stop_between_tests_does_not_launch_next_child(self):
        self.case(True)

    def case(self, stop):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("batch_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            self.execute_case(base, stop)
