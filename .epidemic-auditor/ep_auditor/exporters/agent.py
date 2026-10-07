from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from ep_auditor.core.storage import RunStorage


def export_agent_pack(run_dir: Path, run_meta: dict[str, Any], summary: dict[str, Any], tests: list[dict[str, Any]] | None = None, static_scan: dict[str, Any] | None = None) -> None:
    agent = run_dir / "agent"
    agent.mkdir(parents=True, exist_ok=True)
    tests = tests or []
    failures = [t for t in tests if t.get("status") != "PASS"]

    index = [
        f"# Epidemic Auditor run {run_meta.get('run_id', run_dir.name)}",
        "",
        f"Status: {run_meta.get('status', 'UNKNOWN')}",
        f"Kind: {run_meta.get('kind', 'unknown')}",
        f"Capture mode: {run_meta.get('capture_mode', 'unknown')}",
        "",
        "## Read first",
        "",
        "- `RUN_SUMMARY.md`: normalized high-level evidence.",
        "- `FAILURES.md`: failed/cancelled tests and excerpts.",
        "- `HOT_PATHS.md`: process-level CPU/RSS hotspots from external sampling.",
        "- `STATIC_SCAN.md`: source-tree counts and markers.",
        "- `../process_samples.jsonl`: raw normalized process-tree samples.",
        "- `../console.jsonl`: timestamped console evidence when this run has a single process.",
        "- `../raw/trace.etl`: raw WPR/ETW trace when Deep capture succeeded.",
        "- `../raw/xperf/profile_stacks.txt`: sampled native call-stack report when xperf symbol decoding succeeded.",
        "- `../raw/xperf/cpu_profile.txt`: sampled CPU-by-function report when available.",
        "",
        "All generated evidence states its provenance. Engine-declared semantic probes are not used by this version.",
    ]
    (agent / "INDEX.md").write_text("\n".join(index) + "\n", encoding="utf-8")

    process = summary.get("process", summary)
    test_summary = summary.get("tests", {})
    lines = [
        "# Run summary", "",
        f"Run: {run_meta.get('run_id', run_dir.name)}",
        f"Title: {run_meta.get('title', '')}",
        f"Status: {run_meta.get('status', '')}",
        f"Started: {run_meta.get('started_utc', '')}",
        f"Finished: {run_meta.get('finished_utc', '')}",
        "",
        "## Process evidence", "",
        f"Wall time: {process.get('wall_seconds', 0):.3f} s" if isinstance(process.get('wall_seconds', 0), (int, float)) else f"Wall time: {process.get('wall_seconds')}",
        f"Peak RSS: {process.get('peak_rss_bytes', 0)} bytes",
        f"Peak threads: {process.get('peak_threads', 0)}",
        f"Read bytes: {process.get('total_read_bytes', 0)}",
        f"Write bytes: {process.get('total_write_bytes', 0)}",
    ]
    if test_summary:
        lines += [
            "", "## Tests", "",
            f"Total: {test_summary.get('total', 0)}",
            f"Passed: {test_summary.get('passed', 0)}",
            f"Failed: {test_summary.get('failed', 0)}",
            f"Cancelled: {test_summary.get('cancelled', 0)}",
        ]
    (agent / "RUN_SUMMARY.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    failure_lines = ["# Failures", ""]
    if not failures:
        failure_lines.append("No failed or cancelled tests were recorded.")
    for item in failures:
        failure_lines += [
            f"## {item.get('test_name')}", "",
            f"Status: {item.get('status')}",
            f"Exit code: {item.get('exit_code')}",
            f"Capture: {item.get('capture_dir')}",
            "", "```text", str(item.get("failure_excerpt", ""))[-12000:], "```", "",
        ]
    (agent / "FAILURES.md").write_text("\n".join(failure_lines) + "\n", encoding="utf-8")

    samples_summary = summary.get("samples", {})
    hot = ["# Process hotspots", "", "These are external process-level samples, not native function-level instrumentation.", ""]
    for p in samples_summary.get("processes", [])[:30]:
        avg = f"{p['cpu_avg']:.2f}%" if p.get("cpu_avg") is not None else "unavailable"
        peak = f"{p['cpu_peak']:.2f}%" if p.get("cpu_peak") is not None else "unavailable"
        hot.append(f"- {p.get('process')}: avg CPU {avg}, peak CPU {peak}, peak RSS {p.get('rss_peak_bytes')} bytes")
    if len(hot) == 4:
        hot.append("No process samples available.")
    (agent / "HOT_PATHS.md").write_text("\n".join(hot) + "\n", encoding="utf-8")

    static_lines = ["# Static scan", ""]
    if static_scan:
        static_lines += [
            f"Files: {static_scan.get('files', 0)}",
            f"Lines: {static_scan.get('lines', 0)}",
            f"Bytes: {static_scan.get('bytes', 0)}",
            f"TODO/FIXME/HACK/XXX markers: {static_scan.get('marker_count', 0)}",
            "", "## Largest files", "",
        ]
        for row in static_scan.get("largest_files", [])[:30]:
            static_lines.append(f"- {row.get('path')}: {row.get('lines')} lines")
        static_lines += ["", "## Markers", ""]
        for row in static_scan.get("markers", [])[:200]:
            static_lines.append(f"- {row.get('file')}:{row.get('line')} [{row.get('marker')}] {row.get('text')}")
    else:
        static_lines.append("Static scan was not part of this run.")
    (agent / "STATIC_SCAN.md").write_text("\n".join(static_lines) + "\n", encoding="utf-8")

    manifest = {
        "run_id": run_meta.get("run_id", run_dir.name),
        "status": run_meta.get("status"),
        "provenance_policy": {
            "external_process": "observed outside the engine process via OS process APIs",
            "external_console": "captured from process stdout/stderr by the auditor",
            "external_ctest": "derived from CTest invocation controlled by the auditor",
            "external_static_scan": "derived from read-only source-tree scan",
            "external_wpr_raw": "raw Windows Performance Recorder trace",
            "external_xperf_analysis": "text reports derived externally from ETL using xperf",
        },
        "files": {
            "index": "INDEX.md",
            "summary": "RUN_SUMMARY.md",
            "failures": "FAILURES.md",
            "hot_paths": "HOT_PATHS.md",
            "static_scan": "STATIC_SCAN.md",
        },
    }
    RunStorage.write_json(agent / "manifest.json", manifest)
