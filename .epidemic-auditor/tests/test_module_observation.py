import ctypes
import json
import os
from pathlib import Path
import shutil
import stat
import tempfile
import unittest

from test_core import AuditorCoreTests
from ep_auditor.collectors.modules import identify_snapshot, observe
from ep_auditor.core.evidence import EvidenceError, RawEvidence
from ep_auditor.core.identity import function_identity, module_identity
from ep_auditor.core.storage import RunStorage
from ep_auditor.analyzers.summary import query_run_view
from ep_auditor.exporters.agent import export_investigation

ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT / "build/auditor-native/bin" / os.environ.get("EP_AUDITOR_NATIVE_CONFIG", "Release") / "EpidemicAuditorObservation.exe"


class ModuleObservationTests(unittest.TestCase):
    def test_binary_and_function_ids_are_rebase_stable_binary_scoped(self):
        sha = "a" * 64
        values = []
        for base in (0x1000, 0x70000000):
            snapshot = {"modules": [{"path": "fixture", "available": True, "base": base, "size": 0x1000}],
                        "symbol": {"function_address": base + 123, "raw_address": base + 130}}
            values.append(identify_snapshot(snapshot, {"fixture": {"sha256": sha}}))
        self.assertEqual(values[0]["symbol"]["functionId"], values[1]["symbol"]["functionId"])
        self.assertEqual(values[0]["symbol"]["function_rva"], 123)
        self.assertNotEqual(function_identity(sha, 123), function_identity("b" * 64, 123))
        self.assertNotEqual(function_identity(sha, 123), function_identity(sha, 124))
        for value in (None, "", "z" * 64):
            with self.assertRaises(EvidenceError):
                module_identity(value)
        with self.assertRaises(EvidenceError):
            function_identity(sha, True)
        missing = identify_snapshot(snapshot, {})
        self.assertIsNone(missing["symbol"]["functionId"])
        self.assertIsNone(missing["modules"][0]["moduleId"])
        self.assertEqual(snapshot["symbol"]["raw_address"], base + 130)

    def test_physical_owned_collector_preserves_unknown_symbol_and_binary_raw(self):
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.GetCurrentProcess.restype = ctypes.c_void_p
        kernel.GetProcessTimes.argtypes = [ctypes.c_void_p, *([ctypes.POINTER(ctypes.c_uint64)] * 4)]
        kernel.GetProcessTimes.restype = ctypes.c_int
        created, ended, cpu_kernel, cpu_user = (ctypes.c_uint64() for _ in range(4))
        self.assertTrue(kernel.GetProcessTimes(kernel.GetCurrentProcess(), ctypes.byref(created), ctypes.byref(ended),
                                              ctypes.byref(cpu_kernel), ctypes.byref(cpu_user)))
        with tempfile.TemporaryDirectory() as temporary:
            cfg, _ = AuditorCoreTests()._config(Path(temporary))
            storage = RunStorage(cfg)
            run = storage.new_run("modules", "external module fixture", "Fast")
            snapshot = observe(cfg, run.run_dir, os.getpid(), created.value, address=1, helper=HELPER)
            self.assertEqual(snapshot["symbol"]["raw_address"], 1)
            self.assertIsNone(snapshot["symbol"]["functionId"])
            known = [module for module in snapshot["modules"] if module["moduleId"]]
            self.assertTrue(known)
            payload, row = RawEvidence(run.run_dir).read_capture("modules/binary_files.json")
            self.assertTrue(row["complete"])
            self.assertEqual(len(json.loads(payload)["fingerprints"]), sum(bool(module.get("available")) for module in snapshot["modules"]))
            storage.finish_run(run, 'PASS')
            module_id = known[0]['moduleId']
            page = export_investigation(run.run_dir, 'module', module_id)
            self.assertTrue(page['items'])
            self.assertTrue(all(item['moduleId'] == module_id for item in page['items']))
            self.assertTrue(all(item['provenance'] == 'external' for item in page['items']))
            self.assertTrue(all(len(item['capture_refs']) == 2 for item in page['items']))
            missing = query_run_view(run.run_dir, 'Modules', scope='module:not-captured')
            self.assertEqual(missing['items'], [])
            shutil.copytree(run.run_dir, Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / self._testMethodName)
            captured_stdout = run.run_dir / 'raw/process/stdout.bin'
            captured_stdout.chmod(stat.S_IWRITE | stat.S_IREAD)
            captured_stdout.write_bytes(b'{}')
            with self.assertRaises(EvidenceError):
                export_investigation(run.run_dir, 'module', module_id)
            with self.assertRaises(EvidenceError):
                other = storage.new_run("modules", "PID reuse rejection", "Fast")
                observe(cfg, other.run_dir, os.getpid(), created.value + 1, helper=HELPER)
            shutil.copytree(other.run_dir, Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / "mismatched_creation")
