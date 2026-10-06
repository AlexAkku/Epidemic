"""Owning generator for the finite stage 5.4 reconciliation overlay."""
import argparse
import copy
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
from goal5_persistence_contract import load_json, encoded, validate_contract, ctest_targets, CONTRACT, INVENTORY
from goal5_reconciliation_contract import expected_review, validate_review, self_test, REVIEW

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--record', action='store_true')
parser.add_argument('--check', action='store_true')
parser.add_argument('--self-test', action='store_true')
args = parser.parse_args()
persistence = load_json(CONTRACT)
participants = load_json(ROOT / 'docs/freeze/goal5_participant_contract.json')
review = expected_review(persistence, participants)
updated = copy.deepcopy(persistence)
for identity, record in review['policies'].items():
    for cid in record['candidate_ids']:
        updated['candidates'][cid]['reconciliation'] = 'M5.4:' + identity + ':' + record['strategy']
entity = updated['candidates']['G5P-b1fa424e204f93c5']
if entity['qualified_symbol'] != 'epidemic::gameplay::entities::EntityService':
    raise ValueError('reviewed EntityService identity drifted')
entity['risk_review']['RUNTIME_HANDLE'] = {
    'decision': 'EXCLUDED_FROM_DURABLE_REPRESENTATION',
    'rationale': 'EntitySnapshot persists EntityId records and semantic generators, excludes EntityHandle. Create/Restore rebuild process-unique bounded handle generations; cross-owner and replacement-owner old handle regressions reject retired capabilities.'}
errors = validate_contract(updated, load_json(INVENTORY), ctest_targets()) + validate_review(review, updated, participants)
if errors: raise ValueError('\n'.join(errors))
if args.self_test: self_test(updated, participants)
if args.record:
    REVIEW.write_text(encoded(review), encoding='utf-8', newline='\n')
    CONTRACT.write_text(encoded(updated), encoding='utf-8', newline='\n')
    print('PASS: source-bound reconciliation overlay recorded for ' + str(sum(len(p['candidate_ids']) for p in review['policies'].values())) + ' durable candidates')
if args.check:
    if not REVIEW.is_file() or REVIEW.read_text(encoding='utf-8') != encoded(review) or persistence != updated:
        raise ValueError('reconciliation overlay stale; review source changes then run owning --record')
    print('PASS: exact durable candidate ownership, finite strategies, helper/DTO source fingerprints and retired EntityHandle exclusion')
if not any((args.record, args.check, args.self_test)): parser.error('choose --record, --check or --self-test')
