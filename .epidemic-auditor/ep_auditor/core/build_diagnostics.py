"""Normalize retained build-tool diagnostics without treating their text as execution proof."""
import hashlib
import json
import re
from pathlib import Path

from .evidence import RawEvidence
from .lineage import EvidenceLineage, normalized_record

MSVC = re.compile(r"^\s*(?:\d+>)?\s*(?:(?P<file>.+?)(?:\((?P<line>\d+)(?:,(?P<column>\d+))?\))?\s*:\s*)?"
                  r"(?P<severity>fatal error|error|warning|note)\s+(?P<code>(?:C|LNK|MSB|D|FTK)\d+)\s*:\s*(?P<message>.*)$")
CMAKE = re.compile(r"^CMake (?P<severity>Error|Warning)(?: \([^)]*\))?(?: at (?P<file>.+?):(?P<line>\d+) \([^)]*\))?:\s*(?P<message>.*)$")


def parse(data: bytes, channel: str, operation: str) -> list[dict]:
    byte_lines = data.splitlines(keepends=True)
    lines = [line.rstrip(b"\r\n").decode("utf-8", errors="replace") for line in byte_lines]
    byte_ends = []
    offset = 0
    for line in byte_lines:
        offset += len(line)
        byte_ends.append(offset)
    rows = []
    for ordinal, text in enumerate(lines):
        match = MSVC.match(text)
        cmake = False
        if not match:
            match = CMAKE.match(text)
            cmake = match is not None
        if not match:
            continue
        fields = match.groupdict()
        code = fields.get("code")
        tool = "cmake" if cmake else "linker" if code.startswith("LNK") else "msbuild" if code.startswith("MSB") else "compiler"
        message = fields["message"]
        end = ordinal
        if cmake:
            continuation = []
            for end, following in enumerate(lines[ordinal + 1:], ordinal + 1):
                if following and not following.startswith(" "):
                    end -= 1
                    break
                continuation.append(following)
            message = "\n".join([message, *continuation]).strip()
        target_match = re.search(r" \[([^\]]+\.vcxproj)\]\s*$", message)
        target = Path(target_match[1]).stem if target_match else None
        if target_match:
            message = message[:target_match.start()].rstrip()
        rows.append({"severity": "error" if fields["severity"].lower() in {"error", "fatal error"} else fields["severity"].lower(),
                     "tool": tool, "code": code, "message": message, "file": fields.get("file"),
                     "line": int(fields["line"]) if fields.get("line") else None,
                     "column": int(fields["column"]) if fields.get("column") else None,
                     "target": target, "phase": "link" if tool == "linker" else "compile" if tool == "compiler" else operation,
                     "channel": channel, "raw_line": ordinal, "raw_end_line": end,
                     "header_end_offset": byte_ends[ordinal], "header_has_newline": byte_lines[ordinal].endswith(b"\n"),
                     "raw_text": "\n".join(lines[ordinal:end + 1]),
                     "decoding": "utf-8/errors=replace", "tool_attribution_confidence": "declared"})
    return rows


def record_diagnostics(run_dir: Path, operation: str, exit_code: int | None) -> None:
    raw = RawEvidence(run_dir)
    lineage = EvidenceLineage(run_dir)
    refs = [lineage.reference("raw", "process/" + channel + ".bin") for channel in ("stdout", "stderr")]
    observation_ref = lineage.reference("raw", "process/stdio_reads.json")
    observations = json.loads(raw.read_bytes("process/stdio_reads.json"))
    rows = []
    for channel, reference in zip(("stdout", "stderr"), refs):
        for value in parse(raw.read_bytes("process/" + channel + ".bin"), channel, operation):
            candidate = next((event for event in observations["events"] if event["channel"] == channel and
                              ((value["header_has_newline"] and event["kind"] == "read" and
                                event["start_offset"] < value["header_end_offset"] <= event["end_offset"])
                               or (not value["header_has_newline"] and event["kind"] == "eof"))), None)
            value["observer_sequence"] = candidate["sequence"] if candidate else None
            value["observed_perf_ns"] = candidate["observed_perf_ns"] if candidate else None
            value["clock_domain"] = "monotonic_trace"
            row = normalized_record({**value, "schemaVersion": 1, "eventType": "build.diagnostic", "required": True,
                                     "provenance": "external", "confidence": "derived"},
                                    {"build.diagnostic"}, reference, value["raw_line"])
            row["diagnostic_id"] = row["eventId"]
            rows.append(row)
    rows.sort(key=lambda row: (row["observer_sequence"] if row["observer_sequence"] is not None else float("inf"), row["raw_line"]))
    if not rows:
        summary = normalized_record({"schemaVersion": 1, "eventType": "build.diagnostics.summary", "required": True,
                                     "diagnostic_count": 0, "observed_exit_code": exit_code,
                                     "provenance": "external", "confidence": "derived"},
                                    {"build.diagnostics.summary"}, observation_ref, 0)
        lineage.write("normalized", "build_diagnostics_summary.json", producer_version="build-diagnostics-1",
                      producer_source=Path(__file__), inputs=refs + [observation_ref], records=[summary])
        return
    lineage.write("normalized", "build_diagnostics.json", producer_version="build-diagnostics-1",
                  producer_source=Path(__file__), inputs=refs + [observation_ref], records=rows)
    # Root failure requires the independently observed tool exit as well as diagnostic text.
    errors = [row for row in rows if row["severity"] == "error"]
    actionable = [row for row in errors if row["tool"] != "msbuild"] or errors
    if exit_code not in {0, None} and actionable and not observations["dropped"] and all(row["observer_sequence"] is not None for row in actionable):
        root = actionable[0]
        reference = lineage.reference("normalized", "build_diagnostics.json")
        identity = json.dumps([reference, root["diagnostic_id"]], sort_keys=True).encode()
        finding = {"finding_id": "FindingId:" + hashlib.sha256(identity).hexdigest(),
                   "diagnostic_id": root["diagnostic_id"], "category": root["phase"] + "_failure",
                   "severity": "error", "message": root["message"], "provenance": "external", "confidence": "derived",
                   "ordering": "first_actionable_in_observer_receive_line_completion_order",
                   "diagnostic_count": len(rows), "observed_tool_exit_code": exit_code}
        lineage.write("findings", "build_root_cause.json", producer_version="build-diagnostics-1",
                      producer_source=Path(__file__), inputs=[reference, lineage.reference("raw", "process/lifecycle.json")], records=[finding])
