"""Bounded compiler-proven declarations, visibility, typed class/call edges.

This is product analysis, not a qualification validator. A failed/partial parse
cannot supply a complete graph. Compiler arguments and source bytes are bound.
"""
import hashlib
import json
from pathlib import Path
import shutil
import threading
import time
import os
import re
import ctypes
from ctypes import wintypes

from ep_auditor.core.evidence import EvidenceError, digest, atomic_json
from ep_auditor.core.owned_process import OwnedProcess


def analyze_translation_unit(root: Path, source: Path, arguments: list[str], *, symbol_filter: str,
                             directory: Path | None = None):
    root, source = root.resolve(), source.resolve()
    compiler = shutil.which('clang-cl')
    if compiler is None or not source.is_relative_to(root):
        raise EvidenceError('semantic analysis requires local clang-cl, owned source and explicit symbol scope')
    before, compiler_sha = digest(source), digest(Path(compiler))
    command = [compiler, *arguments, '/Zs', '/TP', '/clang:-Xclang', '/clang:-ast-dump=json',
               '/clang:-Xclang', '/clang:-ast-dump-filter=' + symbol_filter, str(source)]
    output, errors = bytearray(), bytearray()
    process = OwnedProcess(command, str(directory or root), os.environ.copy())
    exceeded = threading.Event()
    def drain(stream, target, capacity):
        while data := stream.read(65536):
            if len(target) + len(data) > capacity:
                exceeded.set()
                process.cancel('compiler_ast_output_capacity_exceeded')
                return
            target.extend(data)
    readers = [threading.Thread(target=drain, args=(process.stdout, output, 32 * 1024**2)),
               threading.Thread(target=drain, args=(process.stderr, errors, 1024**2))]
    try:
        for reader in readers:
            reader.start()
        deadline = time.monotonic() + 90
        while process.poll() is None:
            if time.monotonic() >= deadline:
                process.cancel('compiler_ast_timeout')
                break
            time.sleep(.05)
        for reader in readers:
            reader.join()
        if process.poll() != 0 or exceeded.is_set():
            raise EvidenceError('compiler AST incomplete: ' + errors.decode(errors='replace')[:1200])
    finally:
        process.close()
    if before != digest(source) or compiler_sha != digest(Path(compiler)):
        raise EvidenceError('compiler/source changed during semantic analysis')
    text = output.decode('utf-8', errors='strict')
    decoder, offset, roots = json.JSONDecoder(), 0, []
    while offset < len(text):
        if text[offset].isspace():
            offset += 1
            continue
        node, offset = decoder.raw_decode(text, offset)
        roots.append(node)
    if not roots:
        raise EvidenceError('compiler returned no declarations for the requested scope')
    symbols, contexts, references, declaration_visibility = {}, {}, [], {}
    def owned(file):
        return file is not None and Path(file).resolve().is_relative_to(root)
    def file_of(node, inherited):
        location = node.get('loc', {})
        location = location.get('expansionLoc', location)
        return location.get('file', inherited)
    files = set()
    def collect_files(node, inherited=str(source)):
        file = file_of(node, inherited)
        if owned(file):
            files.add(Path(file).resolve().relative_to(root).as_posix())
        for child in node.get('inner', []):
            collect_files(child, file)
    for node in roots:
        collect_files(node)
    has_public_layout = any('/include/' in ('/' + file.casefold()) or '/public/' in ('/' + file.casefold()) for file in files)
    def visit(node, scope=(), inherited=str(source), exposed=True, caller=None, callee=False):
        kind, name = node.get('kind'), node.get('name')
        file = file_of(node, inherited)
        if kind == 'NamespaceDecl':
            scope = scope + (name or ('anonymous@' + str(file)),)
            contexts[node.get('id')] = scope
        context = contexts.get(node.get('parentDeclContextId'), scope)
        qualified = '::'.join((*context, name)) if name else None
        classes = {'CXXRecordDecl', 'RecordDecl'}
        functions = {'FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl', 'CXXConversionDecl'}
        relevant = classes | functions | {'FieldDecl', 'VarDecl', 'EnumDecl', 'TypeAliasDecl', 'TypedefDecl'}
        symbol_id = None
        if kind in relevant and name and not node.get('isImplicit') and owned(file):
            relative = Path(file).resolve().relative_to(root).as_posix()
            signature = node.get('type', {}).get('desugaredQualType', node.get('type', {}).get('qualType', ''))
            local_context = (relative if kind == 'FunctionDecl' and node.get('storageClass') == 'static'
                             else caller if not node.get('mangledName') and kind not in functions else None)
            key = (kind, qualified, node.get('mangledName') or signature, local_context)
            symbol_id = 'SourceSymbolId:' + hashlib.sha256(json.dumps(key).encode()).hexdigest()
            public_header = (Path(file).suffix.lower() in {'.h', '.hpp', '.hh', '.hxx'}
                and (not has_public_layout or any(part.casefold() in {'include', 'public'} for part in Path(relative).parts)))
            exported = any(child.get('kind') == 'DLLExportAttr' for child in node.get('inner', []))
            visibility = declaration_visibility.get(node.get('previousDecl'), exposed)
            declaration_visibility[node.get('id')] = visibility
            public = visibility and caller is None and (public_header or exported) and not any(part.startswith('anonymous@') or part == 'detail' for part in context)
            row = symbols.setdefault(symbol_id, {'symbolId': symbol_id, 'kind': kind, 'qualified_name': qualified,
                'type': signature, 'owner_scope': '::'.join(context), 'files': [], 'public_api': False, 'bases': [], 'definition': False,
                'provenance': 'static_analysis', 'confidence': 'observed'})
            if relative not in row['files']:
                row['files'].append(relative)
            row['public_api'] |= public
            row['definition'] |= bool(node.get('completeDefinition') or any(child.get('kind') == 'CompoundStmt' for child in node.get('inner', [])))
            row['bases'].extend(base['type'].get('desugaredQualType', base['type']['qualType']) for base in node.get('bases', []))
            contexts[node.get('id')] = context + (name,) if kind in classes else context
            if kind in functions:
                caller = symbol_id
        elif kind in functions:
            caller = None  # Never attribute an unselected nested function to its enclosing caller.
        reference = node.get('referencedDecl', {}).get('id') or node.get('referencedMemberDecl')
        if caller and reference and callee:
            references.append((caller, reference))
        node['_source_symbol_id'] = symbol_id
        if kind in classes:
            scope = context + (name,) if name else scope
            public_access = node.get('tagUsed') != 'class'
            for child in node.get('inner', []):
                if child.get('kind') == 'AccessSpecDecl':
                    public_access = child['access'] == 'public'
                else:
                    visit(child, scope, file, exposed and public_access, caller)
        else:
            for index, child in enumerate(node.get('inner', [])):
                target = index == 0 if kind in {'CallExpr', 'CXXMemberCallExpr', 'CXXOperatorCallExpr'} else callee
                visit(child, scope, file, exposed, caller, target)
    for node in roots:
        visit(node)
    local_ids = {}
    def map_ids(node):
        if node.get('_source_symbol_id') and node.get('kind') in {
                'FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl', 'CXXConversionDecl'}:
            local_ids[node['id']] = node['_source_symbol_id']
        for child in node.get('inner', []):
            map_ids(child)
    for node in roots:
        map_ids(node)
    call_edges = sorted({(left, local_ids[right]) for left, right in references if right in local_ids})
    records = list(symbols.values())
    class_edges = set()
    class_names = {row['qualified_name']: row['symbolId'] for row in records if row['kind'] in {'CXXRecordDecl', 'RecordDecl'}}
    for row in records:
        types = [row['type'], *row['bases']]
        for type in types:
            for token in re.findall(r'(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*', type):
                prefixes = row['owner_scope'].split('::') if row['owner_scope'] else []
                candidates = ['::'.join((*prefixes[:length], token)) for length in range(len(prefixes), -1, -1)]
                name = next((candidate for candidate in candidates if candidate in class_names), None)
                if name is not None:
                    source_id = class_names.get(row['owner_scope'], row['symbolId'])
                    if source_id != class_names[name]:
                        class_edges.add((source_id, class_names[name]))
    return {'schemaVersion': 1, 'complete': True, 'scope': {'source': str(source), 'symbol_filter': symbol_filter},
        'public_header_policy': 'owned include/Public trees when present, otherwise owned headers; private/protected/detail excluded',
        'call_policy': 'compiler-resolved CallExpr/member/operator targets; address-taking is not a call; constructor/destructor and indirect/virtual runtime dispatch require instrumentation',
        'compiler_sha256': compiler_sha, 'source_sha256': before, 'command': command,
        'symbols': records, 'class_edges': [{'source': source, 'target': target} for source, target in sorted(class_edges)],
        'call_edges': [{'source': left, 'target': right} for left, right in call_edges],
        'provenance': 'static_analysis', 'confidence': 'observed'}


def compilation_arguments(command: str):
    """Use Windows' actual quoting rules; shlex cannot decode CMake CL commands."""
    shell, kernel = ctypes.WinDLL('shell32', use_last_error=True), ctypes.WinDLL('kernel32', use_last_error=True)
    shell.CommandLineToArgvW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int)]
    shell.CommandLineToArgvW.restype = ctypes.POINTER(wintypes.LPWSTR)
    kernel.LocalFree.argtypes, kernel.LocalFree.restype = [ctypes.c_void_p], ctypes.c_void_p
    count = ctypes.c_int()
    pointer = shell.CommandLineToArgvW(command, ctypes.byref(count))
    if not pointer:
        raise EvidenceError('invalid compilation command quoting')
    try:
        return [pointer[index] for index in range(count.value)]
    finally:
        kernel.LocalFree(pointer)


def analyze_compilation_database(root: Path, database: Path, source_paths: list[Path], *, symbol_filters: list[str],
                                 source_fingerprints: dict | None = None):
    if not database.is_file() or database.stat().st_size > 32 * 1024**2:
        return {'complete': False, 'availability': 'unavailable', 'reason': 'compilation database missing/oversized'}
    wanted = {str(path.resolve()).casefold() for path in source_paths}
    database_before = digest(database)
    entries = [row for row in json.loads(database.read_text(encoding='utf-8'))
               if str(Path(row['file']).resolve()).casefold() in wanted]
    if not entries:
        return {'complete': False, 'availability': 'unavailable', 'reason': 'no owned translation units in compilation database'}
    compiler = shutil.which('clang-cl')
    if not compiler:
        return {'complete': False, 'availability': 'unavailable', 'reason': 'clang-cl missing'}
    key = hashlib.sha256(json.dumps({'database': database_before, 'compiler': digest(Path(compiler)),
        'sources': source_fingerprints, 'translation_units': sorted(row['file'] for row in entries),
        'filters': symbol_filters, 'analyzer': digest(Path(__file__))}, sort_keys=True).encode()).hexdigest()
    cache = root / '.ai/auditor/cache/semantic-source.json'
    if source_fingerprints is not None and cache.is_file() and cache.stat().st_size <= 64 * 1024**2:
        try:
            cached = json.loads(cache.read_text(encoding='utf-8'))
            if cached['key'] == key and cached['graph_sha256'] == hashlib.sha256(json.dumps(cached['graph'], sort_keys=True).encode()).hexdigest():
                return cached['graph']
        except (ValueError, KeyError, OSError):
            pass
    graphs, failures, deadline = [], [], time.monotonic() + 900
    for entry in entries:
        if time.monotonic() >= deadline:
            failures.append({'source': entry['file'], 'reason': 'project semantic analysis time budget exceeded'})
            break
        values = entry.get('arguments') or compilation_arguments(entry['command'])
        arguments, skip = [], False
        for argument in values[1:]:
            if skip:
                skip = False
                continue
            if argument in {'-o', '/Fo', '/Fd', '/Fp'}:
                skip = True
            elif (argument not in {'-c', '/c', '--', '/Gh', '/GH'}
                  and not argument.casefold().startswith(('/fo', '/fd', '/fp', '/yc', '/yu'))
                  and str(Path(argument).resolve()).casefold() != str(Path(entry['file']).resolve()).casefold()):
                arguments.append(argument)
        for filter in symbol_filters or ['']:
            try:
                graphs.append(analyze_translation_unit(root, Path(entry['file']), arguments,
                    symbol_filter=filter, directory=Path(entry['directory'])))
            except (EvidenceError, OSError, ValueError) as error:
                failures.append({'source': entry['file'], 'filter': filter, 'reason': str(error)[:1200]})
    symbols, class_edges, call_edges = {}, set(), set()
    for graph in graphs:
        for row in graph['symbols']:
            if row['symbolId'] in symbols:
                existing = symbols[row['symbolId']]
                existing['files'] = sorted(set(existing['files'] + row['files']))
                existing['public_api'] |= row['public_api']
                existing['definition'] |= row['definition']
            else:
                symbols[row['symbolId']] = row
        class_edges.update((row['source'], row['target']) for row in graph['class_edges'])
        call_edges.update((row['source'], row['target']) for row in graph['call_edges'])
    result = {'complete': not failures, 'availability': 'observed' if not failures else 'partial',
        'scope': 'owned translation units and explicit namespace filters from this compilation database',
        'database_sha256': digest(database), 'translation_units': len(entries), 'parsed_scopes': len(graphs),
        'failures': failures, 'symbols': list(symbols.values()),
        'class_edges': [{'source': left, 'target': right} for left, right in sorted(class_edges)],
        'call_edges': [{'source': left, 'target': right} for left, right in sorted(call_edges)],
        'provenance': 'static_analysis', 'confidence': 'observed'}
    if digest(database) != database_before:
        raise EvidenceError('compilation database changed during semantic analysis')
    if source_fingerprints is not None and result['complete']:
        atomic_json(cache, {'key': key, 'graph': result,
                           'graph_sha256': hashlib.sha256(json.dumps(result, sort_keys=True).encode()).hexdigest()})
    return result
