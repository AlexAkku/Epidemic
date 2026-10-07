from __future__ import annotations

import json
import os
import subprocess
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from ep_auditor.config import AuditorConfig
from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage
from ep_auditor.models import TestCase


class AuditorWindow:
    def __init__(self, config: AuditorConfig, manager: RunManager, storage: RunStorage, events: EventBus) -> None:
        self.config = config
        self.manager = manager
        self.storage = storage
        self.events = events
        self.tests: list[TestCase] = []
        self.root = tk.Tk()
        self.root.title("Epidemic Auditor")
        self.root.geometry("1380x900")
        self.root.minsize(1050, 700)
        self.capture_mode = tk.StringVar(value="Fast")
        self.target_scope = tk.StringVar(value=", ".join(config.data.get("capture", {}).get("target_scope", [])))
        self.target_scope.trace_add("write", self._target_scope_changed)
        self.status_text = tk.StringVar(value="Ready")
        self.progress_text = tk.StringVar(value="")
        self._build_ui()
        self._refresh_runs()
        self.root.after(100, self._poll_events)
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    def run(self) -> None:
        self.root.mainloop()

    def _target_scope_changed(self, *_args) -> None:
        self.config.data.setdefault("capture", {})["target_scope"] = [
            value.strip() for value in self.target_scope.get().split(",") if value.strip()]

    def _build_ui(self) -> None:
        top = ttk.Frame(self.root, padding=8)
        top.pack(fill=tk.X)
        self._button(top, "Configure", lambda: self.manager.configure_async(self.capture_mode.get())).pack(side=tk.LEFT, padx=2)
        self._button(top, "Build", lambda: self.manager.build_async(self.capture_mode.get())).pack(side=tk.LEFT, padx=2)
        self._button(top, "Discover Tests", lambda: self.manager.discover_tests_async(self.capture_mode.get())).pack(side=tk.LEFT, padx=2)
        self._button(top, "Run All Tests", lambda: self.manager.run_all_tests_async(self.capture_mode.get())).pack(side=tk.LEFT, padx=2)
        self._button(top, "Run Selected", self._run_selected).pack(side=tk.LEFT, padx=2)
        self._button(top, "Build + Run", lambda: self.manager.build_and_run_async(self.capture_mode.get())).pack(side=tk.LEFT, padx=2)
        self._button(top, "Start Game", lambda: self.manager.start_game_async(self.capture_mode.get())).pack(side=tk.LEFT, padx=2)
        self._button(top, "Static Scan", self.manager.static_scan_async).pack(side=tk.LEFT, padx=2)
        ttk.Separator(top, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=8)
        ttk.Label(top, text="Capture:").pack(side=tk.LEFT)
        self.mode_selector = ttk.Combobox(top, textvariable=self.capture_mode, state="readonly",
                                         values=self.manager.available_modes, width=14)
        self.mode_selector.pack(side=tk.LEFT, padx=4)
        ttk.Label(top, text="Exact targets:").pack(side=tk.LEFT)
        ttk.Entry(top, textvariable=self.target_scope, width=20).pack(side=tk.LEFT, padx=4)
        self.stop_button = ttk.Button(top, text="Stop", command=self.manager.stop)
        self.stop_button.pack(side=tk.RIGHT, padx=2)

        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))
        self.tests_tab = ttk.Frame(self.notebook)
        self.dashboard_tab = ttk.Frame(self.notebook)
        self.runs_tab = ttk.Frame(self.notebook)
        self.console_tab = ttk.Frame(self.notebook)
        self.settings_tab = ttk.Frame(self.notebook)
        self.notebook.add(self.tests_tab, text="Tests")
        self.notebook.add(self.dashboard_tab, text="Dashboard")
        self.notebook.add(self.runs_tab, text="Runs")
        self.notebook.add(self.console_tab, text="Console")
        self.notebook.add(self.settings_tab, text="Settings")

        self._build_tests_tab()
        self._build_dashboard_tab()
        self._build_runs_tab()
        self._build_console_tab()
        self._build_settings_tab()

        status = ttk.Frame(self.root, padding=(8, 2, 8, 6))
        status.pack(fill=tk.X)
        ttk.Label(status, textvariable=self.status_text).pack(side=tk.LEFT)
        ttk.Label(status, textvariable=self.progress_text).pack(side=tk.RIGHT)

    @staticmethod
    def _button(parent, text, command):
        return ttk.Button(parent, text=text, command=command)

    def _build_tests_tab(self) -> None:
        bar = ttk.Frame(self.tests_tab, padding=6)
        bar.pack(fill=tk.X)
        ttk.Label(bar, text="Filter:").pack(side=tk.LEFT)
        self.test_filter = tk.StringVar()
        entry = ttk.Entry(bar, textvariable=self.test_filter, width=40)
        entry.pack(side=tk.LEFT, padx=4)
        entry.bind("<KeyRelease>", lambda _e: self._render_tests())
        ttk.Label(bar, text="Use Ctrl/Shift to select tests and groups. A selected group runs all visible tests inside it.").pack(side=tk.LEFT, padx=12)

        group_bar = ttk.Frame(self.tests_tab, padding=(6, 0, 6, 6))
        group_bar.pack(fill=tk.X)
        ttk.Label(group_bar, text="Label/subsystem group:").pack(side=tk.LEFT)
        self.group_selection = tk.StringVar()
        self.group_selector = ttk.Combobox(group_bar, textvariable=self.group_selection, state="readonly", width=36)
        self.group_selector.pack(side=tk.LEFT, padx=4)
        self._button(group_bar, "Run Group", self._run_group).pack(side=tk.LEFT, padx=4)

        columns = ("group", "labels", "timeout")
        self.test_tree = ttk.Treeview(self.tests_tab, columns=columns, show="tree headings", selectmode="extended")
        self.test_tree.heading("#0", text="Test")
        self.test_tree.heading("group", text="Group")
        self.test_tree.heading("labels", text="Labels")
        self.test_tree.heading("timeout", text="Timeout")
        self.test_tree.column("#0", width=520)
        self.test_tree.column("group", width=180)
        self.test_tree.column("labels", width=300)
        self.test_tree.column("timeout", width=100, anchor=tk.E)
        scroll = ttk.Scrollbar(self.tests_tab, orient=tk.VERTICAL, command=self.test_tree.yview)
        self.test_tree.configure(yscrollcommand=scroll.set)
        self.test_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(6,0), pady=(0,6))
        scroll.pack(side=tk.RIGHT, fill=tk.Y, padx=(0,6), pady=(0,6))

    def _build_dashboard_tab(self) -> None:
        frame = ttk.Frame(self.dashboard_tab, padding=10)
        frame.pack(fill=tk.BOTH, expand=True)
        self.dashboard_text = tk.Text(frame, height=14, wrap="word")
        self.dashboard_text.pack(fill=tk.X)
        self.chart = tk.Canvas(frame, background="white", highlightthickness=1, highlightbackground="#999", height=430)
        self.chart.pack(fill=tk.BOTH, expand=True, pady=(10,0))

    def _build_runs_tab(self) -> None:
        bar = ttk.Frame(self.runs_tab, padding=6)
        bar.pack(fill=tk.X)
        ttk.Button(bar, text="Refresh", command=self._refresh_runs).pack(side=tk.LEFT)
        ttk.Button(bar, text="Open Folder", command=self._open_selected_run).pack(side=tk.LEFT, padx=4)
        ttk.Button(bar, text="Load Dashboard", command=self._load_selected_run).pack(side=tk.LEFT, padx=4)
        columns = ("kind", "status", "mode", "started", "title")
        self.runs_tree = ttk.Treeview(self.runs_tab, columns=columns, show="tree headings", selectmode="browse")
        self.runs_tree.heading("#0", text="Run ID")
        for col, title, width in [("kind","Kind",90),("status","Status",90),("mode","Mode",80),("started","Started UTC",210),("title","Title",440)]:
            self.runs_tree.heading(col, text=title)
            self.runs_tree.column(col, width=width)
        self.runs_tree.pack(fill=tk.BOTH, expand=True, padx=6, pady=(0,6))
        self.runs_tree.bind("<Double-1>", lambda _e: self._load_selected_run())

    def _build_console_tab(self) -> None:
        self.console = tk.Text(self.console_tab, wrap="none", background="#111", foreground="#ddd", insertbackground="white")
        self.console.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)
        self.console.tag_configure("stderr", foreground="#ff8080")
        self.console.tag_configure("info", foreground="#80c0ff")

    def _build_settings_tab(self) -> None:
        frm = ttk.Frame(self.settings_tab, padding=12)
        frm.pack(fill=tk.BOTH, expand=True)
        self.setting_vars = {
            "project_root": tk.StringVar(value=str(self.config.data.get("project_root", ".."))),
            "build_dir": tk.StringVar(value=str(self.config.data.get("build_dir", "../build/audit"))),
            "build_config": tk.StringVar(value=self.config.build_config),
            "run_command": tk.StringVar(value=json.dumps(self.config.run_command, ensure_ascii=False)),
            "run_working_dir": tk.StringVar(value=str(self.config.data.get("run", {}).get("working_dir", ".."))),
        }
        rows = [
            ("Project root", "project_root"),
            ("Build directory", "build_dir"),
            ("Build config", "build_config"),
            ("Run command JSON array", "run_command"),
            ("Run working directory", "run_working_dir"),
        ]
        for r, (label, key) in enumerate(rows):
            ttk.Label(frm, text=label).grid(row=r, column=0, sticky="w", pady=5)
            ttk.Entry(frm, textvariable=self.setting_vars[key], width=110).grid(row=r, column=1, sticky="ew", pady=5, padx=8)
            if key == "project_root":
                ttk.Button(frm, text="Choose CMake root", command=self._choose_project).grid(row=r, column=2, padx=4)
        frm.columnconfigure(1, weight=1)
        ttk.Button(frm, text="Save Settings", command=self._save_settings).grid(row=len(rows), column=1, sticky="e", pady=12)
        ttk.Label(frm, text=f"Config file: {self.config.config_path}").grid(row=len(rows)+1, column=0, columnspan=2, sticky="w")

    def _save_settings(self) -> None:
        try:
            if self.manager.busy:
                raise RuntimeError("Wait for the active operation before changing settings")
            self.config.data["project_root"] = self.setting_vars["project_root"].get().strip()
            self.config.data["build_dir"] = self.setting_vars["build_dir"].get().strip()
            self.config.data.setdefault("cmake", {})["build_config"] = self.setting_vars["build_config"].get().strip()
            self.config.data.setdefault("run", {})["command"] = json.loads(self.setting_vars["run_command"].get())
            self.config.data.setdefault("run", {})["working_dir"] = self.setting_vars["run_working_dir"].get().strip()
            self.config.save()
            messagebox.showinfo("Epidemic Auditor", "Settings saved. Existing controllers read the same config object, so changes apply to future runs.")
        except Exception as exc:
            messagebox.showerror("Settings error", str(exc))

    def _choose_project(self) -> None:
        if self.manager.busy:
            messagebox.showerror("Project selection", "Wait for the active operation before selecting a project")
            return
        selected = filedialog.askdirectory(title="Choose CMake source root")
        if not selected:
            return
        try:
            decision = self.config.admit_project(start=Path(selected))
            self.setting_vars["project_root"].set(decision["selected_root"])
            self.status_text.set("Project selected: " + decision["reason"])
        except Exception as error:
            messagebox.showerror("Project selection", str(error))

    def _run_selected(self) -> None:
        selected_names: set[str] = set()
        for iid in self.test_tree.selection():
            if iid.startswith("test:"):
                selected_names.add(self.test_tree.item(iid, "text"))
            elif iid.startswith("group:"):
                for child in self.test_tree.get_children(iid):
                    selected_names.add(self.test_tree.item(child, "text"))
        selected = [t for t in self.tests if t.name in selected_names]
        if not selected:
            messagebox.showwarning("Epidemic Auditor", "Select one or more tests or groups first.")
            return
        self.manager.run_selected_tests_async(selected, self.capture_mode.get())

    def _run_group(self) -> None:
        label = self.group_selection.get()
        if not label:
            messagebox.showwarning("Epidemic Auditor", "Select a label or subsystem group first.")
            return
        self.manager.run_group_async([label], self.capture_mode.get())

    def _render_tests(self) -> None:
        labels = sorted({value for test in self.tests for value in [test.group, *test.labels]})
        self.group_selector["values"] = labels
        if self.group_selection.get() not in labels:
            self.group_selection.set("")
        for iid in self.test_tree.get_children():
            self.test_tree.delete(iid)
        needle = self.test_filter.get().strip().lower()
        groups: dict[str, str] = {}
        for test in self.tests:
            hay = " ".join([test.name, test.group, *test.labels]).lower()
            if needle and needle not in hay:
                continue
            group_iid = groups.get(test.group)
            if not group_iid:
                group_iid = f"group:{len(groups)}"
                groups[test.group] = group_iid
                self.test_tree.insert("", "end", iid=group_iid, text=test.group, values=(test.group, "", ""), open=True)
            iid = "test:" + str(test.index or len(groups)) + ":" + str(abs(hash(test.name)))
            self.test_tree.insert(group_iid, "end", iid=iid, text=test.name, values=(test.group, ", ".join(test.labels), test.timeout or ""))

    def _refresh_runs(self) -> None:
        for iid in self.runs_tree.get_children():
            self.runs_tree.delete(iid)
        for row in self.storage.list_runs():
            self.runs_tree.insert("", "end", iid=row["run_id"], text=row["run_id"], values=(row["kind"], row["status"], row["capture_mode"], row["started_utc"], row["title"]))

    def _selected_run_id(self) -> str | None:
        sel = self.runs_tree.selection()
        return sel[0] if sel else None

    def _open_selected_run(self) -> None:
        run_id = self._selected_run_id()
        if not run_id:
            return
        row = self.storage.get_run(run_id)
        if not row:
            return
        path = Path(row["run_dir"])
        try:
            if os.name == "nt":
                os.startfile(path)  # type: ignore[attr-defined]
            else:
                subprocess.Popen(["xdg-open", str(path)])
        except Exception as exc:
            messagebox.showerror("Open folder", str(exc))

    def _load_selected_run(self) -> None:
        run_id = self._selected_run_id()
        if not run_id:
            return
        row = self.storage.get_run(run_id)
        if not row:
            return
        run_dir = Path(row["run_dir"])
        summary = self.storage.read_json(run_dir / "metrics" / "summary.json", {})
        self._show_dashboard(run_id, row, summary, run_dir)
        self.notebook.select(self.dashboard_tab)

    def _show_dashboard(self, run_id: str, row: dict, summary: dict, run_dir: Path) -> None:
        self.dashboard_text.delete("1.0", tk.END)
        proc = summary.get("process", {})
        tests = summary.get("tests", {})
        static = summary.get("static_scan", {})
        lines = [
            f"Run: {run_id}",
            f"Status: {row.get('status')}",
            f"Kind: {row.get('kind')}",
            f"Capture: {row.get('capture_mode')}",
        ]
        if proc:
            lines += [
                f"Wall: {proc.get('wall_seconds', 0):.3f} s",
                f"Peak RSS: {self._human_bytes(proc.get('peak_rss_bytes', 0))}",
                f"Peak threads: {proc.get('peak_threads', 0)}",
                f"I/O read/write: {self._human_bytes(proc.get('total_read_bytes', 0))} / {self._human_bytes(proc.get('total_write_bytes', 0))}",
            ]
        if tests:
            lines += [f"Tests: {tests.get('passed', 0)} pass, {tests.get('failed', 0)} fail, {tests.get('cancelled', 0)} cancelled / {tests.get('total', 0)} total"]
        if static:
            lines += [f"Static scan: {static.get('files', 0)} files, {static.get('lines', 0)} lines, {static.get('marker_count', 0)} markers"]
        self.dashboard_text.insert(tk.END, "\n".join(lines))
        sample_path = run_dir / "process_samples.jsonl"
        if sample_path.exists():
            self._draw_samples(sample_path)
        else:
            self._draw_test_durations(run_dir / "tests" / "results.jsonl")


    def _draw_test_durations(self, path: Path) -> None:
        self.chart.delete("all")
        if not path.exists():
            self.chart.create_text(20, 20, anchor="nw", text="No timeline data for this run")
            return
        rows = []
        with path.open("r", encoding="utf-8") as f:
            for line in f:
                if line.strip():
                    rows.append(json.loads(line))
        rows.sort(key=lambda x: float(x.get("wall_seconds", 0)), reverse=True)
        rows = rows[:20]
        self.chart.update_idletasks()
        width = max(700, self.chart.winfo_width())
        left = 330
        top = 35
        row_h = 19
        max_v = max([float(x.get("wall_seconds", 0)) for x in rows] + [1.0])
        self.chart.create_text(12, 10, anchor="nw", text="Slowest tests, seconds")
        for i, row in enumerate(rows):
            y = top + i * row_h
            value = float(row.get("wall_seconds", 0))
            name = str(row.get("test_name", ""))
            self.chart.create_text(12, y, anchor="nw", text=name[:48])
            bar_w = max(1, (value / max_v) * max(100, width - left - 80))
            self.chart.create_rectangle(left, y + 2, left + bar_w, y + 14, outline="#666")
            self.chart.create_text(left + bar_w + 6, y, anchor="nw", text=f"{value:.3f}")

    def _draw_samples(self, path: Path) -> None:
        self.chart.delete("all")
        if not path.exists():
            self.chart.create_text(20, 20, anchor="nw", text="No process samples for this run")
            return
        rows = []
        try:
            with path.open("r", encoding="utf-8") as f:
                for line in f:
                    if line.strip():
                        rows.append(json.loads(line))
        except Exception as exc:
            self.chart.create_text(20, 20, anchor="nw", text=f"Failed to read samples: {exc}")
            return
        if not rows:
            return
        self.chart.update_idletasks()
        width = max(600, self.chart.winfo_width())
        height = max(300, self.chart.winfo_height())
        margin = 50
        plot_h = (height - margin * 3) / 2
        xs = [float(r.get("monotonic_seconds", i)) for i, r in enumerate(rows)]
        cpus = [float(r.get("cpu_percent_sum", 0)) for r in rows]
        mems = [float(r.get("rss_bytes_sum", 0)) / (1024 * 1024) for r in rows]
        self._draw_line_chart(xs, cpus, margin, margin, width - margin * 2, plot_h, "CPU % (sum across process tree)")
        self._draw_line_chart(xs, mems, margin, margin * 2 + plot_h, width - margin * 2, plot_h, "RSS MiB (process tree)")

    def _draw_line_chart(self, xs, ys, left, top, width, height, title) -> None:
        self.chart.create_rectangle(left, top, left + width, top + height, outline="#aaa")
        self.chart.create_text(left + 4, top + 4, anchor="nw", text=title)
        if not xs or not ys:
            return
        min_x, max_x = min(xs), max(xs)
        max_y = max(max(ys), 1.0)
        pts = []
        for x, y in zip(xs, ys):
            px = left + ((x - min_x) / max(max_x - min_x, 1e-9)) * width
            py = top + height - (y / max_y) * (height - 24)
            pts.extend([px, py])
        if len(pts) >= 4:
            self.chart.create_line(*pts, width=2)
        self.chart.create_text(left + width - 4, top + 4, anchor="ne", text=f"peak {max_y:.2f}")

    @staticmethod
    def _human_bytes(value) -> str:
        n = float(value or 0)
        for unit in ("B", "KiB", "MiB", "GiB", "TiB"):
            if n < 1024 or unit == "TiB":
                return f"{n:.2f} {unit}"
            n /= 1024
        return f"{n:.2f} TiB"

    def _poll_events(self) -> None:
        for event in self.events.drain():
            kind = event.kind
            p = event.payload
            if kind == "log":
                tag = "stderr" if p.get("channel") == "stderr" else ("info" if p.get("channel") == "info" else "")
                self.console.insert(tk.END, p.get("text", "") + "\n", tag)
                self.console.see(tk.END)
            elif kind == "status":
                self.status_text.set(p.get("text", ""))
            elif kind == "busy":
                busy = bool(p.get("busy"))
                self.status_text.set(("Running: " if busy else "Finished: ") + p.get("title", ""))
            elif kind == "tests_discovered":
                self.tests = [TestCase(**x) for x in p.get("tests", [])]
                self._render_tests()
            elif kind == "test_progress":
                self.progress_text.set(f"{p.get('current')}/{p.get('total')} {p.get('name')}")
            elif kind == "run_started":
                self.progress_text.set(p.get("run_id", ""))
            elif kind == "run_finished":
                self.progress_text.set(f"{p.get('run_id')} {p.get('status')}")
                self._refresh_runs()
            elif kind == "runs_changed":
                self._refresh_runs()
            elif kind == "error":
                self.status_text.set(p.get("text", "Error"))
                messagebox.showerror("Epidemic Auditor", p.get("text", "Unknown error"))
        self.root.after(100, self._poll_events)

    def _on_close(self) -> None:
        self.manager.shutdown()
        self.root.destroy()
