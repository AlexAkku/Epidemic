import json
import os
from pathlib import Path
import sys
import tempfile
import threading
import unittest
import uuid

from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.ui.main_window import AuditorWindow
from ep_auditor.core.lifecycle import authoritative_run
import test_core


class GuiOperationTests(unittest.TestCase):
    def test_actual_widgets_share_backend_ids_group_labels_stop_and_shutdown(self):
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("gui_operations_" + uuid.uuid4().hex)
                base.mkdir(parents=True)
            config, project = test_core.AuditorCoreTests._config(self, base)
            config.data["cmake"]["configure_args"] = ["-A", "x64"]
            config.data["build_dir"] = str(Path(__file__).resolve().parents[2] / "build/ms62/gui" / uuid.uuid4().hex / "audit")
            script = project / "game.py"
            script.write_text("import pathlib,sys; pathlib.Path(sys.argv[1]).write_text('gui game effect'); print('gui game observed')\n")
            config.data["run"]["command"] = [sys.executable, str(script), str(project / "game.marker")]
            (project / "CMakeLists.txt").write_text('cmake_minimum_required(VERSION 3.20)\nproject(GuiOperations NONE)\nenable_testing()\n'
                'add_test(NAME Alpha COMMAND "${CMAKE_COMMAND}" -E echo alpha)\n'
                'set_tests_properties(Alpha PROPERTIES LABELS "alpha;subsystem")\n'
                'add_test(NAME Beta COMMAND "${CMAKE_COMMAND}" -E echo beta)\n')
            events = EventBus()
            storage = RunStorage(config)
            manager = RunManager(config, storage, events)
            window = AuditorWindow(config, manager, storage, events)
            window.root.withdraw()
            submitted, calls = [], []
            original_submit = manager.submit_operation

            def submit(*args, **kwargs):
                future = original_submit(*args, **kwargs)
                submitted.append(future)
                return future

            manager.submit_operation = submit

            def buttons(widget):
                result = {}
                for child in widget.winfo_children():
                    if child.winfo_class() == "TButton":
                        result[str(child.cget("text"))] = child
                    result.update(buttons(child))
                return result

            widgets = buttons(window.root)

            def invoke(label):
                widgets[label].invoke()
                self.assertIsNotNone(submitted[-1])
                result = submitted[-1].result(timeout=120)
                calls.append({"widget": label, **result})
                window._poll_events()
                window.root.update()
                return result

            try:
                self.assertTrue(hasattr(window, "group_selector"), "GUI has no explicit label/subsystem group control")
                for label in ("Configure", "Build", "Discover Tests"):
                    invoke(label)
                test_items = [item for group in window.test_tree.get_children()
                              for item in window.test_tree.get_children(group)]
                self.assertEqual(len(test_items), 2)
                window.test_tree.selection_set(test_items[0])
                self.assertEqual(invoke("Run Selected")["operation_id"], "run-one-test")
                window.test_tree.selection_set(test_items)
                self.assertEqual(invoke("Run Selected")["operation_id"], "run-selected-tests")
                self.assertIn("subsystem", tuple(window.group_selector["values"]))
                window.group_selection.set("subsystem")
                self.assertEqual(invoke("Run Group")["operation_id"], "run-group")
                for label in ("Run All Tests", "Start Game", "Build + Run"):
                    invoke(label)
                self.assertEqual((project / "game.marker").read_text(), "gui game effect")
                ready = threading.Event()
                original_emit = events.emit

                def emit(kind, **value):
                    original_emit(kind, **value)
                    if kind == "log" and value.get("text") == "gui game ready":
                        ready.set()

                events.emit = emit
                config.data["run"]["command"] = [sys.executable, "-c", "import time; print('gui game ready',flush=True); time.sleep(30)"]
                widgets["Start Game"].invoke()
                self.assertTrue(ready.wait(10))
                widgets["Stop"].invoke()
                stop = submitted[-1].result(timeout=10)
                calls.append({"widget": "Stop", **stop})
                ready.clear()
                widgets["Start Game"].invoke()
                self.assertTrue(ready.wait(10))
                window._on_close()
                shutdown = submitted[-1].result(timeout=10)
                calls.append({"widget": "WM_DELETE_WINDOW", **shutdown})
                for row in storage.list_runs():
                    header = authoritative_run(Path(row["run_dir"]))
                    if header["metadata"].get("backend_operation"):
                        self.assertEqual(header["metadata"]["backend_operation"]["entry_point"], "gui")
                (base / "gui-operations-oracle.json").write_text(json.dumps({"calls": calls, "runs_dir": str(config.runs_dir)}))
            finally:
                manager.shutdown()
                try:
                    window.root.destroy()
                except Exception:
                    pass
