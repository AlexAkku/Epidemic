"""Non-elevated client for the fixed local ETW service protocol. No subprocesses."""
import ctypes
from ctypes import wintypes as w
import json
from pathlib import Path
import re

PIPE = r"\\.\pipe\EpidemicAuditorEtwBroker.v1"
SERVICE = "EpidemicAuditorEtwBroker"
STORE = Path(r"E:\Epidemic engine v1.0\_goal6_handoff\etw-broker\captures")
ID = re.compile(r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\Z")


class BrokerError(RuntimeError):
    pass


def capture_directory(result):
    """Read only the known-folder location and capture identity selected by the broker."""
    if not ID.fullmatch(result.get("capture_id", "")):
        raise BrokerError("canonical broker capture ID required")
    expected = STORE / result["capture_id"]
    supplied = Path(result.get("directory", ""))
    if supplied != expected or supplied.resolve() != expected:
        raise BrokerError("broker output escaped controlled capture location")
    return expected


def request_bytes(operation, *, capture_id=None, profile="GeneralProfile"):
    if operation == "START" and capture_id is None and profile == "GeneralProfile":
        return b"START GeneralProfile"
    if operation == "STATUS" and capture_id is None:
        return b"STATUS"
    if operation in {"STOP", "CANCEL"} and isinstance(capture_id, str) and ID.fullmatch(capture_id):
        return (operation + " " + capture_id).encode("ascii")
    raise BrokerError("fixed operation, allowlisted profile and canonical capture ID required")


class Overlapped(ctypes.Structure):
    _fields_ = [("Internal", ctypes.c_size_t), ("InternalHigh", ctypes.c_size_t),
                ("Offset", w.DWORD), ("OffsetHigh", w.DWORD), ("hEvent", w.HANDLE)]


class ServiceStatus(ctypes.Structure):
    _fields_ = [(name, w.DWORD) for name in ("type", "state", "controls", "win32_exit", "service_exit",
                                          "checkpoint", "wait_hint", "pid", "flags")]


class BrokerClient:
    def __init__(self):
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.advapi = ctypes.WinDLL("advapi32", use_last_error=True)
        definitions = {
            "CreateFileW": ([w.LPCWSTR, w.DWORD, w.DWORD, w.LPVOID, w.DWORD, w.DWORD, w.HANDLE], w.HANDLE),
            "CloseHandle": ([w.HANDLE], w.BOOL), "WaitNamedPipeW": ([w.LPCWSTR, w.DWORD], w.BOOL),
            "SetNamedPipeHandleState": ([w.HANDLE, ctypes.POINTER(w.DWORD), w.LPVOID, w.LPVOID], w.BOOL),
            "GetNamedPipeServerProcessId": ([w.HANDLE, ctypes.POINTER(w.ULONG)], w.BOOL),
            "CreateEventW": ([w.LPVOID, w.BOOL, w.BOOL, w.LPCWSTR], w.HANDLE),
            "ReadFile": ([w.HANDLE, w.LPVOID, w.DWORD, ctypes.POINTER(w.DWORD), ctypes.POINTER(Overlapped)], w.BOOL),
            "WriteFile": ([w.HANDLE, w.LPVOID, w.DWORD, ctypes.POINTER(w.DWORD), ctypes.POINTER(Overlapped)], w.BOOL),
            "WaitForSingleObject": ([w.HANDLE, w.DWORD], w.DWORD),
            "GetOverlappedResult": ([w.HANDLE, ctypes.POINTER(Overlapped), ctypes.POINTER(w.DWORD), w.BOOL], w.BOOL),
            "CancelIoEx": ([w.HANDLE, ctypes.POINTER(Overlapped)], w.BOOL),
            "GetCurrentProcess": ([], w.HANDLE),
        }
        for name, (arguments, result) in definitions.items():
            function = getattr(self.kernel, name)
            function.argtypes, function.restype = arguments, result
        for name, arguments, result in (
            ("OpenSCManagerW", [w.LPCWSTR, w.LPCWSTR, w.DWORD], w.HANDLE),
            ("OpenServiceW", [w.HANDLE, w.LPCWSTR, w.DWORD], w.HANDLE),
            ("QueryServiceStatusEx", [w.HANDLE, ctypes.c_int, w.LPVOID, w.DWORD, ctypes.POINTER(w.DWORD)], w.BOOL),
            ("CloseServiceHandle", [w.HANDLE], w.BOOL),
            ("OpenProcessToken", [w.HANDLE, w.DWORD, ctypes.POINTER(w.HANDLE)], w.BOOL),
            ("GetTokenInformation", [w.HANDLE, ctypes.c_int, w.LPVOID, w.DWORD, ctypes.POINTER(w.DWORD)], w.BOOL),
        ):
            function = getattr(self.advapi, name)
            function.argtypes, function.restype = arguments, result

    def _normal_token(self):
        token = w.HANDLE()
        if not self.advapi.OpenProcessToken(self.kernel.GetCurrentProcess(), 8, ctypes.byref(token)):
            raise BrokerError("client token unavailable")
        try:
            elevated, size = w.DWORD(), w.DWORD()
            if not self.advapi.GetTokenInformation(token, 20, ctypes.byref(elevated), ctypes.sizeof(elevated), ctypes.byref(size)):
                raise BrokerError("client elevation unavailable")
            if elevated.value:
                raise BrokerError("Auditor/broker clients must remain non-elevated")
        finally:
            self.kernel.CloseHandle(token)

    def _service_pid(self):
        manager = self.advapi.OpenSCManagerW(None, None, 1)
        if not manager:
            raise BrokerError("service manager unavailable")
        try:
            service = self.advapi.OpenServiceW(manager, SERVICE, 4)
            if not service:
                raise BrokerError("ETW broker not installed; one-time administrator installation required")
            try:
                status, needed = ServiceStatus(), w.DWORD()
                if not self.advapi.QueryServiceStatusEx(service, 0, ctypes.byref(status), ctypes.sizeof(status), ctypes.byref(needed)):
                    raise BrokerError("ETW broker status unavailable")
                if status.state != 4 or not status.pid:
                    raise BrokerError("ETW broker service not running")
                return status.pid
            finally:
                self.advapi.CloseServiceHandle(service)
        finally:
            self.advapi.CloseServiceHandle(manager)

    def _io(self, pipe, buffer, *, write=False, timeout_ms=300000):
        event = self.kernel.CreateEventW(None, True, False, None)
        if not event:
            raise BrokerError("IPC completion event unavailable")
        pending, count = Overlapped(), w.DWORD()
        pending.hEvent = event
        try:
            function = self.kernel.WriteFile if write else self.kernel.ReadFile
            if not function(pipe, buffer, ctypes.sizeof(buffer), ctypes.byref(count), ctypes.byref(pending)):
                if ctypes.get_last_error() != 997:
                    raise BrokerError("broker IPC failed: " + str(ctypes.get_last_error()))
                if self.kernel.WaitForSingleObject(event, timeout_ms) != 0:
                    self.kernel.CancelIoEx(pipe, ctypes.byref(pending))
                    self.kernel.GetOverlappedResult(pipe, ctypes.byref(pending), ctypes.byref(count), True)
                    raise BrokerError("broker IPC deadline exceeded")
                if not self.kernel.GetOverlappedResult(pipe, ctypes.byref(pending), ctypes.byref(count), False):
                    raise BrokerError("broker IPC completion failed")
            return count.value
        finally:
            self.kernel.CloseHandle(event)

    def request(self, operation, *, capture_id=None, profile="GeneralProfile"):
        payload = request_bytes(operation, capture_id=capture_id, profile=profile)
        return self._exchange(payload)

    def _exchange(self, payload):
        """Private transport; the service independently enforces fixed syntax."""
        self._normal_token()
        expected_pid = self._service_pid()
        if not self.kernel.WaitNamedPipeW(PIPE, 5000):
            raise BrokerError("broker pipe unavailable or busy")
        # Identification SQOS prevents arbitrary client impersonation. Specific
        # access omits FILE_CREATE_PIPE_INSTANCE; the client cannot host this pipe.
        pipe = self.kernel.CreateFileW(PIPE, 0x00100183, 0, None, 3, 0x40110000, None)
        if pipe == ctypes.c_void_p(-1).value:
            raise BrokerError("broker pipe admission denied: " + str(ctypes.get_last_error()))
        try:
            server, mode = w.ULONG(), w.DWORD(2)
            if (not self.kernel.GetNamedPipeServerProcessId(pipe, ctypes.byref(server)) or server.value != expected_pid
                    or expected_pid != self._service_pid()):
                raise BrokerError("pipe server identity differs from SCM service")
            if not self.kernel.SetNamedPipeHandleState(pipe, ctypes.byref(mode), None, None):
                raise BrokerError("bounded IPC message mode unavailable")
            outgoing = ctypes.create_string_buffer(payload, len(payload))
            if self._io(pipe, outgoing, write=True, timeout_ms=5000) != len(payload):
                raise BrokerError("incomplete broker request")
            incoming = ctypes.create_string_buffer(4096)
            count = self._io(pipe, incoming)
            result = json.loads(incoming.raw[:count].decode("utf-8", errors="strict"))
            acknowledgement = ctypes.create_string_buffer(b"ACK", 3)
            self._io(pipe, acknowledgement, write=True, timeout_ms=1000)
            if not isinstance(result, dict) or result.get("protocolVersion") != 1 or result.get("broker_pid") != expected_pid:
                raise BrokerError("broker response schema or process identity invalid")
            if result.get("ok") is not True:
                raise BrokerError("broker rejected request: " + str(result.get("error")))
            if not ID.fullmatch(result.get("capture_id", "")):
                raise BrokerError("broker capture identity invalid")
            capture_directory(result)
            result["client_elevated"] = False
            return result
        finally:
            self.kernel.CloseHandle(pipe)

    def start(self, profile="GeneralProfile"):
        result = self.request("START", profile=profile)
        if result.get("state") != "RECORDING":
            raise BrokerError("broker recording not observed")
        return result

    def stop(self, capture_id):
        result = self.request("STOP", capture_id=capture_id)
        if result.get("state") != "COMPLETE" or result["capture_id"] != capture_id:
            raise BrokerError("broker capture not complete")
        return result

    def status(self):
        return self.request("STATUS")

    def cancel(self, capture_id):
        return self.request("CANCEL", capture_id=capture_id)
