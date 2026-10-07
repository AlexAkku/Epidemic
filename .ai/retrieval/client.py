"""Thin client for the persistent Epidemic retrieval daemon."""

from __future__ import annotations

import argparse
import json
import urllib.request
import urllib.error
from pathlib import Path


HERE = Path(__file__).resolve().parent
CONFIG = json.loads(
    (HERE / "config.json").read_text(encoding="utf-8-sig")
)

BASE = (
    f"http://{CONFIG.get('daemon_host', '127.0.0.1')}:"
    f"{int(CONFIG.get('daemon_port', 8765))}"
)


def request(method: str, route: str, payload=None):
    data = (
        None
        if payload is None
        else json.dumps(payload).encode("utf-8")
    )

    req = urllib.request.Request(
        BASE + route,
        data=data,
        method=method,
        headers={"Content-Type": "application/json"},
    )

    try:
        with urllib.request.urlopen(
            req,
            timeout=900,
        ) as response:
            return json.load(response)

    except urllib.error.HTTPError as exc:
        try:
            body = exc.read().decode("utf-8")
            payload = json.loads(body)
            raise RuntimeError(
                payload.get("error", body)
            ) from None
        except json.JSONDecodeError:
            raise RuntimeError(
                f"HTTP {exc.code}: {body}"
            ) from None

    except urllib.error.URLError as exc:
        raise RuntimeError(
            f"Retrieval daemon unavailable at {BASE}: {exc.reason}"
        ) from None


def main():
    parser = argparse.ArgumentParser()

    sub = parser.add_subparsers(
        dest="command",
        required=True,
    )

    search = sub.add_parser("search")
    search.add_argument("query")
    search.add_argument(
        "--top",
        type=int,
        choices=range(
            1,
            int(CONFIG.get("max_top", 5)) + 1,
        ),
        default=int(CONFIG.get("default_top", 3)),
    )

    sub.add_parser("health")
    sub.add_parser("refresh")

    args = parser.parse_args()

    if args.command == "search":
        result = request(
            "POST",
            "/search",
            {
                "query": args.query,
                "top": args.top,
            },
        )

    elif args.command == "refresh":
        result = request(
            "POST",
            "/refresh",
            {},
        )

    else:
        result = request(
            "GET",
            "/health",
        )

    print(
        json.dumps(
            result,
            ensure_ascii=False,
            separators=(",", ":"),
        )
    )


if __name__ == "__main__":
    main()