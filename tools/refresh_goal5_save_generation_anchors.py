"""Refresh only source locations of the finite reviewed generation API handoff."""
import argparse
import json
import re
from pathlib import Path

from record_goal5_public_delta import anchor

ROOT = Path(__file__).resolve().parents[1]
REVIEW = ROOT / '_goal5_handoff/save_generation_public_surface_delta.json'


def refresh(value, field=''):
    if isinstance(value, dict):
        return {key: refresh(item, key) for key, item in value.items()}
    if isinstance(value, list):
        return [refresh(item, field) for item in value]
    if field not in {'anchor', 'assertions', 'review_evidence'} or not isinstance(value, str):
        return value
    match = re.fullmatch(r'([^\s:]+):\d+::(.+)', value)
    if not match:
        return value
    # Unlike nearest-line repair, this requires exactly one existing assertion.
    return anchor(*match.groups())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if not args.record and not args.check:
        parser.error('choose --record or --check')
    before = json.loads(REVIEW.read_text(encoding='utf-8-sig'))
    if before.get('schema_version') != 1 or before.get('reviewed') is not True:
        raise ValueError('only an already reviewed finite handoff can be refreshed')
    after = refresh(before)
    if args.record:
        REVIEW.write_text(json.dumps(after, indent=2) + '\n', encoding='utf-8', newline='\n')
    elif before != after:
        raise ValueError('generation assertion locations are stale; run --record')
    print('PASS: finite generation anchor locations current; semantic decisions unchanged')


if __name__ == '__main__':
    main()
