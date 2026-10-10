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
    def test_native_debug_release_isolation_and_audit_collector_integration(self):
        root = Path(__file__).resolve().parents[2]
        # Runtime fixture inputs are disposable; compiler products remain in build/.
        with tempfile.TemporaryDirectory(dir=root / ".ai/tmp", prefix="audit-boundary-") as temporary:
            source = Path(temporary)
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.25)\nproject(AuditorBoundary LANGUAGES CXX)\n'
                'set(BUILD_TESTING OFF CACHE BOOL "" FORCE)\n'
                'set(EPIDEMIC_BUILD_RUNTIME OFF CACHE BOOL "" FORCE)\n'
                'set(EPIDEMIC_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)\n'
                'set(EPIDEMIC_WARNINGS_AS_ERRORS ON CACHE BOOL "" FORCE)\n'
                f'add_subdirectory("{root.as_posix()}" engine)\n'
                'add_library(AuditorBoundaryProbe STATIC EXCLUDE_FROM_ALL probe.cpp)\n'
                'target_link_libraries(AuditorBoundaryProbe PUBLIC EpidemicDiagnostics)\n'
                'add_executable(Boundary main.cpp)\n'
                'target_link_libraries(Boundary PRIVATE EpidemicDiagnostics)\n'
                'target_link_options(Boundary PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/boundary.map")\n'
                'if(EP_BOUNDARY_AUDIT)\n'
                'target_compile_definitions(Boundary PRIVATE EP_BOUNDARY_AUDIT)\n'
                'target_compile_options(Boundary PRIVATE /Gh /GH /Ob0 /Zi)\n'
                'target_link_options(Boundary PRIVATE /DEBUG:FULL /INCREMENTAL:NO /OPT:NOICF)\n'
                'target_link_libraries(Boundary PRIVATE AuditorBoundaryProbe AuditorExactRuntime)\nendif()\n', encoding="utf-8")
            (source / "main.cpp").write_text(
                '#include <Epidemic/Diagnostics/profiling.h>\n#include <iostream>\n'
                '#ifdef EP_BOUNDARY_AUDIT\n'
                'std::shared_ptr<epidemic::diagnostics::IProfileCollector> MakeAuditorBoundaryProbe();\n#endif\n'
                'int main() {\n#ifdef EP_BOUNDARY_AUDIT\n'
                'epidemic::diagnostics::SetProfileCollector(MakeAuditorBoundaryProbe());\n#endif\n'
                '{ epidemic::diagnostics::ProfileScope scope("physical_engine_scope"); }\n'
                'epidemic::diagnostics::SetProfileCollector({});\nstd::cout << "ENGINE_BOUNDARY_DONE\\n";\n}\n', encoding="utf-8")
            (source / "probe.cpp").write_text(
                '#include <Epidemic/Diagnostics/profiling.h>\n#include <iostream>\n'
                'class AuditorBoundaryProbe final : public epidemic::diagnostics::IProfileCollector {\n'
                'void Record(epidemic::diagnostics::ProfileEvent event) override {\n'
                'std::cout << "AUDITOR_BOUNDARY_MARKER " << event.name << "\\n"; }\n};\n'
                'std::shared_ptr<epidemic::diagnostics::IProfileCollector> MakeAuditorBoundaryProbe() {\n'
                'return std::make_shared<AuditorBoundaryProbe>(); }\n', encoding="utf-8")

            def command(arguments):
                result = subprocess.run(arguments, capture_output=True, text=True, timeout=180)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-4000:])
                return result.stdout

            for mode, configurations in (("normal", ("Debug", "Release")), ("audit", ("Debug",)), ("deep", ("Debug",))):
                build = root / "build/auditor-boundary" / mode
                command(["cmake", "--fresh", "-S", str(source), "-B", str(build), "-A", "x64",
                         "-DEP_BOUNDARY_AUDIT=" + ("OFF" if mode == "normal" else "ON"),
                         "-DEP_AUDITOR_PROFILE=" + ({'audit': 'audit', 'deep': 'audit-deep'}.get(mode, ''))])
                for configuration in configurations:
                    command(["cmake", "--build", str(build), "--config", configuration,
                             "--target", "Boundary", "--parallel", "2"])
                    capture = source / ('traces-' + mode + '-' + configuration)
                    capture.mkdir()
                    environment = os.environ.copy()
                    environment['EP_AUDITOR_TRACE_DIRECTORY'] = str(capture)
                    result = subprocess.run([str(build / configuration / "Boundary.exe")],
                                            env=environment, capture_output=True, text=True, timeout=30)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    output = result.stdout
                    link_map = (build / configuration / "boundary.map").read_text(errors="replace")
                    self.assertIn("ENGINE_BOUNDARY_DONE", output)
                    if mode != "normal":
                        self.assertIn("AUDITOR_BOUNDARY_MARKER physical_engine_scope", output)
                        self.assertIn("AuditorBoundaryProbe", link_map)
                        self.assertIn("AuditorExactRuntime", link_map)
                        from ep_auditor.collectors.exact_trace import ingest_trace
                        traces = list(capture.glob('*.bin'))
                        self.assertEqual(len(traces), 1)
                        _, _, row, report = ingest_trace(source / 'engine-trace', traces[0])
                        self.assertTrue(report['complete'], report['problems'])
                        self.assertTrue(row['complete'])
                        self.assertTrue(any('ProfileScope' in metric['symbol']['function'] for metric in report['functions']))
                        from ep_auditor.analyzers.architecture import runtime_architecture
                        from ep_auditor.analyzers.rules import evaluate_rules
                        graph = runtime_architecture(report, root, {'edges': []})
                        observed = next(edge for edge in graph['edges'] if edge['target'] == 'EngineBase/Diagnostics')
                        self.assertGreater(observed['calls'], 0)
                        self.assertGreater(observed['inclusive_ns'], 0)
                        rule = {'id': 'physical.boundary', 'kind': 'forbidden_edge', 'domain': 'architecture',
                                'source': observed['source'], 'target': observed['target'], 'config_sha256': 'physical'}
                        self.assertEqual(evaluate_rules([rule], {'architecture': graph})['status'], 'FAIL')
                        allowed = dict(rule, target='Absent/Subsystem')
                        self.assertEqual(evaluate_rules([allowed], {'architecture': graph})['status'],
                                         'PASS' if graph['complete'] else 'UNKNOWN')
                    else:
                        self.assertNotIn("AUDITOR_BOUNDARY_MARKER", output)
                        self.assertNotIn("AuditorBoundaryProbe", link_map)
                        self.assertNotIn("AuditorExactRuntime", link_map)
                        self.assertFalse(list(capture.glob('*.bin')))

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
