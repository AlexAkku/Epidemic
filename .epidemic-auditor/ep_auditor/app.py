from __future__ import annotations

from pathlib import Path

from ep_auditor.config import load_config
from ep_auditor.core.events import EventBus
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.run_manager import RunManager
from ep_auditor.ui.main_window import AuditorWindow


def run_app(config_path: Path) -> None:
    config = load_config(config_path)
    events = EventBus()
    storage = RunStorage(config)
    manager = RunManager(config, storage, events)
    window = AuditorWindow(config, manager, storage, events)
    window.run()
