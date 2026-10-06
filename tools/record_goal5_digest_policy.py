"""Apply only the finite reviewed 5.5.A digest decisions; never change 5.1 ownership."""
import argparse
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
from goal5_persistence_contract import load_json, encoded, gap_report
from goal5_determinism_contract import generate, validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if not args.record: parser.error('explicit --record required')
    path = ROOT / 'docs/freeze/goal5_persistence_contract.json'
    contract = load_json(path)
    expected = generate(load_json(ROOT / '_goal5_handoff/digest_domain_review.json'), contract,
                        load_json(ROOT / 'docs/freeze/goal5_participant_contract.json'))
    errors = validate(load_json(ROOT / 'docs/freeze/goal5_determinism_contract.json'), expected)
    if errors: raise ValueError('\n'.join(errors))
    reviewed = {r['state_id'] for r in expected['states'] if r['decision'] == 'INCLUDED'}
    for identity in reviewed:
        record = contract['candidates'][identity]
        if record['digest_policy'] not in {'DEFER_TO_5_5', 'INCLUDE'}:
            raise ValueError('refusing unreviewed digest policy overwrite: ' + identity)
        record['digest_policy'] = 'INCLUDE'
        evidence = {'kind': 'executable', 'source_path': 'Tests/Truth/whole_engine_restore_tests.cpp',
                    'test_target': 'EpidemicWholeEngineRestoreTruthTests',
                    'test_symbol': 'FreshWholeEngineRestoresAllPlanesAndContinuesCausally',
                    'contract_assertion': 'engine.SemanticState().Digest() == continued_semantic_digest'}
        if evidence not in record['evidence']: record['evidence'].append(evidence)
    path.write_text(encoded(contract), encoding='utf-8', newline='\n')
    (ROOT / 'docs/freeze/goal5_persistence_gaps.json').write_text(encoded(gap_report(contract)), encoding='utf-8', newline='\n')
    print(f'PASS: {len(reviewed)} reviewed digest policies recorded; ownership/roles/dependencies unchanged; execution not inferred.')


if __name__ == '__main__': main()
