"""Retain CMake File API bytes and derive evidence-bound build receipts."""
import hashlib
import json
import re
import shutil
import time
from pathlib import Path
from types import SimpleNamespace
from typing import Callable

from .evidence import EvidenceError, RawEvidence, atomic_json, digest, load_json
from .lifecycle import authoritative_run, profile_for_mode, seal_run_capture
from .lineage import EvidenceLineage, normalized_record
from .build_diagnostics import parse, record_diagnostics

FILE_CAPACITY = 8 * 1024 * 1024


def compiler_failures(raw: RawEvidence, identities: list[dict], operation: str) -> list[dict]:
    observations = load_json(raw.run_dir / "raw/process/stdio_reads.json")
    paths = {str(Path(row["physical_path"]).resolve()).casefold() for row in identities
             if row["physical_path"] and row["language"] in {"C", "CXX", "CUDA", "Fortran"}}
    result = []
    for channel in ("stdout", "stderr"):
        for diagnostic in parse(raw.read_bytes("process/" + channel + ".bin"), channel, operation):
            if diagnostic["severity"] != "error" or diagnostic["tool"] != "compiler":
                continue
            event = next((event for event in observations["events"] if event["channel"] == channel and
                          ((diagnostic["header_has_newline"] and event["kind"] == "read" and
                            event["start_offset"] < diagnostic["header_end_offset"] <= event["end_offset"])
                           or (not diagnostic["header_has_newline"] and event["kind"] == "eof"))), None)
            membership = event.get("job_membership", {}) if event else {}
            available = bool(paths) and membership.get("availability") == "observed" and not observations["dropped"]
            result.append({"channel": channel, "raw_line": diagnostic["raw_line"], "code": diagnostic["code"],
                           "observer_sequence": event["sequence"] if event else None,
                           "availability": "observed" if available else "unavailable",
                           "reason": None if available else membership.get("reason") or "missing_compiler_identity_or_read_observation",
                           "compiler_process_count": sum(str(Path(member["image_path"]).resolve()).casefold() in paths
                                                         for member in membership["members"]) if available else None,
                           "compiler_paths": sorted(paths), "membership": membership,
                           "clock_domain": "monotonic_trace", "provenance": "external", "confidence": "derived",
                           "meaning": "kernel_job_snapshot_at_observer_diagnostic_receive"})
    return result


def request_file_api(build: Path) -> None:
    query = build / ".cmake/api/v1/query"
    query.mkdir(parents=True, exist_ok=True)
    for kind in ("codemodel-v2", "toolchains-v1", "cache-v2"):
        (query / kind).touch()


def _retain(raw: RawEvidence, name: str, data: bytes, *, observed: int | None = None) -> None:
    observed = len(data) if observed is None else observed
    loss = max(0, observed - FILE_CAPACITY)
    now = time.perf_counter_ns()
    raw.append(name, data[:FILE_CAPACITY], collector="cmake.build_receipt", version="1", start_ns=now, end_ns=now,
               health={"capacity": FILE_CAPACITY, "observed": observed, "dropped": loss,
                       "incomplete_reason": "build_receipt_file_capacity_exceeded" if loss else None})
    if loss:
        raise EvidenceError("build receipt raw file capacity exceeded")


def _file(raw: RawEvidence, name: str, path: Path) -> bytes:
    before = path.stat()
    with path.open("rb") as stream:
        data = stream.read(FILE_CAPACITY)
    after = path.stat()
    if (before.st_size != after.st_size or before.st_mtime_ns != after.st_mtime_ns
            or len(data) != min(before.st_size, FILE_CAPACITY)):
        raise EvidenceError("build metadata changed during capture")
    _retain(raw, name, data, observed=before.st_size)
    return data


def _file_api(raw: RawEvidence, build: Path) -> tuple[dict, dict, Callable[[str], dict]]:
    reply = build / ".cmake/api/v1/reply"
    indices = sorted(reply.glob("index-*.json"))
    if not indices:
        raise EvidenceError("successful CMake command lacks File API reply")
    index = json.loads(_file(raw, "build/fileapi/index.json", indices[-1]))
    objects = {}

    def read(name):
        if not isinstance(name, str) or Path(name).name != name or "/" in name or "\\" in name:
            raise EvidenceError("File API descriptor escapes reply directory")
        path = reply / name
        if not path.resolve().is_relative_to(reply.resolve()):
            raise EvidenceError("File API descriptor escapes through link")
        return json.loads(_file(raw, "build/fileapi/" + name, path))

    for row in index["objects"]:
        kind = row["kind"]
        if kind in {"codemodel", "toolchains", "cache"}:
            expected_major = {"codemodel": 2, "toolchains": 1, "cache": 2}[kind]
            if (kind in objects or type(row["version"].get("major")) is not int
                    or row["version"]["major"] != expected_major):
                raise EvidenceError("ambiguous or unsupported File API object")
            value = read(row["jsonFile"])
            if (value.get("kind") != kind or type(value.get("version", {}).get("major")) is not int
                    or value["version"]["major"] != expected_major):
                raise EvidenceError("unsupported File API descriptor schema")
            objects[kind] = value
    if set(objects) != {"codemodel", "toolchains", "cache"}:
        raise EvidenceError("required CMake File API object missing")
    return index, objects, read


def _task_timing(data: bytes, task: str) -> dict:
    matches = []
    pattern = re.compile(r"^\s*(\d+) ms\s+" + task + r"\s+(\d+) calls\s*$")
    for ordinal, line in enumerate(data.decode("utf-8", errors="replace").splitlines()):
        match = pattern.fullmatch(line)
        if match:
            matches.append({"raw_line": ordinal, "milliseconds": int(match[1]), "calls": int(match[2])})
    return {"availability": "observed" if matches else "unavailable",
            "seconds": sum(row["milliseconds"] for row in matches) / 1000 if matches else None,
            "reason": None if matches else "build_generator_did_not_emit_task_timing",
            "kind": "reported_task_aggregate_duration", "clock_domain": "wall", "confidence": "declared",
            "parallel_overlap_possible": True, "measurements": matches}


def record_build(config, run_dir: Path, mode: str, operation: str, result, start_ns: int, finish_ns: int) -> None:
    raw = RawEvidence(run_dir)
    header = load_json(run_dir / "run.json")
    operation_value = {"schemaVersion": 1, "run_id": header["run_id"], "operation": operation,
                       "source_root": str(config.project_root), "build_dir": str(config.build_dir),
                       "build_config": config.build_config,
                       "command": result.command, "cwd": result.cwd, "exit_code": result.exit_code,
                       "cancelled": result.cancelled, "start_ns": start_ns, "finish_ns": finish_ns,
                       "clock_domain": "monotonic_trace", "requested_parallelism": config.parallel,
                       "instrumentation_profile": profile_for_mode(mode)}
    _retain(raw, "build/operation.json", json.dumps(operation_value, sort_keys=True, allow_nan=False).encode())
    configure_command = result.command if operation == "configure" else None
    if operation == "build":
        previous = load_json(config.build_dir / ".auditor_configuration.json")
        previous_dir = Path(previous["run_dir"])
        previous_run = authoritative_run(previous_dir)
        original = RawEvidence(previous_dir).read_bytes("build/operation.json")
        if (previous_run["status"] != "PASS" or previous_run["run_id"] != previous["run_id"]
                or hashlib.sha256(original).hexdigest() != previous["operation_sha256"]):
            raise EvidenceError("build prerequisite configure receipt is unqualified")
        prior = json.loads(original)
        if prior["operation"] != "configure" or prior["instrumentation_profile"] != profile_for_mode(mode):
            raise EvidenceError("build prerequisite profile/operation mismatch")
        configure_command = prior["command"]
        _retain(raw, "build/configure_operation.json", original)
    if (config.build_dir / "CMakeCache.txt").is_file():
        _file(raw, "build/CMakeCache.txt", config.build_dir / "CMakeCache.txt")
    index, objects, read = ({}, {}, None)
    targets, toolchains, identities = [], [], []
    if not result.cancelled and (result.exit_code == 0 or operation == "build"):
        index, objects, read = _file_api(raw, config.build_dir)
        configurations = objects["codemodel"]["configurations"]
        matching = [row for row in configurations if row["name"] == config.build_config]
        if not matching and len(configurations) == 1 and configurations[0]["name"] == "":
            matching = configurations
        if len(matching) != 1:
            raise EvidenceError("requested build configuration missing/ambiguous in File API")
        for descriptor in matching[0]["targets"]:
            target = read(descriptor["jsonFile"])
            flags = [item["fragment"] for group in target.get("compileGroups", []) for item in group.get("compileCommandFragments", [])]
            targets.append({"id": target["id"], "name": target["name"], "type": target["type"],
                            "paths": target["paths"], "artifacts": target.get("artifacts", []),
                            "dependencies": target.get("dependencies", []), "compile_flags": flags,
                            "defines": sorted({item["define"] for group in target.get("compileGroups", []) for item in group.get("defines", [])}),
                            "link_flags": [item["fragment"] for item in target.get("link", {}).get("commandFragments", [])],
                            "optimization": [value for fragment in flags for value in re.findall(r"(?:/O[d12xs]|-O[0-3sg])\b", fragment)],
                            "instrumentation_flags": [value for fragment in flags for value in fragment.split()
                                                      if value in {"/Gh", "/GH", "--coverage", "-fcoverage-mapping"}
                                                      or value.startswith(("-fsanitize=", "/fsanitize=", "-fprofile-"))],
                            "flags_provenance": "external", "flags_confidence": "declared"})
        toolchains = objects["toolchains"]["toolchains"]
        identities = []
        for row in toolchains:
            hint = row.get("compiler", {}).get("path")
            resolved = (str(Path(hint).resolve()) if hint and Path(hint).is_absolute() and Path(hint).is_file()
                        else shutil.which(hint) if hint else None)
            identities.append({"language": row["language"], "declared_path": hint, "physical_path": resolved,
                               "sha256": digest(Path(resolved)) if resolved else None,
                               "availability": "observed" if resolved else "unavailable",
                               "reason": None if resolved else "declared_compiler_not_resolved_to_physical_executable"})
        _retain(raw, "build/compiler_identity.json", json.dumps(identities, sort_keys=True).encode())
    seal_run_capture(SimpleNamespace(run_dir=run_dir, run_id=header["run_id"]))
    lineage = EvidenceLineage(run_dir)
    refs = lineage.references("raw", (path.relative_to(run_dir / "raw").as_posix()
            for path in sorted((run_dir / "raw").rglob("*")) if path.is_file()))
    identity = json.dumps([header["run_id"], refs], sort_keys=True).encode()
    value = {**operation_value, "build_id": "BuildId:" + hashlib.sha256(identity).hexdigest(),
             "configure_command": configure_command, "build_command": result.command if operation == "build" else None,
             "generator": index.get("cmake", {}).get("generator"), "compiler_toolchains": toolchains,
             "targets": targets, "flags_availability": "declared" if objects else "unavailable",
             "compiler_observations_at_failure": compiler_failures(raw, identities, operation),
             "flags_unavailable_reason": None if objects else "command_failed_or_cancelled_before_metadata",
             "timings": {"operation": {"seconds": (finish_ns - start_ns) / 1e9, "clock_domain": "monotonic_trace",
                                       "confidence": "derived", "kind": "observer_command_duration"},
                         "compile": _task_timing(raw.read_bytes("process/stdout.bin"), "CL"),
                         "link": _task_timing(raw.read_bytes("process/stdout.bin"), "Link")},
             "schemaVersion": 1, "eventType": "build.receipt", "required": True,
             "provenance": "external", "confidence": "derived"}
    reference = lineage.reference("raw", "build/operation.json")
    row = normalized_record(value, {"build.receipt"}, reference, 0)
    lineage.write("normalized", "build_receipt.json", producer_version="cmake-build-receipt-1",
                  producer_source=Path(__file__), inputs=refs, records=[row])
    record_diagnostics(run_dir, operation, result.exit_code)
    if operation == "configure" and result.exit_code == 0 and not result.cancelled:
        atomic_json(config.build_dir / ".auditor_configuration.json",
                    {"run_id": header["run_id"], "run_dir": str(run_dir), "operation_sha256": digest(run_dir / "raw/build/operation.json")})
