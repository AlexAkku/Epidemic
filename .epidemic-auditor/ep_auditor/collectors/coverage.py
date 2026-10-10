"""LLVM source coverage: real profiles, merged scopes and source-bound heatmaps."""
import json
import re
from pathlib import Path
import shutil
import subprocess

from ep_auditor.core.evidence import RawEvidence, EvidenceError, digest


def coverage_tools(binaries):
    """Prefer decoders shipped with the compiler recorded by each CMake build."""
    compilers = set()
    for binary in binaries:
        build = next((parent for parent in binary.resolve().parents if (parent / 'CMakeCache.txt').is_file()), None)
        if build is not None:
            for file in (build / 'CMakeFiles').glob('*/CMakeCXXCompiler.cmake'):
                with file.open(encoding='utf-8') as stream:
                    first_lines = ''.join(next(stream, '') for _ in range(12))
                match = re.search(r'set\(CMAKE_CXX_COMPILER\s+"([^"]+)"', first_lines)
                if match:
                    compilers.add(Path(match[1]).parent)
    if len(compilers) > 1:
        raise EvidenceError('coverage binaries use different compiler toolchains')
    directory = next(iter(compilers), None)
    return {name: str(directory / (name + '.exe')) if directory and (directory / (name + '.exe')).is_file()
            else shutil.which(name) for name in ('llvm-profdata', 'llvm-cov')}


def collect_coverage(run: Path, profiles: list[Path], binaries: list[Path], *, scope: dict):
    if not profiles or not binaries or len(profiles) > 4096 or len(binaries) > 256:
        raise EvidenceError('coverage requires bounded real profiles and instrumented binaries')
    tools = coverage_tools(binaries)
    if not all(tools.values()):
        raise EvidenceError('LLVM coverage tools unavailable')
    sources = {str(path.resolve()): digest(path) for path in profiles + binaries}
    tool_hashes = {name: digest(Path(path)) for name, path in tools.items()}
    output = run / 'coverage-analysis'
    output.mkdir(parents=True, exist_ok=True)
    merged, exported = output / 'merged.profdata', output / 'coverage.json'
    objects = [argument for binary in binaries[1:] for argument in ('-object', str(binary))]
    commands = [[tools['llvm-profdata'], 'merge', '-sparse', *map(str, profiles), '-o', str(merged)],
                [tools['llvm-cov'], 'export', str(binaries[0]), '-instr-profile=' + str(merged),
                 *objects],
                [tools['llvm-cov'], 'show', str(binaries[0]), '-instr-profile=' + str(merged),
                 '-format=html', '-show-branches=count', '-output-dir=' + str(output / 'heatmap'), *objects]]
    for index, command in enumerate(commands):
        with (exported if index == 1 else output / ('command-' + str(index) + '.stdout')).open('wb') as stream:
            result = subprocess.run(command, stdout=stream, stderr=subprocess.PIPE, timeout=120)
        if result.returncode or result.stderr.strip():
            raise EvidenceError('LLVM coverage command failed: ' + result.stderr.decode(errors='replace')[:1000])
    if exported.stat().st_size > 128 * 1024**2:
        raise EvidenceError('coverage export exceeds bounded decoder input')
    value = json.loads(exported.read_text(encoding='utf-8'))
    if value.get('type') != 'llvm.coverage.json.export' or str(value.get('version', '')).split('.')[0] not in {'2', '3'}:
        raise EvidenceError('unsupported LLVM coverage export schema')
    if sources != {str(path.resolve()): digest(path) for path in profiles + binaries}:
        raise EvidenceError('coverage producer binary/profile changed during decoding')
    if tool_hashes != {name: digest(Path(path)) for name, path in tools.items()}:
        raise EvidenceError('coverage tools changed during decoding')
    files, functions = [], []
    for unit in value.get('data', []):
        for file in unit['files']:
            source = Path(file['filename'])
            files.append({'file': str(source), 'source_sha256': digest(source) if source.is_file() else None,
                          'compiled_source_match': 'unverified: LLVM mapping does not bind source bytes',
                          'summary': file['summary'], 'segments': file['segments'], 'branches': file['branches']})
        functions.extend({'name': row['name'], 'count': row['count'], 'files': row['filenames'],
                          'regions': row['regions']} for row in unit['functions'])
    if not files or not functions:
        raise EvidenceError('coverage export contains no instrumented source inventory')
    report = {'schemaVersion': 1, 'scope': scope, 'files': files, 'functions': functions,
        'never_called_candidates': [row['name'] for row in functions if row['count'] == 0],
        'once_called_candidates': [row['name'] for row in functions if row['count'] == 1],
        'source_comparison': 'counts bind instrumented binaries; current source hashes are decode-time observations, not proof of compiled source identity; heatmaps require unchanged source; absence is not dead-code proof',
        'producer_inputs': sources, 'decoder_tools': tool_hashes, 'commands': commands,
        'provenance': 'compiler_instrumentation', 'confidence': 'derived'}
    raw = RawEvidence(run)
    heatmap = []
    total = 0
    for path in sorted((output / 'heatmap').rglob('*')):
        if not path.is_file():
            continue
        size = path.stat().st_size
        total += size
        if total > 128 * 1024**2 or len(heatmap) >= 4096:
            raise EvidenceError('coverage heatmap exceeds bounded output capacity')
        relative = 'coverage/heatmap/' + path.relative_to(output / 'heatmap').as_posix()
        fingerprint = digest(path)
        raw.import_file(relative, path, fingerprint, collector='llvm_coverage_heatmap', version='1',
            start_ns=0, end_ns=0, health={'capacity': max(1, size), 'observed': size, 'dropped': 0,
                                        'incomplete_reason': None}, provenance='compiler_instrumentation', confidence='derived')
        heatmap.append({'path': 'raw/' + relative, 'sha256': fingerprint})
    if not any(row['path'].endswith('index.html') for row in heatmap):
        raise EvidenceError('coverage heatmap index missing')
    report['heatmap'] = heatmap
    for ordinal, source in enumerate(profiles):
        size = source.stat().st_size
        raw.import_file('coverage/profile-' + str(ordinal) + '.profraw', source, sources[str(source.resolve())],
            collector='llvm_source_coverage', version='1', start_ns=0, end_ns=0,
            health={'capacity': max(1, size), 'observed': size, 'dropped': 0, 'incomplete_reason': None},
            provenance='compiler_instrumentation')
    payload = json.dumps(report, sort_keys=True).encode()
    raw.append('coverage/report.json', payload, collector='llvm_source_coverage', version='1',
        start_ns=0, end_ns=0, health={'capacity': len(payload), 'observed': len(payload),
                                   'dropped': 0, 'incomplete_reason': None}, provenance='compiler_instrumentation')
    return report


def publish_coverage(run: Path):
    from ep_auditor.core.lineage import EvidenceLineage, normalized_record
    path = run / 'raw/coverage/report.json'
    if not path.is_file():
        return
    lineage = EvidenceLineage(run)
    report = json.loads(path.read_text(encoding='utf-8'))
    ref = lineage.reference('raw', 'coverage/report.json')
    report.update(eventType='coverage.source', required=True)
    row = normalized_record(report, {'coverage.source'}, ref, 0)
    if (run / 'metrics/coverage.json').is_file():
        if (lineage.verify('normalized', 'coverage.json')['records'] != [row]
                or lineage.verify('metrics', 'coverage.json')['records'] != [report]):
            raise EvidenceError('coverage projection differs from captured report')
        return
    lineage.write('normalized', 'coverage.json', producer_version='1', producer_source=Path(__file__),
                  inputs=[ref], records=[row])
    lineage.write('metrics', 'coverage.json', producer_version='1', producer_source=Path(__file__),
                  inputs=[lineage.reference('normalized', 'coverage.json')], records=[report])


def capture_coverage(run: Path, binaries: list[Path], *, scope: dict):
    profiles = sorted((run / 'coverage-capture').glob('*.profraw'))
    if not profiles and not binaries:
        return
    try:
        collect_coverage(run, profiles, binaries, scope=scope)
    except (EvidenceError, OSError, ValueError, subprocess.SubprocessError) as error:
        payload = json.dumps({'availability': 'incomplete', 'error': str(error), 'scope': scope}).encode()
        RawEvidence(run).append('coverage/failure.json', payload, collector='llvm_source_coverage', version='1',
            start_ns=0, end_ns=0, health={'capacity': len(payload), 'observed': len(payload), 'dropped': 0,
                                        'incomplete_reason': 'coverage_capture_failed'})
