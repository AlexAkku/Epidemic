import json
import os
from pathlib import Path
import tempfile
import unittest
import uuid

from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.lifecycle import authoritative_run
from ep_auditor.core.lineage import EvidenceLineage
import test_core


class ReproductionMatrixTests(unittest.TestCase):
    def test_backend_automatically_runs_two_parallelisms_as_independent_attempts(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("matrix_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/matrix" / uuid.uuid4().hex / "audit")
            config.data["cmake"]["configure_args"] = ["-A", "x64"]
            config.data["cmake"]["reproduction_parallelisms"] = [1, 2]
            (project / "CMakeLists.txt").write_text("cmake_minimum_required(VERSION 3.20)\nproject(Matrix CXX)\nadd_executable(Matrix main.cpp)\n")
            (project / "main.cpp").write_text("int main(){ unresolved_identifier; }\n")
            storage = RunStorage(config)
            manager = RunManager(config, storage, EventBus())
            try:
                manager.execute("configure", "Fast")
                for invalid in ([], [1], [1, 1], [True, 2], [1, 11]):
                    config.data["cmake"]["reproduction_parallelisms"] = invalid
                    with self.assertRaises(ValueError):
                        manager.execute("reproduction-matrix", "Fast")
                    self.assertFalse(manager.busy)
                config.data["cmake"]["reproduction_parallelisms"] = [1, 2]
                request = manager.execute("reproduction-matrix", "Fast")
                attempts = []
                for directory in config.runs_dir.iterdir():
                    run = authoritative_run(directory)
                    if run["kind"] != "build":
                        continue
                    self.assertEqual(run["status"], "FAIL")
                    receipt = EvidenceLineage(directory).verify("normalized", "build_receipt.json")["records"][0]
                    self.assertEqual(run["metadata"]["backend_operation"]["invocation_id"], request["invocation_id"])
                    self.assertEqual(run["metadata"]["backend_operation"]["operation_id"], "reproduction-matrix")
                    attempts.append({"run_dir": str(directory), "run_id": run["run_id"],
                                     "build_id": receipt["build_id"], "parallelism": receipt["requested_parallelism"]})
                self.assertEqual(sorted(row["parallelism"] for row in attempts), [1, 2])
                self.assertEqual(len({row["run_id"] for row in attempts}), 2)
                self.assertEqual(len({row["build_id"] for row in attempts}), 2)
                (base / "matrix-oracle.json").write_text(json.dumps({"request": request, "attempts": attempts}))
            finally:
                manager.shutdown()
