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
    parser.add_argument("--mode", default="Fast", choices=["Fast", "Full", "Deep"])
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
    args = parser.parse_args(argv)

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
    events = EventBus()
    storage = RunStorage(cfg)
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
            print("Warning: unmatched names/groups:", ", ".join(sorted(missing)))
        manager.run_selected_tests_async(tests, args.mode)
    elif args.command == "run":
        manager.start_game_async(args.mode)
    elif args.command == "build-run":
        manager.build_and_run_async(args.mode)
    elif args.command == "static":
        manager.static_scan_async()

    try:
        while manager.busy:
            for event in events.drain():
                if event.kind == "log":
                    prefix = "ERR" if event.payload.get("channel") == "stderr" else "OUT"
                    print(f"[{prefix}] {event.payload.get('text','')}")
                elif event.kind in {"status", "error", "run_finished", "test_progress"}:
                    print(f"[{event.kind}] {event.payload}")
            time.sleep(0.05)
        for event in events.drain():
            print(f"[{event.kind}] {event.payload}")
    finally:
        manager.shutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
