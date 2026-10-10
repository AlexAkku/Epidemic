from __future__ import annotations

import argparse
import json
import threading
import time
from pathlib import Path

from ep_auditor.config import load_config
from ep_auditor.core.events import EventBus
from ep_auditor.core.run_manager import RunManager
from ep_auditor.core.storage import RunStorage


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Headless Epidemic Auditor")
    parser.add_argument("--config", default="auditor.config.json")
    from ep_auditor.core.lifecycle import MODES
    parser.add_argument("--mode", default="Fast", choices=list(MODES))
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("configure")
    sub.add_parser("build")
    sub.add_parser("discover")
    sub.add_parser("test-all")
    selected = sub.add_parser("test")
    selected.add_argument("names", nargs="+")
    sub.add_parser("run")
    sub.add_parser("build-run")
    sub.add_parser("static")
    action = sub.add_parser("action", help="Query an Action in an existing sealed RunId")
    action.add_argument("run_directory")
    action.add_argument("action_id")
    log = sub.add_parser("event-log", help="Page a sealed event log and select a recorded Action")
    log.add_argument("run_directory")
    log.add_argument("--offset", type=int, default=0)
    log.add_argument("--limit", type=int, default=50)
    log.add_argument("--action-id")
    view = sub.add_parser('view', help='Query a verified, bounded Run view')
    view.add_argument('run_directory')
    view.add_argument('view')
    view.add_argument('--scope')
    view.add_argument('--cursor')
    view.add_argument('--limit', type=int, default=200)
    field = sub.add_parser('view-field', help='Resume a nested evidence list')
    field.add_argument('run_directory')
    field.add_argument('token')
    compare = sub.add_parser('compare', help='Compare compatible sealed runs')
    compare.add_argument('baseline_directory')
    compare.add_argument('run_directory')
    compare.add_argument('--cursor')
    compare.add_argument('--limit', type=int, default=200)
    export = sub.add_parser('export', help='Export verified agent reports')
    export.add_argument('run_directory')
    export.add_argument('--baseline')
    investigate = sub.add_parser('investigate', help='Bounded stable-ID investigation')
    investigate.add_argument('run_directory')
    investigate.add_argument('kind')
    investigate.add_argument('stable_id')
    investigate.add_argument('--cursor')
    history = sub.add_parser('history', help='Verified bounded history for a binary-bound FunctionId')
    history.add_argument('function_id')
    history.add_argument('--cursor')
    history.add_argument('--limit', type=int, default=200)
    stress = sub.add_parser('stress', help='Run an owned finite campaign with disabled/enabled/scope controls')
    stress.add_argument('--spec', required=True, help='Existing JSON campaign configuration')
    args = parser.parse_args(argv)

    if args.command in ('view', 'view-field', 'compare', 'export', 'investigate'):
        from ep_auditor.analyzers.summary import query_run_view, query_view_field
        from ep_auditor.analyzers.compare import compare_runs
        from ep_auditor.exporters.agent import export_agent_pack, export_investigation
        run = Path(args.run_directory).resolve()
        if args.command == 'view':
            result = query_run_view(run, args.view, scope=args.scope, cursor=args.cursor, limit=args.limit)
        elif args.command == 'view-field':
            result = query_view_field(run, args.token)
        elif args.command == 'compare':
            result = compare_runs(Path(args.baseline_directory).resolve(), run, cursor=args.cursor, limit=args.limit)
        elif args.command == 'investigate':
            result = export_investigation(run, args.kind, args.stable_id, cursor=args.cursor)
        else:
            result = {'directory': str(export_agent_pack(run, baseline=args.baseline))}
        print(json.dumps(result, ensure_ascii=False, allow_nan=False))
        return 0

    if args.command == 'event-log':
        from ep_auditor.analyzers.causality import query_event_log
        print(json.dumps(query_event_log(Path(args.run_directory).resolve(), offset=args.offset,
                                        limit=args.limit, action_id=args.action_id), ensure_ascii=False))
        return 0

    if args.command == 'action':
        from ep_auditor.analyzers.causality import query_action
        print(json.dumps(query_action(Path(args.run_directory).resolve(), args.action_id), ensure_ascii=False))
        return 0

    cfg = load_config(Path(args.config).resolve())
    if args.command == 'stress':
        cfg.data['stress'] = json.loads(Path(args.spec).read_text(encoding='utf-8-sig'))
    events = EventBus()
    storage = RunStorage(cfg)
    if args.command == 'history':
        print(json.dumps(storage.history_for_function(args.function_id, cursor=args.cursor, limit=args.limit),
                         ensure_ascii=False, allow_nan=False))
        return 0
    manager = RunManager(cfg, storage, events)

    if args.command in {"test-all", "test"}:
        manager.catalog.discover()

    if args.command == "configure":
        manager.configure_async(args.mode)
    elif args.command == "build":
        manager.build_async(args.mode)
    elif args.command == "discover":
        tests = manager.catalog.discover()
        print(json.dumps([t.to_dict() for t in tests], indent=2, ensure_ascii=False))
        return 0
    elif args.command == "test-all":
        manager.run_all_tests_async(args.mode)
    elif args.command == "test":
        wanted = set(args.names)
        tests = [t for t in manager.catalog.tests if t.name in wanted or t.group in wanted]
        missing = wanted - {t.name for t in tests} - {t.group for t in tests}
        if missing:
            manager.shutdown()
            parser.error('unmatched names/groups: ' + ', '.join(sorted(missing)))
        manager.run_selected_tests_async(tests, args.mode)
    elif args.command == "run":
        manager.start_game_async(args.mode)
    elif args.command == "build-run":
        manager.build_and_run_async(args.mode)
    elif args.command == "static":
        manager.static_scan_async()
    elif args.command == 'stress':
        manager.submit_operation('stress', args.mode)

    failed = False
    completed = False
    try:
        while manager.busy:
            for event in events.drain():
                if event.kind == "log":
                    prefix = "ERR" if event.payload.get("channel") == "stderr" else "OUT"
                    print(f"[{prefix}] {event.payload.get('text','')}")
                elif event.kind in {"status", "error", "run_finished", "test_progress"}:
                    print(f"[{event.kind}] {event.payload}")
                    if event.kind == 'error':
                        failed = True
                    if event.kind == 'run_finished':
                        completed = True
                        failed = failed or event.payload.get('status') != 'PASS'
            time.sleep(0.05)
        for event in events.drain():
            print(f"[{event.kind}] {event.payload}")
            if event.kind == 'error':
                failed = True
            if event.kind == 'run_finished':
                completed = True
                failed = failed or event.payload.get('status') != 'PASS'
    finally:
        manager.shutdown()
    return 0 if completed and not failed else 1


if __name__ == "__main__":
    raise SystemExit(main())
