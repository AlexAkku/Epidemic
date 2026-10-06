"""Extend admission only with the finite reviewed 5.3.B API handoff."""
import argparse
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
from goal5_admission import validate_surface_evolution
from module_dossiers import discover_modules
from public_api_inventory import scan_modules


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true', required=True)
    parser.add_argument('--review', default='_goal5_handoff/save_generation_public_surface_delta.json',
                        choices=['_goal5_handoff/save_generation_public_surface_delta.json',
                                 '_goal5_handoff/restore_public_surface_delta.json'])
    args = parser.parse_args()
    def read(relative):
        return json.loads((ROOT / relative).read_text(encoding='utf-8-sig'))
    review = read(args.review)
    if review.get('schema_version') != 1 or review.get('reviewed') is not True:
        raise ValueError('save-generation handoff is not reviewed')
    if set(review['api']) != set(review['anchors']):
        raise ValueError('review and anchor identities differ')
    rows = {r.id: r for r in scan_modules(discover_modules())}
    coverage = read('docs/freeze/coverage_manifests.json')['api']
    for identity, record in review['api'].items():
        row = rows.get(identity)
        if row is None or row.module != record.get('module'):
            raise ValueError('unknown reviewed callable: ' + identity)
        expected = {k: v for k, v in record.items()
                    if k not in {'review_evidence', 'review_rationale'}}
        expected['obligations'] = {k: v.get('status', 'DISCOVERED') if isinstance(v, dict) else v
                                   for k, v in expected['obligations'].items()}
        if coverage.get(identity) != expected:
            raise ValueError('canonical merge must validate the reviewed evidence first: ' + identity)
    relative = 'docs/freeze/goal5_admission_delta.json'
    delta = read(relative)
    delta['added_api_ids'] = sorted(set(delta['added_api_ids']) | set(review['api']))
    delta['stage'] = 'M5.2.A-E + M5.3.B' + (' + M5.4.A' if 'restore_' in args.review else '')
    errors = validate_surface_evolution(read('docs/freeze/goal5_admission.json'), set(rows), delta)
    if errors:
        raise ValueError('\n'.join(errors))
    (ROOT / relative).write_text(json.dumps(delta, indent=2, sort_keys=True) + '\n', encoding='utf-8', newline='\n')
    print('PASS: admission extended by the reviewed 5.3.B callable identities.')


if __name__ == '__main__':
    main()
