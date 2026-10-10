"""Product regressions; controlled inputs below are not physical acceptance evidence."""
import contextlib
import io
import json
import os
import stat
import sys
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.analyzers.summary import query_run_view, query_view_field
from ep_auditor.analyzers.compare import compare_summaries, compare_runs
from ep_auditor.core.evidence import RawEvidence, EvidenceError, digest
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.exporters.agent import export_agent_pack, export_investigation, REPORTS

ROOT = Path(__file__).resolve().parents[2]


class ViewTests(unittest.TestCase):
    def setUp(self):
        from test_core import AuditorCoreTests
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / '.ai/tmp')
        self.addCleanup(self.temp.cleanup)
        self.config, self.project = AuditorCoreTests._config(self, Path(self.temp.name))
        self.storage = RunStorage(self.config)

    def controlled_run(self, count=5):
        run = self.storage.new_run('process', 'controlled pagination input', 'Fast')
        rows = [{'processes': [{'pid': i, 'frames': list(range(80)), 'functionId': 'FunctionId:unit'}]}
                for i in range(count)]
        RawEvidence(run.run_dir).append('process/process_samples.jsonl',
            ''.join(json.dumps(row) + '\n' for row in rows).encode(), collector='controlled_unit_input', version='1',
            start_ns=1, end_ns=2, health={'capacity': 1000000, 'observed': count, 'dropped': 0, 'incomplete_reason': None})
        self.storage.finish_run(run, 'PASS')
        return run

    def test_bounded_pages_nested_continuation_and_scope_binding(self):
        run = self.controlled_run()
        page = query_run_view(run.run_dir, 'Threads', limit=2)
        self.assertEqual([row['pid'] for row in page['items']], [0, 1])
        self.assertTrue(page['truncated'])
        self.assertEqual(len(page['items'][0]['frames']), 50)
        rest = query_view_field(run.run_dir, page['items'][0]['field_continuations'][0])
        self.assertEqual(rest['items'], list(range(50, 80)))
        self.assertFalse(rest['truncated'])
        second = query_run_view(run.run_dir, 'Threads', limit=2, cursor=page['continuation'])
        self.assertEqual([row['pid'] for row in second['items']], [2, 3])
        with self.assertRaises(EvidenceError):
            query_run_view(run.run_dir, 'Threads', limit=3, cursor=page['continuation'])
        with self.assertRaises(EvidenceError):
            query_run_view(run.run_dir, 'Threads', limit=201)

    def test_unknown_not_zero_or_pass_and_incompatible_comparison(self):
        run = self.controlled_run()
        page = query_run_view(run.run_dir, 'Exceptions')
        self.assertEqual(page['availability'], 'UNKNOWN')
        self.assertFalse(page['complete'])
        delta = compare_summaries({}, {'peak_rss_bytes': 1})['metrics']['peak_rss_bytes']
        self.assertIsNone(delta['delta'])
        self.assertEqual(delta['availability'], 'UNKNOWN')
        result = compare_runs(run.run_dir, run.run_dir)
        self.assertEqual(result['compatibility'], 'INCOMPATIBLE')
        self.assertEqual(result['regression_verdict'], 'UNKNOWN')

    def test_history_routes_and_unknown_are_bounded_without_invented_measurements(self):
        from unittest.mock import Mock
        from ep_auditor.ui.main_window import AuditorWindow
        empty = self.storage.history_for_test('TestId:not-captured', limit=1)
        self.assertEqual(empty['items'], [])
        self.assertEqual(empty['availability'], 'UNKNOWN')
        self.assertIsNone(empty['stability']['pass_rate'])
        for bounds in (0, 201):
            with self.assertRaises(EvidenceError):
                self.storage.history_for_test('TestId:not-captured', limit=bounds)
        with self.assertRaises(EvidenceError):
            self.storage.history_for_test('TestId:not-captured', cursor='invalid')
        window = object.__new__(AuditorWindow)
        window.storage = Mock()
        for kind, method, identity in [('test', 'history_for_test', 'TestId:unit'),
                                      ('regression', 'history_for_regression', 'FunctionId:unit'),
                                      ('function', 'history_for_function', 'FunctionId:unit')]:
            result = AuditorWindow._read_product_view(window, self.project, 'History', identity, None,
                                                  None, False, history_kind=kind)
            getattr(window.storage, method).assert_called_once_with(identity, cursor=None)
            self.assertIs(result, getattr(window.storage, method).return_value)

    def test_persistent_pressure_scope_rejects_conflicts_before_process_launch(self):
        runner = ManagedProcessRunner(self.config)
        cases = [('unknown', 'Memory', {}), ('persistent_pressure', 'Fast', {}),
                 ('persistent_pressure', 'Memory', {'exact_required': True}),
                 ('persistent_pressure', 'Memory', {'diagnostics_required': True}),
                 ('persistent_pressure', 'Memory', {'causal_required': True}),
                 ('persistent_pressure', 'Memory', {'rolling': {}})]
        for scope, mode, additional in cases:
            with self.subTest(scope=scope, mode=mode, conflict=additional):
                self.config.data['capture'] = {'memory_scope': scope, **additional}
                with self.assertRaises(EvidenceError):
                    runner._run(['must-not-launch.exe'], self.project, self.project, mode)

    def test_owned_persistent_file_capture_bounds_and_raw_retention(self):
        run = self.storage.new_run('process', 'logical file capture control', 'Memory')
        workspace = run.run_dir / 'persistent-pressure-workspace'
        workspace.mkdir()
        runner = ManagedProcessRunner(self.config)
        with self.assertRaises(EvidenceError):
            runner._capture_persistent_files(run.run_dir, workspace, 1)
        (workspace / 'unsafe-directory').mkdir()
        with self.assertRaises(EvidenceError):
            runner._capture_persistent_files(run.run_dir, workspace, 1)
        (workspace / 'unsafe-directory').rmdir()
        oversized = workspace / 'oversized.bin'
        oversized.write_bytes(b'x' * (1024**2 + 1))
        with self.assertRaises(EvidenceError):
            runner._capture_persistent_files(run.run_dir, workspace, 1)
        oversized.unlink()
        (workspace / 'known.bin').write_bytes(b'known persistent bytes')
        self.assertEqual(runner._capture_persistent_files(run.run_dir, workspace, 1), (1, 22))
        self.storage.finish_run(run, 'INCOMPLETE')  # Logical helper input, no physical target ran.
        raw = RawEvidence(run.run_dir)
        self.assertEqual(raw.read_bytes('persistent/files/known.bin'), b'known persistent bytes')
        captured = run.run_dir / 'raw/persistent/files/known.bin'
        captured.chmod(stat.S_IREAD | stat.S_IWRITE)
        captured.write_bytes(b'edited persistent bytes')
        with self.assertRaises(EvidenceError):
            raw.verify()

    def test_persistent_pressure_clears_inherited_unrequested_capture_environment(self):
        from unittest.mock import patch
        run = self.storage.new_run('process', 'logical environment admission control', 'Memory')
        self.config.data['capture']['memory_scope'] = 'persistent_pressure'
        variables = ('EP_AUDITOR_TRACE_DIRECTORY', 'EP_AUDITOR_TRACE_CAPACITY',
                     'EP_AUDITOR_REQUIRE_EXACT', 'EP_AUDITOR_DIAGNOSTICS_DIRECTORY',
                     'EP_AUDITOR_CAUSAL_DIRECTORY', 'EP_AUDITOR_ROLLING_DIRECTORY',
                     'EP_AUDITOR_ARM_NEXT_INPUT_ID', 'EP_AUDITOR_ARM_NEXT_INPUT_NAME')
        with patch.dict(os.environ, {key: 'stale-inherited' for key in variables}), \
                patch('ep_auditor.core.process.OwnedProcess', side_effect=OSError('controlled admission refusal')) as admission:
            with self.assertRaises(OSError):
                ManagedProcessRunner(self.config)._run(['does-not-launch.exe'], self.project, run.run_dir, 'Memory')
        environment = admission.call_args.args[2]
        self.assertFalse(any(key in environment for key in variables))
        self.assertEqual(Path(environment['EP_AUDITOR_SAVE_DIRECTORY']), run.run_dir / 'persistent-pressure-workspace')

    def test_raw_tampering_is_not_an_empty_successful_view(self):
        run = self.controlled_run()
        source = run.run_dir / 'raw/process/process_samples.jsonl'
        source.chmod(stat.S_IREAD | stat.S_IWRITE)
        source.write_bytes(b'{}\n')
        with self.assertRaises(EvidenceError):
            query_run_view(run.run_dir, 'Threads')

    def test_real_process_nonzero_exit_findings_exports_and_cli(self):
        run = self.storage.new_run('process', 'physical independent exit control', 'Fast')
        result = ManagedProcessRunner(self.config).run(
            [sys.executable, '-c', "print('PASS'); raise SystemExit(7)"], self.project, run.run_dir, 'Fast')
        self.assertEqual(result.exit_code, 7)
        self.storage.finish_run(run, 'PASS')
        self.assertEqual(run.status, 'FAIL')
        findings = query_run_view(run.run_dir, 'Findings')['items']
        self.assertEqual(findings[0]['rule_id'], 'process.nonzero_exit')
        self.assertEqual(findings[0]['reproduction']['availability'], 'captured_inputs_verified')
        self.assertEqual(findings[0]['reproduction']['rerun_verdict'], 'NOT RUN')
        pack = export_investigation(run.run_dir, 'finding', findings[0]['finding_id'])
        self.assertEqual(len(pack['items']), 1)
        output = export_agent_pack(run.run_dir, {'status': 'PASS'}, {'peak_rss_bytes': 0})
        manifest = json.loads((output / 'manifest.json').read_text())
        self.assertEqual(manifest['status'], 'FAIL')
        for name in REPORTS:
            for suffix in ('.md', '.json', '.jsonl', '.csv'):
                self.assertEqual(manifest['files'][name + suffix], digest(output / (name + suffix)))
        from ep_auditor.cli import main
        stream = io.StringIO()
        with contextlib.redirect_stdout(stream):
            self.assertEqual(main(['view', str(run.run_dir), 'Findings']), 0)
        self.assertEqual(json.loads(stream.getvalue())['items'][0]['finding_id'], findings[0]['finding_id'])

    def test_physical_exact_view_rejects_edited_metrics(self):
        self.config.data['capture']['exact_required'] = True
        run = self.storage.new_run('test', 'physical exact view replay', 'Fast')
        fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorExactFixture.exe'
        self.assertTrue(fixture.is_file())
        result = ManagedProcessRunner(self.config).run([str(fixture), 'balanced'], self.project, run.run_dir, 'Fast')
        self.assertEqual(result.exit_code, 0)
        self.storage.finish_run(run, 'PASS')
        view = query_run_view(run.run_dir, 'Functions')
        self.assertTrue(view['items'])
        self.assertTrue(view['complete'])
        function_id = view['items'][0]['functionId']
        self.assertTrue(self.storage.history_for_function(function_id)['items'])
        modules = query_run_view(run.run_dir, 'Modules')
        self.assertTrue(modules['items'], 'Ordinary instrumented runs must expose their captured module identities')
        module = modules['items'][0]
        self.assertEqual(module['provenance'], 'compiler_instrumentation')
        self.assertTrue(module['capture_refs'])
        investigated = export_investigation(run.run_dir, 'module', module['moduleId'])
        self.assertTrue(all(item['moduleId'] == module['moduleId'] for item in investigated['items']))
        graph = next((run.run_dir / 'graphs/exact').glob('*-calls-*.json'))
        extra = graph.with_name(graph.stem.rsplit('-', 1)[0] + '-999999.json')
        extra.write_bytes(graph.read_bytes())
        with self.assertRaises(EvidenceError):
            query_run_view(run.run_dir, 'Call Tree')
        extra.unlink()  # Test-owned forgery only.
        source = next((run.run_dir / 'metrics/exact').glob('*-functions.json'))
        report = json.loads(source.read_text())
        next(row for row in report['records'] if row.get('functionId'))['calls'] += 1000
        source.write_text(json.dumps(report))
        with self.assertRaises(EvidenceError):
            query_run_view(run.run_dir, 'Functions')
        # Updating the disposable cache must not legitimize a forged projection.
        self.storage.evidence_index.index(run.run_dir, 'functions', 'metrics', source.relative_to(run.run_dir / 'metrics').as_posix())
        with self.assertRaises(EvidenceError):
            self.storage.history_for_function(function_id)

    def test_cached_sanitizer_projection_cannot_hide_or_promote_target_stderr(self):
        run = self.storage.new_run('test', 'target-controlled sanitizer text', 'Fast')
        result = ManagedProcessRunner(self.config).run([sys.executable, '-c',
            'import sys; sys.stderr.write("ERROR: AddressSanitizer: heap-use-after-free\\n")'],
            self.project, run.run_dir, 'Fast')
        self.assertEqual(result.exit_code, 0)
        self.storage.finish_run(run, 'PASS')
        findings = query_run_view(run.run_dir, 'Findings')['items']
        candidate = next(row for row in findings if row['rule_id'] == 'memory.tool_report')
        self.assertEqual(candidate['confidence'], 'heuristic')
        self.assertFalse(candidate['independent_validation'])
        self.assertIn('not independently established', candidate['summary'])
        source = run.run_dir / 'normalized/diagnostics/sanitizer_stderr.json'
        report = json.loads(source.read_text())
        report['records'][0]['candidates'] = []
        report['records'][0]['candidate_count'] = 0
        source.write_text(json.dumps(report))
        with self.assertRaises(EvidenceError):
            query_run_view(run.run_dir, 'Memory')

    def test_dashboard_process_list_resumes_and_missing_rss_stays_unknown(self):
        run = self.controlled_run(count=60)
        page = query_run_view(run.run_dir, 'Dashboard')
        process = page['items'][0]['process']
        self.assertEqual(process['processes_total'], 60)
        self.assertEqual(len(process['processes']), 50)
        self.assertTrue(page['truncated'])
        self.assertTrue(all(row['rss_peak_bytes'] is None for row in process['processes']))
        tail = query_view_field(run.run_dir, page['items'][0]['field_continuations'][0])
        self.assertEqual(len(tail['items']), 10)
        self.assertFalse(tail['truncated'])

    def test_gui_views_share_backend_ids_and_clear_stale_success_on_corruption(self):
        import time
        from ep_auditor.core.events import EventBus
        from ep_auditor.core.run_manager import RunManager
        from ep_auditor.ui.main_window import AuditorWindow
        run = self.storage.new_run('process', 'physical GUI view smoke', 'Fast')
        result = ManagedProcessRunner(self.config).run(
            [sys.executable, '-c', 'raise SystemExit(3)'], self.project, run.run_dir, 'Fast')
        self.assertEqual(result.exit_code, 3)
        self.storage.finish_run(run, 'FAIL')
        events = EventBus()
        window = AuditorWindow(self.config, RunManager(self.config, self.storage, events), self.storage, events)
        window.root.withdraw()
        self.addCleanup(window._on_close)
        errors = []
        window.root.report_callback_exception = lambda *args: errors.append(args)
        window.root.tk.createcommand('capture_tcl_error', lambda text: errors.append(text))
        window.root.tk.eval('proc bgerror {message} {capture_tcl_error $message}')
        tabs = {window.notebook.tab(tab, 'text') for tab in window.notebook.tabs()}
        self.assertTrue({'Build', 'Timeline', 'Call Tree', 'Causal Graph', 'Functions', 'Memory',
                         'Allocations', 'IO', 'Exceptions', 'Coverage', 'Architecture', 'Findings', 'Compare', 'Agent'} <= tabs)
        state = window.product_views['Findings']
        state['run_id'].set(run.run_id)
        def wait():
            deadline = time.monotonic() + 30
            while window.view_future is not None and time.monotonic() < deadline:
                window.root.update()
                time.sleep(.01)
            self.assertIsNone(window.view_future, 'GUI view timed out')
            self.assertFalse(errors, 'Python/Tcl callbacks must fail the smoke test')
        window._request_product_view('Findings', reset=True)
        wait()
        expected = query_run_view(run.run_dir, 'Findings')['items'][0]['finding_id']
        self.assertEqual(state['tree'].item('0', 'text'), expected)
        state['tree'].selection_set('0')
        window._select_product_row('Findings')
        self.assertEqual(json.loads(state['detail'].get('1.0', 'end'))['finding_id'], expected)
        bounded = self.controlled_run(count=1)
        threads = window.product_views['Threads']
        threads['run_id'].set(bounded.run_id)
        window._request_product_view('Threads', reset=True)
        wait()
        threads['tree'].selection_set('0')
        window._select_product_row('Threads')
        self.assertTrue(threads['field_tokens'])
        window._request_product_field('Threads')
        wait()
        self.assertEqual([row['value'] for row in threads['page']['items']], list(range(50, 80)))
        self.assertFalse(threads['page']['truncated'])
        source = run.run_dir / 'raw/process/lifecycle.json'
        source.chmod(stat.S_IREAD | stat.S_IWRITE)
        source.write_bytes(b'{}')
        window._request_product_view('Findings', reset=True)
        wait()
        self.assertIsNone(state['page'])
        self.assertEqual(state['tree'].get_children(), ())
        self.assertIn('Evidence unavailable:', state['status'].get())

    def test_gui_coverage_heatmap_checks_captured_bytes_before_browser_boundary(self):
        from unittest.mock import patch
        from ep_auditor.core.events import EventBus
        from ep_auditor.core.run_manager import RunManager
        from ep_auditor.ui.main_window import AuditorWindow
        import hashlib
        run = self.storage.new_run('coverage', 'synthetic heatmap UI protocol control', 'Coverage')
        raw = RawEvidence(run.run_dir)
        html = b'<html>explicit synthetic UI test only</html>'
        report = json.dumps({'schemaVersion': 1, 'provenance': 'compiler_instrumentation', 'confidence': 'derived',
            'heatmap': [{'path': 'raw/coverage/heatmap/index.html', 'sha256': hashlib.sha256(html).hexdigest()}]}).encode()
        for name, payload in [('coverage/heatmap/index.html', html), ('coverage/report.json', report)]:
            raw.append(name, payload, collector='synthetic_ui_protocol_test', version='1', start_ns=1, end_ns=2,
                health={'capacity': len(payload), 'observed': len(payload), 'dropped': 0, 'incomplete_reason': None})
        self.storage.finish_run(run, 'PASS')
        events = EventBus()
        window = AuditorWindow(self.config, RunManager(self.config, self.storage, events), self.storage, events)
        self.addCleanup(window._on_close)
        window.root.withdraw()
        state = window.product_views['Coverage']
        state['run_dir'] = run.run_dir
        with patch('webbrowser.open', return_value=True) as browser:
            window._open_coverage_heatmap()
            browser.assert_called_once_with((run.run_dir / 'raw/coverage/heatmap/index.html').resolve().as_uri())
        path = run.run_dir / 'raw/coverage/heatmap/index.html'
        path.chmod(stat.S_IREAD | stat.S_IWRITE)
        path.write_bytes(b'changed')
        with patch('webbrowser.open') as browser:
            window._open_coverage_heatmap()
            browser.assert_not_called()
        self.assertIn('Evidence unavailable:', state['status'].get())


class StressTests(unittest.TestCase):
    # Reuse setup, not the view tests (those retain their own focused selection).
    setUp = ViewTests.setUp
    def manager(self, injectors, command=None):
        from ep_auditor.core.events import EventBus
        from ep_auditor.core.run_manager import RunManager
        (self.project / 'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(Controlled LANGUAGES NONE)\n')
        self.config.data['stress'] = {
            'command': command or [sys.executable, '-c', 'import time; time.sleep(.2)'],
            'working_dir': str(self.project), 'iterations': 1, 'timeout_seconds': 20, 'budget_seconds': 120,
            'injectors': injectors, 'observer': str(ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorObservation.exe')}
        manager = RunManager(self.config, self.storage, EventBus())
        self.addCleanup(manager.shutdown)
        return manager

    def test_physical_cpu_memory_and_worker_pressure_have_scope_and_cleanup_controls(self):
        manager = self.manager([{'type': 'cpu_pressure', 'workers': 1},
                                {'type': 'memory_pressure', 'bytes': 16 * 1024**2},
                                {'type': 'worker_pressure', 'workers': 2}])
        manager.execute('stress', 'Fast')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        view = query_run_view(Path(record['run_dir']), 'Stress')
        self.assertEqual(record['status'], 'PASS', [{key: row.get(key) for key in
            ('type', 'status', 'reason', 'measured_effect')} | {'capture_statuses': [control['capture_status'] for control in row.get('controls', [])]}
            for row in view['items']])
        self.assertEqual(len(view['items']), 3)
        self.assertEqual({row['status'] for row in view['items']}, {'PASS'})
        self.assertTrue(all(row['cleanup'] == 'observed_stopped' for row in view['items']))
        self.assertFalse(manager.busy)
        import copy
        from ep_auditor.core.run_manager import evaluate_stress_controls
        cpu = view['items'][0]
        warm_only = copy.deepcopy(cpu['controls'])
        for helper in warm_only[1]['helpers']:
            helper['cpu_seconds_during_target'] = 0
            helper['cpu_seconds'] = 100
        self.assertEqual(evaluate_stress_controls(cpu['parameters'], warm_only)['status'], 'UNKNOWN')
        failed_driver = copy.deepcopy(cpu['controls'])
        failed_driver[1]['capture_status'] = 'FAIL'
        self.assertEqual(evaluate_stress_controls(cpu['parameters'], failed_driver)['status'], 'FAIL')

    def test_native_async_cancel_control_under_owned_cpu_pressure(self):
        fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorShadowFixture.exe'
        manager = self.manager([{'type': 'cpu_pressure', 'workers': 1}], [str(fixture), 'cancel-storm', '200'])
        manager.execute('stress', 'Deep')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        outcome = query_run_view(Path(record['run_dir']), 'Stress')['items'][0]
        self.assertEqual(outcome['status'], 'PASS', outcome.get('reason'))
        for control in outcome['controls']:
            child = Path(control['run_ref']['run_directory'])
            payload = json.loads(RawEvidence(child).read_bytes('process/stdout.bin'))
            self.assertEqual((payload['requests'], payload['cancelled'], payload['completed']), (400, 200, 200))
            self.assertEqual(payload['scenario'], 'native-async-cancel-control')
            timeline = query_run_view(child, 'Timeline')
            self.assertTrue(timeline['items'])
            self.assertTrue(timeline['complete'])
            # Counts above are target reports, not independent API observation.
            causal = [row for row in timeline['items'] if row.get('eventType', '').startswith('causal.')]
            self.assertTrue(causal)
            self.assertEqual({row['provenance'] for row in causal}, {'engine_probe'})

    def test_physical_allocation_failure_requires_external_exception_and_negative_controls(self):
        fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorShadowFixture.exe'
        manager = self.manager([{'type': 'allocation_failure', 'nth': 2, 'scope': 'DefaultAllocator.Allocate',
                                 'enabled_exit_code': 1}], [str(fixture), 'allocation-control'])
        manager.execute('stress', 'Fast')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        view = query_run_view(Path(record['run_dir']), 'Stress')
        self.assertEqual(record['status'], 'PASS', view)
        outcome = view['items'][0]
        self.assertEqual(outcome['measured_effect']['external_cpp_exceptions'], 1)
        self.assertEqual([row['target_exit_code'] for row in outcome['controls']], [0, 1, 0])
        # Remove the independent corroboration: an ACK/exit difference must not PASS.
        from ep_auditor.core.run_manager import evaluate_stress_controls
        controls = outcome['controls']
        controls[1]['external_cpp_exceptions'] = 0
        self.assertEqual(evaluate_stress_controls(outcome['parameters'], controls)['status'], 'UNKNOWN')

    def test_invalid_unbounded_and_missing_effect_never_pass(self):
        from ep_auditor.core.run_manager import validate_stress, evaluate_stress_controls
        manager = self.manager([{'type': 'cpu_pressure', 'workers': 0}])
        with self.assertRaises(ValueError):
            manager.execute('stress', 'Fast')
        self.assertFalse(manager.busy)
        self.assertEqual(self.storage.list_runs(), [])
        self.config.data['stress']['injectors'][0]['workers'] = 1
        self.config.data['stress']['iterations'] = 0
        with self.assertRaises(ValueError):
            validate_stress(self.config.data['stress'])
        self.assertEqual(evaluate_stress_controls({'type': 'cpu_pressure'}, [])['status'], 'FAIL')

    def test_unavailable_observer_retains_incomplete_child_without_false_complete(self):
        binary = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorShadowFixture.exe'
        manager = self.manager([{'type': 'allocation_failure', 'nth': 1, 'scope': 'DefaultAllocator.Allocate'}],
                               [str(binary), '--allocator-injection'])
        self.config.data['stress']['observer'] = str(self.project / 'missing-observer.exe')
        manager.execute('stress', 'Fast')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        self.assertEqual(record['status'], 'FAIL')
        children = [row for row in self.storage.list_runs() if row['kind'] == 'stress-control']
        self.assertEqual(len(children), 1)
        self.assertEqual(children[0]['status'], 'INCOMPLETE')
        raw = json.loads(RawEvidence(Path(record['run_dir'])).read_bytes('stress/campaign.json'))
        self.assertEqual([row['run_id'] for row in raw['children']], [children[0]['run_id']])
        view = query_run_view(Path(record['run_dir']), 'Stress')
        self.assertFalse(view['complete'])
        self.assertEqual(view['items'][0]['status'], 'FAIL')
        self.assertEqual(view['items'][0]['child_refs'][0]['run_id'], children[0]['run_id'])
        self.assertFalse(manager.busy)

    def test_physical_timeout_replays_driver_failure_without_false_pass(self):
        manager = self.manager([{'type': 'memory_pressure', 'bytes': 16 * 1024**2}],
                               [sys.executable, '-c', 'import time; time.sleep(5)'])
        self.config.data['stress']['timeout_seconds'] = 1
        manager.execute('stress', 'Fast')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        self.assertEqual(record['status'], 'FAIL')
        view = query_run_view(Path(record['run_dir']), 'Stress')
        outcome = view['items'][0]
        self.assertEqual(outcome['status'], 'FAIL')
        for control in outcome['controls']:
            self.assertEqual(control['error'], 'target timeout')
            self.assertEqual(control['capture_status'], 'FAIL')
            driver = json.loads(RawEvidence(Path(control['run_ref']['run_directory'])).read_bytes('process/lifecycle.json'))
            self.assertEqual(driver['exit_code'], 124)
            self.assertFalse(driver['cancelled'])

    def test_physical_scoped_io_delay_on_production_save_restore(self):
        binary = ROOT / 'build/audit-deep/EngineFramework/build/tests/Release/EpidemicExtendedGameplayIntegrationSaveParticipantTests.exe'
        self.assertTrue(binary.is_file())
        manager = self.manager([{'type': 'io_delay', 'delay_ms': 500, 'scope': 'capture'}],
                               [str(binary), '--auditor-save-load'])
        self.config.data.update(project_root=str(ROOT), build_dir=str(ROOT / 'build/audit-deep'))
        self.config.data['cmake']['build_config'] = 'Release'
        self.config.data['stress']['budget_seconds'] = 600
        manager.execute('stress', 'Deep')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        outcome = query_run_view(Path(record['run_dir']), 'Stress')['items'][0]
        self.assertEqual(record['status'], 'PASS', {key: outcome.get(key) for key in ('status', 'reason', 'measured_effect')})
        self.assertGreaterEqual(outcome['measured_effect']['completion_latency_delta_seconds'], .375)
        self.assertEqual(outcome['measured_effect']['media_irp_latency'], 'UNKNOWN')
        self.assertEqual([control['target_exit_code'] for control in outcome['controls']], [0, 0, 0])

    def test_production_save_restore_under_measured_cpu_window(self):
        binary = ROOT / 'build/audit-deep/EngineFramework/build/tests/Release/EpidemicExtendedGameplayIntegrationSaveParticipantTests.exe'
        self.assertTrue(binary.is_file())
        manager = self.manager([{'type': 'cpu_pressure', 'workers': 1}],
            [str(binary), '--auditor-save-load-stress', '2'])
        self.config.data.update(project_root=str(ROOT), build_dir=str(ROOT / 'build/audit-deep'))
        self.config.data['cmake']['build_config'] = 'Release'
        self.config.data['capture']['exact_capacity_events'] = 1048576
        self.config.data['stress']['budget_seconds'] = 900
        self.config.data['stress']['storage_budget_bytes'] = 2 * 1024**3
        manager.execute('stress', 'Deep')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        outcome = query_run_view(Path(record['run_dir']), 'Stress')['items'][0]
        self.assertEqual(outcome['status'], 'PASS', outcome.get('reason'))
        self.assertGreater(outcome['measured_effect']['cpu_seconds'], .005)
        for control in outcome['controls']:
            child = Path(control['run_ref']['run_directory'])
            payload = json.loads(RawEvidence(child).read_bytes('process/stdout.bin'))
            self.assertEqual((payload['iterations'], payload['completed'], payload['failures'], payload['state_checks']), (2, 2, 0, 2))
            actions = query_run_view(child, 'Actions')
            self.assertEqual(len(actions['items']), 2)
            self.assertTrue(actions['complete'])
            self.assertEqual({row['id'] for row in actions['items']}, {'save-load-action-1', 'save-load-action-2'})

    def test_physical_small_persistent_pressure_scope_on_production_save_restore(self):
        binary = ROOT / 'build/audit-memory/EngineFramework/build/tests/Release/EpidemicExtendedGameplayIntegrationSaveParticipantTests.exe'
        self.assertTrue(binary.is_file())
        self.config.data['capture']['memory_scope'] = 'persistent_pressure'
        run = self.storage.new_run('process', 'bounded production persistence capture', 'Memory')
        result = ManagedProcessRunner(self.config).run([str(binary), '--auditor-save-load-stress', '2'],
                                                       self.project, run.run_dir, 'Memory', timeout=60)
        self.assertEqual(result.exit_code, 0)
        self.storage.finish_run(run, 'PASS')
        self.assertEqual(run.status, 'PASS')
        raw = RawEvidence(run.run_dir)
        lifecycle = json.loads(raw.read_bytes('process/lifecycle.json'))
        self.assertEqual(lifecycle['capture_scope']['persistent_count'], 2)
        self.assertEqual(json.loads(raw.read_bytes('process/stdout.bin'))['state_checks'], 2)
        self.assertEqual(len([row for row in raw.verify()['raw_files']
                             if row['path'].startswith('raw/persistent/files/')]), 2)
        self.assertEqual(query_run_view(run.run_dir, 'Call Tree')['availability'], 'UNKNOWN')
        self.assertEqual(query_run_view(run.run_dir, 'Actions')['availability'], 'UNKNOWN')

    def test_thousand_production_save_restore_under_memory_pressure(self):
        binary = ROOT / 'build/audit-memory/EngineFramework/build/tests/Release/EpidemicExtendedGameplayIntegrationSaveParticipantTests.exe'
        self.assertTrue(binary.is_file())
        if retained := os.environ.get('EP_AUDITOR_FIXTURE_EVIDENCE'):
            evidence = Path(retained) / ('save-restore-memory-' + uuid.uuid4().hex)
            evidence.mkdir(parents=True, exist_ok=False)
            self.config.data['storage'] = {'runs_dir': str(evidence / 'runs'),
                                         'database': str(evidence / 'auditor.sqlite3')}
            self.storage = RunStorage(self.config)
        manager = self.manager([{'type': 'memory_pressure', 'bytes': 32 * 1024**2}],
            [str(binary), '--auditor-save-load-stress', '1000'])
        self.config.data.update(project_root=str(ROOT), build_dir=str(ROOT / 'build/audit-memory'))
        self.config.data['cmake']['build_config'] = 'Release'
        self.config.data['capture']['memory_scope'] = 'persistent_pressure'
        self.config.data['stress'].update(timeout_seconds=900, budget_seconds=3300, storage_budget_bytes=2 * 1024**3)
        manager.execute('stress', 'Memory')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        outcome = query_run_view(Path(record['run_dir']), 'Stress')['items'][0]
        self.assertEqual(outcome['status'], 'PASS', outcome.get('reason'))
        for control in outcome['controls']:
            raw = RawEvidence(Path(control['run_ref']['run_directory']))
            payload = json.loads(raw.read_bytes('process/stdout.bin'))
            self.assertEqual((payload['iterations'], payload['completed'], payload['failures'], payload['state_checks']),
                             (1000, 1000, 0, 1000))
            lifecycle = json.loads(raw.read_bytes('process/lifecycle.json'))
            self.assertEqual(lifecycle['capture_scope']['persistent_count'], 1000)
            files = [row for row in raw.verify()['raw_files'] if row['path'].startswith('raw/persistent/files/')]
            self.assertEqual(len(files), 1000)
            self.assertEqual({Path(row['path']).name for row in files},
                             {f'inventory-save-{cycle}.bin' for cycle in range(1, 1001)})
            self.assertTrue(all(row['size'] > 0 and row['sha256'] for row in files))
            self.assertFalse(any(row['path'].startswith(('raw/exact/', 'raw/causal/', 'raw/diagnostics/'))
                                 for row in raw.verify()['raw_files']))

    def test_owned_rolling_capture_under_cpu_pressure(self):
        fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorShadowFixture.exe'
        manager = self.manager([{'type': 'cpu_pressure', 'workers': 1}], [str(fixture), 'rolling-long-frame'])
        self.config.data['capture']['rolling'] = {'pre_seconds': .2, 'post_seconds': .2, 'capacity_bytes': 8 * 1024**2}
        manager.execute('stress', 'Deep')
        record = next(row for row in self.storage.list_runs() if row['kind'] == 'stress')
        outcome = query_run_view(Path(record['run_dir']), 'Stress')['items'][0]
        self.assertEqual(outcome['status'], 'PASS', outcome.get('reason'))
        for control in outcome['controls']:
            raw = RawEvidence(Path(control['run_ref']['run_directory']))
            report = json.loads(raw.read_bytes('rolling/probes.json'))
            self.assertTrue(report['retained_bytes_complete'])
            self.assertTrue(report['complete'])
            self.assertEqual(report['dropped'], 0)
            self.assertTrue(any(row['trigger'] == 'long_frame' and row['records'] for row in report['snapshots']))
            self.assertEqual(report['exact_availability'], 'UNKNOWN', 'Probe-only fixture must not pretend to be compiler tracing')


class IntegratedAcceptanceTests(unittest.TestCase):
    setUp = ViewTests.setUp

    def test_real_hot_path_compare_agent_gui_and_incompatible_control(self):
        import time
        from ep_auditor.core.events import EventBus
        from ep_auditor.core.run_manager import RunManager
        from ep_auditor.ui.main_window import AuditorWindow
        self.config.data.update(project_root=str(ROOT), build_dir=str(ROOT / 'build/auditor-native'))
        fixture = ROOT / 'build/auditor-native/bin/Debug/EpidemicAuditorExactFixture.exe'
        runs = []
        for variant in ('hot-baseline', 'hot-regression'):
            run = self.storage.new_run('scenario', variant, 'Fast')
            result = ManagedProcessRunner(self.config).run([str(fixture), variant], fixture.parent, run.run_dir, 'Fast')
            self.assertEqual(result.exit_code, 0)
            self.storage.finish_run(run, 'PASS')
            self.assertEqual(run.status, 'PASS')
            runs.append(run)
        result = compare_runs(*(run.run_dir for run in runs))
        self.assertEqual(result['compatibility'], 'compatible', result['incompatibilities'])
        self.assertEqual(result['regression_verdict'], 'candidate')
        dashboard = query_run_view(runs[1].run_dir, 'Dashboard', baseline=runs[0].run_dir)['items'][0]
        self.assertEqual(dashboard['baseline_state'], 'compatible')
        self.assertEqual(dashboard['baseline_run_id'], runs[0].run_id)
        self.assertEqual(dashboard['regressions']['verdict'], 'candidate')
        self.assertTrue(dashboard['regressions']['items'])
        self.assertTrue(dashboard['regressions']['items'][0]['evidence_refs'])
        hot = next(row for row in result['differences']['function_timings'] if
                   row['metrics']['p95_ns']['delta'] is not None and row['metrics']['p95_ns']['delta'] > 10000000)
        self.assertTrue(hot['evidence_refs'])
        self.assertTrue(result['differences']['findings'])
        self.assertEqual({row['confidence'] for row in result['differences']['findings']}, {'heuristic'})
        paged = compare_runs(*(run.run_dir for run in runs), limit=1)
        token = paged['continuations']['function_timings']
        resumed = compare_runs(*(run.run_dir for run in runs), limit=1, cursor=token)
        self.assertNotEqual(paged['differences']['function_timings'], resumed['differences']['function_timings'])
        with self.assertRaises(EvidenceError):
            compare_runs(runs[1].run_dir, runs[0].run_dir, limit=1, cursor=token)
        output = export_agent_pack(runs[1].run_dir, baseline=runs[0].run_dir)
        self.assertEqual(json.loads((output / 'REGRESSIONS.json').read_text())['items'][0]['regression_verdict'], 'candidate')
        function_id = hot['function_ids'][1]
        history = self.storage.history_for_function(function_id, limit=1)
        self.assertTrue(history['performance_regression'])
        self.assertTrue(history['truncated'])
        second = self.storage.history_for_function(function_id, limit=1, cursor=history['continuation'])
        self.assertNotEqual(history['items'][0]['run_id'], second['items'][0]['run_id'])
        self.assertGreater(second['items'][0]['mean_ns'], history['items'][0]['mean_ns'])
        regressions = self.storage.history_for_regression(function_id, limit=1)
        self.assertIsNone(regressions['items'][0]['regression']['candidate'])
        measured = self.storage.history_for_regression(function_id, limit=1, cursor=regressions['continuation'])
        self.assertTrue(measured['items'][0]['regression']['candidate'])
        self.assertEqual(measured['items'][0]['regression']['baseline_run_id'], regressions['items'][0]['run_id'])
        self.assertTrue(measured['items'][0]['regression']['baseline_evidence']['sha256'])
        with self.assertRaises(EvidenceError):
            self.storage.history_for_function('FunctionId:not-captured', limit=1, cursor=history['continuation'])
        with self.assertRaises(EvidenceError):
            self.storage.history_for_function(function_id, limit=201)
        self.assertEqual(self.storage.history_for_function('FunctionId:not-captured')['availability'], 'UNKNOWN')
        investigation = export_investigation(runs[1].run_dir, 'function', function_id)
        self.assertEqual(investigation['items'][0]['functionId'], function_id)
        self.assertEqual({row['functionId'] for row in investigation['items']}, {function_id},
                         'References from other callers must not select those callers as the primary function')
        missing = query_run_view(runs[1].run_dir, 'Functions', scope='FunctionId:nonexistent')
        self.assertEqual(missing['availability'], 'UNKNOWN')
        self.assertFalse(missing['complete'])
        events = EventBus()
        window = AuditorWindow(self.config, RunManager(self.config, self.storage, events), self.storage, events)
        self.addCleanup(window._on_close)
        window.root.withdraw()
        errors = []
        window.root.report_callback_exception = lambda *args: errors.append(args)
        window.root.tk.createcommand('capture_tcl_error', lambda text: errors.append(text))
        window.root.tk.eval('proc bgerror {message} {capture_tcl_error $message}')
        for name in ('Dashboard', 'Modules'):
            product = window.product_views[name]
            product['run_id'].set(runs[1].run_id)
            if name == 'Dashboard':
                product['baseline'].set(runs[0].run_id)
            window._request_product_view(name, reset=True)
            deadline = time.monotonic() + 30
            while window.view_future is not None and time.monotonic() < deadline:
                window.root.update()
                time.sleep(.01)
            self.assertIsNone(window.view_future)
            self.assertFalse(errors)
            self.assertEqual(product['page'], query_run_view(runs[1].run_dir, name,
                baseline=runs[0].run_dir if name == 'Dashboard' else None))
            self.assertTrue(product['tree'].get_children())
            if name == 'Modules':
                for index, module in enumerate(product['page']['items']):
                    self.assertEqual(product['tree'].item(str(index), 'text'), module['moduleId'])
            else:
                self.assertEqual(product['tree'].item('0', 'text'), runs[1].run_id)
                self.assertTrue(product['page']['items'][0]['panels']['Functions']['observations'])
                self.assertEqual(product['page']['items'][0]['baseline_state'], 'compatible')
                self.assertEqual(product['page']['items'][0]['regressions']['verdict'], 'candidate')
        state = window.product_views['Compare']
        state['run_id'].set(runs[1].run_id)
        state['baseline'].set(runs[0].run_id)
        window._request_product_view('Compare', reset=True)
        deadline = time.monotonic() + 30
        while window.view_future is not None and time.monotonic() < deadline:
            window.root.update()
            time.sleep(.01)
        self.assertIsNone(window.view_future)
        self.assertFalse(errors)
        self.assertEqual(state['page']['regression_verdict'], 'candidate')
        self.assertTrue(state['tree'].get_children())
        history_state = window.product_views['History']
        history_state['run_id'].set(runs[1].run_id)
        history_state['scope'].set(function_id)
        window._request_product_view('History', reset=True)
        deadline = time.monotonic() + 30
        while window.view_future is not None and time.monotonic() < deadline:
            window.root.update()
            time.sleep(.01)
        self.assertIsNone(window.view_future)
        self.assertTrue(history_state['page']['performance_regression'])
        self.assertTrue(any(history_state['chart'].type(item) == 'line' for item in history_state['chart'].find_all()))
        self.assertFalse(errors)
        calls = query_run_view(runs[1].run_dir, 'Call Tree')
        window._draw_product_chart(state, 'Call Tree', calls['items'])
        self.assertTrue(any(tag.startswith('row:') for item in state['chart'].find_all()
                            for tag in state['chart'].gettags(item)), 'Physical call intervals must render selectable flame bars')
        state = window.product_views['Functions']
        state['run_id'].set(runs[1].run_id)
        window._request_product_view('Functions', reset=True)
        deadline = time.monotonic() + 30
        while window.view_future is not None and time.monotonic() < deadline:
            window.root.update()
            time.sleep(.01)
        self.assertIsNone(window.view_future)
        selected = next(index for index, row in enumerate(state['page']['items'])
                        if row.get('symbol', {}).get('source_file') and row['symbol'].get('source_line'))
        state['tree'].selection_set(str(selected))
        window._show_product_source('Functions')
        self.assertIn('captured source hash', state['detail'].get('1.0', 'end'))
        self.assertFalse(errors)
