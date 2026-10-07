from __future__ import annotations

from typing import Any


def compare_summaries(baseline: dict[str, Any], current: dict[str, Any]) -> dict[str, Any]:
    keys = ["wall_seconds", "peak_rss_bytes", "peak_threads", "total_read_bytes", "total_write_bytes"]
    metrics: dict[str, Any] = {}
    for key in keys:
        a = float(baseline.get(key, 0) or 0)
        b = float(current.get(key, 0) or 0)
        pct = None if a == 0 else ((b - a) / a) * 100.0
        metrics[key] = {"baseline": a, "current": b, "delta": b - a, "delta_percent": pct}
    return {"metrics": metrics}
