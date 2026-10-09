"""Attach bounded sanitizer *text candidates* to independently captured stderr."""
from __future__ import annotations

from pathlib import Path
import json
import os
import subprocess

from ep_auditor.analyzers.sanitizer import CAPACITY, parse_stderr, parse_verifier_xml
from ep_auditor.core.evidence import RawEvidence, EvidenceError
from ep_auditor.core.lineage import EvidenceLineage, normalized_record


def capture_verifier_fixture(config, run: Path, binary: Path, variant: str, *, helper: Path) -> dict:
    """Explicit, bounded Heaps/Leak controls; never activate arbitrary image names."""
    if os.name != 'nt' or binary.name != 'EpidemicAuditorShadowFixture.exe' or variant not in {
            'verifier-good', 'verifier-bad', 'leak-good', 'leak-bad'}:
        raise EvidenceError('unsupported explicit verifier fixture scope')
    import winreg
    from ep_auditor.collectors.modules import capture_api_snapshot
    tool = Path(os.environ['SystemRoot']) / 'System32/appverif.exe'
    if not tool.is_file():
        raise EvidenceError('Application Verifier unavailable; profile not executed')
    key = r'SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options' + '\\' + binary.name
    def neutral():
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key, 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as handle:
                return winreg.QueryInfoKey(handle)[1] == 0
        except FileNotFoundError:
            return True
    if not neutral():
        raise EvidenceError('refuse preexisting verifier fixture settings')
    export = (run / 'verifier-export.xml').resolve()
    def command(*args):
        # appverif has a requireAdministrator manifest: use the explicit narrow
        # ShellExecute recipe, never elevate the Python/UI process itself.
        quote = lambda value: "'" + str(value).replace("'", "''") + "'"
        script = ("$ErrorActionPreference='Stop'; $p=Start-Process -FilePath " + quote(tool)
                  + ' -ArgumentList ' + quote(subprocess.list2cmdline(list(args)))
                  + ' -Wait -PassThru -WindowStyle Hidden; exit $p.ExitCode')
        powershell = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
        result = subprocess.run([str(powershell), '-NoProfile', '-NonInteractive', '-Command', script],
                                capture_output=True, timeout=60,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        if result.returncode != 0:
            raise EvidenceError('Application Verifier command failed: ' + str(result.returncode))
    try:
        # Keep export ownership with this caller; the privileged tool truncates
        # an existing owned file rather than creating an administrator-only one.
        with export.open('xb'):
            pass
        command('-enable', 'Heaps', 'Leak', '-for', binary.name)
        snapshot, _ = capture_api_snapshot(config, run, binary, [variant], helper=helper)
        if not snapshot['verifier_modules']:
            raise EvidenceError('Application Verifier activation unobserved')
        command('-export', 'log', '-for', binary.name, '-with', 'To=' + str(export))
        if not export.is_file() or export.stat().st_size > CAPACITY:
            raise EvidenceError('Application Verifier export unavailable or oversized')
        data = export.read_bytes()
        report = parse_verifier_xml(data, expected_pid=snapshot['pid'])
        raw = RawEvidence(run)
        raw.append('diagnostics/verifier.xml', data, collector='external_application_verifier', version='1',
                   start_ns=0, end_ns=0, health={'capacity': len(data), 'observed': len(data),
                                               'dropped': 0, 'incomplete_reason': None})
        return report
    finally:
        if not neutral():
            command('-delete', 'settings', '-for', binary.name)
        if not neutral():
            raise EvidenceError('owned Application Verifier settings not cleaned')
        export.unlink(missing_ok=True)


def publish_sanitizer(run: Path) -> None:
    raw = RawEvidence(run)
    manifest = raw.verify()
    if 'raw/diagnostics/verifier.xml' in {row['path'] for row in manifest['raw_files']}:
        publish_verifier(run)
    if 'raw/process/stderr.bin' not in {row['path'] for row in manifest['raw_files']}:
        return
    lineage = EvidenceLineage(run)
    ref = lineage.reference('raw', 'process/stderr.bin')
    source = run / 'raw/process/stderr.bin'
    if source.stat().st_size > CAPACITY:
        # Do not convert a partial/oversized stderr into a clean sanitizer verdict.
        return
    report = parse_stderr(raw.read_bytes('process/stderr.bin'))
    record = normalized_record({'schemaVersion': 1, 'eventType': 'diagnostics.sanitizer_stderr',
                                'required': True, **report}, {'diagnostics.sanitizer_stderr'}, ref, 0)
    destination = run / 'normalized/diagnostics/sanitizer_stderr.json'
    if destination.is_file():
        lineage.verify('normalized', 'diagnostics/sanitizer_stderr.json')
        return
    lineage.write('normalized', 'diagnostics/sanitizer_stderr.json', producer_version='sanitizer-stderr-1',
                  producer_source=Path(__file__), inputs=[ref], records=[record])


def publish_verifier(run: Path, *, verify_only: bool = False) -> None:
    raw = RawEvidence(run)
    raw.verify()
    binding = json.loads(raw.read_bytes('modules/api_observer_binding.json'))
    snapshot = json.loads(raw.read_capture('process/stdout.bin')[0])
    if snapshot['pid'] != binding['target_pid'] or not snapshot['verifier_modules']:
        raise EvidenceError('verifier activation/target binding missing')
    report = parse_verifier_xml(raw.read_bytes('diagnostics/verifier.xml'), expected_pid=binding['target_pid'])
    report['activation_verified'] = True
    lineage = EvidenceLineage(run)
    inputs = [lineage.reference('raw', path) for path in (
        'diagnostics/verifier.xml', 'modules/api_observer_binding.json', 'process/stdout.bin')]
    records = [normalized_record({'eventType': 'diagnostics.verifier', 'required': True, **report},
                                 {'diagnostics.verifier'}, inputs[0], 0)]
    path = 'diagnostics/verifier.json'
    if (run / 'normalized' / path).is_file():
        if lineage.verify('normalized', path)['records'] != records:
            raise EvidenceError('verifier replay differs from sealed capture')
    else:
        if verify_only:
            raise EvidenceError('required verifier result missing')
        lineage.write('normalized', path, producer_version='verifier-1', producer_source=Path(__file__),
                      inputs=inputs, records=records)
