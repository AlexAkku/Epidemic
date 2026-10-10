import sys
import os
import uuid
import tempfile
import unittest
from pathlib import Path

from ep_auditor.core.build_diagnostics import record_diagnostics
from ep_auditor.core.lifecycle import seal_run_capture
from ep_auditor.core.lineage import EvidenceLineage
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
import test_core


class DiagnosticOrderTests(unittest.TestCase):
    def test_first_actionable_is_observed_across_stderr_and_stdout(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("diagnostic_order_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            storage = RunStorage(config)
            run = storage.new_run("fixture", "controlled diagnostic chronology", "Fast")
            command = [sys.executable, "-c", 'import sys,time;sys.stderr.write("fixture.cpp(1): error C1189: first\\n");sys.stderr.flush();time.sleep(0.3);sys.stdout.write("fixture.cpp(2): error C2143: second\\n");sys.stdout.flush();sys.exit(1)']
            result = ManagedProcessRunner(config).run(command, project, run.run_dir, "Fast")
            self.assertEqual(result.exit_code, 1)
            seal_run_capture(run)
            record_diagnostics(run.run_dir, "build", result.exit_code)
            lineage = EvidenceLineage(run.run_dir)
            finding = lineage.verify("findings", "build_root_cause.json")["records"][0]
            rows = lineage.verify("normalized", "build_diagnostics.json")["records"]
            first = next(row for row in rows if row["code"] == "C1189")
            self.assertEqual(finding["diagnostic_id"], first["diagnostic_id"])
