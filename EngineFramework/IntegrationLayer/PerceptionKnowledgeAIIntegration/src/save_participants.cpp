// Generated owner bindings; gameplay state remains authoritative in its
// original owner.
#include "Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<epidemic::gameplay::ai::AIService> {
  using Owner = epidemic::gameplay::ai::AIService;
  using Snapshot = epidemic::gameplay::ai::AISnapshot;
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
    savegame::detail::SwapPreparedMember(live.agents_, candidate.agents_);
    savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
    savegame::detail::SwapPreparedMember(live.diagnostics_,
                                         candidate.diagnostics_);
    savegame::detail::SwapPreparedMember(live.due_agents_,
                                         candidate.due_agents_);
    savegame::detail::SwapPreparedMember(live.intent_ids_,
                                         candidate.intent_ids_);
    savegame::detail::SwapPreparedMember(live.intent_to_agent_,
                                         candidate.intent_to_agent_);
    savegame::detail::SwapPreparedMember(live.journal_epoch_,
                                         candidate.journal_epoch_);
    savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                         candidate.next_change_sequence_);
    savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    savegame::detail::SwapPreparedMember(live.tick_budget_,
                                         candidate.tick_budget_);
  }
};
template <>
struct SnapshotRestoreAccess<epidemic::gameplay::knowledge::KnowledgeService> {
  using Owner = epidemic::gameplay::knowledge::KnowledgeService;
  using Snapshot = epidemic::gameplay::knowledge::KnowledgeSnapshot;
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
    savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
    savegame::detail::SwapPreparedMember(live.definitions_frozen_,
                                         candidate.definitions_frozen_);
    savegame::detail::SwapPreparedMember(live.journal_epoch_,
                                         candidate.journal_epoch_);
    savegame::detail::SwapPreparedMember(live.knowledge_, candidate.knowledge_);
    savegame::detail::SwapPreparedMember(live.knowledge_by_owner_,
                                         candidate.knowledge_by_owner_);
    savegame::detail::SwapPreparedMember(live.knowledge_by_subject_,
                                         candidate.knowledge_by_subject_);
    savegame::detail::SwapPreparedMember(live.knowledge_by_topic_,
                                         candidate.knowledge_by_topic_);
    savegame::detail::SwapPreparedMember(live.knowledge_ids_,
                                         candidate.knowledge_ids_);
    savegame::detail::SwapPreparedMember(live.memories_, candidate.memories_);
    savegame::detail::SwapPreparedMember(live.memories_by_owner_,
                                         candidate.memories_by_owner_);
    savegame::detail::SwapPreparedMember(live.memory_ids_,
                                         candidate.memory_ids_);
    savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                         candidate.next_change_sequence_);
    savegame::detail::SwapPreparedMember(live.profiles_, candidate.profiles_);
    savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
  }
};
template <>
struct SnapshotRestoreAccess<
    epidemic::gameplay::perception::PerceptionService> {
  using Owner = epidemic::gameplay::perception::PerceptionService;
  using Snapshot = epidemic::gameplay::perception::PerceptionSnapshot;
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
    savegame::detail::SwapPreparedMember(live.awareness_, candidate.awareness_);
    savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
    savegame::detail::SwapPreparedMember(live.journal_epoch_,
                                         candidate.journal_epoch_);
    savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                         candidate.next_change_sequence_);
    savegame::detail::SwapPreparedMember(live.observation_ids_,
                                         candidate.observation_ids_);
    savegame::detail::SwapPreparedMember(live.observations_,
                                         candidate.observations_);
    savegame::detail::SwapPreparedMember(live.observations_by_perceiver_,
                                         candidate.observations_by_perceiver_);
    savegame::detail::SwapPreparedMember(live.pending_by_ready_,
                                         candidate.pending_by_ready_);
    savegame::detail::SwapPreparedMember(live.pending_observations_,
                                         candidate.pending_observations_);
    savegame::detail::SwapPreparedMember(live.perceivers_,
                                         candidate.perceivers_);
    savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    savegame::detail::SwapPreparedMember(live.stimuli_, candidate.stimuli_);
    savegame::detail::SwapPreparedMember(live.stimuli_by_area_,
                                         candidate.stimuli_by_area_);
    savegame::detail::SwapPreparedMember(live.stimulus_ids_,
                                         candidate.stimulus_ids_);
    savegame::detail::SwapPreparedMember(live.tick_budget_,
                                         candidate.tick_budget_);
  }
};
} // namespace epidemic::gameplay
namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant>
CreateAIServiceSaveParticipant(epidemic::gameplay::ai::AIService &owner,
                               SaveRestoreWorkspace &workspace) {
  return std::make_unique<
      detail::SnapshotParticipant<epidemic::gameplay::ai::AIService>>(
      owner, workspace, "framework.ai", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateKnowledgeServiceSaveParticipant(
    epidemic::gameplay::knowledge::KnowledgeService &owner,
    SaveRestoreWorkspace &workspace) {
  return std::make_unique<detail::SnapshotParticipant<
      epidemic::gameplay::knowledge::KnowledgeService>>(
      owner, workspace, "framework.knowledge",
      std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreatePerceptionServiceSaveParticipant(
    epidemic::gameplay::perception::PerceptionService &owner,
    SaveRestoreWorkspace &workspace) {
  return std::make_unique<detail::SnapshotParticipant<
      epidemic::gameplay::perception::PerceptionService>>(
      owner, workspace, "framework.perception",
      std::vector<SaveParticipantId>{});
}
} // namespace epidemic::gameplay::savegame
