"""Exact-current execution acceptance; source-ledger PASS alone cannot close Goal5."""
import argparse
import copy
import hashlib
import json
import subprocess
from pathlib import Path
import xml.etree.ElementTree as ET
from goal5_participant_contract import source_fingerprints, graph_fingerprint, validate_qualification_receipt
from goal5_required_scenarios_contract import qualify

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / '_goal5_handoff/final_qualification.json'
PROFILES = ('base-debug', 'base-release', 'runtime-debug', 'runtime-release', 'full-debug', 'full-release')
EVIDENCE = ('_goal5_handoff/final_matrix_report.json', '_goal5_handoff/final_validators_report.json',
    '_goal5_handoff/current_stage56_required_process.json', '_goal5_handoff/current_stage56_clangcl_process.json',
    '_goal5_handoff/final_generators.json', '_goal5_handoff/final_defects.json',
    '_goal5_handoff/final_debug_junit.xml', '_goal5_handoff/final_release_junit.xml',
    '_goal5_handoff/final_whole_repeats_junit.xml', '_goal5_handoff/final_storage_repeats_junit.xml',
    '_goal5_handoff/final_clangcl_headers_wrapper.json', '_goal5_handoff/participant_qualification.json',
    '_goal5_handoff/current_stage56_static_classifications.json', '_goal5_handoff/final_static_wrapper.json',
    '_goal5_handoff/final_diff_audit.json', '_goal5_handoff/current_stage56_generator_idempotence.json')

def load(name):
    return json.loads((ROOT / name).read_text(encoding='utf-8-sig'))

def digest(name):
    return hashlib.sha256((ROOT / name).read_bytes()).hexdigest()

def check_execution():
    current = source_fingerprints()
    matrix = load(EVIDENCE[0]); validators = load(EVIDENCE[1])
    for report, mode in ((matrix, 'Matrix'), (validators, 'Validators')):
        assert report['status'] == 'ok' and report['mode'] == mode, mode + ' not PASS'
        assert report['goal5_source_sha256'] == current, mode + ' tree is stale'
        stages = {stage['name']: stage for stage in report['stages']}
        assert stages['lint:freeze-validators']['status'] == 'ok'
    stages = {stage['name']: stage for stage in matrix['stages']}
    assert stages['self-check']['status'] == 'ok'
    for profile in PROFILES:
        for phase in ('build', 'test'):
            assert stages[phase + ':' + profile]['status'] == 'ok'
        assert 'self-containment passed' in stages['build:' + profile]['note'], 'header gate missing'
    counts = matrix['metrics']['test_counts']
    manifest = load('docs/freeze/ctest_manifest.json')['profiles']
    assert counts['passed'] == counts['total'] == sum(len(manifest[p]['tests']) for p in PROFILES)
    assert counts['failed'] == counts['skipped'] == 0
    for name in EVIDENCE[2:4]: qualify(ROOT / name)
    generated = load(EVIDENCE[4])
    assert generated['status'] == 'PASS' and generated['passes'] == 2
    assert generated['artifacts'] and all(digest(name) == value for name, value in generated['artifacts'].items())
    cpp = load(EVIDENCE[15])
    assert cpp['status'] == 'PASS' and cpp['strategy'] == 'TWO_INDEPENDENT_RENDERS_EXACT_CURRENT_BYTES_NO_SOURCE_MUTATION'
    assert cpp['artifacts'] and all(digest(name) == value for name, value in cpp['artifacts'].items())
    defects = load(EVIDENCE[5])
    assert defects['status'] == 'REVIEWED' and defects['defects']
    names = {test['name'] for test in manifest['full-debug']['tests']}
    for identity, defect in defects['defects'].items():
        assert identity.startswith('G5-') and defect['status'] == 'FIXED' and defect['review_status'] == 'REVIEWED'
        assert defect['test_target'] in names and defect['regression_symbol']
        assert defect['source_sha256'] == digest(defect['source'])
        assert defect['regression_symbol'] in (ROOT / defect['regression_source']).read_text(encoding='utf-8')
        assert defect['reproducer'] and all((ROOT / name).is_file() for name in defect['reproducer'])
    labels = {'persistence-wire', 'persistence-storage', 'persistence-whole-engine',
        'determinism', 'determinism-save-load', 'goal5-regression'}
    header = load(EVIDENCE[10])
    assert header['status'] == 'PASS' and header['exit_code'] == 0, 'ClangCL headers not PASS'
    assert header['source_sha256'] == current, 'ClangCL header source tree is stale'
    assert (ROOT / header['stdout_log']).is_file()
    graph = load('docs/freeze/goal5_participant_contract.json')
    assert not validate_qualification_receipt(load(EVIDENCE[11]), current, graph_fingerprint(graph), manifest)
    static = load(EVIDENCE[12]); raw = load(EVIDENCE[13])
    assert static['status'] == 'PASS_CLASSIFIED_CHECKER_LIMITATIONS'
    assert static['raw_wrapper_status'] == raw['status'] == 'FAIL'
    assert static['checks'] == raw['checks'] == 36 and static['successful_checks'] == 32
    assert len(raw['failures']) == len(static['classifications']) == 4
    assert all(digest(name) == value for name, value in static['inspected_source_sha256'].items())
    audit = load(EVIDENCE[14])
    diff = subprocess.run(['git', 'diff', '--binary'], cwd=ROOT, check=True, capture_output=True).stdout
    assert audit['status'] == 'PASS' and audit['complete_diff_sha256'] == hashlib.sha256(diff).hexdigest()
    assert all(digest(name) == value for name, value in audit['protected_source_sha256'].items())
    for index, name in enumerate(EVIDENCE[6:10]):
        cases = ET.parse(ROOT / name).findall('.//testcase')
        assert cases and all(case.find('failure') is None and case.find('skipped') is None for case in cases)
        actual = [case.attrib['name'] for case in cases]
        if index < 2:
            profile = 'full-debug' if index == 0 else 'full-release'
            expected = {test['name'] for test in manifest[profile]['tests'] if labels.intersection(test['labels'])}
            assert set(actual) == expected and len(actual) == len(expected), 'focused tests incomplete'
        else:
            expected = 'EpidemicWholeEngineRestoreTruthTests' if index == 2 else 'EpidemicPersistenceProcessTruthTests'
            assert actual == [expected] * 3, 'three independent repeat runs missing'
    assert load('docs/freeze/goal5_persistence_gaps.json')['gaps'] == []
    ledger = load('docs/freeze/goal5_ledger.json')
    assert ledger['summary'] == {'modules': 78, 'BLOCKED': 0, 'UNCLASSIFIED': 0}
    return current, counts

def generate():
    current, counts = check_execution()
    return {'schema_version': 1, 'status': 'MILESTONE_5_PASS',
        'completion_states': ['PERSISTENCE_QUALIFIED', 'DETERMINISM_QUALIFIED', 'GOAL5_COMPLETE'],
        'requirements': ['M5.1', 'M5.2', 'M5.3', 'M5.4', 'M5.5', 'M5.6', 'M5.7'],
        'source_sha256': current, 'evidence_sha256': {name: digest(name) for name in EVIDENCE},
        'matrix_tests': counts, 'unresolved_code_debt': 0, 'unresolved_evidence_debt': 0}

def validate(receipt):
    assert receipt == generate(), 'final qualification stale or fabricated'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ('record', 'check', 'self-test'): parser.add_argument('--' + flag, action='store_true')
    args = parser.parse_args()
    value = generate()
    if args.self_test:
        for mutate in (lambda x: x.update(unresolved_code_debt=1),
                       lambda x: x.update(source_sha256={}),
                       lambda x: x.update(evidence_sha256={}),
                       lambda x: x.update(status='PASS_NOTE_ONLY')):
            bad = copy.deepcopy(value); mutate(bad)
            try: validate(bad)
            except AssertionError: pass
            else: raise AssertionError('fake final qualification accepted')
    if args.record: OUTPUT.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    if args.check: validate(load('_goal5_handoff/final_qualification.json'))
    print('PASS: exact-tree execution, source-bound evidence, no open debt')

if __name__ == '__main__': main()
