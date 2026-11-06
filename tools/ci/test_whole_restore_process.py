"""Whole-engine committed save read by a fresh process, with corruption rejection."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

executable = Path(sys.argv[1]).resolve(strict=True)
root = Path(__file__).resolve().parents[2] / "Temp/U/process-tests"
root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="whole-", dir=root) as temporary:
    save = Path(temporary) / "whole.save"
    def run(mode):
        result = subprocess.run([str(executable), mode, str(save)], check=True, capture_output=True, timeout=120)
        values = re.findall(rb"WHOLE_FINGERPRINT ([0-9]+)", result.stdout)
        assert len(values) == 1, result.stdout
        return values[0]
    fingerprint = run("--write")
    before = save.read_bytes()
    assert run("--read") == fingerprint and run("--read") == fingerprint
    assert save.read_bytes() == before
    broken = bytearray(before); broken[0] ^= 1; save.write_bytes(broken)
    rejected = subprocess.run([str(executable), "--read", str(save)], capture_output=True, timeout=120)
    assert rejected.returncode == 1, ("Expected an explicit restore rejection, not acceptance or a child crash", rejected.returncode, rejected.stderr)
    save.write_bytes(before)
    assert run("--read") == fingerprint
print("PASS whole-engine independent read/continue, unchanged save and corrupt-file rejection")
