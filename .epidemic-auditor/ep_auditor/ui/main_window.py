from __future__ import annotations

import json
import os
import subprocess
import tkinter as tk
from concurrent.futures import ThreadPoolExecutor
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
        self._closed = False
        self.poll_after_id = None
        self.root.bind('<Destroy>', self._on_destroy, add='+')
        self.root.title("Epidemic Auditor")
        self.root.geometry("1380x900")
        self.root.minsize(1050, 700)
        self.capture_mode = tk.StringVar(value="Fast")
        self.target_scope = tk.StringVar(value=", ".join(config.data.get("capture", {}).get("target_scope", [])))
        self.target_scope.trace_add("write", self._target_scope_changed)
        self.status_text = tk.StringVar(value="Ready")
        self.progress_text = tk.StringVar(value="")
        self.trace_executor = ThreadPoolExecutor(max_workers=1, thread_name_prefix="auditor-event-log")
        self.trace_future = None
        self.trace_run = None
        self.trace_offset = 0
        self.trace_action = None
        self.trace_result = None
        self.trace_detail_offset = 0
        self.view_future = None
        self.product_views = {}
        self._build_ui()
        self._refresh_runs()
        self.poll_after_id = self.root.after(100, self._poll_events)
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
        self.actions_tab = ttk.Frame(self.notebook)
        self.notebook.add(self.tests_tab, text="Test launcher")
        self.notebook.add(self.dashboard_tab, text="Process summary")
        self.notebook.add(self.runs_tab, text="Runs")
        self.notebook.add(self.actions_tab, text="Actions / Event Log")
        self.notebook.add(self.console_tab, text="Console")
        self.notebook.add(self.settings_tab, text="Settings")

        self._build_tests_tab()
        self._build_dashboard_tab()
        self._build_runs_tab()
        self._build_actions_tab()
        self._build_console_tab()
        self._build_settings_tab()
        self._build_product_views()
        navigation = ttk.Frame(self.root, padding=(8, 0, 8, 4))
        navigation.pack(fill=tk.X, before=self.notebook)
        ttk.Label(navigation, text='View:').pack(side=tk.LEFT)
        self.view_navigation = ttk.Combobox(navigation, state='readonly', width=28,
            values=[self.notebook.tab(tab, 'text') for tab in self.notebook.tabs()])
        self.view_navigation.pack(side=tk.LEFT, padx=6)
        self.view_navigation.bind('<<ComboboxSelected>>', lambda _event: self.notebook.select(
            self.notebook.tabs()[self.view_navigation.current()]))

        status = ttk.Frame(self.root, padding=(8, 2, 8, 6))
        status.pack(fill=tk.X)
        ttk.Label(status, textvariable=self.status_text).pack(side=tk.LEFT)
        ttk.Label(status, textvariable=self.progress_text).pack(side=tk.RIGHT)

    @staticmethod
    def _button(parent, text, command):
        return ttk.Button(parent, text=text, command=command)

    def _build_product_views(self):
        from ep_auditor.analyzers.summary import VIEWS
        for name in ['Dashboard'] + [view for view in VIEWS if view not in ('Actions',)] + ['History', 'Compare', 'Agent']:
            frame = ttk.Frame(self.notebook, padding=6)
            self.notebook.add(frame, text=name)
            bar = ttk.Frame(frame)
            bar.pack(fill=tk.X)
            run_id, scope, baseline = tk.StringVar(), tk.StringVar(), tk.StringVar()
            ttk.Label(bar, text='RunId (or selected run):').pack(side=tk.LEFT)
            ttk.Entry(bar, textvariable=run_id, width=30).pack(side=tk.LEFT, padx=4)
            ttk.Label(bar, text='Baseline RunId:' if name in ('Compare', 'Dashboard') else 'Stable ID:').pack(side=tk.LEFT)
            ttk.Entry(bar, textvariable=baseline if name in ('Compare', 'Dashboard') else scope, width=36).pack(side=tk.LEFT, padx=4)
            load = ttk.Button(bar, text='Compare' if name == 'Compare' else 'Load',
                              command=lambda view=name: self._request_product_view(view, reset=True))
            load.pack(side=tk.LEFT, padx=4)
            next_page = ttk.Button(bar, text='Next page', state='disabled',
                                   command=lambda view=name: self._request_product_view(view))
            next_page.pack(side=tk.LEFT)
            status = tk.StringVar(value='Select a sealed run. Unobserved activity is UNKNOWN.')
            ttk.Label(frame, textvariable=status).pack(anchor='w', pady=6)
            chart = tk.Canvas(frame, height=170, background='white', highlightthickness=1)
            chart.pack(fill=tk.X, side=tk.BOTTOM)
            body = ttk.Frame(frame)
            body.pack(fill=tk.BOTH, expand=True)
            tree = ttk.Treeview(body, columns=('kind', 'confidence', 'complete'), show='tree headings')
            for key, label in (('#0', 'Stable ID / source'), ('kind', 'Type / rule'),
                               ('confidence', 'Provenance / confidence'), ('complete', 'Observation')):
                tree.heading(key, text=label)
            tree.column('#0', width=400)
            scroll = ttk.Scrollbar(body, orient=tk.VERTICAL, command=tree.yview)
            tree.configure(yscrollcommand=scroll.set)
            tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
            scroll.pack(fill=tk.Y, side=tk.LEFT)
            detail = tk.Text(body, width=64, wrap='word', state='disabled')
            detail.pack(fill=tk.BOTH, expand=True, side=tk.RIGHT)
            state = dict(frame=frame, run_id=run_id, scope=scope, baseline=baseline, tree=tree, detail=detail,
                         chart=chart, status=status, load=load, next=next_page, page=None, cursor=None)
            self.product_views[name] = state
            state['metric'] = tk.StringVar(value='automatic')
            metric_bar = ttk.Frame(frame)
            metric_bar.pack(fill=tk.X, before=body)
            ttk.Label(metric_bar, text='Chart metric (verified page):').pack(side=tk.LEFT)
            selector = ttk.Combobox(metric_bar, textvariable=state['metric'], state='readonly', width=22,
                values=('automatic', 'inclusive_ns', 'self_ns', 'calls', 'p95_ns', 'p99_ns', 'size_bytes',
                        'bytes', 'latency_ns', 'rss_bytes_sum', 'cpu_percent_sum', 'wall_seconds'))
            selector.pack(side=tk.LEFT, padx=4)
            selector.bind('<<ComboboxSelected>>', lambda _event, view=name: self._refresh_product_chart(view))
            state['field_choice'] = tk.StringVar()
            state['field_selector'] = ttk.Combobox(metric_bar, textvariable=state['field_choice'], state='readonly', width=35)
            state['field_selector'].pack(side=tk.LEFT, padx=4)
            ttk.Button(metric_bar, text='More frames / list', command=lambda view=name: self._request_product_field(view)).pack(side=tk.LEFT)
            ttk.Button(bar, text='Source context', command=lambda view=name: self._show_product_source(view)).pack(side=tk.LEFT, padx=4)
            tree.bind('<<TreeviewSelect>>', lambda _event, view=name: self._select_product_row(view))
            if name == 'Coverage':
                ttk.Button(metric_bar, text='Open captured heatmap', command=self._open_coverage_heatmap).pack(side=tk.LEFT, padx=4)
            chart.bind('<Button-1>', lambda event, view=name: self._select_chart_row(view, event))
            if name == 'Agent':
                state['agent_kind'] = tk.StringVar(value='action')
                ttk.Combobox(bar, textvariable=state['agent_kind'], state='readonly', width=10,
                             values=('finding', 'test', 'action', 'trace', 'function', 'module', 'scenario')).pack(side=tk.LEFT, padx=4)
                ttk.Button(bar, text='Export reports', command=lambda: self._request_product_view('Agent', export=True)).pack(side=tk.LEFT, padx=4)
            if name == 'Compare':
                state['compare_field'] = tk.StringVar(value='function_timings')
                ttk.Combobox(bar, textvariable=state['compare_field'], state='readonly', width=18,
                    values=('function_timings', 'findings', 'new_functions', 'removed_functions', 'new_edges', 'removed_edges')).pack(side=tk.LEFT, padx=4)
            if name == 'History':
                state['history_kind'] = tk.StringVar(value='function')
                ttk.Combobox(bar, textvariable=state['history_kind'], state='readonly', width=12,
                             values=('function', 'test', 'regression')).pack(side=tk.LEFT, padx=4)
            if name == 'Stress':
                ttk.Button(bar, text='Configure / start campaign', command=self._configure_stress).pack(side=tk.LEFT, padx=4)

    def _configure_stress(self):
        dialog = tk.Toplevel(self.root)
        dialog.title('Owned stress campaign')
        ttk.Label(dialog, text='Explicit target, finite budget and injectors. Stop owns only this campaign tree.').pack(padx=10, pady=8)
        editor = tk.Text(dialog, width=90, height=24)
        editor.pack(fill=tk.BOTH, expand=True, padx=10)
        editor.insert('1.0', json.dumps(self.config.data.get('stress', {
            'command': self.config.run_command, 'working_dir': str(self.config.run_working_dir),
            'iterations': 1, 'timeout_seconds': 30, 'budget_seconds': 300,
            'injectors': [{'type': 'cpu_pressure', 'workers': 1}]}), indent=2))
        status = tk.StringVar()
        ttk.Label(dialog, textvariable=status).pack(anchor='w', padx=10, pady=6)
        def start():
            try:
                from ep_auditor.core.run_manager import validate_stress
                spec = validate_stress(json.loads(editor.get('1.0', 'end')))
                if self.manager.busy:
                    raise RuntimeError('Wait for the active operation')
                self.config.data['stress'] = spec
                future = self.manager.submit_operation('stress', self.capture_mode.get())
                if future is None:
                    raise RuntimeError('Campaign admission failed')
                dialog.destroy()
            except Exception as error:
                status.set(str(error))
        ttk.Button(dialog, text='Start owned campaign', command=start).pack(pady=8)

    def _request_product_view(self, name, *, reset=False, export=False):
        if self._closed or self.view_future is not None or self.trace_future is not None:
            return
        state = self.product_views[name]
        if not reset and state.get('field_mode') and state.get('cursor'):
            self._request_product_field(name, continuation=state['cursor'])
            return
        state['field_mode'] = False
        state['field_tokens'] = {}
        state['field_choice'].set('')
        state['field_selector'].configure(values=())
        run_id = state['run_id'].get().strip() or self._selected_run_id()
        row = self.storage.get_run(run_id) if run_id else None
        if not row:
            state['status'].set('No verified RunId selected')
            return
        baseline = None
        if name == 'Compare' or (name == 'Dashboard' and state['baseline'].get().strip()):
            base = self.storage.get_run(state['baseline'].get().strip())
            if not base:
                state['status'].set('Select a baseline RunId')
                return
            baseline = Path(base['run_dir'])
        cursor = None if reset else state['cursor']
        state['run_dir'] = Path(row['run_dir'])
        state['page'] = None
        state['tree'].delete(*state['tree'].get_children())
        state['chart'].delete('all')
        self._product_detail(state, 'Reading verified capture…')
        state['load'].configure(state='disabled')
        state['next'].configure(state='disabled')
        state['status'].set('Reading verified capture…')
        self.view_future = (name, self.trace_executor.submit(self._read_product_view, Path(row['run_dir']), name,
                                                            state['scope'].get().strip() or None, cursor, baseline, export,
                                                            state['agent_kind'].get() if name == 'Agent' else None,
                                                            state['compare_field'].get() if name == 'Compare' else None,
                                                            state['history_kind'].get() if name == 'History' else None))

    def _read_product_view(self, run, name, scope, cursor, baseline, export, agent_kind=None, compare_field=None, history_kind=None):
        from ep_auditor.analyzers.summary import query_run_view
        if name == 'History':
            if history_kind == 'test':
                return self.storage.history_for_test(scope, cursor=cursor)
            if history_kind == 'regression':
                return self.storage.history_for_regression(scope, cursor=cursor)
            return self.storage.history_for_function(scope, cursor=cursor)
        if name == 'Dashboard':
            return query_run_view(run, name, cursor=cursor, baseline=baseline)
        if name == 'Compare':
            from ep_auditor.analyzers.compare import compare_runs
            result = compare_runs(baseline, run, cursor=cursor)
            field = compare_field or 'function_timings'
            items = result['differences'].get(field, [])
            rows = [dict(row, functionId=row['function_ids'][1]) if isinstance(row, dict) and row.get('function_ids')
                    else row if isinstance(row, dict) else {'id': row, 'eventType': field} for row in items]
            return dict(result, items=rows, availability=result['compatibility'],
                        continuation=result.get('continuations', {}).get(field))
        if name == 'Agent':
            from ep_auditor.exporters.agent import export_agent_pack, export_investigation
            if export:
                return {'items': [{'directory': str(export_agent_pack(run))}], 'availability': 'exported', 'complete': False}
            if scope:
                kind = next((kind for prefix, kind in [('FindingId:', 'finding'), ('TestId:', 'test'),
                    ('ActionId:', 'action'), ('TraceId:', 'trace'), ('FunctionId:', 'function'),
                    ('ModuleId:', 'module'), ('ScenarioId:', 'scenario')] if scope.startswith(prefix)), None)
                kind = kind or agent_kind
                if not kind:
                    raise ValueError('Select the kind and a recorded stable ID')
                return export_investigation(run, kind, scope, cursor=cursor)
            return query_run_view(run, 'Dashboard')
        return query_run_view(run, name, scope=scope, cursor=cursor)

    def _poll_product_view(self):
        if self.view_future is None or not self.view_future[1].done():
            return
        name, future = self.view_future
        self.view_future = None
        state = self.product_views[name]
        state['load'].configure(state='normal')
        try:
            page = future.result()
            state['page'], state['cursor'] = page, page.get('continuation')
            state['status'].set(f"{page.get('run_id', '')} · {page.get('availability', 'UNKNOWN')} · "
                f"{'complete observation' if page.get('complete') is True else 'partial/unknown observation'} · "
                f"{len(page.get('items', []))} rows · truncated={page.get('truncated', False)}")
            state['next'].configure(state='normal' if state['cursor'] else 'disabled')
            for index, row in enumerate(page.get('items', [])):
                identity = next((row[key] for key in ('finding_id', 'functionId', 'moduleId', 'callId', 'event_id', 'id', 'test_id', 'pid', 'run_id') if row.get(key) is not None), index)
                state['tree'].insert('', tk.END, iid=str(index), text=str(identity), values=(
                    row.get('rule_id', row.get('type', row.get('eventType', row.get('graph_kind', 'report')))),
                    str(row.get('provenance', 'UNKNOWN')) + ' / ' + str(row.get('confidence', 'UNKNOWN')),
                    'observed' if page.get('availability') != 'UNKNOWN' else 'UNKNOWN'))
            self._product_detail(state, json.dumps({key: value for key, value in page.items() if key != 'items'}, indent=2))
            self._draw_product_chart(state, name, page.get('items', []))
        except Exception as error:
            state['page'], state['cursor'] = None, None
            state['status'].set('Evidence unavailable: ' + str(error))
            self._product_detail(state, 'No verified result. ' + str(error))

    @staticmethod
    def _product_detail(state, value):
        state['detail'].configure(state='normal')
        state['detail'].delete('1.0', tk.END)
        state['detail'].insert('1.0', value)
        state['detail'].configure(state='disabled')

    def _select_product_row(self, name):
        state = self.product_views[name]
        selected = state['tree'].selection()
        if selected and state['page']:
            row = state['page']['items'][int(selected[0])]
            self._product_detail(state, json.dumps(row, indent=2, ensure_ascii=False))
            import base64
            tokens = row.get('field_continuations', [])
            labels = [' / '.join(map(str, json.loads(base64.urlsafe_b64decode(token))['field'])) for token in tokens]
            state['field_tokens'] = dict(zip(labels, tokens))
            state['field_selector'].configure(values=labels)
            state['field_choice'].set(labels[0] if labels else '')

    def _request_product_field(self, name, *, continuation=None):
        if self._closed or self.view_future is not None or self.trace_future is not None:
            return
        state = self.product_views[name]
        token = continuation or state.get('field_tokens', {}).get(state['field_choice'].get())
        if not token or not state.get('run_dir'):
            return
        state['field_mode'] = True
        state['tree'].delete(*state['tree'].get_children())
        state['chart'].delete('all')
        state['load'].configure(state='disabled')
        state['next'].configure(state='disabled')
        self.view_future = (name, self.trace_executor.submit(self._read_product_field, state['run_dir'], token))

    @staticmethod
    def _read_product_field(run, token):
        from ep_auditor.analyzers.summary import query_view_field
        from ep_auditor.core.lifecycle import authoritative_run
        page = query_view_field(run, token)
        page['items'] = [row if isinstance(row, dict) else {'value': row} for row in page['items']]
        header = authoritative_run(run)
        page.update(run_id=header['run_id'], availability='observed' if page['items'] else 'UNKNOWN', status=header['status'])
        return page

    def _select_chart_row(self, name, event):
        state = self.product_views[name]
        for item in state['chart'].find_overlapping(event.x, event.y, event.x, event.y):
            for tag in state['chart'].gettags(item):
                if tag.startswith('row:'):
                    identity = tag[4:]
                    if state['tree'].exists(identity):
                        state['tree'].selection_set(identity)
                        state['tree'].see(identity)
                        self._select_product_row(name)
                    return

    def _refresh_product_chart(self, name):
        state = self.product_views[name]
        if state['page']:
            self._draw_product_chart(state, name, state['page']['items'])

    def _open_coverage_heatmap(self):
        from ep_auditor.core.evidence import RawEvidence, EvidenceError
        from ep_auditor.core.lifecycle import authoritative_run
        from ep_auditor.analyzers.summary import _verified_rows
        import webbrowser
        state = self.product_views['Coverage']
        try:
            run = state.get('run_dir')
            if run is None:
                raise EvidenceError('No verified Coverage Run selected')
            authoritative_run(run)
            name = 'coverage/heatmap/index.html'
            entries = [item for row, _, _, _ in _verified_rows(run, ('metrics/coverage.json',))
                       for item in row.get('heatmap', []) if item.get('path') == 'raw/' + name]
            if len(entries) != 1:
                raise EvidenceError('Captured coverage index unavailable in selected report')
            raw = RawEvidence(run)
            import hashlib
            if hashlib.sha256(raw.read_bytes(name)).hexdigest() != entries[0]['sha256']:
                raise EvidenceError('Captured heatmap differs from its coverage report')
            if not webbrowser.open((raw.raw / name).resolve(strict=True).as_uri()):
                raise EvidenceError('No local HTML viewer accepted the captured heatmap')
        except (EvidenceError, OSError, ValueError) as error:
            state['status'].set('Evidence unavailable: ' + str(error))

    def _show_product_source(self, name):
        from ep_auditor.core.evidence import EvidenceError, load_json, digest
        from ep_auditor.core.lifecycle import authoritative_run
        state = self.product_views[name]
        selected = state['tree'].selection()
        if not selected or not state['page']:
            return
        try:
            row = state['page']['items'][int(selected[0])]
            source = row.get('symbol', {}).get('source', row.get('source_ref', {}))
            if not isinstance(source, dict):
                raise EvidenceError('No verified file/line source location')
            filename = source.get('file', row.get('symbol', {}).get('source_file', row.get('source_file')))
            line = source.get('line', row.get('symbol', {}).get('source_line', row.get('source_line', row.get('symbol', {}).get('line'))))
            run = state['run_dir']
            authoritative_run(run)
            environment = load_json(run / 'environment.json')
            root = Path(environment['project_root']).resolve(strict=True)
            path = Path(filename) if filename else None
            if path is None:
                raise EvidenceError('Source location unavailable')
            path = (path if path.is_absolute() else root / path).resolve(strict=True)
            if path.is_relative_to(Path(__file__).resolve().parents[2]):
                relative = path.relative_to(Path(__file__).resolve().parents[2]).as_posix()
                expected = environment['auditor_source']['files'].get(relative)
            else:
                relative = path.relative_to(root).as_posix()
                expected = environment['engine_source']['files'].get(relative)
            with path.open('rb') as stream:
                payload = stream.read(2 * 1024**2 + 1)
            import hashlib
            if not expected or len(payload) > 2 * 1024**2 or hashlib.sha256(payload).hexdigest() != expected:
                raise EvidenceError('Local source differs from captured input or exceeds display capacity')
            if type(line) is not int or not 1 <= line <= 200000:
                raise EvidenceError('Invalid source line')
            import itertools
            start = max(1, line - 10)
            excerpt = ''.join(f'{number}: {text}\n' for number, text in enumerate(
                itertools.islice(payload.decode('utf-8').splitlines(), start - 1, start + 39), start))
            self._product_detail(state, f'{relative} · captured source hash {expected}\n\n{excerpt}')
        except (EvidenceError, OSError, ValueError, KeyError, TypeError) as error:
            self._product_detail(state, 'Source context unavailable: ' + str(error))

    @staticmethod
    def _draw_product_chart(state, name, rows):
        canvas = state['chart']
        canvas.delete('all')
        # Every chart is explicitly page-local and links back to its verified
        # row. Missing timestamps/edges are never invented for display.
        canvas.create_text(6, 4, anchor='nw', text=name + ' · verified page only; use continuation for remaining data')
        width = max(600, canvas.winfo_width()) - 30
        metric = state['metric'].get() if 'metric' in state else 'automatic'
        if name in ('Allocations', 'Exceptions'):
            from collections import Counter
            bins = Counter()
            for row in rows:
                value = row.get('size_bytes') if name == 'Allocations' else row.get('code')
                if type(value) is int and value >= 0:
                    low = 1 << max(0, value.bit_length() - 1)
                    label = ('0 bytes' if value == 0 else f'{low}–{2 * low - 1} bytes') if name == 'Allocations' else hex(value)
                    bins[label] += 1
            if bins:
                maximum = max(bins.values())
                for index, (label, count) in enumerate(sorted(bins.items())[:10]):
                    top = 27 + index * 12
                    canvas.create_text(8, top, anchor='nw', text=f'{label}: {count}')
                    canvas.create_rectangle(180, top, 180 + 280 * count / maximum, top + 10, fill='#286b9b', outline='')
                return
        if name in ('Call Tree', 'Threads', 'Locks & Waits', 'IO', 'Timeline'):
            intervals = []
            for index, row in enumerate(rows):
                series = row.get('intervals', [row]) if name == 'Threads' else [row]
                for entry in series:
                    start = entry.get('start_ns', entry.get('request_ns'))
                    end = entry.get('end_ns', entry.get('complete_ns'))
                    if type(start) is int and type(end) is int and end >= start:
                        intervals.append((index, entry, start, end))
            if intervals:
                origin = min(item[2] for item in intervals)
                span = max(item[3] for item in intervals) - origin or 1
                calls = {row.get('callId'): row for row in rows if row.get('callId')}
                for index, entry, start, end in intervals[:200]:
                    depth, parent, visited = 0, entry.get('parentCallId'), set()
                    while parent in calls and parent not in visited and depth < 20:
                        visited.add(parent)
                        depth += 1
                        parent = calls[parent].get('parentCallId')
                    lane = depth if name == 'Call Tree' else index % 10
                    top = 27 + lane * 12
                    left = 12 + width * (start - origin) / span
                    right = max(left + 2, 12 + width * (end - origin) / span)
                    canvas.create_rectangle(left, top, right, top + 10, fill='#286b9b', outline='white', tags=('row:' + str(index),))
                canvas.create_text(12, 153, anchor='nw', text=f'Observed interval {span} ns · flame/timeline clipping limited to this page')
                return
        if name in ('Causal Graph', 'Architecture'):
            edges = []
            for index, row in enumerate(rows):
                for edge in row.get('edges', [row]):
                    source, target = edge.get('source'), edge.get('target')
                    if source is not None and target is not None:
                        edges.append((index, str(source), str(target)))
            nodes = sorted({node for _, source, target in edges for node in (source, target)})[:20]
            if nodes:
                import math
                coordinates = {node: (width / 2 + width * .38 * math.cos(i * 2 * math.pi / len(nodes)),
                                     88 + 55 * math.sin(i * 2 * math.pi / len(nodes))) for i, node in enumerate(nodes)}
                for index, source, target in edges[:200]:
                    if source in coordinates and target in coordinates:
                        canvas.create_line(*coordinates[source], *coordinates[target], arrow=tk.LAST, fill='#286b9b', tags=('row:' + str(index),))
                for node, (x, y) in coordinates.items():
                    canvas.create_text(x, y, text=node[:20], fill='#222')
                return
        if name in ('Memory', 'History'):
            history_kind = state['page'].get('history_kind') if state.get('page') else None
            if history_kind == 'regression':
                rows = [dict(row, delta_ns=row['regression']['delta_ns']) for row in rows]
            default_key = 'wall_seconds' if history_kind == 'test' else 'delta_ns' if history_kind == 'regression' else 'mean_ns'
            series_key = (metric if metric in ('calls', 'mean_ns', 'p95_ns', 'cpu_self_100ns', 'wall_seconds', 'delta_ns') else default_key) if name == 'History' else (
                metric if metric in ('rss_bytes_sum', 'cpu_percent_sum') else 'rss_bytes_sum')
            measured = [(index, row[series_key]) for index, row in enumerate(rows) if type(row.get(series_key)) in (int, float)]
            if measured:
                maximum = max(abs(value) for _, value in measured) or 1
                points = [coordinate for position, (index, value) in enumerate(measured) for coordinate in
                          (12 + width * position / max(1, len(measured) - 1),
                           (88 - 55 * value / maximum) if history_kind == 'regression' else 145 - 110 * value / maximum)]
                if len(points) >= 4:
                    canvas.create_line(*points, fill='#286b9b', width=2)
                canvas.create_text(12, 153, anchor='nw', text=f'Observed {series_key} samples · peak in page {maximum}')
                return
        keys = (metric,) if metric != 'automatic' else ('inclusive_ns', 'self_ns', 'calls', 'size_bytes', 'bytes', 'latency_ns', 'timestamp_ns', 'wall_seconds', 'rss_bytes_sum')
        candidates = [(row, next((key for key in keys
                                  if type(row.get(key)) in (int, float) and row[key] >= 0), None)) for row in rows[:20]]
        values = [(row, key) for row, key in candidates if key]
        if not values:
            canvas.create_text(12, 12, anchor='nw', text=name + ': no measured numeric series in this page (UNKNOWN)')
            return
        key = values[0][1]
        values = [(row, field) for row, field in values if field == key]
        maximum = max(row[key] for row, _ in values) or 1
        for index, (row, _) in enumerate(values[:10]):
            top = 20 + index * 14
            canvas.create_text(6, top, anchor='nw', text=str(row.get('functionId', row.get('event_id', index)))[:24])
            canvas.create_rectangle(180, top, 180 + 280 * row[key] / maximum, top + 10, fill='#286b9b', outline='', tags=('row:' + str(rows.index(row)),))
        canvas.create_text(480, 12, anchor='nw', text=key + ' (verified page)')

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

    def _build_actions_tab(self) -> None:
        bar = ttk.Frame(self.actions_tab, padding=6)
        bar.pack(fill=tk.X)
        self.trace_controls = []
        for label, command in (("Open Capture", self._open_trace_capture),
                               ("Load Selected Run", self._load_trace_run)):
            button = ttk.Button(bar, text=label, command=command)
            button.pack(side=tk.LEFT, padx=3)
            self.trace_controls.append(button)
        ttk.Label(bar, text="Action:").pack(side=tk.LEFT, padx=(12, 3))
        self.trace_choice = ttk.Combobox(bar, state="readonly", width=48)
        self.trace_choice.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.trace_choice.bind("<<ComboboxSelected>>", self._select_trace_action)
        for label, delta in (("Previous", -50), ("Next", 50)):
            button = ttk.Button(bar, text=label, command=lambda d=delta: self._trace_page(d))
            button.pack(side=tk.LEFT, padx=3)
            self.trace_controls.append(button)
        self.trace_status = tk.StringVar(value="Open a sealed capture or select a run in Runs.")
        trace_status_label = ttk.Label(self.actions_tab, textvariable=self.trace_status, padding=6, justify=tk.LEFT)
        trace_status_label.pack(fill=tk.X)
        trace_status_label.bind("<Configure>", lambda e: trace_status_label.configure(wraplength=max(200, e.width - 12)))
        panes = ttk.Panedwindow(self.actions_tab, orient=tk.VERTICAL)
        panes.pack(fill=tk.BOTH, expand=True, padx=6, pady=(0, 6))
        events = ttk.Frame(panes)
        details = ttk.Frame(panes)
        panes.add(events, weight=2)
        panes.add(details, weight=1)
        columns = ("time", "type", "thread", "event")
        self.trace_tree = ttk.Treeview(events, columns=columns, show="headings", selectmode="browse")
        for column, label, width in (("time", "Trace clock (ns)", 180), ("type", "Event", 210),
                                     ("thread", "PID / epoch / thread", 210), ("event", "Event ID", 360)):
            self.trace_tree.heading(column, text=label)
            self.trace_tree.column(column, width=width)
        scroll = ttk.Scrollbar(events, orient=tk.VERTICAL, command=self.trace_tree.yview)
        self.trace_tree.configure(yscrollcommand=scroll.set)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.trace_tree.pack(fill=tk.BOTH, expand=True)
        self.trace_tree.bind("<<TreeviewSelect>>", self._show_trace_event)
        detail_bar = ttk.Frame(details)
        detail_bar.pack(fill=tk.X)
        ttk.Label(detail_bar, text="Action details:").pack(side=tk.LEFT, padx=3)
        self.trace_category = ttk.Combobox(detail_bar, state="readonly", width=24,
            values=("summary", "function_profile", "call_tree", "causal_graph", "exceptions",
                    "allocations", "threads", "io", "state_changes", "findings"))
        self.trace_category.set("summary")
        self.trace_category.pack(side=tk.LEFT)
        self.trace_category.bind("<<ComboboxSelected>>", lambda _e: self._trace_detail_page(0, reset=True))
        for label, delta in (("Previous details", -50), ("Next details", 50)):
            ttk.Button(detail_bar, text=label, command=lambda d=delta: self._trace_detail_page(d)).pack(side=tk.LEFT, padx=3)
        self.trace_details = tk.Text(details, wrap="word", height=12, state="disabled")
        detail_scroll = ttk.Scrollbar(details, orient=tk.VERTICAL, command=self.trace_details.yview)
        self.trace_details.configure(yscrollcommand=detail_scroll.set)
        detail_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.trace_details.pack(fill=tk.BOTH, expand=True)

    def _open_trace_capture(self) -> None:
        directory = filedialog.askdirectory(title="Open sealed Auditor capture")
        if directory:
            self._request_trace(Path(directory), reset=True)

    def _load_trace_run(self) -> None:
        run_id = self._selected_run_id()
        row = self.storage.get_run(run_id) if run_id else None
        if row:
            self._request_trace(Path(row["run_dir"]), reset=True)
        else:
            self.trace_status.set("Select a captured run in Runs first.")

    @staticmethod
    def _read_trace(run, offset, action_id):
        from ep_auditor.analyzers.causality import query_action, query_event_log
        page = query_event_log(run, offset=offset, limit=50, action_id=action_id)
        return page, query_action(run, action_id) if action_id is not None else None

    def _request_trace(self, run, *, reset=False) -> None:
        if self.trace_future is not None:
            return
        if reset:
            self.trace_run, self.trace_offset, self.trace_action = run, 0, None
        self.trace_result = None
        self.trace_tree.delete(*self.trace_tree.get_children())
        self._trace_text("Loading and verifying sealed evidence…")
        self.trace_status.set("Loading and verifying sealed evidence…")
        for button in self.trace_controls:
            button.configure(state="disabled")
        self.trace_choice.configure(state="disabled")
        self.trace_future = self.trace_executor.submit(self._read_trace, run, self.trace_offset, self.trace_action)

    def _select_trace_action(self, _event=None) -> None:
        index = self.trace_choice.current()
        if self.trace_result is None or index < 0:
            return
        self.trace_action = None if index == 0 else self.trace_result[0]["actions"][index - 1]["id"]
        self.trace_offset = 0
        self._request_trace(self.trace_run)

    def _trace_page(self, delta) -> None:
        if self.trace_result is None:
            return
        candidate = max(0, self.trace_offset + delta)
        if candidate >= self.trace_result[0]["total"] and candidate != 0:
            return
        self.trace_offset = candidate
        self._request_trace(self.trace_run)

    def _poll_trace(self) -> None:
        if self.trace_future is None or not self.trace_future.done():
            return
        future, self.trace_future = self.trace_future, None
        for button in self.trace_controls:
            button.configure(state="normal")
        self.trace_choice.configure(state="readonly")
        try:
            self.trace_result = future.result()
            page, action = self.trace_result
            self.trace_choice["values"] = ["All events"] + [f"{a['name']} [{a['id']}]" for a in page["actions"]]
            self.trace_choice.current(0 if self.trace_action is None else
                1 + next(i for i, a in enumerate(page["actions"]) if a["id"] == self.trace_action))
            complete = page["complete"] is True and (action is None or action["complete"] is True)
            self.trace_status.set(f"Run {page['run_id']} · {'COMPLETE capture' if complete else 'INCOMPLETE capture'} · "
                f"events {self.trace_offset + (1 if page['events'] else 0)}–{self.trace_offset + len(page['events'])} / {page['total']} · "
                "Unobserved activity is UNKNOWN; capture completeness is not a safety verdict.")
            for i, row in enumerate(page["events"]):
                thread = row.get("thread", (row.get("pid"), row.get("process_epoch"), row.get("tid")))
                self.trace_tree.insert("", tk.END, iid=str(i), values=(row.get("timestamp_ns"), row.get("type"), thread, row.get("event_id")))
            self._trace_detail_page(0, reset=True)
        except Exception as exc:
            self.trace_result = None
            self.trace_choice["values"] = ()
            self.trace_choice.set("")
            self.trace_status.set(f"Evidence unavailable: {exc}")
            self._trace_text("No verified result. Open a sealed, intact capture and retry.")

    def _trace_text(self, value) -> None:
        self.trace_details.configure(state="normal")
        self.trace_details.delete("1.0", tk.END)
        self.trace_details.insert("1.0", value)
        self.trace_details.configure(state="disabled")

    def _show_trace_event(self, _event=None) -> None:
        selected = self.trace_tree.selection()
        if selected and self.trace_result is not None:
            self._trace_text(json.dumps(self.trace_result[0]["events"][int(selected[0])], indent=2, ensure_ascii=False))

    def _trace_detail_page(self, delta, *, reset=False) -> None:
        action = self.trace_result[1] if self.trace_result else None
        if action is None:
            self._trace_text("Select an Action to inspect its profile, call tree, causal graph and diagnostics.")
            return
        category = self.trace_category.get()
        value = ({key: val for key, val in action.items() if not isinstance(val, (list, dict))} | {"action": action["action"]}
                 if category == "summary" else action[category])
        self.trace_detail_offset = 0 if reset else max(0, self.trace_detail_offset + delta)
        if isinstance(value, list):
            self.trace_detail_offset = min(self.trace_detail_offset, max(0, ((len(value) - 1) // 50) * 50))
            value = {"offset": self.trace_detail_offset, "total": len(value),
                     "records": value[self.trace_detail_offset:self.trace_detail_offset + 50]}
        elif category == "causal_graph":
            value = {key: (val[self.trace_detail_offset:self.trace_detail_offset + 50] if isinstance(val, list) else val)
                     for key, val in value.items()} | {"offset": self.trace_detail_offset,
                         "totals": {key: len(val) for key, val in action[category].items() if isinstance(val, list)}}
        self._trace_text(json.dumps(value, indent=2, ensure_ascii=False))

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
        state = self.product_views['Dashboard']
        state['run_id'].set(run_id)
        self._request_product_view('Dashboard', reset=True)
        self.notebook.select(state['frame'])

    def _show_dashboard(self, run_id: str, row: dict, summary: dict, run_dir: Path) -> None:
        self.dashboard_text.delete("1.0", tk.END)
        lines = [
            f"Run: {run_id}",
            'Open the verified Dashboard view for authoritative status and metrics.',
            f"Kind: {row.get('kind')}",
            f"Capture: {row.get('capture_mode')}",
        ]
        self.dashboard_text.insert(tk.END, "\n".join(lines))
        self.chart.delete('all')
        self.chart.create_text(12, 12, anchor='nw', text='Metrics are UNKNOWN until the sealed capture is queried.')


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
        if self._closed:
            return
        # This entry point also supports explicit refreshes. Replace, rather
        # than orphan, the outstanding timer before scheduling the next one.
        if self.poll_after_id is not None:
            self.root.after_cancel(self.poll_after_id)
            self.poll_after_id = None
        self._poll_trace()
        self._poll_product_view()
        for event in self.events.drain():
            if self._closed:
                return
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
        if not self._closed:
            self.poll_after_id = self.root.after(100, self._poll_events)

    def _release_resources(self) -> None:
        if self._closed:
            return
        self._closed = True
        if self.poll_after_id is not None:
            self.root.after_cancel(self.poll_after_id)
            self.poll_after_id = None
        self.trace_executor.shutdown(wait=False, cancel_futures=True)
        self.manager.shutdown()

    def _on_destroy(self, event) -> None:
        if event.widget is self.root:
            self._release_resources()

    def _on_close(self) -> None:
        if self._closed:
            return
        try:
            self._release_resources()
        finally:
            self.root.destroy()
