"""Validate the reviewed Goal 5 persistence contract and generate its gap report."""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.dont_write_bytecode = True

from module_dossiers import ROOT, discover_modules

FREEZE = ROOT / "docs" / "freeze"
INVENTORY = FREEZE / "goal5_persistence_candidates.json"
CONTRACT = FREEZE / "goal5_persistence_contract.json"
GAPS = FREEZE / "goal5_persistence_gaps.json"
CTEST = FREEZE / "ctest_manifest.json"
ROLES = {
    "PERSISTENT_OWNER", "PERSISTENT_CHECKPOINT", "RECONSTRUCTED_TRANSIENT",
    "READ_ONLY_SNAPSHOT", "NON_PERSISTENT_OWNER",
}
PLANES = {"FRAMEWORK_SAVEGAME", "RUNTIME_PERSISTENCE", "EXTERNAL", "NONE"}
APPLICABILITY = {"APPLICABLE", "NOT_APPLICABLE", "BLOCKED"}
REVIEW_STATUSES = {"DISCOVERED", "REVIEWED", "BLOCKED"}
DETERMINISM = {"YES", "NO", "DERIVED"}
DIGEST_POLICIES = {"INCLUDE", "DERIVED_AND_VERIFIED", "DEFER_TO_5_5", "EXCLUDE"}
PERSISTENT_ROLES = {"PERSISTENT_OWNER", "PERSISTENT_CHECKPOINT"}
FORBIDDEN_MARKERS = re.compile(r"\b(?:TODO|UNKNOWN|UNCLASSIFIED)\b", re.IGNORECASE)
GENERIC_EVIDENCE = re.compile(
    r"^(?:main|int\s+main|return\s+0|#\s*include|using\b|generic\s+smoke)", re.IGNORECASE)


class DuplicateKeyError(ValueError):
    pass


def reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise DuplicateKeyError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_json(path: Path) -> dict[str, object]:
    data = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=reject_duplicate_keys)
    if not isinstance(data, dict):
        raise ValueError(f"{path.name} root must be an object")
    return data


def ctest_targets() -> set[str]:
    manifest = load_json(CTEST)
    return {
        str(test["name"])
        for profile in manifest.get("profiles", {}).values()
        if isinstance(profile, dict)
        for test in profile.get("tests", [])
        if isinstance(test, dict) and test.get("name")
    }


def contains_forbidden(value: object) -> bool:
    if isinstance(value, str):
        return bool(FORBIDDEN_MARKERS.search(value))
    if isinstance(value, list):
        return any(contains_forbidden(item) for item in value)
    if isinstance(value, dict):
        return any(contains_forbidden(item) for item in value.values())
    return False


def valid_dependency(value: object, candidate_ids: set[str], modules: set[str],
                     participants: set[str]) -> bool:
    if not isinstance(value, str) or not value:
        return False
    if value.startswith("GAP:"):
        return True
    if value.startswith("candidate:"):
        return value.removeprefix("candidate:") in candidate_ids
    if value.startswith("module:"):
        return value.removeprefix("module:") in modules
    if value.startswith("participant:"):
        return value.removeprefix("participant:") in participants
    return False


def evidence_errors(candidate_id: str, evidence: object, targets: set[str],
                    verify_sources: bool) -> list[str]:
    errors: list[str] = []
    if not isinstance(evidence, list) or not evidence:
        return [f"{candidate_id}: evidence must be a non-empty list"]
    for index, item in enumerate(evidence):
        prefix = f"{candidate_id}: evidence[{index}]"
        if not isinstance(item, dict):
            errors.append(f"{prefix} must be an object")
            continue
        kind = item.get("kind")
        source = item.get("source_path")
        assertion = item.get("contract_assertion")
        if kind not in {"executable", "reviewed_contract"}:
            errors.append(f"{prefix} has unknown kind")
        if not isinstance(source, str) or not source:
            errors.append(f"{prefix} has no source_path")
        if not isinstance(assertion, str) or not assertion.strip() or GENERIC_EVIDENCE.match(assertion.strip()):
            errors.append(f"{prefix} has generic or empty assertion")
        if kind == "executable":
            target = item.get("test_target")
            symbol = item.get("test_symbol")
            test_case = item.get("test_case")
            identity = symbol if isinstance(symbol, str) and symbol.strip() else test_case
            if target not in targets:
                errors.append(f"{prefix} references stale test target: {target}")
            if not isinstance(identity, str) or not identity.strip() or GENERIC_EVIDENCE.match(identity.strip()):
                errors.append(f"{prefix} has generic or empty test identity")
            if verify_sources and isinstance(source, str):
                path = ROOT / source
                if not path.is_file():
                    errors.append(f"{prefix} source does not exist: {source}")
                elif isinstance(symbol, str) and symbol.strip() and symbol.split("(", 1)[0].split()[-1] not in path.read_text(
                        encoding="utf-8", errors="ignore"):
                    errors.append(f"{prefix} test symbol is stale: {symbol}")
    return errors


def validate_contract(contract: object, inventory: object, targets: set[str],
                      verify_sources: bool = True) -> list[str]:
    if not isinstance(contract, dict) or not isinstance(inventory, dict):
        return ["contract and inventory roots must be objects"]
    errors: list[str] = []
    if contract.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    inventory_modules = set(str(value) for value in inventory.get("modules", []))
    inventory_candidates = {
        str(item.get("candidate_id")): item for item in inventory.get("candidates", [])
        if isinstance(item, dict) and item.get("candidate_id")
    }
    modules = contract.get("modules")
    candidates = contract.get("candidates")
    if not isinstance(modules, dict):
        errors.append("modules must be an object")
        modules = {}
    if not isinstance(candidates, dict):
        errors.append("candidates must be an object")
        candidates = {}

    contract_modules = set(modules)
    for missing in sorted(inventory_modules - contract_modules):
        errors.append(f"missing module decision: {missing}")
    for unknown in sorted(contract_modules - inventory_modules):
        errors.append(f"unknown module: {unknown}")
    contract_ids = set(candidates)
    for missing in sorted(set(inventory_candidates) - contract_ids):
        errors.append(f"missing candidate: {missing}")
    for unknown in sorted(contract_ids - set(inventory_candidates)):
        errors.append(f"unknown candidate ID: {unknown}")

    assigned: list[str] = []
    for module, record in modules.items():
        if not isinstance(record, dict):
            errors.append(f"module {module}: record must be an object")
            continue
        if record.get("module") != module:
            errors.append(f"module {module}: embedded module identity mismatch")
        if record.get("applicability") not in APPLICABILITY:
            errors.append(f"module {module}: unknown applicability")
        if record.get("applicability") == "BLOCKED":
            errors.append(f"module {module}: BLOCKED is not allowed at 5.1 exit")
        if not isinstance(record.get("reason"), str) or not record["reason"].strip():
            errors.append(f"module {module}: empty applicability reason")
        if contains_forbidden(record):
            errors.append(f"module {module}: forbidden placeholder marker")
        ids = record.get("candidate_ids")
        if not isinstance(ids, list) or any(not isinstance(item, str) for item in ids):
            errors.append(f"module {module}: candidate_ids must be a string list")
        else:
            assigned.extend(ids)
            expected = sorted(
                candidate_id for candidate_id, item in inventory_candidates.items()
                if item.get("module") == module)
            if ids != expected:
                errors.append(f"module {module}: candidate_ids do not exactly match discovery")
    duplicate_assignments = sorted(candidate_id for candidate_id, count in Counter(assigned).items() if count > 1)
    for candidate_id in duplicate_assignments:
        errors.append(f"candidate assigned to multiple modules: {candidate_id}")

    participants = {
        str(record.get("participant_id")) for record in candidates.values()
        if isinstance(record, dict) and isinstance(record.get("participant_id"), str)
        and record.get("participant_id") not in {"N/A", "RUNTIME_PERSISTENCE_PLANE"}
        and not str(record.get("participant_id")).startswith("GAP:")
    }
    owners: dict[str, str] = {}
    for candidate_id, record in candidates.items():
        if not isinstance(record, dict):
            errors.append(f"{candidate_id}: record must be an object")
            continue
        discovered = inventory_candidates.get(candidate_id, {})
        if record.get("candidate_id") != candidate_id:
            errors.append(f"{candidate_id}: embedded candidate ID mismatch")
        if record.get("module") != discovered.get("module"):
            errors.append(f"{candidate_id}: module does not match discovery")
        if record.get("qualified_symbol") != discovered.get("qualified_symbol"):
            errors.append(f"{candidate_id}: qualified symbol does not match discovery")
        role = record.get("persistence_role")
        plane = record.get("save_plane")
        if role not in ROLES:
            errors.append(f"{candidate_id}: unknown classification")
        if plane not in PLANES:
            errors.append(f"{candidate_id}: unknown save plane")
        if record.get("review_status") not in REVIEW_STATUSES:
            errors.append(f"{candidate_id}: unknown review status")
        elif record.get("review_status") != "REVIEWED":
            errors.append(f"{candidate_id}: candidate is not REVIEWED")
        if not isinstance(record.get("rationale"), str) or not record["rationale"].strip():
            errors.append(f"{candidate_id}: empty rationale")
        if contains_forbidden(record):
            errors.append(f"{candidate_id}: forbidden placeholder marker")
        for field in ('capture_contract','restore_contract','durable_schema','migration_policy','durable_storage','transient_rebuild','reconciliation'):
            if not isinstance(record.get(field),str) or not record[field].strip():
                errors.append(f'{candidate_id}: missing reviewed contract field {field}')

        owner = record.get("authoritative_owner")
        if role in PERSISTENT_ROLES:
            if not isinstance(owner, str) or not owner or owner == "N/A":
                errors.append(f"{candidate_id}: persistent state has no authoritative owner")
            if plane == "NONE":
                errors.append(f"{candidate_id}: persistent state has no save plane")
        if role == "PERSISTENT_OWNER" and isinstance(owner, str) and owner not in {"", "N/A"}:
            previous = owners.get(owner)
            if previous is not None and previous != candidate_id:
                errors.append(f"{candidate_id}: duplicate authoritative owner {owner}")
            owners[owner] = candidate_id
        if role == "PERSISTENT_CHECKPOINT" and (not isinstance(owner, str) or owner in {"", "N/A"}):
            errors.append(f"{candidate_id}: persistent checkpoint has no owner")
        if role == "RECONSTRUCTED_TRANSIENT":
            rebuild = record.get("transient_rebuild")
            if not isinstance(rebuild, str) or not rebuild.strip() or rebuild.startswith("N/A"):
                errors.append(f"{candidate_id}: reconstructed transient has no rebuild rationale")
        if role == "READ_ONLY_SNAPSHOT" and not str(record.get("rationale", "")).strip():
            errors.append(f"{candidate_id}: read-only snapshot has no non-durable rationale")
        if plane == "FRAMEWORK_SAVEGAME":
            participant = record.get("participant_id")
            if not isinstance(participant, str) or participant in {"", "N/A"}:
                errors.append(f"{candidate_id}: Framework state has no participant mapping or 5.2 gap")
        if plane == "RUNTIME_PERSISTENCE" and record.get("participant_id") != "RUNTIME_PERSISTENCE_PLANE":
            errors.append(f"{candidate_id}: Runtime state does not map to Runtime Persistence")

        dependencies = record.get("dependencies")
        if not isinstance(dependencies, list):
            errors.append(f"{candidate_id}: dependencies must be a list")
        else:
            for dependency in dependencies:
                if not valid_dependency(dependency, contract_ids, contract_modules, participants):
                    errors.append(f"{candidate_id}: missing dependency: {dependency}")

        risks = set(str(value) for value in discovered.get("risk_flags", []))
        decisions = record.get("risk_review")
        if not isinstance(decisions, dict) or set(decisions) != risks:
            errors.append(f"{candidate_id}: dangerous field flags lack exact reviewed decisions")
        else:
            for flag, decision in decisions.items():
                if (not isinstance(decision, dict) or not str(decision.get("decision", "")).strip()
                        or not str(decision.get("rationale", "")).strip()):
                    errors.append(f"{candidate_id}: risk {flag} lacks decision or rationale")

        relevance = record.get("deterministic_relevance")
        digest = record.get("digest_policy")
        if relevance not in DETERMINISM:
            errors.append(f"{candidate_id}: invalid deterministic relevance")
        if digest not in DIGEST_POLICIES:
            errors.append(f"{candidate_id}: deterministic state has no digest decision")
        if relevance == "YES" and digest not in {"INCLUDE", "DERIVED_AND_VERIFIED", "DEFER_TO_5_5"}:
            errors.append(f"{candidate_id}: deterministic state has no digest decision")
        errors.extend(evidence_errors(candidate_id, record.get("evidence"), targets, verify_sources))
    return errors


def gap_report(contract: dict[str, object]) -> dict[str, object]:
    gaps: list[dict[str, object]] = []
    for candidate_id, record in contract["candidates"].items():
        if record["persistence_role"] not in PERSISTENT_ROLES:
            continue
        categories: dict[str, list[str]] = {}
        if str(record.get("participant_id", "")).startswith("GAP:"):
            categories.setdefault("MISSING_PARTICIPANT", []).append("participant_id")
        if (str(record.get("durable_schema", "")).startswith("GAP:")
                or str(record.get("migration_policy", "")).startswith("GAP:")):
            categories.setdefault("MISSING_SCHEMA_POLICY", []).extend(["durable_schema", "migration_policy"])
        if str(record.get("durable_storage", "")).startswith("GAP:"):
            categories.setdefault("MISSING_DURABLE_STORAGE", []).append("durable_storage")
        if any(str(value).startswith("GAP:") for value in record.get("dependencies", [])):
            categories.setdefault("MISSING_DEPENDENCY_DECISION", []).append("dependencies")
        if str(record.get("reconciliation", "")).startswith("GAP:"):
            categories.setdefault("MISSING_RECONCILIATION", []).append("reconciliation")
        if not any(item.get("kind") == "executable" for item in record.get("evidence", [])):
            categories.setdefault("MISSING_TEST_EVIDENCE", []).append("evidence")
        if record.get("digest_policy") == "DEFER_TO_5_5":
            categories.setdefault("MISSING_DETERMINISM_POLICY", []).append("digest_policy")
        if any(item.get("decision") == "REMAP_ON_RESTORE" for item in record.get("risk_review", {}).values()):
            categories.setdefault("SUSPECT_TRANSIENT_IDENTITY", []).append("risk_review")
        for category, fields in categories.items():
            identity = f"{category}|{candidate_id}"
            gaps.append({
                "gap_id": "G5G-" + hashlib.sha256(identity.encode("utf-8")).hexdigest()[:16],
                "category": category,
                "candidate_id": candidate_id,
                "module": record["module"],
                "fields": sorted(set(fields)),
                "target_stage": "5.3" if any(str(record.get(field, '')).startswith('GAP:5.3_') for field in fields) else (
                    "5.5" if category == "MISSING_DETERMINISM_POLICY" else (
                    "5.4" if category in {"MISSING_RECONCILIATION", "SUSPECT_TRANSIENT_IDENTITY"} else "5.2")),
            })
    gaps.sort(key=lambda item: (item["category"], item["module"], item["candidate_id"]))
    counts = Counter(item["category"] for item in gaps)
    return {
        "schema_version": 1,
        "generated_from": "docs/freeze/goal5_persistence_contract.json",
        "summary": {"total": len(gaps), "by_category": dict(sorted(counts.items()))},
        "gaps": gaps,
    }


def encoded(data: dict[str, object]) -> str:
    return json.dumps(data, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def fixture() -> tuple[dict[str, object], dict[str, object], set[str]]:
    inventory = {
        "modules": ["EngineFramework/Fake"],
        "candidates": [
            {"candidate_id": "c1", "module": "EngineFramework/Fake",
             "qualified_symbol": "fake::Owner", "risk_flags": []},
            {"candidate_id": "c2", "module": "EngineFramework/Fake",
             "qualified_symbol": "fake::Checkpoint", "risk_flags": ["RUNTIME_HANDLE"]},
        ],
    }
    evidence = [{"kind": "executable", "test_target": "FakeTests",
                 "test_symbol": "RestoreRoundTrip", "contract_assertion": "restored state equals captured state",
                 "source_path": "fake_tests.cpp"}]
    base = {
        "schema_version": 1,
        "modules": {"EngineFramework/Fake": {"module": "EngineFramework/Fake", "applicability": "APPLICABLE",
                    "reason": "reviewed fixture", "candidate_ids": ["c1", "c2"]}},
        "candidates": {
            "c1": {"candidate_id": "c1", "module": "EngineFramework/Fake", "qualified_symbol": "fake::Owner",
                   "persistence_role": "PERSISTENT_OWNER", "save_plane": "FRAMEWORK_SAVEGAME",
                   "authoritative_owner": "fake::Owner", "capture_contract": "CaptureSnapshot",
                   "restore_contract": "RestoreSnapshot", "participant_id": "fake.owner",
                   "durable_schema": "fake.v1", "migration_policy": "CURRENT_ONLY", "durable_storage": "fixture",
                   "dependencies": [], "transient_rebuild": "N/A", "reconciliation": "N/A",
                   "deterministic_relevance": "YES", "digest_policy": "INCLUDE", "risk_review": {},
                   "evidence": evidence, "review_status": "REVIEWED", "rationale": "reviewed owner"},
            "c2": {"candidate_id": "c2", "module": "EngineFramework/Fake", "qualified_symbol": "fake::Checkpoint",
                   "persistence_role": "PERSISTENT_CHECKPOINT", "save_plane": "FRAMEWORK_SAVEGAME",
                   "authoritative_owner": "fake::Owner", "capture_contract": "owner capture",
                   "restore_contract": "owner restore", "participant_id": "fake.owner",
                   "durable_schema": "fake.v1", "migration_policy": "CURRENT_ONLY", "durable_storage": "fixture",
                   "dependencies": ["candidate:c1"], "transient_rebuild": "N/A", "reconciliation": "N/A",
                   "deterministic_relevance": "YES", "digest_policy": "INCLUDE",
                   "risk_review": {"RUNTIME_HANDLE": {"decision": "REMAP_ON_RESTORE", "rationale": "fixture remap"}},
                   "evidence": evidence, "review_status": "REVIEWED", "rationale": "reviewed checkpoint"},
        },
    }
    return base, inventory, {"FakeTests"}


def self_test() -> None:
    base, inventory, targets = fixture()
    assert not validate_contract(base, inventory, targets, verify_sources=False)
    cases: list[tuple[str, dict[str, object], dict[str, object], str]] = []
    missing = copy.deepcopy(base); missing["candidates"].pop("c2")
    cases.append(("missing candidate", missing, inventory, "missing candidate"))
    duplicate_owner = copy.deepcopy(base); duplicate_owner["candidates"]["c2"]["persistence_role"] = "PERSISTENT_OWNER"
    cases.append(("duplicate owner", duplicate_owner, inventory, "duplicate authoritative owner"))
    no_plane = copy.deepcopy(base); no_plane["candidates"]["c1"]["save_plane"] = "NONE"
    cases.append(("owner without plane", no_plane, inventory, "no save plane"))
    no_risk = copy.deepcopy(base); no_risk["candidates"]["c2"]["risk_review"] = {}
    cases.append(("runtime handle without decision", no_risk, inventory, "dangerous field flags"))
    bad_dependency = copy.deepcopy(base); bad_dependency["candidates"]["c2"]["dependencies"] = ["candidate:missing"]
    cases.append(("missing dependency", bad_dependency, inventory, "missing dependency"))
    unknown_module = copy.deepcopy(base); unknown_module["modules"]["EngineFramework/Unknown"] = {
        "module": "EngineFramework/Unknown", "applicability": "NOT_APPLICABLE", "reason": "fixture", "candidate_ids": []}
    cases.append(("unknown module", unknown_module, inventory, "unknown module"))
    generic = copy.deepcopy(base); generic["candidates"]["c1"]["evidence"][0]["test_symbol"] = "main"
    cases.append(("generic evidence", generic, inventory, "generic or empty test identity"))
    no_owner = copy.deepcopy(base); no_owner["candidates"]["c2"]["authoritative_owner"] = "N/A"
    cases.append(("checkpoint without owner", no_owner, inventory, "checkpoint has no owner"))
    no_digest = copy.deepcopy(base); no_digest["candidates"]["c1"]["digest_policy"] = ""
    cases.append(("deterministic state without digest", no_digest, inventory, "no digest decision"))
    unknown_role = copy.deepcopy(base); unknown_role["candidates"]["c1"]["persistence_role"] = "MYSTERY"
    cases.append(("unknown classification", unknown_role, inventory, "unknown classification"))
    stale_target = copy.deepcopy(base); stale_target["candidates"]["c1"]["evidence"][0]["test_target"] = "OldTests"
    cases.append(("stale target", stale_target, inventory, "stale test target"))
    bad_module_reason=copy.deepcopy(base); bad_module_reason['modules']['EngineFramework/Fake']['reason']='TODO'
    cases.append(('placeholder module review',bad_module_reason,inventory,'forbidden placeholder'))
    missing_schema=copy.deepcopy(base); missing_schema['candidates']['c1'].pop('durable_schema')
    cases.append(('missing contract field',missing_schema,inventory,'missing reviewed contract field'))
    for name, contract, candidate_inventory, expected in cases:
        errors = validate_contract(contract, candidate_inventory, targets, verify_sources=False)
        assert any(expected in error for error in errors), (name, errors)
    try:
        json.loads('{"candidates":{"c1":{},"c1":{}}}', object_pairs_hook=reject_duplicate_keys)
    except DuplicateKeyError:
        pass
    else:
        raise AssertionError("duplicate candidate key was accepted")
    print("PASS: persistence contract validator accepted the positive fixture and rejected 13 semantic failures "
          "plus duplicate JSON identity.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--record-gaps", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    try:
        inventory = load_json(INVENTORY)
        contract = load_json(CONTRACT)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"BLOCKED: cannot load Goal 5 persistence contract: {error}", file=sys.stderr)
        return 1
    errors = validate_contract(contract, inventory, ctest_targets())
    if errors:
        for error in errors[:80]:
            print(f"BLOCKED: {error}", file=sys.stderr)
        if len(errors) > 80:
            print(f"BLOCKED: {len(errors) - 80} additional errors omitted", file=sys.stderr)
        return 1
    report = gap_report(contract)
    if args.record_gaps:
        GAPS.write_text(encoded(report), encoding="utf-8", newline="\n")
        print(f"Recorded {report['summary']['total']} explicit persistence gaps.")
    if args.check:
        if not GAPS.is_file() or GAPS.read_text(encoding="utf-8") != encoded(report):
            print("BLOCKED: goal5_persistence_gaps.json is missing or stale; run --record-gaps.", file=sys.stderr)
            return 1
        role_counts = Counter(item["persistence_role"] for item in contract["candidates"].values())
        print(f"PASS: {len(contract['modules'])} modules and {len(contract['candidates'])} candidates are reviewed; "
              f"UNCLASSIFIED=0, UNKNOWN=0, explicit gaps={report['summary']['total']}; "
              f"roles={dict(sorted(role_counts.items()))}.")
    if not args.check and not args.record_gaps:
        parser.error("choose --check, --record-gaps or --self-test")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
