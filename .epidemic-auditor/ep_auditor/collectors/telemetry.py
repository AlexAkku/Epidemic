"""MS6.5/6.6 run-owned telemetry transport.

A shadow-probe stream is *not* independent observation. Files are captured as
engine_probe even when the payload claims to be external. The capture health is
based on actual bytes and the producer's explicit dropped-event counter. A
missing stream is not evidence that no events occurred.
"""
from __future__ import annotations

import json
import stat
import time
import hashlib
import mmap
import struct
import heapq
import threading
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest

MAX_FILE = 8 * 1024 * 1024
MAX_FILES = 8
MAX_TOTAL = 32 * 1024 * 1024
FAMILIES = {'diagnostics': 'diagnostic-capture', 'causal': 'causal-capture'}


class RollingCapture:
    """Run-owned live probe ring. Intentional age eviction differs from capacity loss.

    Stores original JSONL bytes/ordinals in bounded snapshots; it is not a
    replacement for exact compiler tracing and never claims unobserved coverage.
    """
    TRIGGERS = {'crash', 'assert', 'contract', 'long_frame', 'suspected_deadlock', 'memory_error'}

    def __init__(self, run: Path, options: dict):
        if not isinstance(options, dict):
            raise EvidenceError('rolling options must be an object')
        self.run = run
        self.pre_ns = self._seconds(options.get('pre_seconds', 2))
        self.post_ns = self._seconds(options.get('post_seconds', 1))
        self.capacity = options.get('capacity_bytes', 4 * 1024 * 1024)
        if type(self.capacity) is not int or not 65536 <= self.capacity <= 8 * 1024 * 1024:
            raise EvidenceError('rolling memory capacity invalid')
        self.ring, self.sources, self.snapshots = [], {}, []
        self.latest_ns = self.row_sequence = 0
        self.size = self.snapshot_bytes = self.dropped = 0
        self.started_ns = time.perf_counter_ns()
        self.alive_until_ns = self.started_ns
        self.liveness_pid = None
        self.life_lock = threading.Lock()
        self.pending_exit = None
        self.last_poll_ns = 0
        self.seen_triggers = set()
        self.exact_sources = {}

    @staticmethod
    def _seconds(value):
        if type(value) not in (int, float) or not 0 <= value <= 60:
            raise EvidenceError('rolling seconds exceed finite bound')
        return int(value * 1_000_000_000)

    def trigger(self, kind: str, stamp: int, *, provenance='external', reason=None):
        if kind not in self.TRIGGERS or type(stamp) is not int or stamp < 0:
            raise EvidenceError('invalid rolling trigger')
        if kind in self.seen_triggers:
            return
        if len(self.snapshots) >= 6:
            self.dropped += 1
            return
        self.seen_triggers.add(kind)
        before = [row for _, _, row in self.ring if stamp - self.pre_ns <= row['timestamp_ns'] <= stamp + self.post_ns]
        retained = []
        for row in before:
            size = self._size(row)
            if self.snapshot_bytes + size > self.capacity:
                self.dropped += 1
            else:
                retained.append(row)
                self.snapshot_bytes += size
        self.snapshots.append({'trigger': kind, 'trigger_ns': stamp, 'provenance': provenance,
            'reason': reason, 'start_ns': max(0, stamp - self.pre_ns), 'end_ns': stamp + self.post_ns,
            'records': retained, 'pre_coverage': 'observed_from_run_start' if self.started_ns > stamp - self.pre_ns
                            else 'observed_live_window'})

    def observe_liveness(self, code, stamp):
        # Called by the existing sampler. It never mutates ring/snapshot state.
        with self.life_lock:
            if code is None:
                self.alive_until_ns = max(self.alive_until_ns, stamp)
            elif self.pending_exit is None:
                self.pending_exit = (code, stamp)

    def observe_process_exit(self, code, stamp=None):
        terminal = {0xC0000420: 'assert', 0xE0434242: 'contract', 0xC0000005: 'crash',
                    0xC0000409: 'crash', 0xE0424242: 'crash', 0xC000001D: 'crash'}
        kind = terminal.get((code & 0xffffffff) if type(code) is int else None)
        if kind:
            self.trigger(kind, time.perf_counter_ns() if stamp is None else stamp, reason='observed_process_exception_exit')

    def _accept(self, source, ordinal, line):
        try:
            def unique(pairs):
                row = {}
                for key, value in pairs:
                    if key in row:
                        raise ValueError('duplicate key')
                    row[key] = value
                return row
            event = json.loads(line, object_pairs_hook=unique)
            family = 'diagnostics' if source.parent.name == 'diagnostic-capture' else 'causal'
            from ep_auditor.collectors.telemetry_pipeline import preflight_stream
            if not preflight_stream(line, family, 1):
                raise ValueError('invalid physical probe')
            stamp = event['timestamp_ns']
            row = {'source': source.relative_to(self.run).as_posix(), 'ordinal': ordinal,
                   'sha256': hashlib.sha256(line).hexdigest(), 'timestamp_ns': stamp,
                   'source_line': line.decode('utf-8')}
        except (ValueError, KeyError, UnicodeError, EvidenceError):
            self.dropped += 1
            return
        self._retain(row)
        automatic = {'crash': 'crash', 'assert': 'assert', 'contract': 'contract', 'memory.error': 'memory_error'}
        kind = automatic.get(event['type'])
        if event['type'] == 'capture.trigger':
            candidate = event.get('trigger_id')
            if candidate in self.TRIGGERS:
                kind = candidate
        if kind:
            self.trigger(kind, stamp, provenance='engine_probe', reason='explicit_live_probe')

    @staticmethod
    def _size(row):
        if 'source_bytes_hex' in row:
            return len(row['source_bytes_hex']) // 2
        return len(row['source_line'].encode('utf-8'))

    def _retain(self, row):
        stamp, size = row['timestamp_ns'], self._size(row)
        self.latest_ns = max(self.latest_ns, stamp)
        cutoff = self.latest_ns - self.pre_ns
        while self.ring and (self.ring[0][0] < cutoff or self.size + size > self.capacity
                             or len(self.ring) >= 131072):
            _, _, old = heapq.heappop(self.ring)
            if old['timestamp_ns'] >= cutoff:
                self.dropped += 1
            self.size -= self._size(old)
        if size > self.capacity:
            self.dropped += 1
            return
        if stamp >= cutoff:
            heapq.heappush(self.ring, (stamp, self.row_sequence, row))
            self.row_sequence += 1
            self.size += size
        for capture in self.snapshots:
            if capture['start_ns'] <= stamp <= capture['end_ns']:
                if self.snapshot_bytes + size > self.capacity:
                    self.dropped += 1
                else:
                    capture['records'].append(row)
                    self.snapshot_bytes += size

    def _poll_exact(self, *, final=False):
        from ep_auditor.collectors.exact_trace import EVENT
        layout = struct.Struct('<8s7Q')
        directory = self.run / 'rolling-exact-capture'
        if _linked(directory):
            self.dropped += 1
            return
        for source in sorted(directory.glob('*.bin')):
            if _linked(source) or not layout.size <= source.stat().st_size <= layout.size + 65536 * EVENT.size:
                self.dropped += 1
                continue
            if source not in self.exact_sources and len(self.exact_sources) >= 16:
                self.dropped += 1
                return
            try:
                with source.open('rb') as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as view:
                    header_bytes = view[:layout.size]
                    magic, generation, total, capacity, frequency, pid, birth, base = layout.unpack(header_bytes)
                    if generation & 1:
                        if final:
                            self.dropped += 1
                        continue # Writer in progress; retry on the next owned poll.
                    if not any(header_bytes):
                        if final:
                            self.dropped += 1
                        continue # Mapping exists before its first header commit.
                    if (magic != b'EPROLL01' or not 256 <= capacity <= 65536
                            or source.stat().st_size != layout.size + capacity * EVENT.size
                            or not all((frequency, pid, birth, base))):
                        raise ValueError('invalid live exact ring header')
                    previous = self.exact_sources.get(source, 0)
                    if total < previous:
                        raise ValueError('live exact sequence reversed')
                    start = max(previous, total - min(total, capacity))
                    payloads = []
                    for ordinal in range(start, total):
                        offset = layout.size + (ordinal % capacity) * EVENT.size
                        payloads.append((ordinal, view[offset:offset + EVENT.size]))
                    if view[:layout.size] != header_bytes:
                        continue # Concurrent wrap must not produce a torn observation.
                    rows = []
                    for ordinal, payload in payloads:
                        sequence, qpc, cpu, cycles, address, image, thread_birth, event_pid, tid, kind, *reserved = EVENT.unpack(payload)
                        if sequence != ordinal or event_pid != pid or not all((qpc, address, image, thread_birth, tid)) or kind not in (1, 2) or any(reserved):
                            raise ValueError('invalid live exact ring event')
                        rows.append({'source': source.relative_to(self.run).as_posix(), 'ordinal': ordinal,
                            'sha256': hashlib.sha256(payload).hexdigest(), 'source_bytes_hex': payload.hex(),
                            'timestamp_ns': qpc * 1_000_000_000 // frequency,
                            'pid': pid, 'process_epoch': birth, 'tid': tid, 'producer_base': base,
                            'type': 'ENTER' if kind == 1 else 'EXIT',
                            'provenance': 'compiler_instrumentation', 'confidence': 'observed'})
                self.dropped += start - previous
                self.exact_sources[source] = total
                for row in rows:
                    self._retain(row)
            except (OSError, ValueError, struct.error):
                self.dropped += 1

    def poll(self, *, final=False, process_alive=True):
        now = time.perf_counter_ns()
        if process_alive:
            self.observe_liveness(None, now)
        with self.life_lock:
            pending, self.pending_exit = self.pending_exit, None
        if pending is not None:
            self.observe_process_exit(*pending)
        if not final and now - self.last_poll_ns < 100_000_000:
            return
        self.last_poll_ns = now
        self._poll_exact(final=final)
        budget = 8 * 65536 if final else 65536
        for family in FAMILIES.values():
            directory = self.run / family
            if _linked(directory):
                self.dropped += 1
                continue
            for source in sorted(directory.glob('*.jsonl')):
                if source not in self.sources:
                    if len(self.sources) >= MAX_FILES * len(FAMILIES):
                        self.dropped += 1
                        return
                    self.sources[source] = [0, 0, b'']
                if _linked(source):
                    self.dropped += 1
                    continue
                state = self.sources[source]
                try:
                    if source.stat().st_size < state[0]:
                        raise ValueError('producer truncated')
                    with source.open('rb') as stream:
                        stream.seek(state[0])
                        payload = stream.read(budget)
                    state[0] += len(payload)
                    lines = (state[2] + payload).split(b'\n')
                    state[2] = lines.pop()
                    if len(state[2]) > 65536:
                        raise ValueError('producer line over budget')
                    for line in lines:
                        self._accept(source, state[1], line + b'\n')
                        state[1] += 1
                    if final and (state[2] or source.stat().st_size > state[0]):
                        self.dropped += 1
                except (OSError, ValueError):
                    self.dropped += 1

    def finish(self, *, process_exited: bool):
        self.poll(final=True, process_alive=not process_exited)
        if not self.snapshots:
            return
        now = time.perf_counter_ns()
        with self.life_lock:
            observed_alive = self.alive_until_ns
        for capture in self.snapshots:
            capture['post_coverage'] = ('observed_live_window' if observed_alive >= capture['end_ns'] else
                                        'process_ended' if process_exited else 'INCOMPLETE')
            capture['records'].sort(key=lambda row: (row['timestamp_ns'], row['source'], row['ordinal']))
        window_complete = not self.dropped and all(row['pre_coverage'] == 'observed_live_window'
            and row['post_coverage'] == 'observed_live_window' for row in self.snapshots)
        payload = json.dumps({'schemaVersion': 1, 'pre_ns': self.pre_ns, 'post_ns': self.post_ns,
            'liveness_pid': self.liveness_pid, 'last_observed_alive_ns': observed_alive,
            'liveness_clock': 'python_perf_counter_windows_qpc',
            'scope': 'live engine probes and compiler ENTER/EXIT ring; separate full exact capture is not inferred',
            'exact_availability': 'observed_compiler_ring' if self.exact_sources else 'UNKNOWN',
            'dropped': self.dropped, 'retained_bytes_complete': not self.dropped,
            'complete': window_complete,
            'snapshots': self.snapshots},
            sort_keys=True).encode('utf-8')
        RawEvidence(self.run).append('rolling/probes.json', payload, collector='live_probe_rolling_buffer', version='1',
            start_ns=self.started_ns, end_ns=now,
            health={'capacity': max(1, len(payload)), 'observed': len(payload), 'dropped': 0,
                    'incomplete_reason': 'rolling_capture_loss' if self.dropped else 'rolling_window_incomplete' if not window_complete else None},
            provenance='external', confidence='derived')


def _linked(path: Path) -> bool:
    try:
        info = path.lstat()
        return stat.S_ISLNK(info.st_mode) or bool(getattr(info, 'st_file_attributes', 0) &
                                                getattr(stat, 'FILE_ATTRIBUTE_REPARSE_POINT', 0x400))
    except FileNotFoundError:
        return False


def _failure(raw: RawEvidence, family: str, reason: str) -> None:
    payload = json.dumps({'schemaVersion': 1, 'availability': 'incomplete', 'reason': reason}, sort_keys=True).encode()
    raw.append(f'{family}/capture-failure.json', payload, collector='engine_probe_transport', version='1',
               start_ns=0, end_ns=0, health={'capacity': max(1, len(payload)), 'observed': len(payload),
                                            'dropped': 0, 'incomplete_reason': reason},
               provenance='engine_probe', confidence='declared')


def capture_streams(run: Path, family: str, *, required: bool = False) -> None:
    if family not in FAMILIES:
        raise EvidenceError('unknown telemetry family')
    directory = run / FAMILIES[family]
    raw = RawEvidence(run)
    if _linked(directory):
        _failure(raw, family, 'linked_producer_directory')
        return
    streams = []
    if directory.is_dir():
        for source in directory.glob('*.jsonl'):
            streams.append(source)
            if len(streams) > MAX_FILES:
                _failure(raw, family, 'producer_count_exceeded')
                return
    streams.sort()
    if not streams:
        if required:
            _failure(raw, family, 'required_' + family + '_producer_missing')
        return
    if len(streams) > MAX_FILES:
        _failure(raw, family, 'producer_count_exceeded')
        return
    observed_total = 0
    for number, source in enumerate(streams):
        # A producer cannot inject names into an evidence path or escape its run.
        if _linked(source) or not source.is_file() or not source.resolve().is_relative_to(directory.resolve()):
            _failure(raw, family, 'non_regular_capture_source')
            return
        size = source.stat().st_size
        observed_total += size
        if observed_total > MAX_TOTAL:
            _failure(raw, family, 'producer_total_byte_budget_exceeded')
            return
        health_path = source.with_suffix('.health.json')
        reason = None
        declared = None
        if not health_path.is_file() or _linked(health_path):
            reason = 'producer_health_missing'
        else:
            try:
                if health_path.stat().st_size > 65536:
                    raise ValueError('health budget exceeded')
                def unique_health(pairs):
                    out = {}
                    for key, value in pairs:
                        if key in out:
                            raise ValueError('duplicate producer health key')
                        out[key] = value
                    return out
                declared = json.loads(health_path.read_text(encoding='utf-8'),
                                      object_pairs_hook=unique_health,
                                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('nonfinite health')))
                if (type(declared.get('schemaVersion')) is not int or declared['schemaVersion'] != 1
                        or type(declared.get('emitted')) is not int or not 0 <= declared['emitted'] < (1 << 64)
                        or type(declared.get('dropped')) is not int or not 0 <= declared['dropped'] < (1 << 64)
                        or type(declared.get('complete')) is not bool):
                    raise ValueError('invalid health counters')
                if (not declared['complete'] or declared['dropped']):
                    reason = 'producer_incomplete_or_dropped'
            except (ValueError, KeyError, TypeError):
                reason = 'producer_health_invalid'
        if size <= MAX_FILE and size and reason is None:
            from ep_auditor.collectors.telemetry_pipeline import preflight_stream
            if not preflight_stream(source.read_bytes(), family, declared['emitted'] if declared else None):
                reason = 'producer_payload_or_count_invalid'
        if size > MAX_FILE:
            reason = 'producer_byte_budget_exceeded'
        if not size:
            reason = 'empty_producer_stream'
        if size <= MAX_FILE:
            # Keep even invalid source bytes. The analyzer will refuse to derive
            # a successful observation from malformed records.
            raw.import_file(f'{family}/stream-{number}.jsonl', source, digest(source),
                            collector='engine_probe_jsonl', version='1', start_ns=0, end_ns=0,
                            health={'capacity': MAX_FILE, 'observed': size,
                                    'dropped': 0,  # source file bytes were imported without loss; producer event loss stays in the reason/health sidecar
                                    'incomplete_reason': reason}, provenance='engine_probe', confidence='declared')
        else:
            # Retain the bounded physical prefix and record exactly how many
            # source bytes were not captured. Do not call an oversized producer clean.
            with source.open('rb') as stream:
                prefix = stream.read(MAX_FILE)
            raw.append(f'{family}/stream-{number}.jsonl', prefix,
                       collector='engine_probe_jsonl', version='1', start_ns=0, end_ns=0,
                       health={'capacity': MAX_FILE, 'observed': size,
                               'dropped': size - len(prefix),
                               'incomplete_reason': 'producer_byte_budget_exceeded'},
                       provenance='engine_probe', confidence='declared')
        if health_path.is_file() and health_path.stat().st_size <= 65536 and not _linked(health_path):
            raw.import_file(f'{family}/stream-{number}.health.json', health_path, digest(health_path),
                            collector='engine_probe_health', version='1', start_ns=0, end_ns=0,
                            health={'capacity': 65536, 'observed': health_path.stat().st_size, 'dropped': 0,
                                    'incomplete_reason': reason}, provenance='engine_probe', confidence='declared')
