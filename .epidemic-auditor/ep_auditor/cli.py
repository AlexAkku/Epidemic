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
    args = parser.parse_args(argv)

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
