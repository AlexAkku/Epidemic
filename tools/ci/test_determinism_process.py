"""Compare canonical scenario JSON across real fresh executable processes."""
import argparse
import json
import re
from pathlib import Path
import subprocess
import tempfile


def semantic(data):
    return {key: data[key] for key in ('scenario_id', 'config_id', 'descriptor_hex', 'seed_set', 'semantic_digest', 'event_trace_digest', 'normalized_trace', 'step_count')}


def validate_artifact(data):
    if data.get('status') != 'PASS' or data.get('failure') is not None:
        raise ValueError('failing reproduction artifact')
    if not re.fullmatch(r'(?:[0-9a-f]{2})+', data.get('descriptor_hex', '')):
        raise ValueError('missing/noncanonical descriptor')
    for key in ('semantic_digest', 'event_trace_digest'):
        if not re.fullmatch(r'[0-9a-f]{16}', data.get(key, '')):
            raise ValueError('missing/noncanonical ' + key)
    if not isinstance(data.get('step_count'), int) or data['step_count'] <= 0:
        raise ValueError('empty scenario')
    events = data.get('normalized_trace')
    if not isinstance(events, list) or not events:
        raise ValueError('setup-only/empty semantic trace')
    previous = 0
    for event in events:
        if not event.get('event_type') or not event.get('semantic_payload'):
            raise ValueError('empty semantic event')
        for key in ('subject', 'object'):
            if not isinstance(event.get(key), list) or len(event[key]) != 2 or any(type(value) is not int or value < 0 for value in event[key]):
                raise ValueError('invalid persistent event identity')
        if type(event.get('logical_order')) is not int or event['logical_order'] <= previous:
            raise ValueError('duplicate/unordered semantic events')
        previous = event['logical_order']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable')
    parser.add_argument('--scenario', action='append')
    parser.add_argument('--repeats', type=int, default=2)
    args = parser.parse_args()
    if args.repeats < 2: parser.error('fresh-process qualification requires at least two repeats')
    with tempfile.TemporaryDirectory(prefix='epidemic-determinism-d2-') as directory:
        for scenario in args.scenario or ['mini']:
            reference = None
            for mode in ('control', 'save_reload'):
                for index in range(args.repeats):
                    output = Path(directory) / f'{scenario}-{mode}-{index}.json'
                    process = subprocess.run([args.executable, '--emit', scenario, mode, '20261004', str(output)],
                                             capture_output=True, text=True, timeout=180)
                    if process.returncode:
                        raise RuntimeError(f'scenario={scenario} mode={mode} seed=20261004 run={index} exit={process.returncode}\n{process.stdout}\n{process.stderr}')
                    data = json.loads(output.read_text(encoding='utf-8'))
                    validate_artifact(data)
                    actual = semantic(data)
                    if reference is None: reference = actual
                    elif actual != reference:
                        expected_events, actual_events = reference['normalized_trace'], actual['normalized_trace']
                        first = next((i for i, (a, b) in enumerate(zip(expected_events, actual_events)) if a != b),
                                     min(len(expected_events), len(actual_events)))
                        raise RuntimeError(f'scenario={scenario} mode={mode} seed=20261004 run={index} '
                                           f'expected={reference["semantic_digest"]} actual={actual["semantic_digest"]} first_trace={first}')
            print(f'PASS: {scenario} D2 {args.repeats} fresh processes per control/save_reload; state and full trace exact.')


if __name__ == '__main__': main()
