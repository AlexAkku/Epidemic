import tempfile
import shutil
import unittest
from pathlib import Path

from ep_auditor.core.environment import capture_environment
from ep_auditor.core.evidence import digest
from ep_auditor.core.evidence import EvidenceError
import test_core


class EnvironmentEvidenceTests(unittest.TestCase):
    def test_environment_actual_identity_and_source_drift(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            source = project / "EngineBase/main.cpp"
            source.parent.mkdir()
            source.write_text("int main() { return 0; }\n")
            environment = capture_environment(config)
            self.assertEqual(environment["provenance"], "external")
            self.assertEqual(environment["confidence"], "observed")
            self.assertEqual(environment["instrumentation_profile"], "audit")
            self.assertEqual(environment["engine_source"]["files"]["EngineBase/main.cpp"], digest(source))
            self.assertGreater(environment["total_memory_bytes"], 0)
            self.assertTrue(environment["auditor_source"]["files"])
            self.assertIn('native/tracing/exact_hooks.asm', environment['auditor_source']['files'])
            self.assertIn('rules/architecture.yaml', environment['auditor_source']['files'])
            self.assertTrue(environment["compiler_toolchain"]["availability"] in {"observed", "unavailable"})
            before = environment["engine_source"]["sha256"]
            source.write_text("int main() { return 1; }\n")
            changed = capture_environment(config, instrumentation_profile="audit")
            self.assertNotEqual(before, changed["engine_source"]["sha256"])
            self.assertEqual(changed["git"]["availability"], "unavailable")

    def test_missing_toolchain_is_explicit_not_zero(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            environment = capture_environment(config, instrumentation_profile="audit-coverage")
            self.assertEqual(environment["compiler_toolchain"]["availability"], "unavailable")
            self.assertIsNone(environment["compiler_toolchain"]["compiler"])
            self.assertTrue(environment["compiler_toolchain"]["reason"])
            self.assertEqual(environment["instrumentation_profile"], "audit-coverage")

    def test_observed_compiler_and_excluded_artifact_drift(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            compiler = shutil.which("clang-cl") or shutil.which("clang")
            self.assertIsNotNone(compiler, "qualification requires an actual installed compiler")
            config.build_dir.mkdir(parents=True)
            cache = config.build_dir / "CMakeCache.txt"
            cache.write_text("CMAKE_CXX_COMPILER:FILEPATH=" + compiler + "\nCMAKE_GENERATOR:INTERNAL=Ninja\n")
            environment = capture_environment(config)
            toolchain = environment["compiler_toolchain"]
            self.assertEqual(toolchain["availability"], "observed")
            self.assertEqual(toolchain["compiler_sha256"], digest(Path(compiler)))
            self.assertEqual(toolchain["cache_sha256"], digest(cache))
            self.assertTrue(toolchain["reported_version"])
            (project / "build/noise.cpp").write_text("artifact only")
            changed = capture_environment(config)
            self.assertEqual(environment["engine_source"], changed["engine_source"])
            with self.assertRaises(EvidenceError):
                capture_environment(config, instrumentation_profile="fake-PASS")
