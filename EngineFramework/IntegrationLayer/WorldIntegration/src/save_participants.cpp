// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/WorldIntegration/world_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"

namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<epidemic::gameplay::environment::EnvironmentService> {
    using Owner = epidemic::gameplay::environment::EnvironmentService;
    using Snapshot = epidemic::gameplay::environment::EnvironmentSnapshot;

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
        savegame::detail::SwapPreparedMember(live.global_layers_, candidate.global_layers_);
        savegame::detail::SwapPreparedMember(live.ids_, candidate.ids_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.large_layers_, candidate.large_layers_);
        savegame::detail::SwapPreparedMember(live.last_change_sequence_,
                                             candidate.last_change_sequence_);
        savegame::detail::SwapPreparedMember(live.layers_, candidate.layers_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.spatial_index_, candidate.spatial_index_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::interaction::InteractionService> {
    using Owner = epidemic::gameplay::interaction::InteractionService;
    using Snapshot = epidemic::gameplay::interaction::InteractionSnapshot;

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
        savegame::detail::SwapPreparedMember(live.ids_, candidate.ids_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.session_by_schedule_,
                                             candidate.session_by_schedule_);
        savegame::detail::SwapPreparedMember(live.sessions_, candidate.sessions_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::world::WorldService> {
    using Owner = epidemic::gameplay::world::WorldService;
    using Snapshot = epidemic::gameplay::world::WorldSnapshot;

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
        savegame::detail::SwapPreparedMember(live.alteration_ids_, candidate.alteration_ids_);
        savegame::detail::SwapPreparedMember(live.alteration_index_, candidate.alteration_index_);
        savegame::detail::SwapPreparedMember(live.alterations_, candidate.alterations_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.features_, candidate.features_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.large_alterations_, candidate.large_alterations_);
        savegame::detail::SwapPreparedMember(live.last_change_sequence_,
                                             candidate.last_change_sequence_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.object_placements_, candidate.object_placements_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::world_integration::WorldFactsAdapter> {
    using Owner = epidemic::gameplay::world_integration::WorldFactsAdapter;
    using Snapshot = epidemic::gameplay::world_integration::WorldFactsCheckpoint;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureCheckpoint();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.world_), w.Resolve(o.environment_),
                                            w.Resolve(o.interaction_), w.Resolve(o.facts_));
        copy->world_event_ = o.world_event_;
        copy->environment_event_ = o.environment_event_;
        copy->interaction_event_ = o.interaction_event_;
        return copy;
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreCheckpoint(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.wc_, candidate.wc_);
        savegame::detail::SwapPreparedMember(live.ec_, candidate.ec_);
        savegame::detail::SwapPreparedMember(live.ic_, candidate.ic_);
    }

};

} // namespace epidemic::gameplay

namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant>
CreateEnvironmentServiceSaveParticipant(epidemic::gameplay::environment::EnvironmentService &owner,
                                        SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::environment::EnvironmentService>>(owner, workspace,
                                                              "framework.environment",
                                                              std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateInteractionServiceSaveParticipant(epidemic::gameplay::interaction::InteractionService &owner,
                                        SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::interaction::InteractionService>>(owner, workspace,
                                                              "framework.interaction",
                                                              std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateWorldServiceSaveParticipant(epidemic::gameplay::world::WorldService &owner,
                                  SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::world::WorldService>>(owner, workspace, "framework.world",
                                                  std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant> CreateWorldFactsAdapterSaveParticipant(
    epidemic::gameplay::world_integration::WorldFactsAdapter &owner,
    SaveRestoreWorkspace &workspace) {
    return std::make_unique<
        detail::SnapshotParticipant<epidemic::gameplay::world_integration::WorldFactsAdapter>>(
        owner, workspace, "integration.world_facts",
        std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.world"),
                                       SaveParticipantId::FromString("framework.environment"),
                                       SaveParticipantId::FromString("framework.interaction"),
                                       SaveParticipantId::FromString("framework.facts")});
}

} // namespace epidemic::gameplay::savegame
