"""Exact finite candidate review; discovery and executed qualification stay separate."""
import argparse
import copy
import json
import re
from pathlib import Path
from goal5_determinism_inventory import generate as discover
from goal5_determinism_inventory import mask_noncode, scopes
from goal5_persistence_contract import encoded, load_json

ROOT = Path(__file__).resolve().parents[2]
REVIEW = ROOT / '_goal5_handoff/determinism_candidate_review.json'
OUTPUT = ROOT / 'docs/freeze/goal5_determinism_reviewed.json'
DECISIONS = {'DETERMINISTIC_BY_CONSTRUCTION', 'CANONICALIZED', 'EXPLICIT_INPUT',
             'BUDGET_NON_SEMANTIC', 'TOLERANCE_DEFINED', 'EXCLUDED_NON_SEMANTIC', 'DEFECT'}


def validate(review, inventory, domain, manifest, cache=None):
    errors = []
    def require(value, message):
        if not value: errors.append(message)
    require(review.get('schema_version') == 1 and review.get('stage') == '5.6.B', 'finite review schema')
    require(review.get('execution_receipt_required') is True, 'review cannot manufacture executed PASS')
    records = review.get('candidates', [])
    actual = {c['candidate_id']: c for c in inventory['candidates']}
    ids = [r.get('candidate_id') for r in records]
    cache = {} if cache is None else cache
    source_cache = cache.setdefault('text', {})
    scope_cache = cache.setdefault('scopes', {})
    def source_text(path):
        if path not in source_cache:
            file = ROOT / path
            source_cache[path] = file.read_text(encoding='utf-8-sig') if file.is_file() else ''
        return source_cache[path]
    def live_anchor(anchor, fallback_path=None):
        match = re.fullmatch(r'(.+):(\d+)::(.+)', anchor)
        if match:
            path, line, token = match.group(1), int(match.group(2)), match.group(3)
            lines = source_text(path).splitlines()
            return 0 < line <= len(lines) and token in lines[line-1] and bool(mask_noncode(lines[line-1]).strip())
        return bool(fallback_path and anchor in mask_noncode(source_text(fallback_path)))
    def scope_code(path, wanted):
        if path not in scope_cache:
            code = mask_noncode(source_text(path)); by_symbol = {}
            for a,b,_,symbol in scopes(code):
                by_symbol.setdefault(symbol, []).append(code[a:b])
            scope_cache[path] = {symbol: re.sub(r'\s+', ' ', '\n'.join(parts)) for symbol,parts in by_symbol.items()}
        return scope_cache[path].get(wanted, '')
    require(len(ids) == len(set(ids)) and set(ids) == set(actual), 'candidate inventory not exact')
    for r in records:
        identity = r.get('candidate_id'); source = actual.get(identity)
        if source is None: continue
        require(r.get('review_status') == 'REVIEWED', 'unreviewed: ' + identity)
        require(r.get('decision') in DECISIONS - {'DEFECT'}, 'unknown/open defect: ' + identity)
        if r.get('decision') == 'EXCLUDED_NON_SEMANTIC':
            require(source['pattern'] == 'THREAD_IDENTITY' or
                    (source['pattern'] == 'WALL_CLOCK' and source['module'] not in
                     {'EngineRuntime/Navigation','EngineRuntime/Resources','EngineRuntime/Streaming'}),
                    'unjustified semantic exclusion: ' + identity)
        require(r.get('source') == source, 'stale exact source context: ' + identity)
        require(bool(r.get('rationale')) and bool(r.get('contract')), 'missing rationale/contract: ' + identity)
        evidence = r.get('evidence', {})
        require(evidence.get('source_context_sha256') == source['source_context_sha256'], 'unbound evidence: ' + identity)
        require(evidence.get('source_path') == source['source_path'] and
                evidence.get('qualified_symbol') == source['qualified_symbol'], 'wrong source evidence: ' + identity)
        test_anchor = evidence.get('test_anchor', '')
        require(live_anchor(test_anchor), 'missing/dead executable anchor: ' + identity)
        test_path = test_anchor.split(':', 1)[0]
        require(bool(evidence.get('assertions')) and all(live_anchor(a, test_path) for a in evidence.get('assertions', [])),
                'missing executable assertions: ' + identity)
        if r.get('decision') == 'CANONICALIZED':
            proof = evidence.get('canonicalizers', [])
            code = scope_code(source['source_path'], source['qualified_symbol'])
            require(bool(proof) and all(p in code for p in proof), 'missing executable ordering proof: ' + identity)
        for profile in ('full-debug', 'full-release'):
            targets = {t['name'] for t in manifest['profiles'][profile]['tests']}
            require(evidence.get('test_target') in targets, 'missing evidence target: ' + identity)
        policy = r.get('policy', {})
        for name in ('ordering', 'floating', 'random_stream', 'wall_clock', 'completion'):
            require(bool(policy.get(name)), 'missing ' + name + ' policy: ' + identity)
        if source['pattern'] == 'AMBIENT_RANDOM':
            require(r.get('decision') == 'EXCLUDED_NON_SEMANTIC', 'ambient semantic random: ' + identity)
        if source['pattern'] == 'WALL_CLOCK':
            require(policy.get('wall_clock') in {'EXCLUDED_DIAGNOSTIC', 'EXPLICIT_COMPLETION_AND_ZERO_TIME_WORK_COUNT'}, 'wall-clock semantic influence: ' + identity)
        if source['pattern'] in {'UNORDERED_ITERATION', 'SERIALIZATION_ITERATION'}:
            require(policy.get('ordering') in {'CANONICAL_STABLE_VALUE_ORDER', 'COMMUTATIVE_PER_KEY_OR_ORDERED_INPUT', 'TECHNICAL_PROJECTION_EXPLICIT_INPUT'}, 'unreviewed observable order: ' + identity)
    included = [s for s in domain['states'] if s['decision'] == 'INCLUDED']
    require({s['digest_adapter'] for s in included} == set(domain['owners']), 'owner digest gap')
    require(len(included) == 152 and len(domain['owners']) == 49, 'durable domain drift')
    return errors


def generated(review):
    return {**review, 'qualification': 'SOURCE_REVIEWED_EXECUTION_REQUIRED'}


def check(self_tests=False, record=False):
    review = load_json(REVIEW); inventory = discover()
    domain = load_json(ROOT / 'docs/freeze/goal5_determinism_contract.json')
    manifest = load_json(ROOT / 'docs/freeze/ctest_manifest.json')
    cache = {}
    errors = validate(review, inventory, domain, manifest, cache)
    if self_tests:
        mutations = [lambda r: r['candidates'].pop(),
                     lambda r: r['candidates'].append(copy.deepcopy(r['candidates'][0])),
                     lambda r: r['candidates'][0].update(review_status='DISCOVERED'),
                     lambda r: r['candidates'][0].update(decision='UNKNOWN'),
                     lambda r: r['candidates'][0].update(decision='DEFECT'),
                     lambda r: next(c for c in r['candidates'] if c['source']['pattern']=='UNORDERED_ITERATION').update(decision='EXCLUDED_NON_SEMANTIC'),
                     lambda r: r['candidates'][0]['source'].update(source_context_sha256='stale'),
                     lambda r: r['candidates'][0]['evidence'].update(test_target='SetupOnlyMissing'),
                     lambda r: r['candidates'][0]['evidence'].update(qualified_symbol='Setup'),
                     lambda r: r['candidates'][0]['evidence'].update(assertions=['// note-only acceptance']),
                     lambda r: next(c for c in r['candidates'] if c['decision']=='CANONICALIZED')['evidence'].update(canonicalizers=['std::sort(not_this_scope)']),
                     lambda r: r['candidates'][0]['policy'].update(random_stream=''),
                     lambda r: next(c for c in r['candidates'] if c['source']['pattern']=='WALL_CLOCK')['policy'].update(wall_clock='AMBIENT_SEMANTIC'),
                     lambda r: next(c for c in r['candidates'] if c['source']['pattern']=='UNORDERED_ITERATION')['policy'].update(ordering='UNREVIEWED'),
                     lambda r: r.update(execution_receipt_required=False)]
        for mutate in mutations:
            bad = copy.deepcopy(review); mutate(bad)
            if not validate(bad, inventory, domain, manifest, cache): errors.append('candidate negative accepted')
    if errors: raise ValueError('\n'.join(errors))
    value = encoded(generated(review))
    if record: OUTPUT.write_text(value, encoding='utf-8', newline='\n')
    elif not OUTPUT.is_file() or OUTPUT.read_text(encoding='utf-8') != value:
        raise ValueError('reviewed determinism registry stale; run owning --record')
    print(f'PASS: exact {len(inventory["candidates"])} reviewed candidates; zero unknown/open defects; execution remains separate.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ('record', 'check', 'self-test'): parser.add_argument('--'+flag, action='store_true')
    args = parser.parse_args(); check(args.self_test, args.record)
