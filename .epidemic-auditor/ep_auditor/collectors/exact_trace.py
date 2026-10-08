"""Bounded native /Gh /GH capture ingestion. Raw hook sites need matched PDBs."""
import hashlib
import heapq
import json
import struct
import subprocess
import sys
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.identity import function_identity

HEADER = struct.Struct('<8sIIQIQQQQQIII')
EVENT = struct.Struct('<7Q2I8B')
HEADER_NAMES = ('magic', 'version', 'header_bytes', 'frequency', 'pid', 'birth', 'image_base',
                'capacity', 'written', 'dropped', 'complete', 'path_chars', 'event_bytes')


def read_header(path: Path):
    with path.open('rb') as stream:
        data = stream.read(HEADER.size)
        if len(data) != HEADER.size:
            raise EvidenceError('truncated exact trace header')
        header = dict(zip(HEADER_NAMES, HEADER.unpack(data)))
        if (header.pop('magic') != b'EPTRACE2' or header['version'] != 2
                or header['event_bytes'] != EVENT.size or not 0 < header['path_chars'] < 32768
                or header['header_bytes'] != HEADER.size + header['path_chars'] * 2
                or not 0 < header['capacity'] <= 1048576 or header['written'] > header['capacity']
                or not header['frequency'] or not header['pid'] or not header['birth']
                or not header['image_base'] or header['complete'] not in (0, 1)):
            raise EvidenceError('invalid exact trace header')
        header['image_path'] = stream.read(header['path_chars'] * 2).decode('utf-16-le', errors='strict')
        if path.stat().st_size != header['header_bytes'] + header['written'] * EVENT.size:
            raise EvidenceError('exact trace record count/byte length mismatch')
        header['trace_sha256'] = digest(path)
        return header


def iter_events(path: Path, header: dict):
    previous, previous_qpc, clocks = -1, 0, {}
    with path.open('rb') as stream:
        stream.seek(header['header_bytes'])
        for ordinal in range(header['written']):
            values = EVENT.unpack(stream.read(EVENT.size))
            sequence, qpc, cpu, cycles, address, image, born, pid, tid, kind, *reserved = values
            identity = (tid, born)
            if (sequence <= previous or qpc < previous_qpc or not qpc or pid != header['pid'] or not tid or not born
                    or not address or not image or kind not in (1, 2) or any(reserved)):
                raise EvidenceError('invalid exact event identity/sequence')
            if not header['dropped'] and sequence != previous + 1:
                raise EvidenceError('exact sequence gap with zero declared loss')
            prior = clocks.get(identity, (0, 0, 0))
            if qpc < prior[0] or cpu < prior[1] or cycles < prior[2]:
                raise EvidenceError('exact thread clock/counters reversed')
            clocks[identity] = (qpc, cpu, cycles)
            previous = sequence
            previous_qpc = qpc
            yield {'sequence': sequence, 'timestamp_ns': qpc * 1000000000 // header['frequency'],
                   'cpu_100ns': cpu, 'cycles': cycles, 'address': address, 'image_base': image,
                   'pid': pid, 'process_birth': header['birth'], 'tid': tid, 'thread_birth': born,
                   'kind': 'ENTER' if kind == 1 else 'EXIT', 'rawOrdinal': ordinal,
                   'trace_sha256': header['trace_sha256'],
                   'clock_domain': 'monotonic_trace_ns', 'provenance': 'compiler_instrumentation', 'confidence': 'observed'}


def symbolize_trace(path: Path, header: dict, output: Path, *, helper: Path | None = None):
    image = Path(header['image_path'])
    if not image.is_file():
        raise EvidenceError('captured exact binary unavailable for PDB binding')
    image_sha = digest(image)
    candidates = (Path(sys.executable).parent / 'EpidemicAuditorObservation.exe',
                  Path(sys.executable).parent.parent / 'EpidemicAuditorObservation.exe',
                  Path(__file__).resolve().parents[3] / '.ai/tools/auditor/EpidemicAuditorObservation.exe')
    helper = helper or next((candidate for candidate in candidates if candidate.is_file()), None)
    if helper is None:
        raise EvidenceError('exact trace symbol helper unavailable')
    helper_sha = digest(helper)
    addresses = set()
    for event in iter_events(path, header):
        if event['image_base'] == header['image_base']:
            addresses.add(event['address'])
        if len(addresses) > 131072:
            raise EvidenceError('exact symbol inventory exceeded bound')
    output.mkdir(parents=True, exist_ok=True)
    symbols, commands, pdb = {}, [], None
    addresses = sorted(addresses)
    for offset in range(0, len(addresses), 8192):
        batch = output / ('addresses-' + str(offset) + '.txt')
        batch.write_text('\n'.join(map(str, addresses[offset:offset + 8192])), encoding='ascii')
        command = [str(helper), '--symbol-file', str(image), str(header['image_base']), str(batch)]
        result = subprocess.run(command, capture_output=True, timeout=60, check=False)
        if result.returncode or len(result.stdout) > 64 * 1024 * 1024:
            raise EvidenceError('exact native symbol decoding failed')
        value = json.loads(result.stdout)
        signature = (value['pdb_matched'], value['pdb_guid'], value['pdb_age'])
        if signature[0] is not True or not isinstance(signature[1], str) or not signature[1] or type(signature[2]) is not int or signature[2] <= 0:
            raise EvidenceError('exact symbols require a matched PDB identity')
        if pdb is not None and signature != pdb:
            raise EvidenceError('PDB identity changed across symbol batches')
        pdb = signature
        if {row['address'] for row in value['symbols']} != set(addresses[offset:offset + 8192]):
            raise EvidenceError('symbol decoder omitted/added hook sites')
        for row in value['symbols']:
            start = row['function_address']
            row['functionId'] = function_identity(image_sha, start - header['image_base']) if start is not None else None
            row['module_sha256'] = image_sha
            symbols[row['address']] = row
        commands.append(command)
    if image_sha != digest(image) or helper_sha != digest(helper):
        raise EvidenceError('exact binary/helper changed during symbol decoding')
    return {'symbols': symbols, 'pdb_identity': pdb, 'binary_sha256': image_sha,
            'helper_sha256': helper_sha, 'decoder_commands': commands}


def ingest_trace(run: Path, source: Path, *, helper: Path | None = None):
    header = read_header(source)
    decoded = symbolize_trace(source, header, run / 'exact-symbols' / source.stem, helper=helper)
    from ep_auditor.analyzers.exact_calls import reconstruct_calls
    report = reconstruct_calls(iter_events(source, header),
        {header['image_base']: decoded['symbols']},
        producer_complete=bool(header['complete'] and not header['dropped'] and header['written']))
    symbol_unknown = any(row['functionId'] is None for row in decoded['symbols'].values())
    reason = ('exact_producer_not_finalized' if not header['complete'] else
              'exact_trace_dropped_events' if header['dropped'] else
              'exact_trace_empty' if not header['written'] else
              'exact_symbols_unavailable' if symbol_unknown else
              'exact_trace_unbalanced' if not report['complete'] else None)
    raw = RawEvidence(run)
    name = 'exact/' + source.name
    # Capacity is bytes for streamed file import; event capacity/counters remain
    # explicitly bound in the original header and independent producer metadata.
    size = source.stat().st_size
    trace_row = raw.import_file(name, source, header['trace_sha256'], collector='native_msvc_exact', version='2',
        start_ns=0, end_ns=0, health={'capacity': header['header_bytes'] + header['capacity'] * EVENT.size,
                                   'observed': size, 'dropped': 0, 'incomplete_reason': reason},
        provenance='compiler_instrumentation', confidence='observed')
    encoded = json.dumps({'header': header, 'symbolization': decoded, 'trace_sha256': trace_row['sha256']}, sort_keys=True).encode()
    raw.append('exact/' + source.stem + '-symbols.json', encoded, collector='native_pdb_symbols', version='2',
        start_ns=0, end_ns=0, health={'capacity': len(encoded), 'observed': len(encoded),
                                   'dropped': 0, 'incomplete_reason': reason})
    return header, decoded, trace_row, report


def capture_directory(run: Path, *, helper: Path | None = None, required: bool = False):
    """Register finalized producers before the ordinary run's raw seal."""
    raw = RawEvidence(run)
    sources = sorted((run / 'exact-capture').glob('*.bin'))
    groups = {}
    if required and not sources:
        payload = b'{"availability":"incomplete","reason":"required exact producer missing"}'
        raw.append('exact/missing.json', payload, collector='native_msvc_exact', version='2', start_ns=0, end_ns=0,
            health={'capacity': len(payload), 'observed': len(payload), 'dropped': 0,
                    'incomplete_reason': 'required_exact_trace_missing'})
    for source in sources:
        try:
            header, decoded, _, _ = ingest_trace(run, source, helper=helper)
            groups.setdefault((header['pid'], header['birth']), []).append((source, header, decoded))
        except (EvidenceError, OSError, ValueError, subprocess.SubprocessError) as error:
            # Decoder failure must not discard the original bounded producer bytes.
            if not (run / 'raw/exact' / source.name).exists() and source.is_file():
                size = source.stat().st_size
                if size <= HEADER.size + 65536 + 1048576 * EVENT.size:
                    raw.import_file('exact/' + source.name, source, digest(source), collector='native_msvc_exact',
                        version='2', start_ns=0, end_ns=0,
                        health={'capacity': max(1, size), 'observed': size, 'dropped': 0,
                                'incomplete_reason': 'exact_trace_import_failed'}, provenance='compiler_instrumentation')
            payload = json.dumps({'source': source.name, 'error': str(error),
                                  'availability': 'incomplete'}, sort_keys=True).encode()
            raw.append('exact/' + source.stem + '-failure.json', payload,
                collector='native_msvc_exact', version='2', start_ns=0, end_ns=0,
                health={'capacity': len(payload), 'observed': len(payload), 'dropped': 0,
                        'incomplete_reason': 'exact_trace_import_failed'})
    from ep_auditor.analyzers.exact_calls import reconstruct_calls
    for (pid, birth), producers in groups.items():
        complete, problems = False, {}
        try:
            if (len(producers) > 128 or sum(header['written'] for _, header, _ in producers) > 1048576
                    or len({header['frequency'] for _, header, _ in producers}) != 1
                    or len({header['image_base'] for _, header, _ in producers}) != len(producers)):
                raise EvidenceError('exact process producer capacity/clock/module identity invalid')
            report = reconstruct_calls(heapq.merge(*(iter_events(source, header) for source, header, _ in producers),
                key=lambda event: event['timestamp_ns']),
                {header['image_base']: decoded['symbols'] for _, header, decoded in producers},
                producer_complete=all(header['complete'] and not header['dropped'] and header['written']
                                      for _, header, _ in producers))
            complete, problems = report['complete'], report['problems']
        except (EvidenceError, OSError, ValueError) as error:
            problems = {'error': str(error)}
        payload = json.dumps({'pid': pid, 'process_birth': birth, 'complete': complete, 'problems': problems,
                              'producer_hashes': [header['trace_sha256'] for _, header, _ in producers]}, sort_keys=True).encode()
        raw.append('exact/process-' + str(pid) + '-' + str(birth) + '-health.json', payload,
            collector='native_msvc_exact_process', version='2', start_ns=0, end_ns=0,
            health={'capacity': len(payload), 'observed': len(payload), 'dropped': 0,
                    'incomplete_reason': None if complete else 'exact_process_pairing_incomplete'})


def publish_capture(run: Path):
    """Use the existing raw -> normalized -> graphs/metrics product lineage."""
    from ep_auditor.core.lineage import EvidenceLineage, normalized_record
    from ep_auditor.analyzers.exact_calls import reconstruct_calls
    lineage = EvidenceLineage(run)
    groups = {}
    for source in sorted((run / 'raw/exact').glob('*.bin')):
        if not (run / 'raw/exact' / (source.stem + '-symbols.json')).is_file():
            continue  # Original undecodable bytes stay registered as incomplete.
        header = read_header(source)
        groups.setdefault((header['pid'], header['birth']), []).append((source, header))
    for (pid, birth), producers in groups.items():
        if len(producers) > 128 or sum(header['written'] for _, header in producers) > 1048576:
            raise EvidenceError('exact process reconstruction exceeds bounded producer/event capacity')
        if len({header['frequency'] for _, header in producers}) != 1:
            raise EvidenceError('exact producers disagree on the process QPC domain')
        prefix = 'exact/process-' + str(pid) + '-' + str(birth)
        destination = run / 'metrics' / (prefix + '-functions.json')
        if destination.is_file():
            lineage.verify('metrics', prefix + '-functions.json')
            continue
        refs, symbol_refs, symbolizations, producer_hashes = [], [], {}, []
        for source, header in producers:
            symbols_path = 'exact/' + source.stem + '-symbols.json'
            decoded = json.loads((run / 'raw' / symbols_path).read_text())['symbolization']
            symbols = {int(address): symbol for address, symbol in decoded['symbols'].items()}
            if header['image_base'] in symbolizations:
                raise EvidenceError('module base reused across exact producers; load lifetime unavailable')
            symbolizations[header['image_base']] = symbols
            raw_ref = lineage.reference('raw', 'exact/' + source.name)
            symbol_refs.append(lineage.reference('raw', symbols_path))
            producer_hashes.append(header['trace_sha256'])
            rows, chunk = [], 0
            def write_chunk():
                nonlocal rows, chunk
                path = 'exact/' + source.stem + '-events-' + str(chunk) + '.json'
                lineage.write('normalized', path, producer_version='2', producer_source=Path(__file__),
                              inputs=[raw_ref], records=rows)
                refs.append(lineage.reference('normalized', path))
                rows, chunk = [], chunk + 1
            for event in iter_events(source, header):
                event.update(schemaVersion=1, eventType='exact.' + event['kind'].lower(), required=True)
                rows.append(normalized_record(event, {'exact.enter', 'exact.exit'}, raw_ref, event['rawOrdinal']))
                if len(rows) == 4096:
                    write_chunk()
            if rows:
                write_chunk()
        events = heapq.merge(*(iter_events(source, header) for source, header in producers),
                             key=lambda event: event['timestamp_ns'])
        report = reconstruct_calls(events, symbolizations, producer_complete=all(
            header['complete'] and not header['dropped'] and header['written'] for _, header in producers))
        # An empty trace still has a registered, explicitly incomplete raw input.
        health_path = prefix + '-health.json'
        graph_inputs = refs + symbol_refs
        if (run / 'raw' / health_path).is_file():
            graph_inputs.append(lineage.reference('raw', health_path))
        capture_sha = hashlib.sha256(json.dumps(sorted(producer_hashes)).encode()).hexdigest()
        for offset in range(0, len(report['calls']), 2048):
            lineage.write('graphs', prefix + '-calls-' + str(offset) + '.json',
                producer_version='2', producer_source=Path(__file__).parents[1] / 'analyzers/exact_calls.py',
                inputs=graph_inputs, records=[dict(row, provenance='compiler_instrumentation', confidence='derived')
                                             for row in report['calls'][offset:offset + 2048]])
        lineage.write('metrics', prefix + '-functions.json', producer_version='2',
            producer_source=Path(__file__).parents[1] / 'analyzers/exact_calls.py', inputs=graph_inputs,
            records=[{'provenance': 'compiler_instrumentation', 'confidence': 'derived', 'eventType': 'exact.summary',
                      'scope': report['scope'], 'complete': report['complete'], 'problems': report['problems'],
                      'rankings': report['rankings'], 'hot_paths': report['hot_paths'],
                      'candidate_policy': report['candidate_policy'], 'edges': report['edges']}] +
                    [dict(row, eventType='exact.function', trace_sha256=capture_sha, producer_hashes=producer_hashes,
                          provenance='compiler_instrumentation', confidence='derived')
                     for row in report['functions']])
        static_path = run / 'raw/static/architecture.json'
        if static_path.is_file():
            from ep_auditor.analyzers.architecture import runtime_architecture
            from ep_auditor.analyzers.rules import load_rules, evaluate_rules
            static = json.loads(static_path.read_text(encoding='utf-8'))['architecture']
            project_root = Path(json.loads((run / 'environment.json').read_text())['project_root'])
            architecture = runtime_architecture(report, project_root, static)
            architecture_path = prefix + '-architecture.json'
            lineage.write('graphs', architecture_path, producer_version='1',
                producer_source=Path(__file__).parents[1] / 'analyzers/architecture.py',
                inputs=graph_inputs + [lineage.reference('raw', 'static/architecture.json')], records=[architecture])
            findings = evaluate_rules(load_rules(Path(__file__).resolve().parents[2] / 'rules'), {'architecture': architecture})
            lineage.write('findings', prefix + '-rules.json', producer_version='1',
                producer_source=Path(__file__).parents[1] / 'analyzers/rules.py',
                inputs=[lineage.reference('graphs', architecture_path)], records=findings['findings'])
