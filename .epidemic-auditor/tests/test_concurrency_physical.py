"""Controlled native fixtures checked with external Windows observations, not labels."""
import ctypes
import json
import os
from pathlib import Path
import tempfile
import time
import unittest
import uuid
import shutil

from ep_auditor.core.owned_process import OwnedProcess
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import RawEvidence
from ep_auditor.collectors.process_stats import windows_clock_calibration
from test_core import AuditorCoreTests

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "build/ms62/native/Release/EpidemicAuditorConcurrencyFixture.exe"
HELPER = FIXTURE.with_name("EpidemicAuditorObservation.exe")


class PhysicalConcurrencyTests(unittest.TestCase):
    def setUp(self):
        self.clock_begin = windows_clock_calibration()
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        signatures = {"CreateEventW": ([ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_wchar_p], ctypes.c_void_p),
                      "SetEvent": ([ctypes.c_void_p], ctypes.c_int), "CloseHandle": ([ctypes.c_void_p], ctypes.c_int),
                      "OpenThread": ([ctypes.c_uint32, ctypes.c_int, ctypes.c_uint32], ctypes.c_void_p),
                      "GetThreadPriority": ([ctypes.c_void_p], ctypes.c_int),
                      "GetThreadTimes": ([ctypes.c_void_p, *([ctypes.POINTER(ctypes.c_uint64)] * 4)], ctypes.c_int)}
        for name, (arguments, result) in signatures.items():
            function = getattr(self.kernel, name)
            function.argtypes, function.restype = arguments, result

    def actor(self, directory, mode, variant, name):
        self.assertTrue(FIXTURE.is_file(), "mandatory fixture must be built")
        started = time.perf_counter_ns()
        process = OwnedProcess([str(FIXTURE), mode, variant, name], str(directory), os.environ.copy())
        payloads = []
        for _ in range(3 if mode == "contention" and variant == "bad" else 1):
            payloads.append(json.loads(process.stdout.readline()))
        self.assertTrue(all(row["pid"] == process.pid for row in payloads))
        return process, started, payloads

    def retain(self, mode, variant, payload, started):
        payload["clock_calibrations"] = [self.clock_begin, windows_clock_calibration()]
        path = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / (mode + "_" + variant)
        encoded = json.dumps(payload).encode()
        RawEvidence(path).append("control/external_observations.json", encoded, collector="external_fixture_observer", version="1",
            start_ns=started, end_ns=time.perf_counter_ns(), health={"capacity": len(encoded), "observed": len(encoded),
                                                                  "dropped": 0, "incomplete_reason": None})

    def wait(self, process):
        self.assertEqual(process.api.WaitForSingleObject(process.handle, 12000), process.api.WAIT_OBJECT_0)
        self.assertEqual(process.poll(), 0)

    def test_cooperative_cancel_and_completion_good_bad_have_external_proof_of_effect(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for mode in ("cancel", "completion"):
                for variant in ("good", "bad"):
                    name = "Local\\AuditorControl_" + uuid.uuid4().hex
                    event = self.kernel.CreateEventW(None, True, False, name)
                    self.assertTrue(event)
                    process = None
                    try:
                        process, started, declarations = self.actor(directory, mode, variant, name)
                        request_begin = time.perf_counter_ns()
                        self.assertTrue(self.kernel.SetEvent(event))
                        request_end = time.perf_counter_ns()
                        self.wait(process)
                        ended = time.perf_counter_ns()
                        latency = ended - request_end
                        self.retain(mode, variant, {"fixture_declarations": declarations, "pid": process.pid,
                            "creation_filetime_100ns": process.creation_filetime_100ns, "object_name": name,
                            "observation_kind": "external_control_request", "api": "SetEvent", "api_succeeded": True,
                            "request_begin_ns": request_begin, "request_end_ns": request_end, "observed_end_ns": ended,
                            "clock_domain": "monotonic_trace", "process_end_latency_ns": latency}, started)
                        if variant == "bad": self.assertGreater(latency, 1000000000)
                        else: self.assertLess(latency, 500000000)
                    finally:
                        if process: process.close()
                        self.kernel.CloseHandle(event)

    def test_starvation_configuration_and_cpu_share_are_independently_observed(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            for variant in ("good", "bad"):
                process, started, declarations = self.actor(directory, "starvation", variant, "unused")
                handle = None
                try:
                    tid = declarations[0]["worker_tid"]
                    handle = self.kernel.OpenThread(0x0800, False, tid)
                    self.assertTrue(handle)
                    priority = self.kernel.GetThreadPriority(handle)
                    self.assertEqual(priority, -15 if variant == "bad" else 0)
                    time.sleep(0.4)
                    created, ended, kernel, user = (ctypes.c_uint64() for _ in range(4))
                    self.assertTrue(self.kernel.GetThreadTimes(handle, ctypes.byref(created), ctypes.byref(ended),
                                                              ctypes.byref(kernel), ctypes.byref(user)))
                    cpu_ns = (kernel.value + user.value) * 100
                    self.retain("starvation", variant, {"pid": process.pid, "os_tid": tid, "priority": priority,
                        "observed_cpu_ns": cpu_ns, "fixture_declarations": declarations,
                        "scope": "external thread priority/CPU during controlled competing workload; READY timeline requires ETW"}, started)
                    if variant == "bad": self.assertLess(cpu_ns, 50000000)
                    else: self.assertGreater(cpu_ns, 50000000)
                    self.wait(process)
                finally:
                    if handle: self.kernel.CloseHandle(handle)
                    process.close()

    def test_mutex_and_contention_pairs_have_external_owner_and_duration_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            config, _ = AuditorCoreTests()._config(directory)
            storage = RunStorage(config)
            for mode in ("mutex", "contention"):
                for variant in ("good", "bad"):
                    name = "Local\\AuditorWait_" + uuid.uuid4().hex
                    release = self.kernel.CreateEventW(None, True, False, name + "_release")
                    self.assertTrue(release)
                    process, started, declarations = self.actor(directory, mode, variant, name)
                    snapshots = []
                    try:
                        if variant == "bad":
                            time.sleep(0.1)
                            probes = declarations * 2 if mode == "mutex" else declarations
                            for probe_index, declaration in enumerate(probes):
                                if mode == "mutex" and probe_index == 1:
                                    time.sleep(1.1)
                                run = storage.new_run("wait", "independent wait snapshot", "Fast")
                                result = ManagedProcessRunner(config).run([str(HELPER), "--pid", str(process.pid),
                                    "--thread", str(declaration["worker_tid"]), "--wait-only"], directory, run.run_dir, "Fast", timeout=5)
                                destination = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / f"probe_{mode}_{variant}_{declaration['worker_tid']}_{probe_index}"
                                shutil.copytree(run.run_dir, destination)
                                self.assertEqual(result.exit_code, 0, (run.run_dir / "stderr.log").read_text()[:500])
                                raw, _ = RawEvidence(run.run_dir).read_capture("process/stdout.bin")
                                snapshot = json.loads(raw)
                                self.assertEqual(snapshot["creation_filetime_100ns"], process.creation_filetime_100ns)
                                chain = snapshot["wait_chain"]
                                self.assertTrue(chain["available"], chain)
                                self.assertFalse(chain["cycle"])
                                self.assertTrue(any(node.get("os_tid") == declaration["owner_tid"] for node in chain["nodes"]))
                                self.assertTrue(any(node.get("name", "").endswith(name.split("\\")[-1]) for node in chain["nodes"]))
                                snapshots.append(snapshot)
                            if mode == "mutex":
                                self.assertEqual(len(snapshots), 2)
                            remaining = 3 - (time.perf_counter_ns() - started) / 1e9
                            if remaining > 0: time.sleep(remaining)
                            self.assertTrue(self.kernel.SetEvent(release))
                        self.wait(process)
                        elapsed = time.perf_counter_ns() - started
                        self.retain(mode, variant, {"pid": process.pid, "fixture_declarations": declarations,
                            "wct_snapshots": snapshots, "observed_process_duration_ns": elapsed,
                            "scope": "WCT owners/resources at snapshots and process lifetime; continuous hold duration unproven"}, started)
                        if variant == "bad": self.assertGreater(elapsed, 2800000000)
                        else: self.assertLess(elapsed, 1000000000)
                    finally:
                        self.kernel.SetEvent(release)
                        process.close()
                        self.kernel.CloseHandle(release)

    def test_orphan_good_bad_are_kernel_job_observations_after_root_exit(self):
        with tempfile.TemporaryDirectory() as temporary:
            for variant in ("good", "bad"):
                process, started, declarations = self.actor(Path(temporary), "orphan", variant, "unused")
                try:
                    child = json.loads(process.stdout.readline())["child_pid"]
                    self.wait(process)
                    membership = process.observe_members()
                    attempts = [membership]
                    for _ in range(10):
                        if membership["availability"] == "observed": break
                        time.sleep(0.02)
                        membership = process.observe_members()
                        attempts.append(membership)
                    self.assertEqual(membership["availability"], "observed", membership)
                    present = child in [member["pid"] for member in membership["members"]]
                    self.retain("orphan", variant, {"pid": process.pid, "child_pid": child, "root_exit_code": process.poll(),
                        "kernel_membership_after_root_exit": membership, "membership_attempts": attempts,
                        "fixture_declarations": declarations}, started)
                    self.assertEqual(present, variant == "bad")
                    if present: time.sleep(1.3)
                    self.assertEqual(process.active_process_count(), 0)
                finally: process.close()

    def test_mutex_api_hold_and_acquisition_stack_are_external_and_existing_object_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            config, _ = AuditorCoreTests()._config(directory)
            storage = RunStorage(config)
            for variant in ("good", "bad", "existing"):
                started = time.perf_counter_ns()
                run = storage.new_run("mutex_api", "intrusive external mutex fixture", "Fast")
                result = ManagedProcessRunner(config).run([str(HELPER), "--mutex-fixture", str(FIXTURE), variant],
                    directory, run.run_dir, "Fast", timeout=15)
                destination = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("mutex_api_run_" + variant)
                shutil.copytree(run.run_dir, destination)
                self.assertEqual(result.exit_code, 0, (run.run_dir / "stderr.log").read_text()[:500])
                raw, _ = RawEvidence(run.run_dir).read_capture("process/stdout.bin")
                observed = json.loads(raw)
                self.assertEqual(observed["provenance"], "external")
                self.assertIn("Intrusive debugger", observed["scope"])
                self.retain("mutex_api", variant, observed, started)
                if variant == "existing":
                    self.assertEqual(observed.get("create_status"), 0x40000000)
                    self.assertTrue(observed["not_new"])
                    self.assertFalse(observed["complete"])
                    self.assertEqual(observed["acquisition_stack"], [])
                    continue
                self.assertTrue(observed["complete"])
                self.assertEqual(observed.get("create_status"), 0)
                self.assertEqual(observed.get("release_status"), 0)
                frequency = observed["qpc_frequency"]
                lower = (observed["release_begin"] - observed["acquire_end"]) * 1000000000 // frequency
                upper = (observed["release_end"] - observed["acquire_begin"]) * 1000000000 // frequency
                self.assertGreaterEqual(upper, lower)
                if variant == "bad": self.assertGreater(lower, 2500000000)
                else: self.assertLess(upper, 1000000000)
                callers = [frame for frame in observed["acquisition_stack"] if frame["source_file"]
                    and frame["source_file"].lower().endswith("concurrency_fixture.cpp")]
                self.assertTrue(callers, observed["acquisition_stack"])
                self.assertTrue(any(frame["source_line"] == 56 for frame in callers), callers)
