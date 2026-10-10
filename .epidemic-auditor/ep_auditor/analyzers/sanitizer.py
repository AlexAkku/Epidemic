"""Bounded ASan/LSan stderr report candidates.

An arbitrary target can print an ASan-looking line. This parser therefore
never asserts an observed memory violation or clean memory status on its own.
"""
from __future__ import annotations

import re
from itertools import chain, islice
from ep_auditor.core.evidence import EvidenceError

MARKER = re.compile(rb'(?:ERROR:\s*AddressSanitizer:\s*|SUMMARY:\s*AddressSanitizer:\s*|WARNING:\s*ThreadSanitizer:\s*)([a-zA-Z0-9_-]{3,96})')
LEAK = re.compile(rb'LeakSanitizer:\s*detected memory leaks')
CAPACITY = 16 * 1024 * 1024
MAX_FINDINGS = 128


def parse_verifier_xml(data: bytes, *, expected_pid: int) -> dict:
    """Parse a bounded tool export, binding its session to an observed process.

    Empty exports establish neither activation nor absence of memory errors.
    Page-heap access violations may be recorded only by the independent debugger.
    """
    from xml.etree import ElementTree
    if (not isinstance(data, bytes) or not data or len(data) > CAPACITY
            or type(expected_pid) is not int or not 0 < expected_pid <= 0xFFFFFFFF):
        raise EvidenceError('invalid Application Verifier export scope')
    if b'\x00' in data or b'<!DOCTYPE' in data.upper() or b'<!ENTITY' in data.upper():
        raise EvidenceError('Application Verifier XML declarations forbidden')
    try:
        root = ElementTree.fromstring(data)
    except ElementTree.ParseError as exc:
        raise EvidenceError('invalid Application Verifier XML') from exc
    namespace = '{Application Verifier}'
    if root.tag != namespace + 'logfile':
        raise EvidenceError('unsupported Application Verifier export schema')
    sessions = list(root.iter(namespace + 'logSession'))
    if len(sessions) != 1 or sessions[0].get('PID') != str(expected_pid):
        raise EvidenceError('Application Verifier process session mismatch')
    entries = []
    for entry in root.iter(namespace + 'logEntry'):
        if len(entries) == MAX_FINDINGS:
            break
        entries.append({'attributes': dict(entry.attrib), 'text': ''.join(entry.itertext())[:4096],
                        'classification': 'tool_report', 'independent_validation': False})
    return {'schemaVersion': 1, 'pid': expected_pid, 'session': dict(sessions[0].attrib),
            'candidates': entries, 'memory_safety_verdict': 'UNKNOWN', 'activation_verified': False,
            'provenance': 'external', 'confidence': 'declared',
            'warning': 'Join with debugger module/exception capture; empty XML does not imply memory safety'}


def parse_stderr(data: bytes) -> dict:
    if not isinstance(data, bytes) or len(data) > CAPACITY:
        raise EvidenceError('sanitizer stderr beyond bounded decoder scope')
    findings = []
    for match in MARKER.finditer(data):
        if len(findings) == MAX_FINDINGS:
            break
        findings.append({'kind': match.group(1).decode('ascii'), 'raw_byte_offset': match.start(),
                         'classification': 'stderr_text_candidate', 'independent_validation': False})
    for match in LEAK.finditer(data):
        if len(findings) == MAX_FINDINGS:
            break
        findings.append({'kind': 'leak_report', 'raw_byte_offset': match.start(),
                         'classification': 'stderr_text_candidate', 'independent_validation': False})
    return {'schemaVersion': 1, 'candidates': findings, 'candidate_count': len(findings),
            'truncated': sum(1 for _ in islice(chain(MARKER.finditer(data), LEAK.finditer(data)), MAX_FINDINGS + 1)) > MAX_FINDINGS,
            'source': 'raw captured process stderr', 'memory_safety_verdict': 'UNKNOWN',
            'provenance': 'external', 'confidence': 'heuristic',
            'warning': 'Raw stderr is externally recorded but content is target-controlled; ASan activation/binary identity are unverified'}
