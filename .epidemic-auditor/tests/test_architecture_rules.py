"""Real source good/bad graphs and explicitly declared rule observations."""
import json
from pathlib import Path
import tempfile
import unittest

from ep_auditor.analyzers.architecture import static_architecture, runtime_architecture
from ep_auditor.analyzers.rules import load_rules, evaluate_rules, DOMAINS
from ep_auditor.core.evidence import EvidenceError

ROOT = Path(__file__).resolve().parents[2]


class ArchitectureRulesTests(unittest.TestCase):
    def test_real_include_cycle_forbidden_layer_and_allowed_control(self):
        with tempfile.TemporaryDirectory(dir=ROOT / '.ai/tmp') as temporary:
            root = Path(temporary)
            left, right = root / 'EngineBase/Kernel/a.h', root / 'EngineRuntime/World/b.h'
            left.parent.mkdir(parents=True)
            right.parent.mkdir(parents=True)
            left.write_text('#include "../../EngineRuntime/World/b.h"\nnamespace core { class Owner {}; int Public(); }')
            right.write_text('#include "../../EngineBase/Kernel/a.h"\nnamespace runtime { class Child : core::Owner {}; }')
            graph = static_architecture([left, right], root)
            self.assertTrue(graph['include_cycles'])
            self.assertTrue(graph['module_cycles'])
            self.assertTrue(graph['namespace_edges'])
            self.assertTrue(graph['class_edges'])
            self.assertTrue(graph['public_api_edges'])
            rules = load_rules(ROOT / '.epidemic-auditor/rules')
            outcome = evaluate_rules(rules, {'architecture': graph})
            self.assertEqual(outcome['status'], 'FAIL')
            self.assertEqual(next(row['status'] for row in outcome['findings'] if row['rule_id'] == 'base-no-runtime'), 'FAIL')
            left.write_text('namespace core { class Owner {}; int Public(); }')
            graph = static_architecture([left, right], root)
            self.assertFalse(graph['include_cycles'])
            self.assertFalse(graph['module_cycles'])
            outcome = evaluate_rules(rules, {'architecture': graph})
            self.assertNotIn('FAIL', {row['status'] for row in outcome['findings']})
            self.assertEqual(outcome['status'], 'UNKNOWN', 'lexical source scan is not a compiler-complete proof')

    def test_runtime_graph_count_cost_unexpected_and_allowed_controls(self):
        root = Path('E:/Epidemic')
        report = {'complete': True, 'functions': [
            {'functionId': 'caller', 'symbol': {'source_file': str(root / 'EngineBase/Kernel/a.cpp')}},
            {'functionId': 'callee', 'symbol': {'source_file': str(root / 'EngineRuntime/World/b.cpp')}}],
            'calls': [{'caller': 'caller', 'functionId': 'callee', 'cpu_inclusive_100ns': 17, 'inclusive_ns': 123}]}
        graph = runtime_architecture(report, root, {'edges': []})
        self.assertEqual(graph['edges'][0]['calls'], 1)
        self.assertEqual(graph['edges'][0]['cpu_100ns'], 17)
        self.assertTrue(graph['unexpected_edge_candidates'])
        self.assertFalse(runtime_architecture(report, root, {'edges': graph['edges']})['unexpected_edge_candidates'])
        rules = load_rules(ROOT / '.epidemic-auditor/rules')
        self.assertEqual(evaluate_rules(rules, {'architecture': graph})['status'], 'FAIL')

    def test_all_six_domains_missing_invalid_positive_negative(self):
        rules = load_rules(ROOT / '.epidemic-auditor/rules')
        self.assertEqual(evaluate_rules(rules, {})['status'], 'UNKNOWN')
        facts = {'architecture': {'edges': [], 'complete': True}}
        for rule in rules:
            if rule['kind'] == 'max_metric':
                facts[rule['domain']] = {rule['metric']: {'value': 0, 'complete': True, 'fixture': 'declared control'}}
        self.assertEqual(evaluate_rules(rules, facts)['status'], 'PASS')
        for rule in rules:
            if rule['kind'] == 'max_metric':
                value = facts[rule['domain']][rule['metric']]
                value['value'] = 1
                self.assertEqual(evaluate_rules(rules, facts)['status'], 'FAIL')
                value['value'] = 0
        with tempfile.TemporaryDirectory(dir=ROOT / '.ai/tmp') as temporary:
            directory = Path(temporary)
            for domain in DOMAINS:
                (directory / (domain + '.yaml')).write_text((ROOT / '.epidemic-auditor/rules' / (domain + '.yaml')).read_text())
            (directory / 'io.yaml').write_text('{"schemaVersion":1,"schemaVersion":1,"domain":"io","rules":[]}')
            with self.assertRaises(EvidenceError): load_rules(directory)
