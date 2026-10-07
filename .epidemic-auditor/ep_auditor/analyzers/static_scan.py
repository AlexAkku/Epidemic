from __future__ import annotations

import re
from collections import Counter
from pathlib import Path
from typing import Any

from ep_auditor.config import AuditorConfig


_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
_MARKERS = ("TODO", "FIXME", "HACK", "XXX")


def scan_sources(config: AuditorConfig) -> dict[str, Any]:
    files: list[dict[str, Any]] = []
    include_edges: Counter[tuple[str, str]] = Counter()
    markers: list[dict[str, Any]] = []
    total_lines = 0
    total_bytes = 0

    for root in config.static_roots:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in config.static_extensions:
                continue
            lowered_parts = {p.lower() for p in path.parts}
            if lowered_parts & config.static_exclude:
                continue
            try:
                raw = path.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            lines = raw.splitlines()
            rel = _relative(path, config.project_root)
            total_lines += len(lines)
            size = path.stat().st_size
            total_bytes += size
            includes: list[str] = []
            for lineno, line in enumerate(lines, start=1):
                match = _INCLUDE_RE.match(line)
                if match:
                    target = match.group(1)
                    includes.append(target)
                    include_edges[(rel, target)] += 1
                upper = line.upper()
                for marker in _MARKERS:
                    if marker in upper:
                        markers.append({
                            "file": rel,
                            "line": lineno,
                            "marker": marker,
                            "text": line.strip()[:500],
                            "provenance": "external_static_scan",
                        })
            files.append({
                "path": rel,
                "lines": len(lines),
                "bytes": size,
                "include_count": len(includes),
            })

    files.sort(key=lambda x: (-x["lines"], x["path"]))
    edges = [
        {"source": src, "target": dst, "count": count, "provenance": "external_static_scan"}
        for (src, dst), count in include_edges.most_common()
    ]
    return {
        "provenance": "external_static_scan",
        "files": len(files),
        "lines": total_lines,
        "bytes": total_bytes,
        "marker_count": len(markers),
        "markers": markers,
        "largest_files": files[:100],
        "include_edges": edges,
        "scanned_roots": [str(x) for x in config.static_roots],
    }


def _relative(path: Path, root: Path) -> str:
    try:
        return str(path.resolve().relative_to(root.resolve())).replace("\\", "/")
    except ValueError:
        return str(path.resolve()).replace("\\", "/")
