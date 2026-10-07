from __future__ import annotations

from pathlib import Path
import json
import time

from ep_auditor.config import AuditorConfig
from ep_auditor.core.process import ManagedProcessRunner, LogCallback
from ep_auditor.models import ProcessResult
from ep_auditor.core.evidence import EvidenceError, RawEvidence, atomic_json, load_json
from ep_auditor.core.lifecycle import profile_for_mode
from ep_auditor.core.build_receipts import record_build, request_file_api


class BuildController:
    def __init__(self, config: AuditorConfig, runner: ManagedProcessRunner) -> None:
        self.config = config
        self.runner = runner

    def configure(self, run_dir: Path, capture_mode: str, log: LogCallback | None = None) -> ProcessResult:
        selection = self.config.admit_project()
        config = self.config.for_mode(capture_mode)
        profile = profile_for_mode(capture_mode)
        for argument in config.configure_args:
            if (argument.startswith(("-S", "-B", "--preset", "--build", "-DEP_AUDITOR_PROFILE"))):
                raise EvidenceError("configure arguments override owned source/build/profile")
        self._admit_build(config, profile, create=True)
        self._capture_project(run_dir, selection)
        request_file_api(config.build_dir)
        cmd = ["cmake", "-S", str(config.project_root), "-B", str(config.build_dir),
               *config.configure_args, "-DEP_AUDITOR_PROFILE:STRING=" + profile]
        start_ns = time.perf_counter_ns()
        result = self.runner.run(cmd, config.project_root, run_dir, capture_mode, log_callback=log)
        record_build(config, run_dir, capture_mode, "configure", result, start_ns, time.perf_counter_ns())
        return result

    def build(self, run_dir: Path, capture_mode: str, log: LogCallback | None = None) -> ProcessResult:
        selection = self.config.admit_project()
        config = self.config.for_mode(capture_mode)
        self._admit_build(config, profile_for_mode(capture_mode), create=False)
        self._capture_project(run_dir, selection)
        cmd = ["cmake", "--build", str(config.build_dir), "--config", config.build_config]
        if config.parallel > 0:
            cmd += ["--parallel", str(config.parallel)]
        else:
            cmd += ["--parallel"]
        cache = (config.build_dir / "CMakeCache.txt").read_text(encoding="utf-8")
        if "CMAKE_GENERATOR:INTERNAL=Visual Studio" in cache:
            cmd += ["--", "/clp:PerformanceSummary"]
        start_ns = time.perf_counter_ns()
        result = self.runner.run(cmd, config.project_root, run_dir, capture_mode, log_callback=log)
        record_build(config, run_dir, capture_mode, "build", result, start_ns, time.perf_counter_ns())
        return result

    @staticmethod
    def _capture_project(run_dir: Path, selection: dict) -> None:
        payload = json.dumps(selection, sort_keys=True, allow_nan=False).encode()
        now = time.perf_counter_ns()
        RawEvidence(run_dir).append("project/selection.json", payload, collector="cmake.source_root_discovery",
                                   version="1", start_ns=now, end_ns=now,
                                   health={"capacity": max(1, len(payload)), "observed": len(payload), "dropped": 0,
                                           "incomplete_reason": None})

    @staticmethod
    def _admit_build(config: AuditorConfig, profile: str, *, create: bool) -> None:
        directory = config.build_dir
        owner = directory / ".auditor_build_owner.json"
        identity = {"schemaVersion": 1, "project_root": str(config.project_root), "profile": profile}
        if owner.is_file():
            if load_json(owner) != identity:
                raise EvidenceError("build directory belongs to another source/profile")
        elif not create or (directory / "CMakeCache.txt").exists():
            raise EvidenceError("build directory lacks Auditor ownership; normal cache cannot be adopted")
        else:
            directory.mkdir(parents=True, exist_ok=True)
            atomic_json(owner, identity)
