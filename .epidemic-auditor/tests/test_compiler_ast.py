"""Compiler-derived C++ visibility/types and rejected invalid source, no mocks."""
from pathlib import Path
import tempfile
import unittest
import json
import shutil

from ep_auditor.analyzers.compiler_ast import analyze_translation_unit, analyze_compilation_database
from ep_auditor.core.evidence import EvidenceError, digest

ROOT = Path(__file__).resolve().parents[2]


class CompilerAstTests(unittest.TestCase):
    def test_actual_cpp_visibility_dependencies_calls_and_conditional_source(self):
        with tempfile.TemporaryDirectory(dir=ROOT / '.ai/tmp') as temporary:
            root = Path(temporary)
            header, source = root / 'api.h', root / 'main.cpp'
            header.write_text('namespace semantic_fixture {\n'
                'struct Base {};\nclass Owner { void Hidden(); public: Base* value; void Visible(); };\n'
                'inline void Owner::Hidden() {}\ninline int InlineApi() { int secret = 7; return secret; }\n'
                'void Leaf(); void Caller();\n#ifdef BAD_EDGE\nstruct Conditional { Owner* dependency; };\n#endif\n}\n')
            source.write_text('#include "api.h"\nnamespace semantic_fixture { void Leaf() { int local = 1; (void)local; } '
                'void Caller() { int local = 2; (void)local; Leaf(); } '
                'int Recursive(int n) { return n ? Recursive(n - 1) : 0; } '
                'auto AddressOnly() { return &Leaf; } static int LocalHelper() { return 1; } }\n')
            graph = analyze_translation_unit(root, source, [], symbol_filter='semantic_fixture')
            self.assertTrue(graph['complete'])
            api = {row['qualified_name'] for row in graph['symbols'] if row['public_api']}
            self.assertIn('semantic_fixture::Owner::Visible', api)
            self.assertNotIn('semantic_fixture::Owner::Hidden', api)
            self.assertIn('semantic_fixture::InlineApi', api)
            self.assertNotIn('semantic_fixture::secret', api)
            locals = [row for row in graph['symbols'] if row['qualified_name'] == 'semantic_fixture::local']
            self.assertEqual(len(locals), 2, 'same-name locals in different functions must have distinct identities')
            self.assertNotIn('semantic_fixture::Conditional', {row['qualified_name'] for row in graph['symbols']})
            self.assertTrue(graph['class_edges'])
            self.assertTrue(graph['call_edges'])
            recursive = next(row['symbolId'] for row in graph['symbols']
                             if row['qualified_name'] == 'semantic_fixture::Recursive')
            self.assertIn({'source': recursive, 'target': recursive}, graph['call_edges'])
            address_only = next(row['symbolId'] for row in graph['symbols']
                                if row['qualified_name'] == 'semantic_fixture::AddressOnly')
            self.assertFalse(any(edge['source'] == address_only for edge in graph['call_edges']))
            other = root / 'other.cpp'
            other.write_text('namespace semantic_fixture { static int LocalHelper() { return 2; } }')
            second = analyze_translation_unit(root, other, [], symbol_filter='semantic_fixture')
            first_helper = next(row['symbolId'] for row in graph['symbols'] if row['qualified_name'].endswith('::LocalHelper'))
            second_helper = next(row['symbolId'] for row in second['symbols'] if row['qualified_name'].endswith('::LocalHelper'))
            self.assertNotEqual(first_helper, second_helper, 'internal-linkage functions in different files must not merge')
            bad = analyze_translation_unit(root, source, ['/DBAD_EDGE'], symbol_filter='semantic_fixture')
            self.assertIn('semantic_fixture::Conditional', {row['qualified_name'] for row in bad['symbols']})
            database = ROOT / 'build/auditor-boundary/compiler-ast/compile_commands.json'
            database.parent.mkdir(parents=True, exist_ok=True)
            database.write_text(json.dumps([{'directory': str(root), 'file': str(source),
                'command': '"' + shutil.which('clang-cl') + '" /nologo /Foignored.obj /Fdignored.pdb -c -- "' + str(source) + '"'}]))
            fingerprints = {str(file): digest(file) for file in (source, header)}
            project = analyze_compilation_database(root, database, [source], symbol_filters=['semantic_fixture'],
                                                   source_fingerprints=fingerprints)
            self.assertTrue(project['complete'], project.get('failures'))
            self.assertTrue(project['class_edges'])
            cache = root / '.ai/auditor/cache/semantic-source.json'
            stamp = cache.stat().st_mtime_ns
            reused = analyze_compilation_database(root, database, [source], symbol_filters=['semantic_fixture'],
                                                  source_fingerprints=fingerprints)
            self.assertEqual(reused, project)
            self.assertEqual(cache.stat().st_mtime_ns, stamp)
            source.write_text('namespace semantic_fixture { not_a_type broken; }')
            with self.assertRaises(EvidenceError): analyze_translation_unit(root, source, [], symbol_filter='semantic_fixture')
