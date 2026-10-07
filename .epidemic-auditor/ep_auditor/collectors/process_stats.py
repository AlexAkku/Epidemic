from __future__ import annotations

import threading
import time
import hashlib
import json
import ctypes
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import psutil

from ep_auditor.core.storage import RunStorage


def windows_clock_calibration():
    """Bracket a precise UTC observation with independent monotonic readings."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    for name in ("QueryPerformanceCounter", "QueryPerformanceFrequency", "GetSystemTimePreciseAsFileTime"):
        function = getattr(kernel, name)
        function.argtypes = [ctypes.POINTER(ctypes.c_uint64)]
        function.restype = None if name == "GetSystemTimePreciseAsFileTime" else ctypes.c_int
    frequency, begin, end, utc = (ctypes.c_uint64() for _ in range(4))
    if not kernel.QueryPerformanceFrequency(ctypes.byref(frequency)) or frequency.value == 0:
        raise ctypes.WinError(ctypes.get_last_error())
    if not kernel.QueryPerformanceCounter(ctypes.byref(begin)):
        raise ctypes.WinError(ctypes.get_last_error())
    perf_begin = time.perf_counter_ns()
    kernel.GetSystemTimePreciseAsFileTime(ctypes.byref(utc))
    perf_end = time.perf_counter_ns()
    if not kernel.QueryPerformanceCounter(ctypes.byref(end)):
        raise ctypes.WinError(ctypes.get_last_error())
    if end.value < begin.value or perf_end < perf_begin:
        raise RuntimeError("clock calibration reversed")
    return {"utc_filetime_100ns": utc.value, "utc_unix_ns": (utc.value - 116444736000000000) * 100,
            "qpc_begin_ticks": begin.value, "qpc_end_ticks": end.value, "qpc_frequency_hz": frequency.value,
            "perf_begin_ns": perf_begin, "perf_end_ns": perf_end,
            "provenance": "external", "confidence": "observed",
            "scope": "UTC sampled within QPC/perf brackets; clock continuity between anchors not presumed"}


class ProcessStatsCollector:
    def __init__(self, interval_seconds: float, capacity_bytes: int = 16 * 1024 * 1024) -> None:
        if not 0.005 <= interval_seconds <= 60 or type(capacity_bytes) is not int or not 1024 <= capacity_bytes <= 64 * 1024 * 1024:
            raise ValueError("invalid bounded resource sampling configuration")
        self.interval = interval_seconds
        self.capacity_bytes = capacity_bytes
        self.stored_bytes = 0
        self.dropped_samples = 0
        self.error: str | None = None
        self.capture_state: dict[str, dict[str, int]] = {}
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._root_pid: int | None = None
        self._run_dir: Path | None = None
        self.summary: dict[str, Any] = {
            "sample_count": 0,
            "peak_rss_bytes": 0,
            "peak_processes": 0,
            "peak_threads": 0,
            "total_read_bytes": 0,
            "total_write_bytes": 0,
        }
        self._baseline_io: dict[tuple[int, float], tuple[int, int]] = {}
        self._last_cpu: dict[tuple[int, float], tuple[float, float]] = {}
        self._known_processes: set[psutil.Process] = set()

    def start(self, pid: int, run_dir: Path) -> None:
        if self._thread is not None:
            raise RuntimeError("resource collector is single-use")
        self._root_pid = pid
        self._run_dir = run_dir
        self._stop.clear()
        self._thread = threading.Thread(target=self._loop, name=f"stats-{pid}", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._thread:
            self._thread.join(timeout=max(2.0, self.interval * 4))
            if self._thread.is_alive():
                raise RuntimeError("resource collector failed to stop")
        if self.error:
            raise RuntimeError("resource collector failed: " + self.error)

    def _tree(self, root: psutil.Process) -> list[psutil.Process]:
        # psutil.Process equality includes creation time; PID reuse cannot attach
        # an unrelated process to the already observed managed tree.
        live: set[psutil.Process] = set()
        for parent in self._known_processes | {root}:
            try:
                if parent.is_running():
                    live.add(parent)
                    live.update(parent.children(recursive=True))
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        self._known_processes = live
        return sorted(live, key=lambda process: process.pid)

    def _loop(self) -> None:
        try:
            self._collect()
        except Exception as error:
            self.error = repr(error)

    def _collect(self) -> None:
        assert self._root_pid is not None
        assert self._run_dir is not None
        started = time.perf_counter()
        samples_path = self._run_dir / "process_samples.jsonl"
        children_path = self._run_dir / "children.jsonl"
        samples_path.touch(exist_ok=False)
        children_path.touch(exist_ok=False)
        known_pids: set[tuple[int, float]] = set()
        try:
            root = psutil.Process(self._root_pid)
        except psutil.Error:
            self.summary["availability"] = "unavailable"
            self.summary["reason"] = "root_exited_before_sampling"
            return

        while not self._stop.is_set():
            now_mono = time.perf_counter()
            processes = self._tree(root)
            aggregate_cpu = 0.0
            aggregate_rss = 0
            aggregate_threads = 0
            aggregate_read = 0
            aggregate_write = 0
            cpu_complete = True
            io_complete = True
            proc_rows: list[dict[str, Any]] = []

            for proc in processes:
                try:
                    with proc.oneshot():
                        mem = proc.memory_info()
                        io = None
                        try:
                            io = proc.io_counters()
                        except (psutil.AccessDenied, AttributeError, NotImplementedError):
                            pass
                        cpu_times = proc.cpu_times()
                        threads = proc.num_threads()
                        name = proc.name()
                        status = proc.status()
                        create_time = proc.create_time()
                    cpu_total = float(cpu_times.user + cpu_times.system)
                    identity = (proc.pid, create_time)
                    previous = self._last_cpu.get(identity)
                    if previous is None:
                        cpu = None
                        cpu_complete = False
                    else:
                        prev_total, prev_wall = previous
                        cpu = max(0.0, ((cpu_total - prev_total) / max(now_mono - prev_wall, 1e-9)) * 100.0)
                    self._last_cpu[identity] = (cpu_total, now_mono)
                    read_bytes = int(io.read_bytes) if io else None
                    write_bytes = int(io.write_bytes) if io else None
                    if io:
                        if identity not in self._baseline_io:
                            self._baseline_io[identity] = (read_bytes, write_bytes)
                        base_read, base_write = self._baseline_io[identity]
                        aggregate_read += max(0, read_bytes - base_read)
                        aggregate_write += max(0, write_bytes - base_write)
                    else:
                        io_complete = False
                    aggregate_cpu += cpu or 0
                    aggregate_rss += int(mem.rss)
                    aggregate_threads += int(threads)
                    proc_rows.append({
                        "pid": proc.pid,
                        "ppid": proc.ppid(),
                        "name": name,
                        "status": status,
                        "cpu_percent": cpu,
                        "cpu_time_seconds": cpu_total,
                        "rss_bytes": int(mem.rss),
                        "peak_rss_bytes": getattr(mem, "peak_wset", None),
                        "threads": int(threads),
                        "read_bytes": read_bytes,
                        "write_bytes": write_bytes,
                        "create_time": create_time,
                        "provenance": "external",
                        "confidence": "observed",
                        "cpu_percent_confidence": "derived",
                    })
                    if identity not in known_pids:
                        known_pids.add(identity)
                        context = {}
                        for key, getter in (("command", proc.cmdline), ("cwd", proc.cwd), ("environment", proc.environ)):
                            try:
                                context[key] = getter()
                            except psutil.Error as error:
                                context[key] = None
                                context[key + "_unavailable_reason"] = type(error).__name__
                        context["environment_sha256"] = (hashlib.sha256(json.dumps(context["environment"], sort_keys=True).encode()).hexdigest()
                                                         if context["environment"] is not None else None)
                        self._record(children_path, {
                            "observed_utc": datetime.now(timezone.utc).isoformat(),
                            "monotonic_seconds": now_mono - started,
                            "pid": proc.pid,
                            "ppid": proc.ppid(),
                            "name": name,
                            "creation_time_utc_seconds": create_time,
                            **context,
                            "provenance": "external", "confidence": "observed",
                        })
                except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                    continue

            sample = {
                "observed_utc": datetime.now(timezone.utc).isoformat(),
                "monotonic_seconds": now_mono - started,
                "root_pid": self._root_pid,
                "process_count": len(proc_rows),
                "cpu_percent_sum": aggregate_cpu if cpu_complete and proc_rows else None,
                "cpu_time_seconds_sum": sum(row["cpu_time_seconds"] for row in proc_rows) if proc_rows else None,
                "root_cpu_time_seconds": next((row["cpu_time_seconds"] for row in proc_rows if row["pid"] == self._root_pid), None),
                "rss_bytes_sum": aggregate_rss if proc_rows else None,
                "thread_count_sum": aggregate_threads if proc_rows else None,
                "read_bytes_delta": aggregate_read if io_complete and proc_rows else None,
                "write_bytes_delta": aggregate_write if io_complete and proc_rows else None,
                "processes": proc_rows,
                "trace_time_perf_ns": time.perf_counter_ns(),
                "clock_domain": "python_perf_counter_windows_qpc",
                "game_time": None, "simulation_time": None,
                "coverage": "interval_sampling_can_miss_short_lived_processes",
                "provenance": "external", "confidence": "observed",
            }
            if not self._record(samples_path, sample):
                self.dropped_samples += 1
            self.summary["sample_count"] += 1
            self.summary["peak_rss_bytes"] = max(self.summary["peak_rss_bytes"], aggregate_rss)
            self.summary["peak_processes"] = max(self.summary["peak_processes"], len(proc_rows))
            self.summary["peak_threads"] = max(self.summary["peak_threads"], aggregate_threads)
            self.summary["total_read_bytes"] = max(self.summary["total_read_bytes"], aggregate_read)
            self.summary["total_write_bytes"] = max(self.summary["total_write_bytes"], aggregate_write)

            self._stop.wait(self.interval)

    def _record(self, path: Path, value: dict) -> bool:
        encoded = (json.dumps(value, sort_keys=True, allow_nan=False) + "\n").encode()
        state = self.capture_state.setdefault(path.name, {"observed": 0, "stored": 0, "dropped": 0})
        state["observed"] += len(encoded)
        if state["stored"] + len(encoded) > self.capacity_bytes:
            state["dropped"] += len(encoded)
            return False
        with path.open("ab") as stream:
            stream.write(encoded)
        state["stored"] += len(encoded)
        return True
