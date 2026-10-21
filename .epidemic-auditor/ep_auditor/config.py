from __future__ import annotations

import json
from copy import deepcopy
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(slots=True)
class AuditorConfig:
    config_path: Path
    data: dict[str, Any]
    backend_operation: dict[str, Any] | None = None

    @property
    def auditor_root(self) -> Path:
        return self.config_path.parent.resolve()

    def resolve(self, value: str | Path) -> Path:
        path = Path(value)
        if path.is_absolute():
            return path.resolve()
        return (self.auditor_root / path).resolve()

    @property
    def project_root(self) -> Path:
        return self.resolve(self.data.get("project_root", ".."))

    def admit_project(self, *, start: Path | None = None, choice: Path | None = None) -> dict:
        from ep_auditor.core.project import discover_project
        from ep_auditor.core.evidence import digest
        requested = start.resolve() if start is not None else self.project_root
        previous = self.data.get("project_selection")
        if previous and start is None and choice is None and previous["selected_root"] == str(requested):
            selected = next(row for row in previous["candidates"] if row["path"] == str(requested))
            if (requested / "CMakeLists.txt").is_file() and digest(requested / "CMakeLists.txt") == selected["cmake_sha256"]:
                return deepcopy(previous)
        selection = discover_project(requested, choice)
        self.data["project_root"] = selection["selected_root"]
        self.data["project_selection"] = deepcopy(selection)
        return selection

    @property
    def build_dir(self) -> Path:
        return self.resolve(self.data.get("build_dir", "../build/audit"))

    def for_mode(self, mode: str) -> AuditorConfig:
        from ep_auditor.core.lifecycle import PROFILES, profile_for_mode
        profile = profile_for_mode(mode)
        base = self.build_dir
        if base.name in set(PROFILES.values()):
            directory = base.parent / profile
        else:
            directory = base if profile == "audit" else base / profile
        data = deepcopy(self.data)
        data["build_dir"] = str(directory)
        data.setdefault("capture", {})["selected_mode"] = mode
        return AuditorConfig(self.config_path, data, deepcopy(self.backend_operation))

    @property
    def runs_dir(self) -> Path:
        return self.resolve(self.data.get("storage", {}).get("runs_dir", str(self.project_root / ".ai/auditor/runs")))

    @property
    def database_path(self) -> Path:
        return self.resolve(self.data.get("storage", {}).get("database", str(self.project_root / ".ai/auditor/auditor.sqlite3")))

    @property
    def build_config(self) -> str:
        return str(self.data.get("cmake", {}).get("build_config", "Debug"))

    @property
    def configure_args(self) -> list[str]:
        return [str(x) for x in self.data.get("cmake", {}).get("configure_args", [])]

    @property
    def parallel(self) -> int:
        value = self.data.get('cmake', {}).get('parallel', 2)
        if type(value) is not int or value < 0:
            from ep_auditor.core.evidence import EvidenceError
            raise EvidenceError('compiler parallelism must be a nonnegative integer')
        return value or 2

    @property
    def test_timeout(self) -> float:
        return float(self.data.get("tests", {}).get("timeout_seconds", 300))

    @property
    def ctest_args(self) -> list[str]:
        return [str(x) for x in self.data.get("tests", {}).get("ctest_args", [])]

    @property
    def sample_interval_seconds(self) -> float:
        ms = max(25, int(self.data.get("capture", {}).get("sample_interval_ms", 200)))
        return ms / 1000.0

    @property
    def enable_wpr_in_deep(self) -> bool:
        return bool(self.data.get("capture", {}).get("enable_wpr_in_deep", True))

    @property
    def wpr_profile(self) -> str:
        return str(self.data.get("capture", {}).get("wpr_profile", "GeneralProfile"))

    @property
    def run_scenario(self) -> dict:
        from ep_auditor.core.evidence import EvidenceError
        run = self.data.get('run', {})
        selected = run.get('selected_scenario')
        if selected is None:
            return {'id': 'unspecified', 'kind': 'unspecified', 'content_version': 'unspecified',
                    'command': run.get('command', []), 'provenance': 'declared_configuration'}
        scenarios = run.get('scenarios', {})
        if not isinstance(selected, str) or selected not in scenarios:
            raise EvidenceError('run.selected_scenario does not name a configured scenario')
        row = scenarios[selected]
        if not isinstance(row, dict) or row.get('kind') not in {'current', 'legacy'}:
            raise EvidenceError('scenario kind must be current or legacy')
        if row['kind'] == 'legacy' and run.get('allow_legacy') is not True:
            raise EvidenceError('legacy scenario requires explicit run.allow_legacy=true')
        version, command = row.get('content_version'), row.get('command')
        if not isinstance(version, str) or not version.strip():
            raise EvidenceError('scenario requires a declared content_version')
        if not isinstance(command, list) or not command or any(not isinstance(x, str) for x in command):
            raise EvidenceError('scenario requires a nonempty string command list')
        return {'id': selected, 'kind': row['kind'], 'content_version': version,
                'command': command, 'provenance': 'declared_configuration'}

    @property
    def run_command(self) -> list[str]:
        return [self._run_value(x) for x in self.run_scenario['command']]

    def _run_value(self, value) -> str:
        # Explicit placeholders follow the admitted physical profile; no target
        # discovery or rewriting of ordinary executable paths is performed.
        return str(value).replace("{build_dir}", self.build_dir.as_posix())

    @property
    def run_working_dir(self) -> Path:
        return self.resolve(self._run_value(self.data.get("run", {}).get("working_dir", "..")))

    @property
    def run_environment(self) -> dict[str, str]:
        return {str(k): self._run_value(v) for k, v in self.data.get("run", {}).get("environment", {}).items()}

    @property
    def static_roots(self) -> list[Path]:
        return [self.resolve(x) for x in self.data.get("static_scan", {}).get("roots", [])]

    @property
    def static_extensions(self) -> set[str]:
        return {str(x).lower() for x in self.data.get("static_scan", {}).get("extensions", [])}

    @property
    def static_exclude(self) -> set[str]:
        return {"deleted"} | {str(x).lower() for x in self.data.get("static_scan", {}).get("exclude", [])}

    def save(self) -> None:
        self.config_path.write_text(json.dumps(self.data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def load_config(path: Path) -> AuditorConfig:
    if not path.exists():
        raise FileNotFoundError(f"Auditor config not found: {path}")
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    cfg = AuditorConfig(path.resolve(), data)
    cfg.runs_dir.mkdir(parents=True, exist_ok=True)
    cfg.database_path.parent.mkdir(parents=True, exist_ok=True)
    return cfg
