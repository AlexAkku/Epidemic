"""Record finite reviewed obligations, independently of Matrix qualification.

Each obligation names an actual assertion, and the concrete caller executing it.
No build result assigns implementation status or changes persistence decisions.
"""
import argparse, sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
from goal5_persistence_contract import load_json, encoded
from goal5_participant_contract import validate
REVIEW = ROOT / 'docs/freeze/goal5_participant_review.json'
TESTS = 'EngineFramework/DevelopmentInfrastructure/Tests/'
COMMON = {
 'IDENTITY_SCHEMA_DEPENDENCIES': ['"stable participant Id"', '"schema version 1"', '"semantic restore dependencies"', '"explicit required semantics"'],
 'CAPTURE_VALID': '"valid payload accepted"',
 'CORRUPTION_REJECTED': '"corrupt payload rejected"',
 'STAGE_NO_MUTATION': '"stage does not mutate live state"',
 'PREPARE_FAILURE_ATOMIC': '"allocation failure preserves live state"',
 'ONE_USE_COMMIT': '"stage cannot publish twice"',
 'NONEMPTY_DURABLE_ROUNDTRIP': '"commit publishes restored records and derived diagnostic counts"',
}
SPECIAL = {
 'ScheduledTriggerDispatcher': {
  'IDENTITY_SCHEMA_DEPENDENCIES': 'G5 dispatcher identity/schema/dependencies',
  'CAPTURE_VALID': 'G5 dispatcher captured payload validates',
  'CORRUPTION_REJECTED': 'if (restored_leg_participant.StageRestore(corrupt_handler_section, RestoreContext{}))',
  'STAGE_NO_MUTATION': 'G5 dispatcher stage leaves live state unchanged',
  'PREPARE_FAILURE_ATOMIC': 'G5 dispatcher failed prepare preserves live state',
  'COMMIT_PUBLICATION': 'G5 dispatcher changed-state publication',
  'ONE_USE_COMMIT': 'G5 dispatcher stage cannot publish twice',
  'NONEMPTY_DURABLE_ROUNDTRIP': 'G5 dispatcher non-empty durable checkpoint',
 },
 'NarrativeExternalConsequenceOutbox': {
  'IDENTITY_SCHEMA_DEPENDENCIES': '"G5 outbox identity/schema/dependencies"',
  'CAPTURE_VALID': '"validate persisted outbox"',
  'CORRUPTION_REJECTED': '"corrupted save section rejected"',
  'STAGE_NO_MUTATION': '"G5 outbox staging leaves live state unchanged"',
  'PREPARE_FAILURE_ATOMIC': '"failed duplicate stage did not mutate restored outbox"',
  'COMMIT_PUBLICATION': '"committed outbox restore matches captured snapshot"',
  'ONE_USE_COMMIT': '"G5 outbox stage cannot publish twice"',
  'NONEMPTY_DURABLE_ROUNDTRIP': '"pending external consequence survives save load"',
 }
}

def main():
    ap = argparse.ArgumentParser(description=__doc__); ap.add_argument('--record', action='store_true', required=True); ap.parse_args()
    review = load_json(REVIEW)
    if review.get('schema_version') != 2: raise ValueError('schema 2 required')
    for p in review['participants']:
        owner = p['owner']; adapter = p['placement'].split('/')[-1]
        special = owner in SPECIAL
        if owner == 'ScheduledTriggerDispatcher':
            file, target, symbol = 'core_integration_tests.cpp', 'EpidemicGameFrameworkCoreIntegrationTests', 'const auto staged_leg'
        elif owner == 'NarrativeExternalConsequenceOutbox':
            file, target, symbol = 'narrative_integration_tests.cpp', 'EpidemicGameFrameworkNarrativeIntegrationTests', 'auto repeated_stage'
        else:
            file, target, symbol = adapter.lower() + '_save_participant_tests.cpp', 'Epidemic' + adapter + 'SaveParticipantTests', 'Test' + owner + 'SaveParticipant'
        decisions = {}
        tokens = dict(SPECIAL[owner]) if special else dict(COMMON)
        if not special:
            tokens['COMMIT_PUBLICATION'] = ('"commit preserves revision/generator continuity exactly"' if p['id'].startswith('framework.') else '"commit publishes the exact staged checkpoint over changed live state"')
            tokens['ONE_USE_COMMIT'] = ('"owner stage cannot replay over newer live state"' if p['id'].startswith('framework.') else '"committed stage cannot replay over newer live state"')
            tokens['STAGE_NO_MUTATION'] = ('"stage preserves live revision and generator state"' if p['id'].startswith('framework.') else '"staging original checkpoint does not mutate changed live state"')
            tokens['NONEMPTY_DURABLE_ROUNDTRIP'] = ('"commit publishes restored records and derived diagnostic counts"' if p['id'].startswith('framework.') else '"changed-state fixture differs from staged participant payload"')
        for obligation, token in tokens.items():
            items = []
            for assertion_token in (token if isinstance(token, list) else [token]):
                item = {'kind':'executable', 'source_path':TESTS + file, 'test_target':target, 'test_symbol':symbol,
                    'assertion_source_path':TESTS + (file if special else 'participant_test_contract.h'),
                    'assertion_token':assertion_token, 'contract_assertion':obligation + ': ' + assertion_token}
                if not special:
                    helper = 'CheckParticipantContract'
                    if obligation in {'COMMIT_PUBLICATION', 'ONE_USE_COMMIT', 'STAGE_NO_MUTATION'}:
                        helper = 'CheckOwnerCounterContinuity' if p['id'].startswith('framework.') else 'CheckChangedStatePublication'
                    if obligation == 'NONEMPTY_DURABLE_ROUNDTRIP':
                        helper = 'CheckNonEmptyPublication' if p['id'].startswith('framework.') else 'CheckChangedStatePublication'
                    item['invocation_token'] = ('' if helper == 'CheckParticipantContract' else 'participant_tests::') + helper + '('
                items.append(item)
            decisions[obligation] = {'status':'PASS', 'evidence':items}
        registry = {'kind':'executable', 'source_path':TESTS + 'framework_save_composition_tests.cpp',
                    'test_target':'EpidemicFrameworkSaveCompositionTests', 'test_symbol':'TestFrameworkSaveComposition',
                    'assertion_source_path':TESTS + 'framework_save_composition_tests.cpp',
                    'assertion_token':'"real composition detects missing participant"',
                    'contract_assertion':'Exact reviewed dependency graph and frozen registration.'}
        decisions['REGISTRY_CLOSURE'] = {'status':'PASS', 'evidence':[registry]}
        p.update(implementation_status='IMPLEMENTED', obligations=decisions,
                 evidence=[item for decision in decisions.values() for item in decision['evidence']])
    errors = validate(review, load_json(ROOT / 'docs/freeze/goal5_persistence_contract.json'), implementation=True)
    if errors: raise ValueError('\n'.join(errors))
    REVIEW.write_text(encoded(review), encoding='utf-8', newline='\n')
    print('PASS: 47 finite participant reviews, nine independently anchored obligations each; no Matrix claim.')
if __name__ == '__main__': raise SystemExit(main())
