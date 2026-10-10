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


class BuildDiagnosticsTests(unittest.TestCase):
    def test_actual_warning_syntax_link_and_missing_include_keep_all_diagnostics(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("build_diagnostics_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["cmake"].update({"parallel": 2, "configure_args": ["-A", "x64"]})
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/diagnostics" / uuid.uuid4().hex / "audit")
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Diagnostics LANGUAGES CXX)\n'
                'add_executable(Diagnostics main.cpp)\ntarget_compile_options(Diagnostics PRIVATE /W4)\n')
            source = project / "main.cpp"
            source.write_text('int main(){return 0;}\n')
            storage = RunStorage(config)
            builder = BuildController(config, ManagedProcessRunner(config))
            configured = storage.new_run("configure", "diagnostic fixture", "Fast")
            result = builder.configure(configured.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            storage.finish_run(configured, "PASS", {})
            cases = [("warning", 'int main(){int unused = 17;return 0;}\n', "warning", "C4189"),
                     ("syntax", 'int main( { broken; }\n', "error", None),
                     ("link", 'extern int missing_symbol();\nint main(){return missing_symbol();}\n', "error", "LNK2019"),
                     ("include", '#include "missing_controlled_header.h"\nint main(){return 0;}\n', "error", "C1083")]
            for title, body, severity, code in cases:
                source.write_text(body)
                run = storage.new_run("build", title, "Fast")
                result = builder.build(run.run_dir, "Fast")
                self.assertEqual(result.exit_code == 0, title == "warning")
                storage.finish_run(run, "PASS" if result.exit_code == 0 else "FAIL", {})
                lineage = EvidenceLineage(run.run_dir)
                rows = lineage.verify("normalized", "build_diagnostics.json")["records"]
                relevant = [row for row in rows if row["severity"] == severity]
                self.assertTrue(relevant)
                if code:
                    self.assertTrue(any(row["code"] == code for row in relevant))
                self.assertTrue(all(row["rawReference"]["layer"] == "raw" for row in rows))
                receipt = lineage.verify("normalized", "build_receipt.json")["records"][0]
                self.assertTrue(receipt["compiler_toolchains"])  # Failed builds retain their known compilation context.
                if severity == "error":
                    findings = lineage.verify("findings", "build_root_cause.json")["records"]
                    self.assertEqual(len(findings), 1)
                    self.assertEqual(findings[0]["diagnostic_id"], relevant[0]["diagnostic_id"])

    def test_actual_configure_failure_is_structured(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("configure_failure_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Failure LANGUAGES NONE)\nmessage(FATAL_ERROR "controlled configure root cause")\n')
            storage = RunStorage(config)
            run = storage.new_run("configure", "controlled configure failure", "Fast")
            result = BuildController(config, ManagedProcessRunner(config)).configure(run.run_dir, "Fast")
            self.assertNotEqual(result.exit_code, 0)
            storage.finish_run(run, "FAIL", {})
            rows = EvidenceLineage(run.run_dir).verify("normalized", "build_diagnostics.json")["records"]
            self.assertTrue(any(row["tool"] == "cmake" and "controlled configure root cause" in row["message"] for row in rows))
