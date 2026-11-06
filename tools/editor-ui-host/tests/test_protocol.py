"""Adversarial checks of the actual companion's private stdio protocol (stdlib only)."""
from pathlib import Path
import struct
import subprocess
import sys

executable = Path(sys.argv[1]).resolve(strict=True)


def frame(body):
    return struct.pack(">I", len(body)) + body


def request(op=1, args=(), major=1, suffix=b""):
    body = struct.pack(">IHHQBQQH", 0x45505549, major, 0, 17, op, 0, 0, len(args))
    return frame(body + b"".join(struct.pack(">I", len(s)) + s for s in args) + suffix)


def run(data, expected_error=None, exit_code=0):
    process = subprocess.run([str(executable)], input=data, capture_output=True, timeout=10)
    assert process.returncode == exit_code, (process.returncode, process.stderr.decode(errors="replace"))
    if expected_error is not None:
        assert len(process.stdout) >= 29, "Missing bounded error response"
        size = struct.unpack_from(">I", process.stdout)[0]
        assert size == len(process.stdout) - 4 and size <= 16 * 1024 * 1024
        assert process.stdout[20] == 1, "Rejected request returned success"
        assert expected_error.encode() in process.stdout, process.stdout


run(request(major=2), "ui.protocol_version")
run(request(op=254), "ui.stale_generation")
run(request(suffix=b"extra"), "ui.trailing_bytes")
run(request(args=[b"\xff"]), "ui.invalid_utf8_text")
run(request(args=[b"a\x00b"]), "ui.invalid_utf8_text")
run(request(args=[b""] * 9), "ui.argument_limit")
run(frame(b"\x45"), "ui.truncated_message")
run(struct.pack(">I", 16 * 1024 * 1024 + 1), exit_code=2)
run(struct.pack(">I", 0), exit_code=2)
run(struct.pack(">I", 20) + b"short", exit_code=2)
run(b"\x00\x00", exit_code=1)
run(b"")
print("PASS U2 protocol: version, trailing bytes, UTF-8/NUL, argument/frame limits, truncation, clean EOF (12 cases)")
