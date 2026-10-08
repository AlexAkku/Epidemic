"""Auditor product rules; missing observations are UNKNOWN, never zero/PASS."""
import fnmatch
import json
import math
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, digest

DOMAINS = ('architecture', 'concurrency', 'persistence', 'performance', 'memory', 'io')


def _unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise EvidenceError('duplicate rule key: ' + key)
        result[key] = value
    return result


def load_rules(directory: Path):
    rules, ids = [], set()
    for domain in DOMAINS:
        path = directory / (domain + '.yaml')
        if not path.is_file() or path.stat().st_size > 1024**2:
            raise EvidenceError('missing/oversized rule configuration: ' + str(path))
        # The shipped schema deliberately uses YAML's JSON subset; no executable tags.
        try:
            value = json.loads(path.read_text(encoding='utf-8'), object_pairs_hook=_unique)
        except ValueError as error:
            raise EvidenceError('rules require the documented YAML JSON subset') from error
        if (not isinstance(value, dict) or set(value) != {'schemaVersion', 'domain', 'rules'}
                or type(value['schemaVersion']) is not int or value['schemaVersion'] != 1 or value['domain'] != domain):
            raise EvidenceError('unsupported rule schema/domain')
        if not isinstance(value['rules'], list) or not value['rules']:
            raise EvidenceError('rule domain has no executable rules')
        for rule in value['rules']:
            if not isinstance(rule, dict):
                raise EvidenceError('rule must be an object')
            kind = rule.get('kind')
            keys = {'forbidden_edge': {'id', 'kind', 'source', 'target'},
                    'max_metric': {'id', 'kind', 'metric', 'maximum'}}.get(kind)
            if keys is None or set(rule) != keys or not isinstance(rule['id'], str) or not rule['id'] or rule['id'] in ids:
                raise EvidenceError('unknown/duplicate/malformed rule')
            ids.add(rule['id'])
            if kind == 'max_metric':
                if (not isinstance(rule['metric'], str) or not rule['metric'] or type(rule['maximum']) not in (int, float)
                        or not math.isfinite(rule['maximum']) or rule['maximum'] < 0):
                    raise EvidenceError('invalid rule metric/limit')
            elif any(not isinstance(rule[key], str) or not rule[key] for key in ('source', 'target')):
                raise EvidenceError('invalid rule edge pattern')
            rules.append(dict(rule, domain=domain, config_sha256=digest(path)))
    return rules


def evaluate_rules(rules: list[dict], facts: dict):
    findings = []
    for rule in rules:
        status, evidence = 'UNKNOWN', []
        if rule['kind'] == 'forbidden_edge':
            graph = facts.get('architecture')
            if graph is not None:
                evidence = [edge for edge in graph['edges'] if fnmatch.fnmatchcase(edge['source'], rule['source'])
                            and fnmatch.fnmatchcase(edge['target'], rule['target'])]
                status = 'FAIL' if evidence else 'PASS' if graph.get('complete') is True else 'UNKNOWN'
        else:
            value = facts.get(rule['domain'], {}).get(rule['metric'])
            if value is not None:
                if not isinstance(value, dict) or value.get('complete') is not True:
                    status = 'UNKNOWN'
                elif type(value.get('value')) not in (int, float) or not math.isfinite(value['value']) or value['value'] < 0:
                    raise EvidenceError('invalid observed rule metric')
                else:
                    evidence = [value]
                    status = 'FAIL' if value['value'] > rule['maximum'] else 'PASS'
        findings.append({'rule_id': rule['id'], 'domain': rule['domain'], 'status': status, 'evidence': evidence,
            'config_sha256': rule['config_sha256'], 'provenance': 'static_analysis', 'confidence': 'derived'})
    return {'status': 'FAIL' if any(row['status'] == 'FAIL' for row in findings) else
                      'UNKNOWN' if any(row['status'] == 'UNKNOWN' for row in findings) else 'PASS', 'findings': findings}
