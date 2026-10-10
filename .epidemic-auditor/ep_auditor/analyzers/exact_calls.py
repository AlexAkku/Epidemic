"""Exact paired instrumented intervals, with unreported loss never accepted."""
from collections import Counter, defaultdict
from math import ceil
import hashlib
from statistics import median

from ep_auditor.core.evidence import EvidenceError


def reconstruct_calls(events, symbolizations, *, producer_complete=True):
    stacks, calls, edges, problems = defaultdict(list), [], Counter(), Counter()
    functions = {}
    previous = {}
    for event in events:
        thread = (event['pid'], event['process_birth'], event['tid'], event['thread_birth'])
        prior = previous.get(thread)
        if prior and prior['trace_sha256'] != event['trace_sha256'] and prior['timestamp_ns'] == event['timestamp_ns']:
            problems['ambiguous_cross_module_order'] += 1
        previous[thread] = event
        symbols = symbolizations.get(event['image_base'], {})
        symbol = symbols.get(event['address'])
        identity = symbol['functionId'] if symbol else None
        if identity is None:
            problems['unknown_symbols'] += 1
            continue
        functions[identity] = symbol
        stack = stacks[thread]
        if event['kind'] == 'ENTER':
            call_id = 'CallId:' + hashlib.sha256((event['trace_sha256'] + ':' + str(event['rawOrdinal'])).encode()).hexdigest()
            stack.append({'event': event, 'functionId': identity, 'children_ns': 0,
                          'children_cpu_100ns': 0, 'children_cycles': 0,
                          'callId': call_id, 'parentCallId': stack[-1]['callId'] if stack else None,
                          'caller': stack[-1]['functionId'] if stack else None})
        else:
            if not stack or stack[-1]['functionId'] != identity:
                problems['unmatched_exit'] += 1
                problems['unmatched_enter'] += len(stack)
                stack.clear()
                continue
            frame = stack.pop()
            begin = frame['event']
            duration = event['timestamp_ns'] - begin['timestamp_ns']
            cpu = event['cpu_100ns'] - begin['cpu_100ns']
            cycles = event['cycles'] - begin['cycles']
            if min(duration - frame['children_ns'], cpu - frame['children_cpu_100ns'], cycles - frame['children_cycles']) < 0:
                raise EvidenceError('exact call interval/counters violate nesting')
            call = {'callId': frame['callId'], 'parentCallId': frame['parentCallId'],
                    'functionId': identity, 'caller': frame['caller'], 'thread': thread,
                    'start_ns': begin['timestamp_ns'], 'end_ns': event['timestamp_ns'],
                    'inclusive_ns': duration, 'self_ns': duration - frame['children_ns'],
                    'cpu_inclusive_100ns': cpu, 'cpu_self_100ns': cpu - frame['children_cpu_100ns'],
                    'cycles_inclusive': cycles, 'cycles_self': cycles - frame['children_cycles'],
                    'rawOrdinals': [begin['rawOrdinal'], event['rawOrdinal']], 'confidence': 'derived'}
            call['rawTraceHashes'] = [begin['trace_sha256'], event['trace_sha256']]
            calls.append(call)
            if stack:
                stack[-1]['children_ns'] += duration
                stack[-1]['children_cpu_100ns'] += cpu
                stack[-1]['children_cycles'] += cycles
            if frame['caller']:
                edges[(frame['caller'], identity)] += 1
    problems['unmatched_enter'] += sum(map(len, stacks.values()))
    grouped = defaultdict(list)
    for call in calls:
        grouped[call['functionId']].append(call)
    metrics = []
    for identity, rows in grouped.items():
        durations = sorted(row['inclusive_ns'] for row in rows)
        percentile = lambda fraction: durations[max(0, ceil(fraction * len(durations)) - 1)]
        metrics.append({'functionId': identity, 'symbol': functions[identity], 'calls': len(rows),
            'inclusive_ns': sum(durations), 'self_ns': sum(row['self_ns'] for row in rows),
            'cpu_inclusive_100ns': sum(row['cpu_inclusive_100ns'] for row in rows),
            'cpu_self_100ns': sum(row['cpu_self_100ns'] for row in rows),
            'min_ns': durations[0], 'max_ns': durations[-1], 'mean_ns': sum(durations) / len(durations),
            'p50_ns': median(durations), 'p95_ns': percentile(.95), 'p99_ns': percentile(.99),
            'callers': sorted({row['caller'] for row in rows if row['caller']}),
            'callees': sorted({callee for caller, callee in edges if caller == identity}),
            'caller_count': len({row['caller'] for row in rows if row['caller']}),
            'callee_count': len({callee for caller, callee in edges if caller == identity}),
            'threads': [{'identity': list(thread), 'calls': count} for thread, count in Counter(row['thread'] for row in rows).items()],
            'time_basis': 'instrumented QPC wall intervals; profiler overhead included',
            'cpu_basis': 'GetThreadTimes kernel+user counter deltas; counter granularity limits short calls'})
    rankings = {name: [row['functionId'] for row in sorted(metrics, key=lambda row: row[field], reverse=True)[:25]]
                for name, field in (('wall_total', 'inclusive_ns'), ('cpu_total', 'cpu_inclusive_100ns'),
                                    ('cpu_self', 'cpu_self_100ns'), ('call_count', 'calls'))}
    rankings['spike_candidates'] = [row['functionId'] for row in metrics
        if row['calls'] >= 10 and row['max_ns'] > 5 * max(1, row['p50_ns'])]
    # Derive path IDs in ENTER order, avoiding quadratic work for deep recursion.
    paths, path_nodes, invocation_paths = Counter(), {}, {}
    for call in sorted(calls, key=lambda row: (row['start_ns'], -row['end_ns'])):
        parent = invocation_paths.get(call['parentCallId'])
        path_id = hashlib.sha256(((parent or '') + ':' + call['functionId']).encode()).hexdigest()
        invocation_paths[call['callId']] = path_id
        path_nodes[path_id] = (parent, call['functionId'])
        paths[path_id] += call['self_ns']
    hot_paths = []
    for path_id, cost in paths.most_common(25):
        path, current = [], path_id
        while current is not None and len(path) < 256:
            current, function = path_nodes[current]
            path.append(function)
        hot_paths.append({'path': list(reversed(path)), 'pathId': path_id,
                          'display_truncated': current is not None, 'self_ns': cost})
    return {'complete': producer_complete and not any(problems.values()), 'problems': dict(problems),
            'rankings': rankings,
            'hot_paths': hot_paths,
            'candidate_policy': 'spikes are heuristic (>=10 calls, max >5*p50); absence is not dead-code proof',
            'calls': calls, 'edges': [{'caller': caller, 'callee': callee, 'calls': count} for (caller, callee), count in edges.items()],
            'functions': metrics, 'scope': 'MSVC emitted /Gh /GH hook sites; no claim for uninstrumented dependencies or intrinsics'}
