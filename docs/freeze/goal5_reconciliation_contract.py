"""Finite, source-bound reconciliation decisions for every durable MS5 candidate."""
import copy
import hashlib
import json
import sys
from pathlib import Path
sys.dont_write_bytecode = True
from module_dossiers import ROOT
from goal5_persistence_contract import PERSISTENT_ROLES
from goal5_persistence_inventory import strip_comments, matching_brace

REVIEW = ROOT / '_goal5_handoff/reconciliation_review.json'
CORE = {'abilities', 'ai', 'combat', 'conditions', 'construction', 'crime', 'dialogue',
        'economy', 'effects', 'encounters', 'entities', 'environment', 'equipment',
        'facts', 'interaction', 'items', 'knowledge', 'loot', 'materials', 'narrative',
        'navigation_semantics', 'needs_life', 'ownership', 'perception', 'population',
        'processes', 'progression', 'resources', 'roles_jobs', 'simulation', 'society',
        'time', 'traversal', 'world'}
STRATEGIES = {'framework.' + name: 'PRIVATE_VALUE_RESTORE_REBUILDS_TRANSIENT_INDEXES' for name in CORE}
STRATEGIES.update({
    'framework.facts': 'QUIESCENT_FACT_VALUES_KEEP_FRESH_IMMUTABLE_REGISTRATIONS',
    'framework.time': 'CLOCKS_GENERATORS_AND_PENDING_TRIGGERS_RESTORED',
    'integration.ability_effects': 'DURABLE_DELIVERY_LEDGER_RETRY_BY_OWNER',
    'integration.ability_time': 'DURABLE_PENDING_OUTPUTS_RETRY_BY_OWNER',
    'integration.condition_progression': 'EPOCH_GAP_REPROJECT_WITHOUT_REPLACING_IDENTICAL_MODIFIERS',
    'integration.death_reward': 'RETAINED_COMBAT_SEQUENCE_REBOUND_WITH_DURABLE_DELIVERY_LEDGER',
    'integration.interaction_effects': 'DURABLE_EFFECT_OUTCOMES_RETRY_BY_OWNER',
    'integration.narrative.external_consequence_delivery': 'DURABLE_OUTBOX_PENDING_RECIPIENTS_RETRY_BY_OWNER',
    'integration.population_encounter': 'VALIDATED_POPULATION_ENCOUNTER_LEGS_RETRY_BY_OWNER',
    'integration.population_lifecycle': 'DURABLE_RECONCILIATION_ROWS_RETRY_BY_OWNER',
    'integration.scheduled_trigger_dispatcher': 'DURABLE_DISPATCH_STATE_WITH_FRESH_REGISTERED_HANDLERS',
    'integration.social_legal': 'REBUILT_CRIME_LIFECYCLE_CURSOR_WITH_DURABLE_PENALTY_LEDGER',
    'integration.state': 'REBIND_REBUILT_JOURNALS_RETAIN_CONDITION_DELIVERY_AND_DEFERRED_ROWS',
    'integration.trade': 'VALIDATED_MONEY_GOODS_OWNERSHIP_LEGS_RETRY_BY_OWNER',
    'integration.world_facts': 'REVISION_MATCH_REBINDS_REBUILT_JOURNALS_OTHERWISE_REJECT_GAP',
    'runtime.time': 'PRIVATE_VALIDATED_TIME_CHECKPOINT_HOST_THEN_NOEXCEPT_SWAP',
    'runtime.persistence': 'PRIVATE_VALIDATED_PERSISTENCE_HOST_THEN_NOEXCEPT_SWAP',
})

def groups(persistence):
    result = {}
    for cid, candidate in persistence['candidates'].items():
        if candidate['persistence_role'] not in PERSISTENT_ROLES:
            continue
        identity = {'EngineRuntime/Time': 'runtime.time', 'EngineRuntime/Persistence': 'runtime.persistence'}.get(
            candidate['module'], candidate['participant_id'])
        result.setdefault(identity, []).append(cid)
    return {key: sorted(value) for key, value in sorted(result.items())}

def source_scope(participant):
    import re
    symbol = participant['owner'] + '::' + participant['restore']
    matches = []
    for path in (ROOT / participant['owner_module'] / 'src').glob('*.cpp'):
        clean = strip_comments(path.read_text(encoding='utf-8'))
        for match in re.finditer(r'\b' + re.escape(symbol) + r'\s*\(', clean):
            opening = clean.find('{', match.end()); closing = matching_brace(clean, opening)
            if closing is None: raise ValueError('unclosed restore body: ' + symbol)
            matches.append({'source': path.relative_to(ROOT).as_posix(), 'symbol': symbol,
                            'body_sha256': hashlib.sha256(clean[opening:closing + 1].encode()).hexdigest()})
    if len(matches) != 1: raise ValueError('restore definition ambiguous/missing: ' + symbol)
    return matches[0]

def expected_review(persistence, participants):
    by_id = {p['id']: p for p in participants['participants']}
    if set(by_id) != set(STRATEGIES) - {'runtime.time', 'runtime.persistence'}:
        raise ValueError('finite reviewed participant set drifted')
    records = {}
    for identity, ids in groups(persistence).items():
        if identity not in STRATEGIES: raise ValueError('unreviewed reconciliation boundary: ' + identity)
        if identity in by_id:
            participant = by_id[identity]
            if not set(ids) <= set(participant['covered_candidate_ids']):
                raise ValueError('participant does not own durable candidates: ' + identity)
            record = source_scope(participant)
            record['target'] = participant['evidence'][0]['test_target']
            module = participant['owner_module']
        else:
            module = 'EngineRuntime/' + ('Time' if identity == 'runtime.time' else 'Persistence')
            path = ROOT / 'Tests/Truth/goal5_whole_composition.h'
            record = {'source': path.relative_to(ROOT).as_posix(), 'symbol': 'RuntimeRestorePort',
                      'body_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                      'target': 'EpidemicWholeEngineRestoreTruthTests'}
        # Pin helpers and DTO declarations too, not only the outer Restore call.
        modules = {module}
        if identity in by_id: modules.add(by_id[identity]['placement'])
        paths = sorted({p for scope in modules for pattern in ('*.cpp', '*.h') for p in (ROOT / scope).rglob(pattern)})
        record.update(candidate_ids=ids, strategy=STRATEGIES[identity],
                      source_hashes={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths})
        records[identity] = record
    coordination = ['EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h',
                    'EngineFramework/GameplayWorldStateOwners/SaveGame/internal/Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h',
                    'EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp',
                    'Tests/Truth/goal5_digest.h', 'Tests/Truth/goal5_whole_composition.h',
                    'Tests/Truth/whole_engine_restore_tests.cpp',
                    'EngineFramework/DevelopmentInfrastructure/Tests/save_participant_mechanics_tests.cpp']
    return {'schema_version': 1, 'stage': '5.4', 'policies': records,
            'coordination_sources': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in coordination}}

def validate_review(review, persistence, participants, sources=True):
    errors = []
    expected = expected_review(persistence, participants)
    if review.get('schema_version') != 1 or review.get('stage') != '5.4': errors.append('reconciliation schema/stage mismatch')
    if sources and review.get('coordination_sources') != expected['coordination_sources']:
        errors.append('private validation/digest/restore qualification source proof drifted')
    policies = review.get('policies', {})
    if set(policies) != set(expected['policies']): errors.append('finite reconciliation boundaries missing/unknown')
    for identity in set(policies) & set(expected['policies']):
        actual, wanted = policies[identity], expected['policies'][identity]
        keys = wanted if sources else ('candidate_ids', 'strategy', 'source', 'symbol', 'target')
        if any(actual.get(key) != wanted[key] for key in keys): errors.append('reconciliation ownership/source proof drifted: ' + identity)
        for cid in wanted['candidate_ids']:
            marker = 'M5.4:' + identity + ':' + wanted['strategy']
            if persistence['candidates'][cid]['reconciliation'] != marker:
                errors.append('durable reconciliation decision missing: ' + cid)
    return errors

def validate_current(persistence, participants):
    if not REVIEW.is_file(): return ['finite reconciliation review missing']
    errors = validate_review(json.loads(REVIEW.read_text(encoding='utf-8')), persistence, participants)
    from goal5_persistence_contract import gap_report
    if any(gap['target_stage'] == '5.4' for gap in gap_report(persistence)['gaps']):
        errors.append('stage 5.4 persistence gaps remain open')
    from goal5_participant_bindings import state_types
    if 'EntityHandle' in state_types('EngineFramework/GameplayWorldStateOwners/Entities', 'EntitySnapshot'):
        errors.append('process-local EntityHandle entered durable EntitySnapshot closure')
    return errors

def self_test(persistence, participants):
    review = expected_review(persistence, participants)
    valid = copy.deepcopy(persistence)
    for identity, record in review['policies'].items():
        for cid in record['candidate_ids']: valid['candidates'][cid]['reconciliation'] = 'M5.4:' + identity + ':' + record['strategy']
    if validate_review(review, valid, participants): raise ValueError('valid reconciliation fixture rejected')
    identity = next(iter(review['policies']))
    mutations = [lambda r: r['policies'].pop(identity),
                 lambda r: r['policies'][identity]['candidate_ids'].pop(),
                 lambda r: r['policies'][identity].update(strategy='NOTE: reviewed'),
                 lambda r: r['policies'][identity].update(body_sha256='0' * 64),
                 lambda r: r['policies'][identity].update(source_hashes={})]
    for mutation in mutations:
        bad = copy.deepcopy(review); mutation(bad)
        if not validate_review(bad, valid, participants): raise ValueError('invalid reconciliation proof accepted')
    valid['candidates'][review['policies'][identity]['candidate_ids'][0]]['reconciliation'] = 'GAP:5.4'
    if not validate_review(review, valid, participants): raise ValueError('unclosed reconciliation accepted')
    print('PASS: finite ownership, missing boundary/candidate, note-only policy, source drift and open-gap negatives')
