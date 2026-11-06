"""Required A-H scenarios in independent native processes; no in-process substitute."""
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(executable, directory, scenario, mode, iteration):
    output = directory / f"{scenario}-{mode}-{iteration}.json"
    subprocess.run([str(executable), "--scenario", scenario, mode,
                    str(directory / f"{scenario}-{mode}-{iteration}.save"), str(output)],
                   check=True, timeout=120, capture_output=True)
    value = json.loads(output.read_text(encoding="utf-8"))
    assert value["status"] == "PASS" and value["failure"] is None, value
    assert value["mode"] == mode and value["semantic_digest"] and value["event_trace_digest"]
    assert re.fullmatch(r"[0-9a-f]{64}", value["build_source_sha256"])
    bytes.fromhex(value["descriptor_hex"])
    return {key: value[key] for key in ("scenario_id", "config_id", "seed_set", "descriptor_hex",
            "semantic_digest", "event_trace_digest", "step_count", "normalized_trace", "build_source_sha256")}


def main():
    executable = Path(sys.argv[1]).resolve(strict=True)
    root = Path(__file__).resolve().parents[2] / "Temp/U/process-tests"
    root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="required-", dir=root) as temporary:
        directory = Path(temporary)
        identities = set()
        for scenario in "ABCDEFGH":
            reference = run(executable, directory, scenario, "control", 0)
            identities.add(reference["scenario_id"])
            for repetition in range(1, 10):
                assert run(executable, directory, scenario, "control", repetition) == reference, (scenario, repetition)
            assert run(executable, directory, scenario, "reload", 0) == reference, (scenario, "reload")
        assert len(identities) == 8, "Required scenarios are not distinct"
    print("PASS independent A-H: ten controls + fresh reload each; descriptor, build stamp, semantic/event digests and full trace")


if __name__ == "__main__":
    main()
