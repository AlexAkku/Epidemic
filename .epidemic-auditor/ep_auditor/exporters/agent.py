"""Bounded agent projections of the same sealed evidence used by the GUI."""
from __future__ import annotations

import csv
import html
import io
import json
from pathlib import Path

from ep_auditor.analyzers.summary import query_run_view, query_view_field
from ep_auditor.core.evidence import atomic_json, digest, load_json, EvidenceError
from ep_auditor.core.exports import _write
from ep_auditor.core.lifecycle import authoritative_run

REPORTS = {
    'RUN_SUMMARY': 'Dashboard', 'BUILD_DIAGNOSTICS': 'Build', 'FAILURES': 'Findings',
    'ACTIONS': 'Actions', 'EXCEPTIONS': 'Exceptions', 'FUNCTION_PROFILE': 'Functions',
    'CALL_GRAPH': 'Call Tree', 'THREADS': 'Threads', 'LOCKS': 'Locks & Waits',
    'MEMORY': 'Memory', 'IO': 'IO', 'COVERAGE': 'Coverage', 'ARCHITECTURE': 'Architecture',
    'HOT_PATHS': 'Hot Paths', 'IMPORTED_APIS': 'Imported APIs', 'STATIC_SCAN': 'Static Scan',
    'STRESS': 'Stress', 'MODULES': 'Modules',
}
KINDS = {'finding': 'Findings', 'test': 'Tests', 'action': 'Actions', 'trace': 'Timeline',
         'function': 'Functions', 'module': 'Modules', 'scenario': 'Timeline'}


def export_investigation(run_dir, kind, stable_id, *, cursor=None, limit=200, frame_limit=50):
    if kind not in KINDS:
        raise ValueError('unsupported investigation kind')
    if not stable_id:
        raise ValueError('a stable ID is required')
    view = KINDS[kind]
    page = query_run_view(run_dir, view, scope=stable_id, cursor=cursor, limit=limit, frame_limit=frame_limit)
    # Related findings/neighbors remain independent, explicitly bounded pages.
    page['related_findings'] = query_run_view(run_dir, 'Findings', scope=stable_id, limit=20, reference_scope=True)
    page['neighbors'] = query_run_view(run_dir, 'Causal Graph', scope=stable_id, limit=20)
    page['events'] = query_run_view(run_dir, 'Timeline', scope=stable_id, limit=200, frame_limit=frame_limit)
    page['calls'] = query_run_view(run_dir, 'Call Tree', scope=stable_id, limit=20, frame_limit=frame_limit)
    return page


def _report(agent, name, page):
    compact = lambda value: json.dumps(value, ensure_ascii=False, sort_keys=True, allow_nan=False)
    atomic_json(agent / (name + '.json'), page)
    metadata = {key: value for key, value in page.items() if key != 'items'}
    rows = [{'metadata': metadata, 'record': row} for row in page.get('items', [])]
    # An empty category still exports UNKNOWN metadata rather than an empty proof.
    if not rows:
        rows = [{'metadata': metadata, 'record': None}]
    _write(agent / (name + '.jsonl'), ''.join(compact(row) + '\n' for row in rows))
    stream = io.StringIO(newline='')
    writer = csv.writer(stream)
    writer.writerow(('metadata_json', 'record_json'))
    writer.writerows((compact(row['metadata']), compact(row['record'])) for row in rows)
    _write(agent / (name + '.csv'), stream.getvalue())
    _write(agent / (name + '.md'), '# ' + name.replace('_', ' ').title() + '\n\n<pre>' + html.escape(compact(page)) + '</pre>\n')


def export_agent_pack(run_dir: Path, run_meta=None, summary=None, tests=None, static_scan=None, *, baseline=None):
    """Legacy callers stay compatible; caller summaries never become evidence."""
    run_dir = Path(run_dir)
    header = authoritative_run(run_dir)
    snapshot = digest(run_dir / 'manifest.json')
    agent = run_dir / 'agent'
    agent.mkdir(parents=True, exist_ok=True)
    pages = {name: query_run_view(run_dir, view) for name, view in REPORTS.items()}
    from ep_auditor.analyzers.compare import compare_runs
    pages['REGRESSIONS'] = ({'items': [compare_runs(baseline, run_dir)], 'availability': 'observed'} if baseline else
                            {'items': [], 'availability': 'UNKNOWN', 'reason': 'No baseline selected'})
    raw = load_json(run_dir / 'manifest.json', expected_sha256=snapshot)
    files = raw['raw_files']
    pages['RAW_EVIDENCE'] = {'run_id': header['run_id'], 'items': files[:200],
                             'snapshot': snapshot, 'truncated': len(files) > 200,
                             'manifest': '../manifest.json', 'continuation': 200 if len(files) > 200 else None}
    pages['INDEX'] = {'run_id': header['run_id'], 'status': header['status'], 'items': [
        {'report': name, 'availability': page.get('availability', 'observed'),
         'truncated': page.get('truncated', False), 'path': name + '.json'} for name, page in pages.items()],
        'policy': 'UNKNOWN is not zero; a heuristic finding is not a verified defect; follow continuation tokens'}
    for name, page in pages.items():
        _report(agent, name, page)
    names = [name + suffix for name in pages for suffix in ('.md', '.json', '.jsonl', '.csv')]
    if digest(run_dir / 'manifest.json') != snapshot or authoritative_run(run_dir) != header:
        raise EvidenceError('run changed during agent export; no completed pack can be published')
    atomic_json(agent / 'manifest.json', {'schemaVersion': 1, 'run_id': header['run_id'], 'status': header['status'],
                                        'capture_manifest_sha256': snapshot,
                                        'producer_source_sha256': digest(Path(__file__)),
                                        'files': {name: digest(agent / name) for name in names}})
    return agent


def query_raw_evidence(run_dir, *, offset=0, limit=200, snapshot=None):
    from ep_auditor.core.evidence import RawEvidence, EvidenceError
    run_dir = Path(run_dir)
    authoritative_run(run_dir)
    manifest = RawEvidence(run_dir).verify()
    current = digest(run_dir / 'manifest.json')
    if type(offset) is not int or offset < 0 or type(limit) is not int or not 1 <= limit <= 200 or snapshot not in (None, current):
        raise EvidenceError('invalid raw evidence page or changed snapshot')
    rows = manifest['raw_files']
    more = offset + limit < len(rows)
    return {'items': rows[offset:offset + limit], 'snapshot': current, 'truncated': more,
            'continuation': offset + limit if more else None}
