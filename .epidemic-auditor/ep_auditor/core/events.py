from __future__ import annotations

import queue
from dataclasses import dataclass
from datetime import datetime, timezone
from typing import Any


@dataclass(slots=True)
class UiEvent:
    kind: str
    payload: dict[str, Any]
    created_utc: str


class EventBus:
    def __init__(self) -> None:
        self._queue: queue.Queue[UiEvent] = queue.Queue()

    def emit(self, kind: str, **payload: Any) -> None:
        self._queue.put(UiEvent(kind, payload, datetime.now(timezone.utc).isoformat()))

    def drain(self, limit: int = 1000) -> list[UiEvent]:
        out: list[UiEvent] = []
        for _ in range(limit):
            try:
                out.append(self._queue.get_nowait())
            except queue.Empty:
                break
        return out
