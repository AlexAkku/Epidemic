"""Fresh composition equivalence across native process boundaries."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

executable = Path(sys.argv[1]).resolve(strict=True)
root = Path(__file__).resolve().parents[2] / "Temp/U/process-tests"
root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="determinism-", dir=root) as temporary:
    directory = Path(temporary)
    def run(mode, seed, index):
        file = directory / f"{mode}-{seed}-{index}.json"
        subprocess.run([str(executable), "--emit", "mini", mode, str(seed), str(file)], check=True, capture_output=True, timeout=120)
        value = json.loads(file.read_text(encoding="utf-8"))
        assert value.pop("mode") == mode
        assert value["status"] == "PASS" and value["failure"] is None
        return value
    reference = run("control", 20261004, 0)
    for n in range(1, 10):
        assert run("control", 20261004, n) == reference, n
    assert run("save_reload", 20261004, 0) == reference
    assert run("control", 20261005, 0)["semantic_digest"] != reference["semantic_digest"]
print("PASS fresh-process determinism, reload continuation, trace equivalence and changed input")
