"""Rebase exact reviewed B01 RuntimeBridge evidence after the MS5 restore fix.

This changes source locations only, never review decisions or assertion tokens.
"""
import argparse
from pathlib import Path
from record_goal5_public_delta import read, write, rebase_source

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.record == args.check:
        parser.error('choose exactly one of --record/--check')
    for name in ('coverage_reviews.json', 'public_api_anchors.json', 'local_ready.json'):
        path = ROOT / '_goal4_handoff/B01' / name
        before = read(path)
        after = rebase_source(before)
        if args.record:
            write(path, after)
        elif before != after:
            raise ValueError('B01 reviewed source locations stale: ' + name)
    print('PASS: exact reviewed RuntimeBridge anchors preserved; source locations current.')


if __name__ == '__main__':
    main()
