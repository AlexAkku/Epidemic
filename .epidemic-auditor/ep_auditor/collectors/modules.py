"""Owned native process/module/symbol/WCT snapshots with hash-bound raw evidence."""
import json
from pathlib import Path
import sys
import time

from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.identity import module_identity, function_identity
from ep_auditor.core.process import ManagedProcessRunner


def identify_snapshot(snapshot, fingerprints):
    """Disk fingerprints are observed separately from the loaded memory image."""
    result = json.loads(json.dumps(snapshot, allow_nan=False))
    for module in result["modules"]:
        fingerprint = fingerprints.get(module.get("path"))
        module["binary_file_observation"] = fingerprint
        sha = fingerprint.get("sha256") if fingerprint else None
        module["moduleId"] = module_identity(sha) if sha else None
        module["identity_scope"] = "observed disk binary; loaded memory bytes not hashed"
    symbol = result["symbol"]
    address = symbol.get("function_address")
    owners = [module for module in result["modules"] if module.get("available")
              and address is not None and module["base"] <= address < module["base"] + module["size"]]
    symbol["moduleId"], symbol["functionId"], symbol["function_rva"] = None, None, None
    if len(owners) == 1:
        module = owners[0]
        symbol["function_rva"] = address - module["base"]
        if module["moduleId"]:
            symbol["moduleId"] = module["moduleId"]
            symbol["functionId"] = function_identity(module["binary_file_observation"]["sha256"], symbol["function_rva"])
    return result


def observe(config, run_dir: Path, pid: int, creation_filetime_100ns: int, *,
            address: int | None = None, thread_id: int | None = None, helper: Path | None = None):
    if type(pid) is not int or not 0 < pid <= 0xFFFFFFFF or type(creation_filetime_100ns) is not int or creation_filetime_100ns <= 0:
        raise EvidenceError("observation requires target PID and creation identity")
    helper = helper or Path(sys.executable).parent / "EpidemicAuditorObservation.exe"
    if not helper.is_file():
        raise EvidenceError("native observation helper unavailable")
    command = [str(helper.resolve()), "--pid", str(pid)]
    for option, value, limit in (("--address", address, 2**64 - 1), ("--thread", thread_id, 0xFFFFFFFF)):
        if value is not None:
            if type(value) is not int or not 0 <= value <= limit or (option == "--thread" and value == 0):
                raise EvidenceError("invalid observation address/thread")
            command += [option, str(value)]
    started = time.perf_counter_ns()
    helper_sha = digest(helper)
    result = ManagedProcessRunner(config).run(command, helper.parent, run_dir, "Fast", timeout=20)
    if result.exit_code != 0 or helper_sha != digest(helper):
        raise EvidenceError("native observation failed or helper changed")
    capture = RawEvidence(run_dir)
    stdout, raw = capture.read_capture("process/stdout.bin")
    if not raw["complete"]:
        raise EvidenceError("native observation stdout incomplete")
    snapshot = json.loads(stdout.decode("utf-8"))
    if snapshot["pid"] != pid or snapshot["creation_filetime_100ns"] != creation_filetime_100ns:
        raise EvidenceError("target process identity changed")
    fingerprints = {}
    for module in snapshot["modules"]:
        if not module.get("available"):
            continue
        path = Path(module["path"])
        try:
            before = path.stat()
            if before.st_size > 512 * 1024 * 1024:
                raise OSError("binary exceeds observation byte budget")
            sha = digest(path)
            after = path.stat()
            if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
                raise OSError("binary changed during observation")
            fingerprints[str(path)] = {"sha256": sha, "size": after.st_size, "provenance": "external", "confidence": "observed"}
        except OSError as error:
            fingerprints[str(path)] = {"sha256": None, "unavailable_reason": str(error)}
    payload = json.dumps({"helper_sha256": helper_sha, "snapshot_raw_sha256": raw["sha256"],
                          "fingerprints": fingerprints}, sort_keys=True).encode()
    capture.append("modules/binary_files.json", payload, collector="external_binary_files", version="1",
                   start_ns=started, end_ns=time.perf_counter_ns(), health={
                       "capacity": len(payload), "observed": len(payload), "dropped": 0, "incomplete_reason": None})
    return identify_snapshot(snapshot, fingerprints)
