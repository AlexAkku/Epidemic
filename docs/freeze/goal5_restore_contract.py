"""Generate/check the production-derived M5.4 restore graph; never infer whole-engine qualification."""
from __future__ import annotations
import argparse
import copy
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'docs/freeze/goal5_restore_contract.json'
REVIEW = ROOT / '_goal5_handoff/restore_contract_review.json'
PERSISTENCE = ROOT / 'docs/freeze/goal5_persistence_contract.json'
PARTICIPANTS = ROOT / 'docs/freeze/goal5_participant_contract.json'
WHOLE_REVIEW = ROOT / '_goal5_handoff/whole_engine_restore_review.json'
FAILURE_REVIEW = ROOT / '_goal5_handoff/whole_engine_restore_failure_review.json'
PHASES = ('LOAD', 'VALIDATE', 'STAGE', 'COMMIT_SEMANTIC', 'REBUILD_TRANSIENT',
          'RECONCILE_EXTERNAL', 'MATERIALIZE_RUNTIME', 'RESUME')
PERSISTENT = {'PERSISTENT_OWNER', 'PERSISTENT_CHECKPOINT'}

def read(path):
    from goal5_persistence_contract import load_json
    return load_json(path)

def generate(review, persistence, participants):
    if review.get('reviewed') is not True or review.get('stage') != '5.4.A':
        raise ValueError('explicit reviewed restore policy required')
    nodes = []
    for participant in participants['participants']:
        nodes.append({'id': participant['id'], 'kind': 'FRAMEWORK_PARTICIPANT',
                      'dependencies': participant['dependencies'],
                      'candidate_ids': participant['covered_candidate_ids']})
    for identity, module in [('runtime.time', 'EngineRuntime/Time'),
                             ('runtime.persistence', 'EngineRuntime/Persistence')]:
        nodes.append({'id': identity, 'kind': 'RUNTIME_PREPARED_PLANE', 'dependencies': [],
                      'candidate_ids': sorted(cid for cid, candidate in persistence['candidates'].items()
                                              if candidate['module'] == module and candidate['persistence_role'] in PERSISTENT)})
    semantic_ids = sorted(node['id'] for node in nodes)
    nodes.extend([
        {'id': 'restore.rebuild_transient', 'kind': 'POSTCOMMIT', 'dependencies': semantic_ids, 'candidate_ids': []},
        {'id': 'restore.reconcile_external', 'kind': 'POSTCOMMIT', 'dependencies': ['restore.rebuild_transient'], 'candidate_ids': []},
        {'id': 'restore.runtime_bridge', 'kind': 'POSTCOMMIT', 'dependencies': ['restore.reconcile_external'],
         'candidate_ids': sorted(cid for cid, c in persistence['candidates'].items() if c['module'].endswith('/RuntimeBridge'))},
        {'id': 'restore.resume', 'kind': 'RESUME_GUARD', 'dependencies': ['restore.runtime_bridge'], 'candidate_ids': []},
    ])
    result = copy.deepcopy(review)
    result.pop('reviewed')
    result['generated_from'] = '_goal5_handoff/restore_contract_review.json + goal5_participant_contract.json + goal5_persistence_contract.json'
    result['nodes'] = sorted(nodes, key=lambda node: node['id'])
    return result

def validate(data, persistence, participants, source_checks=True):
    errors = []
    def require(condition, message):
        if not condition: errors.append(message)
    require(data.get('schema_version') == 1 and data.get('stage') == '5.4.A', 'restore schema/stage mismatch')
    require(data.get('qualification') == 'COORDINATION_CORE_ONLY', '5.4.A must not claim whole-engine execution')
    require(set(data.get('phase_mapping', {})) == set(PHASES), 'missing or unknown restore phase')
    require(all(isinstance(value, str) and len(value) > 30 for value in data.get('phase_mapping', {}).values()), 'empty equivalent phase mapping')
    nodes = data.get('nodes', [])
    by_id = {node['id']: node for node in nodes}
    require(len(by_id) == len(nodes), 'duplicate restore node')
    expected_participants = {p['id']: p for p in participants['participants']}
    actual = {n['id']: n for n in nodes if n['kind'] == 'FRAMEWORK_PARTICIPANT'}
    require(set(actual) == set(expected_participants), 'participant graph coverage differs from production contract')
    for identity in set(actual) & set(expected_participants):
        node, participant = actual[identity], expected_participants[identity]
        require(node['dependencies'] == participant['dependencies'], 'participant dependencies differ from production: ' + identity)
        require(node['candidate_ids'] == participant['covered_candidate_ids'], 'participant candidate coverage differs: ' + identity)
    expected_durable = {cid for cid, candidate in persistence['candidates'].items() if candidate['persistence_role'] in PERSISTENT}
    for node in nodes:
        require(set(node['candidate_ids']) <= set(persistence['candidates']), 'unknown restore candidate')
    covered = [cid for node in nodes for cid in node['candidate_ids'] if cid in expected_durable]
    require(set(covered) == expected_durable and len(covered) == len(set(covered)), 'durable coverage missing or overlapping')
    for identity, module in [('runtime.time', 'EngineRuntime/Time'), ('runtime.persistence', 'EngineRuntime/Persistence')]:
        node = by_id.get(identity, {})
        expected = {cid for cid, c in persistence['candidates'].items() if c['module'] == module and c['persistence_role'] in PERSISTENT}
        require(node.get('kind') == 'RUNTIME_PREPARED_PLANE' and set(node.get('candidate_ids', [])) == expected and bool(expected), 'missing Runtime prepared plane: ' + identity)
    visiting, visited = set(), set()
    def visit(identity):
        if identity in visiting:
            errors.append('restore graph cycle'); return
        if identity in visited: return
        visiting.add(identity)
        for dependency in by_id[identity]['dependencies']:
            if dependency not in by_id: errors.append('unknown restore dependency: ' + dependency)
            else: visit(dependency)
        visiting.remove(identity); visited.add(identity)
    for identity in by_id: visit(identity)
    require(all(data.get('precommit', {}).get(key) for key in ('fallible_preparation', 'publication', 'failure_policy')), 'precommit atomicity policy missing')
    required = {'failure_policy': 'SEMANTIC_RETAINED_RECONCILIATION_REQUIRED',
                'retry_policy': 'COMPLETED_PHASE_CURSOR_WITH_IDEMPOTENT_IN_PHASE_RETRY',
                'resume_policy': 'ALL_PHASES_COMPLETE_AND_QUIESCENCE_RELEASED',
                'thread_policy': 'OWNER_THREAD_ONLY',
                'runtime_bridge_policy': 'REBUILD_FROM_SEMANTIC_IDENTITIES_REJECT_OLD_RUNTIME_OBSERVATIONS'}
    require(data.get('postcommit') == required, 'postcommit/retry/resume/RuntimeBridge policy mismatch')
    semantic = sorted(p['id'] for p in participants['participants']) + ['runtime.persistence', 'runtime.time']
    for identity, dependencies in [('restore.rebuild_transient', sorted(semantic)),
                                   ('restore.reconcile_external', ['restore.rebuild_transient']),
                                   ('restore.runtime_bridge', ['restore.reconcile_external']),
                                   ('restore.resume', ['restore.runtime_bridge'])]:
        require(by_id.get(identity, {}).get('dependencies') == dependencies, 'postcommit order/resume guard graph mismatch: ' + identity)
    if source_checks:
        from goal5_reconciliation_contract import validate_current
        errors.extend(validate_current(persistence, participants))
        source = (ROOT / 'EngineFramework/IntegrationLayer/Integration/src/save_restore.cpp').read_text(encoding='utf-8')
        header = (ROOT / 'EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h').read_text(encoding='utf-8')
        for token in ('runtime_.Prepare(', 'framework_.Restore(', 'runtime_stage.Value()->Commit();',
                      'lease_ = std::move(lease.Value());', '++completed_steps_;', 'lease_.reset();',
                      'WholeRestorePhase::ReconciliationRequired', '!OnOwnerThread()'):
            require(token in source, 'missing production restore boundary: ' + token)
        require('OperationGuard operation(operation_active_);' in source and 'operation_active_ ||' in source,
                'provider reentrancy guard missing')
        require('phase_ == WholeRestorePhase::ReadyToResume && !lease_' in header, 'production resume guard missing')
        require('virtual void Commit() noexcept = 0;' in header, 'Runtime prepared commit must be noexcept')
        evidence = data['evidence']
        test = (ROOT / evidence['source']).read_text(encoding='utf-8')
        main = test[test.index('int main()'):]
        for key in ('precommit_symbol', 'postcommit_symbol'):
            require('bool ' + evidence[key] + '()' in test and evidence[key] + '()' in main, 'restore proof is not executed: ' + key)
        for symbol in ('DestroyingStoppedCompositionReleasesRestoreOwnership', 'StoreCallbackCannotReenterRestoreBeforeQuiescence'):
            require('bool ' + symbol + '()' in test and symbol + '()' in main, 'restore lifecycle proof is not executed: ' + symbol)
        for assertion in ('preserve both live planes', 'retains restored semantic state in both planes',
                          'blocks simulation resume', 'not duplicated by retry', 'wrong-thread calls'):
            require(assertion in test, 'missing restore behavior assertion: ' + assertion)
        cmake = (ROOT / 'EngineFramework/DevelopmentInfrastructure/Tests/CMakeLists.txt').read_text(encoding='utf-8')
        require('epidemic_add_test(' + evidence['target'] + ')' in cmake, 'restore proof target not registered')
    return errors

def encoded(data): return json.dumps(data, indent=2, sort_keys=True) + '\n'

def executable_source(text):
    # Keep literals intact: fault-point strings are executable test inputs.
    return re.sub(r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|//[^\n]*|/\*.*?\*/',
                  lambda match: match.group(1) or '', text, flags=re.S)

def function_body(text, symbol):
    from goal5_persistence_inventory import matching_brace
    text = executable_source(text)
    match = re.search(r'\b' + re.escape(symbol) + r'\s*\([^;{}]*\)\s*(?:noexcept\s*)?(?:->[^{}]+)?\{', text)
    if not match: return ''
    start = text.index('{', match.start())
    end = matching_brace(text, start)
    return text[match.start():end + 1] if end is not None else ''

def registered_case(text, symbol):
    return bool(re.search(r'\{\s*"(?:\\.|[^"\\])*"\s*,\s*' + re.escape(symbol) + r'\s*\}', function_body(text, 'main')))

def scoped_token(token, text):
    literals = {'restore.validate', 'restore.stage', 'restore.pre_commit', 'reconcile.accepted',
                'materialize.world_accepted', 'materialize.queued', 'materialize.accepted',
                'bridge.prepare', '--write', '--read', 'WHOLE_FINGERPRINT'}
    text = executable_source(text)
    if token.startswith('"') or token in literals: return token in text
    code = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '', text)
    return token in code

def validate_whole_source(review, source_checks=True):
    errors = []
    def require(condition, message):
        if not condition: errors.append(message)
    require(review.get('schema_version') == 1 and review.get('stage') == '5.4.B' and review.get('reviewed') is True, 'whole-engine finite review missing')
    required = {'REAL_NONEMPTY_THREE_PLANE_SEED', 'PRODUCTION_SAVE_STORAGE', 'COMPLETE_OLD_COMPOSITION_DESTRUCTION',
                'FRESH_RESTORE_ALL_PHASES', 'IDENTITY_GENERATORS_REVISIONS_PENDING_TIME',
                'RUNTIMEBRIDGE_REBUILD_STALE_OBSERVATION', 'CAUSAL_CONTINUATION_EXACTLY_ONCE'}
    require(set(review.get('obligations', {})) == required, 'whole-engine typed obligations incomplete')
    require(review.get('execution_receipt_required') is True and bool(review.get('remaining_qualification')), 'source review must not invent execution qualification')
    require(set(review.get('defects', {})) == {'G5-RUNTIME-IDENTITY-001', 'G5-RUNTIME-IDENTITY-002', 'G5-RESTORE-ENTITY-HANDLE-001', 'G5-RESTORE-CONDITION-PROJECTION-001', 'G5-RESTORE-VALIDATION-VIEW-001', 'G5-RESTORE-DEATH-ENUM-001'}, 'restore defects lack explicit review')
    for defect in review.get('defects', {}).values():
        require(all(isinstance(defect.get(key), str) and len(defect[key]) > 30 for key in ('root_cause', 'fix', 'regression')), 'identity defect review incomplete')
    for tokens in review.get('obligations', {}).values():
        require(isinstance(tokens, list) and len(tokens) >= 2 and all(isinstance(t, str) and t.strip() for t in tokens), 'empty/duplicate generic whole-engine evidence')
        require(len(tokens) == len(set(tokens)), 'duplicate whole-engine evidence')
    if source_checks:
        test = (ROOT / review['source']).read_text(encoding='utf-8')
        fixture = (ROOT / review['composition_source']).read_text(encoding='utf-8')
        happy = function_body(test, 'FreshWholeEngineRestoresAllPlanesAndContinuesCausally')
        scopes = {
            'REAL_NONEMPTY_THREE_PLANE_SEED': happy + function_body(fixture, 'Seed'),
            'PRODUCTION_SAVE_STORAGE': happy,
            'COMPLETE_OLD_COMPOSITION_DESTRUCTION': happy,
            'FRESH_RESTORE_ALL_PHASES': happy + function_body(fixture, 'Restore'),
            'IDENTITY_GENERATORS_REVISIONS_PENDING_TIME': happy,
            'RUNTIMEBRIDGE_REBUILD_STALE_OBSERVATION': happy,
            'CAUSAL_CONTINUATION_EXACTLY_ONCE': happy,
        }
        for obligation, tokens in review['obligations'].items():
            for token in tokens:
                require(scoped_token(token, scopes.get(obligation, '')), 'whole-engine scoped assertion missing: ' + obligation + ': ' + token)
        for token in ('RegisterEngineBase(', 'RegisterDefaultEngineRuntime(', 'live_.swap(prepared_)',
                      'DecodeRuntimeTimeCheckpointPackage(', 'DecodeRuntimePersistencePackage(',
                      'std::make_unique<rb::EngineRuntimeBridgeBackend>', 'ig::FileSaveGenerationStore store;',
                      'ig::SaveGenerationCoordinator saver;', 'ig::WholeEngineRestoreCoordinator restorer;',
                      'profiles.size() == 2', 'action_commits == 1', 'MaterializePersistentObjects();'):
            require(token in fixture, 'alternate/non-production whole composition: ' + token)
        for symbol in ('FreshBackendsRejectRetiredOpaqueCapabilities', 'FreshWholeEngineRestoresAllPlanesAndContinuesCausally',
                       'RetainedConditionJournalReconcilesWithoutChangingProgression',
                       'NonemptyCrossOwnerRestoreUsesSavedDependencies', 'DeathRewardRejectsUnknownDeliveryStateAtomically',
                       'SocialLifecycleCursorExcludesRebuiltJournalIdentity'):
            require(bool(function_body(test, symbol)) and registered_case(test, symbol), 'dead whole-engine proof: ' + symbol)
        reconciliation = function_body(test, 'RetainedConditionJournalReconcilesWithoutChangingProgression')
        for token in ('conditions.RestoreSnapshot(conditions.CaptureSnapshot())', 'adapter.RestoreCheckpoint(checkpoint)',
                      'adapter.ProcessChanges()', 'after.CanonicalBytes() == before.CanonicalBytes()',
                      'retry.CanonicalBytes() == before.CanonicalBytes()'):
            require(scoped_token(token, reconciliation), 'condition reconciliation regression missing: ' + token)
        cross_owner = function_body(test, 'NonemptyCrossOwnerRestoreUsesSavedDependencies')
        for token in ('engine.SeedCrossOwnerReward()', 'engine.framework.loot.GetDiagnostics().bundles == 0',
                      'engine.DurablePackage() == before', 'engine.SemanticPackage() == saved',
                      'engine.framework.death_reward.ProcessChanges()'):
            require(scoped_token(token, cross_owner), 'private dependency restore regression missing: ' + token)
        enum_case = function_body(test, 'DeathRewardRejectsUnknownDeliveryStateAtomically')
        require(scoped_token('!engine.framework.death_reward.RestoreCheckpoint(invalid)', enum_case) and
                scoped_token('actual.CanonicalBytes() == expected.CanonicalBytes()', enum_case), 'invalid delivery enum atomicity regression missing')
        cmake = (ROOT / 'Tests/Truth/CMakeLists.txt').read_text(encoding='utf-8')
        require('epidemic_add_truth_test(' + review['target'] in cmake and 'truth;persistence-whole-engine;goal5-regression' in cmake, 'whole-engine target/labels missing')
        utility = (ROOT / 'EngineFramework/DevelopmentInfrastructure/Tests/goal5_framework_composition.h').read_text(encoding='utf-8')
        require('orchestrator.RegisterComposition(participants, {}, expected)' in utility, 'whole-engine bypasses production participant registry')
        bridge = (ROOT / 'EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp').read_text(encoding='utf-8')
        require('AllocateRuntimeCapability' in bridge and 'AllocateBindingGeneration' in bridge, 'opaque identity reuse regression unfixed')
    return errors

def validate_failure_source(review, source_checks=True):
    errors = []
    def require(condition, message):
        if not condition: errors.append(message)
    require(review.get('stage') == '5.4.C' and review.get('schema_version') == 1 and review.get('reviewed') is True, 'missing finite whole failure review')
    require(review.get('execution_receipt_required') is True, 'failure source review cannot claim execution')
    require(review.get('precommit_policy') == 'LIVE_PLANES_UNCHANGED_PRIVATE_STAGES_RELEASED', 'precommit rollback/atomicity policy invalid')
    require(review.get('postcommit_policy') == 'SEMANTIC_RETAINED_RESUME_BLOCKED_RETRY_IDEMPOTENT', 'postcommit recovery policy invalid')
    required = {'LOAD', 'VALIDATE', 'STAGE', 'COMMIT_PREPARATION', 'RECONCILE_EXTERNAL', 'MATERIALIZE_RUNTIME', 'RESUME_GUARD', 'FRESH_PROCESS', 'CAUSAL_CONTINUATION'}
    require(set(review.get('obligations', {})) == required, 'whole failure obligations incomplete')
    for tokens in review.get('obligations', {}).values():
        require(isinstance(tokens, list) and len(tokens) >= 2 and len(tokens) == len(set(tokens)), 'generic or duplicate failure evidence')
    require(set(review.get('defects', {})) == {'G5-RUNTIME-PUBLISH-001'}, 'Runtime publication defect review missing')
    for defect in review.get('defects', {}).values():
        require(all(len(defect.get(key, '')) > 30 for key in ('root_cause', 'fix', 'regression')), 'publication defect lacks cause/fix/regression')
    if source_checks:
        test = (ROOT / review['source']).read_text(encoding='utf-8')
        atomic = function_body(test, 'WholeLoadAndPrecommitFailuresAreAtomic')
        recovery = function_body(test, 'WholePostcommitRecoveryRetainsSemanticsAndNoDuplicateRuntimeObject')
        scopes = {key: atomic for key in ('LOAD', 'VALIDATE', 'STAGE', 'COMMIT_PREPARATION')}
        scopes.update({key: recovery for key in ('RECONCILE_EXTERNAL', 'MATERIALIZE_RUNTIME', 'CAUSAL_CONTINUATION')})
        scopes.update(RESUME_GUARD=atomic + recovery, FRESH_PROCESS=function_body(test, 'main'))
        for obligation, tokens in review['obligations'].items():
            for token in tokens: require(scoped_token(token, scopes.get(obligation, '')), 'whole failure scoped assertion missing: ' + token)
        for symbol in ('WholeLoadAndPrecommitFailuresAreAtomic', 'WholePostcommitRecoveryRetainsSemanticsAndNoDuplicateRuntimeObject'):
            require(bool(function_body(test, symbol)) and registered_case(test, symbol), 'dead failure test: ' + symbol)
        cmake = (ROOT / 'Tests/Truth/CMakeLists.txt').read_text(encoding='utf-8')
        require('add_test(NAME ' + review['process_target'] in cmake, 'whole process target missing')
        process = (ROOT / 'tools/ci/test_whole_restore_process.py').read_text(encoding='utf-8')
        require("('--write', '--read')" in process and 'fingerprints[0] != fingerprints[1]' in process, 'process smoke does not compare fresh executable continuation')
        bridge = (ROOT / 'EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp').read_text(encoding='utf-8')
        for token in ('staged_forward.extract', 'staged_reverse.extract', 'staged_paths.extract', 'staged_binding.extract', 'remaining.reserve(queue_.size())', 'try { result = ProcessOne'):
            require(token in bridge, 'accepted work publication/retry preparation missing: ' + token)
    return errors

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    persistence, participants = read(PERSISTENCE), read(PARTICIPANTS)
    expected = generate(read(REVIEW), persistence, participants)
    errors = validate(expected, persistence, participants)
    whole = read(WHOLE_REVIEW)
    errors += validate_whole_source(whole)
    failure = read(FAILURE_REVIEW)
    errors += validate_failure_source(failure)
    if args.self_test:
        scoped_fixture = 'void Atomic() { TRUTH_REQUIRE(actual); /* TRUTH_REQUIRE(comment); */ } void Other() { TRUTH_REQUIRE(wrong); } int main() { return Run({{"atomic", Atomic}}); }'
        assert scoped_token('TRUTH_REQUIRE(actual)', function_body(scoped_fixture, 'Atomic'))
        assert not scoped_token('TRUTH_REQUIRE(comment)', function_body(scoped_fixture, 'Atomic'))
        assert not scoped_token('TRUTH_REQUIRE(wrong)', function_body(scoped_fixture, 'Atomic'))
        assert not scoped_token('TRUTH_REQUIRE(fake)', 'void Atomic() { auto note = "TRUTH_REQUIRE(fake)"; }')
        assert registered_case(scoped_fixture, 'Atomic')
        assert not registered_case(scoped_fixture, 'Other')
        assert not registered_case('int main() { /* {"atomic", Atomic} */ }', 'Atomic')
        assert executable_source('auto path = "https://example"; // note') == 'auto path = "https://example"; '
        print('Checked scoped-source proof: wrong function, comments, quoted assertions and dead registrations rejected.')
        cases = []
        def malformed(name, change):
            data = copy.deepcopy(expected); change(data); cases.append((name, data))
        malformed('phase missing', lambda d: d['phase_mapping'].pop('STAGE'))
        malformed('participant missing', lambda d: d['nodes'].pop(0))
        malformed('duplicate node', lambda d: d['nodes'].append(copy.deepcopy(d['nodes'][0])))
        malformed('unknown dependency', lambda d: d['nodes'][0]['dependencies'].append('missing'))
        malformed('cycle', lambda d: d['nodes'][0]['dependencies'].append(d['nodes'][0]['id']))
        malformed('durable coverage missing', lambda d: d['nodes'][0]['candidate_ids'].clear())
        malformed('unknown candidate', lambda d: d['nodes'][0]['candidate_ids'].append('missing'))
        malformed('Runtime Time missing', lambda d: d['nodes'].__setitem__(slice(None), [n for n in d['nodes'] if n['id'] != 'runtime.time']))
        malformed('resume dependency absent', lambda d: next(n for n in d['nodes'] if n['id'] == 'restore.resume')['dependencies'].clear())
        malformed('fallible boundary absent', lambda d: d['precommit'].pop('fallible_preparation'))
        malformed('retry rollback', lambda d: d['postcommit'].update(failure_policy='ROLLBACK'))
        malformed('early resume', lambda d: d['postcommit'].update(resume_policy='AFTER_COMMIT'))
        malformed('stale handle acceptance', lambda d: d['postcommit'].pop('runtime_bridge_policy'))
        malformed('false whole-engine claim', lambda d: d.update(qualification='PASS'))
        for name, data in cases:
            if not validate(data, persistence, participants, False): errors.append('negative fixture accepted: ' + name)
        print(f'Checked {len(cases)} restore-contract negative fixtures.')
        whole_cases = []
        for name, change in [
            ('missing identity proof', lambda d: d['obligations'].pop('RUNTIMEBRIDGE_REBUILD_STALE_OBSERVATION')),
            ('fake execution', lambda d: d.update(execution_receipt_required=False)),
            ('duplicate evidence', lambda d: d['obligations']['PRODUCTION_SAVE_STORAGE'].append(d['obligations']['PRODUCTION_SAVE_STORAGE'][0])),
            ('missing root cause', lambda d: d['defects']['G5-RUNTIME-IDENTITY-001'].pop('root_cause')),
        ]:
            data = copy.deepcopy(whole); change(data); whole_cases.append((name, data))
        for name, data in whole_cases:
            if not validate_whole_source(data, False): errors.append('whole-engine negative fixture accepted: ' + name)
        print(f'Checked {len(whole_cases)} whole-engine source-evidence negative fixtures; execution receipt remains separately required.')
        for name, change in [
            ('missing LOAD', lambda d: d['obligations'].pop('LOAD')),
            ('early resume', lambda d: d.update(postcommit_policy='RESUME_AFTER_COMMIT')),
            ('rollback published semantics', lambda d: d.update(precommit_policy='IGNORE_FAILURE')),
            ('fake execution', lambda d: d.update(execution_receipt_required=False)),
            ('missing defect', lambda d: d['defects'].clear()),
            ('duplicate materialization proof', lambda d: d['obligations']['MATERIALIZE_RUNTIME'].append(d['obligations']['MATERIALIZE_RUNTIME'][0])),
        ]:
            data = copy.deepcopy(failure); change(data)
            if not validate_failure_source(data, False): errors.append('whole failure negative fixture accepted: ' + name)
        print('Checked 6 whole-engine failure evidence negative fixtures.')
    if errors: raise ValueError('\n'.join(errors))
    if args.record: OUTPUT.write_text(encoded(expected), encoding='utf-8', newline='\n')
    elif args.check and (not OUTPUT.is_file() or OUTPUT.read_text(encoding='utf-8') != encoded(expected)):
        raise ValueError('restore contract stale: run owning --record generator')
    print(f'PASS: production-derived restore graph has {len(expected["nodes"])} nodes; coordination-core policy only, whole-engine qualification remains explicit.')

if __name__ == '__main__': main()
