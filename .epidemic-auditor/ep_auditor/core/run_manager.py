from __future__ import annotations

import json
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Iterable

from ep_auditor.analyzers.static_scan import scan_sources, capture_scan
from ep_auditor.analyzers.summary import aggregate_test_results, summarize_process_samples
from ep_auditor.config import AuditorConfig
from ep_auditor.core.build import BuildController
from ep_auditor.core.events import EventBus
from ep_auditor.core.process import ManagedProcessRunner, ProcessLaunchError
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.tests import TestCatalog, TestExecutor
from ep_auditor.core.lifecycle import MODES, seal_run_capture
from ep_auditor.core.evidence import RawEvidence
from ep_auditor.core.lineage import EvidenceLineage, normalized_record
from ep_auditor.core.batch import capture_batch
from ep_auditor.exporters.agent import export_agent_pack
from ep_auditor.models import RunRecord, TestCase


class RunManager:
    available_modes = MODES

    def __init__(self, config: AuditorConfig, storage: RunStorage, events: EventBus) -> None:
        self.config = config
        self.storage = storage
        self.events = events
        self.runner = ManagedProcessRunner(config)
        self.builder = BuildController(config, self.runner)
        self.catalog = TestCatalog(config, self.runner)
        self.tests = TestExecutor(config, self.runner, storage)
        self._executor = ThreadPoolExecutor(max_workers=1, thread_name_prefix="auditor")
        self._busy = threading.Event()
        self._cancel_requested = threading.Event()
        self._cancel_reason = None
        self.runner.operation_cancel = self._cancel_requested
        self._operation_lock = threading.Lock()
        self._closed = False

    @property
    def busy(self) -> bool:
        return self._busy.is_set()

    def _run_stress(self, capture_mode):
        import sys
        import time
        import uuid
        from ep_auditor.core.lifecycle import authoritative_run
        from ep_auditor.core.evidence import EvidenceError, atomic_json, digest
        spec = self.config.data['stress']
        record = self.storage.new_run('stress', 'Owned injection campaign', capture_mode)
        self.events.emit('run_started', run_id=record.run_id, title=record.title)
        outcomes, references = [], []
        active_child = None
        stored_bytes = 0
        deadline = time.monotonic() + spec['budget_seconds']
        try:
            for iteration in range(spec['iterations']):
                for injector in spec['injectors']:
                    controls = []
                    for phase in ('disabled', 'enabled', 'outside_scope'):
                        if self._cancel_requested.is_set():
                            raise InterruptedError('owned campaign stopped')
                        remaining = deadline - time.monotonic()
                        if remaining <= 0:
                            raise TimeoutError('campaign time budget exhausted')
                        if stored_bytes >= spec['storage_budget_bytes']:
                            raise MemoryError('campaign storage budget exhausted; owned captures retained')
                        child = self.storage.new_run('stress-control', phase + ' ' + injector['type'], capture_mode)
                        active_child = child
                        child.metadata.update(parent_run_id=record.run_id, iteration=iteration, control=phase)
                        self.storage.write_json(child.run_dir / 'run.json', child.to_dict())
                        request = {**spec, 'injector': injector, 'phase': phase,
                                   'injector_id': 'InjectorId:' + str(uuid.uuid4()),
                                   'timeout_seconds': min(spec['timeout_seconds'], remaining),
                                   'observation_path': str(child.run_dir / 'stress-observation.json')}
                        request_path = child.run_dir / 'stress-request.json'
                        atomic_json(request_path, request)
                        started = time.perf_counter_ns()
                        # This driver and every target/helper descendant belong to
                        # the existing suspended-before-assignment Windows Job.
                        command = [sys.executable, '-B', '-c',
                            'import sys; from ep_auditor.core.run_manager import stress_child; raise SystemExit(stress_child(sys.argv[1]))',
                            str(request_path)]
                        if injector['type'] == 'allocation_failure':
                            from ep_auditor.collectors.modules import capture_api_snapshot
                            import shutil
                            binary = Path(shutil.which(spec['command'][0]) or spec['command'][0]).resolve(strict=True)
                            candidates = [Path(spec['observer'])] if spec.get('observer') else [
                                Path(sys.executable).parent / 'EpidemicAuditorObservation.exe',
                                Path(sys.executable).parent.parent / 'EpidemicAuditorObservation.exe',
                                Path(__file__).resolve().parents[3] / '.ai/tools/auditor/EpidemicAuditorObservation.exe']
                            helper = next((path for path in candidates if path.is_file()), None)
                            if helper is None:
                                raise EvidenceError('native exception observer unavailable')
                            environment = {'EP_AUDITOR_ALLOC_SCOPE': '', 'EP_AUDITOR_ALLOC_FAIL_N': '',
                                           'EP_AUDITOR_ALLOC_MODULE': '', 'EP_AUDITOR_IO_SCOPE': '', 'EP_AUDITOR_IO_DELAY_MS': ''}
                            if phase != 'disabled':
                                environment.update(EP_AUDITOR_ALLOC_SCOPE=injector['scope'] if phase == 'enabled' else 'outside.allocator',
                                    EP_AUDITOR_ALLOC_FAIL_N=str(injector['nth']), EP_AUDITOR_ALLOC_MODULE=str(binary))
                            snapshot, _ = capture_api_snapshot(self.config, child.run_dir, binary, spec['command'][1:],
                                helper=helper, capture_mode=capture_mode, env_extra=environment, runner=self.runner,
                                timeout_seconds=min(25, request['timeout_seconds']))
                            from ep_auditor.analyzers.diagnostics import analyze_exception_snapshot
                            exceptions = analyze_exception_snapshot(snapshot)
                            observation = {'phase': phase, 'injector_id': request['injector_id'], 'command': spec['command'],
                                'parameters': injector,
                                'target_exit_code': snapshot['exit_code'], 'helpers': [], 'cleanup': 'observed_stopped',
                                'target_wall_seconds': (snapshot['end_qpc'] - snapshot['begin_qpc']) / snapshot['qpc_frequency'],
                                'requested_interval': {'timeout_seconds': request['timeout_seconds']},
                                'observed_interval': {'begin_qpc': snapshot['begin_qpc'], 'end_qpc': snapshot['end_qpc'],
                                                      'qpc_frequency': snapshot['qpc_frequency']},
                                'external_cpp_exceptions': sum(event['code'] == 0xE06D7363 for event in exceptions['events']),
                                'exception_capture_complete': exceptions['complete'], 'provenance': 'external', 'confidence': 'derived'}
                            atomic_json(Path(request['observation_path']), observation)
                            physical_exit, cancelled = snapshot['exit_code'], False
                        else:
                            result = self.runner.run(command, Path(spec['working_dir']), child.run_dir,
                                capture_mode, self._log,
                                env_extra={'PYTHONPATH': str(Path(__file__).resolve().parents[2])},
                                timeout=request['timeout_seconds'] + 5)
                            physical_exit, cancelled = result.exit_code, result.cancelled
                        ended = time.perf_counter_ns()
                        raw = RawEvidence(child.run_dir)
                        for name, payload in [('request.json', json.dumps(request).encode())]:
                            raw.append('stress/' + name, payload, collector='auditor_stress_request', version='1',
                                start_ns=started, end_ns=ended, health={'capacity': len(payload), 'observed': len(payload),
                                'dropped': 0, 'incomplete_reason': None})
                        observation_path = Path(request['observation_path'])
                        if not observation_path.is_file() or observation_path.stat().st_size > 1024**2:
                            raise EvidenceError('owned stress observation missing/oversized')
                        observation = json.loads(observation_path.read_text(encoding='utf-8'))
                        raw.import_file('stress/observation.json', observation_path, digest(observation_path),
                            collector='external_owned_stress_driver', version='1', start_ns=started, end_ns=ended,
                            health={'capacity': 1024**2, 'observed': observation_path.stat().st_size, 'dropped': 0,
                                    'incomplete_reason': None})
                        status = 'CANCELLED' if cancelled and self._cancel_requested.is_set() else (
                            'FAIL' if cancelled or physical_exit != 0 else 'PASS')
                        self.storage.finish_run(child, status)
                        active_child = None
                        stored_bytes += sum(path.stat().st_size for path in child.run_dir.rglob('*') if path.is_file())
                        header = authoritative_run(child.run_dir)
                        ref = {'run_id': child.run_id, 'run_directory': str(child.run_dir),
                               'manifest_sha256': digest(child.run_dir / 'manifest.json'),
                               'run_sha256': digest(child.run_dir / 'run.json')}
                        references.append(ref)
                        if stored_bytes > spec['storage_budget_bytes']:
                            raise MemoryError('campaign storage budget exceeded; no complete campaign verdict')
                        controls.append({**observation, 'run_ref': ref, 'capture_status': header['status']})
                    outcome = evaluate_stress_controls(injector, controls)
                    outcomes.append(dict(iteration=iteration, **outcome))
                    if len(json.dumps([outcomes, references], allow_nan=False).encode()) > 12 * 1024**2:
                        raise MemoryError('bounded campaign outcome capacity reached')
            status = 'FAIL' if any(row['status'] == 'FAIL' for row in outcomes) else (
                'INCOMPLETE' if not outcomes or any(row['status'] != 'PASS' for row in outcomes) else 'PASS')
        except InterruptedError:
            status = 'CANCELLED'
        except Exception as error:
            status = 'CANCELLED' if self._cancel_requested.is_set() else 'FAIL'
            outcomes.append({'status': status, 'reason': str(error)})
            self.events.emit('error', text=str(error))
        if active_child is not None:
            # A failed launch/collector must not leave an admitted child looking
            # like a successful completed control. Finalization remains strict.
            self.storage.finish_run(active_child, 'CANCELLED' if status == 'CANCELLED' else 'INCOMPLETE')
            references.append({'run_id': active_child.run_id, 'run_directory': str(active_child.run_dir),
                'manifest_sha256': digest(active_child.run_dir / 'manifest.json'),
                'run_sha256': digest(active_child.run_dir / 'run.json')})
        if outcomes and 'controls' not in outcomes[-1] and outcomes[-1]['status'] != 'PASS':
            outcomes[-1]['child_refs'] = references[-3:]
        now = time.perf_counter_ns()
        payload = json.dumps({'schemaVersion': 1, 'eventType': 'stress.campaign', 'outcomes': outcomes,
                              'children': references, 'status': status, 'provenance': 'external',
                              'storage_health': {'budget_bytes': spec['storage_budget_bytes'], 'observed_bytes': stored_bytes},
                              'confidence': 'derived'}, allow_nan=False).encode()
        RawEvidence(record.run_dir).append('stress/campaign.json', payload, collector='owned_campaign_analysis', version='1',
            start_ns=now, end_ns=now, health={'capacity': 16 * 1024**2, 'observed': len(payload), 'dropped': 0,
                                            'incomplete_reason': None})
        self.storage.finish_run(record, status, {'outcomes': outcomes})
        self._export_run(record.run_dir)
        self.events.emit('run_finished', run_id=record.run_id, status=record.status)
        return record

    def shutdown(self) -> None:
        with self._operation_lock:
            self._closed = True
        self._cancel_reason = "application_shutdown"
        self._cancel_requested.set()
        self.runner.cancel("application_shutdown")
        self._executor.shutdown(wait=False, cancel_futures=True)

    def stop(self) -> None:
        self._cancel_reason = "user_stop"
        self._cancel_requested.set()
        self.runner.cancel()
        self.events.emit("status", text="Stop requested")

    def discover_tests_async(self, capture_mode: str = "Fast") -> None:
        return self.submit_operation("discover-tests", capture_mode)

    def configure_async(self, capture_mode: str) -> None:
        return self.submit_operation("configure", capture_mode)

    def build_async(self, capture_mode: str) -> None:
        return self.submit_operation("build", capture_mode)

    def run_all_tests_async(self, capture_mode: str) -> None:
        return self.submit_operation("run-all-tests", capture_mode)

    def run_selected_tests_async(self, selected: Iterable[TestCase], capture_mode: str) -> None:
        selected_list = list(selected)
        operation = "run-one-test" if len(selected_list) == 1 else "run-selected-tests"
        return self.submit_operation(operation, capture_mode, test_ids=[test.test_id for test in selected_list])

    def build_and_run_async(self, capture_mode: str) -> None:
        return self.submit_operation("build-and-run", capture_mode)

    def start_game_async(self, capture_mode: str) -> None:
        return self.submit_operation("start-game", capture_mode)

    def run_group_async(self, labels: list[str], capture_mode: str):
        return self.submit_operation("run-group", capture_mode, labels=labels)

    def execute(self, operation_id: str, capture_mode: str = "Fast", *, test_ids=None, labels=None):
        from ep_auditor.core.operations import admit, perform
        if operation_id == "stop":
            self.stop()
            return {"operation_id": "stop"}
        return perform(self, admit(self, operation_id, capture_mode, test_ids, labels, "headless"))

    def submit_operation(self, operation_id: str, capture_mode: str, *, test_ids=None, labels=None):
        from ep_auditor.core.operations import admit, perform
        try:
            admitted = admit(self, operation_id, capture_mode, test_ids, labels, "gui")
        except (ValueError, RuntimeError) as error:
            self.events.emit("error", text=str(error))
            return None

        def action():
            try:
                return perform(self, admitted)
            except Exception as error:
                self.events.emit("error", text=str(error))
                raise
        try:
            return self._executor.submit(action)
        except RuntimeError:
            self._busy.clear()
            raise

    def static_scan_async(self) -> None:
        self._submit("Static scan", self._static_scan_only)

    def _submit(self, title: str, func) -> None:
        if self.busy:
            self.events.emit("error", text=f"Auditor is busy. Cannot start: {title}")
            return
        self._busy.set()
        self._cancel_requested.clear()
        self._cancel_reason = None
        self.events.emit("busy", busy=True, title=title)

        def wrapped() -> None:
            try:
                func()
            except Exception as exc:
                self.events.emit("error", text=f"{title} failed: {exc}")
            finally:
                self._busy.clear()
                self.events.emit("busy", busy=False, title=title)
                self.events.emit("runs_changed")

        self._executor.submit(wrapped)

    def _log(self, channel: str, line: str) -> None:
        self.events.emit("log", channel=channel, text=line)

    def _discover_tests(self, capture_mode: str = "Fast") -> None:
        self.events.emit("status", text="Discovering CTest tests...")
        self.catalog = TestCatalog(self.config.for_mode(capture_mode), self.runner)
        tests = self.catalog.discover()
        self.events.emit("tests_discovered", tests=[t.to_dict() for t in tests])
        self.events.emit("status", text=f"Discovered {len(tests)} tests")

    def _run_process_operation(self, kind: str, title: str, capture_mode: str, operation) -> None:
        if kind in {"configure", "build"}:
            self.config.admit_project()
        record = self.storage.new_run(kind, title, capture_mode)
        self.events.emit("run_started", run_id=record.run_id, title=title)
        static = scan_sources(self.config) if capture_mode.lower() in {"full", "deep"} else None
        if static:
            capture_scan(record.run_dir, static)
            self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", static)
        # BuildController seals its receipt. Retain source observations before
        # invoking it, rather than appending to an already immutable capture.
        result = operation(record.run_dir, capture_mode, self._log)
        samples = summarize_process_samples(record.run_dir)
        status = "PASS" if result.exit_code == 0 and not result.cancelled else ("CANCELLED" if result.cancelled else "FAIL")
        summary = {"process": result.to_dict(), "samples": samples, "static_scan": static or {}}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, status, summary)
        self._export_run(record.run_dir, record.to_dict(), summary, static_scan=static)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _run_tests(self, tests: list[TestCase], capture_mode: str, title: str) -> None:
        if not tests:
            raise RuntimeError("No tests selected. Discover tests first or select at least one test.")
        record = self.storage.new_run("tests", title, capture_mode)
        self.events.emit("run_started", run_id=record.run_id, title=title)
        results: list[dict] = []
        for index, test in enumerate(tests, start=1):
            if self._cancel_requested.is_set():
                break
            self.events.emit("test_progress", current=index, total=len(tests), name=test.name)
            if self._cancel_requested.is_set():
                break
            self._log("info", f"[{index}/{len(tests)}] {test.name}")
            result = self.tests.run_one(test, record.run_dir, capture_mode, self._log)
            data = result.to_dict()
            results.append(data)
            self.storage.append_jsonl(record.run_dir / "tests" / "results.jsonl", data)
        test_summary = aggregate_test_results(results)
        static = scan_sources(self.config) if capture_mode.lower() in {"full", "deep"} else None
        if static:
            capture_scan(record.run_dir, static)
            self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", static)
        status, batch = capture_batch(record, tests, results,
                                     self._cancel_reason if self._cancel_requested.is_set() else None)
        summary = {"tests": test_summary, "batch": batch, "static_scan": static or {}}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, status, summary)
        self._export_run(record.run_dir, record.to_dict(), summary, tests=results, static_scan=static)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _start_game(self, capture_mode: str) -> None:
        if not self.config.run_command:
            raise RuntimeError("run.command is empty in auditor.config.json")
        scenario = self.config.run_scenario
        record = self.storage.new_run("game", "Game run [" + scenario['kind'] + ": " + scenario['id'] + "]", capture_mode)
        self.storage.write_json(record.run_dir / 'scenario.json', scenario)
        self.events.emit("run_started", run_id=record.run_id, title="Game run")
        command = [self._resolve_command_part(x) for x in self.config.run_command]
        try:
            result = self.runner.run(
                command,
                self.config.run_working_dir,
                record.run_dir,
                capture_mode,
                log_callback=self._log,
                env_extra=self.config.run_environment,
            )
        except ProcessLaunchError:
            seal_run_capture(record)
            error = json.loads(RawEvidence(record.run_dir).read_bytes("process/launch_failure.json"))
            lineage = EvidenceLineage(record.run_dir)
            reference = lineage.reference("raw", "process/launch_failure.json")
            row = normalized_record({**error, "required": True}, {"process.launch.failure"}, reference, 0)
            lineage.write("normalized", "launch_failure.json", producer_version="process-admission-1",
                          producer_source=Path(__file__), inputs=[reference], records=[row])
            summary = {"launch_failure": error}
            self.storage.finish_run(record, "FAIL", summary)
            self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)
            raise
        static = scan_sources(self.config) if capture_mode.lower() in {"full", "deep"} else None
        if static:
            capture_scan(record.run_dir, static)
            self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", static)
        samples = summarize_process_samples(record.run_dir)
        status = "PASS" if result.exit_code == 0 and not result.cancelled else ("CANCELLED" if result.cancelled else "FAIL")
        summary = {"process": result.to_dict(), "samples": samples, "static_scan": static or {}, "scenario": scenario}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, status, summary)
        self._export_run(record.run_dir, record.to_dict(), summary, static_scan=static)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _build_and_run(self, capture_mode: str) -> None:
        build_record = self.storage.new_run("build", "Build before game", capture_mode)
        self.events.emit("run_started", run_id=build_record.run_id, title="Build before game")
        result = self.builder.build(build_record.run_dir, capture_mode, self._log)
        build_status = "CANCELLED" if result.cancelled else "PASS" if result.exit_code == 0 else "FAIL"
        summary = {"process": result.to_dict(), "samples": summarize_process_samples(build_record.run_dir)}
        self.storage.write_json(build_record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(build_record, build_status, summary)
        self._export_run(build_record.run_dir, build_record.to_dict(), summary)
        self.events.emit("run_finished", run_id=build_record.run_id, status=build_record.status, summary=summary)
        if build_record.status != "PASS":
            return
        self._start_game(capture_mode)

    def _static_scan_only(self) -> None:
        record = self.storage.new_run("static", "Static source scan", "Full")
        self.events.emit("run_started", run_id=record.run_id, title="Static source scan")
        result = scan_sources(self.config)
        capture_scan(record.run_dir, result)
        self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", result)
        summary = {"static_scan": result}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, "PASS", summary)
        self._export_run(record.run_dir, record.to_dict(), summary, static_scan=result)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _export_run(self, *args, **kwargs):
        # Sealing, verification and final status precede this optional projection.
        enabled = self.config.data.get('storage', {}).get('auto_export_agent_pack', True)
        if type(enabled) is not bool:
            raise ValueError('auto_export_agent_pack must be boolean')
        if enabled:
            return export_agent_pack(*args, **kwargs)

    def _resolve_command_part(self, value: str) -> str:
        path = Path(value)
        if path.is_absolute():
            return str(path)
        candidate = self.config.resolve(path)
        return str(candidate) if candidate.exists() else value


def validate_stress(spec):
    """Admit only finite, explicit, owned operations; never infer a Game target."""
    from copy import deepcopy
    result = deepcopy(spec)
    command = result.get('command')
    if not isinstance(command, list) or not command or any(not isinstance(value, str) or not value or '\x00' in value for value in command):
        raise ValueError('stress requires an explicit executable/arguments')
    working = Path(result.get('working_dir', '')).resolve()
    if not result.get('working_dir') or not working.is_dir():
        raise ValueError('stress requires an existing working directory')
    result['working_dir'] = str(working)
    storage_budget = result.setdefault('storage_budget_bytes', 512 * 1024**2)
    if type(storage_budget) is not int or not 64 * 1024**2 <= storage_budget <= 4 * 1024**3:
        raise ValueError('campaign storage budget must be 64 MiB–4 GiB')
    for key, default, maximum in [('iterations', 1, 1000), ('timeout_seconds', 30, 1800), ('budget_seconds', 300, 3600)]:
        value = result.setdefault(key, default)
        if type(value) is not int or not 1 <= value <= maximum:
            raise ValueError('invalid finite stress ' + key)
    injectors = result.get('injectors')
    if not isinstance(injectors, list) or not 1 <= len(injectors) <= 5:
        raise ValueError('stress requires 1–5 explicit injectors')
    for injector in injectors:
        if not isinstance(injector, dict) or injector.get('type') not in {
            'cpu_pressure', 'memory_pressure', 'worker_pressure', 'allocation_failure', 'io_delay'}:
            raise ValueError('unsupported injector')
        kind = injector['type']
        parameter, maximum = {'cpu_pressure': ('workers', 2), 'worker_pressure': ('workers', 2),
                              'memory_pressure': ('bytes', 512 * 1024**2),
                              'allocation_failure': ('nth', 1000000), 'io_delay': ('delay_ms', 5000)}[kind]
        value = injector.get(parameter)
        if type(value) is not int or not 1 <= value <= maximum:
            raise ValueError('invalid injector ' + parameter)
        if kind == 'allocation_failure' and injector.get('scope') != 'DefaultAllocator.Allocate':
            raise ValueError('allocation injection requires the supported explicit allocator scope')
        if kind == 'io_delay':
            scope = Path(injector.get('scope', '')).resolve()
            if injector.get('scope') != 'capture' and (not injector.get('scope') or not scope.is_relative_to(working)):
                raise ValueError('IO delay scope must be inside the admitted target working directory')
            if injector['scope'] != 'capture':
                injector['scope'] = str(scope)
        for key in ('baseline_exit_code', 'enabled_exit_code'):
            value = injector.setdefault(key, 0)
            if type(value) is not int or not 0 <= value <= 0xFFFFFFFF:
                raise ValueError('invalid expected physical exit code')
    return result


def evaluate_stress_controls(injector, controls):
    """A configured injector/ACK alone cannot certify an observed effect."""
    if len(controls) != 3 or [row.get('phase') for row in controls] != ['disabled', 'enabled', 'outside_scope']:
        return {'status': 'FAIL', 'reason': 'required controls absent', 'controls': controls}
    baseline, enabled, outside = controls
    result = {'injector_id': enabled['injector_id'], 'type': injector['type'], 'parameters': injector,
              'target_scope': enabled['command'], 'controls': controls,
              'requested_interval': enabled['requested_interval'], 'observed_interval': enabled['observed_interval'],
              'cleanup': enabled['cleanup'], 'provenance': 'external', 'confidence': 'derived'}
    if any(row['cleanup'] != 'observed_stopped' or row.get('resource_failure') or row.get('error') for row in controls):
        return dict(result, status='FAIL', reason='cleanup/resource guard failure')
    if any(row['capture_status'] == 'INCOMPLETE' for row in controls):
        return dict(result, status='UNKNOWN', reason='incomplete mandatory capture')
    if injector['type'] != 'allocation_failure' and any(
            row['capture_status'] != ('PASS' if row['target_exit_code'] == 0 else 'FAIL') for row in controls):
        return dict(result, status='FAIL', reason='owned driver/control did not complete successfully')
    if any(row['target_exit_code'] != (injector['enabled_exit_code'] if index == 1 else injector['baseline_exit_code'])
           for index, row in enumerate(controls)):
        return dict(result, status='FAIL', reason='physical target outcome differs from requested control')
    kind = injector['type']
    if kind in ('cpu_pressure', 'worker_pressure'):
        effect = sum(row.get('cpu_seconds_during_target', 0) for row in enabled['helpers'])
        observed = len(enabled['helpers']) == injector['workers'] and effect > .005
        isolated = not baseline['helpers'] and not outside['helpers']
        metric = {'cpu_seconds': effect, 'method': 'OS process CPU delta during target interval of owned helper identities'}
    elif kind == 'memory_pressure':
        effect = sum(row.get('minimum_rss_during_target', 0) for row in enabled['helpers'])
        observed, isolated = effect >= injector['bytes'], not baseline['helpers'] and not outside['helpers']
        metric = {'rss_bytes': effect, 'requested_bytes': injector['bytes'], 'method': 'OS working set of owned helper identity'}
    elif kind == 'io_delay':
        effect = enabled['target_wall_seconds'] - baseline['target_wall_seconds']
        observed = effect >= injector['delay_ms'] / 1000 * .75
        isolated = abs(outside['target_wall_seconds'] - baseline['target_wall_seconds']) < max(.25, injector['delay_ms'] / 1000 * .5)
        metric = {'completion_latency_delta_seconds': effect, 'media_irp_latency': 'UNKNOWN',
                  'method': 'external process monotonic completion, matched disabled/outside-scope controls'}
    else:
        effect = enabled['target_exit_code'] != baseline['target_exit_code']
        observed, isolated = effect, outside['target_exit_code'] == baseline['target_exit_code']
        metric = {'target_outcome_changed': effect, 'allocation_failure_proof': 'requires external exception capture',
                  'method': 'external target exit plus controlled adapter input; not standalone allocation evidence'}
        corroborated = (enabled.get('exception_capture_complete') is True and enabled.get('external_cpp_exceptions', 0) == 1
                        and baseline.get('external_cpp_exceptions') == 0 and outside.get('external_cpp_exceptions') == 0)
        metric['external_cpp_exceptions'] = enabled.get('external_cpp_exceptions')
        return dict(result, status='PASS' if observed and isolated and corroborated else 'UNKNOWN', measured_effect=metric,
                    reason=None if observed and isolated and corroborated else 'independent exception/effect correlation absent')
    return dict(result, status='PASS' if observed and isolated else 'UNKNOWN', measured_effect=metric,
                reason=None if observed and isolated else 'effect/scope control not observed')


def stress_child(request_path):
    """Owned driver; observation is external to the target, never a target ACK."""
    import os
    import shutil
    import subprocess
    import sys
    import time
    import psutil
    from ep_auditor.core.evidence import atomic_json, digest
    request = json.loads(Path(request_path).read_text(encoding='utf-8-sig'))
    injector, phase = request['injector'], request['phase']
    enabled = phase == 'enabled'
    environment = os.environ.copy()
    for key in ('EP_AUDITOR_ALLOC_SCOPE', 'EP_AUDITOR_ALLOC_FAIL_N', 'EP_AUDITOR_ALLOC_MODULE', 'EP_AUDITOR_IO_SCOPE', 'EP_AUDITOR_IO_DELAY_MS'):
        environment.pop(key, None)
    if injector['type'] == 'allocation_failure' and phase != 'disabled':
        environment.update(EP_AUDITOR_ALLOC_SCOPE=injector['scope'] if enabled else 'outside.allocator',
                           EP_AUDITOR_ALLOC_FAIL_N=str(injector['nth']))
    if injector['type'] == 'io_delay' and phase != 'disabled':
        scope = environment.get('EP_AUDITOR_CAUSAL_DIRECTORY') if injector['scope'] == 'capture' else injector['scope']
        if not scope:
            raise ValueError('owned capture scope absent')
        environment.update(EP_AUDITOR_IO_SCOPE=scope if enabled else str(Path(scope) / 'outside-injection-scope'),
                           EP_AUDITOR_IO_DELAY_MS=str(injector['delay_ms']))
    start = time.perf_counter_ns()
    deadline = time.monotonic() + request['timeout_seconds']
    helpers, observations, target = [], [], None
    target_started, target_finished, target_exit = None, None, None
    resource_failure, error = None, None
    try:
        executable = shutil.which(request['command'][0])
        if not executable:
            raise FileNotFoundError('stress target executable absent')
        command = [str(Path(executable).resolve()), *request['command'][1:]]
        image_sha = digest(Path(command[0]))
        if injector['type'] == 'allocation_failure' and phase != 'disabled':
            environment['EP_AUDITOR_ALLOC_MODULE'] = command[0]
        if enabled and injector['type'] in ('cpu_pressure', 'worker_pressure', 'memory_pressure'):
            if min(available_stress_memory()) < 4 * 1024**3 + injector.get('bytes', 0):
                raise MemoryError('4 GiB physical/commit reserve would be violated')
            if injector['type'] == 'memory_pressure':
                code = 'import time; block=bytearray(' + str(injector['bytes']) + '); time.sleep(1800)'
                count = 1
            else:
                code, count = 'while True: pass', injector['workers']
            for _ in range(count):
                # A Windows venv python.exe is a redirector: its OS CPU/RSS
                # belong to the waiting launcher, not the pressure interpreter.
                # Launch the real interpreter for these stdlib-only helpers so
                # measurement and cleanup address the actual owned workload.
                helper = subprocess.Popen([getattr(sys, '_base_executable', sys.executable), '-B', '-c', code],
                                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                helpers.append(helper)
                process = psutil.Process(helper.pid)
                observations.append({'pid': helper.pid, 'creation_time': process.create_time(),
                                     'cpu_seconds': 0, 'rss_bytes': 0, 'image_path': process.exe()})
            # Python/helper startup is not a fixed-duration operation. Wait for
            # real OS measurements, with a finite deadline, before the target.
            warm_deadline = min(deadline, time.monotonic() + 5)
            while time.monotonic() < warm_deadline:
                ready = True
                for helper, observed in zip(helpers, observations):
                    process = psutil.Process(helper.pid)
                    cpu = process.cpu_times()
                    observed['cpu_seconds'] = cpu.user + cpu.system
                    observed['rss_bytes'] = process.memory_info().rss
                    ready = ready and (observed['rss_bytes'] >= injector['bytes'] + 12 * 1024**2
                        if injector['type'] == 'memory_pressure' else observed['cpu_seconds'] >= .1)
                if ready:
                    break
                if min(available_stress_memory()) < 4 * 1024**3:
                    raise MemoryError('physical/commit reserve below 4 GiB during pressure admission')
                time.sleep(.02)
        for helper, observed in zip(helpers, observations):
            process = psutil.Process(helper.pid)
            cpu = process.cpu_times()
            observed.update(cpu_seconds_at_target_start=cpu.user + cpu.system,
                            minimum_rss_during_target=process.memory_info().rss,
                            threads=process.num_threads())
        target_started = time.perf_counter_ns()
        target = subprocess.Popen(command, cwd=request['working_dir'], env=environment)
        target_identity = {'pid': target.pid, 'creation_time': psutil.Process(target.pid).create_time(),
                           'image_sha256': image_sha, 'image_path': command[0]}
        while True:
            for helper, observed in zip(helpers, observations):
                if helper.poll() is not None:
                    raise RuntimeError('owned pressure helper exited during target interval')
                process = psutil.Process(helper.pid)
                if process.create_time() != observed['creation_time']:
                    raise RuntimeError('owned helper identity changed')
                cpu = process.cpu_times()
                observed['cpu_seconds'] = max(observed['cpu_seconds'], cpu.user + cpu.system)
                observed['cpu_seconds_during_target'] = cpu.user + cpu.system - observed['cpu_seconds_at_target_start']
                rss = process.memory_info().rss
                observed['rss_bytes'] = max(observed['rss_bytes'], rss)
                observed['minimum_rss_during_target'] = min(observed['minimum_rss_during_target'], rss)
            target_exit = target.poll()
            if target_exit is not None:
                break
            if time.monotonic() >= deadline:
                error = 'target timeout'
                target.kill()
                target_exit = target.wait(timeout=5)
                break
            if min(available_stress_memory()) < 4 * 1024**3:
                resource_failure = 'physical/commit reserve below 4 GiB'
                target.kill()
                target_exit = target.wait(timeout=5)
                break
            time.sleep(.02)
        target_finished = time.perf_counter_ns()
    except Exception as exception:
        error = str(exception)
    finally:
        cleanup = 'observed_stopped'
        for process in ([target] if target is not None else []) + helpers:
            try:
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=5)
            except Exception:
                cleanup = 'INCOMPLETE'
        end = time.perf_counter_ns()
        atomic_json(Path(request['observation_path']), {
            'schemaVersion': 1, 'injector_id': request['injector_id'], 'type': injector['type'], 'phase': phase,
            'parameters': injector, 'command': request['command'],
            'target_identity': locals().get('target_identity'), 'target_exit_code': target_exit,
            'target_wall_seconds': (target_finished - target_started) / 1e9 if target_started and target_finished else None,
            'requested_interval': {'timeout_seconds': request['timeout_seconds']},
            'observed_interval': {'start_perf_ns': start, 'end_perf_ns': end},
            'target_interval': {'start_perf_ns': target_started, 'end_perf_ns': target_finished},
            'helpers': observations, 'cleanup': cleanup, 'resource_failure': resource_failure, 'error': error,
            'provenance': 'external', 'confidence': 'observed'})
    return 124 if error or resource_failure or cleanup != 'observed_stopped' else target_exit


def available_stress_memory():
    import ctypes
    import psutil
    class Memory(ctypes.Structure):
        _fields_ = [('length', ctypes.c_uint32), ('load', ctypes.c_uint32)] + [
            (name, ctypes.c_uint64) for name in ('total_physical', 'available_physical', 'total_commit',
             'available_commit', 'total_virtual', 'available_virtual', 'available_extended')]
    memory = Memory()
    memory.length = ctypes.sizeof(memory)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.GlobalMemoryStatusEx.argtypes = [ctypes.POINTER(Memory)]
    kernel.GlobalMemoryStatusEx.restype = ctypes.c_int
    if not kernel.GlobalMemoryStatusEx(ctypes.byref(memory)):
        raise ctypes.WinError(ctypes.get_last_error())
    return psutil.virtual_memory().available, memory.available_commit
