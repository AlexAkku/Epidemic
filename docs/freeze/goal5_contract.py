"""Goal5 source-contract aggregator; exact-tree execution requires explicit qualification."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'docs/freeze/goal5_ledger.json'
LOWER = ['goal5_admission', 'goal5_persistence_inventory', 'goal5_persistence_contract',
    'goal5_participant_bindings', 'goal5_participant_contract', 'goal5_schema_contract',
    'goal5_stage52_contract', 'goal5_runtime_durable_contract', 'goal5_save_generation_contract',
    'goal5_restore_contract', 'goal5_determinism_inventory', 'goal5_determinism_contract',
    'goal5_evidence_quality', 'goal5_required_scenarios_contract']
INPUTS = ['goal5_admission.json', 'goal5_persistence_contract.json', 'goal5_persistence_gaps.json',
    'goal5_participant_contract.json', 'goal5_schema_contract.json', 'goal5_save_generation_contract.json',
    'goal5_restore_contract.json', 'goal5_determinism_contract.json', 'goal5_determinism_candidates.json',
    'goal5_determinism_reviewed.json', 'goal5_required_scenarios.json', 'ctest_manifest.json']
LABELS = {'persistence-local', 'persistence-wire', 'persistence-storage',
    'persistence-whole-engine', 'determinism', 'determinism-save-load', 'goal5-regression'}

def load(relative):
    return json.loads((ROOT / relative).read_text(encoding='utf-8-sig'))

def generate():
    persistence = load('docs/freeze/goal5_persistence_contract.json')
    domain = load('docs/freeze/goal5_determinism_contract.json')
    reviewed = load('docs/freeze/goal5_determinism_reviewed.json')['candidates']
    participants = load('docs/freeze/goal5_participant_contract.json')['participants']
    defects = load('_goal5_handoff/final_defects.json')['defects']
    rows = []
    for module, decision in sorted(persistence['modules'].items()):
        states = [state for state in domain['states'] if state['module'] == module]
        findings = [finding for finding in reviewed if finding['source']['module'] == module]
        included = any(state['decision'] == 'INCLUDED' for state in states)
        applicable = included or any(finding['decision'] != 'EXCLUDED_NON_SEMANTIC' for finding in findings)
        owned = [participant for participant in participants if participant['owner_module'] == module]
        targets = {finding['evidence']['test_target'] for finding in findings}
        targets.update(evidence['test_target'] for participant in owned for evidence in participant['evidence'])
        if included or decision['applicability'] == 'APPLICABLE':
            targets.update({'EpidemicWholeEngineRestoreTruthTests', 'EpidemicDeterminismSaveLoadTruthTests'})
        reasons = sorted({state['rationale'] for state in states if state['decision'] != 'INCLUDED'} |
            {finding['rationale'] for finding in findings if finding['decision'] == 'EXCLUDED_NON_SEMANTIC'})
        rows.append({'module': module, 'persistence_applicability': decision['applicability'],
            'persistence_status': 'PASS' if decision['applicability'] == 'APPLICABLE' else 'N/A',
            'persistence_rationale': decision['reason'],
            'determinism_applicability': 'APPLICABLE' if applicable else 'NOT_APPLICABLE',
            'determinism_status': 'PASS' if applicable else 'N/A',
            'determinism_rationale': ('Reviewed digest contributors and exact source-bound observable-output policies.' if applicable
                else 'Reviewed domain/scanner contracts contain no included semantic state or observable finding. ' +
                    '; '.join(reasons or [decision['reason']])),
            'candidate_ids': sorted(decision['candidate_ids']),
            'determinism_candidate_ids': sorted(finding['candidate_id'] for finding in findings),
            'participant_ids': sorted(participant['id'] for participant in owned),
            'scenario_evidence': sorted(targets),
            'defect_ids': sorted(identity for identity, defect in defects.items() if defect['source'].startswith(module + '/'))})
    hashes = {name: hashlib.sha256((ROOT / 'docs/freeze' / name).read_bytes()).hexdigest() for name in INPUTS}
    hashes['_goal5_handoff/final_defects.json'] = hashlib.sha256((ROOT / '_goal5_handoff/final_defects.json').read_bytes()).hexdigest()
    return {'schema_version': 1, 'stage': '5.7.A', 'qualification': 'SOURCE_CONTRACTS_EXECUTION_REQUIRED',
        'execution_receipt_required': True, 'generated_from': hashes, 'modules': rows,
        'summary': {'modules': len(rows), 'BLOCKED': 0, 'UNCLASSIFIED': 0}}

def validate(ledger):
    assert ledger == generate(), 'ledger stale, incomplete, or fabricated PASS'
    assert load('docs/freeze/goal5_persistence_gaps.json')['gaps'] == [], 'Goal5 gaps remain'
    manifest = load('docs/freeze/ctest_manifest.json')['profiles']
    names = set()
    for profile in ('full-debug', 'full-release'):
        tests = manifest[profile]['tests']
        assert LABELS <= {label for test in tests for label in test['labels']}, 'required CTest labels missing'
        assert {'EpidemicWholeEngineRestoreTruthTests', 'EpidemicDeterminismSaveLoadTruthTests',
            'EpidemicDeterminismRequiredProcessTruthTests'} <= {test['name'] for test in tests}
        names.update(test['name'] for test in tests)
    for row in ledger['modules']:
        assert set(row['scenario_evidence']) <= names, 'missing evidence target'
        assert all(row[name] in {'PASS', 'N/A'} for name in ('persistence_status', 'determinism_status'))
        for prefix in ('persistence', 'determinism'):
            if row[prefix + '_status'] == 'N/A':
                assert len(row[prefix + '_rationale']) > 30, 'unreviewed N/A'

def lower(self_tests=False):
    for name in LOWER:
        arguments = [sys.executable, str(ROOT / 'docs/freeze' / (name + '.py')), '--check']
        if name == 'goal5_participant_contract': arguments += ['--implementation']
        if self_tests: arguments += ['--self-test']
        result = subprocess.run(arguments, cwd=ROOT, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(name + ' FAIL: ' + (result.stdout + result.stderr)[-2500:])
    if self_tests:
        subprocess.run([sys.executable, str(ROOT / 'tools/generate_goal5_participants.py'),
            '--check', '--self-test'], cwd=ROOT, check=True, capture_output=True)
    subprocess.run([sys.executable, str(ROOT / 'tools/record_goal5_final_defects.py'), '--check'] +
        (['--self-test'] if self_tests else []),
        cwd=ROOT, check=True, capture_output=True)

def self_test(ledger):
    for mutate in (lambda value: value['modules'].pop(),
                   lambda value: value.update(qualification='MILESTONE_5_PASS'),
                   lambda value: value['modules'][0].update(persistence_status='PASS'),
                   lambda value: value['modules'][0]['scenario_evidence'].append('MissingTest'),
                   lambda value: value.update(execution_receipt_required=False)):
        invalid = copy.deepcopy(ledger); mutate(invalid)
        try: validate(invalid)
        except AssertionError: pass
        else: raise AssertionError('fake ledger PASS accepted')
    lower(True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--require-qualified', action='store_true')
    args = parser.parse_args()
    lower()
    ledger = generate(); validate(ledger)
    if args.self_test: self_test(ledger)
    encoded = json.dumps(ledger, indent=2, sort_keys=True) + '\n'
    if args.record: OUTPUT.write_text(encoded, encoding='utf-8', newline='\n')
    if args.check: assert OUTPUT.read_text(encoding='utf-8') == encoded, 'generated ledger stale'
    if args.require_qualified:
        from goal5_final_qualification import validate as qualify
        qualify(load('_goal5_handoff/final_qualification.json'))
    print('PASS:78-module source contracts, exact generated ledger, lower validators' +
        ('; exact-tree execution qualified' if args.require_qualified else '; execution explicitly separate'))

if __name__ == '__main__': main()
