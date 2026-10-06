"""Semantic-neutral, production-only nondeterminism discovery (5.6.A).

Structural lexical scopes preserve source positions and qualified owners. A finding
is not a defect, exclusion or PASS; every current context requires finite review.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.dont_write_bytecode = True
from module_dossiers import ROOT, discover_modules
from goal5_persistence_inventory import production_files, eligible_path
from goal5_persistence_contract import encoded

OUTPUT = ROOT / 'docs/freeze/goal5_determinism_candidates.json'
PATTERNS = {
    'UNORDERED_STORAGE': r'\b(?:std::)?unordered_(?:map|set|multimap|multiset)\b',
    'AMBIENT_RANDOM': r'\b(?:std::)?random_device\b',
    'WALL_CLOCK': r'\b(?:steady_clock|system_clock|high_resolution_clock)\b',
    'POINTER_HASH_ORDER': r'\b(?:hash|less|greater)\s*<[^;{}\n]*\*|\breinterpret_cast\s*<\s*(?:std::)?(?:u?intptr_t|size_t|u?int(?:32|64)_t)\s*>',
    'THREAD_IDENTITY': r'\b(?:this_thread\s*::\s*get_id|thread\s*::\s*id|thread_id\w*|owner_thread_\w*)\b',
    'ASYNC_COMPLETION': r'\b(?:Complete\w*|OnComplete\w*|completion\w*|completed_at\w*|pending_completions\w*)\b',
    'TIMING_IDENTITY': r'\b(?:fetch_add|next_direct_order_|next_event_sequence_|next_id_|next_sequence_)\b',
    'SAME_TIME_ORDER': r'\b(?:Schedule(?:At|After|Task|Event|Trigger)?|priority_queue|multimap|MergeSubmittedBatches|ExpireDueFacts)\s*(?:\(|<)',
    'FLOATING_CALCULATION': r'\b(?:std::)?(?:sqrt|cos|sin|acos|atan2|hypot|round|lround|llround|floor|ceil|fmod|pow|lerp)\s*\(',
    'PARALLEL_REDUCTION': r'\b(?:reduce|transform_reduce|execution\s*::\s*(?:par|par_unseq))\b',
}


def sha(value):
    return hashlib.sha256(value.encode()).hexdigest()


def mask_noncode(text):
    """Mask comments/literals without moving offsets; C++ digit separators survive."""
    out = list(text); index = 0
    def blank(begin, end):
        out[begin:end] = ['\n' if char == '\n' else ' ' for char in text[begin:end]]
    while index < len(text):
        begin = index
        if text.startswith('//', index):
            end = text.find('\n', index); index = len(text) if end < 0 else end
        elif text.startswith('/*', index):
            end = text.find('*/', index + 2); index = len(text) if end < 0 else end + 2
        elif text.startswith('R"', index):
            opening = text.find('(', index + 2, index + 20)
            if opening < 0: index += 1; continue
            closing = text.find(')' + text[index + 2:opening] + '"', opening + 1)
            index = len(text) if closing < 0 else closing + len(text[index + 2:opening]) + 2
        elif text[index] in '\"\'' and not (text[index] == "'" and index > 0 and text[index-1].isalnum() and index+1 < len(text) and text[index+1].isalnum()):
            quote = text[index]; index += 1
            while index < len(text):
                if text[index] == '\\': index += 2
                elif text[index] == quote: index += 1; break
                else: index += 1
        else:
            index += 1; continue
        blank(begin, min(index, len(text)))
    return ''.join(out)


def scopes(code):
    pairs, stack = [], []
    for token in re.finditer(r'[{}]', code):
        if token[0] == '{': stack.append(token.start())
        elif stack: pairs.append((stack.pop(), token.start() + 1))
    found = []
    for begin, end in sorted(pairs):
        boundary = max(code.rfind(';', 0, begin), code.rfind('}', 0, begin), code.rfind('{', 0, begin)) + 1
        prefix = code[boundary:begin].strip()
        namespace = re.search(r'\bnamespace\s+([\w:]+)\s*$', prefix)
        owner = re.search(r'\b(?:class|struct)\s+(\w+)(?:\s+final)?(?:\s*:[^{}]+)?\s*$', prefix)
        function = re.search(r'([\w:~]+)\s*\([^;{}\[\]]*\)\s*(?:const\s*)?(?:noexcept(?:\([^)]*\))?\s*)?(?:override\s*)?(?:final\s*)?(?:->[^{}]+)?$', prefix, re.S)
        if namespace: kind, name = 'NAMESPACE', namespace[1]
        elif owner: kind, name = 'TYPE', owner[1]
        elif function and function[1] not in {'if', 'for', 'while', 'switch', 'catch', 'constexpr'}:
            kind, name = 'FUNCTION', function[1]
        else: continue
        parents = [item[3] for item in found if item[0] < begin < item[1] and item[2] != 'FUNCTION']
        found.append((boundary, end, kind, '::'.join(parents[-1:] + [name])))
    return found


def unordered_names(code):
    names = set()
    for match in re.finditer(r'\bunordered_(?:map|set|multimap|multiset)\s*<', code):
        depth, end = 1, match.end()
        while end < len(code) and depth:
            if code[end] == '<': depth += 1
            elif code[end] == '>': depth -= 1
            end += 1
        declaration = re.match(r'\s*([A-Za-z_]\w*)\s*(?:[;={]|$)', code[end:])
        if declaration: names.add(declaration[1])
    return names


def scan_text(module, path, text, owner_state_ids=(), module_unordered_names=()):
    if not eligible_path(Path(path)): return []
    code = mask_noncode(text); contexts = scopes(code)
    local_unordered_names = unordered_names(code) | set(module_unordered_names)
    matches = [(kind, hit) for kind, pattern in PATTERNS.items() for hit in re.finditer(pattern, code)]
    # Variable-only semantic branches must not disappear merely because no float
    # literal occurs in the comparison (e.g. cosine >= threshold).
    floating_names = set(re.findall(r'\b(?:float|double|long\s+double)\s+(\w+)', code))
    floating_names.update(re.findall(r'\b(?:cosine|threshold|distance|cost|attenuation|radius|scale|delta|weight)\w*\b', code, re.I))
    pointer_declaration = r'(?:^|[;{}(,])\s*(?:const\s+)?[\w:]+(?:\s*<[^;{}]*>)?\s*\*\s*(\w+)'
    for hit in re.finditer(r'\b(?:if|while)\s*\([^;{}]*?(?:<=|>=|<|>|==|!=)[^;{}]*?\)|\breturn\s+(?![\w:]+\s*<)[^;{}]*?(?:<=|>=|<|>|==|!=)[^;{}]*;', code, re.S):
        names = set(re.findall(r'\b\w+\b', hit[0]))
        if names & floating_names or re.search(r'\b\d+\.\d+', hit[0]): matches.append(('FLOATING_BRANCH', hit))
        enclosing = [scope for scope in contexts if scope[0] <= hit.start() < scope[1] and scope[2] == 'FUNCTION']
        scope = min(enclosing, key=lambda scope: scope[1]-scope[0], default=(0,len(code),'FILE',path))
        pointer_names = set(re.findall(pointer_declaration, code[scope[0]:scope[1]], re.M))
        for comparison in re.finditer(r'\b(\w+)\s*(?:<=|>=|<(?!<)|>(?!>))\s*(\w+)\b', hit[0]):
            if comparison[1] in pointer_names and comparison[2] in pointer_names:
                matches.append(('ADDRESS_ORDER', hit)); break
    for hit in re.finditer(r'[^;{}\n]*(?:seed|Seed)[^;{}\n]*(?:now\s*\(|get_id|process|clock)[^;{}\n]*', code):
        matches.append(('CLOCK_PROCESS_SEED', hit))
    # Serialization loops are candidates even when the container type is hidden
    # behind an alias; review must establish order separately from byte encoding.
    for hit in re.finditer(r'\bfor\s*\([^;{}]*:[^;{}]*\)', code):
        enclosing = [scope for scope in contexts if scope[0] <= hit.start() < scope[1]]
        if any(re.search(r'Serializ|Encod|Captur|Snapshot|Save|Write', scope[3], re.I) for scope in enclosing):
            matches.append(('SERIALIZATION_ITERATION', hit))
        container = hit[0].rsplit(':', 1)[-1]
        if set(re.findall(r'\b\w+\b', container)) & local_unordered_names:
            matches.append(('UNORDERED_ITERATION', hit))
    records = {}
    for kind, hit in matches:
        enclosing = [scope for scope in contexts if scope[0] <= hit.start() < scope[1]]
        selected = min((scope for scope in enclosing if scope[2] in {'FUNCTION', 'TYPE'}), key=lambda scope: scope[1]-scope[0], default=None)
        begin, end, context, symbol = selected if selected else (0, len(text), 'FILE', Path(path).stem)
        expression_begin = max(code.rfind('\n', 0, hit.start()), code.rfind(';', 0, hit.start())) + 1
        expression_end = code.find('\n', hit.end()); expression_end = len(code) if expression_end < 0 else expression_end
        expression = re.sub(r'\s+', ' ', code[expression_begin:expression_end]).strip()
        identity = 'G5D-' + sha(module + '\n' + path + '\n' + symbol + '\n' + kind)[:20]
        record = {'candidate_id': identity, 'module': module, 'source_path': path, 'qualified_symbol': symbol,
                  'pattern': kind, 'normalized_expression': expression, 'context_kind': context,
                  'owner_state_ids': sorted(owner_state_ids),
                  'deterministic_relevance': 'REVIEW_REQUIRED' if owner_state_ids else 'MODULE_APPLICABILITY_REVIEW_REQUIRED',
                  'source_context_sha256': sha(re.sub(r'\s+', ' ', text[begin:end]).strip()),
                  'context_hashes': [sha(re.sub(r'\s+', ' ', text[begin:end]).strip())],
                  'line_hint': text.count('\n', 0, hit.start()) + 1, 'occurrences': 1}
        if identity in records:
            records[identity]['occurrences'] += 1
            expressions = set(records[identity]['normalized_expression'].split('\n'))
            expressions.add(expression)
            records[identity]['normalized_expression'] = '\n'.join(sorted(expressions))
            hashes = sorted(set(records[identity]['context_hashes']) | set(record['context_hashes']))
            records[identity]['context_hashes'] = hashes
            records[identity]['source_context_sha256'] = hashes[0] if len(hashes) == 1 else sha('\n'.join(hashes))
        else: records[identity] = record
    return sorted(records.values(), key=lambda record: (record['module'], record['qualified_symbol'], record['pattern'], record['candidate_id']))


def generate():
    persistence = json.loads((ROOT / 'docs/freeze/goal5_persistence_contract.json').read_text(encoding='utf-8'))
    records, file_count = [], 0
    modules = discover_modules()
    for module in modules:
        owners = [identity for identity, state in persistence['candidates'].items() if state['module'] == module.key and state['deterministic_relevance'] != 'NO']
        paths = production_files(module)
        unordered = set().union(*(unordered_names(mask_noncode(path.read_text(encoding='utf-8-sig'))) for path in paths))
        for path in paths:
            file_count += 1
            records.extend(scan_text(module.key, path.relative_to(ROOT).as_posix(), path.read_text(encoding='utf-8-sig'), owners, unordered))
    if len({record['candidate_id'] for record in records}) != len(records): raise ValueError('duplicate determinism identity')
    return {'schema_version': 1, 'stage': '5.6.A', 'semantics': 'DISCOVERY_ONLY_NO_REVIEW_OR_PASS',
            'modules': [module.key for module in modules], 'production_files': file_count,
            'pattern_counts': dict(sorted(Counter(record['pattern'] for record in records).items())), 'candidates': records}


def self_test():
    source = '''namespace demo { struct Owner { std::unordered_map<int,int> values; double cosine, threshold;
    void Run() { std::random_device random; auto now=std::steady_clock::now();
      std::hash<Thing*> hash; auto address=reinterpret_cast<uintptr_t>(this); std::thread::id thread;
      CompleteWork(); next_id_.fetch_add(1); ScheduleAt(now); std::sqrt(1.0); std::reduce(first,last);
      if (cosine >= threshold) {} auto seed = clock.now(); Thing *a; Thing *b; if (a < b) {} return cosine >= threshold;
    } void CaptureSnapshot() { for (auto &item : values) Write(item); } }; }'''
    records = scan_text('EngineFramework/Fake', 'EngineFramework/Fake/src/fake.cpp', source)
    assert set(PATTERNS) | {'FLOATING_BRANCH','ADDRESS_ORDER','CLOCK_PROCESS_SEED','SERIALIZATION_ITERATION','UNORDERED_ITERATION'} <= {r['pattern'] for r in records}
    assert any(r['context_kind'] == 'FUNCTION' and 'Run' in r['qualified_symbol'] for r in records)
    assert all('decision' not in r and 'review_status' not in r for r in records)
    literal = '/* std::random_device */ // steady_clock\n const char *s="unordered_map"; auto raw=R"tag(thread::id)tag"; auto n=1\'000\'000;'
    assert not scan_text('EngineFramework/Fake', 'EngineFramework/Fake/src/fake.cpp', literal)
    assert "1'000'000" in mask_noncode(literal)
    for excluded in ['Tests', 'tests', 'generated', 'Deltas']:
        assert not scan_text('EngineFramework/Fake', f'EngineFramework/Fake/{excluded}/fake.cpp', source)
    diagnostic = scan_text('EngineBase/Diagnostics', 'EngineBase/Diagnostics/src/log.cpp', 'void Log() { auto t=std::steady_clock::now(); }')
    assert diagnostic and diagnostic[0]['deterministic_relevance'] == 'MODULE_APPLICABILITY_REVIEW_REQUIRED'
    moved = scan_text('EngineFramework/Fake', 'EngineFramework/Fake/src/fake.cpp', '\n\n'+source)
    assert {r['candidate_id'] for r in records} == {r['candidate_id'] for r in moved}
    changed = scan_text('EngineFramework/Fake', 'EngineFramework/Fake/src/fake.cpp', source.replace('CompleteWork()', 'CompleteOtherWork()'))
    assert {r['source_context_sha256'] for r in records} != {r['source_context_sha256'] for r in changed}
    assert not any(r['pattern'] == 'ADDRESS_ORDER' for r in scan_text('EngineFramework/Fake', 'EngineFramework/Fake/src/fake.cpp', 'void Run() { Item* item; if (item->count > 1) {} }'))
    lambda_context = scan_text('EngineFramework/Fake', 'EngineFramework/Fake/src/fake.cpp', 'void Run() { std::sort(a.begin(),a.end(),[](auto x,auto y) { return x.cost < y.cost; }); }')
    assert lambda_context and all(r['qualified_symbol'] == 'Run' for r in lambda_context)
    print('PASS: 15 pattern classes, qualified scopes, literals/digit separators, excluded paths, explicit diagnostics review, stable IDs, member-vs-address ordering and stale-context detection')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ['record','check','self-test']: parser.add_argument('--'+option, action='store_true')
    args = parser.parse_args()
    if args.self_test: self_test()
    if args.record or args.check:
        data = generate(); value = encoded(data)
        if args.record: OUTPUT.write_text(value, encoding='utf-8', newline='\n')
        elif not OUTPUT.is_file() or OUTPUT.read_text(encoding='utf-8') != value: raise ValueError('determinism inventory stale; regenerate explicitly')
        print(f'PASS: {len(data["modules"])} modules, {data["production_files"]} production files, {len(data["candidates"])} semantic-neutral findings; {data["pattern_counts"]}')


if __name__ == '__main__': main()
