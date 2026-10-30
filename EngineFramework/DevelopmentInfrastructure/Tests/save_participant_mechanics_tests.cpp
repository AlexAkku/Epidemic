#include "participant_test_contract.h"
#include <array>
using namespace epidemic::gameplay;
using namespace epidemic::gameplay::savegame;
namespace {
enum class Kind : std::uint16_t {
    Valid = 7
};
struct ValueSnapshot {
    std::uint32_t revision = 1;
    std::int64_t signed_value = -19;
    double amount = 3.25;
    Kind kind = Kind::Valid;
    std::vector<std::string> records;
    std::optional<std::array<std::uint32_t, 2>> choice;
    GameplayTagSet tags;
    bool enabled = true;
};
struct ValueOwner {
    ValueSnapshot state;
    ValueSnapshot CaptureSnapshot() const {
        return state;
    }
    epidemic::foundation::Result<void> RestoreSnapshot(ValueSnapshot snapshot) {
        if (snapshot.revision == 0 || snapshot.kind != Kind::Valid)
            return epidemic::foundation::Result<void>::Failure(
                epidemic::foundation::Error::Create("fixture.invalid", "invalid durable value"));
        state = std::move(snapshot);
        return epidemic::foundation::Result<void>::Success();
    }
};
struct WrongStage final : IRestoreStage {};
}
void TestSnapshotParticipantMechanics() {
    using participant_tests::Check;
    static_assert(savegame::detail::AggregateAccepts<ValueSnapshot>(std::make_index_sequence<8>{}));
    static_assert(savegame::detail::AggregateArity<ValueSnapshot>() == 8);
    SaveRestoreWorkspace workspace;
    ValueOwner live;
    savegame::detail::WorkspaceTestAccess::Bind(workspace, live);
    savegame::detail::WorkspaceTestAccess::Bind(workspace, live);
    Check(&savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace) == &live &&
              &savegame::detail::WorkspaceTestAccess::Resolve(workspace, live) == &live,
          "workspace resolves authoritative live owner");
    ValueOwner other;
    bool duplicate_rejected = false;
    try {
        savegame::detail::WorkspaceTestAccess::Bind(workspace, other);
    } catch (const std::invalid_argument &) {
        duplicate_rejected = true;
    }
    Check(duplicate_rejected, "workspace rejects overlapping authoritative owners");
    SaveRestoreWorkspace missing;
    bool missing_rejected = false;
    try {
        (void)savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(missing);
    } catch (const std::runtime_error &) {
        missing_rejected = true;
    }
    Check(missing_rejected && &savegame::detail::WorkspaceTestAccess::Resolve(missing, live) == &live,
          "workspace missing requirement fails and resolve fallback is explicit");
    savegame::detail::SnapshotParticipant<ValueOwner> participant(live, workspace, "fixture.value");
    participant_tests::CheckParticipantContract(participant, "fixture.value", {});
    live.state.revision = 27;
    live.state.records = {"alpha", "beta"};
    live.state.choice = std::array<std::uint32_t, 2>{17, 91};
    live.state.tags.Add(TagId::FromString("fixture.tag"));
    auto captured = participant.CaptureSnapshot({});
    Check(bool(captured), "field codec captures non-empty records, optional, tags, signed and floating values");
    auto decoded = savegame::detail::DecodeParticipantSnapshot<ValueSnapshot>(captured.Value(), participant.Id());
    Check(decoded.revision == 27 && decoded.records == live.state.records && decoded.choice == live.state.choice &&
              decoded.tags == live.state.tags && decoded.signed_value == -19 && decoded.amount == 3.25 &&
              decoded.enabled,
          "field codec roundtrips actual values without object layout");
    live.state.revision = 99;
    live.state.records.clear();
    participant.BeginRestoreValidation();
    participant.BeginRestoreValidation();
    Check(bool(participant.ValidateSnapshot(captured.Value(), {})), "restore validation prepares dependency view");
    Check(live.state.revision == 99 && live.state.records.empty() &&
              savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace).state.revision == 27 &&
              savegame::detail::WorkspaceTestAccess::Resolve(workspace, live).state.records.size() == 2,
          "validated dependencies remain private and retain durable values");
    participant.EndRestoreValidation();
    Check(&savegame::detail::WorkspaceTestAccess::Resolve(workspace, live) != &live,
          "shared validation view survives until the last participant exits");
    participant.EndRestoreValidation();
    Check(&savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace) == &live &&
              &savegame::detail::WorkspaceTestAccess::Resolve(workspace, live) == &live,
          "last validation exit discards private candidates and restores live fallback");
    participant.BeginRestoreValidation();
    auto invalid_validation = captured.Value();
    invalid_validation.payload_hash ^= 1;
    Check(!participant.ValidateSnapshot(invalid_validation, {}), "invalid validation never seeds a dependency view");
    participant.EndRestoreValidation();
    Check(&savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace) == &live && live.state.revision == 99,
          "validation failure preserves live state and leaves no retained candidate");
    auto stage = participant.StageRestore(captured.Value(), {});
    Check(bool(stage) && live.state.revision == 99 && live.state.records.empty(),
          "prepared state remains separate from live owner");
    Check(savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace).state.revision == 27 &&
              savegame::detail::WorkspaceTestAccess::Resolve(workspace, live).state.records.size() == 2,
          "dependent staging resolves prepared candidate rather than live owner");
    Check(!participant.StageRestore(captured.Value(), {}), "duplicate active stage is rejected");
    WrongStage wrong;
    participant.CommitRestore(wrong);
    Check(live.state.revision == 99, "wrong stage cannot publish live state");
    stage.Value().reset();
    Check(&savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace) == &live,
          "abandoned stage removes candidate binding");
    stage = participant.StageRestore(captured.Value(), {});
    participant.CommitRestore(*stage.Value());
    Check(live.state.revision == 27 && live.state.records.size() == 2 &&
              &savegame::detail::WorkspaceTestAccess::Require<ValueOwner>(workspace) == &live,
          "commit publishes prepared values and removes staging binding");
    auto invalid = decoded;
    invalid.revision = 0;
    savegame::detail::SnapshotWriter writer;
    std::uint32_t magic = 0x31504645;
    savegame::detail::TransferSnapshot(writer, magic);
    savegame::detail::TransferSnapshot(writer, invalid);
    auto semantic = captured.Value();
    semantic.payload = writer.bytes;
    semantic.payload_hash = SaveGameOrchestrator::HashBytes(semantic.payload);
    Check(!participant.ValidateSnapshot(semantic, {}) && !participant.StageRestore(semantic, {}) &&
              live.state.revision == 27,
          "semantic invalidity is rejected despite correct checksum with no live mutation");
    auto bad_bool = captured.Value();
    bad_bool.payload.back() = std::byte{2};
    bad_bool.payload_hash = SaveGameOrchestrator::HashBytes(bad_bool.payload);
    Check(!participant.ValidateSnapshot(bad_bool, {}), "noncanonical boolean rejected");
    auto bad_magic = captured.Value();
    bad_magic.payload[0] ^= std::byte{1};
    bad_magic.payload_hash = SaveGameOrchestrator::HashBytes(bad_magic.payload);
    Check(!participant.ValidateSnapshot(bad_magic, {}), "wrong payload magic rejected");
    savegame::detail::SnapshotWriter byte_writer;
    std::uint8_t byte = 42;
    byte_writer.Byte(byte);
    savegame::detail::SnapshotReader byte_reader(byte_writer.bytes);
    byte = 0;
    byte_reader.Byte(byte);
    Check(byte == 42 && byte_reader.Remaining() == 0 && byte_writer.Remaining() > 0,
          "byte codec bounds and remaining counts");
    bool truncated = false;
    try {
        byte_reader.Byte(byte);
    } catch (const std::runtime_error &) {
        truncated = true;
    }
    Check(truncated, "reader refuses reads past payload boundary");
    std::uint64_t excessive = 1'048'577;
    savegame::detail::SnapshotWriter length_writer;
    savegame::detail::TransferSnapshot(length_writer, excessive);
    savegame::detail::SnapshotReader length_reader(length_writer.bytes);
    std::vector<std::uint32_t> values;
    bool bounded = false;
    try {
        savegame::detail::TransferSnapshot(length_reader, values);
    } catch (const std::runtime_error &) {
        bounded = true;
    }
    Check(bounded && values.empty(), "untrusted collection length is bounded before allocation");
    std::vector<int> prepared{1, 2}, published{3};
    savegame::detail::SwapPreparedMember(published, prepared);
    Check(published.size() == 2 && prepared.size() == 1, "prepared member publication swaps without allocation");
}
