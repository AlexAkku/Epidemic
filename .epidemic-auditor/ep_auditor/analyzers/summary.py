from __future__ import annotations

import json
import io
from collections import defaultdict
from pathlib import Path
from typing import Any
from ep_auditor.core.evidence import RawEvidence, load_json


def summarize_process_samples(run_dir: Path) -> dict[str, Any]:
    catalog_path = run_dir / "capture_catalog.json"
    name = "process/process_samples.jsonl"
    if not catalog_path.exists() or name not in load_json(catalog_path)["files"]:
        return {"samples": 0, "cpu_avg": None, "cpu_peak": None, "rss_peak_bytes": None, "threads_peak": None,
                "processes": [], "availability": "unavailable", "reason": "canonical_sampling_capture_absent"}
    payload, source = RawEvidence(run_dir).read_capture(name)
    count = 0
    cpu_sum = 0.0
    cpu_peak = 0.0
    cpu_count = 0
    rss_peak = 0
    threads_peak = 0
    rss_count = 0
    threads_count = 0
    per_proc: dict[str, dict[str, float]] = defaultdict(lambda: {"samples": 0.0, "cpu_count": 0, "cpu_sum": 0.0, "cpu_peak": 0.0, "rss_peak": 0.0})
    with io.StringIO(payload.decode("utf-8")) as f:
        for line in f:
            if not line.strip():
                continue
            row = json.loads(line)
            count += 1
            if row.get("cpu_percent_sum") is not None:
                cpu = float(row["cpu_percent_sum"])
                cpu_sum += cpu
                cpu_peak = max(cpu_peak, cpu)
                cpu_count += 1
            if row.get("rss_bytes_sum") is not None:
                rss_peak = max(rss_peak, int(row["rss_bytes_sum"]))
                rss_count += 1
            if row.get("thread_count_sum") is not None:
                threads_peak = max(threads_peak, int(row["thread_count_sum"]))
                threads_count += 1
            for proc in row.get("processes", []):
                key = f"{proc.get('name','?')}[{proc.get('pid','?')}@{proc.get('create_time','unavailable')}]"
                entry = per_proc[key]
                entry["samples"] += 1
                if proc.get("cpu_percent") is not None:
                    pcpu = float(proc["cpu_percent"])
                    entry["cpu_sum"] += pcpu
                    entry["cpu_peak"] = max(entry["cpu_peak"], pcpu)
                    entry["cpu_count"] += 1
                entry["rss_peak"] = max(entry["rss_peak"], float(proc.get("rss_bytes", 0)))
    processes = []
    for name, x in per_proc.items():
        samples = x["cpu_count"]
        processes.append({
            "process": name,
            "cpu_avg": x["cpu_sum"] / samples if samples else None,
            "cpu_peak": x["cpu_peak"] if samples else None,
            "rss_peak_bytes": int(x["rss_peak"]),
        })
    processes.sort(key=lambda x: (-(x["cpu_avg"] or 0), -x["rss_peak_bytes"]))
    return {
        "samples": count,
        "cpu_avg": cpu_sum / cpu_count if cpu_count else None,
        "cpu_peak": cpu_peak if cpu_count else None,
        "cpu_observed_samples": cpu_count,
        "rss_peak_bytes": rss_peak if rss_count else None,
        "threads_peak": threads_peak if threads_count else None,
        "processes": processes[:50],
        "raw_source": {"path": source["path"], "sha256": source["sha256"]},
        "capture_health": source["health"], "provenance": source["provenance"], "confidence": "derived",
    }


def aggregate_test_results(results: list[dict[str, Any]]) -> dict[str, Any]:
    passed = sum(1 for x in results if x.get("status") == "PASS")
    failed = sum(1 for x in results if x.get("status") == "FAIL")
    cancelled = sum(1 for x in results if x.get("status") == "CANCELLED")
    return {
        "total": len(results),
        "passed": passed,
        "failed": failed,
        "cancelled": cancelled,
        "wall_seconds_sum": sum(float(x.get("wall_seconds", 0.0)) for x in results),
        "peak_rss_bytes": max((int(x.get("peak_rss_bytes", 0)) for x in results), default=0),
        "slowest": sorted(results, key=lambda x: float(x.get("wall_seconds", 0.0)), reverse=True)[:25],
    }
