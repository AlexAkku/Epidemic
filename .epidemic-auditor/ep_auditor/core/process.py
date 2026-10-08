from __future__ import annotations

import os
import codecs
import hashlib
import json
import queue
import subprocess
import threading
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable

from ep_auditor.collectors.process_stats import ProcessStatsCollector
from ep_auditor.collectors.wpr import WprCollector
from ep_auditor.collectors.xperf import XperfPostProcessor
from ep_auditor.config import AuditorConfig
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.owned_process import OwnedProcess
from ep_auditor.models import ProcessResult


class ProcessLaunchError(OSError):
    """Observed owned-process admission failure, with retained original API error."""


LogCallback = Callable[[str, str], None]


class ManagedProcessRunner:
    def __init__(self, config: AuditorConfig) -> None:
        self.config = config
        self._active_lock = threading.RLock()
        self._run_lock = threading.Lock()
        self._lifetime_scope = None
        self._capture_workers = []
        self._active: OwnedProcess | None = None
        self.operation_cancel = None
        self._pending_stop_reason = None

    def cancel(self, reason="user_stop") -> None:
        if reason in {"user_stop", "application_shutdown"}:
            self._pending_stop_reason = reason
        with self._active_lock:
            proc = self._active
            if proc:
                proc.cancel(reason)

    def run(
        self, command: list[str], cwd: Path, run_dir: Path, capture_mode: str,
        log_callback: LogCallback | None = None, env_extra: dict[str, str] | None = None,
        timeout: float | None = None,
    ) -> ProcessResult:
        with self._run_lock:
            if self.operation_cancel is None:
                self._pending_stop_reason = None
            self._lifetime_scope = None
            self._capture_workers = []
            try:
                return self._run(command, cwd, run_dir, capture_mode, log_callback, env_extra, timeout)
            except BaseException:
                with self._active_lock:
                    if self._lifetime_scope:
                        self._lifetime_scope.cancel("observer_operation_failure")
                        if self._lifetime_scope.termination_observation:
                            RunStorage.write_json(run_dir / "termination.json", self._lifetime_scope.termination_observation)
                raise
            finally:
                with self._active_lock:
                    self._active = None
                    if self._lifetime_scope:
                        self._lifetime_scope.poll()
                        self._lifetime_scope.close()
                        self._lifetime_scope = None
                for worker in self._capture_workers:
                    if worker.ident is not None:
                        worker.join()
                self._capture_workers = []

    def _run(
        self,
        command: list[str],
        cwd: Path,
        run_dir: Path,
        capture_mode: str,
        log_callback: LogCallback | None = None,
        env_extra: dict[str, str] | None = None,
        timeout: float | None = None,
    ) -> ProcessResult:
        started_wall = datetime.now(timezone.utc)
        started_perf = time.perf_counter()
        capture_start_ns = time.perf_counter_ns()
        with self._active_lock:
            reason = self._pending_stop_reason
            if reason is None and self.operation_cancel is not None and self.operation_cancel.is_set():
                reason = "user_stop"
        if reason is not None:
            # No target existed: record admission cancellation, rather than
            # starting a suspended process and reporting missing resource samples.
            payload = json.dumps({"schemaVersion": 1, "eventType": "process.admission.cancelled",
                                  "command": command, "cwd": str(cwd), "pid": None,
                                  "reason": reason, "provenance": "external",
                                  "confidence": "observed"}, sort_keys=True).encode()
            capture = RawEvidence(run_dir)
            capture.append("process/admission_cancelled.json", payload,
                collector="owned_process_admission", version="1", start_ns=capture_start_ns,
                end_ns=time.perf_counter_ns(), health={"capacity": len(payload), "observed": len(payload),
                                                     "dropped": 0, "incomplete_reason": None})
            # Preserve the consumer stream contract. These are empty because
            # admission was cancelled, not because a running target was unobserved.
            for channel in ("stdout", "stderr"):
                capture.append("process/" + channel + ".bin", b"", collector="external_stdio." + channel,
                    version="1", start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                    health={"capacity": 1, "observed": 0, "dropped": 0, "incomplete_reason": None})
            reads = json.dumps({"schemaVersion": 1, "clock_domain": "monotonic_trace",
                                "capacity_unit": "read_events", "capacity": 1,
                                "observed": 0, "dropped": 0, "events": [],
                                "availability": "not_applicable", "reason": "admission_cancelled"}).encode()
            capture.append("process/stdio_reads.json", reads, collector="external_stdio_read_observation",
                version="1", start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                health={"capacity": 1, "observed": 0, "dropped": 0, "incomplete_reason": None})
            return ProcessResult(command=command, cwd=str(cwd), exit_code=None,
                started_utc=started_wall.isoformat(), finished_utc=datetime.now(timezone.utc).isoformat(),
                wall_seconds=time.perf_counter() - started_perf, cancelled=True,
                notes=["cancelled_before_process_creation: " + reason])
        capacity = self.config.data.get("capture", {}).get("stdio_raw_capacity_bytes", 16 * 1024 * 1024)
        if type(capacity) is not int or not 0 < capacity <= 64 * 1024 * 1024:
            raise EvidenceError("invalid bounded stdio raw capacity")
        env = os.environ.copy()
        env.update(env_extra or {})
        if capture_mode.lower() in {'fast', 'full', 'deep', 'memory', 'stress', 'targeted exact'}:
            exact_directory = (run_dir / 'exact-capture').resolve()
            exact_directory.mkdir(parents=True, exist_ok=True)
            env['EP_AUDITOR_TRACE_DIRECTORY'] = str(exact_directory)
            env.setdefault('EP_AUDITOR_TRACE_CAPACITY', '262144')
        if capture_mode.lower() == 'coverage':
            coverage_directory = (run_dir / 'coverage-capture').resolve()
            coverage_directory.mkdir(parents=True, exist_ok=True)
            env['LLVM_PROFILE_FILE'] = str(coverage_directory / 'profile-%p-%m.profraw')
        console_path = run_dir / "console.jsonl"
        raw_stdout = run_dir / "stdout.log"
        raw_stderr = run_dir / "stderr.log"
        cancelled = False
        notes: list[str] = []

        wpr = WprCollector(self.config.wpr_profile)
        if capture_mode.lower() == "deep" and self.config.enable_wpr_in_deep:
            wpr.start(run_dir)
            if wpr.note:
                notes.append(wpr.note)

        creationflags = 0
        if os.name == "nt":
            creationflags |= getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)

        try:
            def assigned(process):
                with self._active_lock:
                    self._active = self._lifetime_scope = process
                    reason = self._pending_stop_reason
                    if reason is not None or self.operation_cancel is not None and self.operation_cancel.is_set():
                        process.cancel(reason or "user_stop")
            proc = OwnedProcess(command, str(cwd), env, creationflags, on_assigned=assigned)
        except OSError as error:
            payload = json.dumps({"schemaVersion": 1, "eventType": "process.launch.failure",
                "command": command, "cwd": str(cwd), "pid": None, "exit_code": None,
                "errno": error.errno, "winerror": getattr(error, "winerror", None), "message": str(error),
                "api_phase": "owned_process_admission", "provenance": "external", "confidence": "observed"},
                sort_keys=True).encode()
            RawEvidence(run_dir).append("process/launch_failure.json", payload,
                collector="owned_process_admission_error", version="1", start_ns=capture_start_ns,
                end_ns=time.perf_counter_ns(), health={"capacity": len(payload), "observed": len(payload),
                                                     "dropped": 0, "incomplete_reason": None})
            wpr.stop()
            raise ProcessLaunchError(error.errno, str(error), command[0]) from error
        except Exception:
            wpr.stop()
            raise
        stats = ProcessStatsCollector(self.config.sample_interval_seconds)
        stats.start(proc.pid, run_dir)

        out_queue: queue.Queue[tuple[str, str, float, str]] = queue.Queue(maxsize=1024)
        line_counts = {"stdout": 0, "stderr": 0}
        capture = RawEvidence(run_dir)
        capture_lock = threading.Lock()
        stream_states = {}
        read_observations = {"events": [], "observed": 0}
        read_event_capacity = 4096

        def pump(stream, channel: str, raw_path: Path) -> None:
            if stream is None:
                return
            state = {"observed": 0, "stored": 0, "newlines": 0, "console_dropped": 0, "error": None}
            stream_states[channel] = state
            decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
            pending = ""
            last = b""

            def append(data, reason, read_size=0, final=False):
                with capture_lock:
                    if read_size or final:
                        sequence = read_observations["observed"]
                        read_observations["observed"] += 1
                        if len(read_observations["events"]) < read_event_capacity:
                            read_observations["events"].append({"sequence": sequence, "channel": channel,
                                "kind": "read" if read_size else "read_failure" if state["error"] else "eof",
                                "start_offset": state["observed"] - read_size, "end_offset": state["observed"],
                                "observed_perf_ns": time.perf_counter_ns(),
                                "job_membership": proc.observe_members()})
                    capture.append("process/" + channel + ".bin", data, collector="external_stdio." + channel,
                                   version="1", start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                                   health={"capacity": capacity, "observed": state["observed"],
                                           "dropped": state["observed"] - state["stored"], "incomplete_reason": reason})

            def display(text):
                nonlocal pending
                pending += text
                while "\n" in pending or len(pending) >= 4096:
                    newline = pending.find("\n")
                    cut = newline + 1 if 0 <= newline < 4096 else 4096
                    line, pending = pending[:cut].rstrip("\r\n"), pending[cut:]
                    try:
                        out_queue.put_nowait((channel, line, time.perf_counter() - started_perf, datetime.now(timezone.utc).isoformat()))
                    except queue.Full:
                        state["console_dropped"] += 1

            try:
                append(b"", "capture_in_progress")
                with raw_path.open("wb") as raw:
                    while chunk := stream.read1(65536):
                        kept = chunk[:max(0, capacity - state["stored"])]
                        state["observed"] += len(chunk)
                        state["stored"] += len(kept)
                        state["newlines"] += chunk.count(b"\n")
                        last = chunk[-1:]
                        raw.write(kept)
                        raw.flush()
                        append(kept, "capture_in_progress", read_size=len(chunk))
                        display(decoder.decode(chunk))
                    display(decoder.decode(b"", final=True))
                    if pending:
                        display("\n")
            except Exception as error:
                state["error"] = str(error)
            finally:
                line_counts[channel] = state["newlines"] + int(bool(last) and last != b"\n")
                reason = ("stdio_read_failed_unobserved_tail" if state["error"] else
                          "raw_byte_budget_exceeded" if state["observed"] != state["stored"] else None)
                try:
                    append(b"", reason, final=True)
                except Exception as error:
                    state["error"] = str(error)
                stream.close()

        threads = [
            threading.Thread(target=pump, args=(proc.stdout, "stdout", raw_stdout), daemon=True),
            threading.Thread(target=pump, args=(proc.stderr, "stderr", raw_stderr), daemon=True),
        ]
        self._capture_workers = threads
        for t in threads:
            t.start()

        deadline = started_perf + timeout if timeout and timeout > 0 else None
        process_finished_perf = started_perf
        process_finished_wall = started_wall
        try:
            while proc.poll() is None or any(t.is_alive() for t in threads) or not out_queue.empty():
                if deadline is not None and time.perf_counter() >= deadline and proc.active_process_count():
                    notes.append(f"timeout after {timeout:.3f}s")
                    self.cancel("timeout")
                    cancelled = True
                try:
                    channel, line, offset, utc = out_queue.get(timeout=0.05)
                    RunStorage.append_jsonl(console_path, {
                        "observed_utc": utc,
                        "monotonic_seconds": offset,
                        "channel": channel,
                        "text": line,
                        "provenance": "external",
                        "confidence": "derived",
                        "raw_source": "raw/process/" + channel + ".bin",
                        "text_decoding": "utf-8 with replacement; raw retains original bytes",
                    })
                    if log_callback:
                        log_callback(channel, line)
                except queue.Empty:
                    pass
            for t in threads:
                t.join(timeout=1.0)
            # A child may deliberately close inherited stdio and outlive the root.
            # End that owned scope explicitly before recording its lifecycle.
            if proc.active_process_count():
                proc.cancel("root_exit_live_descendants")
            process_finished_perf = time.perf_counter()
            process_finished_wall = datetime.now(timezone.utc)
        finally:
            stats.stop()
            etl = wpr.stop()
            if wpr.note and wpr.note not in notes:
                notes.append(wpr.note)
            if etl:
                wpr_meta = {
                    "trace": str(etl),
                    "profile": self.config.wpr_profile,
                    "provenance": "external_wpr_raw",
                }
                xperf = XperfPostProcessor()
                executable = Path(command[0])
                if not executable.is_absolute():
                    executable = cwd / executable
                xperf_meta = xperf.process(etl, proc.pid, run_dir / "xperf" / "reports",
                                          symbol_dirs=(executable.parent, cwd))
                for name, report in list(xperf_meta["reports"].items()):
                    source = Path(report)
                    size = source.stat().st_size
                    capture.import_file("xperf/" + source.name, source, digest(source),
                        collector="external_xperf_analysis", version="1",
                        start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                        health={"capacity": 2 * 1024**3, "observed": size, "dropped": 0,
                                "incomplete_reason": None if size else "empty_xperf_report"})
                    xperf_meta["reports"][name] = str(capture.raw / "xperf" / source.name)
                wpr_meta["xperf"] = xperf_meta
                payload = json.dumps(wpr_meta, sort_keys=True).encode()
                capture.append("wpr.json", payload, collector="external_wpr_analysis", version="1",
                    start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                    health={"capacity": len(payload), "observed": len(payload), "dropped": 0,
                            "incomplete_reason": "xperf_reports_incomplete" if xperf_meta["errors"] else None})
            with self._active_lock:
                self._active = None

        finished_wall = process_finished_wall
        wall_seconds = process_finished_perf - started_perf
        exit_code = proc.poll()
        cancelled = cancelled or proc.cancel_reason not in {None, "root_exit_live_descendants"}
        if proc.termination_observation:
            RunStorage.write_json(run_dir / "termination.json", proc.termination_observation)
        summary = stats.summary
        for name in ("process_samples.jsonl", "children.jsonl"):
            path = run_dir / name
            state = stats.capture_state.get(name, {"observed": 0, "stored": 0, "dropped": 0})
            payload = path.read_bytes()
            capture.append("process/" + name, payload, collector="windows_resource_sampling", version="2",
                start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                health={"capacity": stats.capacity_bytes, "observed": state["observed"], "dropped": state["dropped"],
                        "incomplete_reason": "resource_sampling_capacity_exceeded" if state["dropped"] else
                                             summary.get("reason")})
        read_loss = read_observations["observed"] - len(read_observations["events"])
        read_payload = json.dumps({"schemaVersion": 1, "clock_domain": "monotonic_trace", "capacity_unit": "read_events",
                                   "capacity": read_event_capacity, "observed": read_observations["observed"],
                                   "dropped": read_loss, "events": read_observations["events"]}, sort_keys=True).encode()
        capture.append("process/stdio_reads.json", read_payload, collector="external_stdio_read_observation", version="1",
                       start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                       health={"capacity": read_event_capacity, "observed": read_observations["observed"], "dropped": read_loss,
                               "incomplete_reason": "stdio_read_observation_capacity_exceeded" if read_loss else None})
        for channel, state in stream_states.items():
            if state["error"]:
                notes.append(channel + " capture failed: " + state["error"])
            if state["observed"] != state["stored"]:
                notes.append(channel + " raw dropped bytes: " + str(state["observed"] - state["stored"]))
            if state["console_dropped"]:
                notes.append(channel + " derived console dropped records: " + str(state["console_dropped"]))
        result = ProcessResult(
            command=command,
            cwd=str(cwd),
            exit_code=exit_code,
            started_utc=started_wall.isoformat(),
            finished_utc=finished_wall.isoformat(),
            wall_seconds=wall_seconds,
            cancelled=cancelled,
            pid=proc.pid,
            stdout_lines=line_counts["stdout"],
            stderr_lines=line_counts["stderr"],
            sample_count=int(summary["sample_count"]),
            peak_rss_bytes=int(summary["peak_rss_bytes"]),
            peak_processes=int(summary["peak_processes"]),
            peak_threads=int(summary["peak_threads"]),
            total_read_bytes=int(summary["total_read_bytes"]),
            total_write_bytes=int(summary["total_write_bytes"]),
            notes=notes,
        )
        lifecycle = {"schemaVersion": 1, "eventType": "process.lifecycle", "pid": proc.pid,
                     "creator_pid": os.getpid(), "main_thread_id": proc.thread_id,
                     "creation_filetime_100ns": proc.creation_filetime_100ns,
                     "creation_clock_domain": "windows_filetime_100ns_since_1601_utc",
                     "image_path": proc.image_path, "image_sha256": proc.image_sha256,
                     "command": command, "cwd": str(cwd), "environment": env,
                     "exit_code": exit_code, "cancelled": cancelled,
                     "observer_start_perf_ns": capture_start_ns, "observer_finish_perf_ns": time.perf_counter_ns(),
                     "environment_sha256": hashlib.sha256(json.dumps(env, sort_keys=True).encode()).hexdigest(),
                     "termination": proc.termination_observation, "provenance": "external", "confidence": "observed"}
        payload = json.dumps(lifecycle, sort_keys=True, allow_nan=False).encode()
        capture.append("process/lifecycle.json", payload, collector="windows_job_process_lifecycle", version="1",
                       start_ns=capture_start_ns, end_ns=time.perf_counter_ns(),
                       health={"capacity": 1024 * 1024, "observed": len(payload), "dropped": 0,
                               "incomplete_reason": None if exit_code is not None else "process_exit_unobserved"})
        RunStorage.write_json(run_dir / "metrics" / "process.json", result.to_dict())
        from ep_auditor.collectors.exact_trace import capture_directory
        capture_directory(run_dir, required=bool(self.config.data.get('capture', {}).get('exact_required', False))
                          or env.get('EP_AUDITOR_REQUIRE_EXACT') == '1')
        if capture_mode.lower() == 'coverage':
            from ep_auditor.collectors.coverage import capture_coverage
            inputs = json.loads(env.get('EP_AUDITOR_COVERAGE_BINARIES', '[]'))
            if not isinstance(inputs, list) or any(not isinstance(path, str) for path in inputs):
                raise EvidenceError('invalid declared coverage binary scope')
            capture_coverage(run_dir, [Path(path) for path in inputs], scope={'kind': 'owned_process', 'command': command})
        proc.close()
        return result
