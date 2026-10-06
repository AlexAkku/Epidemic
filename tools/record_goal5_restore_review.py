"""Record only the finite reviewed M5.4.A surface and nonsemantic coordinator types.

No existing persistence decision is overwritten; canonical merge/admission are separate.
"""
import argparse
import copy
import json
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
sys.path.insert(0, str(ROOT / 'tools'))
from coverage_manifests import generate
from module_dossiers import discover_modules
from public_api_inventory import scan_modules
from record_goal5_public_delta import anchor, write

TEST = 'EngineFramework/DevelopmentInfrastructure/Tests/restore_coordination_tests.cpp'
TARGET = 'EpidemicGameFrameworkRestoreCoordinationTests'
HEADER = 'EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h'
# Exact callable identities: unknown additions are never automatically reviewed.
REVIEW = {
    'e285f5d1f80e10b8': ('DESTRUCTOR', 'unpublished Runtime prepared state is released on precommit failure'),
    '234f0caecec0d4fc': ('MUTATOR', 'precommit retry publishes both staged planes exactly once'),
    '608cd71567b9e890': ('DESTRUCTOR', 'std::has_virtual_destructor_v<IRuntimeGenerationRestorePort>'),
    '1a8ed521ea7ce2aa': ('MUTATOR', 'validation, staging and commit preparation failure preserve both live planes'),
    'a3dc32b8bd2a4236': ('CONSTRUCTOR', 'fresh restore starts with simulation disabled'),
    '262fcd1e7349a380': ('DESTRUCTOR', 'coordinator destruction releases owned restore resources without replaying semantic publication'),
    'a33f44faf4158c0d': ('CONSTRUCTOR', '!std::is_copy_constructible_v<WholeEngineRestoreCoordinator>'),
    '37eba61cf2ceda31': ('MUTATOR', '!std::is_copy_assignable_v<WholeEngineRestoreCoordinator>'),
    '01d6ad002ec4c504': ('MUTATOR', 'validation, staging and commit preparation failure preserve both live planes'),
    '36043e103825bdfb': ('MUTATOR', 'accepted external effect and semantic commit are not duplicated by retry'),
    '247cb1497531427a': ('QUERY', 'postcommit failure owns reconciliation and blocks simulation resume'),
    '635a1b921893c526': ('QUERY', 'postcommit failure owns reconciliation and blocks simulation resume'),
    '636e99b2f4570966': ('QUERY', 'fresh restore starts with simulation disabled'),
    'bb418d00f29f67bc': ('MUTATOR', 'validated dependencies remain private and retain durable values'),
    '9ca97e2efaa2a183': ('MUTATOR', 'last validation exit discards private candidates and restores live fallback'),
}
VALIDATION_HOOKS = {'bb418d00f29f67bc', '9ca97e2efaa2a183'}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true', required=True)
    parser.parse_args()
    rows = {row.id: row for row in scan_modules(discover_modules())}
    overrides_path = ROOT / 'docs/freeze/public_api_classification_overrides.json'
    overrides = json.loads(overrides_path.read_text(encoding='utf-8'))
    overrides['overrides']['36043e103825bdfb'] = {'classification': 'MUTATOR', 'reviewed': True,
        'rationale': 'Retry advances a retained postcommit phase cursor without repeating semantic publication; guarded and owner-thread-only.'}
    write(overrides_path, overrides)
    baseline = generate()
    delta = {'schema_version': 1, 'reviewed': True, 'api': {}, 'anchors': {},
             'lifecycle': {}, 'stale_identity': {}, 'external_boundary': {}}
    for identity, (classification, token) in REVIEW.items():
        row = rows[identity]
        is_hook = identity in VALIDATION_HOOKS
        test_source = 'EngineFramework/DevelopmentInfrastructure/Tests/save_participant_mechanics_tests.cpp' if is_hook else TEST
        test_target = 'EpidemicGameFrameworkSaveGameTests' if is_hook else TARGET
        expected_header = 'EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h' if is_hook else HEADER
        if row.header != expected_header: raise ValueError('reviewed callable moved to another header')
        record = copy.deepcopy(baseline['api'][identity])
        record['classification'] = classification
        record['obligations'] = {key: 'REVIEWED' for key in ('preconditions', 'success', 'failure', 'noop')} if classification == 'MUTATOR' else {}
        record['review_evidence'] = anchor(test_source, token)
        record['review_rationale'] = ('Restore validation is private and dependency ordered. No-throw Begin/End share a workspace view until the last participant exits; every exit discards candidates without changing live owners. Stage/Commit remain separate.' if is_hook else 'Explicit 5.4.A coordination-core contract: private precommit stages, no-throw publication, retained postcommit recovery lease, owner-thread-only guarded resume. Whole-engine qualification is not claimed.')
        delta['api'][identity] = record
        delta['anchors'][identity] = {
            'contract': {'reviewed': True, 'anchor': anchor(test_source, token)},
            'test': {'reviewed': True, 'anchor': anchor(test_source, token), 'assertions': [anchor(test_source, token)], 'target': test_target}}
        for section in ('lifecycle', 'stale_identity', 'external_boundary'):
            for sid, surface in baseline[section].items():
                if surface['callable_id'] == identity:
                    delta[section][sid] = dict(surface, status='REVIEWED')
    # SaveGame hooks belong to its existing Goal 4 owner handoff. Keep the
    # Integration/Runtime-only supplemental merge boundary unchanged.
    owner_path = ROOT / '_goal4_handoff/B07/coverage_reviews.json'
    anchor_path = ROOT / '_goal4_handoff/B07/public_api_anchors.json'
    owner = json.loads(owner_path.read_text(encoding='utf-8'))
    anchors = json.loads(anchor_path.read_text(encoding='utf-8'))
    if not any(r['module'] == 'EngineFramework/GameplayWorldStateOwners/SaveGame' for r in owner['api'].values()):
        raise ValueError('SaveGame owner handoff moved')
    for identity in VALIDATION_HOOKS:
        owner['api'][identity] = delta['api'].pop(identity)
        anchors['anchors'][identity] = delta['anchors'].pop(identity)
        for section in ('lifecycle', 'stale_identity', 'external_boundary'):
            for sid in list(delta[section]):
                if delta[section][sid]['callable_id'] == identity:
                    owner[section][sid] = delta[section].pop(sid)
    write(owner_path, owner)
    write(anchor_path, anchors)
    write(ROOT / '_goal5_handoff/restore_public_surface_delta.json', delta)
    admission_path = ROOT / 'docs/freeze/goal5_admission_delta.json'
    admission = json.loads(admission_path.read_text(encoding='utf-8'))
    actual = set(rows)
    expected = set(admission['baseline_api_ids']) | set(admission['added_api_ids']) | VALIDATION_HOOKS
    if actual != expected: raise ValueError('unreviewed API additions/deletions outside finite restore delta')
    admission['added_api_ids'] = sorted(set(admission['added_api_ids']) | VALIDATION_HOOKS)
    admission['stage'] = 'M5.2-M5.4'
    write(admission_path, admission)
    inventory = json.loads((ROOT / 'docs/freeze/goal5_persistence_candidates.json').read_text(encoding='utf-8'))
    reviewed_types = {
        'G5P-fe96cc7c2f37d2a7': 'epidemic::gameplay::integration::WholeEngineRestoreCoordinator',
        'G5P-b9955c6d5f6018dc': 'epidemic::gameplay::integration::WholeRestorePhase'}
    raw = {'schema_version': 1, 'candidates': {}}
    for candidate in inventory['candidates']:
        identity = candidate['candidate_id']
        if identity not in reviewed_types: continue
        if candidate['qualified_symbol'] != reviewed_types[identity]: raise ValueError('reviewed type identity drifted')
        raw['candidates'][identity] = {
            'candidate_id': identity, 'module': candidate['module'], 'qualified_symbol': candidate['qualified_symbol'],
            'persistence_role': 'NON_PERSISTENT_OWNER', 'save_plane': 'NONE', 'authoritative_owner': 'N/A',
            'capture_contract': 'N/A: process-local one-cycle coordinator, never a gameplay owner',
            'restore_contract': 'LoadAndRestore stages real owners; RetryReconciliation retains private process-local cursor',
            'durable_schema': 'N/A: process-local coordination only', 'migration_policy': 'N/A: not serialized',
            'durable_storage': 'N/A: owning state lives in the three generation payloads',
            'transient_rebuild': 'Create a fresh coordinator against the fresh composition and store',
            'reconciliation': 'Retain the recovery lease after semantic commit; retry only unfinished phases before CanResume',
            'dependencies': [], 'deterministic_relevance': 'NO', 'digest_policy': 'EXCLUDE',
            'risk_review': {flag: {'decision': 'EXCLUDED_FROM_DURABLE_REPRESENTATION',
                                   'rationale': 'Injected callbacks are fresh composition wiring; never captured as semantic state.'} for flag in candidate['risk_flags']},
            'review_status': 'REVIEWED', 'rationale': 'Phase/cursor/lease describe this process restore lifecycle; persistent owners remain exclusively assigned to their existing participants/Runtime planes.',
            'evidence': [{'kind': 'executable', 'source_path': TEST, 'test_target': TARGET,
                          'test_symbol': 'PostcommitFailureRetainsSemanticsAndRetriesOnlyUnfinishedPhases',
                          'contract_assertion': 'postcommit failure owns reconciliation and blocks simulation resume'}]}
    if set(raw['candidates']) != set(reviewed_types): raise ValueError('reviewed types absent from inventory')
    write(ROOT / '_goal5_handoff/restore_persistence_review.json', raw)
    print('PASS: exact 15 callable and two nonsemantic coordinator type reviews recorded; no whole-engine qualification inferred.')

if __name__ == '__main__': main()
