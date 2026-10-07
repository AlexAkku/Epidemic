from __future__ import annotations

import json
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Iterable

from ep_auditor.analyzers.static_scan import scan_sources
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
        result = operation(record.run_dir, capture_mode, self._log)
        static = scan_sources(self.config) if capture_mode.lower() in {"full", "deep"} else None
        if static:
            self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", static)
        samples = summarize_process_samples(record.run_dir)
        status = "PASS" if result.exit_code == 0 and not result.cancelled else ("CANCELLED" if result.cancelled else "FAIL")
        summary = {"process": result.to_dict(), "samples": samples, "static_scan": static or {}}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, status, summary)
        export_agent_pack(record.run_dir, record.to_dict(), summary, static_scan=static)
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
            self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", static)
        status, batch = capture_batch(record, tests, results,
                                     self._cancel_reason if self._cancel_requested.is_set() else None)
        summary = {"tests": test_summary, "batch": batch, "static_scan": static or {}}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, status, summary)
        export_agent_pack(record.run_dir, record.to_dict(), summary, tests=results, static_scan=static)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _start_game(self, capture_mode: str) -> None:
        if not self.config.run_command:
            raise RuntimeError("run.command is empty in auditor.config.json")
        record = self.storage.new_run("game", "Game run", capture_mode)
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
            self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", static)
        samples = summarize_process_samples(record.run_dir)
        status = "PASS" if result.exit_code == 0 and not result.cancelled else ("CANCELLED" if result.cancelled else "FAIL")
        summary = {"process": result.to_dict(), "samples": samples, "static_scan": static or {}}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, status, summary)
        export_agent_pack(record.run_dir, record.to_dict(), summary, static_scan=static)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _build_and_run(self, capture_mode: str) -> None:
        build_record = self.storage.new_run("build", "Build before game", capture_mode)
        self.events.emit("run_started", run_id=build_record.run_id, title="Build before game")
        result = self.builder.build(build_record.run_dir, capture_mode, self._log)
        build_status = "CANCELLED" if result.cancelled else "PASS" if result.exit_code == 0 else "FAIL"
        summary = {"process": result.to_dict(), "samples": summarize_process_samples(build_record.run_dir)}
        self.storage.write_json(build_record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(build_record, build_status, summary)
        export_agent_pack(build_record.run_dir, build_record.to_dict(), summary)
        self.events.emit("run_finished", run_id=build_record.run_id, status=build_record.status, summary=summary)
        if build_record.status != "PASS":
            return
        self._start_game(capture_mode)

    def _static_scan_only(self) -> None:
        record = self.storage.new_run("static", "Static source scan", "Full")
        self.events.emit("run_started", run_id=record.run_id, title="Static source scan")
        result = scan_sources(self.config)
        self.storage.write_json(record.run_dir / "metrics" / "static_scan.json", result)
        summary = {"static_scan": result}
        self.storage.write_json(record.run_dir / "metrics" / "summary.json", summary)
        self.storage.finish_run(record, "PASS", summary)
        export_agent_pack(record.run_dir, record.to_dict(), summary, static_scan=result)
        self.events.emit("run_finished", run_id=record.run_id, status=record.status, summary=summary)

    def _resolve_command_part(self, value: str) -> str:
        path = Path(value)
        if path.is_absolute():
            return str(path)
        candidate = self.config.resolve(path)
        return str(candidate) if candidate.exists() else value
