"""Refresh unchanged assertion locations in the owning Goal 4 handoff."""
import argparse
import difflib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PREFIX = 'EngineFramework/DevelopmentInfrastructure/Tests/facts_tests.cpp'
OLD = NEW = []
LINE_MAP = {}


def refresh(value):
    if isinstance(value, dict):
        return {key: refresh(item) for key, item in value.items()}
    if isinstance(value, list):
        return [refresh(item) for item in value]
    if not isinstance(value, str):
        return value
    match = re.fullmatch(re.escape(PREFIX) + r':(\d+)::(.+)', value)
    if not match:
        return value
    line, symbol = int(match.group(1)), match.group(2)
    # Only unchanged Git lines can move. This also resolves repeated assertions
    # without guessing by nearest occurrence or altering their meaning.
    if 1 <= line <= len(NEW) and symbol in NEW[line - 1]:
        return value
    mapped = LINE_MAP.get(line)
    if mapped is None or symbol not in OLD[line - 1] or symbol not in NEW[mapped - 1]:
        raise ValueError(f'Facts evidence is not an unchanged source line: {value}')
    return f'{PREFIX}:{mapped}::{symbol}'


def main():
    global PREFIX, OLD, NEW, LINE_MAP
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--source', default=PREFIX)
    args = parser.parse_args()
    PREFIX = args.source
    OLD = subprocess.run(['git', 'show', 'HEAD:' + PREFIX], cwd=ROOT,
                         check=True, capture_output=True, text=True).stdout.splitlines()
    NEW = (ROOT / PREFIX).read_text(encoding='utf-8').splitlines()
    LINE_MAP = {old + i + 1: new + i + 1
                for tag, old, end, new, _ in difflib.SequenceMatcher(None, OLD, NEW, autojunk=False).get_opcodes()
                if tag == 'equal' for i in range(end - old)}
    changed = 0
    updates = []
    for path in sorted((ROOT / '_goal4_handoff').glob('B*/*.json')):
        before = json.loads(path.read_text(encoding='utf-8-sig'))
        after = refresh(before)
        if after != before:
            if not args.record:
                raise ValueError(f'stale Facts source evidence: {path}; use --record')
            updates.append((path, after))
    for path, after in updates:
        path.write_text(json.dumps(after, indent=2, ensure_ascii=False) + '\n', encoding='utf-8', newline='\n')
        changed += 1
    print(f'PASS: Facts handoff assertion anchors exact; {changed} records refreshed')


if __name__ == '__main__':
    main()
