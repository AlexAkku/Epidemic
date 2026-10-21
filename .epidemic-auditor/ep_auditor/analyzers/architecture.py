"""Source dependency candidates and PDB-bound observed runtime architecture.

Lexical analyses are explicitly heuristic: preprocessor/type semantics require
the compiler's checks. Non-observation is never a dead-code proof.
"""
from collections import Counter, defaultdict
import hashlib
from pathlib import Path
import re


def module_for(path: str, root: Path):
    try:
        parts = Path(path).resolve().relative_to(root.resolve()).parts
    except ValueError:
        return None
    if not parts:
        return None
    return '/'.join(parts[:2]) if len(parts) > 2 else parts[0] + '/*'


def cycles(edges):
    graph, result, done = defaultdict(set), [], set()
    for source, target in edges:
        graph[source].add(target)
    for start in sorted(set(graph) - done):
        active, stack = {start: 0}, [(start, iter(sorted(graph[start])))]
        while stack:
            node, children = stack[-1]
            child = next(children, None)
            if child is None:
                done.add(node)
                active.pop(node)
                stack.pop()
            elif child in active:
                result.append([entry[0] for entry in stack[active[child]:]] + [child])
            elif child not in done:
                active[child] = len(stack)
                stack.append((child, iter(sorted(graph[child]))))
    return result


def static_architecture(paths: list[Path], root: Path):
    root = root.resolve()
    def source_name(path):
        absolute = path.resolve()
        if absolute.is_relative_to(root):
            return absolute.relative_to(root).as_posix()
        return '@external/' + absolute.as_posix()

    # Only explicitly inventoried external files are owned. Resolved SDK/STL
    # symbols outside this inventory remain external in the runtime graph.
    inventory = {source_name(path): path for path in paths}
    source_modules = {str(path.resolve()): module_for(str(path), root) or
                      '@external/' + path.resolve().parent.as_posix() + '/*'
                      for path in inventory.values()}
    modules = {name: source_modules[str(path.resolve())] for name, path in inventory.items()}
    includes, unresolved, namespaces, classes, apis, functions, ownership = [], [], {}, {}, [], [], []
    bodies, references = defaultdict(list), {}
    for name, path in sorted(inventory.items()):
        text = path.read_text(encoding='utf-8', errors='replace')
        for match in re.finditer(r'^\s*#\s*include\s*([<"])([^>"\n]+)', text, re.MULTILINE):
            target = match[2].replace('\\', '/')
            relative = source_name(path.parent / target)
            candidates = [relative] if relative in inventory else [file for file in inventory if file.endswith('/' + target) or file == target]
            if len(candidates) == 1:
                includes.append((name, candidates[0]))
            elif match[1] == '"' or target.startswith('Epidemic/'):
                unresolved.append({'source': name, 'include': target, 'candidates': candidates})
        # Remove comments/literals while preserving positions and line numbers.
        clean = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                       lambda match: re.sub(r'[^\n]', ' ', match[0]), text, flags=re.DOTALL)
        namespaces[name] = sorted(set(re.findall(r'\bnamespace\s+([\w:]+)', clean)))
        classes[name] = [{'name': match[1], 'bases': match[2] or ''} for match in
                         re.finditer(r'\b(?:class|struct)\s+(\w+)\s*(?::([^{};]+))?\s*\{', clean)]
        references[name] = set(re.findall(r'\b\w+\b', clean))
        if path.suffix in {'.h', '.hpp', '.hh', '.ixx', '.cppm'}:
            apis.extend({'file': name, 'name': match[1], 'line': clean.count('\n', 0, match.start()) + 1}
                        for match in re.finditer(r'\b([\w:~]+)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?[;{]', clean)
                        if match[1] not in {'if', 'while', 'switch', 'for', 'catch', 'sizeof'})
        for match in re.finditer(r'\b([\w:~]+)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?\{', clean):
            if match[1] in {'if', 'while', 'switch', 'for', 'catch'}:
                continue
            start, depth, end = match.end(), 1, match.end()
            while depth and end < len(clean):
                depth += (clean[end] == '{') - (clean[end] == '}')
                end += 1
            body = clean[start:end - 1]
            row = {'file': name, 'name': match[1], 'line': clean.count('\n', 0, match.start()) + 1,
                   'lines': body.count('\n') + 1, 'complexity': 1 + len(re.findall(r'\b(?:if|for|while|case|catch)\b|&&|\|\||\?', body))}
            functions.append(row)
            tokens = re.findall(r'\w+|[^\s]', body)
            if len(tokens) >= 20:
                bodies[hashlib.sha256(' '.join(tokens).encode()).hexdigest()].append(row)
        for match in re.finditer(r'\bnew\b|\bdelete\b|\breturn\s*&\s*\w+|\bshared_ptr\s*<', clean):
            ownership.append({'file': name, 'line': clean.count('\n', 0, match.start()) + 1,
                              'pattern': match[0], 'confidence': 'heuristic'})
    module_edges = sorted({(modules[source], modules[target])
                           for source, target in includes if modules[source] != modules[target]})
    namespace_edges = sorted({(left, right) for source, target in includes for left in namespaces[source]
                              for right in namespaces[target] if left != right})
    class_edges = [{'source': name, 'target': other_file, 'class': declaration['name']}
                   for name, words in references.items() for other_file, declarations in classes.items()
                   if name != other_file for declaration in declarations if declaration['name'] in words]
    api_files = {row['file'] for row in apis}
    return {'scope': 'lexical source inventory; conditional includes/type resolution are not compiler-proven',
        'complete': False, 'provenance': 'static_analysis', 'confidence': 'heuristic',
        'source_modules': source_modules,
        'include_edges': [{'source': source, 'target': target} for source, target in includes],
        'edges': [{'source': source, 'target': target} for source, target in module_edges],
        'namespace_edges': [{'source': left, 'target': right} for left, right in namespace_edges],
        'classes': classes, 'namespaces': namespaces, 'class_edges': class_edges, 'public_apis': apis,
        'public_api_edges': [{'source': source, 'target': target} for source, target in includes if target in api_files],
        'include_cycles': cycles(includes), 'module_cycles': cycles(module_edges),
        'unresolved_includes': unresolved, 'functions': functions, 'large_functions': [row for row in functions if row['lines'] > 100],
        'ownership_candidates': ownership, 'duplicate_candidates': [rows for rows in bodies.values() if len(rows) > 1],
        'unused_candidates': 'requires covered binary/function inventory; unreferenced lexical names are not proof'}


def runtime_architecture(report: dict, root: Path, static: dict):
    symbols = {row['functionId']: row['symbol'] for row in report['functions']}
    edges, unknown, external = {}, 0, 0
    for call in report['calls']:
        if not call['caller']:
            continue
        sources = [symbols[identity].get('source_file') for identity in (call['caller'], call['functionId'])]
        if any(not source for source in sources):
            unknown += 1
            continue
        owned = static.get('source_modules', {})
        modules = [owned.get(str(Path(source).resolve()), module_for(source, root)) for source in sources]
        if any(module is None for module in modules):
            external += 1  # Resolved SDK/STL sources are outside the owned graph scope.
            continue
        key = tuple(modules)
        if key[0] == key[1]:
            continue
        row = edges.setdefault(key, {'source': key[0], 'target': key[1], 'calls': 0, 'cpu_100ns': 0, 'inclusive_ns': 0})
        row['calls'] += 1
        row['cpu_100ns'] += call['cpu_inclusive_100ns']
        row['inclusive_ns'] += call['inclusive_ns']
    allowed = {(row['source'], row['target']) for row in static['edges']}
    return {'edges': list(edges.values()), 'complete': report['complete'] and not unknown, 'unresolved_calls': unknown,
        'resolved_external_calls': external, 'scope': 'owned source modules; resolved external sources excluded explicitly',
        'unexpected_edge_candidates': [row for key, row in edges.items() if key not in allowed],
        'never_observed_static_edge_candidates': [row for row in static['edges'] if (row['source'], row['target']) not in edges],
        'comparison_policy': 'lexical include edges versus complete observed exact calls; unexpected/absent edges are candidates',
        'cost_policy': 'sum of inclusive callee CPU/wall intervals; nested interactions can overlap',
        'provenance': 'compiler_instrumentation', 'confidence': 'derived'}
