import ctypes
from ctypes import wintypes as w
import json
import os
import tempfile
import sys
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.build import BuildController
from ep_auditor.core.lineage import EvidenceLineage
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.owned_process import OwnedProcess
import test_core


class PdbParallelTests(unittest.TestCase):
    def test_live_kernel_membership_and_unavailable_observation_are_distinct(self):
        process = OwnedProcess([sys.executable, "-c", "import time; time.sleep(30)"], str(Path.cwd()), dict(os.environ))
        try:
            observation = process.observe_members()
            self.assertEqual(observation["availability"], "observed")
            self.assertEqual(len(observation["members"]), 1)
            member = observation["members"][0]
            self.assertEqual(member["pid"], process.pid)
            self.assertEqual(Path(member["image_path"]).resolve(), Path(sys.executable).resolve())
            self.assertEqual(member["creation_filetime_100ns"], process.creation_filetime_100ns)
            self.assertLessEqual(observation["start_perf_ns"], observation["end_perf_ns"])
        finally:
            process.cancel("fixture_complete")
            process.close()
        self.assertEqual(process.observe_members()["availability"], "unavailable")

    def test_real_locked_compile_pdb_failure_two_parallelisms_and_unlocked_effect(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("pdb_parallel_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["cmake"]["configure_args"] = ["-A", "x64"]
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/pdb" / uuid.uuid4().hex / "audit")
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(PdbFixture LANGUAGES CXX)\n'
                'add_executable(PdbFixture main.cpp)\nset_target_properties(PdbFixture PROPERTIES COMPILE_PDB_NAME SharedCompile '
                'COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb")\n')
            (project / "main.cpp").write_text('int main(){return 0;}\n')
            storage = RunStorage(config)
            builder = BuildController(config, ManagedProcessRunner(config))
            configured = storage.new_run("configure", "PDB configure", "Fast")
            result = builder.configure(configured.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(configured, "PASS", {})
            pdb = config.build_dir / "pdb/Debug/SharedCompile.pdb"
            pdb.parent.mkdir(parents=True, exist_ok=True)
            kernel = ctypes.WinDLL("kernel32", use_last_error=True)
            kernel.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, ctypes.c_void_p, w.DWORD, w.DWORD, w.HANDLE]
            kernel.CreateFileW.restype = w.HANDLE
            kernel.CloseHandle.argtypes, kernel.CloseHandle.restype = [w.HANDLE], w.BOOL
            handle = kernel.CreateFileW(str(pdb), 0xC0000000, 0, None, 2, 0x80, None)
            if handle == ctypes.c_void_p(-1).value:
                raise ctypes.WinError(ctypes.get_last_error())
            attempts = []
            try:
                for parallelism in (1, 2):
                    config.data["cmake"]["parallel"] = parallelism
                    run = storage.new_run("build", "locked PDB parallel " + str(parallelism), "Fast")
                    result = builder.build(run.run_dir, "Fast")
                    self.assertNotEqual(result.exit_code, 0)
                    storage.finish_run(run, "FAIL", {})
                    lineage = EvidenceLineage(run.run_dir)
                    diagnostics = lineage.verify("normalized", "build_diagnostics.json")["records"]
                    self.assertTrue(any(row["code"] in {"C1090", "C1041", "C1033"} for row in diagnostics))
                    receipt = lineage.verify("normalized", "build_receipt.json")["records"][0]
                    observations = receipt["compiler_observations_at_failure"]
                    self.assertTrue(observations)
                    self.assertTrue(all(row["availability"] == "observed" and row["compiler_process_count"] >= 0 for row in observations))
                    self.assertEqual(receipt["requested_parallelism"], parallelism)
                    attempts.append({"run_id": run.run_id, "build_id": receipt["build_id"], "parallelism": parallelism})
            finally:
                kernel.CloseHandle(handle)
            # The fixture owns this precreated empty lock target; restore the absent-file control.
            pdb.unlink()
            positive = storage.new_run("build", "unlocked PDB positive", "Fast")
            result = builder.build(positive.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(positive, "PASS", {})
            self.assertTrue(pdb.is_file())
            summary = EvidenceLineage(positive.run_dir).verify("normalized", "build_diagnostics_summary.json")["records"][0]
            self.assertEqual(summary["diagnostic_count"], 0)
            self.assertEqual(summary["observed_exit_code"], 0)
            self.assertEqual(len({row["run_id"] for row in attempts}), 2)
            self.assertEqual(len({row["build_id"] for row in attempts}), 2)
            (base / "pdb-oracle.json").write_text(json.dumps({"attempts": attempts, "positive_run_id": positive.run_id,
                "lock_target": str(pdb), "fault": "CreateFileW exclusive sharing on fixture-owned empty compile PDB"}))
