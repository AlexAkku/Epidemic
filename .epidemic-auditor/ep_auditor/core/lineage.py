"""Versioned evidence envelopes with verified, strictly downward lineage."""
from __future__ import annotations

import hashlib
import json
import os
import re
from copy import deepcopy
from datetime import datetime
from pathlib import Path

from .evidence import (CONFIDENCE, PROVENANCE, EvidenceError, RawEvidence,
                       atomic_json, checked_path, digest, load_json, utc_now)

LAYERS = ("raw", "normalized", "graphs", "metrics", "findings", "agent")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")


def normalized_record(value: dict, supported_types: set[str], raw_ref: dict, ordinal: int) -> dict:
    """Retain extensions and optional unknown types; reject required unknown types."""
    if not isinstance(value, dict) or type(value.get("schemaVersion")) is not int or value["schemaVersion"] != 1:
        raise EvidenceError("unsupported normalized schema version")
    event_type = value.get("eventType")
    if not isinstance(event_type, str) or not event_type.strip():
        raise EvidenceError("stable event type missing")
    if type(value.get("required")) is not bool:
        raise EvidenceError("record required/optional policy missing")
    if event_type not in supported_types and value["required"]:
        raise EvidenceError("unknown required record type: " + event_type)
    if value.get("provenance") not in PROVENANCE or value.get("confidence") not in CONFIDENCE:
        raise EvidenceError("invalid normalized provenance/confidence")
    if (raw_ref.get("layer") != "raw" or not SHA256.fullmatch(str(raw_ref.get("sha256", "")))
            or type(ordinal) is not int or ordinal < 0):
        raise EvidenceError("normalized raw identity missing")
    identity = json.dumps([raw_ref, ordinal], sort_keys=True, separators=(",", ":")).encode()
    result = deepcopy(value)
    expected = "EventId:" + hashlib.sha256(identity).hexdigest()
    if "eventId" in result and result["eventId"] != expected:
        raise EvidenceError("event identity disagrees with raw position")
    result.update(eventId=expected, rawReference=deepcopy(raw_ref), rawOrdinal=ordinal,
                  typeSupport="known" if event_type in supported_types else "unknown_optional")
    return result


class EvidenceLineage:
    def __init__(self, run_dir: Path):
        self.run_dir = run_dir
        self.raw = RawEvidence(run_dir)

    def reference(self, layer: str, path: str) -> dict:
        if layer not in LAYERS:
            raise EvidenceError("unknown evidence layer")
        target = checked_path(self.run_dir / layer, path)
        if not target.is_file():
            raise EvidenceError("input evidence missing")
        ref = {"layer": layer, "path": path, "sha256": digest(target)}
        self._input(ref, len(LAYERS))
        return ref

    def _input(self, ref: dict, upper: int) -> bool:
        if (not isinstance(ref, dict) or ref.get("layer") not in LAYERS
                or not isinstance(ref.get("path"), str)
                or not SHA256.fullmatch(str(ref.get("sha256", "")))):
            raise EvidenceError("invalid input evidence reference")
        index = LAYERS.index(ref["layer"])
        if index >= upper:
            raise EvidenceError("lineage must point to a lower evidence layer")
        target = checked_path(self.run_dir / ref["layer"], ref.get("path", ""))
        if not target.is_file() or digest(target) != ref["sha256"]:
            raise EvidenceError("input evidence hash mismatch")
        if index == 0:
            manifest = self.raw.verify()
            rows = [row for row in manifest["raw_files"] if row["path"] == "raw/" + ref["path"]]
            if len(rows) != 1 or rows[0]["sha256"] != ref["sha256"]:
                raise EvidenceError("input raw not registered in manifest")
            return rows[0]["complete"]
        return self.verify(ref["layer"], ref["path"])["completeness"]["complete"]

    def _validate(self, value: dict, layer: str) -> dict:
        if (not isinstance(value, dict) or type(value.get("schemaVersion")) is not int or value["schemaVersion"] != 1
                or value.get("layer") != layer):
            raise EvidenceError("unsupported derived schema/layer")
        if not isinstance(value.get("producerVersion"), str) or not value["producerVersion"].strip():
            raise EvidenceError("derived producer version missing")
        if not SHA256.fullmatch(str(value.get("producerSourceSha256", ""))):
            raise EvidenceError("derived producer source fingerprint missing")
        if not isinstance(value.get("generatedAt"), str) or not value["generatedAt"]:
            raise EvidenceError("derived generation time missing")
        try:
            if datetime.fromisoformat(value["generatedAt"]).utcoffset() is None:
                raise ValueError("timezone missing")
        except ValueError as error:
            raise EvidenceError("invalid derived generation time") from error
        if not isinstance(value.get("records"), list) or not value["records"]:
            raise EvidenceError("derived records absent")
        inputs = value.get("inputs")
        if not isinstance(inputs, list) or not inputs:
            raise EvidenceError("derived input lineage absent")
        complete = all([self._input(ref, LAYERS.index(layer)) for ref in inputs])
        for record in value["records"]:
            if (not isinstance(record, dict) or record.get("provenance") not in PROVENANCE
                    or record.get("confidence") not in CONFIDENCE):
                raise EvidenceError("derived record provenance/confidence missing")
            if layer == "normalized":
                ref = record.get("rawReference")
                if ref not in inputs:
                    raise EvidenceError("normalized raw reference not an envelope input")
                # Type support is checked by the consumer registry in read_normalized.
                normalized_record(record, {record.get("eventType")}, ref, record.get("rawOrdinal"))
        health = value.get("completeness")
        if (not isinstance(health, dict) or type(health.get("complete")) is not bool
                or health["complete"] != complete
                or health.get("incompleteReason") != (None if complete else "input_evidence_incomplete")):
            raise EvidenceError("derived completeness disagrees with input evidence")
        return value

    def write(self, layer: str, path: str, *, producer_version: str, producer_source: Path,
              inputs: list[dict], records: list[dict]) -> dict:
        if layer not in LAYERS[1:]:
            raise EvidenceError("derived output layer invalid")
        complete = all([self._input(ref, LAYERS.index(layer)) for ref in inputs])
        value = {"schemaVersion": 1, "layer": layer, "producerVersion": producer_version,
                 "producerSourceSha256": digest(producer_source), "inputs": deepcopy(inputs),
                 "records": deepcopy(records), "generatedAt": utc_now(),
                 "completeness": {"complete": complete,
                                  "incompleteReason": None if complete else "input_evidence_incomplete"}}
        self._validate(value, layer)
        target = checked_path(self.run_dir / layer, path)
        target.parent.mkdir(parents=True, exist_ok=True)
        claim = target.with_name(target.name + ".writer.lock")
        try:
            descriptor = os.open(claim, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        except FileExistsError as error:
            raise EvidenceError("derived output writer active or interrupted") from error
        try:
            os.close(descriptor)
            if target.exists():
                raise EvidenceError("derived version already exists; use a new output path")
            atomic_json(target, value)
        finally:
            claim.unlink()
        return self.verify(layer, path)

    def verify(self, layer: str, path: str) -> dict:
        if layer not in LAYERS[1:]:
            raise EvidenceError("derived verification layer invalid")
        return self._validate(load_json(checked_path(self.run_dir / layer, path)), layer)

    def read_normalized(self, path: str, supported_types: set[str]) -> list[dict]:
        envelope = self.verify("normalized", path)
        return [normalized_record(row, supported_types, row["rawReference"], row["rawOrdinal"])
                for row in envelope["records"]]
