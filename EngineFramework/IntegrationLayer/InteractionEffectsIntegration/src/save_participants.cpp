// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay {
template <>
struct SnapshotRestoreAccess<epidemic::gameplay::interaction_effects_integration::InteractionEffectExecutor> {
    using Owner = epidemic::gameplay::interaction_effects_integration::InteractionEffectExecutor;
    using Snapshot = epidemic::gameplay::interaction_effects_integration::InteractionEffectsSnapshot;
    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }
    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.effects_), o.interaction_type_);
        copy->definitions_ = o.definitions_;
        copy->frozen_ = o.frozen_;
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
        savegame::detail::SwapPreparedMember(live.deliveries_, candidate.deliveries_);
    }
};
}
namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant> CreateInteractionEffectExecutorSaveParticipant(
    epidemic::gameplay::interaction_effects_integration::InteractionEffectExecutor &owner,
    SaveRestoreWorkspace &workspace) {
    return std::make_unique<
        detail::SnapshotParticipant<epidemic::gameplay::interaction_effects_integration::InteractionEffectExecutor>>(
        owner, workspace, "integration.interaction_effects",
        std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.interaction"),
                                       SaveParticipantId::FromString("framework.effects")});
}
}
