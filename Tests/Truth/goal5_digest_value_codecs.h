// Generated exact private value codecs; source remains the production
// participant implementation.
#pragma once
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
template <> struct SnapshotValueCodec<materials::SubstanceAmount> {
  template <class IO>
  static void Transfer(IO &io, materials::SubstanceAmount &amount) {
    TransferSnapshot(io, amount.micro);
  }
};
template <> struct SnapshotValueCodec<knowledge::KnowledgeRecord> {
  template <typename IO>
  static void Transfer(IO &io, knowledge::KnowledgeRecord &value) {
    TransferSnapshot(io, value.id);
    TransferSnapshot(io, value.owner);
    TransferSnapshot(io, value.type);
    TransferSnapshot(io, value.topic);
    TransferSnapshot(io, value.assertion);
    TransferSnapshot(io, value.epistemic_state);
    TransferSnapshot(io, value.confidence);
    TransferSnapshot(io, value.subject);
    TransferSnapshot(io, value.source_kind);
    TransferSnapshot(io, value.source);
    TransferSnapshot(io, value.derived_from);
    TransferSnapshot(io, value.original_source_kind);
    TransferSnapshot(io, value.original_source);
    TransferSnapshot(io, value.transmission_depth);
    TransferSnapshot(io, value.learned_at);
    TransferSnapshot(io, value.last_confirmed_at);
    TransferSnapshot(io, value.last_decay_at);
    TransferSnapshot(io, value.decay_rule);
    TransferSnapshot(io, value.persistence);
    TransferSnapshot(io, value.payload);
    TransferSnapshot(io, value.revision);
  }
};
template <> struct SnapshotValueCodec<processes::ProcessInstance> {
  template <typename IO>
  static void Transfer(IO &io, processes::ProcessInstance &value) {
    TransferSnapshot(io, value.id);
    TransferSnapshot(io, value.recipe);
    TransferSnapshot(io, value.actor);
    TransferSnapshot(io, value.station);
    TransferSnapshot(io, value.target);
    TransferSnapshot(io, value.simulation_area);
    TransferSnapshot(io, value.state);
    TransferSnapshot(io, value.started_at);
    TransferSnapshot(io, value.due_at);
    TransferSnapshot(io, value.last_updated_at);
    TransferSnapshot(io, value.reserved_inputs);
    TransferSnapshot(io, value.prepared_outputs);
    TransferSnapshot(io, value.total_duration);
    TransferSnapshot(io, value.paused_remaining);
    TransferSnapshot(io, value.quality);
    TransferSnapshot(io, value.runtime_payload);
    TransferSnapshot(io, value.revision);
  }
};
template <> struct SnapshotValueCodec<::epidemic::gameplay::ChangeCursor> {
  template <typename IO>
  static void Transfer(IO &io, ::epidemic::gameplay::ChangeCursor &value) {
    if constexpr (!IO::reading &&
                  requires { IO::semantic_observation_identity; }) {
      auto epoch = static_cast<std::uint64_t>(value.IsValid());
      TransferSnapshot(io, epoch);
    } else
      TransferSnapshot(io, value.epoch);
    TransferSnapshot(io, value.sequence);
  }
};
} // namespace epidemic::gameplay::savegame::detail
