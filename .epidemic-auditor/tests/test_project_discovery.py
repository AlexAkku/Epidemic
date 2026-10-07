import os
import json
import tempfile
import unittest
import uuid
from pathlib import Path
from unittest.mock import patch

from ep_auditor.core.events import EventBus
from ep_auditor.core.evidence import EvidenceError, load_json
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.ui.main_window import AuditorWindow
import test_core


class ProjectDiscoveryTests(unittest.TestCase):
    def test_unique_nested_cmake_root_is_selected_and_reason_is_retained(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("project_discovery_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            source = project / "engine"
            source.mkdir()
            (source / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Selected LANGUAGES NONE)\n')
            storage = RunStorage(config)
            manager = RunManager(config, storage, EventBus())
            try:
                manager._run_process_operation("configure", "configure nested", "Fast", manager.builder.configure)
                run = next(row for row in storage.list_runs() if row["kind"] == "configure")
                self.assertEqual(run["status"], "PASS")
                selected = load_json(Path(run["run_dir"]) / "run.json")["metadata"]["project_selection"]
                self.assertEqual(selected["selected_root"], str(source))
                self.assertEqual(selected["reason"], "unique_cmake_source_root")
                self.assertEqual(config.project_root, source)
            finally:
                manager.shutdown()

    def test_ambiguous_roots_require_explicit_choice_without_launch(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("project_ambiguity_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            for name in ("first", "second"):
                source = project / name
                source.mkdir()
                (source / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(Choice LANGUAGES NONE)\n')
            with self.assertRaises(EvidenceError) as ambiguity:
                config.admit_project()
            self.assertIn("ambiguous CMake roots", str(ambiguity.exception))
            selection = config.admit_project(choice=project / "second")
            self.assertEqual(selection["reason"], "explicit_choice_from_ambiguous_roots")
            self.assertEqual(config.project_root, project / "second")
            with self.assertRaises(EvidenceError):
                config.admit_project(start=project, choice=project / "unobserved")
            self.assertEqual(config.project_root, project / "second")
            storage = RunStorage(config)
            self.assertEqual(storage.list_runs(), [])
            (base / "choice-oracle.json").write_text(json.dumps({"ambiguity_error": str(ambiguity.exception),
                                                                "choice": selection, "run_count": 0}))

    def test_gui_root_choice_uses_backend_admission(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            source = project / "engine"
            source.mkdir()
            (source / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(GuiRoot LANGUAGES NONE)\n')
            storage, events = RunStorage(config), EventBus()
            manager = RunManager(config, storage, events)
            window = AuditorWindow(config, manager, storage, events)
            window.root.withdraw()
            try:
                with patch("ep_auditor.ui.main_window.filedialog.askdirectory", return_value=str(source)):
                    window._choose_project()
                self.assertEqual(config.project_root, source)
                self.assertEqual(window.setting_vars["project_root"].get(), str(source))
                self.assertEqual(config.data["project_selection"]["selected_root"], str(source))
            finally:
                manager.shutdown()
                window.root.destroy()
