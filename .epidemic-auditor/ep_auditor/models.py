from __future__ import annotations

from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Any


@dataclass(slots=True)
class TestCase:
    name: str
    labels: list[str] = field(default_factory=list)
    command: list[str] = field(default_factory=list)
    working_directory: str | None = None
    timeout: float | None = None
    group: str = "Ungrouped"
    index: int | None = None
    test_id: str | None = None
    executable: str | None = None
    arguments: list[str] = field(default_factory=list)
    source_location: dict[str, Any] | None = None
    build_profile: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(slots=True)
class ProcessResult:
    command: list[str]
    cwd: str
    exit_code: int | None
    started_utc: str
    finished_utc: str
    wall_seconds: float
    cancelled: bool = False
    pid: int | None = None
    stdout_lines: int = 0
    stderr_lines: int = 0
    sample_count: int = 0
    peak_rss_bytes: int = 0
    peak_processes: int = 0
    peak_threads: int = 0
    total_read_bytes: int = 0
    total_write_bytes: int = 0
    notes: list[str] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(slots=True)
class TestResult:
    test_name: str
    status: str
    exit_code: int | None
    wall_seconds: float
    run_id: str
    capture_dir: str
    labels: list[str] = field(default_factory=list)
    peak_rss_bytes: int = 0
    peak_threads: int = 0
    failure_excerpt: str = ""
    test_id: str | None = None
    compatibility_id: str | None = None
    test_run_id: str | None = None

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(slots=True)
class RunRecord:
    run_id: str
    kind: str
    title: str
    capture_mode: str
    run_dir: Path
    started_utc: str
    finished_utc: str | None = None
    status: str = "RUNNING"
    command: list[str] = field(default_factory=list)
    metadata: dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["run_dir"] = str(self.run_dir)
        return data
