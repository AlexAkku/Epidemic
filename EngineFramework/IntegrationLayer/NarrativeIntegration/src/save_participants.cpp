// Generated owner bindings; gameplay state remains authoritative in its
// original owner.
#include "Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay::savegame::detail {
template <> struct SnapshotValueCodec<narrative::NarrativeValue> {
  template <typename IO>
  static void Transfer(IO &io, narrative::NarrativeValue &value) {
    std::uint8_t alternative = static_cast<std::uint8_t>(value.index());
    TransferSnapshot(io, alternative);
    if (alternative >= 5)
      throw std::runtime_error("invalid NarrativeValue alternative");
    if constexpr (IO::reading) {
      switch (alternative) {
      case 0:
        value.emplace<0>();
        break;
      case 1:
        value.emplace<1>();
        break;
      case 2:
        value.emplace<2>();
        break;
      case 3:
        value.emplace<3>();
        break;
      case 4:
        value.emplace<4>();
        break;
      }
    }
    std::visit([&](auto &field) { TransferSnapshot(io, field); }, value);
  }
};
} // namespace epidemic::gameplay::savegame::detail

namespace epidemic::gameplay {
template <>
struct SnapshotRestoreAccess<epidemic::gameplay::narrative::NarrativeService> {
  using Owner = epidemic::gameplay::narrative::NarrativeService;
  using Snapshot = epidemic::gameplay::narrative::NarrativeSnapshot;
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
    savegame::detail::SwapPreparedMember(live.activated_beats_,
                                         candidate.activated_beats_);
    savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
    savegame::detail::SwapPreparedMember(live.choice_ids_,
                                         candidate.choice_ids_);
    savegame::detail::SwapPreparedMember(live.choices_, candidate.choices_);
    savegame::detail::SwapPreparedMember(live.clue_ids_, candidate.clue_ids_);
    savegame::detail::SwapPreparedMember(live.clues_, candidate.clues_);
    savegame::detail::SwapPreparedMember(live.consequence_idempotency_,
                                         candidate.consequence_idempotency_);
    savegame::detail::SwapPreparedMember(live.consequence_ids_,
                                         candidate.consequence_ids_);
    savegame::detail::SwapPreparedMember(live.consequences_,
                                         candidate.consequences_);
    savegame::detail::SwapPreparedMember(
        live.evaluation_budget_exhausted_,
        candidate.evaluation_budget_exhausted_);
    savegame::detail::SwapPreparedMember(live.evaluation_counter_,
                                         candidate.evaluation_counter_);
    savegame::detail::SwapPreparedMember(live.evaluation_depth_,
                                         candidate.evaluation_depth_);
    savegame::detail::SwapPreparedMember(live.evaluation_limit_,
                                         candidate.evaluation_limit_);
    savegame::detail::SwapPreparedMember(live.event_executions_,
                                         candidate.event_executions_);
    savegame::detail::SwapPreparedMember(live.flags_, candidate.flags_);
    savegame::detail::SwapPreparedMember(live.journal_, candidate.journal_);
    savegame::detail::SwapPreparedMember(live.journal_epoch_,
                                         candidate.journal_epoch_);
    savegame::detail::SwapPreparedMember(live.journal_ids_,
                                         candidate.journal_ids_);
    savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                         candidate.next_change_sequence_);
    savegame::detail::SwapPreparedMember(live.objectives_,
                                         candidate.objectives_);
    savegame::detail::SwapPreparedMember(live.processed_event_keys_,
                                         candidate.processed_event_keys_);
    savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    savegame::detail::SwapPreparedMember(live.rumor_expiry_index_,
                                         candidate.rumor_expiry_index_);
    savegame::detail::SwapPreparedMember(live.rumor_ids_, candidate.rumor_ids_);
    savegame::detail::SwapPreparedMember(live.rumors_, candidate.rumors_);
    savegame::detail::SwapPreparedMember(live.storylet_runtime_,
                                         candidate.storylet_runtime_);
    savegame::detail::SwapPreparedMember(live.threads_, candidate.threads_);
    savegame::detail::SwapPreparedMember(live.variables_, candidate.variables_);
  }
};
} // namespace epidemic::gameplay
namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant> CreateNarrativeServiceSaveParticipant(
    epidemic::gameplay::narrative::NarrativeService &owner,
    SaveRestoreWorkspace &workspace) {
  return std::make_unique<detail::SnapshotParticipant<
      epidemic::gameplay::narrative::NarrativeService>>(
      owner, workspace, "framework.narrative",
      std::vector<SaveParticipantId>{});
}
} // namespace epidemic::gameplay::savegame
