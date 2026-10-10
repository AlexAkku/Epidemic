"""Windows kernel-owned process trees, assigned while the target is suspended."""
from __future__ import annotations

import ctypes
from ctypes import wintypes as w
import io
import os
import subprocess
import time
from .evidence import digest


class BasicLimits(ctypes.Structure):
    _fields_ = [("user_time", ctypes.c_int64), ("job_time", ctypes.c_int64), ("flags", w.DWORD),
                ("min_working_set", ctypes.c_size_t), ("max_working_set", ctypes.c_size_t),
                ("active_limit", w.DWORD), ("affinity", ctypes.c_size_t), ("priority", w.DWORD), ("scheduling", w.DWORD)]


class IoCounters(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint64) for name in
                ("read_ops", "write_ops", "other_ops", "read_bytes", "write_bytes", "other_bytes")]


class ExtendedLimits(ctypes.Structure):
    _fields_ = [("basic", BasicLimits), ("io", IoCounters), ("process_memory", ctypes.c_size_t),
                ("job_memory", ctypes.c_size_t), ("peak_process_memory", ctypes.c_size_t), ("peak_job_memory", ctypes.c_size_t)]


class Accounting(ctypes.Structure):
    _fields_ = [(name, ctypes.c_int64) for name in ("user", "kernel", "period_user", "period_kernel")]
    _fields_ += [(name, w.DWORD) for name in ("page_faults", "total_processes", "active_processes", "terminated_processes")]


class OwnedProcess:
    """Popen-compatible byte streams; cancellation addresses a Job handle, never a PID."""
    def __init__(self, command: list[str], cwd: str, environment: dict[str, str], creationflags=0, on_assigned=None):
        if os.name != "nt":
            raise RuntimeError("owned Auditor process trees require Windows")
        import _winapi
        import msvcrt
        self.api = _winapi
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        signatures = {
            "CreateJobObjectW": ([ctypes.c_void_p, w.LPCWSTR], w.HANDLE),
            "SetInformationJobObject": ([w.HANDLE, ctypes.c_int, ctypes.c_void_p, w.DWORD], w.BOOL),
            "QueryInformationJobObject": ([w.HANDLE, ctypes.c_int, ctypes.c_void_p, w.DWORD, ctypes.c_void_p], w.BOOL),
            "AssignProcessToJobObject": ([w.HANDLE, w.HANDLE], w.BOOL),
            "TerminateJobObject": ([w.HANDLE, w.UINT], w.BOOL),
            "OpenProcess": ([w.DWORD, w.BOOL, w.DWORD], w.HANDLE),
            "IsProcessInJob": ([w.HANDLE, w.HANDLE, ctypes.POINTER(w.BOOL)], w.BOOL),
            "ResumeThread": ([w.HANDLE], w.DWORD),
            "QueryFullProcessImageNameW": ([w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD)], w.BOOL),
            "GetProcessTimes": ([w.HANDLE, ctypes.POINTER(w.FILETIME), ctypes.POINTER(w.FILETIME),
                                 ctypes.POINTER(w.FILETIME), ctypes.POINTER(w.FILETIME)], w.BOOL),
        }
        for name, (arguments, result) in signatures.items():
            function = getattr(self.kernel, name)
            function.argtypes, function.restype = arguments, result
        self.job = self.kernel.CreateJobObjectW(None, None)
        if not self.job:
            raise ctypes.WinError(ctypes.get_last_error())
        self.handle = None
        self.stdout = self.stderr = None
        self.returncode = None
        self.cancel_reason = None
        self.termination_observation = None
        temporary_handles = []
        read_handles = []
        input_fd = None
        thread_handle = None
        try:
            limits = ExtendedLimits()
            limits.basic.flags = 0x2000 | 0x200  # KILL_ON_JOB_CLOSE | JOB_MEMORY
            limits.job_memory = 8 * 1024**3
            self._check(self.kernel.SetInformationJobObject(self.job, 9, ctypes.byref(limits), ctypes.sizeof(limits)))
            current = _winapi.GetCurrentProcess()

            def inheritable(handle):
                copied = _winapi.DuplicateHandle(current, handle, current, 0, True, _winapi.DUPLICATE_SAME_ACCESS)
                temporary_handles.append(copied)
                return copied

            output_read, output_write = _winapi.CreatePipe(None, 0)
            read_handles.append(output_read)
            temporary_handles.append(output_write)
            error_read, error_write = _winapi.CreatePipe(None, 0)
            read_handles.append(error_read)
            temporary_handles.append(error_write)
            input_fd = os.open(os.devnull, os.O_RDONLY)
            startup = subprocess.STARTUPINFO()
            startup.dwFlags = _winapi.STARTF_USESTDHANDLES
            startup.hStdInput = inheritable(msvcrt.get_osfhandle(input_fd))
            startup.hStdOutput = inheritable(output_write)
            startup.hStdError = inheritable(error_write)
            startup.lpAttributeList = {"handle_list": [startup.hStdInput, startup.hStdOutput, startup.hStdError]}
            flags = creationflags | 0x4 | 0x08000000  # CREATE_SUSPENDED | CREATE_NO_WINDOW
            self.handle, thread_handle, self.pid, self.thread_id = _winapi.CreateProcess(
                None, subprocess.list2cmdline(command), None, None, True, flags, environment, cwd, startup)
            # Target code cannot create an escaping child before this assignment.
            self._check(self.kernel.AssignProcessToJobObject(self.job, self.handle))
            buffer = ctypes.create_unicode_buffer(32768)
            length = w.DWORD(len(buffer))
            self._check(self.kernel.QueryFullProcessImageNameW(self.handle, 0, buffer, ctypes.byref(length)))
            self.image_path = buffer.value
            from pathlib import Path
            self.image_sha256 = digest(Path(self.image_path))
            created, exited, kernel, user = w.FILETIME(), w.FILETIME(), w.FILETIME(), w.FILETIME()
            self._check(self.kernel.GetProcessTimes(self.handle, ctypes.byref(created), ctypes.byref(exited),
                                                   ctypes.byref(kernel), ctypes.byref(user)))
            self.creation_filetime_100ns = (created.dwHighDateTime << 32) | created.dwLowDateTime
            if on_assigned is not None:
                on_assigned(self)
            if self.cancel_reason is None:
                self._check(self.kernel.ResumeThread(thread_handle) != 0xFFFFFFFF)
            output_fd = msvcrt.open_osfhandle(output_read, os.O_RDONLY | os.O_BINARY)
            read_handles.remove(output_read)
            self.stdout = io.open(output_fd, "rb", buffering=65536)
            error_fd = msvcrt.open_osfhandle(error_read, os.O_RDONLY | os.O_BINARY)
            read_handles.remove(error_read)
            self.stderr = io.open(error_fd, "rb", buffering=65536)
        except BaseException:
            if self.handle:
                _winapi.TerminateProcess(self.handle, 1223)
                _winapi.WaitForSingleObject(self.handle, 10000)
            self.close()
            raise
        finally:
            for handle in temporary_handles + read_handles + ([thread_handle] if thread_handle else []):
                _winapi.CloseHandle(handle)
            if input_fd is not None:
                os.close(input_fd)

    @staticmethod
    def _check(value):
        if not value:
            raise ctypes.WinError(ctypes.get_last_error())

    def active_process_count(self) -> int:
        if not self.job:
            return 0
        information = Accounting()
        self._check(self.kernel.QueryInformationJobObject(self.job, 1, ctypes.byref(information), ctypes.sizeof(information), None))
        return information.active_processes

    def observe_members(self) -> dict:
        """Bounded kernel membership observation; races never become an observed zero."""
        class ProcessIds(ctypes.Structure):
            _fields_ = [("assigned", w.DWORD), ("listed", w.DWORD), ("ids", ctypes.c_size_t * 256)]
        information = ProcessIds()
        members = []
        started = time.perf_counter_ns()
        try:
            if not self.job:
                raise RuntimeError("managed_job_closed")
            self._check(self.kernel.QueryInformationJobObject(self.job, 3, ctypes.byref(information),
                                                             ctypes.sizeof(information), None))
            if information.assigned != information.listed or information.listed > 256:
                raise RuntimeError("job_membership_capacity_or_concurrent_change")
            for pid in information.ids[:information.listed]:
                handle = self.kernel.OpenProcess(0x1000, False, pid)
                if not handle:
                    raise ctypes.WinError(ctypes.get_last_error())
                try:
                    belongs = w.BOOL()
                    self._check(self.kernel.IsProcessInJob(handle, self.job, ctypes.byref(belongs)))
                    if not belongs.value:
                        raise RuntimeError("job_member_identity_race")
                    buffer, length = ctypes.create_unicode_buffer(32768), w.DWORD(32768)
                    self._check(self.kernel.QueryFullProcessImageNameW(handle, 0, buffer, ctypes.byref(length)))
                    created, exited, kernel, user = w.FILETIME(), w.FILETIME(), w.FILETIME(), w.FILETIME()
                    self._check(self.kernel.GetProcessTimes(handle, ctypes.byref(created), ctypes.byref(exited),
                                                           ctypes.byref(kernel), ctypes.byref(user)))
                    members.append({"pid": pid, "image_path": buffer.value,
                                    "creation_filetime_100ns": (created.dwHighDateTime << 32) | created.dwLowDateTime})
                finally:
                    self.api.CloseHandle(handle)
            return {"availability": "observed", "members": members, "reason": None,
                    "start_perf_ns": started, "end_perf_ns": time.perf_counter_ns(),
                    "method": "QueryInformationJobObject+IsProcessInJob+QueryFullProcessImageNameW"}
        except (OSError, RuntimeError) as error:
            return {"availability": "unavailable", "members": members, "reason": str(error),
                    "start_perf_ns": started, "end_perf_ns": time.perf_counter_ns(),
                    "method": "QueryInformationJobObject+IsProcessInJob+QueryFullProcessImageNameW"}

    def cancel(self, reason="user_stop") -> bool:
        count = self.active_process_count()
        if not count:
            return False
        self._check(self.kernel.TerminateJobObject(self.job, 1223))
        self.cancel_reason = reason
        self.termination_observation = {"method": "TerminateJobObject", "active_processes_before": count,
                                        "purpose": "scope_cleanup" if reason == "root_exit_live_descendants" else "cancellation",
                                        "reason": reason, "provenance": "external", "confidence": "observed"}
        return True

    def poll(self) -> int | None:
        if self.returncode is None and self.handle:
            if self.api.WaitForSingleObject(self.handle, 0) == self.api.WAIT_OBJECT_0:
                self.returncode = self.api.GetExitCodeProcess(self.handle)
        return self.returncode

    def close(self):
        # Close the kill-on-close Job before touching a pipe whose reader may block.
        if self.job:
            self.api.CloseHandle(self.job)
            self.job = None
        for stream in (self.stdout, self.stderr):
            if stream:
                stream.close()
        if self.handle:
            self.api.CloseHandle(self.handle)
            self.handle = None
