"""MS6.5/6.6 normalized evidence and derived reports from sealed raw streams."""
from __future__ import annotations

import json
import hashlib
import heapq
from pathlib import Path

from ep_auditor.analyzers.diagnostics import analyze as diagnostics_analyze, validate_event as diagnostics_validate
from ep_auditor.analyzers.causality import causal_graph, action_windows, validate_event as causal_validate
from ep_auditor.core.evidence import EvidenceError, RawEvidence
from ep_auditor.core.lineage import EvidenceLineage, normalized_record


def _events(payload: bytes, family: str):
    validator = diagnostics_validate if family == 'diagnostics' else causal_validate
    if not payload or len(payload) > 16 * 1024 * 1024 or not payload.endswith(b'\n'):
        raise EvidenceError('telemetry stream empty, truncated or over capacity')
    if len(payload.splitlines()) > 200_000:
        raise EvidenceError('telemetry event budget exceeded')
    events, clocks = [], {}
    for line in payload.splitlines():
        if not line or len(line) > 65536:
            raise EvidenceError('invalid telemetry record size')
        try:
            def unique(pairs):
                value = {}
                for key, item in pairs:
                    if key in value:
                        raise ValueError('duplicate telemetry JSON key')
                    value[key] = item
                return value
            def reject_nonfinite(value):
                raise ValueError('nonfinite telemetry JSON number')
            event = json.loads(line, object_pairs_hook=unique, parse_constant=reject_nonfinite)
        except (ValueError, UnicodeDecodeError) as exc:
            raise EvidenceError('invalid telemetry JSONL record') from exc
        validated = validator(event)
        thread = (validated['pid'], validated['process_epoch'], validated['tid'])
        if validated['timestamp_ns'] < clocks.get(thread, 0):
            raise EvidenceError('producer per-thread clock reversed')
        clocks[thread] = validated['timestamp_ns']
        events.append(validated)
    return events


def preflight_stream(data: bytes, family: str, expected_count: int | None) -> bool:
    """Truthful producer count check before the raw manifest is sealed."""
    try:
        events = _events(data, family)
        return expected_count is not None and expected_count == len(events)
    except (EvidenceError, ValueError, KeyError, TypeError):
        return False


def _resolve_exact_calls(run: Path, raw: RawEvidence, events: list[dict]):
    """Resolve explicit compiler entry ordinals, never temporal proximity."""
    from ep_auditor.collectors.exact_trace import read_header, iter_events
    from ep_auditor.analyzers.exact_calls import reconstruct_calls
    from ep_auditor.core.identity import module_identity
    wanted = [row for row in events if 'exact_call_ordinal' in row]
    if not wanted:
        return {}, []
    additions = {row['event_id']: {'call_link_status': 'unresolved_exact_locator'} for row in wanted}
    lineage, inputs, groups = EvidenceLineage(run), [], {}
    scopes = {(row['pid'], row['process_epoch']) for row in wanted}
    for source in sorted((run / 'raw/exact').glob('*.bin')):
        header = read_header(source)
        scope = (header['pid'], header['birth'])
        if scope in scopes:
            groups.setdefault(scope, []).append((source, header))
    for scope, producers in groups.items():
        if len(producers) > 128 or sum(header['written'] for _, header in producers) > 1048576:
            raise EvidenceError('exact causal binding capacity exceeded')
        if (len({header['image_base'] for _, header in producers}) != len(producers)
                or len({header['frequency'] for _, header in producers}) != 1):
            continue
        health_path = 'exact/process-' + str(scope[0]) + '-' + str(scope[1]) + '-health.json'
        if (not (run / 'raw' / health_path).is_file()
                or json.loads(raw.read_bytes(health_path)).get('complete') is not True):
            continue
        symbolizations, bindings = {}, {}
        for source, header in producers:
            symbol_path = 'exact/' + source.stem + '-symbols.json'
            symbols = json.loads(raw.read_bytes(symbol_path))['symbolization']
            symbolizations[header['image_base']] = {int(address): symbol for address, symbol in symbols['symbols'].items()}
            trace_ref = lineage.reference('raw', 'exact/' + source.name)
            inputs.extend((trace_ref, lineage.reference('raw', symbol_path)))
            bindings[header['image_base']] = (header, trace_ref, module_identity(symbols['binary_sha256']))
        inputs.append(lineage.reference('raw', health_path))
        report = reconstruct_calls(heapq.merge(*(iter_events(source, header) for source, header in producers),
                                   key=lambda item: item['timestamp_ns']), symbolizations,
                                   producer_complete=all(header['complete'] and not header['dropped'] for _, header in producers))
        if not report['complete']:
            continue
        calls = {row['callId']: row for row in report['calls']}
        for event in wanted:
            if (event['pid'], event['process_epoch']) != scope:
                continue
            binding = bindings.get(event['exact_module_base'])
            if binding is None:
                continue
            header, reference, module = binding
            ordinal = event['exact_call_ordinal']
            call_id = 'CallId:' + hashlib.sha256((header['trace_sha256'] + ':' + str(ordinal)).encode()).hexdigest()
            call = calls.get(call_id)
            if (call is None or call['thread'][:3] != (event['pid'], event['process_epoch'], event['tid'])
                    or not call['start_ns'] <= event['timestamp_ns'] <= call['end_ns']):
                continue
            additions[event['event_id']] = {'call_id': call_id, 'function_id': call['functionId'],
                'module_id': module, 'call_link_status': 'explicit_compiler_entry_verified',
                'exact_call_reference': {'raw': reference, 'enter_ordinal': ordinal}}
    return additions, inputs


def publish_family(run: Path, family: str, *, verify_only: bool = False) -> None:
    if family not in {'diagnostics', 'causal'}:
        raise EvidenceError('unknown telemetry publication family')
    raw = RawEvidence(run)
    manifest = raw.verify()
    files = [row for row in manifest['raw_files']
             if row['path'].startswith('raw/' + family + '/stream-') and row['path'].endswith('.jsonl')]
    if not files:
        return
    lineage, inputs, rows, events, complete = EvidenceLineage(run), [], [], [], True
    for item in sorted(files, key=lambda row: row['path']):
        name = item['path'].removeprefix('raw/')
        ref = lineage.reference('raw', name)
        inputs.append(ref)
        complete = complete and item['complete']
        try:
            parsed = _events(raw.read_bytes(name), family)
        except (EvidenceError, ValueError, KeyError, TypeError):
            # Original bytes survive; their bogus content does not become derived truth.
            complete = False
            parsed = []
        for ordinal, event in enumerate(parsed):
            events.append(dict(event, source_ref=ref, source_ordinal=ordinal))
            rows.append(normalized_record({**event, 'schemaVersion': 1, 'eventType': family + '.' + event['type'],
                                           'required': True, 'provenance': 'engine_probe', 'confidence': 'declared'},
                                          {family + '.' + event['type']}, ref, ordinal))
    if any('exact_call_ordinal' in row for row in events):
        try:
            additions, exact_inputs = _resolve_exact_calls(run, raw, events)
        except (EvidenceError, OSError, ValueError, KeyError, TypeError):
            additions = {row['event_id']: {'call_link_status': 'unavailable_exact_capture'}
                         for row in events if 'exact_call_ordinal' in row}
            exact_inputs = []
        inputs.extend(exact_inputs)
        for event in events:
            event.update(additions.get(event.get('event_id'), {}))
        rows = [normalized_record({**row, **additions.get(row.get('event_id'), {})},
                    {row['eventType']}, row['rawReference'], row['rawOrdinal']) for row in rows]
    # Independent producers write their own files. Their filenames reflect neither
    # execution order nor thread order. Compare monotonic timestamps only after
    # deterministic reassembly, while keeping every normalized row bound to its
    # original physical source and ordinal.
    events.sort(key=lambda e: (e['pid'], e['process_epoch'], e['timestamp_ns'], e['tid'], e.get('event_id', '')))
    if not rows:
        # A stream with no valid facts is still a data-quality observation.
        rows.append(normalized_record({'schemaVersion': 1, 'eventType': family + '.capture_unreadable',
                                       'required': True, 'reason': 'no_valid_observations',
                                       'provenance': 'engine_probe', 'confidence': 'declared'},
                                      {family + '.capture_unreadable'}, inputs[0], 0))
    normal_path = family + '/events.json'
    if not (run / 'normalized' / normal_path).is_file():
        if verify_only:
            raise EvidenceError('required normalized telemetry missing')
        lineage.write('normalized', normal_path, producer_version='ms65-66-1',
                      producer_source=Path(__file__), inputs=inputs, records=rows)
    else:
        if lineage.verify('normalized', normal_path)['records'] != rows:
            raise EvidenceError('normalized telemetry differs from sealed producer bytes')
    otherwise_complete = complete and bool(events)
    try:
        if family == 'diagnostics':
            report = diagnostics_analyze(events, complete=otherwise_complete)
            destination = 'diagnostics/report.json'
            layer = 'metrics'
        else:
            report = causal_graph(events, producer_complete=otherwise_complete)
            destination = 'causal/dag.json'
            layer = 'graphs'
    except (EvidenceError, ValueError, TypeError, KeyError) as exc:
        report = {'schemaVersion': 1, 'complete': False, 'error': str(exc),
                  'provenance': 'engine_probe', 'confidence': 'derived'}
        destination, layer = ('diagnostics/report.json', 'metrics') if family == 'diagnostics' else ('causal/dag.json', 'graphs')
    ref = lineage.reference('normalized', normal_path)
    if family == 'diagnostics' and 'findings' in report:
        for finding in report['findings']:
            identity = json.dumps([ref, finding['source_ref'], finding['source_ordinal'], finding['kind']],
                                  sort_keys=True, separators=(',', ':')).encode()
            finding['finding_id'] = 'FindingId:' + hashlib.sha256(identity).hexdigest()
    record = {'schemaVersion': 1, 'eventType': family + '.analysis', 'required': True,
              'provenance': 'engine_probe', 'confidence': 'derived', **report}
    if not (run / layer / destination).is_file():
        if verify_only:
            raise EvidenceError('required telemetry analysis missing')
        lineage.write(layer, destination, producer_version='ms65-66-1',
                      producer_source=Path(__file__), inputs=[ref], records=[record])
    else:
        if lineage.verify(layer, destination)['records'] != [record]:
            raise EvidenceError('telemetry analysis differs from sealed producer bytes')
    if family == 'causal':
        try:
            actions = action_windows(events, producer_complete=otherwise_complete)
            for item in actions['actions']:
                item['run_id'] = manifest['run_id']
        except (EvidenceError, ValueError, TypeError, KeyError) as exc:
            actions = {'schemaVersion': 1, 'complete': False, 'error': str(exc),
                       'provenance': 'engine_probe', 'confidence': 'derived'}
        # Completed Action boundaries are not a clean causal observation if the
        # same producer stream contains missing/ambiguous edges or corrupt raw.
        if not report.get('complete', False):
            actions['complete'] = False
            actions['causal_graph_incomplete'] = True
        if not (run / 'metrics/causal/actions.json').is_file():
            if verify_only:
                raise EvidenceError('required action analysis missing')
            lineage.write('metrics', 'causal/actions.json', producer_version='ms65-66-1',
                          producer_source=Path(__file__), inputs=[ref], records=[{
                              'schemaVersion': 1, 'eventType': 'causal.actions', 'required': True,
                              **actions}])
        else:
            expected = [{'schemaVersion': 1, 'eventType': 'causal.actions', 'required': True, **actions}]
            if lineage.verify('metrics', 'causal/actions.json')['records'] != expected:
                raise EvidenceError('action analysis differs from sealed producer bytes')


def publish_telemetry(run: Path) -> None:
    for family in ('diagnostics', 'causal'):
        publish_family(run, family)


def telemetry_complete(run: Path) -> bool:
    """Only present streams are mandatory. Never silently PASS incomplete derived evidence."""
    raw = RawEvidence(run)
    manifest = raw.verify()
    lineage = EvidenceLineage(run)
    if any(row['path'] == 'raw/diagnostics/verifier.xml' for row in manifest['raw_files']):
        try:
            from ep_auditor.collectors.sanitizer import publish_verifier
            publish_verifier(run, verify_only=True)
        except (EvidenceError, OSError, ValueError, KeyError, TypeError):
            return False
    if any(row['path'] == 'raw/modules/api_observer_binding.json' for row in manifest['raw_files']):
        try:
            from ep_auditor.collectors.modules import publish_api_snapshot
            # This existing producer verifies replay against immutable raw as
            # well as lineage; edited graph content must not certify itself.
            report = publish_api_snapshot(run, verify_only=True)
            if report.get('complete') is not True:
                return False
        except (EvidenceError, OSError, ValueError, KeyError, TypeError):
            return False
    for family in ('diagnostics', 'causal'):
        present = any(row['path'].startswith('raw/' + family + '/stream-') and
                      row['path'].endswith('.jsonl') for row in manifest['raw_files'])
        if not present:
            continue
        try:
            publish_family(run, family, verify_only=True)
        except (EvidenceError, OSError, KeyError, ValueError, TypeError):
            return False
        surfaces = [('normalized', family + '/events.json'),
                    (('metrics' if family == 'diagnostics' else 'graphs'),
                     ('diagnostics/report.json' if family == 'diagnostics' else 'causal/dag.json'))]
        if family == 'causal':
            surfaces.append(('metrics', 'causal/actions.json'))
        for layer, name in surfaces:
            try:
                data = lineage.verify(layer, name)
                if data.get('completeness', {}).get('complete') is not True:
                    return False
                if layer != 'normalized' and not all(record.get('complete') is True
                                                      for record in data.get('records', [])):
                    return False
            except (EvidenceError, OSError, KeyError, ValueError, TypeError):
                return False
    return True
