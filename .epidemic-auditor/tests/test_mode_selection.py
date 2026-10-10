import tempfile
import os
import uuid
import unittest
from pathlib import Path

from ep_auditor.core.events import EventBus
from ep_auditor.core.evidence import EvidenceError, load_json
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.ui.main_window import AuditorWindow
import test_core


class ModeSelectionTests(unittest.TestCase):
    def test_gui_exposes_all_modes_from_backend_contract(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage, events = RunStorage(config), EventBus()
            manager = RunManager(config, storage, events)
            window = AuditorWindow(config, manager, storage, events)
            window.root.withdraw()
            try:
                def descendants(widget):
                    for child in widget.winfo_children():
                        yield child
                        yield from descendants(child)
                selector = next(widget for widget in descendants(window.root)
                                if widget.winfo_class() == "TCombobox" and "Fast" in widget["values"])
                self.assertEqual(tuple(selector["values"]),
                                 ("Fast", "Full", "Deep", "Memory", "Coverage", "Stress", "Targeted Exact"))
                self.assertEqual(tuple(selector["values"]), manager.available_modes)
                window.target_scope.set("module:controlled, function:controlled")
                self.assertEqual(config.data["capture"]["target_scope"], ["module:controlled", "function:controlled"])
            finally:
                window._on_close()

    def test_targeted_exact_requires_explicit_scope_and_persists_selection(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("mode_selection_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, _ = test_core.AuditorCoreTests._config(self, base)
            storage = RunStorage(config)
            with self.assertRaises(EvidenceError):
                storage.new_run("tests", "no target", "Targeted Exact")
            config.data["capture"]["target_scope"] = ["module:controlled"]
            run = storage.new_run("tests", "explicit target", "Targeted Exact")
            selection = load_json(run.run_dir / "run.json")["metadata"]["mode_selection"]
            self.assertEqual(selection["deep_subprofile"], "Targeted Exact")
            self.assertEqual(selection["target_scope"], ["module:controlled"])
            self.assertEqual(selection["instrumentation_profile"], "audit-deep")
            config.data["capture"]["target_scope"] = []
            self.assertEqual(load_json(run.run_dir / "run.json")["metadata"]["mode_selection"], selection)
            for mode in ("Fast", "Full", "Deep", "Memory", "Coverage", "Stress"):
                admitted = storage.new_run("tests", mode, mode)
                self.assertEqual(load_json(admitted.run_dir / "run.json")["metadata"]["mode_selection"]["mode"], mode)
            for scope in ([""], [7], "module:controlled", 7):
                with self.subTest(scope=scope), self.assertRaises(EvidenceError):
                    storage.new_run("tests", "invalid target", "Targeted Exact", target_scope=scope)
