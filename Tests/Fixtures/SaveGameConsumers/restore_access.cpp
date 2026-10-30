#include "Epidemic/GameFramework/SaveGame/save_game.h"
struct Snapshot {
  std::uint64_t value = 0;
};
struct Owner {
  Snapshot state{};
  Snapshot CaptureSnapshot() const noexcept { return state; }
  epidemic::foundation::Result<void> RestoreSnapshot(Snapshot snapshot) {
    state = snapshot;
    return epidemic::foundation::Result<void>::Success();
  }
};
void Consumer(Owner &live, Owner &prepared) {
  epidemic::gameplay::SnapshotRestoreAccess<Owner>::Commit(live, prepared);
}
