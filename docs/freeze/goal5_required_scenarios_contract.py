"""Finite A-H source contract and separately checked real process qualification."""
import argparse
import copy
import hashlib
import json
import re
from pathlib import Path
from goal5_restore_contract import function_body, registered_case, scoped_token

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'docs/freeze/goal5_required_scenarios.json'
FILES = ['Tests/Truth/goal5_required_scenarios.h', 'Tests/Truth/goal5_required_scenario_runner.h',
    'Tests/Truth/determinism_save_load_tests.cpp', 'Tests/Truth/goal5_whole_composition.h',
    'Tests/Truth/goal5_replay.h', 'Tests/Truth/goal5_scenario.h', 'Tests/Truth/goal5_digest.h',
    'Tests/Truth/goal5_digest_value_codecs.h',
    'EngineFramework/DevelopmentInfrastructure/Tests/goal5_framework_composition.h',
    'tools/ci/test_required_determinism_process.py', 'Tests/Truth/CMakeLists.txt']
OBLIGATIONS = {
    'A': ['CONTROL_CONTINUATION', 'TEN_SAME_EXECUTABLE_RUNS'],
    'B': ['SAVE_DESTROY_FRESH_RESTORE_RECONCILE', 'DIGEST_TRACE_EQUIVALENT'],
    'C': ['MATERIALIZE_STREAM_OUT_ADVANCE_STREAM_IN', 'TRANSIENT_REBUILD'],
    'D': ['THREE_PENDING_DELIVERIES', 'ONE_ATTEMPT_BUDGET', 'COMMON_COMPLETION_EQUIVALENCE'],
    'E': ['ABILITY_RESERVATION', 'COMBAT_EFFECT_COMMIT', 'PERSISTENT_CONDITION_PROGRESSION', 'TERMINAL_LOOT_REWARD', 'DUPLICATE_NO_SEMANTIC_CHANGE'],
    'F': ['OBSERVATION_KNOWLEDGE', 'TARGET_DISAPPEARS', 'AI_REPLAN_INTENT_COMMIT'],
    'G': ['RESOURCE_RESERVATION_PROCESS', 'RESOURCE_PRODUCTION', 'RETRY_NO_DUPLICATE'],
    'H': ['FRESH_RUNTIME_HANDLE', 'RETIRED_HANDLE_CURSOR_OBSERVATION_REJECTED', 'MATERIALIZATION_FAILURE_RESUME_GUARD_RETRY', 'IDENTITY_EXCLUDED_FROM_DIGEST']}
ASSERTIONS = {
    'CombatBegin': ['current_micro == 40\'000\'000', 'resource_reservations.size() == 1'],
    'CombatFinish': ['CombatLifeState::Dead', 'attack.Value() == 7\'000\'000', 'rewards.Value().size() == 1', 'duplicate_rewards.Value().empty()', 'Digest() == settled'],
    'ObservationBegin': ['observed.Value().size() == 1', 'learned.Value().knowledge.IsValid()', 'CommitIntent'],
    'ObservationFinish': ['knowledge.front().id', 'AdvanceTime', 'FindPerceiver', 'RequestReplan', 'context.targets.empty()', 'CommitIntent'],
    'ProductionFinish': ['ms5.iron', 'ms5.product', '== 8', '== 1', 'Digest() == settled'],
    'BudgetBegin': ['pending.size() == 3']}

def scoped(token, body):
    # C++ digit separators are not character literals.
    normalize = lambda value: re.sub(r"(?<=\d)'(?=\d)", '', value)
    return scoped_token(normalize(token), normalize(body))


def expected():
    names = set(FILES)
    for folder in ('EngineBase', 'EngineRuntime', 'EngineFramework'):
        for path in (ROOT / folder).rglob('*'):
            if path.suffix in {'.h', '.hpp', '.cpp', '.cc', '.cxx', '.inl'} and not any(
                    part in {'build', '.ci_cache', 'Deltas'} for part in path.relative_to(ROOT).parts):
                names.add(path.relative_to(ROOT).as_posix())
    return {'schema_version': 1, 'stage': '5.6.C', 'qualification': 'SOURCE_REVIEWED_EXECUTION_REQUIRED',
        'obligations': OBLIGATIONS, 'budget_partition_policy': 'EVENTUAL_SEMANTIC_EQUIVALENCE_AT_COMMON_COMPLETION',
        'required_targets': ['EpidemicDeterminismSaveLoadTruthTests', 'EpidemicDeterminismRequiredProcessTruthTests'],
        'source_sha256': {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in sorted(names)}}

def source_stamp():
    hashes = expected()['source_sha256']
    return hashlib.sha256(''.join(name + ':' + digest + '\n' for name, digest in sorted(hashes.items())).encode()).hexdigest()


def validate(value):
    assert value == expected(), 'required scenario contract missing, fabricated, or source-stale'
    runner = (ROOT / FILES[1]).read_text(encoding='utf-8')
    tests = (ROOT / FILES[2]).read_text(encoding='utf-8')
    assert registered_case(tests, 'RequiredScenarioMatrixHasEquivalentContinuation'), 'A-H regression is dead'
    assert registered_case(tests, 'DeterministicBudgetPartitionsReachCommonCompletion'), 'budget regression is dead'
    for symbol, tokens in ASSERTIONS.items():
        body = function_body(runner, symbol)
        assert body, symbol + ' body missing'
        for token in tokens:
            # Fixed canonical scenario identifiers are data inputs, not quoted assertions.
            assert (token in body if token.startswith('ms5.') else scoped(token, body)), symbol + ': missing scoped evidence ' + token
    matrix = function_body(tests, 'RequiredScenarioMatrixHasEquivalentContinuation')
    for token in ('repetition != 10', 'control.semantic_digest == restored.semantic_digest',
                  'control.normalized_trace == restored.normalized_trace', 'CompareScenario'):
        assert scoped(token, matrix), 'D1/B assertion absent: ' + token
    replay = function_body(runner, 'ReplayRequiredScenario')
    for token in ('!engine.Restore()', '!engine.restorer.CanResume()', 'RetryReconciliation()',
                  '!engine.framework.entities.Resolve(old_handle)', '!engine.backend->ObserveTransform(old_binding.runtime_object)',
                  'binding->runtime_object != old_binding.runtime_object', 'engine.SemanticState().Digest() == result.semantic_digest'):
        assert scoped(token, replay), 'H/B evidence absent: ' + token


def check_compiled_stamp(scenario, current):
    assert scenario.get('build_source_sha256') == current, 'receipt compiled against different source tree'

def qualify(path):
    receipt = json.loads(path.read_text(encoding='utf-8'))
    assert receipt['status'] == 'PASS' and set(receipt) == {'status', 'D2', 'D3'}
    assert receipt['D2']['scenarios'] == receipt['D3']['scenarios'], 'unexplained D3 mismatch'
    for name in ('D2', 'D3'):
        item = receipt[name]
        assert item['fresh_processes'] >= 48 and item['repetitions_per_mode'] >= 3
        assert set(item['scenarios']) == set(OBLIGATIONS)
        executable = Path(item['executable'])
        assert hashlib.sha256(executable.read_bytes()).hexdigest() == item['executable_sha256'], 'stale executable receipt'
        for scenario in item['scenarios'].values():
            check_compiled_stamp(scenario, source_stamp())
            assert scenario['semantic_digest'] and scenario['event_trace_digest'] and scenario['descriptor_hex']
            assert scenario['normalized_trace'] and scenario['step_count'] >= 7
            orders = [event['logical_order'] for event in scenario['normalized_trace']]
            assert orders == sorted(set(orders)), 'trace tie or unordered trace'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--qualification', type=Path)
    args = parser.parse_args()
    value = expected()
    validate(value)
    if args.self_test:
        current = source_stamp()
        check_compiled_stamp({'build_source_sha256': current}, current)
        for stale in ({}, {'build_source_sha256': '0' * 64}, {'build_source_sha256': 'PASS'}):
            try: check_compiled_stamp(stale, current)
            except AssertionError: pass
            else: raise AssertionError('stale or fabricated compiled source stamp accepted')
        for mutation in (lambda d: d['obligations'].pop('H'), lambda d: d.update(qualification='PASS'),
                         lambda d: d['source_sha256'].update({FILES[1]: '0' * 64})):
            invalid = copy.deepcopy(value); mutation(invalid)
            try: validate(invalid)
            except AssertionError: pass
            else: raise AssertionError('invalid source admission accepted')
    encoded = json.dumps(value, indent=2, sort_keys=True) + '\n'
    if args.record: OUTPUT.write_text(encoded, encoding='utf-8', newline='\n')
    if args.check: assert OUTPUT.read_text(encoding='utf-8') == encoded, 'required scenario registry stale'
    if args.qualification: qualify(args.qualification)
    print('PASS: finite A-H obligations, scoped registered regressions, exact source hashes' +
          ('; real D2/D3 receipts qualified' if args.qualification else '; execution separate'))


if __name__ == '__main__': main()
