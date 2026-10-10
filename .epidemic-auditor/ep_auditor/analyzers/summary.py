from __future__ import annotations

import json
import io
from collections import defaultdict
from pathlib import Path
from typing import Any
from ep_auditor.core.evidence import RawEvidence, load_json, digest

# Read-only product views reuse the existing evidence/lifecycle contract.
VIEWS = {
    'Build': ('raw/process/lifecycle.json',),
    'Tests': ('normalized/test_result.json', 'normalized/ctest_catalog.json', 'normalized/tests/*.json'),
    'Timeline': ('normalized/causal/*.json', 'normalized/diagnostics/*.json', 'normalized/windows_events.json'),
    'Actions': ('metrics/causal/actions.json',),
    'Call Tree': ('graphs/exact/*-calls-*.json',),
    'Causal Graph': ('graphs/causal/dag.json',),
    'Functions': ('metrics/exact/*-functions.json',),
    'Threads': ('graphs/*thread*.json', 'graphs/**/*thread*.json', 'graphs/windows_scheduler_sampling.json', 'raw/process/process_samples.jsonl'),
    'Locks & Waits': ('graphs/*wait*.json', 'graphs/**/*wait*.json', 'graphs/*thread*.json', 'graphs/windows_scheduler_sampling.json'),
    'Memory': ('metrics/diagnostics/report.json', 'normalized/diagnostics/sanitizer_stderr.json',
               'normalized/diagnostics/verifier.json', 'raw/process/process_samples.jsonl'),
    'Allocations': ('normalized/diagnostics/*.json', 'metrics/diagnostics/report.json'),
    'IO': ('normalized/modules/api_entries.json', 'normalized/diagnostics/*.json', 'metrics/windows_disk_io.json'),
    'Exceptions': ('metrics/diagnostics/native_exceptions.json', 'normalized/diagnostics/*.json'),
    'Coverage': ('metrics/coverage.json',),
    'Architecture': ('graphs/exact/*-architecture.json', 'findings/exact/*-rules.json'),
    'Findings': ('agent/findings-report.json',),
    'Imported APIs': ('normalized/modules/api_entries.json', 'graphs/modules/api_entry_graph.json'),
    'Modules': ('raw/modules/binary_files.json', 'raw/exact/*-symbols.json'),
    'Static Scan': ('raw/static/*.json',),
    'Hot Paths': ('metrics/exact/*-functions.json',),
    'Stress': ('raw/stress/campaign.json',),
}


def _verified_rows(run, patterns):
    """Consume existing bounded envelopes individually; never load a whole trace."""
    from ep_auditor.core.lineage import EvidenceLineage
    from ep_auditor.core.evidence import EvidenceError, digest
    run = Path(run)
    lineage, raw = EvidenceLineage(run), RawEvidence(run)
    manifest = raw.verify()
    # Product views must replay the existing producers, not merely validate a
    # well-formed envelope whose contents could differ from its raw inputs.
    from ep_auditor.collectors.telemetry_pipeline import telemetry_complete
    if any(not pattern.startswith('raw/') for pattern in patterns) and not telemetry_complete(run):
        if any(any(run.glob(pattern)) for pattern in patterns if pattern in (
                'normalized/windows_events.json', 'graphs/windows_scheduler_sampling.json', 'metrics/windows_disk_io.json')):
            from ep_auditor.collectors.etw_import import verify_etw_capture
            verify_etw_capture(run)
        from ep_auditor.collectors.telemetry_pipeline import publish_family
        for family in ('diagnostics', 'causal'):
            if any('/' + family + '/' in pattern for pattern in patterns):
                publish_family(run, family, verify_only=True)
        if any('/diagnostics/' in pattern or '/modules/' in pattern for pattern in patterns):
            names = {entry['path'] for entry in manifest['raw_files']}
            if 'raw/modules/api_observer_binding.json' in names:
                from ep_auditor.collectors.modules import publish_api_snapshot
                publish_api_snapshot(run, verify_only=True)
            if 'raw/process/stderr.bin' in names and any('sanitizer_stderr' in pattern for pattern in patterns):
                from ep_auditor.collectors.sanitizer import publish_sanitizer
                publish_sanitizer(run)
            if 'raw/diagnostics/verifier.xml' in names:
                from ep_auditor.collectors.sanitizer import publish_verifier
                publish_verifier(run, verify_only=True)
    exact_proof = None
    if any(pattern.startswith(('graphs/exact/', 'metrics/exact/', 'findings/exact/', 'raw/exact/')) for pattern in patterns):
        from ep_auditor.collectors.exact_trace import publish_capture
        if any(any(run.glob(pattern)) for pattern in patterns if pattern.startswith(
                ('graphs/exact/', 'metrics/exact/', 'findings/exact/'))) and not any((run / 'raw/exact').glob('*.bin')):
            raise EvidenceError('exact view has no original trace')
        exact_proof = publish_capture(run, verify_only=True)
    if 'metrics/coverage.json' in patterns and (run / 'metrics/coverage.json').exists():
        from ep_auditor.collectors.coverage import publish_coverage
        if not (run / 'raw/coverage/report.json').is_file():
            raise EvidenceError('coverage view has no captured report')
        publish_coverage(run)
    for name in sorted({p for pattern in patterns for p in run.glob(pattern) if p.is_file()}):
        relative = name.relative_to(run).as_posix()
        layer, path = relative.split('/', 1)
        if layer == 'raw':
            entries = [row for row in manifest['raw_files'] if row['path'] == relative]
            if len(entries) != 1:
                raise EvidenceError('view raw artifact is not registered')
            reference = lineage.reference('raw', path)
            if path.endswith('.jsonl'):
                with name.open(encoding='utf-8') as stream:
                    for ordinal, line in enumerate(stream):
                        if line.strip():
                            yield json.loads(line), reference, ordinal, entries[0]['complete']
            else:
                value = json.loads(raw.read_bytes(path))
                if path == 'modules/binary_files.json':
                    import hashlib
                    snapshot_bytes = raw.read_bytes('process/stdout.bin')
                    lifecycle = json.loads(raw.read_bytes('process/lifecycle.json'))
                    if (hashlib.sha256(snapshot_bytes).hexdigest() != value['snapshot_raw_sha256']
                            or value['helper_sha256'] != lifecycle['image_sha256']):
                        raise EvidenceError('module snapshot/helper binding mismatch')
                    from ep_auditor.collectors.modules import identify_snapshot
                    value = identify_snapshot(json.loads(snapshot_bytes), value['fingerprints'])
                    value.update(provenance='external', confidence='observed')
                    value['capture_refs'] = [lineage.reference('raw', name) for name in
                        ('process/stdout.bin', 'process/lifecycle.json')]
                elif path.startswith('exact/') and path.endswith('-symbols.json'):
                    from ep_auditor.core.identity import module_identity
                    header, symbols = value['header'], value['symbolization']
                    trace = path[:-len('-symbols.json')] + '.bin'
                    trace_reference = lineage.reference('raw', trace)
                    if trace_reference['sha256'] != value['trace_sha256']:
                        raise EvidenceError('module symbol snapshot is not bound to its original trace')
                    value = {'modules': [{'moduleId': module_identity(symbols['binary_sha256']),
                        'path': header['image_path'], 'base': header['image_base'],
                        'sha256': symbols['binary_sha256'], 'pdb_identity': symbols['pdb_identity'],
                        'identity_scope': 'captured disk binary; loaded memory bytes not hashed'}],
                        'provenance': 'compiler_instrumentation', 'confidence': 'derived',
                        'capture_refs': [trace_reference]}
                rows = value if isinstance(value, list) else [value]
                for ordinal, row in enumerate(rows):
                    yield row, reference, ordinal, entries[0]['complete']
        else:
            expected = exact_proof.get(relative) if exact_proof is not None and '/exact/' in relative else None
            envelope = lineage.verify(layer, path, expected_sha256=expected)
            reference = lineage.reference(layer, path)
            if expected is not None and reference['sha256'] != expected:
                raise EvidenceError('view artifact changed after semantic replay')
            for ordinal, row in enumerate(envelope['records']):
                yield row, reference, ordinal, envelope['completeness']['complete']


def _view_items(view, row):
    """Retain real report fields, with no inferred causal edges or outcomes."""
    event = str(row.get('type', row.get('eventType', '')))
    if view == 'Modules':
        return [dict(module, capture_refs=row.get('capture_refs', [])) for module in row.get('modules', [])]
    if view == 'Actions':
        return row.get('actions', [])
    if view == 'Findings':
        return row.get('findings', [])
    if view == 'Call Tree':
        return [row] if row.get('callId') else []
    if view == 'Causal Graph':
        return [dict(item, graph_kind=kind) for kind in ('nodes', 'edges') for item in row.get(kind, [])]
    if view == 'Hot Paths':
        return row.get('hot_paths', [])
    if view == 'Stress':
        return row.get('outcomes', [])
    if view == 'Functions':
        return [row] if row.get('functionId') else []
    if view == 'Allocations':
        if isinstance(row.get('allocation_metrics'), dict):
            return row['allocation_metrics'].get('lifetimes', [])
        return [row] if event in ('allocation', 'deallocation', 'memory.error') else []
    if view == 'Exceptions':
        return row.get('events', []) if event == 'native.exceptions' else (
            [row] if event.startswith('exception.') or event in ('crash', 'assert', 'contract') else [])
    if view == 'IO':
        if event == 'windows.disk.requests':
            return row.get('disk_requests', [])
        return [row] if event.startswith('io.') or row.get('family') in ('file', 'io', 'network') else []
    if view == 'Locks & Waits' and isinstance(row.get('waits'), list):
        return row['waits']
    if view == 'Threads' and isinstance(row.get('threads'), list):
        return row['threads']
    if view == 'Threads' and isinstance(row.get('processes'), list):
        return [dict(item, sample_monotonic=row.get('monotonic_offset_seconds')) for item in row['processes']]
    return [row]


def _verify_stress_children(run, report):
    """Replay campaign decisions from sealed owned child observations."""
    from ep_auditor.core.evidence import EvidenceError, digest
    from ep_auditor.core.lifecycle import authoritative_run
    from ep_auditor.core.run_manager import evaluate_stress_controls
    import uuid
    parent = authoritative_run(run)
    children = report.get('children', [])
    verified = {}
    for reference in children:
        if not reference['run_id'].startswith('RunId:'):
            raise EvidenceError('invalid campaign child RunId')
        child = (run.parent / uuid.UUID(reference['run_id'][6:]).hex).resolve(strict=True)
        if child.parent != run.parent.resolve() or child != Path(reference['run_directory']).resolve():
            raise EvidenceError('campaign child escapes its capture directory')
        if (digest(child / 'manifest.json') != reference['manifest_sha256']
                or digest(child / 'run.json') != reference['run_sha256']):
            raise EvidenceError('campaign child snapshot changed')
        header = authoritative_run(child)
        if header['run_id'] != reference['run_id'] or header.get('metadata', {}).get('parent_run_id') != parent['run_id']:
            raise EvidenceError('campaign child ownership mismatch')
        raw = RawEvidence(child)
        manifest = raw.verify()
        names = {entry['path'] for entry in manifest['raw_files']}
        if not {'raw/stress/observation.json', 'raw/stress/request.json'} <= names:
            if header['status'] not in ('INCOMPLETE', 'CANCELLED'):
                raise EvidenceError('completed campaign child lacks required observation/request')
            verified[reference['run_id']] = (None, None, header)
            continue  # A sealed failed attempt is retained, never a measured control.
        observation = json.loads(raw.read_bytes('stress/observation.json'))
        request = json.loads(raw.read_bytes('stress/request.json'))
        if observation['injector_id'] != request['injector_id'] or observation['phase'] != request['phase']:
            raise EvidenceError('campaign observation/request binding mismatch')
        if observation.get('command') != request['command'] or observation.get('parameters') != request['injector']:
            raise EvidenceError('campaign observed scope/parameters differ from request')
        if request['injector']['type'] != 'allocation_failure':
            driver = json.loads(raw.read_bytes('process/lifecycle.json'))
            # The owned driver returns 124 for its independently observed
            # timeout/resource/cleanup failure, rather than the killed target's
            # exit code. Replay that decision; neither result is a PASS.
            guarded_failure = (observation.get('error') or observation.get('resource_failure')
                               or observation.get('cleanup') != 'observed_stopped')
            expected_exit = 124 if guarded_failure else observation['target_exit_code']
            if driver['exit_code'] != expected_exit or driver['cancelled']:
                raise EvidenceError('campaign driver outcome differs from observed target outcome')
        verified[reference['run_id']] = (observation, request, header)
    if report.get('status') != parent['status']:
        raise EvidenceError('campaign report disagrees with authoritative lifecycle')
    if report.get('status') == 'PASS':
        if not verified or not report.get('outcomes') or any(value[0] is None for value in verified.values()):
            raise EvidenceError('campaign PASS lacks physical observations')
        first_request = next(iter(verified.values()))[1]
        if len(report['outcomes']) != first_request['iterations'] * len(first_request['injectors']):
            raise EvidenceError('campaign PASS omitted required iterations/injectors')
        if any(outcome.get('status') != 'PASS' for outcome in report['outcomes']):
            raise EvidenceError('campaign PASS contains an unsuccessful control')
    for outcome in report.get('outcomes', []):
        if not outcome.get('controls'):
            if outcome.get('status') == 'PASS':
                raise EvidenceError('campaign PASS lacks physical controls')
            continue
        controls = []
        parameters = outcome['parameters']
        for control in outcome['controls']:
            reference = control['run_ref']
            if reference not in children:
                raise EvidenceError('campaign control has unregistered child')
            observation, request, header = verified[reference['run_id']]
            if observation is None or request is None:
                raise EvidenceError('campaign control cites an attempt without physical observation')
            reconstructed = dict(observation, run_ref=reference, capture_status=header['status'])
            if request['injector'] != parameters or control != reconstructed:
                raise EvidenceError('campaign control differs from sealed child evidence')
            controls.append(reconstructed)
        replayed = dict(iteration=outcome['iteration'], **evaluate_stress_controls(parameters, controls))
        if replayed != outcome:
            raise EvidenceError('campaign outcome differs from physical control replay')
    observed = [value for value in verified.values() if value[0] is not None]
    if not observed:
        return False
    request = observed[0][1]
    return (len(observed) == len(verified) == request['iterations'] * len(request['injectors']) * 3
            and all(value[2]['status'] != 'INCOMPLETE' for value in observed))


def query_run_view(run, view, *, scope=None, cursor=None, limit=200, frame_limit=50,
                   frame_offset=0, reference_scope=False, baseline=None):
    """Shared GUI/CLI/agent paging, bound to sealed snapshot and exact selection."""
    import base64
    import hashlib
    from ep_auditor.core.evidence import EvidenceError, digest
    from ep_auditor.core.lifecycle import authoritative_run
    run = Path(run)
    header = authoritative_run(run)
    manifest = RawEvidence(run).verify()
    if (type(limit) is not int or not 1 <= limit <= 200 or type(frame_limit) is not int
            or not 1 <= frame_limit <= 50 or type(frame_offset) is not int or frame_offset < 0):
        raise EvidenceError('view bounds must be finite: 200 events and 50 frames maximum')
    if scope is not None and (not isinstance(scope, str) or not scope or len(scope) > 512):
        raise EvidenceError('invalid stable-ID scope')
    if baseline is not None and view != 'Dashboard':
        raise EvidenceError('baseline is supported only by the Dashboard view')
    baseline_selection = None
    if baseline is not None:
        baseline = Path(baseline).resolve(strict=True)
        baseline_header = authoritative_run(baseline)
        RawEvidence(baseline).verify()
        publish_findings(baseline)
        baseline_selection = [str(baseline), baseline_header['run_id'], digest(baseline / 'manifest.json')]
    if view in ('Findings', 'Dashboard'):
        publish_findings(run)
    snapshot = digest(run / 'manifest.json')
    if type(reference_scope) is not bool:
        raise EvidenceError('invalid reference scope')
    selection = [header['run_id'], snapshot, view, scope, limit, frame_limit, frame_offset, reference_scope,
                 baseline_selection]
    signature = hashlib.sha256(json.dumps(selection, sort_keys=True).encode()).hexdigest()
    offset = 0
    if cursor is not None:
        try:
            if not isinstance(cursor, str) or len(cursor) > 4096:
                raise ValueError('cursor exceeds input capacity')
            token = json.loads(base64.urlsafe_b64decode(cursor.encode()).decode())
            if token['selection'] != signature or type(token['offset']) is not int or token['offset'] < 0:
                raise ValueError('cursor scope or snapshot mismatch')
            offset = token['offset']
        except (ValueError, TypeError, KeyError, UnicodeError) as error:
            raise EvidenceError('invalid view continuation') from error
    if view == 'Dashboard':
        rows = [_dashboard_row(run, header, manifest, baseline=baseline)]
        source_rows = [(row, None, 0, manifest['capture_complete']) for row in rows]
    elif view in VIEWS:
        source_rows = _verified_rows(run, VIEWS[view])
    else:
        raise EvidenceError('unknown Run view')
    items, seen, available, all_complete = [], 0, False, (
        manifest['capture_complete'] and header['status'] != 'INCOMPLETE')
    if view == 'Call Tree':
        for report, _, _, complete in _verified_rows(run, ('metrics/exact/*-functions.json',)):
            all_complete = all_complete and complete and report.get('complete', True) is True
    has_more = False
    for row, reference, ordinal, complete in source_rows:
        if view == 'Stress':
            all_complete = _verify_stress_children(run, row) and all_complete
        available = True
        all_complete = all_complete and complete and row.get('complete', True) is True
        for inner, value in enumerate(_view_items(view, row)):
            if not isinstance(value, dict):
                continue
            identity_key = None if reference_scope else {'Functions': 'functionId', 'Findings': 'finding_id', 'Modules': 'moduleId'}.get(view)
            matches = value.get(identity_key) == scope if identity_key else _contains_id(value, scope)
            if scope and not matches:
                continue
            if seen < offset:
                seen += 1
                continue
            if len(items) == limit:
                has_more = True
                break
            item = dict(value)
            item.setdefault('provenance', row.get('provenance', 'external' if reference and reference['layer'] == 'raw' else 'unknown'))
            item.setdefault('confidence', row.get('confidence', 'derived'))
            item['evidence_ref'] = reference
            item['evidence_ordinal'] = ordinal
            item['report_ordinal'] = inner
            item = _bound_fields(item, frame_limit, frame_offset, selection, reference, ordinal, inner)
            items.append(item)
            seen += 1
        if has_more:
            break
    next_token = None
    if has_more:
        next_token = base64.urlsafe_b64encode(json.dumps(
            {'selection': signature, 'offset': offset + len(items)}).encode()).decode()
    if snapshot != digest(run / 'manifest.json'):
        raise EvidenceError('Run snapshot changed during query')
    if baseline_selection and baseline_selection[2] != digest(baseline / 'manifest.json'):
        raise EvidenceError('Baseline snapshot changed during query')
    selected_available = available and (scope is None or bool(items))
    return {'schemaVersion': 1, 'run_id': header['run_id'], 'view': view, 'scope': scope,
            'status': header['status'], 'availability': 'observed' if selected_available else 'UNKNOWN',
            'complete': bool(selected_available and all_complete), 'items': items,
            'truncated': has_more or any(item.get('field_continuations') for item in items),
            'continuation': next_token, 'snapshot': snapshot,
            'absence_semantics': 'Empty or unobserved categories do not prove zero activity or safety'}


def _bound_fields(item, limit, offset, selection, reference, ordinal, inner, *, prefix=(), continuations=None):
    import base64
    external = continuations is not None
    continuations = [] if continuations is None else continuations
    def visit(value, path):
        if isinstance(value, dict):
            return {key: visit(child, path + [key]) for key, child in value.items()}
        if isinstance(value, list):
            start = offset if path and path[-1] in ('stack', 'frames', 'path') else 0
            bound = min(limit, 20) if path and path[-1] in ('callers', 'callees', 'neighbors', 'related_findings') else limit
            if start + bound < len(value):
                token = {'selection': selection, 'reference': reference, 'ordinal': ordinal,
                         'report_ordinal': inner, 'field': path, 'offset': start + bound}
                continuations.append(base64.urlsafe_b64encode(json.dumps(token).encode()).decode())
            return [visit(child, path + [start + index]) for index, child in enumerate(value[start:start + bound])]
        return value
    bounded = visit(item, list(prefix))
    if continuations and not external:
        bounded['field_continuations'] = continuations
    return bounded


def query_view_field(run, token, *, limit=50):
    """Resume a bounded nested list without trusting paths supplied by a client."""
    import base64
    from ep_auditor.core.evidence import EvidenceError, digest
    from ep_auditor.core.lifecycle import authoritative_run
    run = Path(run)
    if type(limit) is not int or not 1 <= limit <= 50 or not isinstance(token, str) or len(token) > 4096:
        raise EvidenceError('invalid field continuation bounds')
    try:
        request = json.loads(base64.urlsafe_b64decode(token.encode()).decode())
        selection = request['selection']
        if selection[0] != authoritative_run(run)['run_id'] or selection[1] != digest(run / 'manifest.json'):
            raise ValueError('snapshot changed')
        view, scope = selection[2:4]
        reference, ordinal, inner = request['reference'], request['ordinal'], request['report_ordinal']
        if type(ordinal) is not int or type(inner) is not int or ordinal < 0 or inner < 0:
            raise ValueError('invalid row ordinal')
        if view == 'Dashboard':
            manifest = RawEvidence(run).verify()
            baseline = selection[8] if len(selection) > 8 else None
            if baseline and (authoritative_run(baseline[0])['run_id'] != baseline[1]
                             or digest(Path(baseline[0]) / 'manifest.json') != baseline[2]):
                raise ValueError('baseline snapshot changed')
            source_rows = [(_dashboard_row(run, authoritative_run(run), manifest,
                           baseline=Path(baseline[0]) if baseline else None), None, 0, manifest['capture_complete'])]
        else:
            source_rows = _verified_rows(run, VIEWS[view])
        for row, ref, pos, complete in source_rows:
            if ref != reference or pos != ordinal:
                continue
            value = _view_items(view, row)[inner]
            identity_key = None if selection[7] else {'Functions': 'functionId', 'Findings': 'finding_id', 'Modules': 'moduleId'}.get(view)
            matches = value.get(identity_key) == scope if identity_key else _contains_id(value, scope)
            if scope and not matches:
                raise ValueError('scope mismatch')
            for key in request['field']:
                value = value[key]
            offset = request['offset']
            if not isinstance(value, list) or type(offset) is not int or not 0 <= offset <= len(value):
                raise ValueError('invalid field offset')
            request['offset'] = offset + limit
            more = offset + limit < len(value)
            nested = []
            page = [_bound_fields(entry, limit, 0, selection, ref, pos, inner,
                                  prefix=request['field'] + [offset + index], continuations=nested)
                    for index, entry in enumerate(value[offset:offset + limit])]
            return {'items': page, 'complete': complete, 'truncated': more or bool(nested), 'field_continuations': nested,
                    'continuation': base64.urlsafe_b64encode(json.dumps(request).encode()).decode() if more else None,
                    'evidence_ref': ref, 'snapshot': selection[1]}
        raise ValueError('unknown artifact')
    except (ValueError, TypeError, KeyError, IndexError, UnicodeError) as error:
        raise EvidenceError('invalid field continuation') from error


def _contains_id(value, identity):
    if isinstance(value, dict):
        return any(_contains_id(item, identity) for item in value.values())
    if isinstance(value, (list, tuple)):
        return any(_contains_id(item, identity) for item in value)
    return value == identity


def _dashboard_row(run, header, manifest, *, baseline=None):
    panels = {}
    for name in ('Build', 'Tests', 'Exceptions', 'Locks & Waits', 'Functions', 'Coverage', 'Findings'):
        page = query_run_view(run, name, limit=20)
        panels[name] = {'availability': page['availability'], 'complete': page['complete'],
            'observations': page['items'], 'truncated': page['truncated'], 'continuation': page['continuation'],
            'observed_count_lower_bound': len(page['items']),
            'count_scope': 'This page of observed evidence; not a total when truncated or unavailable'}
    result = {'run_id': header['run_id'], 'status': header['status'],
            'capture_complete': manifest['capture_complete'], 'process': summarize_process_samples(run, process_limit=None),
            'panels': panels, 'baseline_state': 'not_compared',
            'build_result': header['status'] if header['kind'] == 'build' else 'UNKNOWN',
            'new_findings': 'UNKNOWN until compatible baseline comparison',
            'regressions': 'UNKNOWN until compatible baseline comparison',
            'available_views': [name for name, patterns in VIEWS.items()
                                if any(any(run.glob(pattern)) for pattern in patterns)],
            'provenance': 'external', 'confidence': 'derived'}
    if baseline is None:
        return result
    from ep_auditor.analyzers.compare import compare_runs
    from ep_auditor.core.evidence import EvidenceError
    comparison = compare_runs(baseline, run, limit=20)
    result['baseline_state'] = comparison['compatibility']
    result['baseline_run_id'] = comparison['baseline_run_id']
    result['comparison'] = comparison
    if comparison['compatibility'] != 'compatible':
        return result
    baseline_issues = set()
    for report, _, _, _ in _verified_rows(baseline, VIEWS['Findings']):
        for finding in report.get('findings', []):
            if finding.get('issue_id'):
                baseline_issues.add(finding['issue_id'])
                if len(baseline_issues) > 100000:
                    raise EvidenceError('Dashboard baseline issue capacity exceeded')
    new_findings, new_count, unknown_identities = [], 0, 0
    for report, _, _, _ in _verified_rows(run, VIEWS['Findings']):
        for finding in report.get('findings', []):
            if not finding.get('issue_id'):
                unknown_identities += 1
            elif finding['issue_id'] not in baseline_issues:
                new_count += 1
                if new_count > 100000:
                    raise EvidenceError('Dashboard new-issue capacity exceeded')
                new_findings.append(dict(finding, baseline_state='new_observed_issue'))
    result['new_findings'] = {
        'availability': 'observed_comparison' if comparison['complete'] else 'UNKNOWN',
        'items': new_findings, 'observed_count': new_count,
        'unknown_identity_occurrences': unknown_identities, 'truncated': new_count > len(new_findings),
        'scope': 'Known-source issues in the observed detector inputs; missing detectors and identity stay UNKNOWN'}
    result['regressions'] = {'verdict': comparison['regression_verdict'],
        'items': comparison['differences']['findings'],
        'continuation': comparison.get('continuations', {}).get('findings'),
        'confidence': 'heuristic', 'scope': 'Compatible measured candidates; no clean-regression or PASS claim'}
    return result


def publish_findings(run):
    """Consolidate actual detector reports; no finding is a fabricated Run verdict."""
    import hashlib
    from ep_auditor.core.evidence import EvidenceError
    from ep_auditor.core.lifecycle import authoritative_run
    from ep_auditor.core.lineage import EvidenceLineage
    run = Path(run)
    header = authoritative_run(run)
    lineage = EvidenceLineage(run)
    sources = ('findings/exact/*-rules.json', 'metrics/exact/*-functions.json', 'metrics/diagnostics/*.json',
               'graphs/**/*architecture*.json', 'raw/process/lifecycle.json',
               'normalized/diagnostics/sanitizer_stderr.json', 'normalized/diagnostics/verifier.json',
               'graphs/windows_scheduler_sampling.json', 'metrics/windows_disk_io.json',
               'normalized/test_result.json', 'normalized/tests/*.json')
    findings, inputs, sources_complete = [], [], True
    reproduction = None
    for launch, launch_ref, _, launch_complete in _verified_rows(run, ('raw/process/lifecycle.json',)):
        image = Path(launch.get('image_path', ''))
        if launch_complete and image.is_file() and digest(image) == launch.get('image_sha256'):
            reproduction = {'availability': 'captured_inputs_verified', 'rerun_verdict': 'NOT RUN',
                'command': launch['command'], 'cwd': launch['cwd'], 'image_sha256': launch['image_sha256'],
                'environment_sha256': launch['environment_sha256'], 'environment_reference': launch_ref,
                'source_fingerprints': RawEvidence(run).verify()['source_fingerprints'],
                'limitation': 'Captured launch inputs; does not prove reconstruction of external state or reproduction of the finding'}
    for report, reference, ordinal, complete in _verified_rows(run, sources):
        inputs.append(reference)
        sources_complete = sources_complete and complete and report.get('complete', True) is True
        candidates = [item for item in report.get('findings', []) if item.get('status') not in ('PASS', 'UNKNOWN')]
        if report.get('rule_id') and report.get('status') == 'FAIL':
            candidates.append(report)
        if reference['path'] == 'process/lifecycle.json' and report.get('exit_code') not in (None, 0):
            candidates.append({'kind': 'process.nonzero_exit', 'summary': 'Independent target exit is nonzero',
                               'category': 'build' if header['kind'] == 'build' else 'exception',
                               'severity': 'error', 'confidence': 'observed', 'provenance': 'external',
                               'source_ref': {'command': report.get('command'), 'exit_code': report['exit_code']}})
        if reference['path'] in ('diagnostics/sanitizer_stderr.json', 'diagnostics/verifier.json'):
            for candidate in report.get('candidates', []):
                candidates.append({**candidate, 'kind': 'memory.tool_report', 'category': 'memory',
                    'summary': 'Tool/target diagnostic candidate; physical violation not independently established',
                    'confidence': 'heuristic', 'source_ref': {'artifact': reference['path'],
                    'offset': candidate.get('raw_byte_offset'), 'attributes': candidate.get('attributes')}})
        if report.get('eventType') == 'native.exceptions':
            for exception in report.get('events', []):
                candidates.append({**exception, 'kind': 'exception.native_notification', 'category': 'exception',
                    'severity': 'info' if exception.get('first_chance') else 'error',
                    'summary': 'Debugger exception notification; first chance does not establish recovery or defect',
                    'confidence': 'observed', 'source_ref': {'code': exception['code'], 'tid': exception['tid'],
                    'address': exception.get('address'), 'event_id': exception.get('event_id'),
                    'raw_json_pointer': exception['raw_json_pointer'], 'first_chance': exception['first_chance']}})
        for wait in report.get('waits', []):
            if wait.get('end_ns', 0) - wait.get('start_ns', 0) >= 100_000_000:
                candidates.append({**wait, 'kind': 'concurrency.long_observed_wait', 'category': 'concurrency',
                    'summary': 'Observed scheduler wait >=100 ms; ownership/deadlock not inferred',
                    'confidence': 'heuristic', 'source_ref': {'threadId': wait['threadId'], 'start_ns': wait['start_ns']}})
        for request in report.get('disk_requests', []):
            if request['latency_ns'] >= 100_000_000:
                candidates.append({**request, 'kind': 'io.long_observed_completion', 'category': 'io',
                    'summary': 'Externally paired disk completion latency >=100 ms', 'confidence': 'heuristic',
                    'source_ref': {'file': request.get('file_name'), 'request_ordinal': request['request_raw_ordinal']}})
        if (report.get('functionId') and report.get('calls', 0) >= 10
                and report.get('max_ns', 0) > 5 * max(1, report.get('p50_ns', 0))):
            candidates.append({'kind': 'performance.observed_latency_spike', 'category': 'performance',
                'function_id': report['functionId'], 'summary': 'Measured maximum exceeds 5× median across >=10 calls',
                'confidence': 'heuristic', 'source_ref': report.get('symbol', {}),
                'metrics': {key: report.get(key) for key in ('calls', 'max_ns', 'p50_ns', 'p95_ns', 'p99_ns')}})
        if report.get('passed') is False or report.get('status') in ('FAIL', 'failed'):
            candidates.append({'kind': 'test.observed_failure', 'category': 'build',
                'summary': 'Observed failing test result', 'confidence': 'observed',
                'source_ref': {'test_id': report.get('test_id'), 'test_name': report.get('test_name')}})
        for candidate_index, item in enumerate(candidates):
            if not isinstance(item, dict):
                raise EvidenceError('malformed detector finding')
            rule = item.get('rule_id', item.get('kind', 'unknown.detector'))
            source = item.get('source_ref', item.get('source_refs', []))
            identity = json.dumps([rule, source, item.get('function_id'), item.get('event_id')], sort_keys=True)
            key = item.get('finding_id', item.get('findingId')) or 'FindingId:' + hashlib.sha256(identity.encode()).hexdigest()
            stable_source = {name: source[name] for name in ('file', 'line', 'source_file', 'source_line', 'function', 'command')
                             if source.get(name) is not None} if isinstance(source, dict) else {}
            issue = ('IssueId:' + hashlib.sha256(json.dumps([rule, stable_source], sort_keys=True).encode()).hexdigest()) if stable_source else None
            occurrence = 'FindingOccurrenceId:' + hashlib.sha256(json.dumps(
                [header['run_id'], key, reference, ordinal, candidate_index], sort_keys=True).encode()).hexdigest()
            category = item.get('category', item.get('domain')) or next((name for name in
                ('concurrency', 'memory', 'architecture', 'exception', 'io', 'build', 'regression', 'performance')
                if name in str(rule).lower()), 'memory' if 'allocation' in str(rule) else 'architecture')
            findings.append({**item, 'finding_id': key, 'issue_id': issue, 'occurrence_id': occurrence,
                'run_id': header['run_id'], 'rule_id': rule,
                'category': category, 'severity': item.get('severity', 'warning'),
                'confidence': item.get('confidence', report.get('confidence', 'derived')),
                'provenance': item.get('provenance', report.get('provenance', 'external')),
                'summary': item.get('summary', str(rule)), 'evidence_refs': [reference], 'source_refs': source,
                'first_seen': header['started_utc'], 'last_seen': header.get('finished_utc'),
                'baseline_state': 'not_compared', 'reproduction': reproduction,
                'complete': complete and report.get('complete', True) is True})
    # Consolidation references existing findings, so it belongs in the upper
    # agent projection layer. Never weaken the strict downward lineage rule.
    destination = run / 'agent/findings-report.json'
    record = {'eventType': 'findings.report', 'provenance': 'external', 'confidence': 'derived',
              'findings': findings, 'complete': bool(inputs) and sources_complete and all(row['complete'] for row in findings),
              'availability': 'derived_from_observed_inputs' if inputs else 'UNKNOWN',
              'detector_coverage': 'Only named observed inputs; missing detectors remain UNKNOWN'}
    unique = list({json.dumps(ref, sort_keys=True): ref for ref in inputs}.values())
    if not unique:
        return record
    if not destination.exists():
        lineage.write('agent', 'findings-report.json', producer_version='ms67-1', producer_source=Path(__file__),
                      inputs=unique, records=[record])
    elif lineage.verify('agent', 'findings-report.json')['records'] != [record]:
        raise EvidenceError('findings report differs from verified detector inputs')
    return record


def summarize_process_samples(run_dir: Path, *, process_limit=50) -> dict[str, Any]:
    catalog_path = run_dir / "capture_catalog.json"
    name = "process/process_samples.jsonl"
    if not catalog_path.exists() or name not in load_json(catalog_path)["files"]:
        return {"samples": 0, "cpu_avg": None, "cpu_peak": None, "rss_peak_bytes": None, "threads_peak": None,
                "processes": [], "availability": "unavailable", "reason": "canonical_sampling_capture_absent"}
    payload, source = RawEvidence(run_dir).read_capture(name)
    count = 0
    cpu_sum = 0.0
    cpu_peak = 0.0
    cpu_count = 0
    rss_peak = 0
    threads_peak = 0
    rss_count = 0
    threads_count = 0
    per_proc: dict[str, dict[str, float]] = defaultdict(lambda: {"samples": 0.0, "cpu_count": 0, "cpu_sum": 0.0, "cpu_peak": 0.0, "rss_peak": 0.0})
    with io.StringIO(payload.decode("utf-8")) as f:
        for line in f:
            if not line.strip():
                continue
            row = json.loads(line)
            count += 1
            if row.get("cpu_percent_sum") is not None:
                cpu = float(row["cpu_percent_sum"])
                cpu_sum += cpu
                cpu_peak = max(cpu_peak, cpu)
                cpu_count += 1
            if row.get("rss_bytes_sum") is not None:
                rss_peak = max(rss_peak, int(row["rss_bytes_sum"]))
                rss_count += 1
            if row.get("thread_count_sum") is not None:
                threads_peak = max(threads_peak, int(row["thread_count_sum"]))
                threads_count += 1
            for proc in row.get("processes", []):
                key = f"{proc.get('name','?')}[{proc.get('pid','?')}@{proc.get('create_time','unavailable')}]"
                entry = per_proc[key]
                entry["samples"] += 1
                if proc.get("cpu_percent") is not None:
                    pcpu = float(proc["cpu_percent"])
                    entry["cpu_sum"] += pcpu
                    entry["cpu_peak"] = max(entry["cpu_peak"], pcpu)
                    entry["cpu_count"] += 1
                if proc.get('rss_bytes') is not None:
                    entry["rss_peak"] = max(entry["rss_peak"], float(proc['rss_bytes']))
                    entry['rss_observed'] = True
    processes = []
    for name, x in per_proc.items():
        samples = x["cpu_count"]
        processes.append({
            "process": name,
            "cpu_avg": x["cpu_sum"] / samples if samples else None,
            "cpu_peak": x["cpu_peak"] if samples else None,
            "rss_peak_bytes": int(x["rss_peak"]) if x.get('rss_observed') else None,
        })
    processes.sort(key=lambda x: (-(x["cpu_avg"] or 0), -(x["rss_peak_bytes"] or 0)))
    return {
        "samples": count,
        "cpu_avg": cpu_sum / cpu_count if cpu_count else None,
        "cpu_peak": cpu_peak if cpu_count else None,
        "cpu_observed_samples": cpu_count,
        "rss_peak_bytes": rss_peak if rss_count else None,
        "threads_peak": threads_peak if threads_count else None,
        "processes": processes if process_limit is None else processes[:process_limit],
        "processes_total": len(processes),
        "processes_truncated": process_limit is not None and len(processes) > process_limit,
        "raw_source": {"path": source["path"], "sha256": source["sha256"]},
        "capture_health": source["health"], "provenance": source["provenance"], "confidence": "derived",
    }


def aggregate_test_results(results: list[dict[str, Any]]) -> dict[str, Any]:
    passed = sum(1 for x in results if x.get("status") == "PASS")
    failed = sum(1 for x in results if x.get("status") == "FAIL")
    cancelled = sum(1 for x in results if x.get("status") == "CANCELLED")
    return {
        "total": len(results),
        "passed": passed,
        "failed": failed,
        "cancelled": cancelled,
        "wall_seconds_sum": sum(float(x.get("wall_seconds", 0.0)) for x in results),
        "peak_rss_bytes": max((int(x.get("peak_rss_bytes", 0)) for x in results), default=0),
        "slowest": sorted(results, key=lambda x: float(x.get("wall_seconds", 0.0)), reverse=True)[:25],
    }
