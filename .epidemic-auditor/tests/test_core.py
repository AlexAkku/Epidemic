from __future__ import annotations

import json
import os
import tempfile
import unittest
from pathlib import Path

from ep_auditor.config import load_config
from ep_auditor.analyzers.static_scan import scan_sources
from ep_auditor.analyzers.summary import aggregate_test_results
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import RawEvidence
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.tests import TestCatalog, TestExecutor


class AuditorCoreTests(unittest.TestCase):
    def _config(self, root: Path):
        project = root / "project"
        project.mkdir()
        auditor = project / ".epidemic-auditor"
        auditor.mkdir()
        cfg_path = auditor / "auditor.config.json"
        cfg_path.write_text(json.dumps({
            "project_root": "..",
            "build_dir": "../build/audit",
            "cmake": {"build_config": "Debug", "configure_args": [], "parallel": 0},
            "tests": {"timeout_seconds": 5, "ctest_args": []},
            "capture": {"sample_interval_ms": 50, "enable_wpr_in_deep": False, "wpr_profile": "GeneralProfile"},
            "run": {"command": [], "working_dir": "..", "environment": {}},
            "storage": {"runs_dir": "data/runs", "database": "data/auditor.sqlite3"},
            "static_scan": {
                "roots": ["../src"],
                "extensions": [".cpp", ".h"],
                "exclude": ["build", ".git", ".epidemic-auditor"]
            }
        }), encoding="utf-8")
        # Self-Test selects a persistent disposable capture directory. Save this
        # as an ordinary on-disk fixture configuration, never patch runtime paths.
        captures = os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE")
        if captures:
            settings = json.loads(cfg_path.read_text(encoding="utf-8"))
            settings["storage"]["runs_dir"] = str(Path(captures).resolve() / "captures" / root.name)
            cfg_path.write_text(json.dumps(settings), encoding="utf-8")
        return load_config(cfg_path), project

    def test_paths_are_relative_to_auditor(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, project = self._config(Path(td))
            self.assertEqual(cfg.project_root, project.resolve())
            self.assertEqual(cfg.build_dir, (project / "build" / "audit").resolve())

    def test_explicit_build_directory_placeholder_tracks_physical_run_profile(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, project = self._config(Path(td))
            cfg.data['run'] = {'command': ['{build_dir}/bin/Release/Game.exe', '--smoke'],
                               'working_dir': '{build_dir}/bin/Release',
                               'environment': {'BINARIES': '["{build_dir}/bin/Release/Game.exe"]'}}
            for mode, profile in [('Full', 'audit'), ('Deep', 'audit-deep'), ('Memory', 'audit-memory'), ('Coverage', 'audit-coverage')]:
                selected = cfg.for_mode(mode)
                executable = (project / 'build' / profile / 'bin/Release/Game.exe').resolve()
                self.assertEqual(Path(selected.run_command[0]), executable)
                self.assertEqual(selected.run_working_dir, executable.parent)
                self.assertEqual(Path(json.loads(selected.run_environment['BINARIES'])[0]), executable)
            self.assertIn('{build_dir}', cfg.data['run']['command'][0])

    def test_config_accepts_utf8_bom_without_changing_path_resolution(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, project = self._config(Path(td))
            text = cfg.config_path.read_text(encoding="utf-8")
            cfg.config_path.write_text(text, encoding="utf-8-sig")
            loaded = load_config(cfg.config_path)
            self.assertEqual(loaded.project_root, project.resolve())
            self.assertEqual(loaded.data, cfg.data)

    def test_run_storage_creates_agent_friendly_tree(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, _ = self._config(Path(td))
            storage = RunStorage(cfg)
            run = storage.new_run("test", "unit", "Fast")
            self.assertTrue((run.run_dir / "run.json").exists())
            self.assertTrue((run.run_dir / "environment.json").exists())
            self.assertTrue((run.run_dir / "agent").is_dir())
            RawEvidence(run.run_dir).append("fixture.bin", b"known", collector="controlled_fixture", version="1",
                                            start_ns=10, end_ns=20,
                                            health={"capacity": 64, "observed": 1, "dropped": 0, "incomplete_reason": None})
            storage.finish_run(run, "PASS", {"ok": True})
            row = storage.get_run(run.run_id)
            self.assertEqual(row["status"], "PASS")
            self.assertTrue((run.run_dir / "manifest.json").is_file())

    def test_static_scan(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, project = self._config(Path(td))
            src = project / "src"
            src.mkdir()
            (src / "a.h").write_text("#pragma once\n// TODO remove\n", encoding="utf-8")
            (src / "a.cpp").write_text('#include "a.h"\nint main(){return 0;}\n', encoding="utf-8")
            result = scan_sources(cfg)
            self.assertEqual(result["files"], 2)
            self.assertEqual(result["marker_count"], 1)
            self.assertTrue(any(e["target"] == "a.h" for e in result["include_edges"]))


    def test_managed_process_capture(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, _ = self._config(Path(td))
            storage = RunStorage(cfg)
            run = storage.new_run("process", "process smoke", "Fast")
            runner = ManagedProcessRunner(cfg)
            import sys
            result = runner.run([sys.executable, "-c", "import time; print('hello'); time.sleep(0.12)"], cfg.project_root, run.run_dir, "Fast")
            self.assertEqual(result.exit_code, 0)
            self.assertGreaterEqual(result.stdout_lines, 1)
            self.assertTrue((run.run_dir / "process_samples.jsonl").exists())
            self.assertTrue((run.run_dir / "console.jsonl").exists())

    def test_ctest_discovery_and_individual_execution(self):
        with tempfile.TemporaryDirectory() as td:
            cfg, project = self._config(Path(td))
            (project / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(AuditorSmoke LANGUAGES NONE)\n"
                "enable_testing()\n"
                "add_test(NAME Sample.Pass COMMAND ${CMAKE_COMMAND} -E echo pass)\n"
                "set_tests_properties(Sample.Pass PROPERTIES LABELS smoke)\n",
                encoding="utf-8",
            )
            import subprocess
            subprocess.run(["cmake", "-S", str(project), "-B", str(cfg.build_dir)], check=True, capture_output=True, text=True)
            catalog = TestCatalog(cfg)
            tests = catalog.discover()
            self.assertEqual(len(tests), 1)
            self.assertEqual(tests[0].name, "Sample.Pass")
            self.assertEqual(tests[0].group, "smoke")
            storage = RunStorage(cfg)
            parent = storage.new_run("tests", "ctest smoke", "Fast")
            executor = TestExecutor(cfg, ManagedProcessRunner(cfg), storage)
            result = executor.run_one(tests[0], parent.run_dir, "Fast")
            self.assertEqual(result.status, "PASS")
            self.assertEqual(result.run_id, parent.run_id)

    def test_test_summary(self):
        summary = aggregate_test_results([
            {"status": "PASS", "wall_seconds": 1.0, "peak_rss_bytes": 10},
            {"status": "FAIL", "wall_seconds": 2.0, "peak_rss_bytes": 20},
        ])
        self.assertEqual(summary["total"], 2)
        self.assertEqual(summary["passed"], 1)
        self.assertEqual(summary["failed"], 1)
        self.assertEqual(summary["peak_rss_bytes"], 20)


if __name__ == "__main__":
    unittest.main()
