#pragma once
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace participant_tests {
inline void Check(bool value, std::string_view assertion) {
    if (!value) {
        std::cerr << assertion << '\n';
        std::exit(1);
    }
}
template <typename Owner> void FreezeOwner(Owner &owner) {
    if constexpr (requires { owner.Freeze(); }) {
        if constexpr (std::is_void_v<decltype(owner.Freeze())>)
            owner.Freeze();
        else
            Check(bool(owner.Freeze()), "owner bootstrap freeze succeeds");
    } else if constexpr (requires { owner.FreezeDefinitions(); })
        Check(bool(owner.FreezeDefinitions()), "owner definitions freeze succeeds");
}

// Called for every concrete participant, never a mock participant substitute.
inline void CheckParticipantContract(epidemic::gameplay::savegame::ISaveParticipant &participant,
                                     std::string_view identity,
                                     const std::vector<epidemic::gameplay::savegame::SaveParticipantId> &dependencies) {
    using namespace epidemic::gameplay::savegame;
    Check(participant.Id() == SaveParticipantId::FromString(identity), "stable participant Id");
    Check(participant.SchemaVersion() == 1, "schema version 1");
    Check(participant.Requirement() == SaveParticipantRequirement::Required, "explicit required semantics");
    Check(participant.Dependencies() == dependencies, "semantic restore dependencies");
    auto capture = participant.CaptureSnapshot({});
    Check(bool(capture), "capture succeeds");
    auto section = std::move(capture).Value();
    Check(!section.payload.empty(), "empty owner still has a non-empty encoded checkpoint");
    Check(bool(participant.ValidateSnapshot(section, {})), "valid payload accepted");
    auto corrupt = section;
    corrupt.payload.back() ^= std::byte{1};
    Check(!participant.ValidateSnapshot(corrupt, {}), "corrupt payload rejected");
    corrupt = section;
    corrupt.payload.pop_back();
    corrupt.payload_hash = SaveGameOrchestrator::HashBytes(corrupt.payload);
    Check(!participant.ValidateSnapshot(corrupt, {}), "truncated payload rejected despite repaired checksum");
    corrupt = section;
    corrupt.payload.push_back(std::byte{0});
    corrupt.payload_hash = SaveGameOrchestrator::HashBytes(corrupt.payload);
    Check(!participant.ValidateSnapshot(corrupt, {}), "trailing data rejected despite repaired checksum");
    corrupt = section;
    ++corrupt.schema_version;
    Check(!participant.ValidateSnapshot(corrupt, {}), "unsupported schema rejected");
    corrupt = section;
    corrupt.participant = SaveParticipantId::FromString("wrong.owner");
    Check(!participant.ValidateSnapshot(corrupt, {}), "wrong participant rejected");
    // No stage is active here: failure must come from the injected preparation
    // fault, not an unrelated duplicate-stage rejection.
    detail::SetSnapshotPrepareFailureForTesting(true);
    const auto allocation_failure = participant.StageRestore(section, {});
    detail::SetSnapshotPrepareFailureForTesting(false);
    Check(!allocation_failure, "allocation failure before commit is returned as a failure");
    auto live = participant.CaptureSnapshot({});
    Check(bool(live) && live.Value().payload == section.payload, "allocation failure preserves live state");
    auto stage = participant.StageRestore(section, {});
    Check(bool(stage), "stage valid checkpoint");
    live = participant.CaptureSnapshot({});
    Check(bool(live) && live.Value().payload == section.payload, "stage does not mutate live state");
    Check(!participant.StageRestore(corrupt, {}), "failed stage preserves existing prepared stage");
    live = participant.CaptureSnapshot({});
    Check(bool(live) && live.Value().payload == section.payload, "failure before commit preserves live state");
    participant.CommitRestore(*stage.Value());
    auto published = participant.CaptureSnapshot({});
    Check(bool(published) && bool(participant.ValidateSnapshot(published.Value(), {})),
          "commit publishes valid staged state");
    participant.CommitRestore(*stage.Value());
    auto repeated = participant.CaptureSnapshot({});
    Check(bool(repeated) && repeated.Value().payload == published.Value().payload, "stage cannot publish twice");
}
} // namespace participant_tests

namespace participant_tests {
template <typename Mutate>
void CheckChangedStatePublication(epidemic::gameplay::savegame::ISaveParticipant &participant, Mutate mutate) {
    auto expected = participant.CaptureSnapshot({});
    Check(bool(expected), "capture expected participant state before changed-state publication test");
    const auto expected_payload = expected.Value().payload;

    Check(bool(mutate()), "mutate live participant state to a second valid checkpoint");
    auto changed = participant.CaptureSnapshot({});
    Check(bool(changed) && changed.Value().payload != expected_payload,
          "changed-state fixture differs from staged participant payload");
    const auto changed_payload = changed.Value().payload;

    auto stage = participant.StageRestore(expected.Value(), {});
    Check(bool(stage), "stage original checkpoint over changed live state");
    auto during = participant.CaptureSnapshot({});
    Check(bool(during) && during.Value().payload == changed_payload,
          "staging original checkpoint does not mutate changed live state");

    participant.CommitRestore(*stage.Value());
    auto published = participant.CaptureSnapshot({});
    Check(bool(published) && published.Value().payload == expected_payload,
          "commit publishes the exact staged checkpoint over changed live state");
    auto newer_stage = participant.StageRestore(changed.Value(), {});
    Check(bool(newer_stage), "prepare a newer changed-state publication");
    participant.CommitRestore(*newer_stage.Value());
    auto after_mutation = participant.CaptureSnapshot({});
    Check(bool(after_mutation) && after_mutation.Value().payload == changed_payload,
          "one-use fixture has a different live state");
    participant.CommitRestore(*stage.Value());
    auto repeated = participant.CaptureSnapshot({});
    Check(bool(repeated) && repeated.Value().payload == after_mutation.Value().payload,
          "committed stage cannot replay over newer live state");
}

template <typename Count, typename Reset>
void CheckNonEmptyPublication(epidemic::gameplay::savegame::ISaveParticipant &participant, Count count, Reset reset) {
    const auto expected = count();
    Check(expected > 0, "non-empty fixture contains actual durable domain records");
    auto section = participant.CaptureSnapshot({});
    Check(bool(section) && bool(participant.ValidateSnapshot(section.Value(), {})),
          "actual non-empty payload validates");
    Check(bool(reset()) && count() == 0, "reset live records and derived diagnostic count");
    auto stage = participant.StageRestore(section.Value(), {});
    Check(bool(stage) && count() == 0, "stage of non-empty state leaves derived live diagnostics untouched");
    participant.CommitRestore(*stage.Value());
    Check(count() == expected, "commit publishes restored records and derived diagnostic counts");
}

template <typename Snapshot> auto SnapshotCounters(Snapshot &snapshot) {
    std::vector<std::uint64_t> counters;
    epidemic::gameplay::savegame::detail::VisitSnapshotFields(snapshot, [&](auto &...fields) {
        const auto inspect = [&](auto &field) {
            using Field = std::remove_cvref_t<decltype(field)>;
            if constexpr (std::is_same_v<Field, epidemic::gameplay::Revision>)
                counters.push_back(field.value);
            // These are unevaluated C++20 member requirements, not unused accesses.
            // cppcheck-suppress constStatement
            else if constexpr (requires {
                                   field.scope;
                                   field.next;
                               }) {
                counters.push_back(field.scope);
                counters.push_back(field.next);
            }
        };
        (inspect(fields), ...);
    });
    return counters;
}

template <typename Owner, typename Capture, typename Restore>
void CheckOwnerCounterContinuity(Owner &owner, epidemic::gameplay::savegame::ISaveParticipant &participant,
                                 Capture capture, Restore restore) {
    const auto baseline = capture();
    auto seeded = baseline;
    epidemic::gameplay::savegame::detail::VisitSnapshotFields(seeded, [&](auto &...fields) {
        const auto seed = [](auto &field) {
            using Field = std::remove_cvref_t<decltype(field)>;
            if constexpr (std::is_same_v<Field, epidemic::gameplay::Revision>)
                field.value += 7;
            // Unevaluated member requirements select generator fields at compile time.
            // cppcheck-suppress constStatement
            else if constexpr (requires {
                                   field.scope;
                                   field.next;
                               })
                field.next += 17;
        };
        (seed(fields), ...);
    });
    Check(bool(restore(std::move(seeded))), "non-empty revision/generator fixture restores");
    auto before = capture();
    const auto expected = SnapshotCounters(before);
    Check(!expected.empty(), "owner snapshot carries durable revision/generator state");
    auto section = participant.CaptureSnapshot({});
    Check(bool(section), "non-empty owner capture succeeds");
    Check(bool(restore(baseline)), "reset live state before staged publication");
    auto reset = capture();
    const auto untouched = SnapshotCounters(reset);
    Check(untouched != expected, "fixture differs from live durable state");
    auto stage = participant.StageRestore(section.Value(), {});
    Check(bool(stage), "stage non-empty owner snapshot");
    auto during = capture();
    Check(SnapshotCounters(during) == untouched, "stage preserves live revision and generator state");
    std::uint64_t old_epoch = 0;
    if constexpr (requires { owner.LatestChangeCursor(); })
        old_epoch = owner.LatestChangeCursor().epoch;
    participant.CommitRestore(*stage.Value());
    auto after = capture();
    Check(SnapshotCounters(after) == expected, "commit preserves revision/generator continuity exactly");
    if constexpr (requires { owner.LatestChangeCursor(); })
        Check(owner.LatestChangeCursor().epoch > old_epoch, "commit publishes the prepared new journal epoch");
    Check(bool(restore(baseline)), "change owner again after first commit");
    participant.CommitRestore(*stage.Value());
    auto repeated = capture();
    Check(SnapshotCounters(repeated) == untouched, "owner stage cannot replay over newer live state");
}
}
