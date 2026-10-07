"""Filesystem authority for run admission and atomic completion; SQLite is an index."""
from __future__ import annotations

import os
import sys
from pathlib import Path

from .evidence import EvidenceError, RawEvidence, atomic_json, digest, load_json, utc_now

RUN_DIRECTORIES = ("build", "tests", "raw", "normalized", "graphs", "metrics", "findings", "agent")
PROFILES = {"fast": "audit", "full": "audit", "deep": "audit-deep", "targeted exact": "audit-deep",
            "memory": "audit-memory", "coverage": "audit-coverage", "stress": "audit-deep"}
MODES = ("Fast", "Full", "Deep", "Memory", "Coverage", "Stress", "Targeted Exact")


def profile_for_mode(mode: str) -> str:
    if mode.lower() not in PROFILES:
        raise EvidenceError("unknown capture mode")
    return PROFILES[mode.lower()]


def mode_selection(mode: str, target_scope: list[str] | None = None) -> dict:
    profile = profile_for_mode(mode)
    if not isinstance(target_scope, (list, type(None))):
        raise EvidenceError("invalid explicit target scope")
    scope = list(target_scope or [])
    if any(not isinstance(item, str) or not item.strip() for item in scope):
        raise EvidenceError("invalid explicit target scope")
    targeted = mode.lower() == "targeted exact"
    if targeted and not scope:
        raise EvidenceError("Targeted Exact requires an explicit target scope")
    return {"mode": next(name for name in MODES if name.lower() == mode.lower()),
            "instrumentation_profile": profile, "deep_subprofile": "Targeted Exact" if targeted else None,
            "target_scope": scope if targeted else [], "provenance": "external", "confidence": "observed"}


def initialize_run(record, environment: dict) -> None:
    for name in RUN_DIRECTORIES:
        (record.run_dir / name).mkdir(parents=True, exist_ok=True)
    record.status = "INCOMPLETE"
    atomic_json(record.run_dir / "environment.json", environment)
    atomic_json(record.run_dir / "run.json", record.to_dict())


def seal_run_capture(record) -> dict:
    environment = load_json(record.run_dir / "environment.json")
    raw = RawEvidence(record.run_dir)
    context = {"run_id": record.run_id, "instrumentation_profile": environment["instrumentation_profile"],
               "source_fingerprints": {"engine": environment["engine_source"]["sha256"],
                                       "auditor": environment["auditor_source"]["sha256"]},
               "binary_fingerprints": {"interpreter": digest(Path(sys.executable))},
               "analyzer_version": environment["auditor_version"],
               "environment_sha256": digest(record.run_dir / "environment.json")}
    manifest = raw.verify() if raw.manifest_path.exists() else raw.seal(context)
    if any(manifest.get(key) != value for key, value in context.items()):
        raise EvidenceError("manifest admission identity changed")
    return manifest


def finalize_run(record, requested_status: str, summary: dict) -> None:
    if requested_status not in {"PASS", "FAIL", "CANCELLED", "INCOMPLETE"}:
        raise EvidenceError("unknown requested run status")
    lock = record.run_dir / "run_commit.lock"
    try:
        descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except FileExistsError as error:
        raise EvidenceError("run finalization active or interrupted") from error
    try:
        os.close(descriptor)
        previous = load_json(record.run_dir / "run.json")
        if previous.get("metadata", {}).get("finalization", {}).get("state") == "committed":
            raise EvidenceError("run already finalized")
        raw = RawEvidence(record.run_dir)
        manifest = seal_run_capture(record)
        record.status = requested_status if manifest["capture_complete"] else "INCOMPLETE"
        record.finished_utc = utc_now()
        record.metadata["finalization"] = {"state": "committed", "requested_status": requested_status,
                                          "manifest_sha256": digest(raw.manifest_path),
                                          "environment_sha256": manifest["environment_sha256"], "summary": summary}
        # Single atomic commit point. A crash before replacement leaves INCOMPLETE.
        atomic_json(record.run_dir / "run.json", record.to_dict())
    finally:
        lock.unlink()


def authoritative_run(run_dir: Path) -> dict:
    record = load_json(run_dir / "run.json")
    completion = record.get("metadata", {}).get("finalization", {})
    if completion.get("state") != "committed":
        record["status"] = "INCOMPLETE"
        return record
    raw = RawEvidence(run_dir)
    manifest = raw.verify()
    if (digest(raw.manifest_path) != completion.get("manifest_sha256")
            or digest(run_dir / "environment.json") != completion.get("environment_sha256")
            or manifest["run_id"] != record["run_id"]
            or manifest.get("environment_sha256") != completion.get("environment_sha256")):
        raise EvidenceError("run commit evidence binding mismatch")
    if not manifest["capture_complete"] and record["status"] != "INCOMPLETE":
        raise EvidenceError("incomplete capture falsely marked clean")
    if record["status"] not in {"PASS", "FAIL", "CANCELLED", "INCOMPLETE"}:
        raise EvidenceError("unknown committed run status")
    return record
