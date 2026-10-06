"""Owning deterministic-domain registry; reviewed policy, not execution qualification."""
from __future__ import annotations
import argparse
import copy
from pathlib import Path
from goal5_persistence_contract import load_json, encoded

ROOT = Path(__file__).resolve().parents[2]
REVIEW = ROOT / '_goal5_handoff/digest_domain_review.json'
OUTPUT = ROOT / 'docs/freeze/goal5_determinism_contract.json'
FLOAT_POLICY = 'FINITE_QUANTIZED_1E-6_INT64_NEGATIVE_ZERO_NORMALIZED_NONFINITE_REJECTED'
EXCLUDED = {'pointers', 'allocator_addresses', 'runtime_transient_handles', 'binding_capabilities', 'thread_ids',
            'diagnostics', 'log_timestamps', 'wall_clock_timings', 'renderer_backend_identities', 'audio_backend_identities', 'os_handles'}


def generate(review, persistence, participants):
    if review.get('schema_version') != 1 or review.get('stage') != '5.5.A' or review.get('reviewed') is not True:
        raise ValueError('finite reviewed digest-domain policy required')
    if review.get('floating_policy') != FLOAT_POLICY or set(review.get('excluded_inputs', [])) != EXCLUDED:
        raise ValueError('noncanonical float or process-local semantic identity policy')
    if review.get('execution_receipt_required') is not True:
        raise ValueError('source registry cannot manufacture execution PASS')
    records = []
    for identity, candidate in sorted(persistence['candidates'].items()):
        if candidate['review_status'] != 'REVIEWED': raise ValueError('unreviewed persistence input')
        role = candidate['persistence_role']
        if role in {'PERSISTENT_OWNER', 'PERSISTENT_CHECKPOINT'}:
            decision = 'INCLUDED'
            adapter = candidate['participant_id'] if candidate['save_plane'] == 'FRAMEWORK_SAVEGAME' else (
                'runtime.time' if candidate['module'] == 'EngineRuntime/Time' else 'runtime.persistence')
            if adapter.startswith('GAP:') or adapter == 'N/A': raise ValueError('unmapped semantic owner')
            reason = 'Authoritative value state at the reviewed durable boundary: ' + candidate['authoritative_owner']
        elif candidate['digest_policy'] == 'DERIVED_AND_VERIFIED':
            decision = 'DERIVED_AND_VERIFIED'
            adapter = {'G5P-55e59901da279541': 'framework.society', 'G5P-68ee9783f67a3a55': 'framework.entities'}.get(identity, 'runtime.rebuild')
            reason = review['derived_policy'] + ' Existing reviewed role: ' + candidate['qualified_symbol']
        elif candidate['deterministic_relevance'] == 'NO' and candidate['digest_policy'] == 'EXCLUDE':
            decision, adapter = 'EXCLUDED_NON_SEMANTIC', 'N/A'
            reason = candidate['rationale'] + ' Existing reviewed nonsemantic value; excluded inputs remain outside the domain.'
        else: raise ValueError('unknown deterministic domain combination: ' + identity)
        records.append({'state_id': identity, 'module': candidate['module'], 'symbol': candidate['qualified_symbol'],
                        'decision': decision, 'digest_adapter': adapter, 'rationale': reason,
                        'ordering_policy': review['ordering_policy'],
                        'floating_policy': FLOAT_POLICY if adapter.startswith('framework.') or adapter.startswith('integration.') else 'NO_FLOATING_VALUE_PACKAGE_OR_EXCLUDED',
                        'evidence': {'source_path': 'Tests/Truth/goal5_whole_composition.h',
                                     'test_symbol': 'SemanticState' if decision == 'INCLUDED' else 'RuntimeHost' if adapter == 'runtime.rebuild' else 'SemanticState',
                                     'contract_assertion': 'all reviewed owner snapshots contribute through generated typed adapters; transient projection identities are excluded',
                                     'execution_receipt_required': True}})
    owners = sorted([p['id'] for p in participants['participants']] + ['runtime.time', 'runtime.persistence'])
    actual = {r['digest_adapter'] for r in records if r['decision'] == 'INCLUDED'}
    if actual != set(owners): raise ValueError('semantic owner registry not exact')
    return {'schema_version': 1, 'stage': '5.5.A', 'qualification': 'SOURCE_REVIEWED_EXECUTION_REQUIRED',
            'generated_from': '5.1 persistence contract + digest_domain_review.json + participant contract',
            'modules': sorted(persistence['modules']), 'owners': owners, 'states': records,
            'excluded_inputs': sorted(EXCLUDED), 'floating_policy': FLOAT_POLICY,
            'runtime_payload_policy': review['runtime_payload_policy'], 'execution_receipt_required': True}


def validate(data, expected, source_checks=True):
    errors = []
    if data != expected: errors.append('digest domain differs from exact reviewed ownership/relevance projection')
    if source_checks:
        source = (ROOT / 'EngineFramework/DevelopmentInfrastructure/Tests/goal5_framework_composition.h').read_text(encoding='utf-8')
        for owner in expected['owners']:
            if owner.startswith('runtime.'): continue
            if f'digest.AddSnapshot("{owner}"' not in source: errors.append('missing typed owner digest adapter: ' + owner)
        writer = (ROOT / 'Tests/Truth/goal5_digest.h').read_text(encoding='utf-8')
        for token in ('SemanticFloat', 'std::isfinite', '1\'000\'000.0L', 'std::map<std::string', 'TransferSnapshot', 'duplicate semantic owner'):
            if token not in writer: errors.append('canonical qualification writer missing: ' + token)
        if 'reinterpret_cast' in writer or 'bit_cast' in writer: errors.append('native/raw floating digest bytes forbidden')
        fixture = (ROOT / 'Tests/Truth/goal5_whole_composition.h').read_text(encoding='utf-8')
        for token in ('AddFrameworkSemanticContributions', 'digest.AddBytes("runtime.time"', 'digest.AddBytes("runtime.persistence"'):
            if token not in fixture: errors.append('whole semantic digest source missing: ' + token)
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    persistence = load_json(ROOT / 'docs/freeze/goal5_persistence_contract.json')
    participants = load_json(ROOT / 'docs/freeze/goal5_participant_contract.json')
    review = load_json(REVIEW)
    expected = generate(review, persistence, participants)
    errors = validate(expected, expected)
    if args.self_test:
        cases = [lambda d: d['states'].pop(), lambda d: d['states'].append(copy.deepcopy(d['states'][0])),
                 lambda d: d['owners'].pop(), lambda d: d['modules'].pop(),
                 lambda d: d['states'][0].update(decision='UNKNOWN'),
                 lambda d: d['states'][0].update(digest_adapter='none'),
                 lambda d: d.update(floating_policy='RAW_NATIVE_FLOAT'),
                 lambda d: d['excluded_inputs'].remove('pointers'),
                 lambda d: d.update(qualification='PASS'), lambda d: d.update(execution_receipt_required=False)]
        for change in cases:
            data = copy.deepcopy(expected); change(data)
            if not validate(data, expected, False): errors.append('accepted malformed digest domain')
        print('PASS: ten missing-owner/duplicate/identity/float/fake-execution digest negatives rejected.')
    if errors: raise ValueError('\n'.join(errors))
    if args.record: OUTPUT.write_text(encoded(expected), encoding='utf-8', newline='\n')
    elif args.check and (not OUTPUT.is_file() or OUTPUT.read_text(encoding='utf-8') != encoded(expected)):
        raise ValueError('digest domain registry stale; run owning --record')
    print(f'PASS: {len(expected["modules"])} modules, {len(expected["states"])} explicit state decisions, {len(expected["owners"])} semantic owner contributions; execution qualification separate.')
    from goal5_determinism_review import check as check_candidates
    check_candidates(args.self_test, args.record)
    from goal5_required_scenarios_contract import OUTPUT as scenario_output, validate as validate_scenarios
    validate_scenarios(load_json(scenario_output))


if __name__ == '__main__': main()
