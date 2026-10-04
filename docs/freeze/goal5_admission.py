"""Validate the exact-tree admission baseline for Milestone 5."""
from __future__ import annotations

import argparse
import copy
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True

from local_ready_contract import validate_ledger
from module_dossiers import ROOT, discover_modules
from public_api_inventory import (
    apply_classification_overrides,
    load_classification_overrides,
    scan_modules,
)

ADMISSION = ROOT / "docs" / "freeze" / "goal5_admission.json"
SURFACE_DELTA = ROOT / "docs" / "freeze" / "goal5_admission_delta.json"
LOCAL_READY = ROOT / "docs" / "freeze" / "local_ready_ledger.json"
EXPECTED_PROFILES = {
    "base-debug": ("Debug", "base"),
    "base-release": ("Release", "base"),
    "runtime-debug": ("Debug", "runtime"),
    "runtime-release": ("Release", "runtime"),
    "full-debug": ("Debug", "full"),
    "full-release": ("Release", "full"),
}
REQUIRED_TOOLS = {"compiler", "cmake", "ctest", "python", "powershell"}


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
        raise ValueError("admission root must be an object")
    return data


def current_counts() -> tuple[int, int, int]:
    modules = discover_modules()
    headers = {path.resolve() for module in modules for path in module.headers}
    callables, override_errors = apply_classification_overrides(
        scan_modules(modules), load_classification_overrides())
    if override_errors:
        raise ValueError("public API classification errors: " + "; ".join(override_errors))
    return len(modules), len(headers), len(callables)


def goal4_errors() -> list[str]:
    ledger = json.loads(LOCAL_READY.read_text(encoding="utf-8"))
    if ledger.get("contract") != "LOCAL_READY":
        return ["local-ready ledger does not declare LOCAL_READY"]
    return validate_ledger(ledger)


def validate_surface_evolution(admission: dict, actual_ids: set[str], delta: dict) -> list[str]:
    errors = []
    if delta.get('schema_version') != 1 or delta.get('reviewed') is not True or delta.get('baseline_source_sha') != admission.get('source_sha'):
        errors.append('post-admission surface delta lacks an exact reviewed baseline')
    baseline = delta.get('baseline_api_ids', [])
    added = delta.get('added_api_ids', [])
    if not isinstance(baseline, list) or not isinstance(added, list) or any(not isinstance(i,str) for i in baseline + added):
        return errors + ['malformed post-admission API IDs']
    if len(baseline) != admission.get('public_api_count') or len(baseline) != len(set(baseline)):
        errors.append('post-admission baseline API identity set is incomplete')
    if len(added) != len(set(added)) or set(added) & set(baseline):
        errors.append('post-admission delta has duplicate/overlapping API IDs')
    if set(baseline) | set(added) != actual_ids:
        errors.append('post-admission exact API identity set differs from discovery')
    return errors


def validate(data: object, counts: tuple[int, int, int] | None = None,
             verify_goal4: bool = True) -> list[str]:
    if not isinstance(data, dict):
        return ["admission root must be an object"]
    errors: list[str] = []
    if data.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    if not re.fullmatch(r"[0-9a-f]{40}", str(data.get("source_sha", ""))):
        errors.append("source_sha must be a full lowercase Git SHA")

    actual = counts if counts is not None else current_counts()
    for field, expected in zip(
            ("production_module_count", "public_header_count", "public_api_count"), actual):
        if field == 'public_api_count' and counts is None and SURFACE_DELTA.is_file():
            rows, override_errors = apply_classification_overrides(scan_modules(discover_modules()), load_classification_overrides())
            errors.extend(override_errors)
            errors.extend(validate_surface_evolution(data, {r.id for r in rows}, load_json(SURFACE_DELTA)))
        elif data.get(field) != expected:
            errors.append(f"{field} does not match current production discovery ({expected})")

    profiles = data.get("profiles")
    if not isinstance(profiles, dict):
        errors.append("profiles must be an object")
    else:
        names = set(profiles)
        for name in sorted(names - set(EXPECTED_PROFILES)):
            errors.append(f"unknown profile: {name}")
        for name in sorted(set(EXPECTED_PROFILES) - names):
            errors.append(f"missing profile: {name}")
        for name in sorted(names & set(EXPECTED_PROFILES)):
            record = profiles[name]
            if not isinstance(record, dict):
                errors.append(f"profile {name} must be an object")
                continue
            configuration, scope = EXPECTED_PROFILES[name]
            if record.get("configuration") != configuration or record.get("scope") != scope:
                errors.append(f"profile {name} has the wrong configuration or scope")
            if record.get("status") != "PASS":
                errors.append(f"profile {name} did not pass admission")
            if not isinstance(record.get("test_count"), int) or record["test_count"] <= 0:
                errors.append(f"profile {name} has no positive test count")

    toolchain = data.get("toolchain")
    if not isinstance(toolchain, dict):
        errors.append("toolchain must be an object")
    else:
        for name in sorted(REQUIRED_TOOLS):
            if not isinstance(toolchain.get(name), str) or not toolchain[name].strip():
                errors.append(f"missing toolchain field: {name}")

    if data.get("goal4_contract") != "PASS":
        errors.append("Goal 4 completion is not PASS")
    if verify_goal4:
        errors.extend(f"Goal 4 contract: {error}" for error in goal4_errors())
    return errors


def positive_fixture() -> dict[str, object]:
    return {
        "schema_version": 1,
        "source_sha": "a" * 40,
        "production_module_count": 3,
        "public_header_count": 5,
        "public_api_count": 8,
        "profiles": {
            name: {"configuration": configuration, "scope": scope,
                   "status": "PASS", "test_count": 1}
            for name, (configuration, scope) in EXPECTED_PROFILES.items()
        },
        "toolchain": {name: "fixture" for name in REQUIRED_TOOLS},
        "goal4_contract": "PASS",
    }


def self_test() -> None:
    base = positive_fixture()
    assert not validate(base, (3, 5, 8), verify_goal4=False)
    delta = {'schema_version':1, 'reviewed':True, 'baseline_source_sha':base['source_sha'],
             'baseline_api_ids':[str(i) for i in range(8)], 'added_api_ids':['new']}
    actual = set(delta['baseline_api_ids'] + ['new'])
    assert not validate_surface_evolution(base, actual, delta)
    for field, value in [('reviewed',False), ('baseline_source_sha','b'*40), ('added_api_ids',['new','new']),
                         ('baseline_api_ids',['missing']), ('added_api_ids',['unreviewed'])]:
        altered = copy.deepcopy(delta); altered[field] = value
        assert validate_surface_evolution(base, actual, altered), field

    fixtures: list[tuple[str, dict[str, object], str]] = []
    missing_sha = copy.deepcopy(base)
    missing_sha.pop("source_sha")
    fixtures.append(("missing source SHA", missing_sha, "source_sha"))
    wrong_count = copy.deepcopy(base)
    wrong_count["production_module_count"] = 4
    fixtures.append(("wrong production count", wrong_count, "production_module_count"))
    missing_goal4 = copy.deepcopy(base)
    missing_goal4["goal4_contract"] = "BLOCKED"
    fixtures.append(("missing Goal 4 completion", missing_goal4, "Goal 4 completion"))
    unknown_profile = copy.deepcopy(base)
    unknown_profile["profiles"]["mystery-debug"] = {
        "configuration": "Debug", "scope": "mystery", "status": "PASS", "test_count": 1}
    fixtures.append(("unknown profile", unknown_profile, "unknown profile"))
    missing_tool = copy.deepcopy(base)
    missing_tool["toolchain"].pop("compiler")
    fixtures.append(("missing toolchain", missing_tool, "missing toolchain"))
    for name, fixture, expected in fixtures:
        errors = validate(fixture, (3, 5, 8), verify_goal4=False)
        assert any(expected in error for error in errors), (name, errors)

    try:
        json.loads('{"profiles":{"base-debug":{},"base-debug":{}}}',
                   object_pairs_hook=reject_duplicate_keys)
    except DuplicateKeyError:
        pass
    else:
        raise AssertionError("duplicate profile fixture was accepted")
    print("PASS: admission baseline and reviewed evolution accepted; 6 malformed baseline and 5 invalid evolution fixtures rejected.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    try:
        data = load_json(ADMISSION)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"BLOCKED: cannot load Goal 5 admission: {error}", file=sys.stderr)
        return 1
    errors = validate(data)
    if errors:
        for error in errors:
            print(f"BLOCKED: {error}", file=sys.stderr)
        return 1
    modules, headers, callables = current_counts()
    print(f"PASS: historical Goal 5 admission baseline ({data['public_api_count']} APIs, 6 profiles) is preserved; "
          f"current reviewed surface is {modules} modules, {headers} headers, {callables} APIs.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
