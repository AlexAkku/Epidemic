"""MS6.5 typed diagnostic facts. No fabricated safety verdict from absent telemetry.

These facts describe tool/producer reports; their provenance remains engine_probe
or tool_report, never proof of a physical violation without external corroboration.
"""
from __future__ import annotations

import re
from collections import Counter

from ep_auditor.core.evidence import EvidenceError

TYPES = {'exception.first_chance', 'exception.second_chance', 'exception.handled',
         'crash', 'assert', 'contract', 'allocation', 'deallocation', 'memory.error',
         'io.open', 'io.close', 'io.read', 'io.write', 'io.seek', 'io.map', 'io.directory',
         'network.connect', 'network.send', 'network.receive', 'api.call'}
MEMORY_ERRORS = {'leak', 'use_after_free', 'double_free', 'out_of_bounds', 'heap_corruption', 'unknown'}
API_FAMILIES = {'file', 'memory', 'synchronization', 'thread', 'socket', 'window', 'graphics', 'other'}
HEX_ID = re.compile(r'^[A-Za-z0-9_.:\-]{1,128}$')


def _uint(value, name):
    if type(value) is not int or not 0 <= value <= (1 << 64) - 1:
        raise EvidenceError('diagnostic invalid ' + name)
    return value


def validate_event(row: dict) -> dict:
    if not isinstance(row, dict) or row.get('type') not in TYPES or (type(row.get('schemaVersion')) is not int or row.get('schemaVersion') != 1):
        raise EvidenceError('diagnostic unknown required type/schema')
    result = dict(row)
    _uint(row.get('timestamp_ns'), 'timestamp')
    _uint(row.get('pid'), 'pid')
    _uint(row.get('process_epoch'), 'process_epoch')
    _uint(row.get('tid'), 'tid')
    if not row['pid'] or not row['tid'] or not row['process_epoch']:
        raise EvidenceError('diagnostic zero process/thread identity')
    if row.get('clock_domain') != 'monotonic_trace_ns':
        raise EvidenceError('diagnostic clock domain unsupported')
    if row.get('provenance') != 'engine_probe':
        raise EvidenceError('tool/target payload cannot claim external provenance')
    kind = row['type']
    if kind.startswith('exception.') or kind == 'crash':
        _uint(row.get('code'), 'exception code')
        _uint(row.get('address'), 'exception address')
        if row.get('handled') is not None and type(row.get('handled')) is not bool:
            raise EvidenceError('invalid handled state')
        if kind in {'exception.first_chance', 'exception.second_chance'} and row.get('handled') is not None:
            raise EvidenceError('chance-only notification cannot prove recovery')
    if kind.startswith('io.') or kind.startswith('network.') or kind == 'api.call':
        if kind == 'api.call' and row.get('family') not in API_FAMILIES:
            raise EvidenceError('unknown API family')
        if kind != 'api.call' and (not isinstance(row.get('operation_id'), str)
                                      or not HEX_ID.fullmatch(row['operation_id'])):
            raise EvidenceError('IO identity missing')
        if kind == 'api.call' and (not isinstance(row.get('name'), str) or not 1 <= len(row['name']) <= 256):
            raise EvidenceError('API identity missing')
        if 'bytes' in row:
            _uint(row['bytes'], 'IO bytes')
        if 'duration_ns' in row:
            _uint(row['duration_ns'], 'IO duration')
    if kind in {'allocation', 'deallocation'}:
        _uint(row.get('address'), 'allocation address')
        _uint(row.get('size_bytes'), 'allocation size')
        if not row['address']:
            raise EvidenceError('null allocation address invalid')
    if kind == 'memory.error' and row.get('error_kind') not in MEMORY_ERRORS:
        raise EvidenceError('unsupported memory diagnostic')
    if row.get('event_id') is not None and (not isinstance(row['event_id'], str)
                                             or not HEX_ID.fullmatch(row['event_id'])):
        raise EvidenceError('invalid diagnostic EventId')
    for name in ('action_id', 'trace_id', 'function_id', 'allocation_id', 'parent_event_id', 'call_id', 'module_id'):
        if row.get(name) is not None and (not isinstance(row[name], str) or not HEX_ID.fullmatch(row[name])):
            raise EvidenceError('invalid diagnostic ' + name)
    if 'exact_call_ordinal' in row or 'exact_module_base' in row:
        if (not row.get('event_id') or _uint(row.get('exact_call_ordinal'), 'exact call ordinal') >= 1048576
                or not _uint(row.get('exact_module_base'), 'exact module base')):
            raise EvidenceError('invalid diagnostic exact call locator')
    if 'stack' in row and (not isinstance(row['stack'], list) or len(row['stack']) > 64
                          or any(type(frame) is not int or not 0 < frame < (1 << 64) for frame in row['stack'])):
        raise EvidenceError('invalid diagnostic stack')
    for name in ('cpu_ns', 'duration_ns', 'main_thread_id'):
        if name in row:
            _uint(row[name], name)
    if 'synchronous' in row and type(row['synchronous']) is not bool:
        raise EvidenceError('invalid IO synchronous state')
    result['provenance'] = 'engine_probe'
    result['confidence'] = 'declared'
    return result


def analyze(events: list[dict], *, complete: bool) -> dict:
    if type(complete) is not bool:
        raise EvidenceError('diagnostic completeness must be boolean')
    if len(events) > 200_000:
        raise EvidenceError('diagnostic analysis capacity exceeded')
    validated, prior, event_ids = [], {}, set()
    for row in events:
        item = validate_event(row)
        event_id = item.get('event_id')
        if event_id is not None:
            if event_id in event_ids:
                raise EvidenceError('duplicate diagnostic EventId')
            event_ids.add(event_id)
        identity = (item['pid'], item['process_epoch'], item['tid'])
        if item['timestamp_ns'] < prior.get(identity, 0):
            raise EvidenceError('diagnostic per-thread clock reversed')
        prior[identity] = item['timestamp_ns']
        validated.append(item)
    counts = Counter(row['type'] for row in validated)
    active, findings, lifetimes = {}, [], []
    live_bytes, peak_bytes, allocation_count = {}, {}, 0
    functions, actions, io_functions, exception_codes = {}, {}, {}, {}
    def finding_fields(row):
        return {key: row.get(key) for key in ('event_id', 'action_id', 'trace_id', 'parent_event_id',
            'call_id', 'function_id', 'module_id', 'source_ref', 'source_ordinal', 'pid', 'process_epoch')}
    # Producer files can interleave. Address ownership is process-scoped, while
    # chronology must follow the shared monotonic clock, not file/thread order.
    for row in sorted(validated, key=lambda item: (item['timestamp_ns'], item.get('event_id', ''))):
        process = (row['pid'], row['process_epoch'])
        if row['type'] == 'allocation':
            key = (row['pid'], row['process_epoch'], row['address'])
            if key in active:
                findings.append({'kind': 'allocation_address_reused_without_free', 'address': row['address'],
                                 'status': 'candidate', 'provenance': 'engine_probe', **finding_fields(row)})
                live_bytes[process] -= active[key]['size_bytes']
            active[key] = row
            allocation_count += 1
            live_bytes[process] = live_bytes.get(process, 0) + row['size_bytes']
            peak_bytes[process] = max(peak_bytes.get(process, 0), live_bytes[process])
            for field, totals in (('function_id', functions), ('action_id', actions)):
                if row.get(field):
                    total = totals.setdefault(row[field], {'count': 0, 'bytes': 0})
                    total['count'] += 1
                    total['bytes'] += row['size_bytes']
        elif row['type'] == 'deallocation':
            key = (row['pid'], row['process_epoch'], row['address'])
            if key not in active:
                findings.append({'kind': 'free_without_observed_allocation', 'address': row['address'],
                                 'status': 'candidate', 'provenance': 'engine_probe', **finding_fields(row)})
            else:
                allocation = active.pop(key)
                live_bytes[process] -= allocation['size_bytes']
                lifetimes.append({'pid': row['pid'], 'process_epoch': row['process_epoch'],
                                  'address': row['address'], 'size_bytes': allocation['size_bytes'],
                                  'duration_ns': row['timestamp_ns'] - allocation['timestamp_ns'],
                                  'allocation_event_id': allocation.get('event_id'),
                                  'deallocation_event_id': row.get('event_id'),
                                  'function_id': allocation.get('function_id'),
                                  'action_id': allocation.get('action_id')})
        elif row['type'] == 'memory.error':
            findings.append({'kind': row['error_kind'], 'status': 'reported_not_independently_verified',
                             'provenance': 'engine_probe', **finding_fields(row)})
        if row['type'].startswith('io.'):
            label = row.get('function_id', 'UNKNOWN')
            total = io_functions.setdefault(label, {'count': 0, 'bytes': 0, 'measured_duration_ns': 0,
                                                    'duration_samples': 0, 'synchronous_main_thread': 0})
            total['count'] += 1
            total['bytes'] += row.get('bytes', 0)
            if 'duration_ns' in row:
                total['measured_duration_ns'] += row['duration_ns']
                total['duration_samples'] += 1
            if row.get('synchronous') is True and row.get('main_thread_id') == row['tid']:
                total['synchronous_main_thread'] += 1
        if row['type'] == 'exception.first_chance':
            total = exception_codes.setdefault(str(row['code']), {'count': 0, 'measured_cpu_ns': 0,
                                                                  'cpu_samples': 0})
            total['count'] += 1
            if 'cpu_ns' in row:
                total['measured_cpu_ns'] += row['cpu_ns']
                total['cpu_samples'] += 1
    span = max((row['timestamp_ns'] for row in validated), default=0) - min(
        (row['timestamp_ns'] for row in validated), default=0)
    # Open allocations are not leaks unless complete whole-process lifetime is proven.
    unresolved = sum('exact_call_ordinal' in row and not row.get('call_id') for row in validated)
    return {'schemaVersion': 1, 'complete': complete and not unresolved, 'unresolved_call_links': unresolved, 'counts': dict(counts),
            'live_allocation_candidates': len(active), 'findings': findings,
            'leak_verdict': 'UNKNOWN', 'handled_exception_verdict': 'UNKNOWN',
            'allocation_metrics': {'lifetimes': lifetimes, 'count': allocation_count,
                'rate_per_second': allocation_count * 1e9 / span if span else None,
                'peak_observed_bytes_by_process': [{'pid': pid, 'process_epoch': epoch, 'bytes': count}
                    for (pid, epoch), count in sorted(peak_bytes.items())],
                'by_function': functions, 'by_action': actions,
                'whole_heap_observed': False, 'unambiguous': not findings},
            'io_by_function': io_functions, 'exception_codes': exception_codes,
            'observation_limit': 'engine-probe claims; absent/partial/uncorrelated telemetry is UNKNOWN',
            'provenance': 'engine_probe', 'confidence': 'derived'}


def analyze_exception_snapshot(snapshot: dict) -> dict:
    """Debugger facts; chance notifications alone never prove handler recovery."""
    rows = snapshot.get('exceptions')
    if not isinstance(rows, list) or len(rows) > 4096:
        raise EvidenceError('invalid native exception scope')
    frequency = _uint(snapshot.get('qpc_frequency'), 'QPC frequency')
    if not frequency:
        raise EvidenceError('native exception clock unavailable')
    codes, facts = {}, []
    for index, row in enumerate(rows):
        if not isinstance(row, dict) or type(row.get('first_chance')) is not bool:
            raise EvidenceError('invalid native exception chance')
        for field in ('code', 'address', 'tid', 'qpc'):
            _uint(row.get(field), 'exception ' + field)
        if row['code'] > 0xFFFFFFFF or not row['address'] or not 0 < row['tid'] <= 0xFFFFFFFF:
            raise EvidenceError('invalid native exception identity')
        stack = row.get('stack')
        if not isinstance(stack, list) or not 0 < len(stack) <= 32:
            raise EvidenceError('native exception stack missing')
        fact = {'code': row['code'], 'tid': row['tid'], 'first_chance': row['first_chance'],
            'classification': 'UNKNOWN' if row['first_chance'] else 'unhandled_second_chance',
            'raw_json_pointer': '/exceptions/' + str(index), 'continuation_observed': False}
        total = codes.setdefault(str(row['code']), {'first_chance_count': 0, 'second_chance_count': 0,
            'thread_cpu_interval_100ns': 0, 'cpu_interval_samples': 0})
        total['first_chance_count' if row['first_chance'] else 'second_chance_count'] += 1
        if 'continuation_qpc' in row:
            end = _uint(row['continuation_qpc'], 'continuation clock')
            if not row['first_chance'] or end < row['qpc']:
                raise EvidenceError('native continuation order invalid')
            fact['continuation_observed'] = True
            fact['continuation_interval_ns'] = (end - row['qpc']) * 1_000_000_000 // frequency
            if 'thread_cpu_interval_100ns' in row:
                cpu = _uint(row['thread_cpu_interval_100ns'], 'thread CPU interval')
                total['thread_cpu_interval_100ns'] += cpu
                total['cpu_interval_samples'] += 1
        elif 'thread_cpu_interval_100ns' in row:
            raise EvidenceError('CPU interval lacks matching continuation')
        facts.append(fact)
    return {'schemaVersion': 1, 'complete': snapshot.get('complete') is True, 'codes': codes, 'events': facts,
        'provenance': 'external', 'confidence': 'derived',
        'cpu_scope': 'measured thread interval from notification to target continuation marker; not handler-exclusive cost',
        'handled_scope': 'marker proves execution after notification; generic handler completion remains UNKNOWN'}


def analyze_api_snapshot(snapshot: dict) -> dict:
    """Derive entry-stack edges from the owned native observer's raw snapshot.

    The collector must bind the helper binary and raw stdout. This decoder is
    never used for target-emitted diagnostic streams, whose trust stays declared.
    """
    if (not isinstance(snapshot, dict) or type(snapshot.get('schemaVersion')) is not int
            or snapshot['schemaVersion'] != 1 or snapshot.get('provenance') != 'external'
            or type(snapshot.get('complete')) is not bool
            or type(snapshot.get('api_entry_scope_complete')) is not bool):
        raise EvidenceError('invalid external API snapshot schema')
    for field in ('pid', 'process_epoch', 'main_thread_id', 'qpc_frequency'):
        if not _uint(snapshot.get(field), field):
            raise EvidenceError('external API identity/clock missing')
    exit_code = _uint(snapshot.get('exit_code'), 'target exit code')
    if exit_code > 0xFFFFFFFF:
        raise EvidenceError('invalid target exit code')
    calls = snapshot.get('api_calls')
    if not isinstance(calls, list) or len(calls) > 4096:
        raise EvidenceError('external API observation capacity exceeded')
    counts, nodes, edges = Counter(), {}, []
    io_callers = {}
    for ordinal, call in enumerate(calls):
        if (not isinstance(call, dict) or not isinstance(call.get('name'), str)
                or not 1 <= len(call['name']) <= 256 or call.get('family') not in API_FAMILIES):
            raise EvidenceError('invalid observed API entry')
        for field in ('tid', 'qpc', 'address'):
            if not _uint(call.get(field), field):
                raise EvidenceError('invalid observed API identity')
        if call['name'] in {'ReadFile', 'WriteFile'}:
            if (type(call.get('main_thread')) is not bool or type(call.get('synchronous_attempt')) is not bool
                    or call['main_thread'] != (call['tid'] == snapshot['main_thread_id'])):
                raise EvidenceError('invalid API IO thread/attempt flags')
            _uint(call.get('requested_bytes'), 'requested bytes')
        stack = call.get('stack')
        if not isinstance(stack, list) or not stack or len(stack) > 32:
            raise EvidenceError('external API stack unavailable or over capacity')
        counts[call['name']] += 1
        path = []
        for frame in stack:
            if not isinstance(frame, dict) or (frame.get('function') is not None
                    and (not isinstance(frame['function'], str) or len(frame['function']) > 2048)):
                raise EvidenceError('invalid API stack frame')
            address = _uint(frame.get('raw_address'), 'frame address')
            if not address:
                raise EvidenceError('null API stack frame')
            base = _uint(frame.get('module_base'), 'module base')
            epoch = _uint(frame.get('module_load_qpc'), 'module load clock')
            identity = f"{snapshot['pid']}:{snapshot['process_epoch']}:{base}:{epoch}:{address}"
            nodes[identity] = {'id': identity, 'raw_address': address, 'function': frame.get('function'),
                               'module_base': base, 'module_load_qpc': epoch,
                               'identity_scope': 'instruction address within observed module lifetime; FunctionId unresolved'}
            if not path or path[-1] != identity:
                path.append(identity)
        for child, parent in zip(path, path[1:]):
            edges.append({'caller': parent, 'callee': child, 'api': call['name'], 'raw_ordinal': 0,
                          'raw_json_pointer': '/api_calls/' + str(ordinal), 'tid': call['tid']})
        if call['name'] in {'ReadFile', 'WriteFile'}:
            caller = path[1] if len(path) > 1 else 'unresolved'
            total = io_callers.setdefault(caller, {'read_attempts': 0, 'write_attempts': 0,
                'requested_bytes': 0, 'synchronous_main_thread_attempts': 0})
            total['read_attempts' if call['name'] == 'ReadFile' else 'write_attempts'] += 1
            total['requested_bytes'] += call['requested_bytes']
            total['synchronous_main_thread_attempts'] += int(call['main_thread'] and call['synchronous_attempt'])
    return {'schemaVersion': 1, 'complete': snapshot['complete'] and snapshot['api_entry_scope_complete'],
            'graph_type': 'observed API-entry unwind stacks', 'nodes': list(nodes.values()), 'edges': edges,
            'counts': dict(counts), 'io_by_caller': io_callers, 'provenance': 'external', 'confidence': 'derived',
            'call_outcomes': 'UNKNOWN', 'disk_latency': 'UNKNOWN',
            'scope': 'armed trusted system exports; entry proves attempt, not successful IO or network packets'}
