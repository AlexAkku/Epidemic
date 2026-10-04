"""Generate the persistence contract from the finite reviewed delta-2 handoff."""
import argparse
import copy
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
from goal5_persistence_contract import load_json, encoded, validate_contract, ctest_targets


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true', required=True)
    parser.parse_args()
    path = ROOT / 'docs/freeze/goal5_persistence_contract.json'
    contract = load_json(path)
    inventory = load_json(ROOT / 'docs/freeze/goal5_persistence_candidates.json')
    review = load_json(ROOT / '_goal5_handoff/delta2_persistence_review.json')
    if review.get('schema_version') != 1:
        raise ValueError('unsupported finite review handoff')
    discovered = {c['candidate_id']: c for c in inventory['candidates']}
    for identity, decision in review['candidates'].items():
        if identity not in discovered or decision['qualified_symbol'] != discovered[identity]['qualified_symbol']:
            raise ValueError('reviewed identity does not match discovery: ' + identity)
        if identity in contract['candidates'] and contract['candidates'][identity] != decision:
            raise ValueError('refusing to overwrite a different existing semantic decision: ' + identity)
        contract['candidates'][identity] = copy.deepcopy(decision)
    for module, decision in contract['modules'].items():
        decision['candidate_ids'] = sorted(c['candidate_id'] for c in inventory['candidates'] if c['module'] == module)
    errors = validate_contract(contract, inventory, ctest_targets())
    if errors:
        raise ValueError('\n'.join(errors))
    path.write_text(encoded(contract), encoding='utf-8', newline='\n')
    print(f"PASS: {len(review['candidates'])} explicit delta-2 source reviews admitted; {len(contract['candidates'])} candidates validated.")


if __name__ == '__main__':
    main()
