#pragma once
#include "Epidemic/GameFramework/StateIntegration/state_adapters.h"
#include "Epidemic/GameFramework/WorldIntegration/world_adapters.h"
#include "goal5_digest_value_codecs.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>

namespace goal5_test_utility {
// Observation identities change at restore to reject stale readers. Rebuilt
// journals have no durable sequence identity; retained journals keep theirs.
template <typename Snapshot>
void NormalizeSemanticObservationSnapshot(Snapshot &snapshot) {
  if constexpr (requires { snapshot.change_epoch; })
    snapshot.change_epoch =
        static_cast<std::uint64_t>(snapshot.change_epoch != 0);
}
inline void NormalizeSemanticObservationSnapshot(
    epidemic::gameplay::state_integration::StateIntegrationCheckpoint
        &snapshot) {
  snapshot.facts.entity_cursor.sequence =
      snapshot.facts.entity_latest.sequence = 0;
  snapshot.facts.material_cursor.sequence =
      snapshot.facts.material_latest.sequence = 0;
  snapshot.facts.effect_cursor.sequence =
      snapshot.facts.effect_latest.sequence = 0;
  snapshot.lifecycle.cursor.sequence =
      snapshot.lifecycle.entity_latest.sequence = 0;
  snapshot.time.effect_cursor.sequence = snapshot.time.effect_latest.sequence =
      0;
}
inline void NormalizeSemanticObservationSnapshot(
    epidemic::gameplay::world_integration::WorldFactsCheckpoint &snapshot) {
  snapshot.world_sequence = snapshot.environment_sequence =
      snapshot.interaction_sequence = 0;
  snapshot.world_epoch = static_cast<std::uint64_t>(snapshot.world_epoch != 0);
  snapshot.environment_epoch =
      static_cast<std::uint64_t>(snapshot.environment_epoch != 0);
  snapshot.interaction_epoch =
      static_cast<std::uint64_t>(snapshot.interaction_epoch != 0);
}
inline void NormalizeSemanticObservationSnapshot(
    epidemic::gameplay::social_legal_integration::SocialLegalCheckpoint
        &snapshot) {
  // Crime restores durable records and rebuilds its lifecycle journal.
  // The penalty ledger and mapping revision remain semantic checkpoint data.
  snapshot.crime_cursor.sequence = 0;
}
// Qualification-only visitor: production wire v1 remains byte-for-byte
// unchanged. Semantic floating values use a finite 1e-6 grid, with +/-0 one
// integer value.
class CanonicalSemanticWriter {
public:
  static constexpr bool reading = false;
  static constexpr bool semantic_observation_identity = true;
  std::vector<std::byte> bytes;
  void Byte(std::uint8_t &value) {
    bytes.push_back(static_cast<std::byte>(value));
  }
  std::size_t Remaining() const noexcept { return 64ull * 1024ull * 1024ull; }
  template <typename Float> void SemanticFloat(Float value) {
    const auto scaled =
        std::round(static_cast<long double>(value) * 1'000'000.0L);
    if (!std::isfinite(scaled) ||
        scaled <= static_cast<long double>(
                      std::numeric_limits<std::int64_t>::min()) ||
        scaled >=
            static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
      throw std::runtime_error("non-finite/out-of-domain semantic float");
    auto canonical = static_cast<std::int64_t>(scaled);
    epidemic::gameplay::savegame::detail::TransferSnapshot(*this, canonical);
  }
};

inline std::string StableDigest(std::span<const std::byte> bytes) {
  std::uint64_t hash = 14695981039346656037ull;
  for (auto byte : bytes) {
    hash ^= std::to_integer<std::uint8_t>(byte);
    hash *= 1099511628211ull;
  }
  std::ostringstream text;
  text << std::hex << std::setfill('0') << std::setw(16) << hash;
  return text.str();
}

class SemanticContributions {
public:
  template <typename Snapshot>
  void AddSnapshot(std::string_view identity, Snapshot snapshot) {
    if constexpr (requires { snapshot.change_epoch; })
      observation_epochs_.emplace(std::string(identity), snapshot.change_epoch);
    NormalizeSemanticObservationSnapshot(snapshot);
    CanonicalSemanticWriter writer;
    epidemic::gameplay::savegame::detail::TransferSnapshot(writer, snapshot);
    AddBytes(identity, std::move(writer.bytes));
  }
  void AddBytes(std::string_view identity, std::vector<std::byte> bytes) {
    if (identity.empty() ||
        !contributions_.emplace(std::string(identity), std::move(bytes)).second)
      throw std::runtime_error("missing/duplicate semantic owner contribution");
  }
  std::vector<std::byte> CanonicalBytes() const {
    CanonicalSemanticWriter writer;
    std::uint32_t domain_version = 1;
    epidemic::gameplay::savegame::detail::TransferSnapshot(writer,
                                                           domain_version);
    auto count = static_cast<std::uint64_t>(contributions_.size());
    epidemic::gameplay::savegame::detail::TransferSnapshot(writer, count);
    for (const auto &[name, bytes] : contributions_) {
      auto identity = name;
      auto value = bytes;
      epidemic::gameplay::savegame::detail::TransferSnapshot(writer, identity);
      epidemic::gameplay::savegame::detail::TransferSnapshot(writer, value);
    }
    return std::move(writer.bytes);
  }
  std::string Digest() const { return StableDigest(CanonicalBytes()); }
  std::size_t Count() const noexcept { return contributions_.size(); }
  const auto &ObservationEpochs() const noexcept { return observation_epochs_; }

private:
  std::map<std::string, std::vector<std::byte>> contributions_;
  std::map<std::string, std::uint64_t> observation_epochs_;
};
} // namespace goal5_test_utility
