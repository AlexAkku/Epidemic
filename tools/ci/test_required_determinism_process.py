"""Execute required A-H descriptors in isolated processes; compare semantic receipts."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def run(executable: Path, root: Path, configuration: str, repetitions: int) -> dict:
    results = {}
    for scenario in "ABCDEFGH":
        reference = None
        for mode in ("control", "reload"):
            for repetition in range(repetitions):
                prefix = root / f"{configuration}-{scenario}-{mode}-{repetition}"
                output = prefix.with_suffix(".json")
                completed = subprocess.run([str(executable), "--scenario", scenario, mode,
                    str(prefix.with_suffix(".save")), str(output)], capture_output=True, text=True, timeout=120)
                if completed.returncode:
                    raise AssertionError(f"{configuration}/{scenario}/{mode}/{repetition}: "
                        f"exit={completed.returncode}; {completed.stderr[-1800:]}")
                value = json.loads(output.read_text(encoding="utf-8"))
                assert value["status"] == "PASS" and value["failure"] is None, value
                semantic = {key: value[key] for key in ("descriptor_hex", "semantic_digest",
                    "event_trace_digest", "normalized_trace", "step_count", "build_source_sha256")}
                if reference is None:
                    reference = semantic
                assert semantic == reference, f"D2 mismatch: {configuration}/{scenario}/{mode}/{repetition}"
        results[scenario] = reference
    return {"executable": str(executable), "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
        "fresh_processes": 8 * 2 * repetitions, "repetitions_per_mode": repetitions, "scenarios": results}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("--compare-executable", type=Path)
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    assert args.repetitions >= 3, "D2 requires multiple independent fresh processes"
    with tempfile.TemporaryDirectory(prefix="epidemic-ms56-process-") as temporary:
        root = Path(temporary)
        primary = run(args.executable.resolve(), root, "primary", args.repetitions)
        receipt = {"status": "PASS", "D2": primary}
        if args.compare_executable:
            comparison = run(args.compare_executable.resolve(), root, "comparison", args.repetitions)
            assert primary["scenarios"] == comparison["scenarios"], "D3 semantic mismatch"
            receipt["D3"] = comparison
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"status": "PASS", "scenarios": 8,
            "fresh_processes": primary["fresh_processes"], "D3": "PASS" if args.compare_executable else "NOT_RUN"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
