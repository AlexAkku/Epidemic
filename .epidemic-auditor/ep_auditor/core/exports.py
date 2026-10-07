"""Four-format exports generated from a verified structured evidence envelope."""
import csv
import html
import io
import json
import os
import tempfile
from pathlib import Path

from .evidence import EvidenceError, atomic_json, checked_path, digest, load_json
from .lineage import EvidenceLineage


def _write(path: Path, payload: str) -> None:
    descriptor, name = tempfile.mkstemp(prefix=path.name + ".", dir=path.parent)
    temporary = Path(name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def export_evidence(lineage: EvidenceLineage, layer: str, path: str, namespace: str) -> Path:
    reference = lineage.reference(layer, path)
    envelope = lineage.verify(layer, path)
    output = checked_path(lineage.run_dir / "exports", namespace)
    if output.exists():
        raise EvidenceError("export namespace already exists")
    output.mkdir(parents=True, exist_ok=False)
    metadata = {key: value for key, value in envelope.items() if key != "records"}
    rows = [{**metadata, "record": record} for record in envelope["records"]]
    compact = lambda value: json.dumps(value, ensure_ascii=False, sort_keys=True, allow_nan=False)
    atomic_json(output / "evidence.json", envelope)
    _write(output / "evidence.jsonl", "".join(compact(row) + "\n" for row in rows))
    csv_stream = io.StringIO(newline="")
    writer = csv.DictWriter(csv_stream, fieldnames=["metadata_json", "record_json"])
    writer.writeheader()
    writer.writerows({"metadata_json": compact(metadata), "record_json": compact(row["record"])} for row in rows)
    _write(output / "evidence.csv", csv_stream.getvalue())
    # Escaped JSON preserves nested fields and prevents raw evidence from becoming HTML.
    markdown = "# Evidence export\n\nMetadata:\n\n<pre>" + html.escape(compact(metadata)) + "</pre>\n\nRecords:\n\n"
    markdown += "\n".join("<pre>" + html.escape(compact(row["record"])) + "</pre>" for row in rows) + "\n"
    _write(output / "evidence.md", markdown)
    if reference != lineage.reference(layer, path):
        raise EvidenceError("export input changed during rendering")
    atomic_json(output / "manifest.json", {"schemaVersion": 1, "input": reference,
                                          "exportProducerSourceSha256": digest(Path(__file__)),
                                          "files": {name: digest(output / name) for name in
                                                    ("evidence.json", "evidence.jsonl", "evidence.csv", "evidence.md")}})
    return output


def verify_export(lineage: EvidenceLineage, namespace: str) -> dict:
    output = checked_path(lineage.run_dir / "exports", namespace)
    manifest = load_json(output / "manifest.json")
    names = {"evidence.json", "evidence.jsonl", "evidence.csv", "evidence.md"}
    if (manifest.get("schemaVersion") != 1 or not isinstance(manifest.get("files"), dict)
            or set(manifest["files"]) != names):
        raise EvidenceError("export manifest formats incomplete")
    reference = manifest["input"]
    if reference != lineage.reference(reference["layer"], reference["path"]):
        raise EvidenceError("export input binding mismatch")
    for name, expected in manifest["files"].items():
        if digest(output / name) != expected:
            raise EvidenceError("export file hash mismatch")
    return manifest
