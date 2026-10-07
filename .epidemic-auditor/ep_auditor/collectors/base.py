from __future__ import annotations

from pathlib import Path


class Collector:
    name = "collector"

    def start(self, pid: int, run_dir: Path) -> None:
        raise NotImplementedError

    def stop(self) -> None:
        raise NotImplementedError
