from __future__ import annotations

import re
import hashlib
import json
from collections import Counter
from pathlib import Path
from typing import Any

from ep_auditor.config import AuditorConfig
from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest


_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
_MARKERS = ("TODO", "FIXME", "HACK", "XXX")


def scan_sources(config: AuditorConfig) -> dict[str, Any]:
    files: list[dict[str, Any]] = []
    include_edges: Counter[tuple[str, str]] = Counter()
    markers: list[dict[str, Any]] = []
    total_lines = 0
    total_bytes = 0
    source_paths = []
    source_hashes = {}

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
                source_bytes = path.read_bytes()
                raw = source_bytes.decode('utf-8', errors='replace')
            except OSError:
                continue
            lines = raw.splitlines()
            source_paths.append(path)
            source_hashes[str(path.resolve())] = hashlib.sha256(source_bytes).hexdigest()
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
    from ep_auditor.analyzers.architecture import static_architecture
    architecture = static_architecture(source_paths, config.project_root)
    from ep_auditor.analyzers.compiler_ast import analyze_compilation_database
    database = config.resolve(config.data.get('static_scan', {}).get('compilation_database',
                              str(config.project_root / 'build/clangd/compile_commands.json')))
    filters = sorted({name.split('::')[0] for names in architecture['namespaces'].values() for name in names})
    architecture['compiler_semantics'] = analyze_compilation_database(config.project_root, database, source_paths,
                                                                     symbol_filters=filters, source_fingerprints=source_hashes)
    if any(digest(Path(path)) != fingerprint for path, fingerprint in source_hashes.items()):
        raise EvidenceError('source changed during static analysis')
    from ep_auditor.analyzers.rules import load_rules, evaluate_rules
    rules = evaluate_rules(load_rules(Path(__file__).resolve().parents[2] / 'rules'), {'architecture': architecture})
    return {
        'architecture': architecture,
        'source_sha256': source_hashes,
        'rules': rules,
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


def capture_scan(run: Path, report: dict):
    payload = json.dumps(report, sort_keys=True).encode()
    RawEvidence(run).append('static/architecture.json', payload, collector='source_dependency_analysis', version='1',
        start_ns=0, end_ns=0, health={'capacity': len(payload), 'observed': len(payload), 'dropped': 0,
                                    'incomplete_reason': None}, provenance='static_analysis', confidence='heuristic')
