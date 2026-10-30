// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<epidemic::gameplay::encounters::EncountersService> {
    using Owner = epidemic::gameplay::encounters::EncountersService;
    using Snapshot = epidemic::gameplay::encounters::EncountersSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }
    static Owner &View(Owner &o) noexcept {
        return o;
    }
    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.definitions_, candidate.definitions_);
        savegame::detail::SwapPreparedMember(live.instance_ids_, candidate.instance_ids_);
        savegame::detail::SwapPreparedMember(live.instances_, candidate.instances_);
        savegame::detail::SwapPreparedMember(live.instances_by_area_, candidate.instances_by_area_);
        savegame::detail::SwapPreparedMember(live.instances_by_state_, candidate.instances_by_state_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.point_ids_, candidate.point_ids_);
        savegame::detail::SwapPreparedMember(live.points_, candidate.points_);
        savegame::detail::SwapPreparedMember(live.request_ids_, candidate.request_ids_);
        savegame::detail::SwapPreparedMember(live.requests_, candidate.requests_);
        savegame::detail::SwapPreparedMember(live.respawn_ids_, candidate.respawn_ids_);
        savegame::detail::SwapPreparedMember(live.respawns_, candidate.respawns_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.spawn_budget_tick_, candidate.spawn_budget_tick_);
        savegame::detail::SwapPreparedMember(live.spawn_operations_used_, candidate.spawn_operations_used_);
        savegame::detail::SwapPreparedMember(live.spawned_, candidate.spawned_);
        savegame::detail::SwapPreparedMember(live.spawned_ids_, candidate.spawned_ids_);
        savegame::detail::SwapPreparedMember(live.tables_, candidate.tables_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::needs_life::NeedsLifeService> {
    using Owner = epidemic::gameplay::needs_life::NeedsLifeService;
    using Snapshot = epidemic::gameplay::needs_life::NeedsLifeSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }
    static Owner &View(Owner &o) noexcept {
        return o;
    }
    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.decay_rules_, candidate.decay_rules_);
        savegame::detail::SwapPreparedMember(live.definitions_, candidate.definitions_);
        savegame::detail::SwapPreparedMember(live.definitions_frozen_, candidate.definitions_frozen_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.pressure_ids_, candidate.pressure_ids_);
        savegame::detail::SwapPreparedMember(live.pressures_, candidate.pressures_);
        savegame::detail::SwapPreparedMember(live.pressures_by_subject_, candidate.pressures_by_subject_);
        savegame::detail::SwapPreparedMember(live.profile_by_subject_, candidate.profile_by_subject_);
        savegame::detail::SwapPreparedMember(live.profile_ids_, candidate.profile_ids_);
        savegame::detail::SwapPreparedMember(live.profiles_, candidate.profiles_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.routine_by_subject_, candidate.routine_by_subject_);
        savegame::detail::SwapPreparedMember(live.routine_ids_, candidate.routine_ids_);
        savegame::detail::SwapPreparedMember(live.routines_, candidate.routines_);
        savegame::detail::SwapPreparedMember(live.simulation_profiles_, candidate.simulation_profiles_);
        savegame::detail::SwapPreparedMember(live.states_, candidate.states_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::population::PopulationService> {
    using Owner = epidemic::gameplay::population::PopulationService;
    using Snapshot = epidemic::gameplay::population::PopulationSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }
    static Owner &View(Owner &o) noexcept {
        return o;
    }
    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.active_allocation_by_unit_, candidate.active_allocation_by_unit_);
        savegame::detail::SwapPreparedMember(live.active_migration_by_unit_, candidate.active_migration_by_unit_);
        savegame::detail::SwapPreparedMember(live.active_residence_by_unit_, candidate.active_residence_by_unit_);
        savegame::detail::SwapPreparedMember(live.allocation_correlation_ids_, candidate.allocation_correlation_ids_);
        savegame::detail::SwapPreparedMember(live.allocation_ids_, candidate.allocation_ids_);
        savegame::detail::SwapPreparedMember(live.allocations_, candidate.allocations_);
        savegame::detail::SwapPreparedMember(live.allocations_by_correlation_, candidate.allocations_by_correlation_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.group_ids_, candidate.group_ids_);
        savegame::detail::SwapPreparedMember(live.groups_, candidate.groups_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.migration_ids_, candidate.migration_ids_);
        savegame::detail::SwapPreparedMember(live.migrations_, candidate.migrations_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.residence_ids_, candidate.residence_ids_);
        savegame::detail::SwapPreparedMember(live.residences_, candidate.residences_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.unit_by_entity_, candidate.unit_by_entity_);
        savegame::detail::SwapPreparedMember(live.unit_ids_, candidate.unit_ids_);
        savegame::detail::SwapPreparedMember(live.units_, candidate.units_);
        savegame::detail::SwapPreparedMember(live.units_by_area_, candidate.units_by_area_);
        savegame::detail::SwapPreparedMember(live.units_by_group_, candidate.units_by_group_);
        savegame::detail::SwapPreparedMember(live.units_by_state_, candidate.units_by_state_);
        savegame::detail::SwapPreparedMember(live.units_by_template_, candidate.units_by_template_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::roles_jobs::RolesJobsService> {
    using Owner = epidemic::gameplay::roles_jobs::RolesJobsService;
    using Snapshot = epidemic::gameplay::roles_jobs::RolesJobsSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }
    static Owner &View(Owner &o) noexcept {
        return o;
    }
    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.assignment_ids_, candidate.assignment_ids_);
        savegame::detail::SwapPreparedMember(live.assignments_, candidate.assignments_);
        savegame::detail::SwapPreparedMember(live.assignments_by_worker_, candidate.assignments_by_worker_);
        savegame::detail::SwapPreparedMember(live.assignments_by_workplace_, candidate.assignments_by_workplace_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.definitions_, candidate.definitions_);
        savegame::detail::SwapPreparedMember(live.duties_, candidate.duties_);
        savegame::detail::SwapPreparedMember(live.duties_by_assignment_, candidate.duties_by_assignment_);
        savegame::detail::SwapPreparedMember(live.duties_by_subject_, candidate.duties_by_subject_);
        savegame::detail::SwapPreparedMember(live.duty_ids_, candidate.duty_ids_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.schedule_ids_, candidate.schedule_ids_);
        savegame::detail::SwapPreparedMember(live.schedules_, candidate.schedules_);
        savegame::detail::SwapPreparedMember(live.schedules_by_assignment_, candidate.schedules_by_assignment_);
        savegame::detail::SwapPreparedMember(live.shift_ids_, candidate.shift_ids_);
        savegame::detail::SwapPreparedMember(live.workplace_ids_, candidate.workplace_ids_);
        savegame::detail::SwapPreparedMember(live.workplaces_, candidate.workplaces_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::population_simulation::PopulationEncounterAdapter> {
    using Owner = epidemic::gameplay::population_simulation::PopulationEncounterAdapter;
    using Snapshot = epidemic::gameplay::population_simulation::PopulationEncounterAdapterSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        auto copy = std::make_unique<Owner>(o);
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s), w.Require<population::PopulationService>(),
                                 w.Require<encounters::EncountersService>());
    }
    static Owner &View(Owner &o) noexcept {
        return o;
    }
    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.plans_, candidate.plans_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::population_simulation::PopulationLifecycleAdapter> {
    using Owner = epidemic::gameplay::population_simulation::PopulationLifecycleAdapter;
    using Snapshot = epidemic::gameplay::population_simulation::PopulationLifecycleAdapterSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        auto copy = std::make_unique<Owner>(o);
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }
    static Owner &View(Owner &o) noexcept {
        return o;
    }
    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.reconciliations_, candidate.reconciliations_);
    }
};
}
namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant>
CreateEncountersServiceSaveParticipant(epidemic::gameplay::encounters::EncountersService &owner,
                                       SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::encounters::EncountersService>>(
        owner, workspace, "framework.encounters", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant>
CreateNeedsLifeServiceSaveParticipant(epidemic::gameplay::needs_life::NeedsLifeService &owner,
                                      SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::needs_life::NeedsLifeService>>(
        owner, workspace, "framework.needs_life", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant>
CreatePopulationServiceSaveParticipant(epidemic::gameplay::population::PopulationService &owner,
                                       SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::population::PopulationService>>(
        owner, workspace, "framework.population", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant>
CreateRolesJobsServiceSaveParticipant(epidemic::gameplay::roles_jobs::RolesJobsService &owner,
                                      SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::roles_jobs::RolesJobsService>>(
        owner, workspace, "framework.roles_jobs", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreatePopulationEncounterAdapterSaveParticipant(
    epidemic::gameplay::population_simulation::PopulationEncounterAdapter &owner, SaveRestoreWorkspace &workspace) {
    return std::make_unique<
        detail::SnapshotParticipant<epidemic::gameplay::population_simulation::PopulationEncounterAdapter>>(
        owner, workspace, "integration.population_encounter",
        std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.population"),
                                       SaveParticipantId::FromString("framework.encounters")});
}
std::unique_ptr<ISaveParticipant> CreatePopulationLifecycleAdapterSaveParticipant(
    epidemic::gameplay::population_simulation::PopulationLifecycleAdapter &owner, SaveRestoreWorkspace &workspace) {
    return std::make_unique<
        detail::SnapshotParticipant<epidemic::gameplay::population_simulation::PopulationLifecycleAdapter>>(
        owner, workspace, "integration.population_lifecycle",
        std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.population"),
                                       SaveParticipantId::FromString("framework.roles_jobs"),
                                       SaveParticipantId::FromString("framework.needs_life")});
}
}
