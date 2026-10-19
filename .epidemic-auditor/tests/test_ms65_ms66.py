"""Linux-runnable MS6.5/6.6 known-good, known-bad and tamper controls."""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.analyzers.causality import causal_graph, action_windows
from ep_auditor.analyzers.diagnostics import analyze
from ep_auditor.analyzers.sanitizer import parse_stderr, parse_verifier_xml
from ep_auditor.collectors.sanitizer import publish_sanitizer
from ep_auditor.collectors.telemetry import capture_streams
from ep_auditor.collectors.telemetry_pipeline import publish_telemetry, telemetry_complete
from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.lineage import EvidenceLineage


def event(kind, idx, tid=1, corr=None, **extra):
    result = dict(schemaVersion=1, type=kind, event_id='e' + str(idx), timestamp_ns=100 + idx * 10,
                  pid=71, process_epoch=1, tid=tid, clock_domain='monotonic_trace_ns', provenance='engine_probe')
    if corr is not None:
        result['correlation_id'] = corr
    result.update(extra)
    return result


def diag(kind, idx, **extra):
    row = dict(schemaVersion=1, type=kind, timestamp_ns=100 + idx * 10,
               pid=71, process_epoch=1, tid=1, clock_domain='monotonic_trace_ns', provenance='engine_probe')
    row.update(extra)
    return row


class CausalAnalyzerTests(unittest.TestCase):
    def test_physics_fanout_keeps_unique_producer_and_io_cancel_is_terminal(self):
        rows = [event('physics.dispatch', 1, 1, 'contact'),
                event('physics.callback', 2, 2, 'contact'), event('physics.callback', 3, 3, 'contact'),
                event('io.submit', 4, 1, 'read'), event('io.cancel', 5, 2, 'read')]
        graph = causal_graph(rows, producer_complete=True)
        self.assertTrue(graph['complete'])
        self.assertEqual({(row['source'], row['target']) for row in graph['edges']}, {('e1','e2'),('e1','e3'),('e4','e5')})
        self.assertFalse(causal_graph(rows + [event('physics.dispatch', 6, 1, 'contact')], producer_complete=True)['complete'])
        self.assertFalse(causal_graph(rows + [event('io.complete', 6, 1, 'read')], producer_complete=True)['complete'])

    def test_explicit_parent_connects_families_and_cancel_is_a_terminal(self):
        rows = [event('job.submit', 1, 1, 'j'),
                event('job.execute', 2, 2, 'j', parent_event_id='e1'),
                event('event.publish', 3, 2, 'q', parent_event_id='e2'),
                event('event.handle', 4, 1, 'q', parent_event_id='e3'),
                event('state.request', 5, 1, 's', parent_event_id='e4'),
                event('state.change', 6, 1, 's', parent_event_id='e5'),
                event('job.submit', 7, 1, 'cancelled'), event('job.cancel', 8, 2, 'cancelled')]
        graph = causal_graph(rows, producer_complete=True)
        self.assertTrue(graph['complete'])
        self.assertEqual({(x['source'], x['target']) for x in graph['edges']},
                         {('e1','e2'),('e2','e3'),('e3','e4'),('e4','e5'),('e5','e6'),('e7','e8')})
        self.assertFalse(causal_graph(rows[1:], producer_complete=True)['complete'])
        self.assertFalse(causal_graph(rows + [event('job.execute', 9, 2, 'cancelled')], producer_complete=True)['complete'])
        with self.assertRaises(EvidenceError):
            causal_graph(rows, producer_complete='true')

    def test_real_cross_thread_chain_and_unrelated_events_not_connected(self):
        events = [event('job.submit', 1, 2, 'job-7', job_id='job-7'),
                  event('event.publish', 2, 1, 'event-10'),
                  event('job.execute', 3, 4, 'job-7', job_id='job-7'),
                  event('event.handle', 4, 9, 'event-10'),
                  event('input.accepted', 5)]
        result = causal_graph(events, producer_complete=True)
        self.assertTrue(result['complete'], result['problems'])
        self.assertEqual({(edge['source'], edge['target']) for edge in result['edges']},
                         {('e1', 'e3'), ('e2', 'e4')})
        self.assertEqual(len(result['nodes']), 5)
        self.assertNotIn(('e4', 'e5'), {(x['source'], x['target']) for x in result['edges']})

    def test_missing_corr_collision_wrong_time_and_missing_producer_are_not_guessed(self):
        cases = [([event('job.submit', 1, 1, 'x')], 'missing_consumer'),
                 ([event('job.execute', 1, 1, 'x')], 'missing_producer'),
                 ([event('job.submit', 1, 1, 'x'), event('job.submit', 2, 1, 'x'),
                   event('job.execute', 3, 2, 'x')], 'ambiguous'),
                 ([event('job.execute', 1, 2, 'x'), event('job.submit', 2, 1, 'x')], 'backwards_causality')]
        for rows, cause in cases:
            with self.subTest(cause=cause):
                report = causal_graph(rows, producer_complete=True)
                self.assertFalse(report['complete'])
                self.assertIn(cause, report['problems'])
                self.assertFalse(report['edges'])

    def test_pid_reuse_is_not_merged_into_an_explicit_edge(self):
        first = event('job.submit', 1, 1, 'x')
        second = event('job.execute', 2, 2, 'x', process_epoch=2)
        result = causal_graph([first, second], producer_complete=True)
        self.assertFalse(result['complete'])
        self.assertEqual(result['edges'], [])
        self.assertIn('missing_consumer', result['problems'])
        self.assertIn('missing_producer', result['problems'])

    def test_corruption_and_provenance_forgery_rejected(self):
        values = [event('job.submit', 1, 1, 'x'), event('job.execute', 2, 2, 'x')]
        for broken in [dict(values[0], provenance='external'), dict(values[0], timestamp_ns=-1),
                       dict(values[0], correlation_id=None), dict(values[0], pid=0)]:
            with self.subTest(broken=broken), self.assertRaises(EvidenceError):
                causal_graph([broken], producer_complete=True)
        with self.assertRaisesRegex(EvidenceError, 'duplicate'):
            causal_graph([values[0], dict(values[1], event_id=values[0]['event_id'])], producer_complete=True)
        self.assertFalse(causal_graph(values, producer_complete=False)['complete'])

    def test_action_scenario_and_rolling_boundaries(self):
        rows = [event('action.start', 1, action_id='a1'), event('scenario.start', 2, scenario_id='s1'),
                event('capture.trigger', 3, trigger_id='t1'), event('scenario.end', 4, scenario_id='s1'),
                event('action.end', 5, action_id='a1')]
        report = action_windows(rows, producer_complete=True, pre_ns=20, post_ns=20)
        self.assertTrue(report['complete'])
        self.assertEqual(len(report['actions']), 1)
        self.assertEqual(len(report['scenarios']['s1']), 1)
        self.assertEqual(report['triggers'][0]['available_pre_events'], 2)
        with self.assertRaises(EvidenceError):
            action_windows(rows, producer_complete=True, pre_ns=100_000_000_000)
        self.assertFalse(action_windows(rows[:2], producer_complete=True)['complete'])


class DiagnosticAnalyzerTests(unittest.TestCase):
    def test_verifier_command_bounds_elevation_and_reports_unverified_timeout(self):
        from unittest.mock import patch
        from ep_auditor.collectors.sanitizer import _verifier_command

        tool = Path('C:/Windows/System32/appverif.exe')
        with patch.dict(os.environ, {'SystemRoot': 'C:/Windows'}), \
                patch('ep_auditor.collectors.sanitizer.subprocess.CREATE_NO_WINDOW', 0, create=True), \
                patch('ep_auditor.collectors.sanitizer.subprocess.run') as invoke:
            invoke.return_value = subprocess.CompletedProcess([], 0, b'', b'')
            _verifier_command(tool, '-enable', 'Heaps', 'Leak', '-for', 'EpidemicAuditorShadowFixture.exe')
            command = invoke.call_args.args[0]
            self.assertEqual(invoke.call_args.kwargs['timeout'], 150)
            self.assertIn('-Verb RunAs', command[-1])
            self.assertIn('WaitForExit(120000)', command[-1])
            self.assertIn('$p.Kill()', command[-1])
            self.assertIn('EpidemicAuditorShadowFixture.exe', command[-1])
            invoke.return_value = subprocess.CompletedProcess([], 1, b'', b'UAC denied')
            with self.assertRaisesRegex(EvidenceError, 'UAC denied'):
                _verifier_command(tool, '-enable', 'Heaps')
            invoke.side_effect = subprocess.TimeoutExpired(command, 150)
            with self.assertRaisesRegex(EvidenceError, 'activation and cleanup are NOT VERIFIED'):
                _verifier_command(tool, '-enable', 'Heaps')

    def test_native_timeout_is_forwarded_without_consuming_debuggee_arguments(self):
        from ep_auditor.collectors.modules import _observer_command
        command, timeout = _observer_command(Path('game.exe'), ['--smoke'], Path('observer.exe'), False, 30)
        self.assertEqual(command, ['observer.exe', '--exceptions', '--timeout-ms', '30000', 'game.exe', '--smoke'])
        self.assertEqual(timeout, 40)
        for invalid in (True, 0, -1, 61, float('nan'), float('inf'), '30'):
            with self.subTest(invalid=invalid), self.assertRaises(EvidenceError):
                _observer_command(Path('game.exe'), [], Path('observer.exe'), False, invalid)
        command, timeout = _observer_command(Path('EpidemicAuditorShadowFixture.exe'), ['verifier-good'],
                                              Path('observer.exe'), True, 30)
        self.assertNotIn('--timeout-ms', command)
        self.assertEqual(timeout, 120)

    def test_verifier_long_budget_is_bounded_and_never_available_to_general_observation(self):
        from ep_auditor.collectors.modules import _observer_execution_plan

        fixture = Path('EpidemicAuditorShadowFixture.exe')

        self.assertEqual(
            _observer_execution_plan(fixture, ['verifier-good'], True),
            ('--exceptions-verifier', 120)
        )

        self.assertEqual(
            _observer_execution_plan(fixture, ['leak-bad'], True),
            ('--exceptions-verifier', 120)
        )

        self.assertEqual(
            _observer_execution_plan(fixture, ['handled'], False),
            ('--exceptions', 25)
        )

        for path, args, scope in (
            (Path('Epidemic.exe'), ['verifier-good'], True),
            (fixture, ['handled'], True),
            (fixture, ['verifier-good', 'extra'], True),
            (fixture, ['verifier-good'], 'yes'),
        ):
            with self.subTest(path=path, args=args, scope=scope):
                with self.assertRaises(EvidenceError):
                    _observer_execution_plan(path, args, scope)

    def test_disk_irp_pairing_reassembly_reuse_and_negative_controls(self):
        from ep_auditor.analyzers.io import disk_requests
        def observed(kind, time, ordinal, **fields):
            return {'kind': kind, 'timestamp_ns': time, 'rawOrdinal': ordinal,
                    'provenance': 'external', 'clock_domain': 'etl_relative_trace_ns',
                    'fields': {'Process Name ( PID)': 'fixture (71)', 'ThreadID': '1', **fields}}
        start = observed('P-Start', 0, 0)
        request = observed('DiskReadInit', 10, 1, IrpPtr='0x123')
        done = observed('DiskRead', 30, 2, IrpPtr='0x123', IOSize='0x200')
        reused = observed('DiskReadInit', 40, 3, IrpPtr='0x123')
        reused_done = observed('DiskRead', 50, 4, IrpPtr='0x123', IOSize='0x100')
        # Independent ETW buffers need deterministic clock reassembly, not
        # guessed adjacent row pairing. Explicit IRP lifetime remains required.
        report = disk_requests([done, reused_done, request, start, reused], complete=True, main_thread_id=1)
        self.assertTrue(report['complete'])
        self.assertEqual([row['latency_ns'] for row in report['disk_requests']], [20,10])
        self.assertTrue(all(row['main_thread'] for row in report['disk_requests']))
        for bad in ([start,done], [start,request], [start,request,dict(request,rawOrdinal=5),done],
                    [start,request,dict(done,timestamp_ns=5)],
                    [start,request,observed('P-End',20,3),done],
                    [request,done], [start,request,observed('DiskRead',30,2,IrpPtr='0x456',IOSize='0x200')]):
            self.assertFalse(disk_requests(bad, complete=True)['complete'])
        self.assertFalse(disk_requests([start,request,done],complete=False)['complete'])
        self.assertFalse(disk_requests([],complete=True)['complete'])

    def test_pe_import_inventory_is_static_and_rejects_truncation(self):
        from ep_auditor.collectors.modules import parse_pe_imports
        good = b'Format: COFF-x86-64\nImport {\n Name: KERNEL32.dll\n Symbol: ReadFile (123)\n}\n'
        report = parse_pe_imports(good)
        self.assertFalse(report['runtime_calls_observed'])
        self.assertEqual(report['imports'][0]['symbols'][0]['name'], 'ReadFile')
        for bad in (good.replace(b'}', b''), good.replace(b'COFF-x86-64', b'ELF64'),
                    good.replace(b' Name: KERNEL32.dll\n', b'')):
            with self.assertRaises(EvidenceError):
                parse_pe_imports(bad)

    def test_verifier_session_binding_and_empty_export_is_not_clean(self):
        data = b'<avrf:logfile xmlns:avrf="Application Verifier"><avrf:logSession PID="123" Version="2"/></avrf:logfile>'
        report = parse_verifier_xml(data, expected_pid=123)
        self.assertEqual(report['candidates'], [])
        self.assertFalse(report['activation_verified'])
        self.assertEqual(report['memory_safety_verdict'], 'UNKNOWN')
        for invalid in (data.replace(b'123', b'124'), data.replace(b'logSession', b'unknown'),
                        b'<!DOCTYPE root><root/>', b'<root/>', '<!DOCTYPE root><root/>'.encode('utf-16')):
            with self.assertRaises(EvidenceError):
                parse_verifier_xml(invalid, expected_pid=123)

    def test_measured_memory_io_and_exception_metrics(self):
        rows = [diag('allocation', 1, address=4096, size_bytes=64, event_id='a1',
                     function_id='allocate', action_id='pickup', stack=[4096]),
                diag('allocation', 2, address=8192, size_bytes=16, event_id='a2'),
                diag('deallocation', 3, address=4096, size_bytes=64, event_id='f1', tid=2),
                diag('io.read', 4, operation_id='read1', bytes=20, duration_ns=900,
                     synchronous=True, main_thread_id=1, function_id='load'),
                diag('io.read', 5, operation_id='read2', bytes=10, tid=2,
                     synchronous=False, main_thread_id=1, function_id='load'),
                diag('exception.first_chance', 6, code=42, address=9, cpu_ns=100),
                diag('exception.first_chance', 7, code=42, address=9)]
        report = analyze(rows, complete=True)
        memory = report['allocation_metrics']
        self.assertEqual(memory['peak_observed_bytes_by_process'][0]['bytes'], 80)
        self.assertEqual(memory['lifetimes'][0]['duration_ns'], 20)
        self.assertEqual(memory['by_action']['pickup'], {'count': 1, 'bytes': 64})
        self.assertFalse(memory['whole_heap_observed'])
        self.assertEqual(report['live_allocation_candidates'], 1)
        self.assertEqual(report['leak_verdict'], 'UNKNOWN')
        self.assertEqual(report['io_by_function']['load']['synchronous_main_thread'], 1)
        self.assertEqual(report['io_by_function']['load']['duration_samples'], 1)
        self.assertEqual(report['exception_codes']['42'],
                         {'count': 2, 'measured_cpu_ns': 100, 'cpu_samples': 1})

    def test_cross_thread_allocator_order_and_identity_isolation(self):
        rows = [diag('deallocation', 3, address=4096, size_bytes=16, tid=2),
                diag('allocation', 1, address=4096, size_bytes=16, tid=1),
                diag('allocation', 2, address=4096, size_bytes=8, pid=72)]
        report = analyze(rows, complete=True)
        self.assertEqual(report['findings'], [])
        self.assertEqual(report['allocation_metrics']['lifetimes'][0]['duration_ns'], 20)
        self.assertEqual(report['live_allocation_candidates'], 1)
        for invalid in (dict(rows[1], stack=[True]), dict(rows[1], action_id='invalid action'),
                        diag('io.read', 1, operation_id='read', synchronous='true')):
            with self.assertRaises(EvidenceError):
                analyze([invalid], complete=True)
        with self.assertRaises(EvidenceError):
            analyze(rows, complete='true')

    def test_allocation_lifecycle_is_candidate_not_synthetic_leak_verdict(self):
        rows = [diag('allocation', 1, address=0x123, size_bytes=12),
                diag('deallocation', 2, address=0x123, size_bytes=12),
                diag('memory.error', 3, error_kind='use_after_free')]
        report = analyze(rows, complete=True)
        self.assertTrue(report['complete'])
        self.assertEqual(report['live_allocation_candidates'], 0)
        self.assertEqual(report['leak_verdict'], 'UNKNOWN')
        self.assertEqual(report['findings'][0]['status'], 'reported_not_independently_verified')

    def test_exception_first_chance_not_mislabeled_unhandled(self):
        report = analyze([diag('exception.first_chance', 1, code=0xC0000005, address=123)], complete=True)
        self.assertEqual(report['handled_exception_verdict'], 'UNKNOWN')
        with self.assertRaises(EvidenceError):
            analyze([diag('exception.first_chance', 1, code=1, address=2, handled=True)], complete=True)

    def test_replayed_diagnostic_event_and_oversize_integer_rejected(self):
        row = diag('allocation', 1, address=0x123, size_bytes=3, event_id='pid:7:1')
        with self.assertRaisesRegex(EvidenceError, 'duplicate'):
            analyze([row, row], complete=True)
        with self.assertRaises(EvidenceError):
            analyze([dict(row, size_bytes=1 << 90)], complete=True)
        with self.assertRaises(EvidenceError):
            analyze([diag('io.read', 1, operation_id='invalid operation')], complete=True)

    def test_stderr_sanitizer_candidate_never_proves_real_violation(self):
        report = parse_stderr(b'==123==ERROR: AddressSanitizer: heap-use-after-free on address 0x12\n')
        self.assertEqual(report['candidates'][0]['kind'], 'heap-use-after-free')
        self.assertFalse(report['candidates'][0]['independent_validation'])
        self.assertEqual(report['memory_safety_verdict'], 'UNKNOWN')
        self.assertFalse(parse_stderr(b'All tests passed')['candidates'])

    def test_api_file_io_and_untrusted_external_claim(self):
        rows = [diag('io.read', 1, operation_id='i1', bytes=100, duration_ns=200),
                diag('api.call', 2, name='ReadFile', family='file')]
        self.assertEqual(analyze(rows, complete=False)['counts']['io.read'], 1)
        with self.assertRaises(EvidenceError):
            analyze([diag('api.call', 1, name='ReadFile', family='file', provenance='external')], complete=True)
        with self.assertRaises(EvidenceError):
            analyze([diag('io.write', 1, operation_id='i1', bytes=-1)], complete=True)


class CapturePipelineTests(unittest.TestCase):
    def test_interactive_event_log_pages_selects_action_and_rejects_tampering(self):
        from ep_auditor.ui.main_window import AuditorWindow
        from ep_auditor.core.events import EventBus
        from ep_auditor.core.run_manager import RunManager
        from ep_auditor.core.storage import RunStorage
        from test_core import AuditorCoreTests
        import time
        rows = [event('action.start', 1, action_id='a', name='Pickup')]
        rows += [event('state.changed', i, action_id='a') for i in range(2, 61)]
        rows += [event('action.end', 61, action_id='a')]
        self.write_stream('causal', rows)
        capture_streams(self.root, 'causal')
        self.seal()
        publish_telemetry(self.root)
        config, _ = AuditorCoreTests()._config(self.root)
        storage, events = RunStorage(config), EventBus()
        window = AuditorWindow(config, RunManager(config, storage, events), storage, events)
        window.root.withdraw()
        self.addCleanup(window._on_close)
        def wait():
            deadline = time.monotonic() + 30
            while window.trace_future is not None and time.monotonic() < deadline:
                window.root.update()
                window._poll_trace()
                time.sleep(.01)
            self.assertIsNone(window.trace_future, 'UI query timed out')
        window._request_trace(self.root, reset=True)
        wait()
        self.assertIsNotNone(window.trace_result, window.trace_status.get())
        self.assertTrue(window.trace_result[0]['complete'], window.trace_status.get())
        self.assertEqual(len(window.trace_tree.get_children()), 50,
                         (window.trace_status.get(), window.trace_result[0]['total']))
        window._trace_page(50)
        wait()
        self.assertEqual(len(window.trace_tree.get_children()), 11)
        window.trace_choice.current(1)
        window.trace_choice.event_generate('<<ComboboxSelected>>')
        window.root.update()
        wait()
        self.assertEqual(window.trace_result[1]['action']['id'], 'a')
        window.trace_tree.selection_set('0')
        window._show_trace_event()
        self.assertIn('rawReference', window.trace_details.get('1.0', 'end'))
        window.trace_category.set('state_changes')
        window._trace_detail_page(0, reset=True)
        detail = json.loads(window.trace_details.get('1.0', 'end'))
        self.assertEqual(detail['total'], 59)
        self.assertEqual(len(detail['records']), 50)
        window._trace_detail_page(50)
        self.assertEqual(len(json.loads(window.trace_details.get('1.0', 'end'))['records']), 9)
        captured = self.root / 'raw/causal/stream-0.jsonl'
        self.assertTrue(captured.is_file())
        import stat
        captured.chmod(stat.S_IREAD | stat.S_IWRITE)  # Simulate tampering of this test-owned sealed raw file.
        captured.write_bytes(b'forged')
        window._request_trace(self.root, reset=True)
        wait()
        self.assertIsNone(window.trace_result)
        self.assertEqual(window.trace_tree.get_children(), ())
        self.assertIn('Evidence unavailable:', window.trace_status.get())

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.raw = RawEvidence(self.root)

    def write_stream(self, family, events, *, dropped=0, health=True, corrupt=False, emitted=None):
        # Production capture names are diagnostic-capture and causal-capture.
        directory = self.root / ('diagnostic-capture' if family == 'diagnostics' else 'causal-capture')
        directory.mkdir(parents=True, exist_ok=True)
        body = b'{bad JSON}\n' if corrupt else b''.join((json.dumps(row) + '\n').encode() for row in events)
        (directory / 'p.jsonl').write_bytes(body)
        if health:
            (directory / 'p.health.json').write_text(json.dumps({
                'schemaVersion': 1, 'emitted': len(events) if emitted is None else emitted,
                'dropped': dropped, 'complete': True}), encoding='utf-8')

    def seal(self):
        return self.raw.seal({'run_id': 'RunId:' + str(uuid.uuid4()), 'instrumentation_profile': 'audit-deep',
                              'source_fingerprints': {'fixture': digest(Path(__file__))},
                              'binary_fingerprints': {'python': digest(Path(sys.executable))},
                              'analyzer_version': '1'})

    def test_genuine_transport_publishes_derived_graph_and_metrics(self):
        self.write_stream('causal', [event('job.submit', 1, 1, 'c'), event('job.execute', 2, 3, 'c')])
        self.write_stream('diagnostics', [diag('io.read', 1, operation_id='i', bytes=8)])
        capture_streams(self.root, 'causal')
        capture_streams(self.root, 'diagnostics')
        manifest = self.seal()
        self.assertTrue(manifest['capture_complete'])
        publish_telemetry(self.root)
        lineage = EvidenceLineage(self.root)
        self.assertTrue(lineage.verify('graphs', 'causal/dag.json')['records'][0]['complete'])
        self.assertEqual(lineage.verify('metrics', 'diagnostics/report.json')['records'][0]['counts']['io.read'], 1)
        self.assertTrue(lineage.verify('metrics', 'causal/actions.json')['completeness']['complete'])

    def test_cached_diagnostic_counts_cannot_certify_themselves(self):
        self.write_stream('diagnostics', [diag('io.read', 1, operation_id='i', bytes=8)])
        capture_streams(self.root, 'diagnostics')
        self.seal()
        publish_telemetry(self.root)
        from ep_auditor.collectors.telemetry_pipeline import telemetry_complete
        self.assertTrue(telemetry_complete(self.root))
        path = self.root / 'metrics/diagnostics/report.json'
        body = json.loads(path.read_text(encoding='utf-8'))
        body['records'][0]['counts']['io.read'] = 99
        path.write_text(json.dumps(body), encoding='utf-8')
        self.assertFalse(telemetry_complete(self.root))

    def test_corrupt_missing_health_and_dropped_producer_cannot_be_complete(self):
        for variant in ('missing', 'corrupt', 'dropped', 'count'):
            with self.subTest(variant=variant), tempfile.TemporaryDirectory() as path:
                root = Path(path)
                case = CapturePipelineTests(methodName='test_genuine_transport_publishes_derived_graph_and_metrics')
                case.root = root
                case.raw = RawEvidence(root)
                case.write_stream('causal', [event('job.submit', 1, 1, 'x')], health=variant != 'missing',
                                  corrupt=variant == 'corrupt', dropped=int(variant == 'dropped'),
                                  emitted=99 if variant == 'count' else None)
                capture_streams(root, 'causal')
                self.assertFalse(RawEvidence(root)._catalog()['files']['causal/stream-0.jsonl']['complete'])
                case.seal()
                publish_telemetry(root)
                graph = EvidenceLineage(root).verify('graphs', 'causal/dag.json')
                self.assertFalse(graph['completeness']['complete'])
                self.assertFalse(graph['records'][0]['complete'])

    def test_external_raw_stderr_preserved_through_lineage(self):
        text = b'ERROR: AddressSanitizer: heap-use-after-free\n'
        self.raw.append('process/stderr.bin', text, collector='external_stdio', version='1',
                        start_ns=0, end_ns=0, health={'capacity': 1024, 'observed': len(text),
                                                    'dropped': 0, 'incomplete_reason': None})
        self.seal()
        publish_sanitizer(self.root)
        report = EvidenceLineage(self.root).verify('normalized', 'diagnostics/sanitizer_stderr.json')
        self.assertEqual(report['records'][0]['candidate_count'], 1)
        self.assertEqual(report['records'][0]['confidence'], 'heuristic')

    def test_multi_producer_order_is_not_a_false_clock_regression(self):
        directory = self.root / 'causal-capture'
        directory.mkdir()
        # Filename sorting is not time sorting. Workers can flush independent files
        # in either order, including events that share the same real thread.
        for filename, row in [('a.jsonl', event('job.execute', 3, 2, 'j')),
                              ('b.jsonl', event('job.submit', 1, 2, 'j'))]:
            (directory / filename).write_text(json.dumps(row) + '\n')
            (directory / filename.replace('.jsonl', '.health.json')).write_text(
                json.dumps({'schemaVersion': 1, 'emitted': 1, 'dropped': 0, 'complete': True}))
        capture_streams(self.root, 'causal')
        self.assertTrue(self.seal()['capture_complete'])
        publish_telemetry(self.root)
        graph = EvidenceLineage(self.root).verify('graphs', 'causal/dag.json')['records'][0]
        self.assertTrue(graph['complete'], graph.get('problems'))
        self.assertEqual(len(graph['edges']), 1)

    def test_live_rolling_ring_retains_physical_pre_post_and_reports_loss(self):
        import time
        from ep_auditor.collectors.telemetry import RollingCapture
        directory = self.root / 'causal-capture'
        directory.mkdir(exist_ok=True)
        source = directory / 'live.jsonl'
        ring = RollingCapture(self.root, {'pre_seconds': 1, 'post_seconds': 1, 'capacity_bytes': 65536})
        before = event('input.accepted', 1)
        before['timestamp_ns'] = time.perf_counter_ns()
        first = (json.dumps(before) + '\n').encode()
        source.write_bytes(first)
        ring.poll(final=True)
        ring.trigger('memory_error', time.perf_counter_ns(), reason='regression control')
        after = event('input.accepted', 2)
        after['timestamp_ns'] = time.perf_counter_ns()
        second = (json.dumps(after) + '\n').encode()
        with source.open('ab') as stream:
            stream.write(second)
        ring.finish(process_exited=True)
        self.assertFalse(self.seal()['capture_complete'])
        report = json.loads(RawEvidence(self.root).read_bytes('rolling/probes.json'))
        snapshot = report['snapshots'][0]
        self.assertTrue(report['retained_bytes_complete'])
        self.assertFalse(report['complete'], 'Short run must not certify the full requested pre/post window')
        self.assertEqual([row['ordinal'] for row in snapshot['records']], [0, 1])
        self.assertEqual([row['source_line'].encode() for row in snapshot['records']], [first, second])
        self.assertEqual(snapshot['post_coverage'], 'process_ended')
        with self.assertRaises(EvidenceError):
            RollingCapture(self.root, {'pre_seconds': float('nan')})
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            producer = root / 'causal-capture'
            producer.mkdir()
            overflow = RollingCapture(root, {'capacity_bytes': 65536, 'post_seconds': 0})
            rows = []
            for index in range(500):
                row = event('input.accepted', index)
                row['timestamp_ns'] = time.perf_counter_ns()
                rows.append(json.dumps(row) + '\n')
            (producer / 'over.jsonl').write_text(''.join(rows), encoding='utf-8')
            overflow.poll(final=True)
            self.assertGreater(overflow.dropped, 0)

    def test_diagnostic_finding_ids_bind_actual_source_and_explicit_cause(self):
        self.write_stream('diagnostics', [diag('memory.error', 1, error_kind='use_after_free',
                           event_id='memory-1', action_id='a', parent_event_id='cause-1')])
        capture_streams(self.root, 'diagnostics')
        self.seal()
        publish_telemetry(self.root)
        report = EvidenceLineage(self.root).verify('metrics', 'diagnostics/report.json')['records'][0]
        finding = report['findings'][0]
        self.assertTrue(finding['finding_id'].startswith('FindingId:'))
        self.assertEqual(finding['event_id'], 'memory-1')
        self.assertEqual(finding['parent_event_id'], 'cause-1')
        self.assertEqual(finding['source_ordinal'], 0)
        self.assertEqual(finding['status'], 'reported_not_independently_verified')
        publish_telemetry(self.root) # Cached replay must reproduce the same ID.
        replay = EvidenceLineage(self.root).verify('metrics', 'diagnostics/report.json')['records'][0]
        self.assertEqual(replay['findings'][0]['finding_id'], finding['finding_id'])

    def test_rolling_compiler_ring_detects_overwrite_and_ignores_active_writer(self):
        import struct
        import time
        from ep_auditor.collectors.telemetry import RollingCapture
        from ep_auditor.collectors.exact_trace import EVENT
        directory = self.root / 'rolling-exact-capture'
        directory.mkdir()
        layout = struct.Struct('<8s7Q')
        source = directory / 'events.bin'
        total, capacity = 4100, 4096
        stamp = time.perf_counter_ns()
        payload = bytearray(layout.size + capacity * EVENT.size)
        payload[:layout.size] = layout.pack(b'EPROLL01', 3, total, capacity, 1_000_000_000, 71, 1, 4096)
        for ordinal in range(total - capacity, total):
            offset = layout.size + (ordinal % capacity) * EVENT.size
            payload[offset:offset + EVENT.size] = EVENT.pack(ordinal, stamp + ordinal, 0, 0,
                8192, 4096, 1, 71, 1, 1 if ordinal % 2 == 0 else 2, *([0] * 7))
        source.write_bytes(payload)
        ring = RollingCapture(self.root, {'pre_seconds': 1})
        ring._poll_exact()
        self.assertFalse(ring.exact_sources)
        self.assertEqual(ring.dropped, 0)
        payload[:layout.size] = layout.pack(b'EPROLL01', 4, total, capacity, 1_000_000_000, 71, 1, 4096)
        source.write_bytes(payload)
        ring._poll_exact()
        self.assertEqual(ring.dropped, 4)
        self.assertEqual(ring.exact_sources[source], total)
        ring._poll_exact()
        self.assertEqual(ring.dropped, 4, 'Same compiler events must not be counted twice')

    def test_rolling_post_window_requires_observed_live_process(self):
        import time
        from ep_auditor.collectors.telemetry import RollingCapture
        ring = RollingCapture(self.root, {'pre_seconds': 0, 'post_seconds': 0.02})
        ring.trigger('crash', time.perf_counter_ns(), reason='controlled termination boundary')
        time.sleep(0.03) # Delayed transport cleanup must not invent a post-crash window.
        ring.finish(process_exited=True)
        self.assertFalse(self.seal()['capture_complete'])
        report = json.loads(RawEvidence(self.root).read_bytes('rolling/probes.json'))
        self.assertFalse(report['complete'])
        self.assertEqual(report['snapshots'][0]['post_coverage'], 'process_ended')

    def test_incomplete_causal_edges_cannot_certify_actions(self):
        self.write_stream('causal', [event('action.start', 1, action_id='a'),
                                     event('job.submit', 2, corr='missing'),
                                     event('action.end', 3, action_id='a')])
        capture_streams(self.root, 'causal')
        self.assertTrue(self.seal()['capture_complete'])
        publish_telemetry(self.root)
        lineage = EvidenceLineage(self.root)
        self.assertFalse(lineage.verify('graphs', 'causal/dag.json')['records'][0]['complete'])
        self.assertFalse(lineage.verify('metrics', 'causal/actions.json')['records'][0]['complete'])

    def test_event_log_pages_sealed_records_and_selects_actual_action(self):
        from ep_auditor.analyzers.causality import query_event_log
        self.write_stream('causal', [event('action.start', 1, action_id='a', name='Pickup'),
                                   event('action.end', 2, action_id='a')])
        capture_streams(self.root, 'causal')
        self.seal()
        publish_telemetry(self.root)
        first = query_event_log(self.root, limit=1)
        second = query_event_log(self.root, offset=1, limit=1, action_id='a')
        self.assertTrue(first['complete'])
        self.assertEqual(first['actions'][0]['name'], 'Pickup')
        self.assertEqual(first['run_id'], first['actions'][0]['run_id'])
        self.assertEqual(first['total'], 2)
        self.assertEqual(first['events'][0]['event_id'], 'e1')
        self.assertEqual(second['events'][0]['event_id'], 'e2')
        self.assertIn('rawReference', second['events'][0])
        for options in ({'offset': -1}, {'limit': 201}, {'action_id': 'absent'}):
            with self.assertRaises(EvidenceError):
                query_event_log(self.root, **options)

    def test_unresolved_diagnostic_call_cannot_certify_action_query(self):
        from ep_auditor.analyzers.causality import query_action
        self.write_stream('causal', [event('action.start', 1, action_id='a'),
                                   event('action.end', 3, action_id='a')])
        self.write_stream('diagnostics', [diag('memory.error', 2, error_kind='unknown',
            event_id='d2', action_id='a', exact_call_ordinal=1, exact_module_base=4096)])
        capture_streams(self.root, 'causal')
        capture_streams(self.root, 'diagnostics')
        self.assertTrue(self.seal()['capture_complete'])
        publish_telemetry(self.root)
        self.assertTrue(EvidenceLineage(self.root).verify('graphs', 'causal/dag.json')['records'][0]['complete'])
        self.assertFalse(query_action(self.root, 'a')['complete'])

    def test_diagnostic_finding_cannot_link_to_another_process_cause(self):
        from ep_auditor.analyzers.causality import query_action
        self.write_stream('causal', [event('action.start', 1, action_id='a'),
                                   event('action.end', 3, action_id='a')])
        self.write_stream('diagnostics', [diag('memory.error', 2, error_kind='unknown',
            event_id='d2', action_id='a', parent_event_id='e1', pid=72)])
        capture_streams(self.root, 'causal')
        capture_streams(self.root, 'diagnostics')
        self.seal()
        publish_telemetry(self.root)
        query = query_action(self.root, 'a')
        self.assertFalse(query['complete'])
        self.assertEqual(query['finding_links'], [])
        self.assertEqual(len(query['unresolved_finding_links']), 1)

    def test_diagnostic_finding_keeps_valid_explicit_cause(self):
        from ep_auditor.analyzers.causality import query_action
        self.write_stream('causal', [event('action.start', 1, action_id='a', trace_id='trace-a'),
                                   event('action.end', 3, action_id='a', trace_id='trace-a')])
        self.write_stream('diagnostics', [diag('memory.error', 2, error_kind='unknown',
            event_id='d2', action_id='a', parent_event_id='e1', trace_id='trace-a')])
        capture_streams(self.root, 'causal')
        capture_streams(self.root, 'diagnostics')
        self.seal()
        publish_telemetry(self.root)
        query = query_action(self.root, 'a')
        self.assertTrue(query['complete'])
        self.assertEqual(query['finding_links'], [{'finding_id': query['findings'][0]['finding_id'], 'cause_event_id': 'e1'}])
        self.assertEqual(query['unresolved_finding_links'], [])

    def test_one_producer_reversed_clock_rejected_even_with_reassembly(self):
        # Sorting separate producers must not hide a physically reversed clock
        # inside one untrusted producer's original event sequence.
        self.write_stream('causal', [event('job.execute', 3, 1, 'z'),
                                     event('job.submit', 1, 1, 'z')])
        capture_streams(self.root, 'causal')
        self.assertFalse(self.seal()['capture_complete'])
        publish_telemetry(self.root)
        self.assertFalse(EvidenceLineage(self.root).verify('graphs', 'causal/dag.json')['records'][0]['complete'])

    def test_actions_reject_reversed_clocks_and_duplicate_event_ids(self):
        good = [event('action.start', 1, 1, action_id='a'), event('action.end', 2, 1, action_id='a')]
        for rows in ([good[1], good[0]], [good[0], dict(good[1], event_id=good[0]['event_id'])]):
            with self.subTest(rows=rows), self.assertRaises(EvidenceError):
                action_windows(rows, producer_complete=True)

    def test_duplicate_health_json_keys_cannot_claim_complete(self):
        self.write_stream('causal', [event('job.submit', 1, 1, 'x')])
        health = self.root / 'causal-capture' / 'p.health.json'
        health.write_text('{"schemaVersion":1,"emitted":1,"emitted":1,"dropped":0,"complete":true}')
        capture_streams(self.root, 'causal')
        self.assertFalse(self.seal()['capture_complete'])

    def test_pass_cannot_be_committed_when_causal_graph_is_incomplete(self):
        from ep_auditor.core.evidence import atomic_json
        from ep_auditor.core.lifecycle import finalize_run, authoritative_run
        from ep_auditor.models import RunRecord
        self.write_stream('causal', [event('job.submit', 1, corr='unpaired')])
        capture_streams(self.root, 'causal')
        identifier = 'RunId:' + str(uuid.uuid4())
        record = RunRecord(run_id=identifier, kind='fixture', title='semantics', capture_mode='Deep',
                           run_dir=self.root, started_utc='2026-10-08T12:00:00+00:00')
        atomic_json(self.root / 'run.json', record.to_dict())
        atomic_json(self.root / 'environment.json', {
            'instrumentation_profile': 'audit-deep', 'engine_source': {'sha256': 'a' * 64},
            'auditor_source': {'sha256': 'b' * 64}, 'auditor_version': 'test'})
        finalize_run(record, 'PASS', {'fixture': True})
        self.assertEqual(record.status, 'INCOMPLETE')
        self.assertEqual(authoritative_run(self.root)['status'], 'INCOMPLETE')

    def test_duplicate_json_key_is_incomplete_not_normalized(self):
        directory = self.root / 'causal-capture'
        directory.mkdir()
        (directory / 'p.jsonl').write_bytes(b'{"schemaVersion":1,"schemaVersion":1,"type":"job.submit"}\n')
        (directory / 'p.health.json').write_text(json.dumps({'schemaVersion': 1, 'emitted': 1,
                                                             'dropped': 0, 'complete': True}))
        capture_streams(self.root, 'causal')
        self.assertFalse(self.seal()['capture_complete'])
        publish_telemetry(self.root)
        self.assertFalse(EvidenceLineage(self.root).verify('graphs', 'causal/dag.json')['completeness']['complete'])

    def test_symlink_capture_dir_is_incomplete(self):
        with tempfile.TemporaryDirectory() as external:
            (self.root / 'causal-capture').symlink_to(external, target_is_directory=True)
            capture_streams(self.root, 'causal')
            self.assertFalse(self.seal()['capture_complete'])

    def test_required_missing_producer_refuses_clean_capture(self):
        capture_streams(self.root, 'causal', required=True)
        self.assertFalse(self.seal()['capture_complete'])

    def test_windows_junction_capture_dir_is_incomplete(self):
        if os.name != 'nt':
            self.skipTest('Windows reparse-point control')
        with tempfile.TemporaryDirectory() as external:
            link = self.root / 'causal-capture'
            result = subprocess.run(['cmd', '/c', 'mklink', '/J', str(link), external],
                                    capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            try:
                capture_streams(self.root, 'causal')
                self.assertFalse(self.seal()['capture_complete'])
            finally:
                # Remove only the junction entry, never recurse into its target.
                link.rmdir()

    def test_health_counter_overflow_is_incomplete(self):
        self.write_stream('diagnostics', [diag('io.read', 1, operation_id='read')], emitted=1 << 80)
        capture_streams(self.root, 'diagnostics')
        self.assertFalse(self.seal()['capture_complete'])

    def test_missing_or_truthy_report_completion_cannot_pass(self):
        self.write_stream('diagnostics', [diag('io.read', 1, operation_id='read')])
        capture_streams(self.root, 'diagnostics')
        self.assertTrue(self.seal()['capture_complete'])
        publish_telemetry(self.root)
        path = self.root / 'metrics/diagnostics/report.json'
        original = json.loads(path.read_bytes())
        self.assertTrue(telemetry_complete(self.root))
        for invalid in (None, 'true', 1, [], False):
            report = json.loads(json.dumps(original))
            if invalid is None:
                report['records'][0].pop('complete')
            else:
                report['records'][0]['complete'] = invalid
            path.write_text(json.dumps(report), encoding='utf-8')
            # Physical input lineage is valid, semantic completeness is not.
            EvidenceLineage(self.root).verify('metrics', 'diagnostics/report.json')
            self.assertFalse(telemetry_complete(self.root), invalid)


class NativeDiagnosticTests(unittest.TestCase):
    root = Path(__file__).resolve().parents[2]

    def binary(self, name):
        path = self.root / 'build/auditor-native/bin' / os.environ.get('EP_AUDITOR_NATIVE_CONFIG', 'Release') / (name + '.exe')
        self.assertTrue(path.is_file(), 'mandatory MS6.5 native fixture/helper must be built: ' + str(path))
        return path

    def observe(self, variant):
        # Exercise ordinary ManagedProcessRunner ownership/capture, without path
        # substitutions. The observer reports target exit separately from its own.
        from test_core import AuditorCoreTests
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.storage import RunStorage
        helper = self.binary('EpidemicAuditorObservation')
        fixture = self.binary('EpidemicAuditorShadowFixture')
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            config, _ = AuditorCoreTests()._config(directory)
            run = RunStorage(config).new_run('exceptions', 'physical debugger', 'Fast')
            before = (digest(helper), digest(fixture))
            result = ManagedProcessRunner(config).run(
                [str(helper), '--exception-fixture', str(fixture), variant], directory, run.run_dir, 'Fast', timeout=25)
            self.assertEqual(result.exit_code, 0)
            data, record = RawEvidence(run.run_dir).read_capture('process/stdout.bin')
            self.assertTrue(record['complete'])
            self.assertEqual(before, (digest(helper), digest(fixture)))
            report = json.loads(data)
            self.assertTrue(report['complete'])
            self.assertEqual(report['provenance'], 'external')
            self.assertGreater(report['process_epoch'], 0)
            from ep_auditor.core.lifecycle import seal_run_capture
            seal_run_capture(run)
            return report

    def test_physical_scheduler_event_bus_dag_and_context(self):
        from collections import Counter
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture, finalize_run, authoritative_run
        binary = self.root / 'build/audit-deep/EngineBase/Tests/Unit/Release/EpidemicCoreUnitTests.exe'
        self.assertTrue(binary.is_file(), 'mandatory audit-deep Core scenario must be built')
        before = digest(binary)
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('scenario', 'real Scheduler/EventBus chain', 'Deep')
            result = ManagedProcessRunner(config).run([str(binary), '--auditor-causal'], binary.parent,
                                                      run.run_dir, 'Deep', timeout=45)
            self.assertEqual(result.exit_code, 0)
            self.assertEqual(before, digest(binary))
            seal_run_capture(run)
            graph = EvidenceLineage(run.run_dir).verify('graphs', 'causal/dag.json')['records'][0]
            self.assertTrue(graph['complete'], graph['problems'])
            nodes = {row['event_id']: row for row in graph['nodes']}
            counts = Counter(row['type'] for row in nodes.values())
            self.assertEqual({key: counts[key] for key in ('job.submit','job.execute','event.publish','event.handle')},
                             {'job.submit':1,'job.execute':1,'event.publish':1,'event.handle':1})
            self.assertEqual(counts['event.handler.execute'], 2)
            self.assertEqual(counts['state.change'], 3)
            self.assertEqual(len(graph['edges']), 12)
            selected = {kind: next(row for row in nodes.values() if row['type'] == kind)
                        for kind in ('job.submit','job.execute','event.publish','event.handle')}
            self.assertNotEqual(selected['job.submit']['thread'], selected['job.execute']['thread'])
            self.assertEqual(selected['job.execute']['thread'], selected['event.publish']['thread'])
            self.assertEqual(selected['job.submit']['thread'], selected['event.handle']['thread'])
            self.assertEqual(len({row['trace_id'] for row in selected.values()}), 1)
            self.assertTrue(all(row['action_id'] == 'causal-action' for row in selected.values()))
            self.assertTrue(all(row['call_link_status'] == 'explicit_compiler_entry_verified' for row in selected.values()))
            self.assertTrue(all(row.get('call_link_status') == 'explicit_compiler_entry_verified'
                                for row in nodes.values()), 'Every causal node, including boundaries, needs a real compiler CallId')
            self.assertTrue(all(row['call_id'].startswith('CallId:') and row['function_id'].startswith('FunctionId:')
                                and row['module_id'].startswith('ModuleId:') for row in selected.values()))
            actual_calls = {record['callId'] for path in (run.run_dir / 'graphs/exact').glob('*-calls-*.json')
                            for record in EvidenceLineage(run.run_dir).verify('graphs', 'exact/' + path.name)['records']}
            self.assertTrue({row['call_id'] for row in selected.values()} <= actual_calls)
            # The extra state pair shares the trace clock but has no explicit
            # execution parent; it must remain detached from the job chain.
            detached = [row for row in nodes.values() if row['type'] == 'state.request' and not row['parent_event_id']]
            self.assertEqual(len(detached), 1)
            self.assertFalse(any(edge['target'] == detached[0]['event_id'] for edge in graph['edges']))
            from ep_auditor.analyzers.causality import query_action
            allocated_action = query_action(run.run_dir, 'causal-action')
            allocations = allocated_action['allocations']
            self.assertEqual({row['type'] for row in allocations}, {'allocation', 'deallocation'})
            self.assertTrue(all(row['size_bytes'] == 512 for row in allocations))
            self.assertTrue(all(row['call_link_status'] == 'explicit_compiler_entry_verified' for row in allocations))
            self.assertTrue(all(row['call_id'] in actual_calls for row in allocations))
            self.assertTrue(all(row['stack'] and all(type(address) is int and address > 0 for address in row['stack'])
                                for row in allocations))
            diagnostic_rows = EvidenceLineage(run.run_dir).verify('normalized', 'diagnostics/events.json')['records']
            self.assertTrue(diagnostic_rows)
            self.assertTrue(all(row['provenance'] == 'engine_probe' and row['confidence'] == 'declared'
                                for row in diagnostic_rows))
            metrics = EvidenceLineage(run.run_dir).verify('metrics', 'diagnostics/report.json')['records'][0]['allocation_metrics']
            allocated = next(row for row in allocations if row['type'] == 'allocation')
            released = next(row for row in allocations if row['type'] == 'deallocation')
            self.assertEqual(allocated['address'], released['address'])
            lifetime = next(row for row in metrics['lifetimes'] if row['allocation_event_id'] == allocated['event_id'])
            self.assertEqual(lifetime['deallocation_event_id'], released['event_id'])
            self.assertEqual(lifetime['duration_ns'], released['timestamp_ns'] - allocated['timestamp_ns'])
            self.assertGreater(lifetime['duration_ns'], 0)
            self.assertEqual(metrics['by_function'][allocated['function_id']], {'count': 1, 'bytes': 512})
            self.assertEqual(metrics['by_action']['causal-action'], {'count': 1, 'bytes': 512})
            span = max(row['timestamp_ns'] for row in diagnostic_rows) - min(row['timestamp_ns'] for row in diagnostic_rows)
            self.assertAlmostEqual(metrics['rate_per_second'], 1e9 / span)
            self.assertFalse(metrics['whole_heap_observed'])
            finalize_run(run, 'PASS', {})
            self.assertEqual(authoritative_run(run.run_dir)['status'], 'PASS')
            self.assert_physical_action_ui(config, run, 'causal-action')

    def assert_physical_action_ui(self, config, run, action_id):
        from ep_auditor.ui.main_window import AuditorWindow
        from ep_auditor.core.events import EventBus
        from ep_auditor.core.run_manager import RunManager
        from ep_auditor.core.storage import RunStorage
        import time
        storage, events = RunStorage(config), EventBus()
        window = AuditorWindow(config, RunManager(config, storage, events), storage, events)
        try:
            window.notebook.select(window.actions_tab)
            def wait():
                deadline = time.monotonic() + 120
                while window.trace_future is not None and time.monotonic() < deadline:
                    window.root.update()
                    window._poll_trace()
                    time.sleep(.02)
                self.assertIsNone(window.trace_future, 'physical Action UI query timed out')
                self.assertIsNotNone(window.trace_result, window.trace_status.get())
            window._request_trace(run.run_dir, reset=True)
            wait()
            index = next(i for i, a in enumerate(window.trace_result[0]['actions']) if a['id'] == action_id)
            window.trace_choice.current(index + 1)
            window.trace_choice.event_generate('<<ComboboxSelected>>')
            window.root.update()
            wait()
            query = window.trace_result[1]
            self.assertTrue(query['complete'])
            self.assertEqual(query['action']['run_id'], run.run_id)
            self.assertTrue(query['call_tree'])
            self.assertTrue(query['causal_graph']['edges'])
            self.assertTrue(window.trace_tree.get_children())
            window.trace_category.set('function_profile')
            window._trace_detail_page(0, reset=True)
            rendered = json.loads(window.trace_details.get('1.0', 'end'))
            self.assertEqual(rendered['total'], len(query['function_profile']))
            self.assertTrue(rendered['records'])
        finally:
            window._on_close()

    def test_physical_dispatcher_sync_event_and_cancel_context(self):
        from collections import Counter
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        binary = self.root / 'build/audit-deep/EngineBase/Tests/Unit/Release/EpidemicCoreUnitTests.exe'
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('scenario', 'real continuation/cancel chain', 'Deep')
            result = ManagedProcessRunner(config).run([str(binary), '--auditor-continuations'], binary.parent,
                                                      run.run_dir, 'Deep', timeout=45)
            self.assertEqual(result.exit_code, 0)
            seal_run_capture(run)
            graph = EvidenceLineage(run.run_dir).verify('graphs', 'causal/dag.json')['records'][0]
            self.assertTrue(graph['complete'], graph['problems'])
            nodes = {row['event_id']: row for row in graph['nodes']}
            counts = Counter(row['type'] for row in nodes.values())
            self.assertEqual(counts['job.submit'], 2)
            self.assertEqual(counts['job.execute'], 1)
            self.assertEqual(counts['job.cancel'], 1)
            for kind in ('async.submit','async.execute','event.publish','event.handle','event.handler.execute'):
                self.assertEqual(counts[kind], 1, kind)
            selected = {kind: next(row for row in nodes.values() if row['type'] == kind)
                        for kind in ('async.submit','async.execute','event.publish','event.handle','event.handler.execute')}
            self.assertNotEqual(selected['async.submit']['thread'], selected['async.execute']['thread'])
            self.assertTrue(all(row['action_id'] == 'continuation-action' for row in selected.values()))
            self.assertTrue(all(row['call_link_status'] == 'explicit_compiler_entry_verified' for row in selected.values()))
            pairs = {(nodes[edge['source']]['type'], nodes[edge['target']]['type']) for edge in graph['edges']}
            self.assertTrue({('job.execute','async.submit'), ('async.submit','async.execute'),
                             ('async.execute','event.publish'), ('event.handle','event.handler.submit')} <= pairs)

    def test_physical_configured_input_arms_once_without_temporal_input_edges(self):
        from collections import Counter
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        from ep_auditor.analyzers.causality import query_action
        binary = self.root / 'build/audit-deep/EngineBase/Tests/Integration/Release/EpidemicPlatformInputIntegrationTests.exe'
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            config.data.setdefault('capture', {})['arm_next_input'] = {'action_id': 'configured-input', 'name': 'First "valid" input'}
            run = RunStorage(config).new_run('scenario', 'configured real input', 'Deep')
            result = ManagedProcessRunner(config).run([str(binary), '--auditor-input'], binary.parent,
                                                      run.run_dir, 'Deep', timeout=45)
            self.assertEqual(result.exit_code, 0)
            seal_run_capture(run)
            graph = EvidenceLineage(run.run_dir).verify('graphs', 'causal/dag.json')['records'][0]
            self.assertTrue(graph['complete'], graph['problems'])
            counts = Counter(row['type'] for row in graph['nodes'])
            self.assertEqual(counts['input.accepted'], 2)
            self.assertEqual(counts['action.start'], 1)
            self.assertEqual(counts['action.end'], 1)
            inputs = {row['event_id'] for row in graph['nodes'] if row['type'] == 'input.accepted'}
            self.assertFalse(any(row['source'] in inputs and row['target'] in inputs for row in graph['edges']))
            query = query_action(run.run_dir, 'configured-input')
            self.assertTrue(query['complete'])
            self.assertEqual(query['action']['name'], 'First "valid" input')
            self.assertTrue(query['causal_graph']['external_parents'])

    def test_physical_coroutine_and_arm_next_input_action_query(self):
        from collections import Counter
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        from ep_auditor.analyzers.causality import query_action
        binary = self.root / 'build/audit-deep/EngineBase/Tests/Unit/Release/EpidemicCoreUnitTests.exe'
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('scenario', 'real coroutine continuation', 'Deep')
            result = ManagedProcessRunner(config).run([str(binary), '--auditor-coroutine'], binary.parent,
                                                      run.run_dir, 'Deep', timeout=45)
            self.assertEqual(result.exit_code, 0)
            seal_run_capture(run)
            graph = EvidenceLineage(run.run_dir).verify('graphs', 'causal/dag.json')['records'][0]
            self.assertTrue(graph['complete'], graph['problems'])
            counts = Counter(row['type'] for row in graph['nodes'])
            self.assertEqual({kind: counts[kind] for kind in ('async.suspend','async.resume','async.submit','async.execute','input.accepted')},
                             {kind: 1 for kind in ('async.suspend','async.resume','async.submit','async.execute','input.accepted')})
            pair = {row['type']: row for row in graph['nodes'] if row['type'] in {'async.suspend','async.resume'}}
            self.assertNotEqual(pair['async.suspend']['thread'], pair['async.resume']['thread'])
            query = query_action(run.run_dir, 'coroutine-action')
            self.assertTrue(query['complete'])
            self.assertEqual(query['action']['name'], 'Resume on main thread')
            self.assertEqual(query['action']['run_id'], run.run_id)
            self.assertTrue(query['call_tree'])
            self.assertTrue(query['function_profile'])
            self.assertGreaterEqual(len(query['threads']), 2)
            with self.assertRaises(EvidenceError):
                query_action(run.run_dir, 'missing-action')

    def test_physical_rolling_crash_and_long_frame_capture(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        for mode, kind in (('rolling-crash', 'crash'), ('rolling-long-frame', 'long_frame')):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temporary:
                config, _ = AuditorCoreTests()._config(Path(temporary))
                config.data.setdefault('capture', {})['rolling'] = {'pre_seconds': 0.2, 'post_seconds': 0.2}
                run = RunStorage(config).new_run('scenario', mode, 'Deep')
                binary = self.binary('EpidemicAuditorShadowFixture')
                result = ManagedProcessRunner(config).run([str(binary), mode], binary.parent,
                                                          run.run_dir, 'Deep', timeout=15)
                self.assertFalse(result.cancelled)
                self.assertEqual(result.exit_code == 0, mode == 'rolling-long-frame')
                raw = RawEvidence(run.run_dir)
                manifest = seal_run_capture(run)
                # Crash deliberately lacks producer final health. It cannot be a
                # clean full capture, while physical rolling bytes still survive.
                if mode == 'rolling-crash':
                    self.assertFalse(manifest['capture_complete'])
                report = json.loads(raw.read_bytes('rolling/probes.json'))
                self.assertTrue(report['retained_bytes_complete'], report)
                self.assertEqual(report['complete'], mode == 'rolling-long-frame')
                snapshot = next(row for row in report['snapshots'] if row['trigger'] == kind)
                before = [row for row in snapshot['records'] if row['timestamp_ns'] < snapshot['trigger_ns']]
                self.assertGreaterEqual(len(before), 2)
                for row in snapshot['records']:
                    source = run.run_dir / row['source']
                    self.assertEqual(source.read_bytes().splitlines(keepends=True)[row['ordinal']], row['source_line'].encode())

    def test_physical_rolling_compiler_events_are_original_enter_exit_bytes(self):
        import hashlib
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        from ep_auditor.collectors.exact_trace import read_header, EVENT
        binary = self.root / 'build/audit-deep/EngineBase/Tests/Unit/Release/EpidemicCoreUnitTests.exe'
        self.assertTrue(binary.is_file(), binary)
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            config.data.setdefault('capture', {})['rolling'] = {'pre_seconds': 0.2, 'post_seconds': 0.2,
                                                               'capacity_bytes': 8 * 1024 * 1024}
            run = RunStorage(config).new_run('scenario', 'rolling-exact', 'Deep')
            result = ManagedProcessRunner(config).run([str(binary), '--auditor-rolling-exact'], binary.parent,
                                                      run.run_dir, 'Deep', timeout=60)
            self.assertFalse(result.cancelled)
            self.assertEqual(result.exit_code, 0)
            seal_run_capture(run)
            report = json.loads(RawEvidence(run.run_dir).read_bytes('rolling/probes.json'))
            self.assertTrue(report['complete'], {key: value for key, value in report.items() if key != 'snapshots'})
            self.assertEqual(report['exact_availability'], 'observed_compiler_ring')
            snapshot = next(row for row in report['snapshots'] if row['trigger'] == 'long_frame')
            rows = [row for row in snapshot['records'] if row.get('provenance') == 'compiler_instrumentation']
            self.assertTrue(any(row['timestamp_ns'] < snapshot['trigger_ns'] for row in rows))
            self.assertTrue(any(row['timestamp_ns'] > snapshot['trigger_ns'] for row in rows))
            originals = {}
            for path in (run.run_dir / 'raw/exact').glob('*.bin'):
                header = read_header(path)
                originals[header['image_base']] = (path.read_bytes(), header)
            for row in rows:
                original, header = originals[row['producer_base']]
                offset = header['header_bytes'] + row['ordinal'] * EVENT.size
                payload = bytes.fromhex(row['source_bytes_hex'])
                self.assertEqual(payload, original[offset:offset + EVENT.size])
                self.assertEqual(hashlib.sha256(payload).hexdigest(), row['sha256'])

    def test_physical_rolling_assert_contract_memory_and_owned_wait_timeout(self):
        import ctypes
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.CreateEventW.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_wchar_p]
        kernel.CreateEventW.restype = ctypes.c_void_p
        kernel.CloseHandle.argtypes = [ctypes.c_void_p]
        name = 'Local\\EpidemicRollingWait-' + uuid.uuid4().hex
        gate = kernel.CreateEventW(None, True, False, name)
        self.assertTrue(gate)
        try:
            cases = (
                (self.binary('EpidemicAuditorShadowFixture'), ['rolling-assert'], 'assert', False),
                (self.binary('EpidemicAuditorShadowFixture'), ['rolling-contract'], 'contract', False),
                (self.binary('EpidemicAuditorMemoryFixture'), ['uaf'], 'memory_error', False),
                (self.binary('EpidemicAuditorConcurrencyFixture'), ['completion', 'good', name], 'suspected_deadlock', True),
            )
            for binary, arguments, kind, expected_cancelled in cases:
                with self.subTest(trigger=kind), tempfile.TemporaryDirectory() as temporary:
                    config, _ = AuditorCoreTests()._config(Path(temporary))
                    config.data.setdefault('capture', {})['rolling'] = {'pre_seconds': 0.2, 'post_seconds': 0.1}
                    run = RunStorage(config).new_run('scenario', 'rolling-' + kind, 'Deep')
                    result = ManagedProcessRunner(config).run([str(binary), *arguments], binary.parent, run.run_dir,
                        'Deep', timeout=1.0 if expected_cancelled else 15,
                        env_extra={'ASAN_OPTIONS': 'halt_on_error=1:detect_leaks=0:detect_stack_use_after_return=0'})
                    self.assertEqual(result.cancelled, expected_cancelled)
                    self.assertIsInstance(result.exit_code, int)
                    self.assertNotEqual(result.exit_code, 0)
                    seal_run_capture(run)
                    report = json.loads(RawEvidence(run.run_dir).read_bytes('rolling/probes.json'))
                    snapshot = next(row for row in report['snapshots'] if row['trigger'] == kind)
                    self.assertFalse(report['complete'], 'Terminated process cannot provide a post-trigger live window')
                    self.assertEqual(snapshot['post_coverage'], 'process_ended')
                    if kind in {'assert', 'contract'}:
                        self.assertGreaterEqual(len(snapshot['records']), 2)
                    if kind == 'memory_error':
                        self.assertIn(b'AddressSanitizer', (run.run_dir / 'raw/process/stderr.bin').read_bytes())
                    if kind == 'suspected_deadlock':
                        self.assertIn('not proof of deadlock', snapshot['reason'])
        finally:
            kernel.CloseHandle(gate)

    def test_physical_independent_non_ctest_scenarios(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        from ep_auditor.analyzers.causality import query_action
        cases = (
            (self.root / 'build/audit-deep/EngineFramework/build/tests/Release/EpidemicGameFrameworkItemsInventoryTests.exe', '--auditor-pickup', 'pickup'),
            (self.root / 'build/audit-deep/EngineFramework/build/tests/Release/EpidemicGameFrameworkCombatTests.exe', '--auditor-combat', 'combat'),
            (self.root / 'build/audit-deep/EngineFramework/build/tests/Release/EpidemicExtendedGameplayIntegrationSaveParticipantTests.exe', '--auditor-save-load', 'save-load'),
            (self.root / 'build/audit-deep/EngineRuntime/Streaming/tests/Release/EpidemicRuntimeStreamingTests.exe', '--auditor-streaming', 'streaming'),
            (self.binary('EpidemicAuditorShadowFixture'), 'menu', 'menu'),
        )
        run_ids = set()
        for binary, argument, name in cases:
            with self.subTest(scenario=name), tempfile.TemporaryDirectory() as temporary:
                config, _ = AuditorCoreTests()._config(Path(temporary))
                run = RunStorage(config).new_run('scenario', name, 'Deep')
                self.assertNotIn(run.run_id, run_ids)
                run_ids.add(run.run_id)
                result = ManagedProcessRunner(config).run([str(binary), argument], binary.parent,
                                                          run.run_dir, 'Deep', timeout=45)
                self.assertEqual(result.exit_code, 0)
                seal_run_capture(run)
                query = query_action(run.run_dir, name + '-action')
                self.assertTrue(query['complete'], query)
                self.assertEqual(query['action']['run_id'], run.run_id)
                self.assertEqual(query['action']['id'], name + '-action')
                self.assertTrue(query['action']['name'])
                windows = EvidenceLineage(run.run_dir).verify('metrics', 'causal/actions.json')['records'][0]
                self.assertIn(name + '-scenario', windows['scenarios'])
                if name == 'save-load':
                    self.assertEqual({row['type'] for row in query['io']}, {'io.read','io.write'})
                    self.assertTrue(all(row['bytes'] > 0 and row['duration_ns'] > 0 for row in query['io']))
                    # Decode's rejected corrupt package uses a C++ exception internally.
                    # /Gh /GH has no unwind EXIT: keep this independent negative
                    # capture INCOMPLETE instead of certifying a fabricated call tree.
                    bad = RunStorage(config).new_run('scenario-negative', 'save-load-corrupt', 'Deep')
                    rejected = ManagedProcessRunner(config).run([str(binary), '--auditor-save-load-corrupt'], binary.parent,
                                                               bad.run_dir, 'Deep', timeout=45)
                    self.assertEqual(rejected.exit_code, 0, 'engine must reject the corrupt package without changing live inventory')
                    seal_run_capture(bad)
                    incomplete = query_action(bad.run_dir, 'save-load-action')
                    self.assertFalse(incomplete['complete'], 'unobserved exception unwind must never claim COMPLETE')
                elif name == 'menu':
                    self.assertEqual(query['call_tree_availability'], 'UNKNOWN')
                else:
                    self.assertTrue(query['call_tree'])
                    self.assertTrue(query['state_changes'])

    def test_physical_command_persistence_entity_owners(self):
        from collections import Counter
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.lifecycle import seal_run_capture
        cases = (
            ('EngineRuntime/Physics/tests/Release/EpidemicRuntimePhysicsTests.exe', '--auditor-physics',
             {'physics.dispatch': 1, 'physics.callback': 1}),
            ('EngineRuntime/Renderer/tests/Release/EpidemicRuntimeRendererTests.exe', '--auditor-render',
             {'render.submit': 1, 'render.execute': 1}),
            ('EngineRuntime/World/tests/Release/EpidemicRuntimeWorldTests.exe', '--auditor-command',
             {'command.dispatch': 1, 'command.execute': 1}),
            ('EngineRuntime/Persistence/tests/Release/EpidemicRuntimePersistenceTests.exe', '--auditor-persistence',
             {'persistence.begin': 3, 'persistence.commit': 1, 'persistence.fail': 1, 'persistence.rollback': 1}),
            ('EngineFramework/build/tests/Release/EpidemicGameFrameworkEntitiesTests.exe', '--auditor-entity',
             {'entity.created': 1, 'entity.lifecycle': 2, 'state.changed': 3}),
        )
        for relative, argument, expected in cases:
            with self.subTest(argument=argument), tempfile.TemporaryDirectory() as temporary:
                binary = self.root / 'build/audit-deep' / relative
                config, _ = AuditorCoreTests()._config(Path(temporary))
                run = RunStorage(config).new_run('scenario', argument, 'Deep')
                result = ManagedProcessRunner(config).run([str(binary), argument], binary.parent,
                                                          run.run_dir, 'Deep', timeout=45)
                self.assertEqual(result.exit_code, 0)
                seal_run_capture(run)
                graph = EvidenceLineage(run.run_dir).verify('graphs', 'causal/dag.json')['records'][0]
                self.assertTrue(graph['complete'], graph['problems'])
                counts = Counter(row['type'] for row in graph['nodes'])
                self.assertEqual({key: counts[key] for key in expected}, expected)
                facts = [row for row in graph['nodes'] if not row['type'].startswith(('action.', 'scenario.'))]
                self.assertTrue(all(row['action_id'] and row['scenario_id'] for row in facts))
                self.assertTrue(all(row['call_link_status'] == 'explicit_compiler_entry_verified' for row in facts))
                if argument == '--auditor-entity':
                    self.assertEqual(len({row['entity_id'] for row in facts}), 1)
                    self.assertTrue(all(row['state_before'] is not None and row['state_after'] is not None for row in facts))
                if argument == '--auditor-physics':
                    pair = {row['type']: row for row in facts}
                    self.assertNotEqual(pair['physics.dispatch']['thread'], pair['physics.callback']['thread'])

    def test_physical_first_chance_recovery_and_storm(self):
        for mode, count in (('handled', 1), ('storm', 100)):
            with self.subTest(mode=mode):
                report = self.observe(mode)
                self.assertEqual(report['exit_code'], 0)
                events = report['exceptions']
                self.assertEqual(len(events), count)
                self.assertTrue(all(row['first_chance'] for row in events))
                self.assertEqual(len(report['recovery_markers']), count)
                self.assertTrue(all(row['stack'] for row in events))
                self.assertTrue(any('handled_exception' in (frame['function'] or '')
                                    for row in events for frame in row['stack']))
                self.assertGreaterEqual(report['process_cpu_100ns'], 0)
                self.assertTrue(all(row['continuation_qpc'] >= row['qpc'] for row in events))
                self.assertTrue(all(type(row['thread_cpu_interval_100ns']) is int
                                    and row['thread_cpu_interval_100ns'] >= 0 for row in events))
                from ep_auditor.analyzers.diagnostics import analyze_exception_snapshot
                summary = analyze_exception_snapshot(report)
                totals = summary['codes'][str(0xE0424242)]
                self.assertEqual(totals['first_chance_count'], count)
                self.assertEqual(totals['cpu_interval_samples'], count)
                self.assertTrue(all(row['continuation_observed'] for row in summary['events']))

    def test_physical_verifier_heap_and_leak_controls(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.lifecycle import finalize_run, authoritative_run
        from ep_auditor.collectors.sanitizer import capture_verifier_fixture
        for mode in ('verifier-good', 'verifier-bad', 'leak-good', 'leak-bad'):
            with tempfile.TemporaryDirectory() as temporary:
                config, _ = AuditorCoreTests()._config(Path(temporary))
                run = RunStorage(config).new_run('verifier', mode, 'Deep')
                report = capture_verifier_fixture(config, run.run_dir,
                    self.binary('EpidemicAuditorShadowFixture'), mode, helper=self.binary('EpidemicAuditorObservation'))
                self.assertEqual(report['memory_safety_verdict'], 'UNKNOWN')
                if mode == 'leak-bad':
                    self.assertTrue(report['candidates'])
                # Verified raw reads require the authoritative sealed capture.
                # Keep the pre-seal rejection as a negative control.
                with self.assertRaises(EvidenceError):
                    RawEvidence(run.run_dir).read_bytes('process/stdout.bin')
                finalize_run(run, 'PASS', {})
                if mode == 'verifier-good':
                    snapshot = json.loads(RawEvidence(run.run_dir).read_bytes('process/stdout.bin'))
                    self.assertEqual(snapshot['exit_code'], 0)
                    self.assertIs(snapshot['complete'], True)
                    self.assertTrue(snapshot['verifier_modules'], 'SDK activation must be physically observed')
                expected = 'PASS' if mode.endswith('good') else 'FAIL'
                self.assertEqual(authoritative_run(run.run_dir)['status'], expected)
                published = EvidenceLineage(run.run_dir).verify('normalized', 'diagnostics/verifier.json')
                self.assertTrue(published['records'][0]['activation_verified'])
                # Preserve the physical capture. Tamper only a verified independent
                # copy, with a positive control before changing its derived record.
                import shutil
                with tempfile.TemporaryDirectory() as tampered:
                    clone = Path(tampered) / 'capture'
                    shutil.copytree(run.run_dir, clone)
                    self.assertEqual(authoritative_run(clone)['status'], expected)
                    path = clone / 'normalized/diagnostics/verifier.json'
                    published['records'][0]['activation_verified'] = False
                    path.write_text(json.dumps(published), encoding='utf-8')
                    with self.assertRaises(EvidenceError):
                        authoritative_run(clone)

    def test_physical_memory_api_calls_are_observed(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.lifecycle import finalize_run, authoritative_run
        from ep_auditor.collectors.modules import capture_api_snapshot
        # The existing disk-io fixture actually allocates and releases an 8 MiB
        # VirtualAlloc buffer. Verifier's HeapAlloc case does not call these APIs.
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('api', 'physical Virtual memory APIs', 'Deep')
            snapshot, graph = capture_api_snapshot(config, run.run_dir,
                self.binary('EpidemicAuditorShadowFixture'), ['disk-io'],
                helper=self.binary('EpidemicAuditorObservation'))
            self.assertEqual(snapshot['exit_code'], 0)
            self.assertIs(graph['complete'], True)
            finalize_run(run, 'PASS', {})
            sealed = json.loads(RawEvidence(run.run_dir).read_bytes('process/stdout.bin'))
            self.assertEqual(sealed, snapshot)
            memory_calls = [row for row in sealed['api_calls'] if row['family'] == 'memory']
            self.assertTrue({'VirtualAlloc', 'VirtualFree'} <= {row['name'] for row in memory_calls})
            self.assertTrue(all(row['address'] and row['stack'] for row in memory_calls))
            self.assertEqual(authoritative_run(run.run_dir)['status'], 'PASS')

    def test_physical_api_export_entries_and_call_stacks(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.storage import RunStorage
        helper = self.binary('EpidemicAuditorObservation')
        fixture = self.binary('EpidemicAuditorShadowFixture')
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            config, _ = AuditorCoreTests()._config(directory)
            run = RunStorage(config).new_run('api', 'physical Win32 entries', 'Deep')
            from ep_auditor.collectors.modules import capture_api_snapshot, publish_api_snapshot
            report, graph = capture_api_snapshot(config, run.run_dir, fixture, [], helper=helper)
            data, raw = RawEvidence(run.run_dir).read_capture('process/stdout.bin')
            self.assertTrue(raw['complete'])
            self.assertEqual(report, json.loads(data))
            self.assertEqual(report['exit_code'], 0)
            self.assertTrue(report['api_entry_scope_complete'])
            names = {row['name'] for row in report['api_calls']}
            self.assertTrue({'CreateFileW', 'ReadFile', 'WriteFile', 'CloseHandle', 'SetFilePointer',
                             'CreateFileMappingW', 'MapViewOfFile', 'FindFirstFileW', 'FindClose'} <= names, names)
            self.assertTrue(all(row['address'] and row['stack'] for row in report['api_calls']))
            reads = [row for row in report['api_calls'] if row['name'] == 'ReadFile']
            self.assertTrue(any(row['synchronous_attempt'] and row['main_thread'] for row in reads))
            self.assertTrue(graph['io_by_caller'])
            from ep_auditor.analyzers.diagnostics import analyze_api_snapshot
            changed = json.loads(json.dumps(report))
            read = next(row for row in changed['api_calls'] if row['name'] == 'ReadFile')
            read['main_thread'] = not read['main_thread']
            with self.assertRaises(EvidenceError):
                analyze_api_snapshot(changed)
            self.assertTrue(any(any('main' in (frame['function'] or '') for frame in row['stack']) for row in reads))
            self.assertIn('MoveFileExW', report['api_supported_exports'])
            self.assertNotIn('MoveFileExW', names)
            from ep_auditor.collectors.modules import capture_pe_imports
            inventory_run = RunStorage(config).new_run('imports', 'static PE inventory', 'Fast')
            inventory = capture_pe_imports(config, inventory_run.run_dir, fixture)
            imported = {symbol['name'] for descriptor in inventory['imports'] for symbol in descriptor['symbols']}
            self.assertIn('MoveFileExW', imported)
            self.assertIn('ReadFile', imported)
            self.assertFalse(inventory['runtime_calls_observed'])
            from ep_auditor.core.lifecycle import seal_run_capture
            seal_run_capture(run)
            published = publish_api_snapshot(run.run_dir)
            self.assertEqual(published, graph)
            self.assertTrue(EvidenceLineage(run.run_dir).verify('graphs', 'modules/api_entry_graph.json')['records'][0]['complete'])
            self.assertTrue(any(edge['api'] == 'ReadFile' and edge['raw_json_pointer'].startswith('/api_calls/')
                                for edge in published['edges']))
            self.assertTrue(telemetry_complete(run.run_dir))
            # Keep the sealed physical capture intact; corrupt only a verified copy.
            import shutil
            with tempfile.TemporaryDirectory() as tampered:
                clone = Path(tampered) / 'capture'
                shutil.copytree(run.run_dir, clone)
                self.assertTrue(telemetry_complete(clone))
                graph_path = clone / 'graphs/modules/api_entry_graph.json'
                changed = json.loads(graph_path.read_bytes())
                changed['records'][0]['counts']['ReadFile'] += 1
                graph_path.write_text(json.dumps(changed), encoding='utf-8')
                self.assertFalse(telemetry_complete(clone))

    def test_physical_second_chance_crash_assert_contract(self):
        for mode, code in (('crash', 0xE0424242), ('assert', 0xC0000420), ('contract', 0xE0434242)):
            with self.subTest(mode=mode):
                report = self.observe(mode)
                self.assertEqual(report['exit_code'], code)
                events = [row for row in report['exceptions'] if row['code'] == code]
                self.assertEqual([row['first_chance'] for row in events], [True, False])
                self.assertTrue(all(row['address'] and row['tid'] and row['stack'] for row in events))
                self.assertEqual(report['recovery_markers'], [])

    def test_physical_shadow_transport(self):
        fixture = self.binary('EpidemicAuditorShadowFixture')
        with tempfile.TemporaryDirectory() as temporary:
            run = Path(temporary)
            causal, diagnostic = run / 'causal-capture', run / 'diagnostic-capture'
            causal.mkdir()
            diagnostic.mkdir()
            env = dict(os.environ, EP_AUDITOR_CAUSAL_DIRECTORY=str(causal),
                       EP_AUDITOR_DIAGNOSTICS_DIRECTORY=str(diagnostic))
            result = subprocess.run([str(fixture)], env=env, capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0)
            for family in ('causal', 'diagnostics'):
                capture_streams(run, family, required=True)
            manifest = RawEvidence(run).seal({'run_id': 'RunId:' + str(uuid.uuid4()),
                'instrumentation_profile': 'audit-deep', 'source_fingerprints': {'test': digest(Path(__file__))},
                'binary_fingerprints': {'fixture': digest(fixture)}, 'analyzer_version': 'ms65-native-1'})
            self.assertTrue(manifest['capture_complete'])
            publish_telemetry(run)
            lineage = EvidenceLineage(run)
            graph = lineage.verify('graphs', 'causal/dag.json')['records'][0]
            report = lineage.verify('metrics', 'diagnostics/report.json')['records'][0]
            self.assertTrue(graph['complete'])
            self.assertEqual(len(graph['edges']), 1)
            self.assertTrue(report['complete'])
            self.assertEqual(report['counts']['io.read'], 1)
            self.assertEqual(report['allocation_metrics']['lifetimes'][0]['size_bytes'], 512)
            self.assertEqual(report['live_allocation_candidates'], 0)
            self.assertEqual(report['allocation_metrics']['by_action']['fixture-a1']['bytes'], 512)
            self.assertEqual(report['allocation_metrics']['by_function']['DefaultAllocator.Allocate']['bytes'], 512)
            rows = lineage.verify('normalized', 'diagnostics/events.json')['records']
            self.assertTrue(all(row.get('stack') for row in rows if row['type'] in {'allocation', 'deallocation', 'io.read'}))

    def test_physical_asan_good_and_memory_failures(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.lifecycle import seal_run_capture, finalize_run, authoritative_run
        from ep_auditor.collectors.sanitizer import publish_sanitizer
        fixture = self.root / 'build/auditor-native/bin/Release/EpidemicAuditorMemoryFixture.exe'
        self.assertTrue(fixture.is_file(), 'mandatory Release ASan fixture must be built')
        options = {'ASAN_OPTIONS': 'halt_on_error=1:detect_leaks=0:detect_stack_use_after_return=0'}
        for mode, expected in (('memory-good', None), ('uaf', 'heap-use-after-free'),
                               ('double-free', 'double-free'), ('oob', 'heap-buffer-overflow'),
                               ('heap-corruption', 'heap-buffer-overflow')):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as temporary:
                config, _ = AuditorCoreTests()._config(Path(temporary))
                run = RunStorage(config).new_run('asan', mode, 'Fast')
                result = ManagedProcessRunner(config).run([str(fixture), mode], fixture.parent,
                    run.run_dir, 'Fast', env_extra=options, timeout=30)
                stderr, raw = RawEvidence(run.run_dir).read_capture('process/stderr.bin')
                self.assertTrue(raw['complete'])
                report = parse_stderr(stderr)
                if expected is None:
                    self.assertEqual(result.exit_code, 0, stderr[-1024:])
                    self.assertEqual(report['candidates'], [])
                else:
                    self.assertNotEqual(result.exit_code, 0)
                    self.assertTrue(any(expected in row['kind'] for row in report['candidates']), stderr[-1024:])
                self.assertEqual(report['memory_safety_verdict'], 'UNKNOWN')
                seal_run_capture(run)
                publish_sanitizer(run.run_dir)
                captured = EvidenceLineage(run.run_dir).verify('normalized', 'diagnostics/sanitizer_stderr.json')
                self.assertEqual(captured['records'][0]['candidates'], report['candidates'])
                self.assertEqual(captured['records'][0]['confidence'], 'heuristic')
                finalize_run(run, 'PASS', {})
                self.assertEqual(authoritative_run(run.run_dir)['status'], 'PASS' if expected is None else 'FAIL')
                if expected is not None:
                    import shutil
                    with tempfile.TemporaryDirectory() as tampered:
                        clone = Path(tampered) / 'capture'
                        shutil.copytree(run.run_dir, clone)
                        self.assertEqual(authoritative_run(clone)['status'], 'FAIL')
                        status_path = clone / 'run.json'
                        forged = json.loads(status_path.read_bytes())
                        forged['status'] = 'PASS'
                        status_path.write_text(json.dumps(forged), encoding='utf-8')
                        with self.assertRaises(EvidenceError):
                            authoritative_run(clone)

    def test_physical_production_action_native_exception(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.lifecycle import seal_run_capture
        from ep_auditor.collectors.modules import capture_api_snapshot
        from ep_auditor.analyzers.causality import query_action
        binary = self.root / 'build/audit-deep/EngineFramework/build/tests/Release/EpidemicExtendedGameplayIntegrationSaveParticipantTests.exe'
        self.assertTrue(binary.is_file(), 'mandatory production inventory scenario must be built')
        helper = self.binary('EpidemicAuditorObservation')
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('scenario-negative', 'production Action exception', 'Deep')
            snapshot, _ = capture_api_snapshot(config, run.run_dir, binary,
                ['--auditor-save-load-corrupt'], helper=helper)
            self.assertEqual(snapshot['exit_code'], 0, 'real decoder must reject corruption and preserve live inventory')
            self.assertTrue(any(row['code'] == 0xE06D7363 for row in snapshot['exceptions']),
                            'the real decoder C++ exception must be independently observed')
            seal_run_capture(run)
            query = query_action(run.run_dir, 'save-load-action')
            self.assertFalse(query['complete'], 'missing unwind EXIT must remain INCOMPLETE')
            native = [row for row in query['exceptions'] if row['provenance'] == 'external']
            self.assertTrue(native, 'Action query must not discard independent native exceptions')
            self.assertTrue(any(row['code'] == 0xE06D7363 for row in native))
            self.assertTrue(any('DecodeSaveGameImage' in (frame.get('function') or '')
                                for row in native for frame in row['stack']))
            self.assertEqual(query['native_exception_availability'], 'observed_external_debugger')
            threads = {tuple(row) for row in query['threads']}
            for row in native:
                self.assertIn((row['pid'], row['process_epoch'], row['tid']), threads)
                self.assertLessEqual(query['action']['start_ns'], row['timestamp_ns'])
                self.assertLessEqual(row['timestamp_ns'], query['action']['end_ns'])
                self.assertTrue(row['stack'])
                self.assertEqual(row['confidence'], 'observed')
                self.assertEqual(row['action_membership_confidence'], 'derived')
                self.assertEqual(row['handling']['classification'], 'UNKNOWN')
                self.assertFalse(row['handling']['continuation_observed'])
                self.assertEqual(row['rawReference']['path'], 'process/stdout.bin')
                self.assertTrue(row['raw_json_pointer'].startswith('/exceptions/'))
            import shutil
            with tempfile.TemporaryDirectory() as tampered:
                clone = Path(tampered) / 'capture'
                shutil.copytree(run.run_dir, clone)
                self.assertTrue(query_action(clone, 'save-load-action')['exceptions'])
                raw_path = clone / 'raw/process/stdout.bin'
                os.chmod(raw_path, 0o600)
                raw_path.write_bytes(raw_path.read_bytes() + b' ')
                with self.assertRaises(EvidenceError):
                    query_action(clone, 'save-load-action')

    def test_observer_success_cannot_hide_target_crash(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.lifecycle import finalize_run, authoritative_run
        from ep_auditor.collectors.modules import capture_api_snapshot
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('crash', 'observed crash', 'Deep')
            report, _ = capture_api_snapshot(config, run.run_dir, self.binary('EpidemicAuditorShadowFixture'),
                                            ['crash'], helper=self.binary('EpidemicAuditorObservation'))
            self.assertNotEqual(report['exit_code'], 0)
            self.assertTrue(report['complete'])
            finalize_run(run, 'PASS', {})
            self.assertEqual(run.status, 'FAIL')
            self.assertEqual(authoritative_run(run.run_dir)['status'], 'FAIL')
            path = run.run_dir / 'run.json'
            committed = json.loads(path.read_text(encoding='utf-8'))
            committed['status'] = 'PASS'
            path.write_text(json.dumps(committed), encoding='utf-8')
            with self.assertRaises(EvidenceError):
                authoritative_run(run.run_dir)

    def test_physical_loopback_network_api_activity(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.collectors.modules import capture_api_snapshot
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('network', 'owned loopback', 'Deep')
            report, graph = capture_api_snapshot(config, run.run_dir, self.binary('EpidemicAuditorShadowFixture'),
                                                ['network'], helper=self.binary('EpidemicAuditorObservation'))
            self.assertEqual(report['exit_code'], 0)
            names = {row['name'] for row in report['api_calls'] if row['family'] == 'socket'}
            self.assertTrue({'connect', 'send', 'recv', 'closesocket'} <= names, names)
            self.assertTrue(graph['complete'])
            self.assertGreater(graph['counts']['send'], 0)

    def test_physical_window_and_graphics_api_activity(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.collectors.modules import capture_api_snapshot
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('graphics', 'hidden window and WARP device', 'Deep')
            report, graph = capture_api_snapshot(config, run.run_dir, self.binary('EpidemicAuditorShadowFixture'),
                                                ['graphics'], helper=self.binary('EpidemicAuditorObservation'))
            self.assertEqual(report['exit_code'], 0)
            names = {row['name'] for row in report['api_calls']}
            self.assertTrue({'CreateWindowExW', 'DestroyWindow', 'PeekMessageW', 'D3D11CreateDevice'} <= names, names)
            self.assertTrue(graph['complete'])

    def test_physical_async_completion_and_cancellation_are_successful_runs(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.core.lifecycle import finalize_run, authoritative_run
        from ep_auditor.collectors.modules import capture_api_snapshot
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            for variant in ('async-io', 'cancel-io'):
                with self.subTest(variant=variant):
                    run = RunStorage(config).new_run('io', variant, 'Deep')
                    report, graph = capture_api_snapshot(config, run.run_dir,
                        self.binary('EpidemicAuditorShadowFixture'), [variant],
                        helper=self.binary('EpidemicAuditorObservation'))
                    self.assertEqual(report['exit_code'], 0)
                    reads = [row for row in report['api_calls'] if row['name'] == 'ReadFile']
                    self.assertTrue(reads)
                    self.assertTrue(all(not row['synchronous_attempt'] and row['main_thread'] for row in reads))
                    names = {row['name'] for row in report['api_calls']}
                    # OVERLAPPED completion signals the event in the kernel;
                    # this scenario neither calls SetEvent nor starts a thread.
                    self.assertTrue({'CreateEventW', 'CreateFileW', 'ReadFile',
                                     'CloseHandle', 'GetOverlappedResult'} <= names, names)
                    self.assertIn('GetOverlappedResult', names)
                    if variant == 'cancel-io':
                        self.assertIn('CancelIoEx', names)
                    else:
                        self.assertIn('WriteFile', names)
                    self.assertTrue(graph['complete'])
                    finalize_run(run, 'PASS', {})
                    self.assertEqual(authoritative_run(run.run_dir)['status'], 'PASS')
                    causal = EvidenceLineage(run.run_dir).verify('graphs', 'causal/dag.json')['records'][0]
                    self.assertTrue(causal['complete'], causal['problems'])
                    terminal = 'io.cancel' if variant == 'cancel-io' else 'io.complete'
                    self.assertEqual({row['type'] for row in causal['nodes']}, {'io.submit', terminal})


    def test_background_io_is_not_main_thread_blocking_attempt(self):
        from test_core import AuditorCoreTests
        from ep_auditor.core.storage import RunStorage
        from ep_auditor.collectors.modules import capture_api_snapshot
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = AuditorCoreTests()._config(Path(temporary))
            run = RunStorage(config).new_run('io', 'worker IO control', 'Deep')
            report, graph = capture_api_snapshot(config, run.run_dir, self.binary('EpidemicAuditorShadowFixture'),
                                                ['background-io'], helper=self.binary('EpidemicAuditorObservation'))
            self.assertEqual(report['exit_code'], 0)
            reads = [row for row in report['api_calls'] if row['name'] == 'ReadFile']
            self.assertTrue(reads)
            self.assertTrue(all(row['synchronous_attempt'] and not row['main_thread'] for row in reads))
            self.assertTrue(graph['complete'])


if __name__ == '__main__':
    unittest.main()
