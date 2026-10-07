"""Immutable captures and independently verifiable, versioned evidence manifests."""
from __future__ import annotations

import hashlib
import json
import os
import re
import tempfile
import uuid
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any

SCHEMA_VERSION = 1
PROVENANCE = {"external", "compiler_instrumentation", "static_analysis", "engine_probe"}
CONFIDENCE = {"observed", "derived", "declared", "heuristic"}


class EvidenceError(ValueError):
    pass


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def load_json(path: Path) -> dict:
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise EvidenceError("duplicate evidence JSON key")
            result[key] = value
        return result
    def nonfinite(value):
        raise EvidenceError("nonfinite evidence JSON number")
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique, parse_constant=nonfinite)


def guard_sealed_raw(path: Path) -> None:
    for parent in path.resolve().parents:
        if parent.name == "raw":
            if (parent.parent / "manifest.json").exists():
                raise EvidenceError("sealed raw evidence is immutable")
            break


def atomic_json(path: Path, value: Any) -> None:
    guard_sealed_raw(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=path.parent)
    temporary = Path(name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            encoder = json.JSONEncoder(ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False)
            for chunk in encoder.iterencode(value):
                stream.write(chunk)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def checked_path(root: Path, name: str) -> Path:
    relative = PurePosixPath(name)
    if (not name or "\\" in name or ":" in name or relative.is_absolute()
            or any(part in {".", ".."} for part in name.split("/")) or relative.as_posix() != name):
        raise EvidenceError("noncanonical evidence path")
    target = root.joinpath(*relative.parts)
    if not target.resolve().is_relative_to(root.resolve()):
        raise EvidenceError("evidence path escapes its layer")
    if any(path.is_symlink() for path in (target, *target.parents) if path.is_relative_to(root)):
        raise EvidenceError("linked evidence path is not an owned capture")
    if target.is_file() and target.stat().st_nlink != 1:
        raise EvidenceError("hard-linked evidence is not an owned capture")
    return target


def capture_health(value: dict) -> dict:
    if not {"capacity", "observed", "dropped", "incomplete_reason"}.issubset(value):
        raise EvidenceError("capture health counters/reason missing")
    for key in ("capacity", "observed", "dropped"):
        if value[key] is not None and (type(value[key]) is not int or value[key] < 0):
            raise EvidenceError("invalid capture health counter")
    reason = value["incomplete_reason"]
    if reason is not None and (not isinstance(reason, str) or not reason.strip()):
        raise EvidenceError("invalid incomplete capture reason")
    known = all(value[key] is not None for key in ("capacity", "observed", "dropped"))
    complete = known and value["capacity"] > 0 and value["dropped"] == 0 and reason is None
    if not complete and reason is None:
        reason = ("capture_counters_unknown" if not known else "collector_dropped_events"
                  if value["dropped"] else "collector_capacity_zero")
    return {**value, "complete": complete, "incomplete_reason": reason}


class RawEvidence:
    """One capture writer per run; consumers receive only verified read access."""

    def __init__(self, run_dir: Path):
        self.run_dir = run_dir.resolve()
        self.raw = self.run_dir / "raw"
        self.catalog_path = self.run_dir / "capture_catalog.json"
        self.manifest_path = self.run_dir / "manifest.json"
        if self.raw.is_symlink():
            raise EvidenceError("raw layer cannot be linked")
        if self.manifest_path.exists():
            if not self.raw.is_dir():
                raise EvidenceError("finalized raw directory missing")
        else:
            self.raw.mkdir(parents=True, exist_ok=True)

    @contextmanager
    def _writer(self):
        lock = self.run_dir / "capture_writer.lock"
        try:
            descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        except FileExistsError as error:
            raise EvidenceError("concurrent or interrupted capture writer") from error
        try:
            with os.fdopen(descriptor, "w") as stream:
                stream.write(str(os.getpid()))
            yield
        finally:
            lock.unlink()

    def _catalog(self) -> dict:
        if not self.catalog_path.exists():
            return {"schema_version": SCHEMA_VERSION, "files": {}}
        value = load_json(self.catalog_path)
        if value["schema_version"] != SCHEMA_VERSION:
            raise EvidenceError("unsupported capture catalog schema")
        return value

    def append(self, name: str, data: bytes, *, collector: str, version: str,
               start_ns: int, end_ns: int, health: dict, provenance: str = "external",
               confidence: str = "observed") -> None:
        with self._writer():
            self._append(name, data, collector=collector, version=version, start_ns=start_ns,
                         end_ns=end_ns, health=health, provenance=provenance, confidence=confidence)

    def import_file(self, name: str, source: Path, expected_sha256: str, *, collector: str, version: str,
                    start_ns: int, end_ns: int, health: dict,
                    provenance: str = "external", confidence: str = "observed") -> dict:
        """Stream a hash-bound capture into the existing catalog with bounded memory."""
        if (not isinstance(expected_sha256, str) or len(expected_sha256) != 64
                or any(character not in "0123456789abcdef" for character in expected_sha256)
                or not isinstance(collector, str) or not collector.strip() or not isinstance(version, str) or not version.strip()
                or type(start_ns) is not int or type(end_ns) is not int or not 0 <= start_ns <= end_ns
                or provenance not in PROVENANCE or confidence not in CONFIDENCE):
            raise EvidenceError("invalid imported capture identity/timing/tags")
        validated_health = capture_health(health)
        if validated_health["capacity"] is None or validated_health["observed"] is None:
            raise EvidenceError("file import requires observed byte count and bounded capacity")
        with self._writer():
            if self.manifest_path.exists():
                raise EvidenceError("finalized raw capture is immutable")
            target = checked_path(self.raw, name)
            state = self._catalog()
            if name in state["files"] or target.exists():
                raise EvidenceError("imported capture already exists")
            target.parent.mkdir(parents=True, exist_ok=True)
            created = False
            hasher, size = hashlib.sha256(), 0
            try:
                with source.open("rb") as input_stream, target.open("xb") as output_stream:
                    created = True
                    while chunk := input_stream.read(1024 * 1024):
                        size += len(chunk)
                        if size > validated_health["capacity"]:
                            raise EvidenceError("import exceeded declared byte capacity")
                        hasher.update(chunk)
                        output_stream.write(chunk)
                    output_stream.flush()
                    os.fsync(output_stream.fileno())
                if (hasher.hexdigest() != expected_sha256 or size != validated_health["observed"]
                        or validated_health["dropped"] != 0):
                    raise EvidenceError("imported source hash/byte count mismatch")
                row = {"path": "raw/" + name, "size": size, "sha256": hasher.hexdigest(),
                       "collector": collector, "collector_version": version, "start_ns": start_ns, "end_ns": end_ns,
                       "clock_domain": "monotonic_trace", "provenance": provenance, "confidence": confidence,
                       "health": validated_health, "complete": validated_health["complete"], "dropped": 0}
                state["files"][name] = row
                atomic_json(self.catalog_path, state)
                return dict(row)
            except BaseException:
                if created:
                    target.unlink(missing_ok=True)
                raise

    def _append(self, name: str, data: bytes, *, collector: str, version: str,
                start_ns: int, end_ns: int, health: dict, provenance: str, confidence: str) -> None:
        if self.manifest_path.exists():
            raise EvidenceError("finalized raw capture is immutable")
        if (not isinstance(collector, str) or not collector.strip() or not isinstance(version, str) or not version.strip()
                or not isinstance(data, bytes) or not isinstance(provenance, str) or not isinstance(confidence, str)
                or provenance not in PROVENANCE or confidence not in CONFIDENCE
                or type(start_ns) is not int or type(end_ns) is not int or start_ns < 0 or end_ns < start_ns):
            raise EvidenceError("invalid raw collector identity/timing/tags")
        target = checked_path(self.raw, name)
        state = self._catalog()
        previous = state["files"].get(name)
        validated_health = capture_health(health)
        if previous:
            if (previous["collector"] != collector or previous["collector_version"] != version
                    or previous["end_ns"] > end_ns or previous["start_ns"] != start_ns
                    or not target.is_file() or previous["sha256"] != digest(target)):
                raise EvidenceError("capture identity changed or append prefix corrupted")
            for key in ("observed", "dropped"):
                old, new = previous["health"][key], validated_health[key]
                if old is not None and (new is None or new < old):
                    raise EvidenceError("capture counters regressed")
        elif target.exists():
            raise EvidenceError("unregistered raw file cannot be adopted silently")
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("ab") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        state["files"][name] = {"path": "raw/" + name, "size": target.stat().st_size,
            "sha256": digest(target), "collector": collector, "collector_version": version,
            "start_ns": start_ns, "end_ns": end_ns, "clock_domain": "monotonic_trace",
            "provenance": provenance, "confidence": confidence, "health": validated_health,
            "complete": validated_health["complete"], "dropped": validated_health["dropped"]}
        atomic_json(self.catalog_path, state)

    def _verify_files(self, rows: list[dict]) -> None:
        names = [row["path"] for row in rows]
        actual = {path.relative_to(self.run_dir).as_posix() for path in self.raw.rglob("*") if path.is_file()}
        if len(set(names)) != len(names) or set(names) != actual:
            raise EvidenceError("missing, duplicate or unregistered raw evidence")
        for row in rows:
            if (not isinstance(row.get("collector"), str) or not row["collector"].strip()
                    or not isinstance(row.get("collector_version"), str) or not row["collector_version"].strip()
                    or type(row.get("start_ns")) is not int or type(row.get("end_ns")) is not int
                    or row["start_ns"] < 0 or row["end_ns"] < row["start_ns"]
                    or row.get("clock_domain") not in {"wall", "monotonic_trace", "cpu", "game", "simulation"}
                    or row.get("provenance") not in PROVENANCE or row.get("confidence") not in CONFIDENCE):
                raise EvidenceError("manifest collector/timing/tags missing or invalid")
            if not row["path"].startswith("raw/"):
                raise EvidenceError("manifest raw path is outside raw layer")
            path = checked_path(self.raw, row["path"][4:])
            if not path.is_file() or row["size"] != path.stat().st_size or row["sha256"] != digest(path):
                raise EvidenceError("raw capture hash/size mismatch")
            health = capture_health(row["health"])
            if row["complete"] != health["complete"] or row["dropped"] != health["dropped"]:
                raise EvidenceError("raw completeness contradicts observed loss")

    def seal(self, context: dict) -> dict:
        with self._writer():
            return self._seal(context)

    def _seal(self, context: dict) -> dict:
        if self.manifest_path.exists():
            raise EvidenceError("capture already finalized")
        self._validate_context(context)
        rows = list(self._catalog()["files"].values())
        self._verify_files(rows)
        manifest = {**context, "schema_version": SCHEMA_VERSION, "generated_at": utc_now(),
                    "storage_producer_sha256": digest(Path(__file__)), "raw_files": rows,
                    "capture_complete": bool(rows) and all(row["complete"] for row in rows)}
        for row in rows:
            checked_path(self.raw, row["path"][4:]).chmod(0o444)
        atomic_json(self.manifest_path, manifest)
        self.manifest_path.chmod(0o444)
        if self.catalog_path.exists():
            self.catalog_path.chmod(0o444)
        return self.verify()

    @staticmethod
    def _validate_context(context: dict) -> None:
        required = {"run_id", "instrumentation_profile", "source_fingerprints", "binary_fingerprints", "analyzer_version"}
        if not required.issubset(context):
            raise EvidenceError("manifest context incomplete")
        try:
            valid_id = context["run_id"] == "RunId:" + str(uuid.UUID(context["run_id"].removeprefix("RunId:")))
        except (ValueError, AttributeError):
            valid_id = False
        if not valid_id or context["instrumentation_profile"] not in {"audit", "audit-deep", "audit-memory", "audit-coverage"}:
            raise EvidenceError("manifest run/profile identity invalid")
        if not isinstance(context["analyzer_version"], str) or not context["analyzer_version"].strip():
            raise EvidenceError("analyzer version missing")
        for key in ("source_fingerprints", "binary_fingerprints"):
            if not isinstance(context[key], dict) or any(
                    not isinstance(name, str) or not name or not isinstance(value, str)
                    or re.fullmatch("[0-9a-f]{64}", value) is None for name, value in context[key].items()):
                raise EvidenceError("invalid manifest source/binary fingerprint")

    def verify(self) -> dict:
        if not self.manifest_path.is_file():
            raise EvidenceError("run capture not finalized")
        manifest = load_json(self.manifest_path)
        if manifest["schema_version"] != SCHEMA_VERSION:
            raise EvidenceError("unsupported manifest schema")
        self._validate_context(manifest)
        self._verify_files(manifest["raw_files"])
        complete = bool(manifest["raw_files"]) and all(row["complete"] for row in manifest["raw_files"])
        if manifest["capture_complete"] != complete:
            raise EvidenceError("manifest falsely claims a complete capture")
        return manifest

    def read_bytes(self, name: str) -> bytes:
        manifest = self.verify()
        if "raw/" + name not in {row["path"] for row in manifest["raw_files"]}:
            raise EvidenceError("unregistered capture read")
        value = checked_path(self.raw, name).read_bytes()
        row = next(row for row in manifest["raw_files"] if row["path"] == "raw/" + name)
        if len(value) != row["size"] or hashlib.sha256(value).hexdigest() != row["sha256"]:
            raise EvidenceError("raw capture changed during verified read")
        return value

    def read_capture(self, name: str) -> tuple[bytes, dict]:
        """Read a hash-bound captured snapshot before sealing or the sealed raw authority.

        A caller must retain the returned raw hash in its derived record. An active
        writer is rejected; unsealed data never acquires a finalized completeness claim.
        """
        if self.manifest_path.is_file():
            manifest = self.verify()
            row = next((item for item in manifest["raw_files"] if item["path"] == "raw/" + name), None)
        else:
            catalog = self._catalog()
            self._verify_files(list(catalog["files"].values()))
            row = catalog["files"].get(name)
        if row is None or row["path"] != "raw/" + name:
            raise EvidenceError("unregistered capture read")
        lock = self.run_dir / "capture_writer.lock"
        if lock.exists():
            raise EvidenceError("capture writer active during derived read")
        health = capture_health(row["health"])
        if row["complete"] != health["complete"] or row["dropped"] != health["dropped"]:
            raise EvidenceError("capture snapshot health mismatch")
        value = checked_path(self.raw, name).read_bytes()
        if lock.exists() or len(value) != row["size"] or hashlib.sha256(value).hexdigest() != row["sha256"]:
            raise EvidenceError("raw capture hash/size mismatch during snapshot read")
        return value, dict(row)
