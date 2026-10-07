import os
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.build import BuildController
from ep_auditor.core.evidence import load_json
from ep_auditor.core.lineage import EvidenceLineage
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
import test_core


class BuildReceiptTests(unittest.TestCase):
    def test_actual_cpp_configure_build_flags_compiler_and_timing_receipts(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("build_receipt_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["cmake"]["parallel"] = 2
            config.data["cmake"]["configure_args"] = ["-A", "x64"]
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/receipt" / uuid.uuid4().hex / "audit")
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Receipt LANGUAGES CXX)\n'
                'add_executable(Receipt main.cpp)\ntarget_compile_definitions(Receipt PRIVATE BUILD_RECEIPT_FIXTURE=17)\n'
                'if(MSVC)\ntarget_compile_options(Receipt PRIVATE /W4 /Od)\nendif()\n')
            (project / "main.cpp").write_text('static_assert(BUILD_RECEIPT_FIXTURE == 17);\nint main(){return 0;}\n')
            storage = RunStorage(config)
            builder = BuildController(config, ManagedProcessRunner(config))
            receipts = []
            for operation in ("configure", "build"):
                run = storage.new_run(operation, operation, "Fast")
                result = getattr(builder, operation)(run.run_dir, "Fast")
                self.assertEqual(result.exit_code, 0)
                storage.finish_run(run, "PASS", {"process": result.to_dict()})
                envelope = EvidenceLineage(run.run_dir).verify("normalized", "build_receipt.json")
                receipt = envelope["records"][0]
                self.assertEqual(receipt["operation"], operation)
                self.assertEqual(receipt["command"], result.command)
                self.assertEqual(receipt["run_id"], run.run_id)
                self.assertTrue(receipt["build_id"].startswith("BuildId:"))
                self.assertEqual(receipt["instrumentation_profile"], "audit")
                self.assertEqual(receipt["requested_parallelism"], 2)
                self.assertTrue(receipt["compiler_toolchains"])
                self.assertTrue(Path(receipt["compiler_toolchains"][0]["compiler"]["path"]).is_file())
                self.assertTrue(receipt["compiler_toolchains"][0]["compiler"]["version"])
                target = next(row for row in receipt["targets"] if row["name"] == "Receipt")
                self.assertIn("BUILD_RECEIPT_FIXTURE=17", target["defines"])
                self.assertTrue(any("/W4" in flag for flag in target["compile_flags"]))
                self.assertTrue(any("/Od" in flag for flag in target["compile_flags"]))
                self.assertGreaterEqual(receipt["timings"]["operation"]["seconds"], 0)
                for phase in ("compile", "link"):
                    timing = receipt["timings"][phase]
                    if timing["availability"] == "observed":
                        self.assertGreaterEqual(timing["seconds"], 0)
                    else:
                        self.assertIsNone(timing["seconds"])
                        self.assertTrue(timing["reason"])
                receipts.append(receipt)
            self.assertNotEqual(receipts[0]["build_id"], receipts[1]["build_id"])
            self.assertEqual(receipts[1]["configure_command"], receipts[0]["command"])
            self.assertEqual(receipts[1]["timings"]["compile"]["availability"], "observed")
            self.assertEqual(receipts[1]["timings"]["link"]["availability"], "observed")
