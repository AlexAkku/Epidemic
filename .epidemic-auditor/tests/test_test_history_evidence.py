import subprocess
import json
import os
import uuid
import tempfile
import unittest
from pathlib import Path

from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.tests import TestCatalog, TestExecutor
from ep_auditor.core.evidence import EvidenceError, RawEvidence
from ep_auditor.core.test_observation import junit_result
import test_core


class TestHistoryEvidenceTests(unittest.TestCase):
    def test_compatible_id_history_duration_and_stability_from_real_results(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("test_history_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(HistoryFixture LANGUAGES NONE)\n'
                                                    'enable_testing()\nadd_test(NAME History.Controlled COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/run.cmake)\n')
            (project / "run.cmake").write_text('if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/fail.flag")\nmessage(FATAL_ERROR "controlled failure")\nendif()\nmessage(STATUS "known good")\n')
            configured = subprocess.run(["cmake", "-S", str(project), "-B", str(config.build_dir)], capture_output=True, timeout=60)
            self.assertEqual(configured.returncode, 0)
            storage = RunStorage(config)
            test = TestCatalog(config).discover()[0]
            parent = storage.new_run("tests", "history", "Fast")
            executor = TestExecutor(config, ManagedProcessRunner(config), storage)
            first = executor.run_one(test, parent.run_dir, "Fast")
            self.assertEqual(first.test_id, test.test_id)
            self.assertTrue(first.compatibility_id.startswith("BuildId:"))
            (project / "fail.flag").write_text("controlled negative effect")
            second = executor.run_one(test, parent.run_dir, "Fast")
            (project / "fail.flag").unlink()
            third = executor.run_one(test, parent.run_dir, "Fast")
            self.assertEqual([first.status, second.status, third.status], ["PASS", "FAIL", "PASS"])
            self.assertEqual(len({first.test_run_id, second.test_run_id, third.test_run_id}), 3)
            self.assertEqual(len({first.compatibility_id, second.compatibility_id, third.compatibility_id}), 1)
            history = storage.history_for_test(test.test_id, first.compatibility_id)
            self.assertEqual(len(history["results"]), 3)
            self.assertEqual(history["stability"]["passed"], 2)
            self.assertEqual(history["stability"]["failed"], 1)
            self.assertAlmostEqual(history["stability"]["pass_rate"], 2 / 3)
            self.assertTrue(all(row["wall_seconds"] >= 0 for row in history["results"]))
            page = storage.history_for_test(test.test_id, first.compatibility_id, limit=1)
            self.assertTrue(page['truncated'])
            self.assertEqual(page['items'][0]['provenance'], 'external_ctest_observation')
            resumed = storage.history_for_test(test.test_id, first.compatibility_id,
                cursor=page['continuation'], limit=1)
            self.assertNotEqual(page['items'][0]['run_id'], resumed['items'][0]['run_id'])
            self.assertTrue(resumed['items'][0]['evidence']['sha256'])
            with self.assertRaises(EvidenceError):
                storage.history_for_test(test.test_id, 'BuildId:wrong', cursor=page['continuation'], limit=1)
            with self.assertRaises(EvidenceError):
                storage.history_for_test(test.test_id, first.compatibility_id, cursor=page['continuation'], limit=2)
            with storage._connect() as connection:
                connection.execute("UPDATE evidence_facts SET fact_json='{}' WHERE domain='tests'")
            with self.assertRaises(EvidenceError):
                storage.history_for_test(test.test_id, first.compatibility_id)
            storage.evidence_index.rebuild()
            self.assertEqual(storage.history_for_test(test.test_id, first.compatibility_id), history)
            (project / "run.cmake").write_text('message(STATUS "changed binary-equivalent script input")\n')
            changed = executor.run_one(test, parent.run_dir, "Fast")
            with self.assertRaises(EvidenceError):
                storage.history_for_test(test.test_id, first.compatibility_id, cursor=page['continuation'], limit=1)
            self.assertNotEqual(changed.compatibility_id, first.compatibility_id)
            self.assertEqual(len(storage.history_for_test(test.test_id, first.compatibility_id)["results"]), 3)
            self.assertEqual(len(storage.history_for_test(test.test_id, changed.compatibility_id)["results"]), 1)
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Empty LANGUAGES NONE)\nenable_testing()\n')
            self.assertEqual(subprocess.run(["cmake", "-S", str(project), "-B", str(config.build_dir)], capture_output=True, timeout=60).returncode, 0)
            missing = executor.run_one(test, parent.run_dir, "Fast")
            self.assertEqual(missing.exit_code, 0)  # Real CTest false-positive trigger: no selected tests.
            self.assertEqual(missing.status, "INCOMPLETE")
            self.assertIsNone(storage.history_for_test(test.test_id, missing.compatibility_id)["stability"]["pass_rate"])
            (base / "history-oracle.json").write_text(json.dumps({"test_id": test.test_id,
                "compatible_id": first.compatibility_id, "changed_id": changed.compatibility_id,
                "missing_id": missing.compatibility_id, "child_runs": [first.test_run_id, second.test_run_id,
                third.test_run_id, changed.test_run_id, missing.test_run_id], "expected": ["PASS", "FAIL", "PASS", "PASS", "INCOMPLETE"]}))

    def test_skipped_missing_duplicate_wrong_name_and_invalid_duration_are_not_pass(self):
        skipped = b'<testsuite><testcase name="Selected" time="0"><skipped/></testcase></testsuite>'
        self.assertEqual(junit_result(skipped, "Selected", 0, False), ("INCOMPLETE", 0))
        for payload in (b'<testsuite/>', b'<testsuite><testcase name="Other" time="0"/></testsuite>',
                        b'<testsuite><testcase name="Selected" time="nan"/></testsuite>',
                        b'<testsuite><testcase name="Selected" time="-1"/></testsuite>',
                        b'<testsuite><testcase name="Selected" time="0"/><testcase name="Selected" time="0"/></testsuite>'):
            with self.subTest(payload=payload), self.assertRaises(EvidenceError):
                junit_result(payload, "Selected", 0, False)
