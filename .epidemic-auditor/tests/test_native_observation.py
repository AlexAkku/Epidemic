"""Physical DbgHelp qualification using the built, independent observation helper."""
import json
import os
from pathlib import Path
import shutil
import tempfile
import unittest
import ctypes
from contextlib import contextmanager
import hashlib
import threading
import time
import uuid

from test_core import AuditorCoreTests
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import RawEvidence, digest

ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT / "build/auditor-native/bin" / os.environ.get("EP_AUDITOR_NATIVE_CONFIG", "Release") / "EpidemicAuditorObservation.exe"
SOURCE = ROOT / ".epidemic-auditor/native/observation.cpp"


class NativeObservationTests(unittest.TestCase):
    @contextmanager
    def mutex_fixture(self, blocked):
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateMutexW.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_wchar_p]
        kernel.CreateMutexW.restype = ctypes.c_void_p
        kernel.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        kernel.WaitForSingleObject.restype = ctypes.c_uint32
        kernel.ReleaseMutex.argtypes = [ctypes.c_void_p]
        kernel.ReleaseMutex.restype = ctypes.c_int
        kernel.CloseHandle.argtypes = [ctypes.c_void_p]
        kernel.CloseHandle.restype = ctypes.c_int
        name = "Local\\AuditorWctFixture_" + uuid.uuid4().hex
        mutex = kernel.CreateMutexW(None, True, name)
        self.assertTrue(mutex)
        ready, acquired, stop = threading.Event(), threading.Event(), threading.Event()
        errors = []
        owner_tid = threading.get_native_id()

        def work():
            ready.set()
            status = kernel.WaitForSingleObject(mutex, 20000)
            if status != 0:
                errors.append(status)
                return
            acquired.set()
            try:
                while not stop.is_set():
                    hashlib.sha256(b"observed busy thread" * 4096).digest()
            finally:
                if not kernel.ReleaseMutex(mutex):
                    errors.append("worker release failed")

        thread = threading.Thread(target=work)
        thread.start()
        owns = True
        try:
            self.assertTrue(ready.wait(5))
            if blocked:
                time.sleep(0.1)
            else:
                self.assertTrue(kernel.ReleaseMutex(mutex))
                owns = False
                self.assertTrue(acquired.wait(5))
            yield thread.native_id, owner_tid, name
        finally:
            if owns:
                kernel.ReleaseMutex(mutex)
            stop.set()
            thread.join(25)
            kernel.CloseHandle(mutex)
            self.assertFalse(thread.is_alive())
            self.assertEqual(errors, [])

    def observe(self, arguments):
        self.assertTrue(HELPER.is_file(), "mandatory physical helper must be built")
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            cfg, _ = AuditorCoreTests()._config(directory)
            executable = HELPER
            storage = RunStorage(cfg)
            run = storage.new_run("symbols", "physical external symbols", "Fast")
            result = ManagedProcessRunner(cfg).run([str(executable), *arguments], directory, run.run_dir, "Fast", timeout=20)
            self.assertEqual(result.exit_code, 0)
            captured, _ = RawEvidence(run.run_dir).read_capture("process/stdout.bin")
            payload = json.loads(captured.decode("utf-8"))
            destination = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / self._testMethodName
            shutil.copytree(run.run_dir, destination)
            (destination / "physical_inputs.json").write_text(json.dumps({
                "exe_sha256": digest(HELPER), "pdb_sha256": digest(HELPER.with_suffix(".pdb")),
                "source_sha256": digest(SOURCE), "arguments": arguments}), encoding="utf-8")
            return payload

    def test_matching_pdb_resolves_real_function_and_exact_source_line(self):
        payload = self.observe(["--self"])
        symbol = payload["symbol"]
        self.assertEqual(symbol["status"], "resolved")
        self.assertEqual(symbol["function"], "AuditorObservationFixture")
        line = next(index for index, value in enumerate(SOURCE.read_text().splitlines(), 1)
                    if value.startswith('extern "C" __declspec(noinline)'))
        self.assertEqual(Path(symbol["source_file"]).resolve(), SOURCE.resolve())
        self.assertEqual(symbol["source_line"], line)
        own = next(module for module in payload["modules"] if Path(module.get("path", "")).name == HELPER.name)
        self.assertIsNotNone(own["pdb_identity"])
        self.assertGreater(own["pdb_identity"]["age"], 0)
        self.assertTrue(own["base"] <= symbol["raw_address"] < own["base"] + own["size"])
        self.assertTrue(payload["module_list_complete"])
        self.assertEqual(payload["module_count"], len(payload["modules"]))
        self.assertEqual(payload["provenance"], "external")

    def test_unknown_address_retained_without_invented_symbol(self):
        payload = self.observe(["--self", "--address", "1"])
        self.assertEqual(payload["symbol"], {"raw_address": 1, "status": "unavailable", "function": None,
                                            "source_file": None, "source_line": None,
                                            "function_address": None, "displacement": None})

    def test_external_wct_proves_named_mutex_and_owner_without_deadlock(self):
        with self.mutex_fixture(True) as (tid, owner, name):
            payload = self.observe(["--pid", str(os.getpid()), "--thread", str(tid)])
            chain = payload["wait_chain"]
            self.assertTrue(chain["available"], chain)
            self.assertFalse(chain["cycle"])
            nodes = chain["nodes"]
            self.assertEqual(nodes[0]["os_tid"], tid)
            self.assertTrue(any(node.get("name", "").endswith(name.split("\\")[-1]) for node in nodes), chain)
            self.assertTrue(any(node.get("os_tid") == owner and node.get("pid") == os.getpid() for node in nodes), chain)

    def test_external_wct_released_mutex_has_no_wait_owner_edge(self):
        with self.mutex_fixture(False) as (tid, owner, name):
            payload = self.observe(["--pid", str(os.getpid()), "--thread", str(tid)])
            chain = payload["wait_chain"]
            self.assertTrue(chain["available"], chain)
            self.assertFalse(chain["cycle"])
            self.assertFalse(any(node.get("os_tid") == owner for node in chain["nodes"]), chain)
            self.assertFalse(any(node.get("name", "").endswith(name.split("\\")[-1]) for node in chain["nodes"]), chain)
