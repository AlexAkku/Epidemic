// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"

namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<epidemic::gameplay::processes::ProcessesService> {
    using Owner = epidemic::gameplay::processes::ProcessesService;
    using Snapshot = epidemic::gameplay::processes::ProcessesSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.instance_ids_, candidate.instance_ids_);
        savegame::detail::SwapPreparedMember(live.instances_, candidate.instances_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.reservation_ids_, candidate.reservation_ids_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.station_by_object_, candidate.station_by_object_);
        savegame::detail::SwapPreparedMember(live.station_ids_, candidate.station_ids_);
        savegame::detail::SwapPreparedMember(live.stations_, candidate.stations_);
    }

};

template <>
struct SnapshotRestoreAccess<epidemic::gameplay::resources::ResourcesProductionService> {
    using Owner = epidemic::gameplay::resources::ResourcesProductionService;
    using Snapshot = epidemic::gameplay::resources::ResourcesSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.amounts_, candidate.amounts_);
        savegame::detail::SwapPreparedMember(live.capabilities_, candidate.capabilities_);
        savegame::detail::SwapPreparedMember(live.capability_ids_, candidate.capability_ids_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.node_ids_, candidate.node_ids_);
        savegame::detail::SwapPreparedMember(live.nodes_, candidate.nodes_);
        savegame::detail::SwapPreparedMember(live.plan_ids_, candidate.plan_ids_);
        savegame::detail::SwapPreparedMember(live.plans_, candidate.plans_);
        savegame::detail::SwapPreparedMember(live.reservation_ids_, candidate.reservation_ids_);
        savegame::detail::SwapPreparedMember(live.reservations_, candidate.reservations_);
        savegame::detail::SwapPreparedMember(live.reserved_amounts_, candidate.reserved_amounts_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.site_ids_, candidate.site_ids_);
        savegame::detail::SwapPreparedMember(live.sites_, candidate.sites_);
        savegame::detail::SwapPreparedMember(live.stockpile_ids_, candidate.stockpile_ids_);
        savegame::detail::SwapPreparedMember(live.stockpiles_, candidate.stockpiles_);
        savegame::detail::SwapPreparedMember(live.transaction_ids_, candidate.transaction_ids_);
        savegame::detail::SwapPreparedMember(live.transactions_, candidate.transactions_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::simulation::SimulationService> {
    using Owner = epidemic::gameplay::simulation::SimulationService;
    using Snapshot = epidemic::gameplay::simulation::SimulationSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.active_intervals_, candidate.active_intervals_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.regions_, candidate.regions_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.summaries_, candidate.summaries_);
        savegame::detail::SwapPreparedMember(live.summary_ids_, candidate.summary_ids_);
        savegame::detail::SwapPreparedMember(live.task_ids_, candidate.task_ids_);
    }

};

} // namespace epidemic::gameplay

namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant>
CreateProcessesServiceSaveParticipant(epidemic::gameplay::processes::ProcessesService &owner,
                                      SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::processes::ProcessesService>>(owner, workspace, "framework.processes",
                                                          std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant> CreateResourcesProductionServiceSaveParticipant(
    epidemic::gameplay::resources::ResourcesProductionService &owner,
    SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::resources::ResourcesProductionService>>(owner, workspace,
                                                                    "framework.resources",
                                                                    std::vector<
                                                                        SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateSimulationServiceSaveParticipant(epidemic::gameplay::simulation::SimulationService &owner,
                                       SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::simulation::SimulationService>>(owner, workspace,
                                                            "framework.simulation",
                                                            std::vector<SaveParticipantId>{});
}

} // namespace epidemic::gameplay::savegame
