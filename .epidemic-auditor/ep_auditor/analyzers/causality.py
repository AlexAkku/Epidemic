"""MS6.6 explicit-correlation DAG. Never infer causality from timestamp proximity.

Call-tree relations (function nesting) are NOT causal edges. A causal edge needs
an explicit correlation token shared by a source event and a consume event.
"""
from __future__ import annotations

from bisect import bisect_left, bisect_right
from collections import Counter, defaultdict
import re

from ep_auditor.core.evidence import EvidenceError

KINDS = {
    'job.submit': 'job.execute', 'event.publish': 'event.handle',
    'command.dispatch': 'command.execute', 'async.suspend': 'async.resume',
    'io.submit': 'io.complete', 'physics.dispatch': 'physics.callback',
    'render.submit': 'render.execute', 'persistence.begin': 'persistence.commit',
    'entity.create': 'entity.change', 'state.request': 'state.change',
    'event.handler.submit': 'event.handler.execute',
    'async.submit': 'async.execute',
}
AUXILIARY = {'action.start', 'action.end', 'input.accepted', 'scenario.start', 'scenario.end',
             'capture.trigger', 'capture.checkpoint', 'entity.created', 'entity.lifecycle', 'state.changed'}
ALTERNATIVE_TERMINALS = {'job.cancel': 'job.submit', 'persistence.fail': 'persistence.begin',
                         'persistence.rollback': 'persistence.begin', 'io.cancel': 'io.submit'}
TERMINAL = set(ALTERNATIVE_TERMINALS)
MAX_EVENTS = 200_000
MAX_EDGES = 200_000
MAX_NS = 10**19
IDENTIFIER = re.compile(r'[A-Za-z0-9_.:-]{1,128}\Z')


def _id(row, key, required=True):
    value = row.get(key)
    if value is None and not required:
        return None
    if not isinstance(value, str) or not IDENTIFIER.fullmatch(value):
        raise EvidenceError('causal identity invalid: ' + key)
    return value


def _uint(row, key):
    value = row.get(key)
    if type(value) is not int or not 0 <= value <= (1 << 64) - 1:
        raise EvidenceError('causal invalid clock/identity: ' + key)
    return value


def validate_event(row):
    if not isinstance(row, dict) or type(row.get('schemaVersion')) is not int or row['schemaVersion'] != 1:
        raise EvidenceError('unsupported causal probe schema')
    kind = row.get('type')
    if kind not in set(KINDS) | set(KINDS.values()) | AUXILIARY | TERMINAL:
        raise EvidenceError('unsupported required causal event kind')
    _id(row, 'event_id')
    if _uint(row, 'timestamp_ns') > MAX_NS:
        raise EvidenceError('causal timestamp too large')
    pid, tid = _uint(row, 'pid'), _uint(row, 'tid')
    epoch = _uint(row, 'process_epoch')
    if not pid or not tid or not epoch or row.get('clock_domain') != 'monotonic_trace_ns':
        raise EvidenceError('causal process/thread/clock scope absent')
    if row.get('provenance') != 'engine_probe':
        raise EvidenceError('shadow probe cannot claim independent observation')
    for key in ('trace_id', 'span_id', 'parent_span_id', 'action_id', 'scenario_id', 'entity_id',
                'job_id', 'command_id', 'function_id', 'module_id', 'call_id', 'finding_id', 'parent_event_id'):
        _id(row, key, False)
    if kind in set(KINDS) | set(KINDS.values()) | TERMINAL:
        _id(row, 'correlation_id')
    if 'exact_call_ordinal' in row or 'exact_module_base' in row:
        if _uint(row, 'exact_call_ordinal') >= 1048576 or not _uint(row, 'exact_module_base'):
            raise EvidenceError('invalid explicit exact call locator')
    if kind.startswith(('action.', 'scenario.')):
        _id(row, 'action_id' if kind.startswith('action.') else 'scenario_id')
    if 'name' in row and (not isinstance(row['name'], str) or not 0 < len(row['name'].encode('utf-8')) <= 256
                          or any(ord(c) < 32 for c in row['name'])):
        raise EvidenceError('invalid bounded Action name')
    if kind in {'capture.trigger', 'capture.checkpoint'}:
        _id(row, 'trigger_id')
    return dict(row, confidence='declared')


def causal_graph(events: list[dict], *, producer_complete: bool) -> dict:
    if type(producer_complete) is not bool:
        raise EvidenceError('causal producer completion must be boolean')
    if len(events) > MAX_EVENTS:
        raise EvidenceError('causal capacity exceeded')
    observations, prior, ids = [], {}, set()
    for row in events:
        event = validate_event(row)
        if event['event_id'] in ids:
            raise EvidenceError('duplicate causal EventId')
        ids.add(event['event_id'])
        thread = (event['pid'], event['process_epoch'], event['tid'])
        timestamp = event['timestamp_ns']
        if timestamp < prior.get(thread, 0):
            raise EvidenceError('causal per-thread timestamp reversed')
        prior[thread] = timestamp
        observations.append(event)

    sources, consumers = defaultdict(list), defaultdict(list)
    for event in observations:
        if event['type'] in KINDS:
            sources[(event['pid'], event['process_epoch'], event['type'], event['correlation_id'])].append(event)
        elif event['type'] in set(KINDS.values()) | TERMINAL:
            consumers[(event['pid'], event['process_epoch'], event['type'], event['correlation_id'])].append(event)
    edges, unresolved, problems = [], [], Counter()
    for event in observations:
        if 'exact_call_ordinal' in event and not event.get('call_id'):
            problems['unresolved_call_link'] += 1
    for (pid, epoch, kind, corr), emitted in sources.items():
        matched = consumers.get((pid, epoch, KINDS[kind], corr), [])
        for terminal, source in ALTERNATIVE_TERMINALS.items():
            if source == kind:
                matched = matched + consumers.get((pid, epoch, terminal, corr), [])
        # One immutable contact can be delivered to multiple explicit consumers.
        # The producer identity remains unique; timestamp proximity is irrelevant.
        fanout = kind == 'physics.dispatch'
        if len(emitted) != 1 or not matched or (not fanout and len(matched) != 1):
            problems['ambiguous' if len(emitted) > 1 or len(matched) > 1 else 'missing_consumer'] += 1
            unresolved.extend(x['event_id'] for x in emitted)
            continue
        start = emitted[0]
        for end in matched:
            if start['timestamp_ns'] > end['timestamp_ns']:
                problems['backwards_causality'] += 1
                unresolved.extend((start['event_id'], end['event_id']))
                continue
            edges.append({'source': start['event_id'], 'target': end['event_id'], 'kind': kind,
                          'correlation_id': corr, 'from_thread': [start['pid'], start['process_epoch'], start['tid']],
                          'to_thread': [end['pid'], end['process_epoch'], end['tid']], 'provenance': 'engine_probe',
                          'confidence': 'declared', 'latency_ns': end['timestamp_ns'] - start['timestamp_ns']})
    for (pid, epoch, kind, corr), consumed in consumers.items():
        reverse = ALTERNATIVE_TERMINALS.get(kind) or next((k for k, v in KINDS.items() if v == kind), None)
        if not sources.get((pid, epoch, reverse, corr)):
            problems['missing_producer'] += len(consumed)
            unresolved.extend(x['event_id'] for x in consumed)
    # Explicit inherited execution context connects different handoff families.
    # Shared TraceId, thread and nearby timestamps never establish this edge.
    by_id = {row['event_id']: row for row in observations}
    pairs = {(edge['source'], edge['target']) for edge in edges}
    for event in observations:
        parent_id = event.get('parent_event_id')
        if not parent_id:
            continue
        parent = by_id.get(parent_id)
        if (not parent or (parent['pid'], parent['process_epoch']) != (event['pid'], event['process_epoch'])):
            problems['missing_parent'] += 1
            unresolved.append(event['event_id'])
            continue
        if parent['timestamp_ns'] > event['timestamp_ns']:
            problems['backwards_causality'] += 1
            unresolved.append(event['event_id'])
            continue
        pair = (parent_id, event['event_id'])
        if pair not in pairs:
            edges.append({'source': parent_id, 'target': event['event_id'], 'kind': 'context.parent',
                          'from_thread': [parent['pid'], parent['process_epoch'], parent['tid']],
                          'to_thread': [event['pid'], event['process_epoch'], event['tid']],
                          'provenance': 'engine_probe', 'confidence': 'declared',
                          'latency_ns': event['timestamp_ns'] - parent['timestamp_ns']})
            pairs.add(pair)
        if len(edges) > MAX_EDGES:
            raise EvidenceError('causal edge capacity exceeded')
    if len(edges) > MAX_EDGES:
        raise EvidenceError('causal edge capacity exceeded')
    # Monotonicity alone does not establish causality; explicitly detect cycles.
    adjacency, indegree = defaultdict(list), Counter()
    for edge in edges:
        adjacency[edge['source']].append(edge['target'])
        indegree[edge['target']] += 1
    nodes = set(ids)
    frontier = [key for key in nodes if not indegree[key]]
    visited = 0
    while frontier:
        u = frontier.pop()
        visited += 1
        for v in adjacency[u]:
            indegree[v] -= 1
            if not indegree[v]:
                frontier.append(v)
    if visited != len(nodes):
        problems['cycle'] += 1
    return {'schemaVersion': 1, 'complete': bool(producer_complete and observations and not problems),
            'problems': dict(problems), 'edge_unresolved': sorted(set(unresolved)),
            'nodes': [{'event_id': row['event_id'], 'type': row['type'], 'timestamp_ns': row['timestamp_ns'],
                       'thread': [row['pid'], row['process_epoch'], row['tid']], 'action_id': row.get('action_id'),
                       **{key: row.get(key) for key in ('trace_id', 'span_id', 'parent_span_id',
                           'call_id', 'finding_id', 'function_id', 'module_id', 'entity_id', 'job_id',
                           'command_id', 'scenario_id', 'parent_event_id', 'call_link_status',
                           'state_before', 'state_after', 'entity_change_kind')}}
                      for row in observations],
            'edges': edges, 'provenance': 'engine_probe', 'confidence': 'derived',
            'meaning': 'explicit producer-consumer correlation, NOT same-thread call tree'}


def action_windows(events: list[dict], *, producer_complete: bool, pre_ns: int = 0, post_ns: int = 0) -> dict:
    if type(pre_ns) is not int or type(post_ns) is not int or not 0 <= pre_ns <= 60_000_000_000 or not 0 <= post_ns <= 60_000_000_000:
        raise EvidenceError('rolling capture window exceeds bound')
    if len(events) > MAX_EVENTS:
        raise EvidenceError('action event budget exceeded')
    starts, actions, scenarios, triggers, problems = {}, [], {}, [], Counter()
    # Validate all input before aggregating. A malformed, duplicated, or
    # reversed event stream cannot yield a clean Action result on its own.
    validated, ids, previous = [], set(), {}
    for row in events:
        item = validate_event(row)
        if item['event_id'] in ids:
            raise EvidenceError('duplicate causal EventId in Action capture')
        ids.add(item['event_id'])
        thread = (item['pid'], item['process_epoch'], item['tid'])
        if item['timestamp_ns'] < previous.get(thread, 0):
            raise EvidenceError('Action per-thread timestamp reversed')
        previous[thread] = item['timestamp_ns']
        validated.append(item)
    times = defaultdict(list)
    for item in validated:
        times[(item['pid'], item['process_epoch'])].append(item['timestamp_ns'])
    for stamps in times.values():
        stamps.sort()
    for event in validated:
        kind = event['type']
        if kind in {'action.start', 'scenario.start'}:
            key = (kind.split('.')[0], event['pid'], event['process_epoch'], event['action_id'] if kind.startswith('action.') else event['scenario_id'])
            if key in starts:
                problems['duplicate_start'] += 1
            starts[key] = event
        elif kind in {'action.end', 'scenario.end'}:
            key = (kind.split('.')[0], event['pid'], event['process_epoch'], event['action_id'] if kind.startswith('action.') else event['scenario_id'])
            begin = starts.pop(key, None)
            if not begin or event['timestamp_ns'] < begin['timestamp_ns']:
                problems['unpaired_end'] += 1
                continue
            window = {'id': key[3], 'kind': key[0], 'start_ns': begin['timestamp_ns'],
                      'end_ns': event['timestamp_ns'], 'name': begin.get('name', key[3]),
                      'pid': event['pid'], 'process_epoch': event['process_epoch'], 'status': 'observed_probe_interval'}
            (actions if key[0] == 'action' else scenarios.setdefault(key[3], [])).append(window)
        elif kind == 'capture.trigger':
            start, end = max(0, event['timestamp_ns'] - pre_ns), event['timestamp_ns'] + post_ns
            if end > MAX_NS:
                raise EvidenceError('rolling capture window clock overflow')
            if len(triggers) >= 4096:
                raise EvidenceError('rolling trigger budget exceeded')
            stamps = times[(event['pid'], event['process_epoch'])]
            triggers.append({'trigger_id': event['trigger_id'], 'window_start_ns': start, 'window_end_ns': end,
                             'available_pre_events': bisect_left(stamps, event['timestamp_ns']) - bisect_left(stamps, start),
                             'available_post_events': bisect_right(stamps, end) - bisect_right(stamps, event['timestamp_ns']),
                             'coverage': 'unverified_retained_input_only'})
    problems['open_intervals'] += len(starts)
    return {'schemaVersion': 1, 'complete': bool(producer_complete and validated and not any(problems.values())),
            'problems': dict(problems), 'actions': actions, 'scenarios': scenarios,
            'triggers': triggers, 'provenance': 'engine_probe', 'confidence': 'derived'}


def query_event_log(run, *, offset=0, limit=50, action_id=None) -> dict:
    """Page the sealed event log and available Actions without launching a capture."""
    from pathlib import Path
    from ep_auditor.core.evidence import RawEvidence
    from ep_auditor.core.lineage import EvidenceLineage
    from ep_auditor.collectors.telemetry_pipeline import publish_family
    if type(offset) is not int or offset < 0 or type(limit) is not int or not 1 <= limit <= 200:
        raise EvidenceError('invalid event-log page bounds')
    if action_id is not None and (not isinstance(action_id, str) or not IDENTIFIER.fullmatch(action_id)):
        raise EvidenceError('invalid event-log ActionId')
    run = Path(run)
    manifest = RawEvidence(run).verify()
    publish_family(run, 'causal', verify_only=True)
    lineage = EvidenceLineage(run)
    actions = lineage.verify('metrics', 'causal/actions.json')['records'][0]
    rows = lineage.verify('normalized', 'causal/events.json')['records']
    availability = {'causal': 'observed_engine_probe_stream', 'diagnostics': 'UNKNOWN'}
    diagnostic_complete = True
    if (run / 'normalized/diagnostics/events.json').is_file():
        publish_family(run, 'diagnostics', verify_only=True)
        rows = rows + lineage.verify('normalized', 'diagnostics/events.json')['records']
        diagnostic_complete = lineage.verify('metrics', 'diagnostics/report.json')['records'][0].get('complete') is True
        availability['diagnostics'] = 'observed_engine_probe_stream'
    if action_id is not None:
        if not any(row['id'] == action_id and row.get('run_id') == manifest['run_id'] for row in actions.get('actions', [])):
            raise EvidenceError('event-log Action absent or belongs to another RunId')
        rows = [row for row in rows if row.get('action_id') == action_id]
    rows.sort(key=lambda row: (row.get('timestamp_ns', 0), row.get('event_id', '')))
    return {'run_id': manifest['run_id'], 'actions': actions.get('actions', []),
            'complete': manifest['capture_complete'] is True and actions.get('complete') is True and diagnostic_complete,
            'availability': availability,
            'total': len(rows), 'offset': offset, 'events': rows[offset:offset + limit]}


def query_action(run, action_id: str) -> dict:
    """Read one sealed Action. Time filtering measures intervals, never creates DAG edges."""
    from pathlib import Path
    from ep_auditor.core.evidence import RawEvidence
    from ep_auditor.core.lineage import EvidenceLineage
    from ep_auditor.collectors.telemetry_pipeline import publish_family
    run = Path(run)
    if not isinstance(action_id, str) or not IDENTIFIER.fullmatch(action_id):
        raise EvidenceError('invalid ActionId')
    raw = RawEvidence(run)
    manifest = raw.verify()
    publish_family(run, 'causal', verify_only=True)
    lineage = EvidenceLineage(run)
    report = lineage.verify('metrics', 'causal/actions.json')['records'][0]
    matches = [row for row in report['actions'] if row['id'] == action_id]
    if len(matches) != 1 or matches[0].get('run_id') != manifest['run_id']:
        raise EvidenceError('Action absent, ambiguous or belongs to another RunId')
    action = matches[0]
    graph = lineage.verify('graphs', 'causal/dag.json')['records'][0]
    nodes = [row for row in graph['nodes'] if row.get('action_id') == action_id]
    node_ids = {row['event_id'] for row in nodes}
    edges = [row for row in graph['edges'] if row['source'] in node_ids and row['target'] in node_ids]
    external_edges = [row for row in graph['edges'] if row['target'] in node_ids and row['source'] not in node_ids]
    external_ids = {row['source'] for row in external_edges}
    threads = {tuple(row['thread']) for row in nodes}
    calls, seen, total = [], set(), 0
    files = sorted((run / 'graphs/exact').glob('*-calls-*.json'))
    if len(files) > 1024:
        raise EvidenceError('Action call-tree file capacity exceeded')
    for path in files:
        rows = lineage.verify('graphs', 'exact/' + path.name)['records']
        total += len(rows)
        if total > 1048576:
            raise EvidenceError('Action call-tree event capacity exceeded')
        for row in rows:
            if (tuple(row['thread'][:3]) in threads and action['start_ns'] <= row['start_ns']
                    and row['end_ns'] <= action['end_ns']):
                if row['callId'] in seen:
                    raise EvidenceError('duplicate Action call-tree CallId')
                seen.add(row['callId'])
                calls.append(row)
    functions = {}
    for call in calls:
        metric = functions.setdefault(call['functionId'], {'function_id': call['functionId'], 'calls': 0,
            'inclusive_ns': 0, 'self_ns': 0, 'cpu_inclusive_100ns': 0, 'cpu_self_100ns': 0})
        metric['calls'] += 1
        for key in ('inclusive_ns', 'self_ns', 'cpu_inclusive_100ns', 'cpu_self_100ns'):
            metric[key] += call[key]
    diagnostic_path = run / 'normalized/diagnostics/events.json'
    diagnostics = []
    findings = []
    diagnostic_complete = True
    if diagnostic_path.is_file():
        publish_family(run, 'diagnostics', verify_only=True)
        diagnostics = [row for row in lineage.verify('normalized', 'diagnostics/events.json')['records']
                       if row.get('action_id') == action_id]
        diagnostic_report = lineage.verify('metrics', 'diagnostics/report.json')['records'][0]
        diagnostic_complete = diagnostic_report.get('complete') is True
        findings = [row for row in diagnostic_report.get('findings', [])
                    if row.get('action_id') == action_id]
    exceptions = [row for row in diagnostics
                  if row['type'].startswith('exception.') or row['type'] in {'crash', 'assert', 'contract'}]
    native_exceptions_available = any(row['path'] == 'raw/modules/api_observer_binding.json'
                                     for row in manifest['raw_files'])
    if native_exceptions_available:
        import json
        from ep_auditor.collectors.modules import publish_api_snapshot
        # Replay verifies the raw/helper/target identity, exit and original graph.
        publish_api_snapshot(run, verify_only=True)
        native_report = lineage.verify('metrics', 'diagnostics/native_exceptions.json')['records'][0]
        snapshot = json.loads(raw.read_bytes('process/stdout.bin'))
        diagnostic_complete = diagnostic_complete and native_report['complete'] is True
        reference = lineage.reference('raw', 'process/stdout.bin')
        for index, row in enumerate(snapshot['exceptions']):
            thread = (snapshot['pid'], snapshot['process_epoch'], row['tid'])
            timestamp = row['qpc'] * 1_000_000_000 // snapshot['qpc_frequency']
            if thread not in threads or not action['start_ns'] <= timestamp <= action['end_ns']:
                continue
            # Interval membership is derived; it never creates a causal edge or
            # asserts handler recovery merely because a first chance was seen.
            fact = native_report['events'][index]
            exceptions.append(dict(row, type='exception.first_chance' if row['first_chance'] else 'exception.second_chance',
                pid=thread[0], process_epoch=thread[1], timestamp_ns=timestamp,
                action_id=action_id, action_membership_confidence='derived',
                action_membership_scope='observed QPC interval on an explicitly linked thread; no causal inference',
                handling={'classification': fact['classification'], 'confidence': 'derived',
                          'continuation_observed': fact['continuation_observed']},
                provenance='external', confidence='observed', rawReference=reference,
                raw_json_pointer='/exceptions/' + str(index)))
    parents = {row['event_id']: row for row in graph['nodes']}
    diagnostic_events = {row.get('event_id'): row for row in diagnostics if row.get('event_id')}
    finding_links, unresolved_finding_links = [], []
    for finding in findings:
        parent_id = finding.get('parent_event_id')
        if parent_id is None:
            continue
        parent = parents.get(parent_id)
        source = diagnostic_events.get(finding.get('event_id'))
        if (parent is None or source is None
                or tuple(parent['thread'][:2]) != (source['pid'], source['process_epoch'])
                or parent['timestamp_ns'] > source['timestamp_ns']
                or (parent.get('trace_id') and source.get('trace_id') and parent['trace_id'] != source['trace_id'])):
            unresolved_finding_links.append({'finding_id': finding['finding_id'], 'cause_event_id': parent_id,
                                             'status': 'edge_unresolved'})
            diagnostic_complete = False
            continue
        finding_links.append({'finding_id': finding['finding_id'], 'cause_event_id': parent_id})
        if parent_id not in node_ids:
            external_ids.add(parent_id)
    return {'schemaVersion': 1, 'action': action,
            'complete': report['complete'] is True and manifest['capture_complete'] is True and graph['complete'] is True and diagnostic_complete,
            'causal_graph': {'nodes': nodes, 'edges': edges + external_edges, 'complete': graph['complete'],
                            'external_parents': [row for row in graph['nodes'] if row['event_id'] in external_ids]},
            'threads': sorted(threads), 'state_changes': [row for row in nodes if row['type'] in {'state.change','state.changed'}],
            'call_tree': calls, 'function_profile': list(functions.values()),
            'call_tree_scope': 'fully-contained intervals on explicitly linked threads; no inferred causality',
            'call_tree_availability': 'observed' if files else 'UNKNOWN',
            'diagnostic_availability': 'observed_engine_probe_stream' if diagnostic_path.is_file() else 'UNKNOWN',
            'absence_semantics': 'Empty categories do not establish zero activity or memory safety; unobserved activity is UNKNOWN',
            'exceptions': exceptions,
            'native_exception_availability': 'observed_external_debugger' if native_exceptions_available else 'UNKNOWN',
            'allocations': [row for row in diagnostics if row['type'] in {'allocation','deallocation','memory.error'}],
            'io': [row for row in diagnostics if row['type'].startswith(('io.','network.'))],
            'findings': findings,
            'finding_links': finding_links, 'unresolved_finding_links': unresolved_finding_links,
            'provenance': 'engine_probe', 'confidence': 'derived'}
