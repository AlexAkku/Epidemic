from __future__ import annotations

import os
import csv
import re
from decimal import Decimal, InvalidOperation
import shutil
import subprocess
from pathlib import Path
from typing import Any
from ep_auditor.core.evidence import EvidenceError

OBSERVATION_KINDS = {"P-Start", "P-End", "P-DCStart", "P-DCEnd", "T-Start", "T-End", "T-DCStart", "T-DCEnd",
                     "CSwitch", "ReadyThread", "SampledProfile", "Stack", "I-Start", "I-End", "I-DCStart", "I-DCEnd",
                     "ImageId", "DbgId/None", "DbgId/Bin", "DbgId/RSDS", "DbgId/ILRSDS"}


def process_id(value: str) -> int:
    match = re.search(r"\(\s*(\d+)\s*\)\s*$", value)
    if not match:
        raise EvidenceError("xperf process identity is malformed")
    return int(match.group(1))


def iter_windows_events(path: Path, pids: set[int], *, encoding: str = "utf-8-sig"):
    """Stream a scoped dump, retaining exact CSV positions and decoder fields.

    Header names, not guessed column offsets, define each version's payload.
    Stacks have no PID: only OS thread lifecycle and event identity may join them.
    Unknown event kinds are outside this selected normalization scope.
    """
    if not pids or any(type(pid) is not int or pid <= 0 for pid in pids):
        raise EvidenceError("positive scoped process identities required")
    headers, threads = {}, set()
    in_header, ended_header = False, False
    with path.open(encoding=encoding, errors="strict", newline="") as stream:
        reader = csv.reader(stream, skipinitialspace=True)
        for ordinal, row in enumerate(reader):
            row = [field.strip() for field in row]
            if not row:
                continue
            kind = row[0]
            if kind == "BeginHeader":
                if in_header or ended_header:
                    raise EvidenceError("duplicate xperf header")
                in_header = True
                continue
            if kind == "EndHeader":
                if not in_header:
                    raise EvidenceError("xperf header termination without header")
                in_header, ended_header = False, True
                continue
            if in_header:
                if kind not in OBSERVATION_KINDS:
                    continue
                if kind in headers and headers[kind] != row[1:]:
                    raise EvidenceError("ambiguous required xperf event schema")
                headers[kind] = row[1:]
                continue
            if not ended_header:
                raise EvidenceError("xperf body precedes event schemas")
            fields = headers.get(kind)
            if not fields or "TimeStamp" not in fields:
                continue
            extensions = []
            # Native xperf appends one unlabelled detail after a trailing
            # Image!Function field. Preserve it opaque; it is not a source line.
            if fields[-1] == "Image!Function" and len(row) - 1 == len(fields) + 1:
                extensions, row = row[-1:], row[:-1]
            # Xperf strings can contain quoted commas; csv.reader owns their parsing.
            if len(row) - 1 != len(fields):
                if any(re.search(r"\(\s*" + str(pid) + r"\s*\)", value) for pid in pids for value in row):
                    raise EvidenceError("scoped xperf event payload disagrees with schema: " + kind)
                continue
            values = dict(zip(fields, row[1:]))
            relevant = set()
            for field, value in values.items():
                if field.endswith("Process Name ( PID)"):
                    # Native xperf uses this sentinel when the readying process
                    # cannot be resolved. The target PID can still be observed.
                    if re.fullmatch(r"Unknown\s*\(\s*-1\s*\)", value):
                        continue
                    pid = process_id(value)
                    if pid in pids:
                        relevant.add(pid)
            if kind == "Stack":
                try:
                    selected = int(values["ThreadID"]) in threads
                except (KeyError, ValueError) as error:
                    raise EvidenceError("malformed xperf stack thread identity") from error
            else:
                selected = bool(relevant)
            if not selected:
                continue
            try:
                stamp = Decimal(values["TimeStamp"]) * 1000
                if not stamp.is_finite() or stamp < 0 or stamp != stamp.to_integral_value():
                    raise ValueError("invalid microsecond timestamp")
                timestamp_ns = int(stamp)
            except (KeyError, ValueError, InvalidOperation) as error:
                raise EvidenceError("malformed xperf timestamp") from error
            if kind in {"T-Start", "T-DCStart"}:
                threads.add(int(values["ThreadID"]))
            yield {"kind": kind, "timestamp_ns": timestamp_ns, "fields": values,
                   "decoder_extensions": extensions,
                   "scoped_pids": sorted(relevant), "rawOrdinal": ordinal,
                   "csv_line": reader.line_num, "clock_domain": "etl_relative_trace_ns",
                   "decoder_text_encoding": encoding,
                   "provenance": "external", "confidence": "observed"}
            if kind == "T-End":
                threads.discard(int(values["ThreadID"]))
        if not ended_header:
            raise EvidenceError("xperf dump schema header missing or incomplete")


class XperfPostProcessor:
    """Best-effort conversion of ETL profiling evidence into agent-readable text.

    It never participates in pass/fail of the observed process. Failure to decode an ETL is
    recorded as diagnostic metadata and raw ETL remains authoritative evidence.
    """

    def __init__(self) -> None:
        self.exe = self._find_xperf()

    @property
    def available(self) -> bool:
        return self.exe is not None

    def process(self, trace: Path, pid: int, output_dir: Path) -> dict[str, Any]:
        output_dir.mkdir(parents=True, exist_ok=True)
        result: dict[str, Any] = {
            "available": self.available,
            "trace": str(trace),
            "pid": pid,
            "reports": {},
            "errors": [],
            "provenance": "external_xperf_analysis",
        }
        if not self.exe:
            result["errors"].append("xperf.exe was not found; install Windows Performance Toolkit for text stack reports")
            return result

        jobs = [
            ("cpu_profile", ["-a", "profile", "-detail"]),
            ("cpu_disk", ["-a", "cpudisk", "-pids", str(pid), "-thread"]),
            ("profile_stacks", ["-a", "stack", "-pid", str(pid), "-event", "Profile", "-butterfly", "1"]),
        ]
        for name, action in jobs:
            out = output_dir / f"{name}.txt"
            cmd = [str(self.exe), "-i", str(trace), "-symbols", "-o", str(out), *action]
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180, check=False)
                if proc.returncode == 0 and out.exists():
                    result["reports"][name] = str(out)
                else:
                    detail = (proc.stderr or proc.stdout or "").strip()
                    result["errors"].append(f"{name}: xperf exit {proc.returncode}: {detail[:4000]}")
            except Exception as exc:
                result["errors"].append(f"{name}: {exc}")
        return result

    @staticmethod
    def _find_xperf() -> Path | None:
        found = shutil.which("xperf.exe") or shutil.which("xperf")
        if found:
            return Path(found)
        candidates: list[Path] = []
        for env_name in ("ProgramFiles(x86)", "ProgramFiles"):
            base = os.environ.get(env_name)
            if not base:
                continue
            candidates += [
                Path(base) / "Windows Kits" / "10" / "Windows Performance Toolkit" / "xperf.exe",
                Path(base) / "Microsoft Windows Performance Toolkit" / "xperf.exe",
            ]
        for path in candidates:
            if path.exists():
                return path
        return None
