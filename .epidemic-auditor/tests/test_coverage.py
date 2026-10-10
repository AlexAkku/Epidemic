"""Physical clang-cl branch/function coverage and merged-run controls."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import uuid
import json

from ep_auditor.collectors.coverage import collect_coverage, publish_coverage
from ep_auditor.core.evidence import RawEvidence, EvidenceError, digest
from ep_auditor.core.lineage import EvidenceLineage
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.process import ManagedProcessRunner
from test_core import AuditorCoreTests
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.tests import TestCatalog
from test_batch_orchestration import RecordingEvents

ROOT = Path(__file__).resolve().parents[2]


class CoverageTests(unittest.TestCase):
    def test_real_engine_coverage_profile_branch_merge_and_source_inventory(self):
        temporary_root = ROOT / '.ai/tmp'
        temporary_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=temporary_root) as temporary:
            source = Path(temporary)
            (source / 'main.cpp').write_text(
                '#include <Epidemic/Diagnostics/profiling.h>\n'
                '__declspec(noinline) int NeverCalled() { return 91; }\n'
                '__declspec(noinline) int Branch(int input) { if (input) return 7; return 3; }\n'
                'int main(int argc, char**) { epidemic::diagnostics::ProfileScope scope("coverage"); '
                'return Branch(argc > 1) == (argc > 1 ? 7 : 3) ? 0 : 1; }\n')
            (source / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.25)\nproject(CoverageFixture LANGUAGES CXX)\n'
                'set(BUILD_TESTING OFF CACHE BOOL "" FORCE)\n'
                'set(EPIDEMIC_BUILD_RUNTIME OFF CACHE BOOL "" FORCE)\n'
                'set(EPIDEMIC_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)\n'
                f'add_subdirectory("{ROOT.as_posix()}" engine)\n'
                'add_executable(CoverageFixture main.cpp)\n'
                'target_link_libraries(CoverageFixture PRIVATE EpidemicDiagnostics)\n'
                'target_compile_options(CoverageFixture PRIVATE /clang:-fprofile-instr-generate /clang:-fcoverage-mapping)\n'
                'enable_testing()\nadd_test(NAME Coverage.Zero COMMAND CoverageFixture)\n'
                'add_test(NAME Coverage.One COMMAND CoverageFixture other-branch)\n')
            build = ROOT / 'build/auditor-boundary/audit-coverage'
            for command in (["cmake", "--fresh", "-S", str(source), "-B", str(build), "-A", "x64",
                             "-T", "ClangCL", "-DEP_AUDITOR_PROFILE=audit-coverage"],
                            ["cmake", "--build", str(build), "--config", "Debug", "--target", "CoverageFixture", "--parallel", "2"]):
                result = subprocess.run(command, capture_output=True, text=True, timeout=180)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-3000:])
            binary = build / 'Debug/CoverageFixture.exe'
            profiles = []
            for ordinal, args in enumerate(([], ['other-branch'])):
                profile = source / (str(ordinal) + '.profraw')
                environment = os.environ.copy()
                environment['LLVM_PROFILE_FILE'] = str(profile)
                result = subprocess.run([str(binary), *args], env=environment, capture_output=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue(profile.is_file())
                profiles.append(profile)
            reports = []
            for name, inputs in (('one-test', profiles[:1]), ('merged-suite', profiles)):
                run = source / name
                report = collect_coverage(run, inputs, [binary], scope={'kind': name, 'runs': len(inputs)})
                self.assertTrue(any(row['path'].endswith('index.html') for row in report['heatmap']))
                reports.append(report)
                RawEvidence(run).seal({'run_id': 'RunId:' + str(uuid.uuid4()), 'instrumentation_profile': 'audit-coverage',
                    'source_fingerprints': {'fixture': digest(source / 'main.cpp')}, 'binary_fingerprints': {'fixture': digest(binary)},
                    'analyzer_version': '1'})
                publish_coverage(run)
                self.assertTrue(EvidenceLineage(run).verify('metrics', 'coverage.json')['completeness']['complete'])
            file_reports = [next(file for file in report['files'] if Path(file['file']) == source / 'main.cpp')
                            for report in reports]
            self.assertLess(file_reports[0]['summary']['branches']['covered'], file_reports[1]['summary']['branches']['covered'])
            # Branch() has both feasible outcomes. main() intentionally retains
            # an unexecuted error return; merging must not falsely cover it.
            branch = next(row for row in file_reports[1]['branches'] if row[0] == 3)
            self.assertGreater(branch[4], 0)
            self.assertGreater(branch[5], 0)
            single = next(row for row in file_reports[0]['branches'] if row[0] == 3)
            self.assertEqual(min(single[4], single[5]), 0)
            self.assertLess(file_reports[1]['summary']['branches']['covered'], file_reports[1]['summary']['branches']['count'])
            self.assertTrue(any('NeverCalled' in name for name in reports[1]['never_called_candidates']))
            config, project = AuditorCoreTests._config(self, source)
            storage = RunStorage(config)
            record = storage.new_run('test', 'normal coverage pipeline', 'Coverage')
            result = ManagedProcessRunner(config).run([str(binary)], project, record.run_dir, 'Coverage',
                env_extra={'EP_AUDITOR_COVERAGE_BINARIES': json.dumps([str(binary)])})
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(record, 'PASS')
            self.assertEqual(record.status, 'PASS')
            self.assertTrue(storage.evidence_index.query(record.run_dir, 'coverage_history'))
            config.data['project_root'] = str(source)
            config.data['build_dir'] = str(build)
            config.data['capture']['selected_mode'] = 'Coverage'
            config.save()
            catalog = TestCatalog(config)
            selected = catalog.discover()
            self.assertEqual({test.name for test in selected}, {'Coverage.Zero', 'Coverage.One'})
            events = RecordingEvents()
            manager = RunManager(config, storage, events)
            try:
                manager._run_tests(selected, 'Coverage', 'two real coverage tests')
            finally:
                manager.shutdown()
            finished = [value for kind, value in events.calls if kind == 'run_finished']
            self.assertEqual(len(finished), 1)
            self.assertEqual(finished[0]['status'], 'PASS')
            parent = storage.get_run(finished[0]['run_id'])
            envelope = EvidenceLineage(Path(parent['run_dir'])).verify('metrics', 'coverage.json')
            merged_report = envelope['records'][0]
            self.assertEqual(merged_report['scope']['observed_count'], 2)
            self.assertTrue(merged_report['scope']['selection_complete'])
            main_source = next(row for row in merged_report['files'] if Path(row['file']) == source / 'main.cpp')
            branch = next(row for row in main_source['branches'] if row[0] == 3)
            self.assertGreater(min(branch[4], branch[5]), 0)
            corrupt = source / 'bad.profraw'
            corrupt.write_bytes(b'not a profile')
            with self.assertRaises(EvidenceError):
                collect_coverage(source / 'invalid', [corrupt], [binary], scope={'kind': 'negative'})
