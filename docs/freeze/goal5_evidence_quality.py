"""Fail-closed source admission for semantic Goal 5 evidence, not execution qualification."""
import argparse
import copy
import re
from pathlib import Path
from goal5_persistence_contract import load_json
from goal5_restore_contract import function_body, registered_case, scoped_token

ROOT = Path(__file__).resolve().parents[2]
REVIEW = ROOT / '_goal5_handoff/deterministic_scenarios_review.json'


def validate(review, domain, manifest, source_checks=True):
    errors = []
    def require(condition, message):
        if not condition: errors.append(message)
    require(review.get('schema_version') == 1 and review.get('reviewed') is True, 'review missing')
    require(review.get('execution_receipt_required') is True, 'source evidence cannot fabricate executed PASS')
    owners = domain.get('owners', [])
    require(len(owners) == 49 and len(set(owners)) == len(owners), 'semantic owner coverage incomplete/duplicated')
    included = [state for state in domain.get('states', []) if state.get('decision') == 'INCLUDED']
    require(len(included) == 152, 'required deterministic state coverage incomplete')
    require(all(state.get('digest_adapter') in owners for state in included), 'required state lacks owner adapter')
    scenarios = review.get('scenarios', [])
    ids = [scenario.get('id') for scenario in scenarios]
    require(bool(ids) and len(ids) == len(set(ids)) and all(ids), 'empty/duplicate scenario identity')
    profiles = manifest.get('profiles', {})
    for scenario in scenarios:
        require(bool(scenario.get('semantic_contract')) and len(scenario.get('assertions', [])) >= 3, 'generic setup-only evidence')
        require(scenario.get('symbol') not in ('main', 'Setup', 'Bootstrap', 'Initialize', None), 'generic setup-only anchor')
        for profile in ('full-debug', 'full-release'):
            tests = {test['name']: test for test in profiles.get(profile, {}).get('tests', [])}
            for field in ('target', 'process_target'):
                target = scenario.get(field)
                require(target in tests and {'truth', 'determinism'} <= set(tests.get(target, {}).get('labels', [])), 'missing/live-label target: ' + str(target))
        if source_checks:
            path = ROOT / scenario.get('source', '')
            require(path.is_file(), 'evidence source missing')
            if path.is_file():
                source = path.read_text(encoding='utf-8')
                symbol = scenario.get('symbol', '')
                body = function_body(source, symbol)
                require(bool(body) and registered_case(source, symbol), 'dead test symbol')
                for assertion in scenario.get('assertions', []):
                    require(scoped_token(assertion, body) and not assertion.startswith('//'), 'semantic assertion missing from registered case')
    if source_checks:
        generated = (ROOT / 'EngineFramework/DevelopmentInfrastructure/Tests/goal5_framework_composition.h').read_text(encoding='utf-8')
        whole = (ROOT / 'Tests/Truth/goal5_whole_composition.h').read_text(encoding='utf-8')
        adapters = re.findall(r'AddSnapshot\("([^"]+)"', generated) + re.findall(r'AddBytes\("([^"]+)"', whole)
        require(sorted(adapters) == sorted(owners), 'actual typed digest adapter coverage differs from reviewed owners')
        replay = (ROOT / 'Tests/Truth/goal5_replay.h').read_text(encoding='utf-8')
        require('engine.SemanticState().Digest()' in replay and 'before != after' in replay, 'trace is not gated by actual semantic publication')
        require('engine.restorer.CanResume()' in replay and 'engine.SemanticState().Digest() == result.semantic_digest' in replay, 'fresh restore is not verified before continuation')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    review = load_json(REVIEW)
    domain = load_json(ROOT / 'docs/freeze/goal5_determinism_contract.json')
    manifest = load_json(ROOT / 'docs/freeze/ctest_manifest.json')
    errors = validate(review, domain, manifest)
    if args.self_test:
        changes = [lambda r, d, m: r.update(execution_receipt_required=False),
                   lambda r, d, m: r['scenarios'].append(copy.deepcopy(r['scenarios'][0])),
                   lambda r, d, m: r['scenarios'][0].update(symbol='Setup'),
                   lambda r, d, m: r['scenarios'][0].update(assertions=[]),
                   lambda r, d, m: r['scenarios'][0]['assertions'].append('std::filesystem::remove_all(path, error)'),
                   lambda r, d, m: r['scenarios'][0].update(target='NonexistentTest'),
                   lambda r, d, m: d['owners'].pop(),
                   lambda r, d, m: d['owners'].append(d['owners'][0]),
                   lambda r, d, m: next(state for state in d['states'] if state['decision'] == 'INCLUDED').update(digest_adapter='missing.owner'),
                   lambda r, d, m: m['profiles']['full-debug']['tests'].clear()]
        for change in changes:
            r, d, m = copy.deepcopy(review), copy.deepcopy(domain), copy.deepcopy(manifest)
            change(r, d, m)
            if not validate(r, d, m, True): errors.append('malformed evidence accepted')
        if not errors: print('PASS: ten fake/dead/generic/duplicate/missing-owner/out-of-case evidence negatives rejected.')
    if errors: raise ValueError('\n'.join(errors))
    print('PASS: live semantic scenario targets and exact owner adapter coverage; execution qualification remains separate.')


if __name__ == '__main__': main()
