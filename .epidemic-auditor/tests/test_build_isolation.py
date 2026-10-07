import os
import subprocess
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.build import BuildController
from ep_auditor.core.evidence import EvidenceError, digest, load_json
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.tests import TestCatalog, TestExecutor
import test_core


class BuildIsolationTests(unittest.TestCase):
    def test_actual_audit_profiles_are_separate_and_normal_cache_is_unchanged(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("build_isolation_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Isolation LANGUAGES NONE)\nenable_testing()\nadd_test(NAME Scope.Pass COMMAND ${CMAKE_COMMAND} -E echo controlled)\n')
            normal = project / "build/normal"
            self.assertEqual(subprocess.run(["cmake", "-S", str(project), "-B", str(normal)], capture_output=True, timeout=60).returncode, 0)
            normal_sha = digest(normal / "CMakeCache.txt")
            storage = RunStorage(config)
            builder = BuildController(config, ManagedProcessRunner(config))
            paths = []
            identities = []
            for mode in ("Fast", "Deep", "Memory", "Coverage"):
                run = storage.new_run("configure", mode, mode)
                result = builder.configure(run.run_dir, mode)
                self.assertEqual(result.exit_code, 0)
                paths.append(result.command[result.command.index("-B") + 1])
                storage.finish_run(run, "PASS", {"process": result.to_dict()})
                scoped = config.for_mode(mode)
                catalog = TestCatalog(scoped)
                discovered = catalog.discover()
                identities.append(discovered[0].test_id)
                expected_profile = {"Fast": "audit", "Deep": "audit-deep", "Memory": "audit-memory", "Coverage": "audit-coverage"}[mode]
                self.assertEqual(load_json(catalog.discovery_run.run_dir / "environment.json")["instrumentation_profile"], expected_profile)
                parent = storage.new_run("tests", mode, mode)
                result = TestExecutor(config, ManagedProcessRunner(config), storage).run_one(discovered[0], parent.run_dir, mode)
                self.assertEqual(result.status, "PASS")
            self.assertEqual(len(set(identities)), 1)
            self.assertEqual(len(set(paths)), 4)
            self.assertEqual(digest(normal / "CMakeCache.txt"), normal_sha)
            config.data["build_dir"] = str(normal)
            blocked = storage.new_run("configure", "normal cache forbidden", "Fast")
            with self.assertRaises(EvidenceError):
                builder.configure(blocked.run_dir, "Fast")
            self.assertEqual(digest(normal / "CMakeCache.txt"), normal_sha)
