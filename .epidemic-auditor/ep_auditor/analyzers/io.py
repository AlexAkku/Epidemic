"""Streaming external disk request/completion facts. Never pair by proximity."""
from __future__ import annotations

from collections import Counter
from ep_auditor.collectors.xperf import process_id
from ep_auditor.core.evidence import EvidenceError


def disk_requests(events, *, complete: bool, main_thread_id: int | None = None) -> dict:
    if type(complete) is not bool:
        raise EvidenceError('IO completeness must be boolean')
    if main_thread_id is not None and (type(main_thread_id) is not int or not 0 < main_thread_id <= 0xFFFFFFFF):
        raise EvidenceError('invalid external main thread identity')
    active, pending, observations, problems = {}, {}, [], Counter()
    selected = []
    for event in events:
        if event.get('provenance') != 'external' or event.get('clock_domain') != 'etl_relative_trace_ns':
            raise EvidenceError('disk facts require external ETW clock/provenance')
        time = event.get('timestamp_ns')
        if type(time) is not int or time < 0:
            raise EvidenceError('IO clock invalid')
        if event['kind'] in {'P-Start','P-DCStart','P-End','P-DCEnd','DiskReadInit','DiskWriteInit','DiskFlushInit',
                             'DiskRead','DiskWrite','DiskFlush'}:
            if len(selected) >= 200_000:
                raise EvidenceError('IO event capacity exceeded')
            selected.append(event)
    # ETW buffers from independent CPUs/providers are not a producer-ordered
    # JSONL stream. Reassemble only these bounded relevant facts by their shared
    # clock; retain original CSV ordinals. Pairing still requires explicit IRP.
    def phase(event):
        kind = event['kind']
        return 0 if kind in {'P-Start','P-DCStart'} else 3 if kind in {'P-End','P-DCEnd'} else 1 if kind.endswith('Init') else 2
    selected.sort(key=lambda event: (event['timestamp_ns'],phase(event),event['rawOrdinal']))
    for event in selected:
        time = event['timestamp_ns']
        kind, fields = event['kind'], event['fields']
        if kind in {'P-Start', 'P-DCStart'}:
            pid = process_id(fields['Process Name ( PID)'])
            if pid in active:
                problems['overlapping_process_lifetime'] += 1
            active[pid] = event['rawOrdinal']
        elif kind in {'P-End', 'P-DCEnd'}:
            pid = process_id(fields['Process Name ( PID)'])
            for key in list(pending):
                if key[0] == pid:
                    problems['request_unfinished_at_process_exit'] += 1
                    pending.pop(key)
            active.pop(pid, None)
        elif kind in {'DiskReadInit','DiskWriteInit','DiskFlushInit','DiskRead','DiskWrite','DiskFlush'}:
            pid = process_id(fields['Process Name ( PID)'])
            if pid not in active:
                problems['process_lifetime_unknown'] += 1
                continue
            try:
                irp = int(fields['IrpPtr'], 16)
                tid = int(fields['ThreadID'])
            except (ValueError, KeyError) as exc:
                raise EvidenceError('invalid disk request identity') from exc
            if not 0 < irp < (1 << 64) or tid <= 0:
                raise EvidenceError('invalid disk request identity')
            operation = kind.removesuffix('Init')
            key = (pid, active[pid], operation, irp)
            if kind.endswith('Init'):
                if key in pending:
                    problems['duplicate_request_identity'] += 1
                    pending[key] = None  # ambiguous: never choose the nearest start
                else:
                    pending[key] = event
                continue
            start = pending.pop(key, None)
            if start is None:
                problems['completion_without_unique_request'] += 1
                continue
            if int(start['fields']['ThreadID']) != tid:
                problems['request_thread_mismatch'] += 1
                continue
            try:
                size = int(fields.get('IOSize', '0'), 16)
            except ValueError as exc:
                raise EvidenceError('invalid disk IO size') from exc
            if not 0 <= size < (1 << 64):
                raise EvidenceError('invalid disk IO size')
            observations.append({'pid': pid, 'process_start_ordinal': active[pid], 'tid': tid,
                'operation': operation, 'irp': irp, 'bytes': size, 'request_ns': start['timestamp_ns'],
                'complete_ns': time, 'latency_ns': time - start['timestamp_ns'],
                'main_thread': tid == main_thread_id if main_thread_id is not None else None,
                'file_name': fields.get('FileName'), 'file_object': fields.get('FileObject'),
                'request_raw_ordinal': start['rawOrdinal'], 'completion_raw_ordinal': event['rawOrdinal']})
    if pending:
        problems['missing_request_completion'] += len(pending)
    return {'schemaVersion': 1, 'complete': complete and not problems and bool(observations),
        'disk_requests': observations, 'problems': dict(problems), 'provenance': 'external', 'confidence': 'derived',
        'scope': 'ETW disk init/completion pairs in observed process lifetimes; not API wall time',
        'action_and_function_attribution': 'UNKNOWN unless explicit cross-stream identities exist'}
