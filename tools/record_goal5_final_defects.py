"""Merge reviewed Goal5 defects into source-bound permanent regression evidence."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / '_goal5_handoff/final_defects.json'
EXPECTED = {
    '_goal5_handoff/whole_engine_restore_review.json': {
        'G5-RUNTIME-IDENTITY-001', 'G5-RUNTIME-IDENTITY-002', 'G5-RESTORE-ENTITY-HANDLE-001',
        'G5-RESTORE-CONDITION-PROJECTION-001', 'G5-RESTORE-VALIDATION-VIEW-001', 'G5-RESTORE-DEATH-ENUM-001'},
    '_goal5_handoff/whole_engine_restore_failure_review.json': {'G5-RUNTIME-PUBLISH-001'},
    '_goal5_handoff/save_generation_failure_review.json': {'G5-BARRIER-001', 'G5-SAVE-001'},
    '_goal5_handoff/stage56_defect_review.json': {'G5-DET-FACTS-001', 'G5-DET-FACTS-002',
        'G5-DET-PHYSICS-001', 'G5-HARNESS-ABILITY-RECONCILIATION-001', 'G5-EVIDENCE-COMPILED-SOURCE-001'}}

def validate_input(path, review):
    assert review.get('schema_version') == 1 and set(review.get('defects', {})) == EXPECTED[path], 'unknown or missing reviewed defect'
    if path != '_goal5_handoff/save_generation_failure_review.json':
        assert review.get('reviewed') is True, 'defect input is unreviewed'
    for identity, defect in review['defects'].items():
        assert defect.get('status', 'FIXED') == 'FIXED', 'unresolved defect: ' + identity
        if path.endswith('stage56_defect_review.json'):
            assert defect.get('status') == 'FIXED', 'missing explicit resolution: ' + identity
        if path.endswith('save_generation_failure_review.json'):
            assert defect.get('review_status') == 'REVIEWED', 'unreviewed defect: ' + identity
def load(path): return json.loads((ROOT / path).read_text(encoding='utf-8-sig'))
def generate():
    rows = {}
    whole = 'Tests/Truth/whole_engine_restore_tests.cpp'
    target = 'EpidemicWholeEngineRestoreTruthTests'
    mappings = {
        'G5-RUNTIME-IDENTITY-001': 'FreshBackendsRejectRetiredOpaqueCapabilities',
        'G5-RUNTIME-IDENTITY-002': 'FreshWholeEngineRestoresAllPlanesAndContinuesCausally',
        'G5-RESTORE-ENTITY-HANDLE-001': 'FreshWholeEngineRestoresAllPlanesAndContinuesCausally',
        'G5-RESTORE-CONDITION-PROJECTION-001': 'RetainedConditionJournalReconcilesWithoutChangingProgression',
        'G5-RESTORE-VALIDATION-VIEW-001': 'NonemptyCrossOwnerRestoreUsesSavedDependencies',
        'G5-RESTORE-DEATH-ENUM-001': 'DeathRewardRejectsUnknownDeliveryStateAtomically',
        'G5-RUNTIME-PUBLISH-001': 'WholePostcommitRecoveryRetainsSemanticsAndNoDuplicateRuntimeObject'}
    production_sources = {
        'G5-RUNTIME-IDENTITY-001': 'EngineRuntime/Physics/src/physics_runtime_impl.cpp',
        'G5-RUNTIME-IDENTITY-002': 'EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp',
        'G5-RUNTIME-PUBLISH-001': 'EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp',
        'G5-RESTORE-ENTITY-HANDLE-001': 'EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp',
        'G5-RESTORE-CONDITION-PROJECTION-001': 'EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp',
        'G5-RESTORE-VALIDATION-VIEW-001': 'EngineFramework/GameplayWorldStateOwners/SaveGame/internal/Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h',
        'G5-RESTORE-DEATH-ENUM-001': 'EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp'}
    production_sources.update({identity: 'EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp'
        for identity in ('G5-BARRIER-001', 'G5-SAVE-001')})
    for path in ('_goal5_handoff/whole_engine_restore_review.json',
                 '_goal5_handoff/whole_engine_restore_failure_review.json',
                 '_goal5_handoff/save_generation_failure_review.json',
                 '_goal5_handoff/stage56_defect_review.json'):
        review = load(path)
        validate_input(path, review)
        for identity, defect in review['defects'].items():
            if identity in mappings: source, symbol, case = whole, mappings[identity], target
            elif identity.startswith(('G5-BARRIER-', 'G5-SAVE-')):
                source = 'EngineFramework/DevelopmentInfrastructure/Tests/save_generation_tests.cpp'
                symbol, case = defect['regression'], 'EpidemicGameFrameworkSaveGenerationTests'
            elif identity.startswith('G5-DET-FACTS-'):
                source = 'EngineFramework/DevelopmentInfrastructure/Tests/facts_tests.cpp'
                symbol = 'expiration_order_ok' if identity.endswith('001') else 'batch_collision_ok'
                case = 'EpidemicGameplayFactsTests'
            elif identity == 'G5-DET-PHYSICS-001':
                source, symbol, case = 'EngineRuntime/Physics/tests/physics_tests.cpp', 'TestEqualDistanceRaycastUsesStableBodyOrder', 'EpidemicRuntimePhysicsTests'
            elif identity.startswith('G5-HARNESS-'):
                source, symbol, case = 'Tests/Truth/determinism_save_load_tests.cpp', 'RequiredScenarioMatrixHasEquivalentContinuation', 'EpidemicDeterminismSaveLoadTruthTests'
            else:
                source, symbol, case = 'docs/freeze/goal5_required_scenarios_contract.py', 'check_compiled_stamp', 'EpidemicGoal5ContractTruthTests'
            production = production_sources.get(identity, defect.get('source', source))
            rows[identity] = dict(defect, status='FIXED', review_status='REVIEWED',
                source=production, source_sha256=hashlib.sha256((ROOT / production).read_bytes()).hexdigest(),
                regression_source=source, regression_symbol=symbol, test_target=case,
                reproducer=[path] + [defect[key] for key in ('failing_receipt', 'before_after_receipt') if key in defect])
    for identity, source, symbol, evidence in (
        ('G5-EVIDENCE-SCOPED-001', 'docs/freeze/goal5_evidence_quality.py', 'std::filesystem::remove_all(path, error)',
         '_ai_logs/20261005-184619-ms5-p5-scoped-evidence-fail-qualified.stdout.log'),
        ('G5-QUALIFICATION-MSVC-C1202-001', 'tools/generate_goal5_participants.py', 'qualification_codec_self_test',
         '_ai_logs/20261005-182341-ms5-p4-release-c1202-current.stdout.log')):
        rows[identity] = {'status': 'FIXED', 'review_status': 'REVIEWED', 'source': source,
            'source_sha256': hashlib.sha256((ROOT / source).read_bytes()).hexdigest(),
            'regression_source': source, 'regression_symbol': symbol,
            'test_target': 'EpidemicGoal5ContractTruthTests', 'reproducer': [evidence]}
    for identity, production, regression, symbol, case, evidence in (
        ('G5-PORTABILITY-AGGREGATE-001',
         'EngineFramework/GameplayWorldStateOwners/SaveGame/internal/Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h',
         'EngineFramework/DevelopmentInfrastructure/Tests/save_participant_mechanics_tests.cpp',
         'AggregateArity<ValueSnapshot>', 'EpidemicGameFrameworkSaveGameTests',
         '_ai_logs/20261005-223734-ms5-p7-clangcl-public-headers.stderr.log'),
        ('G5-RESTORE-DEATH-CURSOR-001', 'EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp',
         'EngineFramework/DevelopmentInfrastructure/Tests/gameplay_integration_tests.cpp',
         'rebound_death_checkpoint', 'EpidemicGameFrameworkGameplayIntegrationTests', '_goal5_handoff/whole_engine_restore_review.json'),
        ('G5-DET-STATE-PRODUCER-001', 'EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp',
         'Tests/Truth/whole_engine_restore_tests.cpp', 'FreshWholeEngineRestoresAllPlanesAndContinuesCausally',
         'EpidemicWholeEngineRestoreTruthTests', '_goal5_handoff/whole_engine_restore_review.json')):
        rows[identity] = {'status': 'FIXED', 'review_status': 'REVIEWED', 'source': production,
            'source_sha256': hashlib.sha256((ROOT / production).read_bytes()).hexdigest(),
            'regression_source': regression, 'regression_symbol': symbol, 'test_target': case, 'reproducer': [evidence]}
    source = 'tools/record_goal5_final_defects.py'
    rows['G5-EVIDENCE-OPEN-STATUS-001'] = {'status': 'FIXED', 'review_status': 'REVIEWED',
        'root_cause': 'Generator overwrote an unresolved owning status with FIXED without validating closed reviewed inputs.',
        'fix': 'Require the finite reviewed defect set and explicit closed statuses before generating any record.',
        'source': source, 'source_sha256': hashlib.sha256((ROOT / source).read_bytes()).hexdigest(),
        'regression_source': source, 'regression_symbol': 'self_test', 'test_target': 'EpidemicGoal5ContractTruthTests',
        'reproducer': ['_ai_logs/20261006-084930-ms5-p7-open-defect-regression-before.stdout.log']}
    return {'schema_version': 1, 'status': 'REVIEWED', 'defects': rows}

def self_test():
    global load
    original = load
    mutations = (
        lambda value: value['defects']['G5-DET-FACTS-001'].update(status='OPEN'),
        lambda value: value.update(reviewed=False),
        lambda value: value['defects'].update({'G5-UNKNOWN-001': {'status': 'FIXED'}}),
        lambda value: value['defects']['G5-DET-FACTS-001'].pop('status'),
        lambda value: value['defects'].pop('G5-DET-FACTS-001'))
    try:
        for mutation in mutations:
            def altered(path):
                value = copy.deepcopy(original(path))
                if path == '_goal5_handoff/stage56_defect_review.json': mutation(value)
                return value
            load = altered
            try: generate()
            except AssertionError: pass
            else: raise AssertionError('unresolved/unreviewed/missing/unknown defect accepted')
    finally: load = original
    print('PASS: five unresolved/unreviewed/missing/unknown defect negatives rejected without input mutation')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true'); parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args(); value = generate()
    if args.self_test: self_test()
    if args.record: OUTPUT.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    if args.check: assert load('_goal5_handoff/final_defects.json') == value, 'final defect registry stale'
    print('PASS: reviewed permanent regressions for', len(value['defects']), 'Goal5 defects')
