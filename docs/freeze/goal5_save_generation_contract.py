#!/usr/bin/env python3
from __future__ import annotations

import argparse
import copy
import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
CONTRACT = ROOT / "docs/freeze/goal5_save_generation_contract.json"
REVIEW = ROOT / "_goal5_handoff/save_generation_contract_review.json"
FAILURE_REVIEW = ROOT / "_goal5_handoff/save_generation_failure_review.json"
PERSISTENCE = ROOT / "docs/freeze/goal5_persistence_contract.json"
PARTICIPANTS = ROOT / "docs/freeze/goal5_participant_contract.json"
INTEGRATION_CMAKE = ROOT / "EngineFramework/IntegrationLayer/Integration/CMakeLists.txt"
ARCH_CMAKE = ROOT / "cmake/ArchitectureFreeze.cmake"
SAVEGAME_HEADER = ROOT / "EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h"
INTEGRATION_HEADER = ROOT / "EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h"
GENERATION_SOURCE = ROOT / "EngineFramework/IntegrationLayer/Integration/src/save_generation.cpp"
RUNTIME_SUPPORT_HEADER = ROOT / "EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h"
RUNTIME_PACKAGE_SOURCE = ROOT / "EngineRuntime/Support/src/runtime_packages.cpp"
SAVE_GENERATION_TEST = ROOT / "EngineFramework/DevelopmentInfrastructure/Tests/save_generation_tests.cpp"
FRAMEWORK_TEST_CMAKE = ROOT / "EngineFramework/DevelopmentInfrastructure/Tests/CMakeLists.txt"

ALLOWED_POLICIES = {
    "QUIESCE",
    "SNAPSHOT_ISOLATED",
    "DURABLE_PENDING",
    "CANCEL_BEFORE_ACCEPT",
    "BLOCK_SAVE",
    "NON_SEMANTIC",
}
REQUIRED_ACTIVITY_POLICY = {
    "AUTHORITATIVE_MUTATIONS": "QUIESCE",
    "ACCEPTED_EXTERNAL_WORK": "DURABLE_PENDING",
    "PENDING_QUEUES": "DURABLE_PENDING",
    "DEFERRED_WORK": "DURABLE_PENDING",
    "RUNTIME_PERSISTENCE_TRANSACTION": "QUIESCE",
    "RUNTIME_TIME_STATE": "QUIESCE",
    "FRAMEWORK_TIME_STATE": "QUIESCE",
    "STREAMING_TRANSITIONS": "BLOCK_SAVE",
    "RUNTIMEBRIDGE_MATERIALIZATION": "BLOCK_SAVE",
    "INTEGRATION_RECONCILIATION": "DURABLE_PENDING",
}
REQUIRED_PLANES = {"FRAMEWORK_SAVEGAME", "RUNTIME_TIME", "RUNTIME_PERSISTENCE"}
DURABLE_ROLES = {"PERSISTENT_OWNER", "PERSISTENT_CHECKPOINT"}


class ContractError(RuntimeError):
    pass


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8-sig") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ContractError(f"{path.relative_to(ROOT)} must contain a JSON object")
    return value


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def nonempty(record: dict[str, Any], key: str, where: str) -> str:
    value = record.get(key)
    require(isinstance(value, str) and bool(value.strip()), f"{where}: missing/non-string {key}")
    return value


def validate_data(contract: dict[str, Any], persistence: dict[str, Any], participants: dict[str, Any], *, source_checks: bool) -> None:
    require(contract.get("schema_version") == 1, "save-generation contract schema_version must be 1")
    require(contract.get("status") == "IMPLEMENTED", "5.3.B contract must be IMPLEMENTED")
    require(contract.get("stage") == "5.3.B", "contract stage must be 5.3.B")

    cut = contract.get("consistent_cut")
    require(isinstance(cut, dict), "consistent_cut must be an object")
    require(cut.get("mechanism") == "HYBRID_QUIESCENCE_DURABLE_PENDING", "current architecture requires the reviewed hybrid quiescence + durable-pending mechanism")
    for key in ("capture_point", "mutation_rule", "accepted_work_rule", "runtime_time_rule", "framework_time_rule", "runtime_persistence_rule", "framework_participant_rule", "release_rule"):
        nonempty(cut, key, "consistent_cut")

    identity = contract.get("generation_identity")
    require(isinstance(identity, dict), "generation_identity must be an object")
    require(identity.get("name") == "SaveGenerationId", "generation identity must be SaveGenerationId")
    require(identity.get("existing_identity_sufficient") is False, "current tree has no stable cross-plane generation identity")
    require(identity.get("representation") == "uint64", "SaveGenerationId representation must be uint64")
    require(identity.get("invalid_value") == 0, "SaveGenerationId zero must be invalid")
    require(identity.get("stable_compare") is True, "SaveGenerationId must support stable comparison")
    require(identity.get("pointer_derived") is False, "SaveGenerationId must not be pointer-derived")
    require(identity.get("wall_clock_derived") is False, "SaveGenerationId must not be wall-clock-derived")
    require(identity.get("source_of_truth") == "whole-generation envelope manifest", "whole-generation envelope must be the single generation source of truth")
    require(identity.get("save_context_extension_required") is False, "5.3.A does not justify widening SaveContext for generation metadata")
    require(identity.get("implementation_stage") == "5.3.B", "SaveGenerationId production implementation belongs to 5.3.B")
    for key in ("allocation_rule", "authoritative_owner", "serialized_in", "restore_validation"):
        nonempty(identity, key, "generation_identity")

    publication = contract.get("publication")
    require(isinstance(publication, dict), "publication must be an object")
    require(publication.get("strategy") == "SINGLE_ATOMIC_GENERATION_PACKAGE", "half-generation prevention requires one atomically published outer generation package")
    payloads = publication.get("plane_payloads")
    require(isinstance(payloads, list) and set(payloads) == REQUIRED_PLANES and len(payloads) == len(REQUIRED_PLANES), "outer package must contain exactly the three reviewed save planes")
    for key in ("rationale", "framework_payload", "runtime_time_payload", "runtime_persistence_payload", "commit_boundary", "failed_publish_rule", "standalone_runtime_packages"):
        nonempty(publication, key, "publication")

    activities = contract.get("activities")
    require(isinstance(activities, list), "activities must be a list")
    by_name: dict[str, dict[str, Any]] = {}
    for record in activities:
        require(isinstance(record, dict), "activity entry must be an object")
        name = nonempty(record, "activity", "activity")
        require(name not in by_name, f"duplicate activity {name}")
        policy = nonempty(record, "policy", f"activity {name}")
        require(policy in ALLOWED_POLICIES, f"activity {name}: unknown policy {policy}")
        for key in ("scope", "cut_rule", "resume_rule"):
            nonempty(record, key, f"activity {name}")
        by_name[name] = record
    require(set(by_name) == set(REQUIRED_ACTIVITY_POLICY), "activity inventory must contain exactly the ten execution-plan categories")
    for name, policy in REQUIRED_ACTIVITY_POLICY.items():
        require(by_name[name].get("policy") == policy, f"activity {name} must use reviewed policy {policy}")

    candidates = persistence.get("candidates")
    require(isinstance(candidates, dict), "persistence contract candidates missing")
    planes = contract.get("planes")
    require(isinstance(planes, list), "planes must be a list")
    plane_by_name: dict[str, dict[str, Any]] = {}
    for plane in planes:
        require(isinstance(plane, dict), "plane entry must be an object")
        name = nonempty(plane, "plane", "plane")
        require(name not in plane_by_name, f"duplicate plane {name}")
        require(plane.get("capture_policy") == "QUIESCE", f"plane {name} must be captured under quiescence")
        nonempty(plane, "generation_binding", f"plane {name}")
        plane_by_name[name] = plane
    require(set(plane_by_name) == REQUIRED_PLANES, "plane inventory must contain Framework, Runtime Time, and Runtime Persistence exactly once")

    framework_durable = [c for c in candidates.values() if c.get("save_plane") == "FRAMEWORK_SAVEGAME" and c.get("persistence_role") in DURABLE_ROLES]
    framework_owners = sum(c.get("persistence_role") == "PERSISTENT_OWNER" for c in framework_durable)
    framework_checkpoints = sum(c.get("persistence_role") == "PERSISTENT_CHECKPOINT" for c in framework_durable)
    framework_plane = plane_by_name["FRAMEWORK_SAVEGAME"]
    require(framework_plane.get("current_candidate_count") == len(framework_durable), "Framework plane candidate count is stale")
    require(framework_plane.get("owner_count") == framework_owners, "Framework plane owner count is stale")
    require(framework_plane.get("checkpoint_count") == framework_checkpoints, "Framework plane checkpoint count is stale")

    runtime_time_expected = {cid for cid, c in candidates.items() if c.get("module") == "EngineRuntime/Time" and c.get("save_plane") == "RUNTIME_PERSISTENCE" and c.get("persistence_role") in DURABLE_ROLES}
    runtime_persistence_expected = {cid for cid, c in candidates.items() if c.get("module") == "EngineRuntime/Persistence" and c.get("save_plane") == "RUNTIME_PERSISTENCE" and c.get("persistence_role") in DURABLE_ROLES}
    require(set(plane_by_name["RUNTIME_TIME"].get("candidate_ids", [])) == runtime_time_expected, "Runtime Time plane candidate mapping is stale/incomplete")
    require(set(plane_by_name["RUNTIME_PERSISTENCE"].get("candidate_ids", [])) == runtime_persistence_expected, "Runtime Persistence plane candidate mapping is stale/incomplete")

    participant_rows = participants.get("participants")
    require(isinstance(participant_rows, list), "participant contract participants missing")
    require(len(participant_rows) == framework_owners, "Framework participant count no longer matches durable owner count; 5.3.A design requires re-review")

    architecture = contract.get("architecture")
    require(isinstance(architecture, dict), "architecture must be an object")
    require(architecture.get("coordinator_module") == "EngineFramework/IntegrationLayer/Integration", "coordinator placement drifted")
    require(architecture.get("coordinator_target") == "EpidemicGameFrameworkCoreIntegration", "coordinator target drifted")
    require(architecture.get("existing_allowed_runtime_dependency") == "EpidemicRuntimeTime", "existing Runtime dependency assumption drifted")
    require(architecture.get("direct_runtime_persistence_dependency_allowed") is False, "direct Integration -> Runtime Persistence dependency is forbidden")
    require(architecture.get("direct_runtime_support_dependency_allowed") is False, "direct Integration -> Runtime Support dependency is forbidden")
    require(architecture.get("runtime_dependency_expansion_required") is False, "5.3.A design must not expand Runtime dependency allowlists")
    require(architecture.get("narrow_injected_port_required") is True, "narrow injected Runtime coordination port is required")
    require(architecture.get("planned_port") == "IRuntimeSaveGenerationPort", "unexpected Runtime coordination port identity")
    require(architecture.get("new_production_module_required") is False, "5.3.A must not introduce a new production module")
    require(architecture.get("architecture_allowlist_change_required") is False, "5.3.A must preserve architecture allowlists")
    nonempty(architecture, "port_contract", "architecture")
    nonempty(architecture, "port_implementation_placement", "architecture")

    handoff = contract.get("stage_b_handoff")
    require(isinstance(handoff, dict), "stage_b_handoff must be an object")
    for key in ("required_before_coding", "implementation_obligations"):
        value = handoff.get(key)
        require(isinstance(value, list) and value and all(isinstance(item, str) and item.strip() for item in value), f"stage_b_handoff.{key} must be a nonempty string list")
    require(handoff.get("status") == "COMPLETE", "5.3.B handoff must be marked COMPLETE after implementation")
    completed = handoff.get("completed_obligations")
    require(isinstance(completed, list) and completed == handoff.get("implementation_obligations"), "5.3.B completed obligations must exactly match the reviewed implementation obligations")

    implementation = contract.get("implementation")
    require(isinstance(implementation, dict), "implementation evidence must be an object")
    required_impl = {
        "generation_header": "EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h",
        "generation_source": "EngineFramework/IntegrationLayer/Integration/src/save_generation.cpp",
        "runtime_wire_header": "EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h",
        "runtime_wire_source": "EngineRuntime/Support/src/runtime_packages.cpp",
        "coordinator": "SaveGenerationCoordinator::CaptureAndCommit",
        "barrier": "SaveGenerationBarrier",
        "runtime_port": "IRuntimeSaveGenerationPort",
        "runtime_port_adapter": "CallbackRuntimeSaveGenerationPort",
        "generation_store": "ISaveGenerationStore",
        "production_file_store": "FileSaveGenerationStore",
        "outer_encoder": "EncodeSaveGenerationPackage",
        "outer_decoder": "DecodeSaveGenerationPackage",
        "runtime_time_encoder": "EncodeRuntimeTimeCheckpointPackage",
        "runtime_persistence_encoder": "EncodeRuntimePersistencePackage",
        "runtime_persistence_capture": "CaptureRuntimePersistenceSnapshot",
        "test_target": "EpidemicGameFrameworkSaveGenerationTests",
        "test_source": "EngineFramework/DevelopmentInfrastructure/Tests/save_generation_tests.cpp",
        "test_symbol": "RunSaveGenerationTests",
    }
    for key, expected in required_impl.items():
        require(implementation.get(key) == expected, f"implementation.{key} drifted")
    require(implementation.get("public_save_context_changed") is False, "SaveContext must not be widened in 5.3.B")
    require(implementation.get("architecture_allowlist_changed") is False, "5.3.B must not widen architecture allowlists")
    require(implementation.get("production_module_count_changed") is False, "5.3.B must not add a production module")

    if source_checks:
        integration_cmake = INTEGRATION_CMAKE.read_text(encoding="utf-8-sig")
        arch_cmake = ARCH_CMAKE.read_text(encoding="utf-8-sig")
        savegame_header = SAVEGAME_HEADER.read_text(encoding="utf-8-sig")
        integration_header = INTEGRATION_HEADER.read_text(encoding="utf-8-sig")
        generation_source = GENERATION_SOURCE.read_text(encoding="utf-8-sig")
        runtime_support_header = RUNTIME_SUPPORT_HEADER.read_text(encoding="utf-8-sig")
        runtime_package_source = RUNTIME_PACKAGE_SOURCE.read_text(encoding="utf-8-sig")
        generation_test = SAVE_GENERATION_TEST.read_text(encoding="utf-8-sig")
        test_cmake = FRAMEWORK_TEST_CMAKE.read_text(encoding="utf-8-sig")
        require("EpidemicRuntimeTime" in integration_cmake, "Integration no longer links the reviewed Runtime Time dependency")
        require("EpidemicRuntimePersistence" not in integration_cmake, "Integration gained forbidden direct Runtime Persistence dependency")
        require("EpidemicRuntimeSupport" not in integration_cmake, "Integration gained forbidden direct Runtime Support dependency")
        require("src/save_generation.cpp" in integration_cmake, "Integration target does not compile the 5.3.B coordinator")
        require('"EpidemicGameFrameworkCoreIntegration:EpidemicRuntimeTime"' in arch_cmake, "architecture allowlist no longer contains the reviewed Integration -> Runtime Time pair")
        require("EpidemicGameFrameworkCoreIntegration:EpidemicRuntimePersistence" not in arch_cmake, "architecture allowlist was widened to Runtime Persistence")
        require("SaveGenerationId" not in savegame_header, "SaveGenerationId must remain outside SaveGame public participant surface")
        require("generation" not in _struct_body(savegame_header, "SaveContext"), "SaveContext unexpectedly contains generation metadata; re-review participant semantics")
        for symbol in ("SaveGenerationId", "IRuntimeSaveGenerationPort", "CallbackRuntimeSaveGenerationPort",
                       "SaveGenerationBarrier", "SaveGenerationCoordinator", "ISaveGenerationStore",
                       "FileSaveGenerationStore", "EncodeSaveGenerationPackage", "DecodeSaveGenerationPackage"):
            require(symbol in integration_header, f"5.3.B public integration contract missing {symbol}")
        for symbol in ("SaveGenerationCoordinator::CaptureAndCommit", "SaveGenerationBarrier::AcquireGenerationLease",
                       "SaveGenerationBarrier::AcquireCaptureLease", "FileSaveGenerationStore::CommitCurrent",
                       "gameplay.save_generation.generation_mismatch"):
            require(symbol in generation_source, f"5.3.B implementation missing {symbol}")
        require("platform::CommitFilePackage" in generation_source, "whole-generation file store does not use the existing atomic replacement primitive")
        for symbol in ("EncodeRuntimeTimeCheckpointPackage", "DecodeRuntimeTimeCheckpointPackage",
                       "EncodeRuntimePersistencePackage", "DecodeRuntimePersistencePackage",
                       "CaptureRuntimePersistenceSnapshot"):
            require(symbol in runtime_support_header and symbol in runtime_package_source, f"Runtime canonical in-memory package bridge missing {symbol}")
        require("RunSaveGenerationTests" in generation_test, "5.3.B coordination regression symbol is missing")
        require("outer lease owns one safe point" in generation_test, "5.3.B test does not prove nested SaveGame capture reuses the outer safe point")
        require("failed outer commit preserves previous authoritative generation" in generation_test, "5.3.B test does not prove failed publication preserves the old generation")
        require("EpidemicGameFrameworkSaveGenerationTests" in test_cmake, "5.3.B test target is not registered")


def _struct_body(text: str, struct_name: str) -> str:
    marker = f"struct {struct_name}"
    start = text.find(marker)
    if start < 0:
        return ""
    open_brace = text.find("{", start)
    close = text.find("};", open_brace)
    if open_brace < 0 or close < 0:
        return ""
    return text[open_brace + 1:close]


def run_check() -> None:
    contract = load_json(CONTRACT)
    require(contract == load_json(REVIEW), "generated contract differs from reviewed handoff; run --record")
    validate_data(contract, load_json(PERSISTENCE), load_json(PARTICIPANTS), source_checks=True)
    validate_failure_review(load_json(FAILURE_REVIEW), source_checks=True)
    print("goal5 save-generation 5.3.B implementation contract: PASS")


def validate_failure_review(review: dict[str, Any], *, source_checks: bool) -> None:
    require(review.get("schema_version") == 1 and review.get("stage") == "5.3.C", "invalid save failure review schema/stage")
    require(review.get("test_target") == "EpidemicGameFrameworkSaveGenerationTests", "failure evidence target is not the registered generation suite")
    require(review.get("test_source") == str(SAVE_GENERATION_TEST.relative_to(ROOT)).replace("\\", "/"), "failure evidence source drifted")
    expected = {
        "LEASE_PREPARATION_ATOMICITY": "LeasePreparationFailureDoesNotStrandQuiescence",
        "SAVE_FAILURE_ATOMICITY": "SaveFailureMatrixPreservesPublishedGeneration",
        "DURABLE_PENDING_EXACTLY_ONCE": "DurablePendingGenerationRestoresOnlyUnfinishedDelivery",
        "LEASE_LIFETIME": "NestedLeaseLifetimeAndWrongThreadAreControlled",
        "LINEAGE_MONOTONICITY": "RunSaveGenerationTests",
    }
    obligations = review.get("obligations", {})
    require(set(obligations) == set(expected), "missing or unknown save failure obligation")
    for obligation, symbol in expected.items():
        row = obligations[obligation]
        require(row.get("test_symbol") == symbol, f"{obligation}: missing exact behavioral test identity")
        require(len(nonempty(row, "assertion", obligation)) >= 40, f"{obligation}: generic assertion")
    defects = review.get("defects", {})
    require(set(defects) == {"G5-BARRIER-001", "G5-SAVE-001"}, "missing reviewed save defects")
    for identity, row in defects.items():
        require(row.get("review_status") == "REVIEWED", f"{identity}: defect must be reviewed")
        require(row.get("regression") in expected.values(), f"{identity}: invalid regression")
        for field in ("root_cause", "fix", "compatibility_impact"):
            nonempty(row, field, identity)
    if source_checks:
        source = SAVE_GENERATION_TEST.read_text(encoding="utf-8-sig")
        for symbol in expected.values():
            require(re.search(r"bool\s+" + re.escape(symbol) + r"\s*\(\s*\)\s*\{", source) is not None,
                    f"missing executable failure test definition {symbol}")
            require(re.search(r"\b" + re.escape(symbol) + r"\(\)\s*(?:&&|\?)", source) is not None,
                    f"failure test is defined but not executed: {symbol}")
        for point in ("acquire", "preflight", "framework", "time", "persistence", "package.encode", "store", "exception"):
            require(f'"{point}"' in source, f"failure matrix missing logical point {point}")
        require(review["test_target"] in FRAMEWORK_TEST_CMAKE.read_text(encoding="utf-8-sig"), "failure test target not registered")
    print("goal5 save-generation typed failure evidence: PASS (execution receipt is separately required)")


def expect_failure(contract: dict[str, Any], persistence: dict[str, Any], participants: dict[str, Any], mutate, name: str) -> None:
    broken = copy.deepcopy(contract)
    mutate(broken)
    try:
        validate_data(broken, persistence, participants, source_checks=False)
    except ContractError:
        return
    raise AssertionError(f"negative fixture unexpectedly passed: {name}")


def run_self_test() -> None:
    contract = load_json(CONTRACT)
    persistence = load_json(PERSISTENCE)
    participants = load_json(PARTICIPANTS)
    validate_data(contract, persistence, participants, source_checks=False)

    expect_failure(contract, persistence, participants,
                   lambda c: c["activities"].pop(), "missing activity")
    expect_failure(contract, persistence, participants,
                   lambda c: c["activities"].append(copy.deepcopy(c["activities"][0])), "duplicate activity")
    expect_failure(contract, persistence, participants,
                   lambda c: c["activities"][-3].update(policy="NON_SEMANTIC"), "streaming not blocked")
    expect_failure(contract, persistence, participants,
                   lambda c: c["generation_identity"].update(pointer_derived=True), "pointer-derived generation")
    expect_failure(contract, persistence, participants,
                   lambda c: c["generation_identity"].update(wall_clock_derived=True), "wall-clock generation")
    expect_failure(contract, persistence, participants,
                   lambda c: c["generation_identity"].update(source_of_truth="runtime time"), "duplicate/wrong generation source")
    expect_failure(contract, persistence, participants,
                   lambda c: c["publication"].update(strategy="THREE_INDEPENDENT_REPLACEMENTS"), "half-generation publication")
    expect_failure(contract, persistence, participants,
                   lambda c: c["planes"].pop(), "missing plane")
    expect_failure(contract, persistence, participants,
                   lambda c: c["architecture"].update(direct_runtime_persistence_dependency_allowed=True), "illegal Runtime Persistence dependency")
    expect_failure(contract, persistence, participants,
                   lambda c: c["architecture"].update(new_production_module_required=True), "hidden production module")
    expect_failure(contract, persistence, participants,
                   lambda c: c["generation_identity"].update(save_context_extension_required=True), "unjustified SaveContext widening")
    expect_failure(contract, persistence, participants,
                   lambda c: c.update(status="DESIGN_FROZEN"), "implementation status regression")
    expect_failure(contract, persistence, participants,
                   lambda c: c["implementation"].update(public_save_context_changed=True), "public SaveContext widening")
    expect_failure(contract, persistence, participants,
                   lambda c: c["implementation"].update(architecture_allowlist_changed=True), "hidden architecture expansion")
    expect_failure(contract, persistence, participants,
                   lambda c: c["stage_b_handoff"].update(status="IN_PROGRESS"), "incomplete stage B")
    failure = load_json(FAILURE_REVIEW)
    validate_failure_review(failure, source_checks=False)
    for mutate in (
        lambda f: f["obligations"].pop("LEASE_PREPARATION_ATOMICITY"),
        lambda f: f["obligations"]["SAVE_FAILURE_ATOMICITY"].update(test_symbol="main"),
        lambda f: f["obligations"]["LEASE_LIFETIME"].update(assertion="works"),
        lambda f: f.update(test_target="NotRegistered"),
        lambda f: f["defects"]["G5-SAVE-001"].update(review_status="PASS"),
        lambda f: f["defects"]["G5-BARRIER-001"].update(regression="main"),
    ):
        broken = copy.deepcopy(failure)
        mutate(broken)
        try:
            validate_failure_review(broken, source_checks=False)
        except ContractError:
            continue
        raise AssertionError("broken failure evidence fixture unexpectedly passed")
    print("goal5 save-generation 5.3.B contract self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--record", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if not args.check and not args.self_test and not args.record:
        parser.error("choose --record, --check and/or --self-test")
    try:
        if args.record:
            contract = load_json(REVIEW)
            validate_data(contract, load_json(PERSISTENCE), load_json(PARTICIPANTS), source_checks=True)
            CONTRACT.write_text(json.dumps(contract, indent=2) + "\n", encoding="utf-8", newline="\n")
            print("Recorded save-generation contract from reviewed handoff.")
        if args.check:
            run_check()
        if args.self_test:
            run_self_test()
    except (ContractError, AssertionError, OSError, json.JSONDecodeError) as error:
        print(f"goal5 save-generation contract: FAIL: {error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

