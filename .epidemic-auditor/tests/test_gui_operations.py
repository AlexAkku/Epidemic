import json
import ctypes
from ctypes import wintypes
import os
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest
import uuid

from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.ui.main_window import AuditorWindow
from ep_auditor.core.lifecycle import authoritative_run
from ep_auditor.core.owned_process import OwnedProcess
from ep_auditor.collectors.etw_broker import BrokerClient
import test_core


class GuiOperationTests(unittest.TestCase):
    def test_gui_poll_timer_and_callback_errors_are_owned_until_destroy(self):
        for close_mode in ('close', 'destroy'):
            with self.subTest(close_mode=close_mode), tempfile.TemporaryDirectory() as temporary:
                config, _ = test_core.AuditorCoreTests._config(self, Path(temporary))
                storage, events = RunStorage(config), EventBus()
                manager = RunManager(config, storage, events)
                window = AuditorWindow(config, manager, storage, events)
                window.root.withdraw()
                python_errors = []
                window.root.report_callback_exception = lambda kind, error, trace: python_errors.append(str(error))
                window.root.tk.setvar('auditor_test_bg_errors', ())
                window.root.tk.call('proc', 'bgerror', 'message',
                                    'lappend ::auditor_test_bg_errors $message')

                def assert_callbacks_clean():
                    self.assertEqual(python_errors, [], 'Python callback failed')
                    self.assertEqual(window.root.tk.splitlist(
                        window.root.tk.getvar('auditor_test_bg_errors')), (), 'Tcl background callback failed')

                try:
                    # Prove the oracle catches both error paths that Tk otherwise
                    # prints without failing unittest.
                    window.root.after(0, lambda: 1 / 0)
                    window.root.update()
                    with self.assertRaises(AssertionError):
                        assert_callbacks_clean()
                    python_errors.clear()
                    window.root.tk.call('after', 0, 'auditor_missing_callback_negative_control')
                    window.root.update()
                    with self.assertRaises(AssertionError):
                        assert_callbacks_clean()
                    window.root.tk.setvar('auditor_test_bg_errors', ())

                    for _ in range(10):
                        window._poll_events()
                        self.assertEqual(len(window.root.tk.splitlist(
                            window.root.tk.call('after', 'info'))), 1, 'polling leaked a second timer')
                    if close_mode == 'destroy':
                        window.root.destroy()
                    else:
                        window._on_close()
                    window._on_close()  # Cleanup is idempotent after either path.
                    self.assertEqual(window.root.tk.splitlist(window.root.tk.call('after', 'info')), ())
                    window.root.tk.call('after', 150)
                    window.root.tk.call('update')
                    assert_callbacks_clean()
                finally:
                    window._on_close()

    def test_native_executable_opens_private_runtime_gui_and_closes_cleanly(self):
        root = Path(__file__).resolve().parents[2]
        executable = root / ".ai/tools/auditor/EpidemicAuditor.exe"
        self.assertTrue(executable.is_file(), "build and deploy the native application before qualification")
        BrokerClient()._normal_token()  # The application inherits a non-elevated token.
        user = ctypes.WinDLL("user32", use_last_error=True)
        callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
        user.EnumWindows.restype = wintypes.BOOL
        user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        user.GetWindowThreadProcessId.restype = wintypes.DWORD
        user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        user.GetWindowTextW.restype = ctypes.c_int
        user.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
        user.ShowWindow.restype = wintypes.BOOL
        user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        user.PostMessageW.restype = wintypes.BOOL
        with tempfile.TemporaryDirectory(dir=root / ".ai/tmp", prefix="native-gui-") as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            process = OwnedProcess([str(executable), "--config", str(config.config_path)],
                                   str(project), dict(os.environ), 0)
            windows = []

            @callback_type
            def observe(hwnd, _):
                pid = wintypes.DWORD()
                user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                if pid.value == process.pid:
                    title = ctypes.create_unicode_buffer(256)
                    user.GetWindowTextW(hwnd, title, len(title))
                    if title.value == "Epidemic Auditor":
                        user.ShowWindow(hwnd, 0)
                        windows.append(hwnd)
                return True

            try:
                deadline = time.monotonic() + 20
                while not windows and process.poll() is None and time.monotonic() < deadline:
                    self.assertTrue(user.EnumWindows(observe, 0))
                    time.sleep(0.05)
                self.assertTrue(windows, "native EXE did not create its actual GUI window")
                self.assertTrue(user.PostMessageW(windows[0], 0x0010, 0, 0))  # WM_CLOSE
                deadline = time.monotonic() + 10
                while process.poll() is None and time.monotonic() < deadline:
                    time.sleep(0.05)
                self.assertEqual(process.poll(), 0, "GUI failed to shut down its private runtime")
            finally:
                if process.poll() is None:
                    process.cancel("fixture_cleanup")
                process.close()

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
                window._on_close()
