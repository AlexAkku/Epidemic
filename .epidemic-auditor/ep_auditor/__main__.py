from __future__ import annotations

import argparse
from pathlib import Path

from ep_auditor.app import run_app


def main() -> None:
    parser = argparse.ArgumentParser(description="Epidemic Auditor")
    parser.add_argument("--config", default="auditor.config.json", help="Path to auditor configuration JSON")
    args = parser.parse_args()
    run_app(Path(args.config).resolve())


if __name__ == "__main__":
    main()
