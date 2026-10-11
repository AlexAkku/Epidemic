// Same valid owner shape as the two negative consumers, but explicitly given
// the PRIVATE include path. Proves those negatives are not ill-formed owners.
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
struct Snapshot {
    std::uint64_t value = 0;
};
struct Owner {
    Snapshot state{};
    Snapshot CaptureSnapshot() const noexcept {
        return state;
    }
    epidemic::foundation::Result<void> RestoreSnapshot(Snapshot snapshot) {
        state = snapshot;
        return epidemic::foundation::Result<void>::Success();
    }
};
void Consumer(Owner &owner, Owner &prepared, epidemic::gameplay::savegame::SaveRestoreWorkspace &workspace) {
    epidemic::gameplay::SnapshotRestoreAccess<Owner>::Commit(owner, prepared);
    epidemic::gameplay::savegame::detail::SnapshotParticipant<Owner> participant(owner, workspace, "external.owner");
}
