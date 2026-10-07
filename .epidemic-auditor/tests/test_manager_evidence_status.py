import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.models import ProcessResult
import test_core


class ManagerEvidenceStatusTests(unittest.TestCase):
    def test_process_success_with_missing_evidence_emits_incomplete(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            manager = object.__new__(RunManager)
            manager.config = config
            manager.storage = RunStorage(config)
            manager.events = Mock()
            manager._log = Mock()
            process = ProcessResult(command=["controlled-no-capture-fixture"], cwd=str(config.project_root),
                                    exit_code=0, started_utc="2026-10-06T00:00:00Z",
                                    finished_utc="2026-10-06T00:00:01Z", wall_seconds=1)
            manager._run_process_operation("fixture", "no capture", "Fast", lambda *args: process)
            finished = [call.kwargs for call in manager.events.emit.call_args_list if call.args[0] == "run_finished"]
            self.assertEqual(len(finished), 1)
            self.assertEqual(finished[0]["status"], "INCOMPLETE")
            self.assertEqual(manager.storage.get_run(finished[0]["run_id"])["status"], "INCOMPLETE")

    def test_unqualified_build_does_not_start_game(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            manager = object.__new__(RunManager)
            manager.config = config
            manager.storage = RunStorage(config)
            manager.events = Mock()
            manager._log = Mock()
            manager._start_game = Mock()
            manager.builder = Mock()
            manager.builder.build.return_value = ProcessResult(command=["controlled-no-capture-fixture"],
                                                               cwd=str(config.project_root), exit_code=0,
                                                               started_utc="2026-10-06T00:00:00Z",
                                                               finished_utc="2026-10-06T00:00:01Z", wall_seconds=1)
            manager._build_and_run("Fast")
            manager._start_game.assert_not_called()
            finished = [call.kwargs for call in manager.events.emit.call_args_list if call.args[0] == "run_finished"]
            self.assertEqual(finished[0]["status"], "INCOMPLETE")
