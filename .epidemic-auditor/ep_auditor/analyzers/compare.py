from __future__ import annotations

from typing import Any


def compare_summaries(baseline: dict[str, Any], current: dict[str, Any]) -> dict[str, Any]:
    keys = ["wall_seconds", "peak_rss_bytes", "peak_threads", "total_read_bytes", "total_write_bytes"]
    metrics: dict[str, Any] = {}
    for key in keys:
        import math
        values = (baseline.get(key), current.get(key))
        known = all(type(value) in (int, float) and math.isfinite(value) and value >= 0 for value in values)
        a, b = values
        metrics[key] = {"baseline": a, "current": b, "delta": b - a if known else None,
                        "delta_percent": ((b - a) / a) * 100.0 if known and a else None,
                        "availability": "observed" if known else "UNKNOWN"}
    return {"metrics": metrics}


def compare_runs(baseline, current, *, cursor=None, limit=200):
    """Compare verified compatible observations, preserving both build identities."""
    from pathlib import Path
    from ep_auditor.core.evidence import RawEvidence, load_json, EvidenceError
    from ep_auditor.core.lifecycle import authoritative_run
    from ep_auditor.analyzers.summary import query_run_view, _verified_rows
    if type(limit) is not int or not 1 <= limit <= 200:
        raise EvidenceError('comparison page limit must be 1–200')
    baseline, current = Path(baseline), Path(current)
    headers = [authoritative_run(run) for run in (baseline, current)]
    manifests = [RawEvidence(run).verify() for run in (baseline, current)]
    environments = [load_json(run / 'environment.json') for run in (baseline, current)]
    fields = ('schemaVersion', 'instrumentation_profile', 'machine', 'hostname', 'platform',
              'processor', 'logical_cpu_count', 'build_config', 'compiler_toolchain',
              'capture_configuration', 'metric_units')
    incompatible = [key for key in fields if environments[0].get(key) != environments[1].get(key)]
    missing = [key for key in fields if key not in environments[0] or key not in environments[1]
               or (key != 'capture_configuration' and (not environments[0].get(key) or not environments[1].get(key)))]
    incompatible += [key for key in missing if key not in incompatible]
    if any(env.get('compiler_toolchain', {}).get('availability') != 'observed' for env in environments):
        incompatible.append('compiler_toolchain_unobserved')
    result = {'schemaVersion': 1, 'baseline_run_id': headers[0]['run_id'], 'current_run_id': headers[1]['run_id'],
              'compatibility': 'INCOMPATIBLE' if incompatible else 'compatible',
              'incompatibilities': incompatible,
              'source_identities': [m['source_fingerprints'] for m in manifests],
              'binary_identities': [m['binary_fingerprints'] for m in manifests],
              'complete': all(m['capture_complete'] for m in manifests) and all(h['status'] != 'INCOMPLETE' for h in headers)}
    if incompatible:
        result.update(regression_verdict='UNKNOWN', metrics={}, differences={})
        if cursor is not None:
            from ep_auditor.core.evidence import EvidenceError
            raise EvidenceError('incompatible comparison cannot resume a page')
        return result
    summaries = [query_run_view(run, 'Dashboard')['items'][0]['process'] for run in (baseline, current)]
    metrics = compare_summaries(*[{'peak_rss_bytes': s.get('rss_peak_bytes'),
                                  'peak_threads': s.get('threads_peak')} for s in summaries])['metrics']
    metrics['cpu_avg'] = _delta(summaries[0].get('cpu_avg'), summaries[1].get('cpu_avg'))
    functions, edges, categories, allocations = [], [], [], []
    ambiguous = False
    for run in (baseline, current):
        rows = list(_verified_rows(run, ('metrics/exact/*-functions.json',)))
        identities, mapping, names = {}, {}, {}
        for row, reference, ordinal, complete in rows:
            result['complete'] = result['complete'] and complete and row.get('complete', True) is True
            if not row.get('functionId'):
                continue
            symbol = row.get('symbol', {})
            source = symbol.get('source_file')
            try:
                source = Path(source).resolve().relative_to(Path(environments[len(functions)]['project_root']).resolve()).as_posix() if source else None
            except ValueError:
                source = None
            key = (source, symbol.get('function')) if source and symbol.get('function') else (None, row['functionId'])
            names.setdefault(key, set()).add(row['functionId'])
            mapping[row['functionId']] = key
            if key in identities:
                # Several producers can contribute the same measured symbol.
                # Aggregate counts but do not invent a percentile of percentiles.
                for metric in ('calls', 'inclusive_ns', 'self_ns', 'cpu_inclusive_100ns', 'cpu_self_100ns'):
                    values = (identities[key].get(metric), row.get(metric))
                    identities[key][metric] = sum(values) if all(type(value) in (int, float) for value in values) else None
                identities[key]['p95_ns'] = identities[key]['p99_ns'] = None
            else:
                identities[key] = dict(row, evidence_ref=reference, evidence_ordinal=ordinal)
        ambiguous = ambiguous or any(len(values) > 1 for values in names.values())
        functions.append(identities)
        edges.append({(mapping.get(edge['caller']), mapping.get(edge['callee']))
                      for report, _, _, _ in rows for edge in report.get('edges', [])
                      if edge['caller'] in mapping and edge['callee'] in mapping})
        categories.append({name: query_run_view(run, name, limit=20) for name in
                           ('Exceptions', 'Locks & Waits', 'Coverage')})
        allocation = [row.get('allocation_metrics', {}) for row, _, _, _ in
                      _verified_rows(run, ('metrics/diagnostics/report.json',))]
        allocations.append(allocation[0] if len(allocation) == 1 else {})
    changed = []
    for identity in sorted(functions[0].keys() & functions[1].keys(), key=str):
        changes = {key: _delta(functions[0][identity].get(key), functions[1][identity].get(key))
                   for key in ('calls', 'inclusive_ns', 'self_ns', 'p95_ns', 'p99_ns',
                               'cpu_inclusive_100ns', 'cpu_self_100ns')}
        if any(row['delta'] for row in changes.values()):
            changed.append({'symbol_identity': identity,
                            'function_ids': [functions[0][identity]['functionId'], functions[1][identity]['functionId']],
                            'evidence_refs': [functions[0][identity]['evidence_ref'], functions[1][identity]['evidence_ref']],
                            'metrics': changes})
    result['symbol_mapping'] = 'ambiguous' if ambiguous else 'source_symbol_or_binary_bound'
    metrics['allocations'] = {key: _delta(allocations[0].get(key), allocations[1].get(key))
                              for key in ('count', 'rate_per_second')}
    metrics['allocation_peaks'] = {'baseline': allocations[0].get('peak_observed_bytes_by_process'),
        'current': allocations[1].get('peak_observed_bytes_by_process'),
        'scope': 'Observed allocator/process peaks; not a simultaneous whole-process heap peak'}
    observations = [_category_metrics(run) for run in (baseline, current)]
    metrics['category_observations'] = {key: _delta(observations[0].get(key), observations[1].get(key))
        for key in sorted(observations[0].keys() | observations[1].keys())}
    candidates = [row for row in changed if result['complete'] and not ambiguous and any(
        metric['delta'] is not None and metric['delta'] >= 50000 and metric['delta_percent'] is not None
        and metric['delta_percent'] >= 20 for name, metric in row['metrics'].items()
        if name in ('inclusive_ns', 'self_ns', 'p95_ns', 'p99_ns'))]
    import hashlib
    import json
    findings = [{'finding_id': 'FindingId:' + hashlib.sha256(json.dumps(row['symbol_identity']).encode()).hexdigest(),
                 'run_id': headers[1]['run_id'], 'category': 'regression', 'severity': 'warning',
                 'rule_id': 'performance.measured_latency_increase', 'confidence': 'heuristic',
                 'provenance': 'compiler_instrumentation', 'summary': 'Measured latency increased in a compatible capture',
                 'baseline_state': 'new_candidate', 'baseline_run_id': headers[0]['run_id'],
                 'evidence_refs': row['evidence_refs'], 'function_ids': row['function_ids'],
                 'metrics': row['metrics'], 'reproduction': None, 'source_refs': row['symbol_identity']}
                for row in candidates]
    for finding in findings:
        finding.update(issue_id=('IssueId:' + finding['finding_id'].split(':', 1)[1]) if finding['source_refs'][0] else None,
                       occurrence_id='FindingOccurrenceId:' + hashlib.sha256(json.dumps(
                           [headers[1]['run_id'], finding['finding_id']]).encode()).hexdigest(),
                       first_seen=headers[1]['started_utc'], last_seen=headers[1].get('finished_utc'))
    result.update(metrics=metrics, differences={
        'new_functions': sorted(functions[1].keys() - functions[0].keys(), key=str),
        'removed_functions': sorted(functions[0].keys() - functions[1].keys(), key=str),
        'new_edges': sorted(edges[1] - edges[0], key=str), 'removed_edges': sorted(edges[0] - edges[1], key=str),
        'function_timings': changed, 'findings': findings, 'categories': categories},
        regression_verdict='candidate' if candidates else 'UNKNOWN',
        function_scope='observed' if all(functions) else 'UNKNOWN',
        candidate_policy={'minimum_increase_percent': 20, 'minimum_increase_ns': 50000,
                          'confidence': 'heuristic', 'policy': 'A measured candidate is not a correctness defect or PASS'},
        absence_semantics='Unobserved or incomparable functions are not proven dead code')
    return _page_comparison(result, baseline, current, cursor, limit)


def _page_comparison(result, baseline, current, cursor, limit):
    import base64
    import hashlib
    import json
    from ep_auditor.core.evidence import digest, EvidenceError
    if type(limit) is not int or not 1 <= limit <= 200:
        raise EvidenceError('comparison page limit must be 1–200')
    identity = [result['baseline_run_id'], result['current_run_id'], digest(baseline / 'manifest.json'),
                digest(current / 'manifest.json'), limit]
    signature = hashlib.sha256(json.dumps(identity).encode()).hexdigest()
    selected, offset = None, 0
    arrays = {key: value for key, value in result['differences'].items() if isinstance(value, list)}
    if cursor is not None:
        try:
            if not isinstance(cursor, str) or len(cursor) > 4096:
                raise ValueError('oversized cursor')
            token = json.loads(base64.urlsafe_b64decode(cursor.encode()).decode())
            selected, offset = token['field'], token['offset']
            if token['selection'] != signature or selected not in arrays or type(offset) is not int or offset < 0:
                raise ValueError('selection changed')
        except (ValueError, TypeError, KeyError, UnicodeError) as error:
            raise EvidenceError('invalid comparison continuation') from error
    result['continuations'], result['totals'], result['truncated'] = {}, {}, False
    for key, values in arrays.items():
        start = offset if key == selected else 0
        result['totals'][key] = len(values)
        result['differences'][key] = values[start:start + limit]
        if start + limit < len(values):
            result['truncated'] = True
            result['continuations'][key] = base64.urlsafe_b64encode(json.dumps({
                'selection': signature, 'field': key, 'offset': start + limit}).encode()).decode()
    result['page_field'] = selected
    result['snapshots'] = identity[2:4]
    return result


def _delta(a, b):
    import math
    known = all(type(value) in (int, float) and math.isfinite(value) and value >= 0 for value in (a, b))
    return {'baseline': a, 'current': b, 'delta': b - a if known else None,
            'delta_percent': (b - a) * 100 / a if known and a else None,
            'availability': 'observed' if known else 'UNKNOWN'}


def _category_metrics(run):
    """Count verified scoped observations, never a truncated GUI page or absence."""
    from ep_auditor.analyzers.summary import VIEWS, _verified_rows, _view_items
    result = {}
    exceptions = list(_verified_rows(run, VIEWS['Exceptions']))
    if exceptions:
        result['exception_notifications'] = sum(len(_view_items('Exceptions', row)) for row, _, _, _ in exceptions)
    waits = list(_verified_rows(run, VIEWS['Locks & Waits']))
    if waits:
        values = [wait for row, _, _, _ in waits for wait in _view_items('Locks & Waits', row)]
        result['wait_intervals'] = len(values)
        durations = [value.get('end_ns') - value.get('start_ns')
                     if type(value.get('end_ns')) is int and type(value.get('start_ns')) is int else None
                     for value in values]
        result['observed_wait_ns'] = sum(durations) if all(type(value) is int and value >= 0 for value in durations) else None
    coverage = list(_verified_rows(run, VIEWS['Coverage']))
    if coverage:
        for kind in ('lines', 'functions', 'regions', 'branches'):
            for key in ('count', 'covered'):
                values = [file.get('summary', {}).get(kind, {}).get(key) for row, _, _, _ in coverage
                          for file in row.get('files', [])]
                result['coverage_' + kind + '_' + key] = (sum(values) if values and
                    all(type(value) is int and value >= 0 for value in values) else None)
    return result
