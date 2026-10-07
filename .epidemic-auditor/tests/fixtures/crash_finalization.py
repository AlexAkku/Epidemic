"""Real child process failure precisely before the atomic run commit."""
import os
from pathlib import Path
import sys
from unittest.mock import patch

AUDITOR = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(AUDITOR), str(AUDITOR / "tests")]
from ep_auditor.core.evidence import RawEvidence, atomic_json
from ep_auditor.core.storage import RunStorage
import test_core

base = Path(sys.argv[1])
config, _ = test_core.AuditorCoreTests._config(None, base)
storage = RunStorage(config)
record = storage.new_run("fixture", "crash", "Fast")
RawEvidence(record.run_dir).append("events.bin", b"observed", collector="controlled_fixture", version="1",
                                  start_ns=10, end_ns=20,
                                  health={"capacity": 64, "observed": 1, "dropped": 0, "incomplete_reason": None})
atomic_json(base / "child_identity.json", {"config": str(config.config_path), "run_id": record.run_id,
                                          "run_dir": str(record.run_dir)})


def interrupted_commit(path, value):
    if path.name != "run.json" or value["metadata"]["finalization"]["state"] != "committed":
        raise RuntimeError("fault injector did not reach intended commit")
    # This sentinel and the sealed manifest are independently examined by parent.
    atomic_json(base / "fault_effect.json", {"point": "before_atomic_run_commit", "candidate_status": value["status"]})
    os._exit(73)


with patch("ep_auditor.core.lifecycle.atomic_json", interrupted_commit):
    storage.finish_run(record, "PASS", {"fixture": True})
raise RuntimeError("crash injection did not fire")
