"""Reviewed placement and durable boundary decisions for M5.2.A.

The explicit owners below, not directory-wide fallback rules, define the mapping.
Recording is an explicit review/generator operation; checks never call record_review.
"""
from __future__ import annotations

import argparse
import copy
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
from module_dossiers import ROOT
from goal5_persistence_inventory import matching_brace, strip_comments, namespace_at
from goal5_persistence_contract import encoded, load_json

# owner module, namespace, owner, snapshot, existing integration placement
OWNER_BINDINGS = [
    ("BaseInfrastructure/Facts", "facts", "GameplayFactsService", "FactsPersistenceSnapshot", "Integration"),
    ("BaseInfrastructure/Time", "time", "GameplayTimeService", "GameplayTimeSnapshot", "Integration"),
    ("Abilities", "abilities", "AbilityService", "AbilitiesSnapshot", "GameplayIntegration"),
    ("AI", "ai", "AIService", "AISnapshot", "PerceptionKnowledgeAIIntegration"),
    ("Combat", "combat", "CombatService", "CombatSnapshot", "GameplayIntegration"),
    ("Conditions", "conditions", "ConditionService", "ConditionsSnapshot", "StateIntegration"),
    ("Construction", "construction", "ConstructionService", "ConstructionSnapshot", "TraversalNavigationConstructionIntegration"),
    ("Crime", "crime", "CrimeService", "CrimeSnapshot", "SocialLegalIntegration"),
    ("Dialogue", "dialogue", "DialogueService", "DialogueSnapshot", "ExtendedGameplayIntegration"),
    ("Economy", "economy", "EconomyService", "EconomySnapshot", "ExtendedGameplayIntegration"),
    ("Effects", "effects", "EffectService", "EffectsSnapshot", "StateIntegration"),
    ("Encounters", "encounters", "EncountersService", "EncountersSnapshot", "PopulationSimulationIntegration"),
    ("Entities", "entities", "EntityService", "EntitySnapshot", "StateIntegration"),
    ("Environment", "environment", "EnvironmentService", "EnvironmentSnapshot", "WorldIntegration"),
    ("Equipment", "equipment", "EquipmentService", "EquipmentSnapshot", "ExtendedGameplayIntegration"),
    ("Interaction", "interaction", "InteractionService", "InteractionSnapshot", "WorldIntegration"),
    ("ItemsInventory", "items", "ItemsInventoryService", "ItemsSnapshot", "ExtendedGameplayIntegration"),
    ("Knowledge", "knowledge", "KnowledgeService", "KnowledgeSnapshot", "PerceptionKnowledgeAIIntegration"),
    ("Loot", "loot", "LootService", "LootSnapshot", "GameplayIntegration"),
    ("Materials", "materials", "MaterialService", "MaterialsSnapshot", "StateIntegration"),
    ("Narrative", "narrative", "NarrativeService", "NarrativeSnapshot", "NarrativeIntegration"),
    ("NavigationSemantics", "navigation_semantics", "NavigationSemanticsService", "NavigationSnapshot", "TraversalNavigationConstructionIntegration"),
    ("NeedsLife", "needs_life", "NeedsLifeService", "NeedsLifeSnapshot", "PopulationSimulationIntegration"),
    ("Ownership", "ownership", "OwnershipService", "OwnershipSnapshot", "SocialLegalIntegration"),
    ("Perception", "perception", "PerceptionService", "PerceptionSnapshot", "PerceptionKnowledgeAIIntegration"),
    ("Population", "population", "PopulationService", "PopulationSnapshot", "PopulationSimulationIntegration"),
    ("Processes", "processes", "ProcessesService", "ProcessesSnapshot", "ProcessResourceSimulationIntegration"),
    ("Progression", "progression", "ProgressionService", "ProgressionSnapshot", "GameplayIntegration"),
    ("ResourcesProduction", "resources", "ResourcesProductionService", "ResourcesSnapshot", "ProcessResourceSimulationIntegration"),
    ("RolesJobs", "roles_jobs", "RolesJobsService", "RolesJobsSnapshot", "PopulationSimulationIntegration"),
    ("Simulation", "simulation", "SimulationService", "SimulationSnapshot", "ProcessResourceSimulationIntegration"),
    ("Society", "society", "SocietyService", "SocietySnapshot", "SocialLegalIntegration"),
    ("Traversal", "traversal", "TraversalService", "TraversalSnapshot", "TraversalNavigationConstructionIntegration"),
    ("World", "world", "WorldService", "WorldSnapshot", "WorldIntegration"),
]

# integration, namespace, owner, root checkpoint, stable participant ID, dependencies
INTEGRATION_BINDINGS = [
    ("Integration", "integration", "ScheduledTriggerDispatcher", "ScheduledTriggerDispatcherSnapshot",
     "integration.scheduled_trigger_dispatcher", []),
    ("NarrativeIntegration", "narrative_integration", "NarrativeExternalConsequenceOutbox", "NarrativeExternalConsequenceSnapshot",
     "integration.narrative.external_consequence_delivery", []),
    ("ExtendedGameplayIntegration", "integration", "TradeCoordinator", "TradeCoordinatorSnapshot", "integration.trade",
     ["framework.economy", "framework.items", "framework.ownership"]),
    ("GameplayIntegration", "integration", "AbilityEffectsDispatcher", "AbilityEffectsCheckpoint", "integration.ability_effects",
     ["framework.abilities", "framework.effects"]),
    ("GameplayIntegration", "integration", "AbilityTimeAdapter", "AbilityTimeCheckpoint", "integration.ability_time",
     ["framework.abilities", "framework.time"]),
    ("GameplayIntegration", "integration", "ConditionProgressionAdapter", "ConditionProgressionCheckpoint", "integration.condition_progression",
     ["framework.conditions", "framework.progression"]),
    ("GameplayIntegration", "integration", "DeathRewardAdapter", "DeathRewardCheckpoint", "integration.death_reward",
     ["framework.combat", "framework.loot"]),
    ("InteractionEffectsIntegration", "interaction_effects_integration", "InteractionEffectExecutor", "InteractionEffectsSnapshot",
     "integration.interaction_effects", ["framework.interaction", "framework.effects"]),
    ("PopulationSimulationIntegration", "population_simulation", "PopulationEncounterAdapter", "PopulationEncounterAdapterSnapshot",
     "integration.population_encounter", ["framework.population", "framework.encounters"]),
    ("PopulationSimulationIntegration", "population_simulation", "PopulationLifecycleAdapter", "PopulationLifecycleAdapterSnapshot",
     "integration.population_lifecycle", ["framework.population", "framework.roles_jobs", "framework.needs_life"]),
    ("SocialLegalIntegration", "social_legal_integration", "CrimeSocietyAdapter", "SocialLegalCheckpoint",
     "integration.social_legal", ["framework.crime", "framework.society"]),
    ("StateIntegration", "state_integration", "StateIntegrationPersistence", "StateIntegrationCheckpoint", "integration.state",
     ["framework.entities", "framework.materials", "framework.conditions", "framework.effects", "framework.facts", "framework.time"]),
    ("WorldIntegration", "world_integration", "WorldFactsAdapter", "WorldFactsCheckpoint", "integration.world_facts",
     ["framework.world", "framework.environment", "framework.interaction", "framework.facts"]),
]

# Reviewed RestoreSnapshot publication footprint, including rebuilt indexes,
# journal/generator continuity and owner-specific reconciliation state. Bootstrap
# registrations not published by RestoreSnapshot are deliberately absent.
DURABLE_MEMBERS = {
    "GameplayTimeService": "clocks_ schedule_ids_ schedule_index_ scheduler_revision_ schedules_ synchronization_rebind_required_ synchronization_revisions_",
    "AbilityService": "changes_ cooldowns_ execution_ids_ executions_ instance_ids_ instances_ journal_epoch_ next_change_sequence_ reconciled_reservations_ resource_reconciliation_required_ schedule_to_execution_ diagnostics_",
    "AIService": "agents_ changes_ diagnostics_ due_agents_ intent_ids_ intent_to_agent_ journal_epoch_ next_change_sequence_ revision_ tick_budget_",
    "CombatService": "changes_ combatants_ journal_epoch_ next_change_sequence_ prepared_plans_ resolution_ids_ resource_reservation_ids_ resource_reservations_ diagnostics_",
    "ConditionService": "applied_ changes_ expired_ id_to_index_ ids_ instances_ journal_epoch_ next_change_sequence_ periodic_triggers_ removed_ revision_ stack_merges_ subject_index_",
    "ConstructionService": "changes_ dead_letters_ execution_ids_ journal_epoch_ next_change_sequence_ outbox_ output_ids_ placed_ids_ placed_objects_ plan_ids_ plans_ revision_ site_ids_ sites_ socket_reservation_by_socket_ socket_reservation_ids_ socket_reservations_by_id_ sockets_",
    "CrimeService": "active_bounty_index_ authorities_ authority_ids_ bounties_ bounty_ids_ changes_ crime_ids_ crimes_ definitions_frozen_ diagnostics_ evidence_ evidence_ids_ journal_epoch_ jurisdictions_ laws_ next_change_sequence_ response_definitions_ response_ids_ responses_ revision_ witness_ids_ witnesses_",
    "DialogueService": "changes_ consequence_ids_ consequences_ journal_epoch_ next_change_sequence_ revision_ session_ids_ sessions_ diagnostics_",
    "EconomyService": "account_ids_ accounts_ changes_ contract_ids_ contracts_ debt_ids_ debts_ diagnostics_ indicators_ journal_epoch_ market_ids_ markets_ next_change_sequence_ offer_ids_ offers_ reservation_ids_ reservations_ revision_ transaction_ids_ transactions_",
    "EffectService": "budget_exhaustions_ changes_ deferred_ deferred_by_schedule_ deferred_ids_ derived_effects_ execution_ids_ executions_ journal_epoch_ next_change_sequence_ operations_ rejections_ targets_ waves_",
    "EncountersService": "changes_ definitions_ instance_ids_ instances_ instances_by_area_ instances_by_state_ journal_epoch_ next_change_sequence_ point_ids_ points_ request_ids_ requests_ respawn_ids_ respawns_ revision_ spawn_budget_tick_ spawn_operations_used_ spawned_ spawned_ids_ tables_",
    "EntityService": "changes_ creates_ destroys_ free_slots_ id_to_slot_ ids_ invalid_handle_lookups_ journal_epoch_ last_change_sequence_ next_change_sequence_ pending_destroy_ pending_destroy_contexts_ pending_destroy_reasons_ revision_ slots_",
    "EnvironmentService": "changes_ global_layers_ ids_ journal_epoch_ large_layers_ last_change_sequence_ layers_ next_change_sequence_ revision_ spatial_index_",
    "EquipmentService": "binding_ids_ bindings_ changes_ diagnostics_ journal_epoch_ loadout_ids_ loadouts_ next_change_sequence_ operation_ids_ profile_by_subject_ profile_ids_ profiles_ reconciliation_state_ revision_ slot_ids_",
    "InteractionService": "changes_ ids_ journal_epoch_ next_change_sequence_ revision_ session_by_schedule_ sessions_",
    "ItemsInventoryService": "active_reserved_quantities_ changes_ container_ids_ container_items_ containers_ definition_items_ diagnostics_ indexes_valid_ item_ids_ items_ journal_epoch_ next_change_sequence_ reservation_ids_ reservations_ revision_ transfer_ids_",
    "KnowledgeService": "changes_ definitions_frozen_ journal_epoch_ knowledge_ knowledge_by_owner_ knowledge_by_subject_ knowledge_by_topic_ knowledge_ids_ memories_ memories_by_owner_ memory_ids_ next_change_sequence_ profiles_ revision_",
    "LootService": "changes_ claimed_ claimed_history_floor_low_ claimed_order_ execution_ids_ generated_ journal_epoch_ next_change_sequence_ pending_ diagnostics_",
    "MaterialService": "changes_ journal_epoch_ last_change_sequence_ next_change_sequence_ reaction_evaluations_ reactions_triggered_ revision_ states_ stimuli_",
    "NarrativeService": "activated_beats_ changes_ choice_ids_ choices_ clue_ids_ clues_ consequence_idempotency_ consequence_ids_ consequences_ evaluation_budget_exhausted_ evaluation_counter_ evaluation_depth_ evaluation_limit_ event_executions_ flags_ journal_ journal_epoch_ journal_ids_ next_change_sequence_ objectives_ processed_event_keys_ revision_ rumor_expiry_index_ rumor_ids_ rumors_ storylet_runtime_ threads_ variables_",
    "NavigationSemanticsService": "changes_ journal_epoch_ layer_ids_ layer_ids_by_area_ layers_ link_ids_by_pair_ links_ next_change_sequence_ profiles_ revision_",
    "NeedsLifeService": "changes_ decay_rules_ definitions_ definitions_frozen_ diagnostics_ journal_epoch_ next_change_sequence_ pressure_ids_ pressures_ pressures_by_subject_ profile_by_subject_ profile_ids_ profiles_ revision_ routine_by_subject_ routine_ids_ routines_ simulation_profiles_ states_",
    "OwnershipService": "canonical_owners_ changes_ claim_ids_ claims_ claims_by_property_ diagnostics_ grant_ids_ grants_ grants_by_subject_ journal_epoch_ next_change_sequence_ ownership_by_owner_ ownership_by_property_ ownership_ids_ records_ revision_ rule_ids_ rules_ rules_by_property_",
    "PerceptionService": "awareness_ changes_ journal_epoch_ next_change_sequence_ observation_ids_ observations_ observations_by_perceiver_ pending_by_ready_ pending_observations_ perceivers_ revision_ stimuli_ stimuli_by_area_ stimulus_ids_ tick_budget_",
    "PopulationService": "active_allocation_by_unit_ active_migration_by_unit_ active_residence_by_unit_ allocation_correlation_ids_ allocation_ids_ allocations_ allocations_by_correlation_ changes_ group_ids_ groups_ journal_epoch_ migration_ids_ migrations_ next_change_sequence_ residence_ids_ residences_ revision_ unit_by_entity_ unit_ids_ units_ units_by_area_ units_by_group_ units_by_state_ units_by_template_",
    "ProcessesService": "changes_ diagnostics_ instance_ids_ instances_ journal_epoch_ next_change_sequence_ reservation_ids_ revision_ station_by_object_ station_ids_ stations_",
    "ProgressionService": "changes_ grant_reservation_ids_ journal_epoch_ modifier_ids_ next_change_sequence_ pending_progress_grants_ profiles_ revision_",
    "ResourcesProductionService": "amounts_ capabilities_ capability_ids_ changes_ diagnostics_ journal_epoch_ next_change_sequence_ node_ids_ nodes_ plan_ids_ plans_ reservation_ids_ reservations_ reserved_amounts_ revision_ site_ids_ sites_ stockpile_ids_ stockpiles_ transaction_ids_ transactions_",
    "RolesJobsService": "assignment_ids_ assignments_ assignments_by_worker_ assignments_by_workplace_ changes_ definitions_ duties_ duties_by_assignment_ duties_by_subject_ duty_ids_ journal_epoch_ next_change_sequence_ revision_ schedule_ids_ schedules_ schedules_by_assignment_ shift_ids_ workplace_ids_ workplaces_",
    "SimulationService": "active_intervals_ changes_ diagnostics_ journal_epoch_ next_change_sequence_ regions_ revision_ summaries_ summary_ids_ task_ids_",
    "SocietyService": "changes_ definitions_frozen_ groups_ journal_epoch_ membership_by_key_ membership_ids_ memberships_ memberships_by_group_ memberships_by_member_ next_change_sequence_ relationship_by_key_ relationship_ids_ relationship_types_ relationships_ relationships_by_subject_ reputation_tracks_ reputations_ reputations_by_subject_ revision_",
    "TraversalService": "capability_grant_ids_ capability_grants_ carrier_by_passenger_ changes_ journal_epoch_ next_change_sequence_ revision_ route_ids_ routes_ session_ids_ sessions_ states_",
    "WorldService": "alteration_ids_ alteration_index_ alterations_ changes_ features_ journal_epoch_ large_alterations_ last_change_sequence_ next_change_sequence_ object_placements_ revision_",
}

# Explicit adapter ports: constructor arguments, immutable configuration to
# copy before validation, and the existing Restore publication footprint.
ADAPTER_ACCESS = {
    "TradeCoordinator": ("w.Resolve(o.economy_), w.Resolve(o.items_), w.Resolve(o.ownership_)", "", "execution_ids_ executions_"),
    "AbilityEffectsDispatcher": ("w.Resolve(o.effects_)", "mappings_ mappings_frozen_ mapping_revision_", "delivered_"),
    "AbilityTimeAdapter": ("w.Resolve(o.time_), w.Resolve(o.abilities_), o.clock_, o.action_", "", "pending_"),
    "ConditionProgressionAdapter": ("w.Resolve(o.conditions_), w.Resolve(o.progression_)", "mappings_ mappings_frozen_ mapping_revision_", "cursor_"),
    "DeathRewardAdapter": ("w.Resolve(o.combat_), w.Resolve(o.loot_)", "tables_ mappings_frozen_ mapping_revision_", "cursor_ deliveries_"),
    "InteractionEffectExecutor": ("w.Resolve(o.effects_), o.interaction_type_", "definitions_ frozen_", "deliveries_"),
    "PopulationEncounterAdapter": ("o", "", "plans_"),
    "PopulationLifecycleAdapter": ("o", "", "reconciliations_"),
    "CrimeSocietyAdapter": ("w.Resolve(o.crime_), w.Resolve(o.society_), o.mappings_", "", "applied_relationship_penalties_ crime_cursor_"),
    "WorldFactsAdapter": ("w.Resolve(o.world_), w.Resolve(o.environment_), w.Resolve(o.interaction_), w.Resolve(o.facts_)", "world_event_ environment_event_ interaction_event_", "wc_ ec_ ic_"),
    "StateFactsAdapter": ("w.Resolve(o.entities_), w.Resolve(o.materials_), w.Resolve(o.conditions_), w.Resolve(o.effects_), w.Resolve(o.facts_)", "entity_changed_ material_changed_ condition_changed_ effect_changed_ active_condition_fact_", "entity_cursor_ material_cursor_ condition_cursor_ effect_cursor_"),
    "StateLifecycleAdapter": ("w.Resolve(o.entities_), w.Resolve(o.materials_), w.Resolve(o.conditions_), w.Resolve(o.effects_)", "", "cursor_"),
    "ConditionEffectsAdapter": ("w.Resolve(o.conditions_), w.Resolve(o.effects_)", "routes_", "cursor_ deliveries_"),
    "StateTimeAdapter": ("w.Resolve(o.conditions_), w.Resolve(o.effects_), w.Resolve(o.time_)", "expire_action_ periodic_action_ deferred_effect_action_ dispatcher_ effect_budget_", "condition_cursor_ effect_cursor_ deferred_reconciliations_"),
}


def bindings():
    result = []
    for module, namespace, owner, snapshot, placement in OWNER_BINDINGS:
        module = module if "/" in module else "GameplayWorldStateOwners/" + module
        result.append({"owner_module": "EngineFramework/" + module,
                       "placement": "EngineFramework/IntegrationLayer/" + placement,
                       "namespace": "epidemic::gameplay::" + namespace, "owner": owner,
                       "snapshot": snapshot, "id": "framework." + namespace,
                       "dependencies": [], "capture": "CapturePersistenceSnapshot" if namespace == "facts" else "CaptureSnapshot",
                       "restore": "RestorePersistenceSnapshot" if namespace == "facts" else "RestoreSnapshot"})
    for placement, namespace, owner, snapshot, identity, dependencies in INTEGRATION_BINDINGS:
        result.append({"owner_module": "EngineFramework/IntegrationLayer/" + placement,
                       "placement": "EngineFramework/IntegrationLayer/" + placement,
                       "namespace": "epidemic::gameplay::" + namespace, "owner": owner,
                       "snapshot": snapshot, "id": identity, "dependencies": dependencies,
                       "capture": "CaptureCheckpoint" if "Checkpoint" in snapshot else "CaptureSnapshot",
                       "restore": "RestoreCheckpoint" if "Checkpoint" in snapshot else "RestoreSnapshot"})
    return result


def state_types(module: str, root: str) -> set[str]:
    """Trace concrete DTO field types, never method bodies, helpers or enum declarations."""
    declarations = {}
    for header in sorted((ROOT / module / "include").rglob("*.h")):
        text = strip_comments(header.read_text(encoding="utf-8"))
        for match in re.finditer(r"\bstruct\s+(\w+)\s*\{", text):
            opening = text.index("{", match.start()); closing = matching_brace(text, opening)
            if closing is None:
                raise ValueError(f"unbalanced DTO: {header}")
            body = text[opening + 1:closing]
            # Methods and initializers are not state-type dependencies. Keep only
            # declaration text at the top brace level, up to each semicolon.
            depth = 0; declaration = ""; fields = []
            for char in body:
                if char == "{": depth += 1
                elif char == "}": depth -= 1
                elif depth == 0:
                    declaration += char
                    if char == ";":
                        if "(" not in declaration and not re.search(r"\b(static|using|friend|typedef)\b", declaration):
                            fields.append(declaration)
                        declaration = ""
            declarations[match.group(1)] = " ".join(fields)
    if root not in declarations:
        raise ValueError(f"missing reviewed snapshot {module}/{root}")
    reachable = set()
    def visit(name):
        if name in reachable: return
        reachable.add(name)
        for token in re.findall(r"\b\w+\b", declarations.get(name, "")):
            if token in declarations: visit(token)
    visit(root)
    return reachable


def record_review():
    persistence_path = ROOT / "docs/freeze/goal5_persistence_contract.json"
    persistence = load_json(persistence_path)
    inventory = load_json(ROOT / "docs/freeze/goal5_persistence_candidates.json")
    discovered = {c["candidate_id"]: c for c in inventory["candidates"]}
    reviewed = persistence.get("candidates", {})
    if set(discovered) != set(reviewed):
        missing = sorted(set(discovered) - set(reviewed))
        stale = sorted(set(reviewed) - set(discovered))
        details = []
        if missing:
            details.append("unreviewed persistence discoveries: " + ", ".join(missing[:8]))
        if stale:
            details.append("stale persistence review entries: " + ", ".join(stale[:8]))
        raise ValueError("; ".join(details))
    for candidate_id, item in discovered.items():
        decision = reviewed[candidate_id]
        if decision.get("module") != item.get("module") or decision.get("qualified_symbol") != item.get("qualified_symbol"):
            raise ValueError(f"persistence review identity drift: {candidate_id}")
        if decision.get("review_status") != "REVIEWED":
            raise ValueError(f"participant mapping requires a reviewed 5.1 decision: {candidate_id}")

    specs = bindings()
    reachable = {b["id"]: state_types(b["owner_module"], b["snapshot"]) for b in specs}
    per_module = {}
    for b in specs:
        per_module.setdefault(b["owner_module"], []).append(b)

    participants = []
    for b in specs:
        participants.append({**b, "requirement": "Required", "schema_version": 1,
            "decision": "EXISTING_PARTICIPANT" if b["owner"] in {
                "ScheduledTriggerDispatcher", "NarrativeExternalConsequenceOutbox"} else (
                "AGGREGATED_PARTICIPANT" if b["owner"] == "StateIntegrationPersistence" else "NEW_PARTICIPANT"),
            "rationale": "One authoritative ownership boundary: exactly the reviewed snapshot DTO, including definition tables when its owner contract persists them. Callback registrations and peer authoritative state are not serialized."
                if b["owner"] != "StateIntegrationPersistence" else
                "Existing StateIntegrationPersistence already owns the atomic four-adapter checkpoint (facts/lifecycle/condition delivery/time); no individual participant duplicates this checkpoint.",
            "covered_candidate_ids": [], "implementation_status": "PLANNED", "evidence": [], "obligations": {}})
    by_id = {p["id"]: p for p in participants}

    def expected_boundary(candidate):
        module = candidate["module"]
        if module not in per_module:
            return None
        name = candidate["qualified_symbol"].split("::")[-1]
        boundary = []
        for b in per_module[module]:
            if candidate["qualified_symbol"] == b["namespace"] + "::" + b["owner"] or name in reachable[b["id"]]:
                boundary.append(b)
            elif b["owner"] == "StateIntegrationPersistence" and name in {
                    "StateFactsAdapter", "StateLifecycleAdapter", "ConditionEffectsAdapter", "StateTimeAdapter"}:
                boundary.append(b)
        if len(boundary) > 1:
            raise ValueError(f"ambiguous authoritative state mapping: {candidate['candidate_id']}/{name}")
        return boundary[0] if boundary else None

    for candidate_id, candidate in reviewed.items():
        module = candidate["module"]
        if not module.startswith("EngineFramework/"):
            continue
        # Restore plumbing itself is transient implementation state and must not
        # participate in the durable ownership graph.
        if "SnapshotRestoreAccess" in candidate["qualified_symbol"] or candidate["qualified_symbol"].endswith("::SnapshotParticipant"):
            if candidate.get("persistence_role") != "RECONSTRUCTED_TRANSIENT" or candidate.get("save_plane") != "NONE":
                raise ValueError(f"restore plumbing must remain transient: {candidate_id}")
            continue
        boundary = expected_boundary(candidate)
        durable = candidate.get("persistence_role") in {"PERSISTENT_OWNER", "PERSISTENT_CHECKPOINT"}
        framework_save = candidate.get("save_plane") == "FRAMEWORK_SAVEGAME"
        if boundary is None:
            if durable and framework_save:
                raise ValueError(f"reviewed Framework durable state has no participant boundary: {candidate_id}")
            continue
        expected_id = boundary["id"]
        if not durable or not framework_save:
            raise ValueError(
                f"5.1 semantic contract excludes state reachable from reviewed participant {expected_id}: {candidate_id}")
        if candidate.get("participant_id") != expected_id:
            raise ValueError(
                f"5.1 participant identity mismatch for {candidate_id}: {candidate.get('participant_id')} != {expected_id}")
        by_id[expected_id]["covered_candidate_ids"].append(candidate_id)

    for p in participants:
        p["covered_candidate_ids"].sort()
        if not p["covered_candidate_ids"]:
            raise ValueError(f"participant has no reviewed durable state: {p['id']}")

    previous_review_path = ROOT / "docs/freeze/goal5_participant_review.json"
    if previous_review_path.is_file():
        previous_by_id = {p["id"]: p for p in load_json(previous_review_path)["participants"]}
        for p in participants:
            previous = previous_by_id.get(p["id"], {})
            if all(previous.get(key) == p.get(key) for key in ("owner", "dependencies", "snapshot", "covered_candidate_ids")):
                p["implementation_status"] = previous.get("implementation_status", "PLANNED")
                p["evidence"] = copy.deepcopy(previous.get("evidence", []))
                p["obligations"] = copy.deepcopy(previous.get("obligations", {}))

    review = {
        "schema_version": 2,
        "review_scope": "M5.2 participant graph generated read-only from the reviewed 5.1 persistence contract. This stage may not change persistence_role, save_plane, owner or participant_id decisions.",
        "participants": sorted(participants, key=lambda p: p["id"]),
    }
    return review


def self_test():
    global load_json
    original = load_json
    path = ROOT / "docs/freeze/goal5_persistence_contract.json"
    before = path.read_bytes()
    state = original(path)
    assert record_review()["participants"]
    mutants = []
    missing = copy.deepcopy(state); missing["candidates"].pop(next(iter(missing["candidates"]))); mutants.append(missing)
    for key, value in (("review_status", "DISCOVERED"), ("participant_id", "wrong.owner"),
                       ("persistence_role", "NON_PERSISTENT_OWNER"), ("save_plane", "NONE")):
        bad = copy.deepcopy(state)
        candidate = next(c for c in bad["candidates"].values() if c.get("save_plane") == "FRAMEWORK_SAVEGAME" and c.get("persistence_role") == "PERSISTENT_OWNER")
        candidate[key] = value; mutants.append(bad)
    try:
        for mutant in mutants:
            load_json = lambda p: copy.deepcopy(mutant) if p == path else original(p)
            try: record_review()
            except ValueError: pass
            else: raise AssertionError("5.2 accepted inconsistent 5.1 semantic decisions")
    finally:
        load_json = original
    assert path.read_bytes() == before, "participant mapping mutated 5.1"
    print("PASS: read-only participant mapping and five semantic-decision mismatch negatives.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record-review", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.record_review or args.check:
        review = record_review()
        path = ROOT / "docs/freeze/goal5_participant_review.json"
        if args.record_review: path.write_text(encoded(review), encoding="utf-8", newline="\n")
        if args.check and path.read_text(encoding="utf-8") != encoded(review): raise ValueError("participant mapping is stale")
        print("PASS: participant mapping agrees with immutable 5.1 decisions.")
    else: parser.error("choose --record-review, --check or --self-test")
