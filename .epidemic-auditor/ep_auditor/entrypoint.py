"""Native desktop entry point and supplementary headless access to the same backend."""
import argparse
import json
import os
from pathlib import Path
import sys


def main():
    runtime = Path(sys.base_prefix)
    environment = runtime / "environment.json"
    if environment.is_file():
        # The isolated embedded interpreter ignores these variables; SDK helpers
        # must not inherit an unrelated Python home/module search path either.
        os.environ.pop("PYTHONHOME", None)
        os.environ.pop("PYTHONPATH", None)
        settings = json.loads(environment.read_text())
        os.environ["TCL_LIBRARY"] = str(runtime / "tcl" / settings["tcl"])
        os.environ["TK_LIBRARY"] = str(runtime / "tcl" / settings["tk"])
    parser = argparse.ArgumentParser(prog="EpidemicAuditor.exe")
    parser.add_argument("--config", type=Path, default=Path(sys.executable).with_name("auditor.config.json"))
    parser.add_argument("--headless", choices=["configure", "build", "build-and-run", "discover-tests", "run-all-tests",
                                            "run-one-test", "run-selected-tests", "run-group", "start-game", "reproduction-matrix"])
    parser.add_argument("--mode", default="Fast")
    parser.add_argument("--test-id", action="append")
    parser.add_argument("--label", action="append")
    args = parser.parse_args()
    if not args.headless:
        from ep_auditor.app import run_app
        run_app(args.config)
        return 0
    from ep_auditor.config import load_config
    from ep_auditor.core.events import EventBus
    from ep_auditor.core.lifecycle import authoritative_run
    from ep_auditor.core.run_manager import RunManager
    from ep_auditor.core.storage import RunStorage
    config = load_config(args.config)
    storage = RunStorage(config)
    manager = RunManager(config, storage, EventBus())
    try:
        result = manager.execute(args.headless, args.mode, test_ids=args.test_id, labels=args.label)
        runs = [authoritative_run(directory) for directory in config.runs_dir.iterdir()
                if (directory / "run.json").is_file()]
        runs = [run for run in runs if run["metadata"].get("backend_operation", {}).get("invocation_id") == result["invocation_id"]]
        value = {**result, "runs": [{"run_id": run["run_id"], "status": run["status"], "run_dir": run["run_dir"]} for run in runs]}
        print(json.dumps(value))
        return 0 if runs and all(run["status"] == "PASS" for run in runs) else 1
    finally:
        manager.shutdown()
