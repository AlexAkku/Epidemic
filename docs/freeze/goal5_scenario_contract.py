"""Source/model admission for the canonical value-only runner; execution is separate."""
import argparse
import copy
from pathlib import Path
from goal5_persistence_contract import load_json

ROOT = Path(__file__).resolve().parents[2]
REVIEW = ROOT / '_goal5_handoff/scenario_runner_review.json'
KINDS = {'SubmitCommand', 'AdvanceGameTime', 'Tick', 'CompleteExternalOperation', 'ApplyBudget', 'StreamOut', 'StreamIn', 'SaveReload', 'Reconcile', 'Materialize'}
POLICIES = {'clock': 'LOGICAL_EXPLICIT', 'seed': 'EXPLICIT_OWNED_STREAMS', 'completion': 'EXPLICIT_ORDERED_INPUT',
            'floating': 'CANONICAL_QUANTIZED', 'trace': 'SEMANTIC_COMMITS_ONLY_WITH_UNIQUE_LOGICAL_ORDER'}
ADVERSARIAL = {'pointer_address', 'unordered_iteration', 'ambient_wall_clock', 'random_device_seed', 'missing_owner', 'missing_tie_breaker', 'duplicate_event_order', 'implicit_async_order', 'raw_float', 'hidden_mutable_state'}


def validate(review, source_checks=True):
    errors = []
    def require(condition, message):
        if not condition: errors.append(message)
    require(review.get('schema_version') == 1 and review.get('stage') == '5.5.B' and review.get('reviewed') is True, 'finite scenario review required')
    require(set(review.get('step_kinds', [])) == KINDS and len(review.get('step_kinds', [])) == len(KINDS), 'missing/duplicate logical step')
    require(review.get('policies') == POLICIES, 'ambient/implicit/noncanonical scenario policy')
    require(set(review.get('descriptor_fields', [])) == {'scenario_id', 'config_id', 'seed_set', 'initial_state_id', 'steps', 'expected_contract'}, 'incomplete reproduction descriptor')
    require(set(review.get('failure_fields', [])) == {'scenario_id', 'seed_set', 'step_index', 'expected_digest', 'actual_digest', 'first_differing_trace_event'}, 'incomplete failure reproduction')
    require(set(review.get('adversarial_classes', [])) == ADVERSARIAL and len(review.get('adversarial_classes', [])) == 10, 'runner adversarial set incomplete')
    require(review.get('execution_receipt_required') is True, 'source admission must not fabricate executed self-tests')
    if source_checks:
        source = (ROOT / review['source']).read_text(encoding='utf-8')
        test = (ROOT / review['self_test_source']).read_text(encoding='utf-8')
        for token in KINDS | set(review['descriptor_fields']) | set(review['failure_fields']):
            require(token in source, 'scenario field/step missing: ' + token)
        for token in ('ValidateScenario', 'EncodeScenario', 'DecodeScenario', 'SemanticTrace', 'RunScenario', 'CompareScenario', 'FailureText', 'logical_order <= previous', 'operation_id.empty()', 'event.logical_order <= events_.back().logical_order'):
            require(token in source, 'missing checked runner contract: ' + token)
        descriptor = source[source.index('struct ScenarioDescriptor'):source.index('inline void ValidateScenario')]
        require(not any(token in descriptor for token in ('std::function', 'void*', 'thread::', 'time_point')), 'descriptor contains process-local callback/pointer identity')
        for token in ('clock.clock_policy', 'random.seed_policy', 'floating.floating_policy', 'tie.steps', 'async.steps', 'trace.Commit', 'ordered.Digest() == reversed.Digest()', 'missing.Digest() != ordered.Digest()', 'pointer_polluted.Digest() != ordered.Digest()', 'CompareScenario(descriptor, first, second)', 'quiet_NaN'):
            require(token in test, 'missing sensitive runner self-test: ' + token)
        whole = (ROOT / 'Tests/Truth/whole_engine_restore_tests.cpp').read_text(encoding='utf-8')
        require(review['test_symbol'] in whole[whole.index('int main('):], 'runner self-test is dead')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    review = load_json(REVIEW)
    errors = validate(review)
    if args.self_test:
        cases = [lambda d: d['step_kinds'].pop(), lambda d: d['step_kinds'].append(d['step_kinds'][0]),
                 lambda d: d['policies'].update(clock='AMBIENT'), lambda d: d['policies'].update(seed='RANDOM_DEVICE'),
                 lambda d: d['policies'].update(completion='IMPLICIT'), lambda d: d['policies'].update(floating='RAW'),
                 lambda d: d['descriptor_fields'].pop(), lambda d: d['failure_fields'].pop(),
                 lambda d: d['adversarial_classes'].pop(), lambda d: d.update(execution_receipt_required=False)]
        for change in cases:
            data = copy.deepcopy(review); change(data)
            if not validate(data, False): errors.append('malformed scenario review accepted')
        print('PASS: ten scenario-policy/reproduction/fake-execution negative fixtures rejected.')
    if errors: raise ValueError('\n'.join(errors))
    print('PASS: value-only canonical scenario/trace/runner source model; executable self-tests remain separately qualified.')


if __name__ == '__main__': main()
