import json
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
import uuid

from ep_auditor.core.events import EventBus
from ep_auditor.core.evidence import RawEvidence
from ep_auditor.core.lifecycle import authoritative_run
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
import test_core


class BuildRunStopTests(unittest.TestCase):
    def test_stop_during_build_preserves_cancelled_and_never_starts_game(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("build_run_stop_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            ready, game_marker = project / "ready.json", project / "game-started"
            blocker = project / "blocking_build.py"
            blocker.write_text("import json, os, time\nfrom pathlib import Path\n"
                "Path(__file__).with_name('ready.json').write_text(json.dumps({'pid':os.getpid()}))\n"
                "time.sleep(120)\n")
            game = project / "game.py"
            game.write_text("from pathlib import Path\nPath(__file__).with_name('game-started').touch()\n")
            (project / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\nproject(BuildRunStop NONE)\n'
                'add_custom_target(blocking ALL COMMAND "' + Path(sys.executable).as_posix()
                + '" "' + blocker.as_posix() + '" VERBATIM)\n')
            config.data["cmake"]["parallel"] = 1
            config.data["run"]["command"] = [sys.executable, str(game)]
            manager = RunManager(config, RunStorage(config), EventBus())
            try:
                manager.execute("configure", "Fast")
                future = manager.submit_operation("build-and-run", "Fast")
                deadline = time.monotonic() + 60
                while not ready.is_file() and time.monotonic() < deadline:
                    if future.done():
                        future.result()
                        self.fail("build completed before controlled blocker became ready")
                    time.sleep(0.05)
                self.assertTrue(ready.is_file(), "real build did not enter controlled blocking command")
                import psutil
                helper = psutil.Process(json.loads(ready.read_text())["pid"])
                helper_identity = {"pid": helper.pid, "creation_time": helper.create_time(), "image": helper.exe()}
                manager.stop()
                future.result(timeout=30)
                runs = [authoritative_run(directory) for directory in config.runs_dir.iterdir()]
                builds = [run for run in runs if run["kind"] == "build"]
                self.assertEqual(len(builds), 1)
                directory = Path(builds[0]["run_dir"])
                lifecycle = json.loads(RawEvidence(directory).read_bytes("process/lifecycle.json"))
                self.assertTrue(lifecycle["cancelled"])
                self.assertEqual(lifecycle["termination"]["reason"], "user_stop")
                self.assertEqual(lifecycle["termination"]["purpose"], "cancellation")
                self.assertFalse(game_marker.exists())
                self.assertFalse(any(run["kind"] == "game" for run in runs))
                self.assertFalse(psutil.pid_exists(helper_identity["pid"]), "owned blocking helper survived Stop")
                (base / "build-run-stop-oracle.json").write_text(json.dumps({
                    "run_dir": str(directory), "helper": helper_identity, "game_marker": str(game_marker)}))
                self.assertEqual(builds[0]["status"], "CANCELLED",
                    "Build + Run mislabeled externally observed user Stop as build FAIL")
            finally:
                manager.shutdown()
