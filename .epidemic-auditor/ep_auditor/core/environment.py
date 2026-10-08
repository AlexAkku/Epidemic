from __future__ import annotations

import os
import hashlib
import json
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

import psutil

from ep_auditor import __version__
from ep_auditor.config import AuditorConfig
from ep_auditor.core.evidence import EvidenceError, digest


def capture_environment(config: AuditorConfig, instrumentation_profile: str = "audit") -> dict[str, Any]:
    if instrumentation_profile not in {"audit", "audit-deep", "audit-memory", "audit-coverage", "normal-debug", "normal-release"}:
        raise EvidenceError("unknown environment instrumentation profile")
    env: dict[str, Any] = {
        "auditor_version": __version__,
        "python": sys.version,
        "python_executable": sys.executable,
        "platform": platform.platform(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "logical_cpu_count": psutil.cpu_count(logical=True),
        "physical_cpu_count": psutil.cpu_count(logical=False),
        "total_memory_bytes": psutil.virtual_memory().total,
        "project_root": str(config.project_root),
        "build_dir": str(config.build_dir),
        "build_config": config.build_config,
        "process_environment_subset": {
            key: os.environ.get(key)
            for key in ("PATH", "INCLUDE", "LIB", "LIBPATH", "CC", "CXX", "VSCMD_VER", "VisualStudioVersion")
            if os.environ.get(key) is not None
        },
        "tools": {},
        "git": {"availability": "unavailable", "reason": "git checkout/tool unavailable"},
        "schemaVersion": 1,
        "instrumentation_profile": instrumentation_profile,
        "provenance": "external",
        "confidence": "observed",
        "engine_source": source_fingerprint(config.project_root),
        "auditor_source": source_fingerprint(Path(__file__).resolve().parents[2]),
        "compiler_toolchain": compiler_toolchain(config.build_dir),
    }
    for tool in ("cmake", "ctest", "git", "cl", "clang", "clang-cl"):
        exe = shutil.which(tool)
        if not exe:
            env["tools"][tool] = {"availability": "unavailable", "path": None, "version": None,
                                   "reason": "tool not on PATH"}
            continue
        env["tools"][tool] = {"availability": "observed", "path": exe, "sha256": digest(Path(exe)),
                               "version": _tool_version(tool, config.project_root)}

    if shutil.which("git") and (config.project_root / ".git").exists():
        env["git"] = {
            "availability": "observed",
            "head": _run_text(["git", "rev-parse", "HEAD"], config.project_root),
            "branch": _run_text(["git", "rev-parse", "--abbrev-ref", "HEAD"], config.project_root),
            "status_porcelain": _run_text(["git", "status", "--porcelain=v1"], config.project_root, limit=200000),
            "submodules": _run_text(["git", "submodule", "status", "--recursive"], config.project_root, limit=200000),
        }
    return env


def source_fingerprint(root: Path) -> dict[str, Any]:
    """Hash source/build inputs, including dirty files; exclude local artifact trees."""
    excluded = {".git", ".ai", ".venv", "venv", "__pycache__", "build", ".idea", "Deltas",
                "_goal6_handoff", "deleted", ".epidemic-auditor"}
    extensions = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl", ".cmake", ".py", ".ps1", ".asm", ".yaml", ".yml"}
    files = {}
    for directory, children, names in os.walk(root):
        children[:] = sorted(name for name in children if name not in excluded)
        for name in sorted(names):
            path = Path(directory) / name
            if path.suffix.lower() in extensions or name in {"CMakeLists.txt", "CMakePresets.json", "auditor.config.json", "requirements.txt"}:
                if path.is_symlink():
                    raise EvidenceError("source fingerprint input is a symbolic link")
                files[path.relative_to(root).as_posix()] = digest(path)
    encoded = json.dumps(files, sort_keys=True, separators=(",", ":")).encode()
    return {"algorithm": "sha256", "scope": "source_and_build_inputs_excluding_local_artifacts",
            "files": files, "sha256": hashlib.sha256(encoded).hexdigest()}


def compiler_toolchain(build_dir: Path) -> dict[str, Any]:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return {"availability": "unavailable", "compiler": None, "reason": "CMakeCache missing"}
    entries = {}
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line.startswith(("#", "//")) and ":" in line and "=" in line:
            name, value = line.split("=", 1)
            entries[name.split(":", 1)[0]] = value
    compiler = entries.get("CMAKE_CXX_COMPILER") or entries.get("CMAKE_C_COMPILER")
    if not compiler or not Path(compiler).is_file():
        return {"availability": "unavailable", "compiler": compiler, "reason": "cached compiler missing",
                "cache_sha256": digest(cache)}
    return {"availability": "observed", "compiler": compiler, "compiler_sha256": digest(Path(compiler)),
            "reported_version": _run_text([compiler, "--version"], build_dir, limit=8000),
            "cache_sha256": digest(cache), "generator": entries.get("CMAKE_GENERATOR"),
            "build_type": entries.get("CMAKE_BUILD_TYPE"), "configurations": entries.get("CMAKE_CONFIGURATION_TYPES")}


def _tool_version(tool: str, cwd: Path) -> str:
    candidates = {
        "cl": [["cl"]],
        "cmake": [["cmake", "--version"]],
        "ctest": [["ctest", "--version"]],
        "git": [["git", "--version"]],
        "clang": [["clang", "--version"]],
        "clang-cl": [["clang-cl", "--version"]],
    }.get(tool, [[tool, "--version"]])
    for cmd in candidates:
        text = _run_text(cmd, cwd, limit=8000)
        if text:
            return text
    return ""


def _run_text(cmd: list[str], cwd: Path, limit: int = 50000) -> str:
    try:
        proc = subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=15, check=False)
        text = (proc.stdout or proc.stderr or "").strip()
        return text[:limit]
    except Exception as exc:
        return f"<unavailable: {exc}>"
