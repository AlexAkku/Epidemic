"""Physical MSVC hooks, PDB matching, counters and deliberately incomplete traces."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import uuid

from ep_auditor.collectors.exact_trace import read_header, ingest_trace, iter_events, capture_directory, EVENT
from ep_auditor.core.evidence import RawEvidence, EvidenceError
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.lineage import EvidenceLineage
from test_core import AuditorCoreTests
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.tests import TestCatalog
from test_batch_orchestration import RecordingEvents

ROOT = Path(__file__).resolve().parents[2]
TEMP = ROOT / '.ai/tmp'
TEMP.mkdir(parents=True, exist_ok=True)


class ExactTraceTests(unittest.TestCase):
    def test_real_windows_unwind_records_and_wide_vectorcall_abi(self):
        native = ROOT / 'build/auditor-native/bin/Debug'
        for executable, arguments in (('EpidemicAuditorExactFixture.exe', ['unwind-check']),
                                      ('EpidemicAuditorExactAvxFixture.exe', [])):
            result = subprocess.run([str(native / executable), *arguments], capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 0, (executable, result.returncode, result.stderr))

    def test_inactive_shadow_hook_finishes_real_scene_debug_without_capture(self):
        # Ordinary CTest has no recorder directory. Exercise that same mode on
        # the production Scene implementation, not a replacement microfixture.
        scene = ROOT / 'build/audit-deep/EngineRuntime/Scene/tests/Debug/EpidemicRuntimeSceneTests.exe'
        self.assertTrue(scene.is_file(), 'mandatory Audit-Deep Debug Scene target must be built')
        env = dict(os.environ)
        for name in ('EP_AUDITOR_TRACE_DIRECTORY', 'EP_AUDITOR_DIAGNOSTICS_DIRECTORY',
                     'EP_AUDITOR_CAUSAL_DIRECTORY', 'EP_AUDITOR_ROLLING_DIRECTORY'):
            env.pop(name, None)
        result = subprocess.run([str(scene)], cwd=scene.parent, env=env,
                                # The full Debug Scene workload takes ~41 s with inactive hooks.
                                capture_output=True, timeout=120)
        self.assertEqual(result.returncode, 0, result.stderr[-2048:])

    def capture(self, directory, variant='balanced', capacity=262144):
        configuration = os.environ.get('EP_AUDITOR_NATIVE_CONFIG', 'Debug')
        native = ROOT / 'build/auditor-native/bin' / configuration
        fixture, helper = native / 'EpidemicAuditorExactFixture.exe', native / 'EpidemicAuditorObservation.exe'
        self.assertTrue(fixture.is_file(), 'mandatory physical exact fixture not built')
        destination = directory / ('capture-' + variant)
        destination.mkdir()
        environment = os.environ.copy()
        environment['EP_AUDITOR_TRACE_DIRECTORY'] = str(destination)
        environment['EP_AUDITOR_TRACE_CAPACITY'] = str(capacity)
        result = subprocess.run([str(fixture), variant], env=environment, capture_output=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)
        traces = list(destination.glob('*.bin'))
        self.assertEqual(len(traces), 1)
        return traces[0], helper

    def test_real_recursion_threads_registers_pdb_tree_and_metrics(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            source, helper = self.capture(directory)
            header, symbols, row, report = ingest_trace(directory / 'run', source, helper=helper)
            self.assertTrue(report['complete'], report['problems'])
            self.assertEqual(header['dropped'], 0)
            functions = {row['symbol']['function']: row for row in report['functions']}
            self.assertEqual(functions['AuditorExactLeaf']['calls'], 6)
            self.assertEqual(functions['AuditorExactRecursive']['calls'], 8)
            self.assertEqual(len(functions['AuditorExactRecursive']['threads']), 2)
            self.assertEqual(functions['AuditorExactRegisters']['calls'], 1)
            leaf = functions['AuditorExactLeaf']
            durations = sorted(call['inclusive_ns'] for call in report['calls'] if call['functionId'] == leaf['functionId'])
            self.assertEqual(leaf['p50_ns'], (durations[2] + durations[3]) / 2)
            self.assertTrue(any(call['parentCallId'] for call in report['calls']))
            self.assertTrue(all(row['symbol']['source_line'] for name, row in functions.items() if name.startswith('AuditorExact')))
            for metric in functions.values():
                self.assertLessEqual(metric['self_ns'], metric['inclusive_ns'])
                self.assertLessEqual(metric['min_ns'], metric['p50_ns'])
                self.assertLessEqual(metric['p50_ns'], metric['p95_ns'])
                self.assertLessEqual(metric['p95_ns'], metric['p99_ns'])

    def test_real_overflow_and_exception_unwind_never_complete(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            for variant, capacity in (('overflow', 8), ('unbalanced', 262144)):
                with self.subTest(variant=variant):
                    source, helper = self.capture(directory, variant, capacity)
                    header, symbols, row, report = ingest_trace(directory / ('run-' + variant), source, helper=helper)
                    self.assertFalse(report['complete'])
                    self.assertFalse(row['complete'])
                    if variant == 'overflow': self.assertGreater(header['dropped'], 0)
                    else: self.assertTrue(report['problems']['unmatched_enter'] or report['problems']['unmatched_exit'])

    def test_truncated_native_records_are_rejected(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            source, _ = self.capture(directory)
            corrupted = directory / 'truncated.bin'
            with source.open('rb') as stream:
                corrupted.write_bytes(stream.read(4096))
            with self.assertRaises(EvidenceError): read_header(corrupted)

    def test_normal_runner_seals_and_publishes_real_trace_and_rejects_missing_producer(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            config, project = AuditorCoreTests._config(self, directory)
            config.data['capture']['exact_required'] = True
            storage = RunStorage(config)
            fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorExactFixture.exe'
            run = storage.new_run('test', 'physical exact pipeline', 'Fast')
            result = ManagedProcessRunner(config).run([str(fixture), 'balanced'], project, run.run_dir, 'Fast')
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(run, 'PASS')
            self.assertEqual(run.status, 'PASS')
            metrics = list((run.run_dir / 'metrics/exact').glob('*-functions.json'))
            self.assertEqual(len(metrics), 1)
            value = EvidenceLineage(run.run_dir).verify('metrics', 'exact/' + metrics[0].name)
            self.assertTrue(value['completeness']['complete'])
            self.assertTrue(any(row.get('eventType') == 'exact.function' for row in value['records']))
            history = storage.history_for_function(next(row['functionId'] for row in value['records']
                                                       if row.get('eventType') == 'exact.function'))
            self.assertEqual(history['observed_runs'], 1)
            self.assertGreater(history['total_calls'], 0)
            self.assertIsNone(history['performance_regression'])
            missing = storage.new_run('test', 'no exact producer', 'Fast')
            result = ManagedProcessRunner(config).run([os.sys.executable, '-c', 'pass'], project, missing.run_dir, 'Fast')
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(missing, 'PASS')
            self.assertEqual(missing.status, 'INCOMPLETE')

    def test_normal_runner_merges_real_exe_and_dll_call_tree(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            config, project = AuditorCoreTests._config(self, directory)
            config.data['capture']['exact_required'] = True
            storage = RunStorage(config)
            fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorExactFixture.exe'
            run = storage.new_run('test', 'physical cross-DLL exact pipeline', 'Fast')
            result = ManagedProcessRunner(config).run([str(fixture), 'cross-dll'], project, run.run_dir, 'Fast')
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(run, 'PASS')
            self.assertEqual(run.status, 'PASS')
            self.assertEqual(len(list((run.run_dir / 'raw/exact').glob('*.bin'))), 2)
            metrics = list((run.run_dir / 'metrics/exact').glob('*-functions.json'))
            self.assertEqual(len(metrics), 1, 'one process must not publish isolated per-DLL trees')
            rows = EvidenceLineage(run.run_dir).verify('metrics', 'exact/' + metrics[0].name)['records']
            functions = {row['symbol']['function']: row for row in rows if row['eventType'] == 'exact.function'}
            self.assertEqual(functions['AuditorExactDllLeaf']['calls'], 2)
            self.assertEqual(functions['AuditorExactDllCall']['calls'], 1)
            main = functions['main']['functionId']
            dll = functions['AuditorExactDllCall']['functionId']
            self.assertIn(main, functions['AuditorExactDllCall']['callers'])
            self.assertIn(dll, functions['main']['callees'])

    def test_decoder_failure_retains_actual_corrupt_bytes_and_never_passes(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            source, _ = self.capture(directory)
            config, _ = AuditorCoreTests._config(self, directory)
            storage = RunStorage(config)
            run = storage.new_run('test', 'corrupt exact producer', 'Fast')
            destination = run.run_dir / 'exact-capture'
            destination.mkdir()
            corrupted = destination / source.name
            with source.open('rb') as stream:
                corrupted.write_bytes(stream.read(4096))
            capture_directory(run.run_dir)
            storage.finish_run(run, 'PASS')
            self.assertEqual(run.status, 'INCOMPLETE')
            self.assertEqual(RawEvidence(run.run_dir).read_bytes('exact/' + source.name), corrupted.read_bytes())

    def test_cross_module_clock_ties_never_produce_complete_or_pass(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            config, _ = AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            run = storage.new_run('test', 'ambiguous physical cross-module trace', 'Fast')
            destination = run.run_dir / 'exact-capture'
            destination.mkdir()
            environment = dict(os.environ, EP_AUDITOR_TRACE_DIRECTORY=str(destination))
            fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorExactFixture.exe'
            result = subprocess.run([str(fixture), 'cross-dll'], env=environment, capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 0)
            traces = list(destination.glob('*.bin'))
            self.assertEqual(len(traces), 2)
            # Corrupt real producer clocks on disk; ordinary import/finalization stays intact.
            for trace in traces:
                header = read_header(trace)
                with trace.open('r+b') as stream:
                    for ordinal in range(header['written']):
                        position = header['header_bytes'] + ordinal * EVENT.size
                        stream.seek(position)
                        event = list(EVENT.unpack(stream.read(EVENT.size)))
                        event[1:4] = [1000000, 0, 0]
                        stream.seek(position)
                        stream.write(EVENT.pack(*event))
            capture_directory(run.run_dir)
            storage.finish_run(run, 'PASS')
            self.assertEqual(run.status, 'INCOMPLETE')
            metric = next((run.run_dir / 'metrics/exact').glob('*-functions.json'))
            value = EvidenceLineage(run.run_dir).verify('metrics', 'exact/' + metric.name)
            self.assertFalse(value['completeness']['complete'])
            summary = next(row for row in value['records'] if row['eventType'] == 'exact.summary')
            self.assertGreater(summary['problems']['ambiguous_cross_module_order'], 0)

    def test_actual_ctest_parent_replays_children_without_doubling_history(self):
        with tempfile.TemporaryDirectory(dir=TEMP) as temporary:
            directory = Path(temporary)
            config, project = AuditorCoreTests._config(self, directory)
            project = TEMP / 'exact-ctest-project'
            project.mkdir(exist_ok=True)
            config.data['project_root'] = str(project)
            native = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorExactFixture.exe'
            (project / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.25)\nproject(ExactBatch NONE)\nenable_testing()\n'
                f'add_test(NAME Exact.Exe COMMAND "{native.as_posix()}" balanced)\n'
                f'add_test(NAME Exact.Dll COMMAND "{native.as_posix()}" cross-dll)\n')
            config.data['build_dir'] = str(ROOT / 'build/auditor-boundary/exact-ctest/audit')
            config.data['cmake']['configure_args'] = ['-A', 'x64']
            config.save()
            storage, events = RunStorage(config), RecordingEvents()
            manager = RunManager(config, storage, events)
            try:
                configure = storage.new_run('configure', 'real exact CTest fixture', 'Fast')
                result = manager.builder.configure(configure.run_dir, 'Fast')
                self.assertEqual(result.exit_code, 0)
                storage.finish_run(configure, 'PASS')
                selected = TestCatalog(config).discover()
                manager._run_tests(selected, 'Fast', 'two exact native tests')
            finally:
                manager.shutdown()
            finished = [value for kind, value in events.calls if kind == 'run_finished']
            self.assertEqual(len(finished), 1)
            self.assertEqual(finished[0]['status'], 'PASS')
            parent = storage.get_run(finished[0]['run_id'])
            parent_dir = Path(parent['run_dir'])
            self.assertEqual(len(list((parent_dir / 'raw/exact').glob('*.bin'))), 3)
            self.assertEqual(len(list((parent_dir / 'metrics/exact').glob('*-functions.json'))), 2)
            facts = storage.evidence_index.query(parent_dir, 'functions')
            leaf = next(item['fact']['functionId'] for item in facts
                        if item['fact'].get('eventType') == 'exact.function'
                        and item['fact']['symbol']['function'] == 'AuditorExactLeaf')
            history = storage.history_for_function(leaf)
            self.assertEqual(history['observed_runs'], 2)
            self.assertEqual(history['observed_tests'], 2)
            self.assertEqual(history['total_calls'], 12, 'parent projections are not new executions')
