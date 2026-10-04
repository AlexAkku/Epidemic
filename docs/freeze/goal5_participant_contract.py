"""Generate/check the reviewed Goal 5 participant graph (no Markdown inputs)."""
from __future__ import annotations

import argparse
import copy
import json
import hashlib
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.dont_write_bytecode = True
from goal5_persistence_contract import load_json, encoded
from module_dossiers import ROOT
from evidence_anchors import registered_test_targets

REVIEW = ROOT / "docs/freeze/goal5_participant_review.json"
CONTRACT = ROOT / "docs/freeze/goal5_participant_contract.json"
PERSISTENCE = ROOT / "docs/freeze/goal5_persistence_contract.json"
QUALIFICATION = ROOT / '_goal5_handoff/participant_qualification.json'
DECISIONS = {"EXISTING_PARTICIPANT", "NEW_PARTICIPANT", "AGGREGATED_PARTICIPANT",
             "RUNTIME_PERSISTENCE", "EXTERNAL_DURABLE"}
OBLIGATIONS = {"IDENTITY_SCHEMA_DEPENDENCIES", "CAPTURE_VALID", "CORRUPTION_REJECTED",
               "STAGE_NO_MUTATION", "PREPARE_FAILURE_ATOMIC", "COMMIT_PUBLICATION",
               "ONE_USE_COMMIT", "NONEMPTY_DURABLE_ROUNDTRIP", "REGISTRY_CLOSURE"}


def validate(graph: dict, persistence: dict, implementation: bool = False) -> list[str]:
    errors: list[str] = []
    if graph.get("schema_version") != 2:
        errors.append("unsupported schema_version")
    participants = graph.get("participants")
    if not isinstance(participants, list):
        return errors + ["participants must be a list"]
    durable = {key: value for key, value in persistence.get("candidates", {}).items()
               if value.get("save_plane") == "FRAMEWORK_SAVEGAME"
               and value.get("persistence_role") in {"PERSISTENT_OWNER", "PERSISTENT_CHECKPOINT"}}
    ids = [p.get("id") for p in participants if isinstance(p, dict)]
    if len(ids) != len(participants) or any(not isinstance(p, str) or not p.strip() for p in ids):
        errors.append("invalid participant identity")
    if len(ids) != len(set(ids)):
        errors.append("duplicate participant ID")
    by_id = {p["id"]: p for p in participants if isinstance(p, dict) and isinstance(p.get("id"), str)}
    covered = Counter()
    authority = defaultdict(set)
    targets = registered_test_targets(ROOT) if implementation else set()
    for participant_id, p in by_id.items():
        if p.get("decision") not in DECISIONS:
            errors.append(f"{participant_id}: invalid granularity decision")
        if not isinstance(p.get("rationale"), str) or not p["rationale"].strip():
            errors.append(f"{participant_id}: missing ownership/granularity rationale")
        if p.get("requirement") not in {"Required", "Optional"}:
            errors.append(f"{participant_id}: required/optional semantics missing")
        if type(p.get("schema_version")) is not int or p["schema_version"] < 1:
            errors.append(f"{participant_id}: invalid schema version")
        dependencies = p.get("dependencies")
        if not isinstance(dependencies, list) or any(not isinstance(d, str) for d in dependencies):
            errors.append(f"{participant_id}: invalid dependencies")
            dependencies = []
        if len(dependencies) != len(set(dependencies)):
            errors.append(f"{participant_id}: duplicate dependency")
        for dependency in dependencies:
            if dependency not in by_id:
                errors.append(f"{participant_id}: unknown dependency {dependency}")
            elif p.get("requirement") == "Required" and by_id[dependency].get("requirement") == "Optional":
                errors.append(f"{participant_id}: required participant depends on optional state")
        candidates = p.get("covered_candidate_ids")
        if not isinstance(candidates, list) or not candidates:
            errors.append(f"{participant_id}: orphan participant without state")
            candidates = []
        for candidate_id in candidates:
            covered[candidate_id] += 1
            candidate = durable.get(candidate_id)
            if candidate is None:
                errors.append(f"{participant_id}: unknown/non-durable candidate {candidate_id}")
                continue
            if candidate.get("participant_id") != participant_id:
                errors.append(f"{candidate_id}: persistence/participant identity mismatch")
            authority[candidate.get("authoritative_owner")].add(participant_id)
        if p.get("implementation_status") not in {"PLANNED", "IMPLEMENTED"}:
            errors.append(f"{participant_id}: invalid implementation status")
        if implementation and p.get("implementation_status") != "IMPLEMENTED":
            errors.append(f"{participant_id}: participant is not implemented")
        obligations = p.get("obligations", {})
        if implementation:
            if not isinstance(obligations, dict) or set(obligations) != OBLIGATIONS:
                errors.append(f"{participant_id}: typed obligation set is incomplete")
            else:
                for obligation, decision in obligations.items():
                    if not isinstance(decision, dict) or decision.get("status") not in {"PASS", "N/A"}:
                        errors.append(f"{participant_id}/{obligation}: invalid obligation status")
                        continue
                    if decision.get("status") == "N/A":
                        errors.append(f"{participant_id}/{obligation}: mandatory participant obligation cannot be N/A")
                        if not isinstance(decision.get("rationale"), str) or not decision["rationale"].strip():
                            errors.append(f"{participant_id}/{obligation}: N/A requires reviewed rationale")
                    else:
                        typed_evidence = decision.get("evidence")
                        if not isinstance(typed_evidence, list) or not typed_evidence:
                            errors.append(f"{participant_id}/{obligation}: PASS requires executable evidence")
            evidence = p.get("evidence", [])
            if not evidence:
                errors.append(f"{participant_id}: missing implementation evidence")
            for item in evidence:
                path = ROOT / str(item.get("source_path", ""))
                symbol = item.get("test_symbol", "")
                if not path.is_file() or not symbol or symbol not in path.read_text(encoding="utf-8"):
                    errors.append(f"{participant_id}: unresolved test symbol")
                if not item.get("contract_assertion") or not item.get("test_target"):
                    errors.append(f"{participant_id}: incomplete implementation evidence")
                elif item['test_target'] not in targets:
                    errors.append(f'{participant_id}: unregistered implementation test target')
            if isinstance(obligations, dict):
                seen_assertions = []
                for obligation, decision in obligations.items():
                    items = decision.get("evidence", []) if isinstance(decision, dict) else []
                    if not isinstance(items, list):
                        errors.append(f"{participant_id}/{obligation}: evidence must be a list")
                        continue
                    for item in items:
                        if not isinstance(item, dict):
                            errors.append(f"{participant_id}/{obligation}: evidence must be an object")
                            continue
                        path = ROOT / str(item.get("source_path", "")); symbol = item.get("test_symbol", "")
                        assertion_path = ROOT / str(item.get("assertion_source_path", ""))
                        token = item.get("assertion_token", "")
                        seen_assertions.append((str(assertion_path), str(token)))
                        if (not path.resolve().is_relative_to(ROOT.resolve()) or not assertion_path.resolve().is_relative_to(ROOT.resolve())):
                            errors.append(f"{participant_id}/{obligation}: evidence escapes project")
                        if item.get('kind') != 'executable': errors.append(f"{participant_id}/{obligation}: non-executable evidence")
                        if (not isinstance(token, str) or not token or not assertion_path.is_file()
                            or assertion_path.read_text(encoding="utf-8").count(token) != 1):
                            errors.append(f"{participant_id}/{obligation}: unresolved/non-unique executable assertion")
                        if not path.is_file() or not symbol or symbol not in path.read_text(encoding="utf-8"):
                            errors.append(f"{participant_id}/{obligation}: unresolved typed evidence symbol")
                        elif assertion_path.name == 'participant_test_contract.h':
                            text = path.read_text(encoding='utf-8')
                            expected_symbol = 'Test' + p.get('owner', '') + 'SaveParticipant'
                            body = re.search(r'\bvoid\s+' + re.escape(expected_symbol) + r'\(\)\s*\{(.*?)\n\}', text, re.S)
                            invocation = item.get('invocation_token')
                            if symbol != expected_symbol or not body or not isinstance(invocation, str) or not invocation or invocation not in body.group(1):
                                errors.append(f"{participant_id}/{obligation}: concrete participant does not execute the reviewed helper")
                        target = item.get("test_target")
                        if not item.get("contract_assertion") or not target:
                            errors.append(f"{participant_id}/{obligation}: incomplete typed evidence")
                        elif target not in targets:
                            errors.append(f"{participant_id}/{obligation}: unregistered typed evidence target")
                if len(seen_assertions) != len(set(seen_assertions)):
                    errors.append(f"{participant_id}: generic assertion reused across distinct obligations")
    for candidate in durable:
        if covered[candidate] == 0:
            errors.append(f"missing coverage: {candidate}")
        elif covered[candidate] != 1:
            errors.append(f"duplicate coverage: {candidate}")
    for owner, owners in authority.items():
        if not owner or owner == "N/A":
            errors.append("durable state without authoritative owner")
        elif len(owners) > 1:
            errors.append(f"overlapping owner: {owner}")
    marks = {}
    def visit(identity):
        if marks.get(identity) == 1:
            errors.append(f"dependency cycle: {identity}")
            return
        if marks.get(identity) == 2:
            return
        marks[identity] = 1
        for dependency in by_id[identity].get("dependencies", []):
            if dependency in by_id:
                visit(dependency)
        marks[identity] = 2
    for identity in by_id:
        visit(identity)
    return errors


def source_fingerprints():
    files = {p for folder in ('EngineBase','EngineRuntime','EngineFramework','Tests/Truth')
             for p in (ROOT / folder).rglob('*') if p.is_file() and
             (p.suffix in {'.h','.cpp'} or p.name == 'CMakeLists.txt')}
    # Framework build directories are outside this source tree. Generator source
    # and explicit templates are also part of the qualified production binding.
    files.update((ROOT / 'tools/goal5_participant_templates').glob('*'))
    files.add(ROOT / 'tools/generate_goal5_participants.py')
    files.add(ROOT / 'docs/freeze/goal5_participant_bindings.py')
    files.add(ROOT / 'docs/freeze/goal5_participant_contract.py')
    files.add(ROOT / 'tools/audit_goal5_restore_members.py')
    files.add(ROOT / 'tools/record_goal5_participant_qualification.py')
    files.update((ROOT / 'tools/ci').glob('*.ps1'))
    files.update((ROOT / 'tools/ci').glob('*.py'))
    files.update((ROOT / 'Tests/Fixtures/SaveGame').rglob('*'))
    files.update((ROOT / 'Tests/Fixtures/RuntimePersistence').rglob('*'))
    files.update(ROOT / 'docs/freeze' / name for name in (
        'goal5_schema_review.json','goal5_schema_contract.json','goal5_schema_contract.py',
        'goal5_stage52_contract.py','goal5_runtime_durable_contract.py','ctest_manifest.py','ctest_manifest.json',
        'goal5_persistence_contract.json','goal5_persistence_contract.py','goal5_persistence_inventory.py'))
    files.update(ROOT / 'tools' / name for name in (
        'record_goal5_stage52_closure.py','record_goal5_save_fixtures.py','record_goal5_public_delta.py'))
    files.add(ROOT / '_goal4_handoff/merge_goal4_evidence.py')
    files.update(ROOT / name for name in ('run.ps1','build.ps1'))
    files.update((ROOT / 'cmake').glob('*.cmake'))
    files.add(ROOT / 'CMakeLists.txt')
    files.add(ROOT / '.github/workflows/architecture-freeze.yml')
    files.add(ROOT / 'docs/freeze/ci_gate_contract.py')
    files.add(ROOT / 'tools/check_goal5_private_surface.py')
    files.add(ROOT / 'tools/record_goal5_participant_evidence.py')
    files.add(ROOT / 'tools/record_goal5_delta2_review.py')
    files.add(ROOT / '_goal5_handoff/delta2_persistence_review.json')
    files.update(ROOT / name for name in (
        'tools/record_goal5_save_generation_admission.py',
        'docs/freeze/goal5_save_generation_contract.py',
        'docs/freeze/goal5_save_generation_contract.json',
        '_goal5_handoff/save_generation_contract_review.json',
        '_goal5_handoff/save_generation_persistence_review.json',
        '_goal5_handoff/save_generation_public_surface_delta.json'))
    files.add(REVIEW)
    files.add(CONTRACT)
    files.add(ROOT / 'docs/freeze/goal5_admission.py')
    files.add(ROOT / 'docs/freeze/goal5_admission_delta.json')
    files.add(ROOT / 'docs/freeze/goal5_admission.json')
    files.update((ROOT / 'Tests/Fixtures/SaveGameConsumers').glob('*.cpp'))
    return {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(files) if p.is_file()}


def graph_fingerprint(graph):
    keys = ('id','schema_version','requirement','dependencies','covered_candidate_ids','owner','owner_module',
            'namespace','snapshot','capture','restore','placement','decision','implementation_status','evidence','obligations')
    reviewed = [{k:p.get(k) for k in keys} for p in sorted(graph['participants'],key=lambda p:p['id'])]
    return hashlib.sha256(encoded(reviewed).encode('utf-8')).hexdigest()


def qualification_errors(graph):
    if not QUALIFICATION.is_file(): return ['missing exact-tree participant qualification receipt']
    receipt = load_json(QUALIFICATION)
    manifest = load_json(ROOT / 'docs/freeze/ctest_manifest.json')
    return validate_qualification_receipt(receipt, source_fingerprints(), graph_fingerprint(graph), manifest['profiles'])


def validate_qualification_receipt(receipt, source_hashes, graph_hash, expected_profiles):
    errors = []
    if receipt.get('schema_version') != 1 or receipt.get('status') != 'PASS': errors.append('invalid participant qualification receipt')
    if receipt.get('source_sha256') != source_hashes: errors.append('participant qualification source fingerprint is stale')
    if receipt.get('graph_sha256') != graph_hash: errors.append('participant qualification graph fingerprint is stale')
    profiles = receipt.get('profiles',{})
    if not isinstance(profiles, dict): return errors + ['malformed qualification profiles']
    if set(profiles) != set(expected_profiles): errors.append('participant qualification requires all six profiles')
    for name,p in expected_profiles.items():
        if profiles.get(name) != {'status':'PASS','test_count':len(p['tests'])}:
            errors.append(f'participant qualification did not pass exact profile {name}')
    return errors


def generate(review: dict) -> dict:
    return {"schema_version": 2, "generated_from": "docs/freeze/goal5_participant_review.json",
            "participants": sorted(copy.deepcopy(review["participants"]), key=lambda p: p["id"])}


def self_test():
    candidates = {"c1": {"save_plane": "FRAMEWORK_SAVEGAME", "persistence_role": "PERSISTENT_OWNER",
                         "participant_id": "a", "authoritative_owner": "owner-a"},
                  "c2": {"save_plane": "FRAMEWORK_SAVEGAME", "persistence_role": "PERSISTENT_CHECKPOINT",
                         "participant_id": "b", "authoritative_owner": "owner-b"}}
    persistence = {"candidates": candidates}
    graph = {"schema_version": 2, "participants": [
        {"id": identity, "decision": "NEW_PARTICIPANT", "requirement": "Required", "schema_version": 1,
         "dependencies": dependencies, "covered_candidate_ids": [candidate], "rationale": "one local state boundary",
         "implementation_status": "PLANNED"}
        for identity, candidate, dependencies in [("a", "c1", []), ("b", "c2", ["a"])]]}
    assert not validate(graph, persistence)
    cases = []
    def negative(label, edit, expected, edit_persistence=None):
        altered = copy.deepcopy(graph); state = copy.deepcopy(persistence)
        edit(altered)
        if edit_persistence:
            edit_persistence(state)
        failures = validate(altered, state)
        assert any(expected in e for e in failures), (label, failures)
        cases.append(label)
    negative("missing coverage", lambda g: g["participants"].pop(), "missing coverage")
    negative("duplicate coverage", lambda g: g["participants"][1]["covered_candidate_ids"].append("c1"), "duplicate coverage")
    negative("cycle", lambda g: g["participants"][0]["dependencies"].append("b"), "dependency cycle")
    negative("unknown dependency", lambda g: g["participants"][0]["dependencies"].append("missing"), "unknown dependency")
    negative("orphan", lambda g: g["participants"][1].update(covered_candidate_ids=[]), "orphan participant")
    negative("overlap owner", lambda g: None, "overlapping owner",
             lambda p: p["candidates"]["c2"].update(authoritative_owner="owner-a"))
    negative("duplicate ID", lambda g: g["participants"][1].update(id="a"), "duplicate participant ID")
    negative("required/optional", lambda g: g["participants"][0].update(requirement="Optional"), "depends on optional")
    negative("missing requirement", lambda g: g["participants"][0].pop("requirement"), "semantics missing")
    negative("schema", lambda g: g["participants"][0].update(schema_version=0), "invalid schema")
    negative("identity drift", lambda g: None, "identity mismatch", lambda p: p["candidates"]["c1"].update(participant_id="b"))
    assert any("not implemented" in e for e in validate(graph, persistence, True))
    implemented = load_json(REVIEW)
    state = load_json(PERSISTENCE)
    assert not validate(implemented, state, True)
    negatives = (
        lambda p: p['obligations'].pop('CAPTURE_VALID'),
        lambda p: p['obligations']['CAPTURE_VALID'].update(status='N/A', rationale='generic exemption'),
        lambda p: p['obligations']['CAPTURE_VALID'].update(status='FAIL'),
        lambda p: p['obligations']['CAPTURE_VALID'].update(evidence=[]),
        lambda p: p['obligations']['CAPTURE_VALID']['evidence'][0].update(test_target='unregistered'),
        lambda p: p['obligations']['CAPTURE_VALID']['evidence'][0].update(assertion_token='missing assertion'),
        lambda p: p['obligations']['CAPTURE_VALID']['evidence'][0].update(kind='document'),
        lambda p: p['obligations']['CAPTURE_VALID'].update(evidence='not a list'),
        lambda p: p['obligations']['CAPTURE_VALID'].update(evidence=[42]),
        lambda p: p['obligations']['CAPTURE_VALID']['evidence'][0].update(invocation_token='missing helper invocation'),
        lambda p: p['obligations'].update(CAPTURE_VALID=copy.deepcopy(p['obligations']['COMMIT_PUBLICATION'])),
        lambda p: p['obligations'].pop('NONEMPTY_DURABLE_ROUNDTRIP'),
    )
    for edit in negatives:
        bad = copy.deepcopy(implemented); edit(bad['participants'][0]); assert validate(bad, state, True)
    bad = copy.deepcopy(implemented); bad['participants'][0]['obligations']['CAPTURE_VALID']['evidence'][0]['contract_assertion'] += ' changed'
    assert graph_fingerprint(bad) != graph_fingerprint(implemented)
    receipt = {'schema_version':1,'status':'PASS','source_sha256':{'source.cpp':'hash'},'graph_sha256':'graph',
               'profiles':{'fixture':{'status':'PASS','test_count':1}}}
    fixture_profiles = {'fixture':{'tests':['test']}}
    assert not validate_qualification_receipt(receipt, {'source.cpp':'hash'}, 'graph', fixture_profiles)
    for key,value in [('status','FAIL'),('source_sha256',{}),('graph_sha256','stale'),('profiles',{}),
                      ('profiles',{'fixture':{'status':'PASS','test_count':2}})]:
        altered = copy.deepcopy(receipt); altered[key] = value
        assert validate_qualification_receipt(altered, {'source.cpp':'hash'}, 'graph', fixture_profiles)
    print(f"PASS: {len(cases)} graph negatives, 12 typed-obligation negatives, evidence identity and 5 qualification negatives; fail-closed.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--record", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--implementation", action="store_true")
    parser.add_argument("--qualification", action="store_true")
    parser.add_argument("--source-fingerprints", action="store_true")
    args = parser.parse_args()
    if args.source_fingerprints:
        print(json.dumps(source_fingerprints(), sort_keys=True)); return 0
    if args.self_test:
        self_test(); return 0
    if not args.check and not args.record:
        parser.error("choose --check, --record or --self-test")
    try:
        expected = generate(load_json(REVIEW))
        implementation = args.implementation or args.qualification
        errors = validate(expected, load_json(PERSISTENCE), implementation)
        if args.qualification: errors.extend(qualification_errors(expected))
        if errors:
            print("\n".join(f"BLOCKED: {error}" for error in errors), file=sys.stderr); return 1
        if args.record:
            CONTRACT.write_text(encoded(expected), encoding="utf-8", newline="\n")
        if args.check and (not CONTRACT.is_file() or CONTRACT.read_text(encoding="utf-8") != encoded(expected)):
            print("BLOCKED: participant contract is stale; run --record", file=sys.stderr); return 1
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"BLOCKED: {error}", file=sys.stderr); return 1
    print(f"PASS: exact participant coverage; {len(expected['participants'])} unique participants; acyclic semantic graph.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
