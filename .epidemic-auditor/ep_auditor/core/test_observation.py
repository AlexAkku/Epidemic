"""CTest result observations and conservative, content-bound compatibility."""
from __future__ import annotations

import hashlib
import json
import math
import xml.etree.ElementTree as ET
from pathlib import Path

from .evidence import EvidenceError, digest


def compatibility(config, test) -> tuple[str, bytes]:
    files = {}
    for item in test.command:
        path = Path(item)
        if path.is_file():
            files[str(path.resolve())] = digest(path)
    cache = config.build_dir / "CMakeCache.txt"
    if cache.is_file():
        files[str(cache.resolve())] = digest(cache)
    if not test.test_id or not files:
        raise EvidenceError("test identity or executable/build compatibility evidence missing")
    value = {"schemaVersion": 1, "test_id": test.test_id,
             "configuration": config.build_config, "profile": test.build_profile,
             "command": test.command, "cwd": test.working_directory, "file_sha256": files}
    payload = json.dumps(value, sort_keys=True, separators=(",", ":"), allow_nan=False).encode()
    return "BuildId:" + hashlib.sha256(payload).hexdigest(), payload


def observed_cancellation(lifecycle: dict) -> bool:
    termination = lifecycle.get("termination")
    cancelled = bool(termination) and termination.get("reason") != "root_exit_live_descendants"
    if termination and termination.get("purpose", "cancellation" if cancelled else "scope_cleanup") != ("cancellation" if cancelled else "scope_cleanup"):
        raise EvidenceError("termination purpose disagrees with observer reason")
    if "cancelled" in lifecycle and (type(lifecycle["cancelled"]) is not bool or lifecycle["cancelled"] != cancelled):
        raise EvidenceError("lifecycle cancellation disagrees with termination observation")
    return cancelled


def junit_result(payload: bytes, name: str, exit_code: int | None, cancelled: bool) -> tuple[str, float | None]:
    if cancelled:
        return "CANCELLED", None
    try:
        root = ET.fromstring(payload)
        cases = list(root.iter("testcase"))
        if len(cases) != 1 or cases[0].get("name") != name:
            raise EvidenceError("CTest did not observe exactly the selected test")
        case = cases[0]
        duration = float(case.attrib["time"])
        if not math.isfinite(duration) or duration < 0:
            raise EvidenceError("invalid CTest test duration")
        if case.find("skipped") is not None or case.get("status") == "notrun":
            return "INCOMPLETE", duration
        failed = case.find("failure") is not None or case.find("error") is not None
        return ("PASS" if exit_code == 0 and not failed else "FAIL"), duration
    except (ET.ParseError, KeyError, ValueError) as error:
        raise EvidenceError("invalid or missing CTest JUnit observation") from error
