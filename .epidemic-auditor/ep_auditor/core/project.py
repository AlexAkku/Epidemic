"""Bounded CMake source-root discovery with explicit ambiguity handling."""
import os
from pathlib import Path

from .evidence import EvidenceError, digest

EXCLUDED = {".git", ".ai", ".venv", ".epidemic-auditor", "build", ".idea", "Deltas", "deleted"}


def discover_project(start: Path, choice: Path | None = None) -> dict:
    start = start.resolve()
    if not start.is_dir():
        raise EvidenceError("requested project directory does not exist")
    candidates = []
    visited = 0
    for name, directories, files in os.walk(start, followlinks=False):
        directory = Path(name)
        visited += 1
        if visited > 2000:
            raise EvidenceError("project discovery capacity exceeded; choose the exact CMake source root")
        depth = len(directory.relative_to(start).parts)
        directories[:] = sorted(child for child in directories if child not in EXCLUDED
                                and (directory / child).resolve().is_relative_to(start))
        if "CMakeLists.txt" in files:
            candidates.append(directory)
            directories[:] = []  # Nested CMake subdirectories belong to this candidate's tree.
        elif depth >= 6 and directories:
            raise EvidenceError("project discovery depth exceeded; choose the exact CMake source root")
    if not candidates:
        raise EvidenceError("no CMake source root found in requested project directory")
    if choice is not None:
        selected = choice.resolve()
        if selected not in candidates:
            raise EvidenceError("explicit project choice is not an observed CMake root")
        reason = "explicit_choice_from_ambiguous_roots" if len(candidates) > 1 else "explicit_cmake_source_root"
    elif len(candidates) == 1:
        selected = candidates[0]
        reason = "configured_cmake_source_root" if selected == start else "unique_cmake_source_root"
    else:
        raise EvidenceError("ambiguous CMake roots; explicit choice required: " + "; ".join(map(str, candidates)))
    return {"schemaVersion": 1, "requested_root": str(start), "selected_root": str(selected),
            "reason": reason, "candidates": [{"path": str(path), "cmake_sha256": digest(path / "CMakeLists.txt")}
                                              for path in candidates],
            "provenance": "external", "confidence": "observed", "visited_directories": visited}
