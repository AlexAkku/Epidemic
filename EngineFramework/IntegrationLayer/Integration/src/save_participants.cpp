// Generated owner bindings; gameplay state remains authoritative in its
// original owner.
#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<facts::GameplayFactsService> {
  using Owner = facts::GameplayFactsService;
  using Snapshot = facts::FactsPersistenceSnapshot;
  static foundation::Result<Snapshot> Capture(const Owner &owner) {
    return owner.CapturePersistenceSnapshot();
  }
  static std::unique_ptr<Owner> Clone(const Owner &owner,
                                      savegame::SaveRestoreWorkspace &) {
    if (owner.dispatching_)
      throw std::runtime_error("cannot stage facts during dispatch");
    auto copy = std::make_unique<Owner>();
    copy->event_types_ = owner.event_types_;
    copy->fact_types_ = owner.fact_types_;
    copy->event_codecs_ = owner.event_codecs_;
    copy->fact_codecs_ = owner.fact_codecs_;
    copy->frozen_ = owner.frozen_;
    copy->fact_changed_event_type_ = owner.fact_changed_event_type_;
    return copy;
  }
  static foundation::Result<void> Restore(Owner &candidate, Snapshot snapshot,
                                          savegame::SaveRestoreWorkspace &) {
    return candidate.RestorePersistenceSnapshot(std::move(snapshot));
  }
  static Owner &View(Owner &candidate) noexcept { return candidate; }
  static void Commit(Owner &live, Owner &candidate) noexcept {
    // Immutable registrations, subscribers, mutex and operational counters
    // stay in the live service. All decoding/allocation was done in
    // Clone/Restore.
    live.facts_.swap(candidate.facts_);
    live.history_.swap(candidate.history_);
    using std::swap;
    swap(live.fact_ids_, candidate.fact_ids_);
    swap(live.event_ids_, candidate.event_ids_);
    swap(live.fact_revision_, candidate.fact_revision_);
    swap(live.next_event_sequence_, candidate.next_event_sequence_);
    live.pending_events_.clear();
    live.submitted_batches_.clear();
  }
};
} // namespace epidemic::gameplay

namespace epidemic::gameplay {
template <>
struct SnapshotRestoreAccess<epidemic::gameplay::time::GameplayTimeService> {
  using Owner = epidemic::gameplay::time::GameplayTimeService;
  using Snapshot = epidemic::gameplay::time::GameplayTimeSnapshot;
  static Snapshot Capture(const Owner &o) { return o.CaptureSnapshot(); }
  static std::unique_ptr<Owner> Clone(const Owner &o,
                                      savegame::SaveRestoreWorkspace &w) {
    (void)w;
    return std::make_unique<Owner>(o);
  }
  static ::epidemic::foundation::Result<void>
  Restore(Owner &o, Snapshot s, savegame::SaveRestoreWorkspace &w) {
    (void)w;
    return o.RestoreSnapshot(std::move(s));
  }
  static Owner &View(Owner &o) noexcept { return o; }
  static void Commit(Owner &live, Owner &candidate) noexcept {
    savegame::detail::SwapPreparedMember(live.clocks_, candidate.clocks_);
    savegame::detail::SwapPreparedMember(live.schedule_ids_,
                                         candidate.schedule_ids_);
    savegame::detail::SwapPreparedMember(live.schedule_index_,
                                         candidate.schedule_index_);
    savegame::detail::SwapPreparedMember(live.scheduler_revision_,
                                         candidate.scheduler_revision_);
    savegame::detail::SwapPreparedMember(live.schedules_, candidate.schedules_);
    savegame::detail::SwapPreparedMember(
        live.synchronization_rebind_required_,
        candidate.synchronization_rebind_required_);
    savegame::detail::SwapPreparedMember(live.synchronization_revisions_,
                                         candidate.synchronization_revisions_);
  }
};
} // namespace epidemic::gameplay
namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant> CreateGameplayFactsServiceSaveParticipant(
    epidemic::gameplay::facts::GameplayFactsService &owner,
    SaveRestoreWorkspace &workspace) {
  return std::make_unique<detail::SnapshotParticipant<
      epidemic::gameplay::facts::GameplayFactsService>>(
      owner, workspace, "framework.facts", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateGameplayTimeServiceSaveParticipant(
    epidemic::gameplay::time::GameplayTimeService &owner,
    SaveRestoreWorkspace &workspace) {
  return std::make_unique<detail::SnapshotParticipant<
      epidemic::gameplay::time::GameplayTimeService>>(
      owner, workspace, "framework.time", std::vector<SaveParticipantId>{});
}
} // namespace epidemic::gameplay::savegame
