"""Production durable store, interruption and Runtime cross-process package tests."""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("executable", type=Path)
parser.add_argument("--local", action="store_true")
parser.add_argument("--runtime-executable", type=Path)
args = parser.parse_args()
executable = args.executable.resolve(strict=True)
root = Path(__file__).resolve().parents[2] / "Temp/U/process-tests"
root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="save-", dir=root) as temporary:
    directory = Path(temporary)
    if args.local:
        subprocess.run([str(executable), str(directory)], check=True, capture_output=True, timeout=120)
    else:
        save = directory / "production.save"
        def run(mode, ticks):
            return subprocess.run([str(executable), mode, str(save), str(ticks)], check=True, capture_output=True, timeout=120)
        run("write", 73); before = save.read_bytes(); run("read", 73)
        interrupted = subprocess.run([str(executable), "interrupt", str(save), "99"], capture_output=True, timeout=120)
        assert interrupted.returncode == 75, ("Expected the durable-store interruption fault, not an unrelated child failure", interrupted.returncode, interrupted.stderr)
        assert save.read_bytes() == before, "Interrupted write replaced committed generation"
        run("read", 73); run("write", 99); run("read", 99)
        runtime = args.runtime_executable.resolve(strict=True)
        package = directory / "runtime"
        package.mkdir()
        subprocess.run([str(runtime), "runtime-write", str(package)], check=True, capture_output=True, timeout=120)
        subprocess.run([str(runtime), "runtime-read", str(package)], check=True, capture_output=True, timeout=120)
print("PASS production persistence process checks")
