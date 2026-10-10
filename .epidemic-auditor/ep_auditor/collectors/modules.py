"""Owned native process/module/symbol/WCT snapshots with hash-bound raw evidence."""
import json
from pathlib import Path
import sys
import time
import io
import re

from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.identity import module_identity, function_identity
from ep_auditor.core.process import ManagedProcessRunner


def parse_pe_imports(payload: bytes) -> dict:
    """Bounded LLVM COFF inventory. Imports are static, never runtime calls."""
    if not isinstance(payload, bytes) or len(payload) > 16 * 1024 * 1024:
        raise EvidenceError('PE import decoder byte budget exceeded')
    try:
        source = payload.decode('utf-8', errors='strict')
    except UnicodeDecodeError as exc:
        raise EvidenceError('invalid PE import tool encoding') from exc
    if 'Format: COFF-x86-64' not in source and 'Format: COFF-i386' not in source:
        raise EvidenceError('unsupported PE import inventory format')
    imports, current = [], None
    for raw in io.StringIO(source):
        line = raw.strip()
        if line in {'Import {', 'DelayImport {'}:
            if current is not None or len(imports) >= 4096:
                raise EvidenceError('PE import descriptor budget/schema exceeded')
            current = {'kind': 'delay' if line.startswith('Delay') else 'normal', 'dll': None, 'symbols': []}
        elif current is not None and line.startswith('Name: '):
            name = line[6:]
            if current['dll'] is not None or not 1 <= len(name) <= 260:
                raise EvidenceError('invalid PE import DLL')
            current['dll'] = name
        elif current is not None and line.startswith('Symbol: '):
            match = re.fullmatch(r'Symbol: (.*?)\s*\((\d+)\)', line)
            if not match or len(match[1]) > 512 or len(current['symbols']) >= 65536:
                raise EvidenceError('invalid PE import symbol/budget')
            ordinal = int(match[2])
            if ordinal > 65535:
                raise EvidenceError('invalid PE import ordinal/hint')
            current['symbols'].append({'name': match[1] or None, 'ordinal_or_hint': ordinal})
        elif current is not None and line == '}':
            if not current['dll']:
                raise EvidenceError('PE import descriptor lacks DLL')
            imports.append(current)
            current = None
    if current is not None:
        raise EvidenceError('truncated PE import inventory')
    return {'schemaVersion': 1, 'imports': imports, 'scope': 'static PE imports; delay imports remain distinct',
            'runtime_calls_observed': False, 'provenance': 'external', 'confidence': 'derived'}


def capture_pe_imports(config, run_dir: Path, binary: Path, *, tool: Path | None = None):
    binary = binary.resolve(strict=True)
    tool = tool or Path(r'C:\Program Files\LLVM\bin\llvm-readobj.exe')
    if not tool.is_file() or binary.stat().st_size > 512 * 1024 * 1024:
        raise EvidenceError('PE import tool/binary unavailable or over capacity')
    before = (digest(binary), digest(tool))
    result = ManagedProcessRunner(config).run([str(tool), '--coff-imports', str(binary)],
                                             binary.parent, run_dir, 'Fast', timeout=30)
    if result.exit_code != 0 or before != (digest(binary), digest(tool)):
        raise EvidenceError('PE import tool failed or inputs changed')
    raw = RawEvidence(run_dir)
    data, captured = raw.read_capture('process/stdout.bin')
    if not captured['complete']:
        raise EvidenceError('PE import output incomplete')
    report = parse_pe_imports(data)
    binding = json.dumps({'binary_sha256': before[0], 'tool_sha256': before[1],
                          'stdout_sha256': captured['sha256'], 'binary': str(binary), 'tool': str(tool)}, sort_keys=True).encode()
    raw.append('modules/pe_import_binding.json', binding, collector='external_pe_import_inventory', version='1',
               start_ns=0, end_ns=0, health={'capacity': len(binding), 'observed': len(binding),
                                            'dropped': 0, 'incomplete_reason': None})
    return report

def _observer_execution_plan(
    binary: Path,
    arguments: list[str],
    verifier_fixture: bool
) -> tuple[str, int]:

    if type(verifier_fixture) is not bool:
        raise EvidenceError(
            'invalid verifier observation selection'
        )

    if not verifier_fixture:
        return '--exceptions', 25

    allowed = (
        ['verifier-good'],
        ['verifier-bad'],
        ['leak-good'],
        ['leak-bad'],
    )

    if (
        binary.name != 'EpidemicAuditorShadowFixture.exe'
        or arguments not in allowed
    ):
        raise EvidenceError(
            'extended verifier observation is restricted '
            'to the owned fixture'
        )

    return '--exceptions-verifier', 120

def capture_api_snapshot(
    config,
    run_dir: Path,
    binary: Path,
    arguments: list[str],
    *,
    helper: Path,
    capture_mode: str = 'Deep',
    verifier_fixture: bool = False,
    env_extra: dict[str, str] | None = None,
    runner=None,
    timeout_seconds: float | None = None
):
    """Own a bounded debuggee through the existing native helper and raw capture."""
    from ep_auditor.analyzers.diagnostics import analyze_api_snapshot
    binary, helper = binary.resolve(strict=True), helper.resolve(strict=True)
    if not isinstance(arguments, list) or any(
        not isinstance(arg, str) or '\x00' in arg
        for arg in arguments
    ):
        raise EvidenceError(
            'invalid native API observation arguments'
        )
    observer_mode, timeout = _observer_execution_plan(
        binary,
        arguments,
        verifier_fixture
    )

    before = (digest(binary), digest(helper))
    if timeout_seconds is not None:
        if type(timeout_seconds) not in (int, float) or not 0 < timeout_seconds <= timeout:
            raise EvidenceError('invalid shortened native observer timeout')
        timeout = timeout_seconds
    result = (runner or ManagedProcessRunner(config)).run(
        [str(helper), observer_mode, str(binary), *arguments],
        binary.parent,
        run_dir,
        capture_mode,
        timeout=timeout,
        env_extra=env_extra
    )
    if before != (digest(binary), digest(helper)):
        raise EvidenceError('native API inputs changed during capture')
    raw = RawEvidence(run_dir)
    data, captured = raw.read_capture('process/stdout.bin')
    if not captured['complete']:
        raise EvidenceError('native API stdout capture incomplete')
    snapshot = json.loads(data)
    if result.exit_code != 0:
        raise EvidenceError('native API capture incomplete: helper_exit=' + str(result.exit_code)
            + ', target_exit=' + str(snapshot.get('exit_code'))
            + ', api_complete=' + str(snapshot.get('api_entry_scope_complete'))
            + ', elapsed_qpc=' + str(snapshot.get('end_qpc', 0) - snapshot.get('begin_qpc', 0))
            + ', observer_deadline_exceeded=' + str(snapshot.get('deadline_exceeded'))
        )
    graph = analyze_api_snapshot(snapshot)
    if not graph['complete']:
        raise EvidenceError('native API entry scope incomplete')
    binding = json.dumps({'binary_sha256': before[0], 'helper_sha256': before[1],
                          'stdout_sha256': captured['sha256'], 'target_pid': snapshot['pid'],
                          'target_process_epoch': snapshot['process_epoch'], 'target_exit_code': snapshot['exit_code']},
                         sort_keys=True).encode()
    raw.append('modules/api_observer_binding.json', binding, collector='external_owned_debugger_api', version='1',
               start_ns=0, end_ns=0, health={'capacity': len(binding), 'observed': len(binding),
                                            'dropped': 0, 'incomplete_reason': None})
    return snapshot, graph


def publish_api_snapshot(run_dir: Path, *, verify_only: bool = False) -> dict:
    """Persist the API graph through the existing sealed-raw lineage contract."""
    from ep_auditor.analyzers.diagnostics import analyze_api_snapshot, analyze_exception_snapshot
    from ep_auditor.core.lineage import EvidenceLineage, normalized_record
    raw = RawEvidence(run_dir)
    raw.verify()
    binding = json.loads(raw.read_bytes('modules/api_observer_binding.json'))
    data, captured = raw.read_capture('process/stdout.bin')
    lifecycle = json.loads(raw.read_bytes('process/lifecycle.json'))
    if (not captured['complete'] or binding['stdout_sha256'] != captured['sha256']
            or binding['helper_sha256'] != lifecycle['image_sha256']):
        raise EvidenceError('API graph raw/helper binding mismatch')
    snapshot = json.loads(data)
    if (snapshot['pid'] != binding['target_pid'] or snapshot['process_epoch'] != binding['target_process_epoch']
            or snapshot['exit_code'] != binding['target_exit_code']):
        raise EvidenceError('API graph target identity/result binding mismatch')
    graph = analyze_api_snapshot(snapshot)
    lineage = EvidenceLineage(run_dir)
    inputs = [lineage.reference('raw', name) for name in ('process/stdout.bin','modules/api_observer_binding.json',
                                                       'process/lifecycle.json')]
    records = [normalized_record({'schemaVersion': 1, 'eventType': 'native.api.entry', 'required': True,
                  'provenance': 'external', 'confidence': 'observed', 'pid': snapshot['pid'],
                  'process_epoch': snapshot['process_epoch'], **call, 'raw_json_pointer': '/api_calls/' + str(index)},
                  {'native.api.entry'}, inputs[0], 0) for index, call in enumerate(snapshot['api_calls'])]
    if not records:
        records = [normalized_record({'schemaVersion': 1, 'eventType': 'native.api.scope', 'required': True,
                       'provenance': 'external', 'confidence': 'observed', 'observed_entries': 0,
                       'supported_exports': snapshot['api_supported_exports']}, {'native.api.scope'}, inputs[0], 0)]
    normal_path = 'modules/api_entries.json'
    expected_graph = [{'eventType': 'native.api.graph', 'required': True, **graph}]
    for layer, path, references, expected in (
            ('normalized', normal_path, inputs, records),
            ('graphs', 'modules/api_entry_graph.json', None, expected_graph),
            ('metrics', 'diagnostics/native_exceptions.json', inputs,
             [{'eventType': 'native.exceptions', 'required': True, **analyze_exception_snapshot(snapshot)}])):
        references = references or [lineage.reference('normalized', normal_path)]
        if (run_dir / layer / path).is_file():
            if (run_dir / layer / path).stat().st_size > 64 * 1024 * 1024:
                raise EvidenceError('API graph replay exceeds byte capacity')
            if lineage.verify(layer, path)['records'] != expected:
                raise EvidenceError('API graph replay differs from sealed physical capture')
        else:
            if verify_only:
                raise EvidenceError('required API graph surface missing')
            lineage.write(layer, path, producer_version='native-api-1', producer_source=Path(__file__),
                          inputs=references, records=expected)
    return graph


def identify_snapshot(snapshot, fingerprints):
    """Disk fingerprints are observed separately from the loaded memory image."""
    result = json.loads(json.dumps(snapshot, allow_nan=False))
    for module in result["modules"]:
        fingerprint = fingerprints.get(module.get("path"))
        module["binary_file_observation"] = fingerprint
        sha = fingerprint.get("sha256") if fingerprint else None
        module["moduleId"] = module_identity(sha) if sha else None
        module["identity_scope"] = "observed disk binary; loaded memory bytes not hashed"
    symbol = result["symbol"]
    address = symbol.get("function_address")
    owners = [module for module in result["modules"] if module.get("available")
              and address is not None and module["base"] <= address < module["base"] + module["size"]]
    symbol["moduleId"], symbol["functionId"], symbol["function_rva"] = None, None, None
    if len(owners) == 1:
        module = owners[0]
        symbol["function_rva"] = address - module["base"]
        if module["moduleId"]:
            symbol["moduleId"] = module["moduleId"]
            symbol["functionId"] = function_identity(module["binary_file_observation"]["sha256"], symbol["function_rva"])
    return result


def observe(config, run_dir: Path, pid: int, creation_filetime_100ns: int, *,
            address: int | None = None, thread_id: int | None = None, helper: Path | None = None):
    if type(pid) is not int or not 0 < pid <= 0xFFFFFFFF or type(creation_filetime_100ns) is not int or creation_filetime_100ns <= 0:
        raise EvidenceError("observation requires target PID and creation identity")
    helper = helper or Path(sys.executable).parent / "EpidemicAuditorObservation.exe"
    if not helper.is_file():
        raise EvidenceError("native observation helper unavailable")
    command = [str(helper.resolve()), "--pid", str(pid)]
    for option, value, limit in (("--address", address, 2**64 - 1), ("--thread", thread_id, 0xFFFFFFFF)):
        if value is not None:
            if type(value) is not int or not 0 <= value <= limit or (option == "--thread" and value == 0):
                raise EvidenceError("invalid observation address/thread")
            command += [option, str(value)]
    started = time.perf_counter_ns()
    helper_sha = digest(helper)
    result = ManagedProcessRunner(config).run(command, helper.parent, run_dir, "Fast", timeout=20)
    if result.exit_code != 0 or helper_sha != digest(helper):
        raise EvidenceError("native observation failed or helper changed")
    capture = RawEvidence(run_dir)
    stdout, raw = capture.read_capture("process/stdout.bin")
    if not raw["complete"]:
        raise EvidenceError("native observation stdout incomplete")
    snapshot = json.loads(stdout.decode("utf-8"))
    if snapshot["pid"] != pid or snapshot["creation_filetime_100ns"] != creation_filetime_100ns:
        raise EvidenceError("target process identity changed")
    fingerprints = {}
    for module in snapshot["modules"]:
        if not module.get("available"):
            continue
        path = Path(module["path"])
        try:
            before = path.stat()
            if before.st_size > 512 * 1024 * 1024:
                raise OSError("binary exceeds observation byte budget")
            sha = digest(path)
            after = path.stat()
            if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
                raise OSError("binary changed during observation")
            fingerprints[str(path)] = {"sha256": sha, "size": after.st_size, "provenance": "external", "confidence": "observed"}
        except OSError as error:
            fingerprints[str(path)] = {"sha256": None, "unavailable_reason": str(error)}
    payload = json.dumps({"helper_sha256": helper_sha, "snapshot_raw_sha256": raw["sha256"],
                          "fingerprints": fingerprints}, sort_keys=True).encode()
    capture.append("modules/binary_files.json", payload, collector="external_binary_files", version="1",
                   start_ns=started, end_ns=time.perf_counter_ns(), health={
                       "capacity": len(payload), "observed": len(payload), "dropped": 0, "incomplete_reason": None})
    return identify_snapshot(snapshot, fingerprints)
