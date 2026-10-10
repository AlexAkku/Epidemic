import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import uuid
from types import SimpleNamespace
from unittest.mock import patch

from ep_auditor.core.tests import TestCatalog
from ep_auditor.core.evidence import EvidenceError, RawEvidence, load_json
from ep_auditor.core.identity import semantic_command, test_identity
import test_core


class CTestCatalogEvidenceTests(unittest.TestCase):
    def test_actual_ctest_stable_ids_source_location_and_profile(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("ctest_catalog_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            source = project / "CMakeLists.txt"
            source.write_text('cmake_minimum_required(VERSION 3.20)\nproject(CatalogFixture LANGUAGES NONE)\n'
                              'enable_testing()\nadd_test(NAME "Sample.[Exact]" COMMAND ${CMAKE_COMMAND} -E echo known)\n'
                              'set_tests_properties("Sample.[Exact]" PROPERTIES LABELS "smoke;subsystem" TIMEOUT 7)\n')
            process = subprocess.run(["cmake", "-S", str(project), "-B", str(config.build_dir)], capture_output=True, timeout=60)
            self.assertEqual(process.returncode, 0, process.stderr.decode(errors="replace"))
            catalog = TestCatalog(config)
            first = catalog.discover()
            raw = RawEvidence(catalog.discovery_run.run_dir)
            observed = json.loads(raw.read_bytes("ctest/stdout.bin"))
            self.assertEqual(observed["tests"][0]["name"], "Sample.[Exact]")
            self.assertEqual(len(first), 1)
            self.assertTrue(first[0].test_id.startswith("TestId:"))
            self.assertEqual(first[0].labels, ["smoke", "subsystem"])
            self.assertEqual(first[0].timeout, 7)
            self.assertEqual(first[0].executable, first[0].command[0])
            self.assertEqual(first[0].arguments, first[0].command[1:])
            self.assertEqual(Path(first[0].source_location["file"]), source)
            self.assertEqual(first[0].source_location["line"], 4)
            self.assertEqual(first[0].build_profile["configuration"], "Debug")
            second = TestCatalog(config).discover()
            self.assertEqual(first[0].test_id, second[0].test_id)
            (base / "catalog.json").write_text(json.dumps([test.to_dict() for test in first], indent=2))

    def test_identity_survives_build_relocation_and_row_reordering(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            command = [str(config.build_dir / "Debug/fixture.exe"), str(project / "fixture.json")]
            identity, arguments = semantic_command(command, project, config.build_dir, "Debug")
            original = test_identity("fixture", identity, arguments)
            relocated = project / "build/other-audit"
            identity2, arguments2 = semantic_command([str(relocated / "Release/fixture.exe"), str(project / "fixture.json")],
                                                     project, relocated, "Release")
            self.assertEqual(original, test_identity("fixture", identity2, arguments2))
            self.assertNotEqual(original, test_identity("fixture", identity2, ["different input"]))
            payload = {"tests": [{"name": "other", "command": ["cmake", "-E", "echo", "other"]},
                                  {"name": "fixture", "command": command}]}
            with patch.object(TestCatalog, "_collect", return_value=SimpleNamespace(returncode=0, stdout=json.dumps(payload), stderr="")):
                cases = TestCatalog(config).discover()
            self.assertEqual(cases[1].test_id, original)

    def test_missing_identity_invalid_backtrace_and_discovery_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            malformed = [{}, {"tests": [{"name": "missing-command"}]},
                         {"tests": [{"name": "known", "command": ["cmake"], "backtrace": 0}],
                          "backtraceGraph": {"nodes": [{"parent": 0}], "files": []}}]
            for payload in malformed:
                with self.subTest(payload=payload):
                    catalog = TestCatalog(config)
                    with patch.object(TestCatalog, "_collect",
                               return_value=SimpleNamespace(returncode=0, stdout=json.dumps(payload), stderr="")), self.assertRaises(EvidenceError):
                        catalog.discover()
                    self.assertEqual(load_json(catalog.discovery_run.run_dir / "run.json")["status"], "FAIL")
            with patch.object(TestCatalog, "_collect",
                       return_value=SimpleNamespace(returncode=7, stderr="controlled discovery failure", stdout="")):
                with self.assertRaisesRegex(RuntimeError, "CTest discovery failed"):
                    TestCatalog(config).discover()
