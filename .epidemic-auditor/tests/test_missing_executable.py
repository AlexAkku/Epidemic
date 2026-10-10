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
from ep_auditor.core.evidence import RawEvidence
import test_core


class MissingExecutableTests(unittest.TestCase):
    def test_missing_game_is_committed_failure_with_original_admission_error(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("missing_executable_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            (project / "CMakeLists.txt").write_text("cmake_minimum_required(VERSION 3.20)\nproject(MissingExecutable NONE)\n")
            missing = project / "absent-game.exe"
            self.assertFalse(missing.exists())
            config.data["run"]["command"] = [str(missing)]
            storage = RunStorage(config)
            manager = RunManager(config, storage, EventBus())
            try:
                with self.assertRaises(OSError):
                    manager.execute("start-game", "Fast")
                directories = list(config.runs_dir.iterdir())
                self.assertEqual(len(directories), 1)
                run = authoritative_run(directories[0])
                self.assertEqual(run["status"], "FAIL", "missing executable left an unqualified INCOMPLETE transaction")
                error = json.loads(RawEvidence(directories[0]).read_bytes("process/launch_failure.json"))
                self.assertEqual(error["command"], [str(missing)])
                self.assertEqual(error["winerror"], 2)
                self.assertIsNone(error["pid"])
                self.assertIsNone(error["exit_code"])
                self.assertEqual(error["provenance"], "external")
                self.assertEqual(error["confidence"], "observed")
                (base / "missing-executable-oracle.json").write_text(json.dumps({"missing": str(missing),
                    "run_dir": str(directories[0]), "expected_winerror": 2}))
            finally:
                manager.shutdown()
