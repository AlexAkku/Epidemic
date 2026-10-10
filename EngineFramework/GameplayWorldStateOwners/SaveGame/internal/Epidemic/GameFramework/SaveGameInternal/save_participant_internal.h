#pragma once

// Internal M5.2 participant mechanics. This directory is a PRIVATE include
// surface of SaveGame consumers and is intentionally unavailable through the
// public SaveGame include path.
#include "Epidemic/GameFramework/SaveGame/save_game.h"

#include <bit>
#include <exception>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace epidemic::gameplay
{
template <typename Owner> struct SnapshotRestoreAccess
{
    using Snapshot = decltype(std::declval<const Owner&>().CaptureSnapshot());
    static Snapshot Capture(const Owner& owner) { return owner.CaptureSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& owner, savegame::SaveRestoreWorkspace&)
    { return std::make_unique<Owner>(owner); }
    static foundation::Result<void> Restore(Owner& candidate, Snapshot snapshot, savegame::SaveRestoreWorkspace&)
    { return candidate.RestoreSnapshot(std::move(snapshot)); }
    static Owner& View(Owner& candidate) noexcept { return candidate; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        static_assert(std::is_nothrow_swappable_v<Owner>, "publication must not allocate or throw");
        using std::swap; swap(live, candidate);
    }
};
}

namespace epidemic::gameplay::savegame::detail
{
template <typename T> void SwapPreparedMember(T& live, T& candidate) noexcept
{
    if constexpr (requires { live.swap(candidate); })
    {
        static_assert(noexcept(live.swap(candidate)), "container publication must be noexcept");
        live.swap(candidate);
    }
    else
    {
        static_assert(std::is_nothrow_swappable_v<T>, "value publication must be noexcept");
        using std::swap; swap(live, candidate);
    }
}
// Fault seam follows the existing SaveGame test seam; it fails before any
// publication or workspace insertion and is thread-local, not durable state.
inline thread_local bool fail_snapshot_prepare_for_test = false;

struct AggregateArgument
{
    // Used only by requires-expressions. MSVC optional may instantiate the body
    // while Clang checks aggregate viability; define it without constructing T.
    template <typename T> operator T() const noexcept { std::terminate(); }
};
template <typename T, std::size_t... I>
consteval bool AggregateAccepts(std::index_sequence<I...>)
{ return requires { T{(static_cast<void>(I), AggregateArgument{})...}; }; }
template <typename T, std::size_t N = 0> consteval std::size_t AggregateArity()
{
    static_assert(N < 33, "large durable DTO requires an explicit field codec");
    if constexpr (AggregateAccepts<T>(std::make_index_sequence<N + 1>{})) return AggregateArity<T, N + 1>();
    else return N;
}
template <typename T, typename F> void VisitSnapshotFields(T& value, F&& visitor);

class SnapshotWriter
{
  public:
    static constexpr bool reading = false;
    std::vector<std::byte> bytes;
    void Byte(std::uint8_t& value) { bytes.push_back(static_cast<std::byte>(value)); }
    std::size_t Remaining() const noexcept { return 64ull * 1024ull * 1024ull; }
};
class SnapshotReader
{
  public:
    static constexpr bool reading = true;
    explicit SnapshotReader(std::span<const std::byte> input) noexcept : bytes(input) {}
    void Byte(std::uint8_t& value)
    {
        if (position == bytes.size()) throw std::runtime_error("truncated participant payload");
        value = std::to_integer<std::uint8_t>(bytes[position++]);
    }
    std::size_t Remaining() const noexcept { return bytes.size() - position; }
  private:
    std::span<const std::byte> bytes;
    std::size_t position = 0;
};
template <typename T> struct IsVector : std::false_type {};
template <typename T, typename A> struct IsVector<std::vector<T, A>> : std::true_type {};
template <typename T> struct IsOptional : std::false_type {};
template <typename T> struct IsOptional<std::optional<T>> : std::true_type {};
template <typename> inline constexpr bool UnsupportedSnapshotType = false;
template <typename T> struct SnapshotValueCodec;

// Field-wise little-endian payload: no object layout, padding, pointer, RTTI,
// std::any or callback identity can cross this boundary. Enum/domain validity
// is additionally checked by the owner's existing RestoreSnapshot contract.
template <typename IO, typename T> void TransferSnapshot(IO& io, T& value)
{
    if constexpr (std::is_same_v<T, bool>)
    {
        std::uint8_t byte = value ? 1 : 0; io.Byte(byte);
        if (byte > 1) throw std::runtime_error("invalid participant bool");
        if constexpr (IO::reading) value = byte != 0;
    }
    else if constexpr (std::is_integral_v<T>)
    {
        using U = std::make_unsigned_t<T>;
        U bits = IO::reading ? 0 : std::bit_cast<U>(value);
        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            std::uint8_t byte = static_cast<std::uint8_t>(bits >> (8 * i)); io.Byte(byte);
            if constexpr (IO::reading) bits |= static_cast<U>(static_cast<U>(byte) << (8 * i));
        }
        if constexpr (IO::reading) value = std::bit_cast<T>(bits);
    }
    else if constexpr (std::is_enum_v<T>)
    {
        auto raw = static_cast<std::underlying_type_t<T>>(value); TransferSnapshot(io, raw);
        if constexpr (IO::reading) value = static_cast<T>(raw);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        if constexpr (requires { io.SemanticFloat(value); })
        { io.SemanticFloat(value); }
        else
        {
        using U = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
        U bits = std::bit_cast<U>(value); TransferSnapshot(io, bits);
        if constexpr (IO::reading) value = std::bit_cast<T>(bits);
        }
    }
    else if constexpr (std::is_same_v<T, std::string> || IsVector<T>::value)
    {
        std::uint64_t count = value.size(); TransferSnapshot(io, count);
        if (count > 1'048'576 || (IO::reading && count > io.Remaining()))
            throw std::runtime_error("participant collection limit exceeded");
        if constexpr (IO::reading) value.resize(static_cast<std::size_t>(count));
        for (auto& element : value) TransferSnapshot(io, element);
    }
    else if constexpr (IsOptional<T>::value)
    {
        bool present = value.has_value(); TransferSnapshot(io, present);
        if constexpr (IO::reading) { if (present) value.emplace(); else value.reset(); }
        if (present) TransferSnapshot(io, *value);
    }
    else if constexpr (std::is_same_v<T, GameplayTagSet>)
    {
        auto tags = value.Values(); TransferSnapshot(io, tags);
        if constexpr (IO::reading)
        {
            value = {};
            TagId previous{};
            for (const auto tag : tags)
            {
                if (!tag.IsValid() || (previous.IsValid() && tag <= previous)) throw std::runtime_error("invalid tag set");
                value.Add(tag); previous = tag;
            }
        }
    }
    else if constexpr (requires { std::tuple_size<T>::value; })
    { std::apply([&](auto&... fields) { (TransferSnapshot(io, fields), ...); }, value); }
    else if constexpr (requires { SnapshotValueCodec<T>::Transfer(io, value); })
    { SnapshotValueCodec<T>::Transfer(io, value); }
    else if constexpr (std::is_aggregate_v<T> && !std::is_pointer_v<T>)
    { VisitSnapshotFields(value, [&](auto&... fields) { (TransferSnapshot(io, fields), ...); }); }
    else static_assert(UnsupportedSnapshotType<T>, "durable field needs an explicit value codec; process-local types are forbidden");
}

template <typename Snapshot> Snapshot DecodeParticipantSnapshot(const SaveSection& section, SaveParticipantId identity)
{
    if (section.participant != identity || section.schema_version != 1 || section.payload.size() > 64ull * 1024ull * 1024ull ||
        section.payload_hash != SaveGameOrchestrator::HashBytes(section.payload))
        throw std::runtime_error("participant identity/schema/hash/size mismatch");
    SnapshotReader reader(section.payload);
    std::uint32_t magic = 0; TransferSnapshot(reader, magic);
    if (magic != 0x31504645) throw std::runtime_error("participant payload magic mismatch");
    Snapshot snapshot{}; TransferSnapshot(reader, snapshot);
    if (reader.Remaining() != 0) throw std::runtime_error("participant payload has trailing bytes");
    return snapshot;
}

template <typename Owner> class SnapshotParticipant final : public ISaveParticipant
{
    using Access = SnapshotRestoreAccess<Owner>;
    using Snapshot = typename Access::Snapshot;
    using PreparedPointer = decltype(Access::Clone(std::declval<const Owner&>(), std::declval<SaveRestoreWorkspace&>()));
    struct Stage final : IRestoreStage
    {
        SnapshotParticipant* participant;
        PreparedPointer prepared;
        bool committed = false;
        Stage(SnapshotParticipant& p, PreparedPointer candidate) : participant(&p), prepared(std::move(candidate)) {}
        ~Stage() override { participant->RemoveStage(*this); }
    };
  public:
    SnapshotParticipant(Owner& owner, SaveRestoreWorkspace& workspace, std::string_view identity,
                        std::vector<SaveParticipantId> dependencies = {})
        : owner_(owner), workspace_(workspace), id_(SaveParticipantId::FromString(identity)), dependencies_(std::move(dependencies))
    {
        workspace_.Bind(owner_);
    }
    SaveParticipantId Id() const noexcept override { return id_; }
    SaveSchemaVersion SchemaVersion() const noexcept override { return 1; }
    std::vector<SaveParticipantId> Dependencies() const override { return dependencies_; }
    void BeginRestoreValidation() noexcept override
    {
        if (workspace_.validation_users_++ == 0) workspace_.validated_.clear();
    }
    void EndRestoreValidation() noexcept override
    {
        if (workspace_.validation_users_ == 0) return;
        --workspace_.validation_users_;
        if (workspace_.validation_users_ == 0) workspace_.validated_.clear();
    }
    foundation::Result<SaveSection> CaptureSnapshot(const SaveContext&) const override
    {
        try
        {
            auto captured = Access::Capture(owner_);
            Snapshot snapshot;
            if constexpr (requires { captured.Value(); })
            {
                if (!captured) return foundation::Result<SaveSection>::Failure(captured.GetError());
                snapshot = std::move(captured).Value();
            }
            else snapshot = std::move(captured);
            SnapshotWriter writer; std::uint32_t magic = 0x31504645; TransferSnapshot(writer, magic);
            TransferSnapshot(writer, snapshot);
            if (writer.bytes.size() > 64ull * 1024ull * 1024ull) throw std::runtime_error("participant payload too large");
            SaveSection section{id_, 1, std::move(writer.bytes), 0};
            section.payload_hash = SaveGameOrchestrator::HashBytes(section.payload);
            return foundation::Result<SaveSection>::Success(std::move(section));
        }
        catch (const std::exception& e) { return foundation::Result<SaveSection>::Failure(Failure(e)); }
    }
    foundation::Result<void> ValidateSnapshot(const SaveSection& section, const RestoreContext&) const override
    {
        try
        {
            auto snapshot = DecodeParticipantSnapshot<Snapshot>(section, id_);
            auto candidate = Access::Clone(owner_, workspace_);
            auto valid = Access::Restore(*candidate, std::move(snapshot), workspace_);
            if (!valid) return valid;
            if (workspace_.validation_users_ != 0)
            {
                auto* view = &Access::View(*candidate);
                auto [entry, inserted] = workspace_.validated_.emplace(typeid(Owner),
                    SaveRestoreWorkspace::ValidationRecord{std::shared_ptr<void>(std::move(candidate)), view});
                if (!inserted) throw std::runtime_error("owner already has a validated restore candidate");
            }
            return foundation::Result<void>::Success();
        }
        catch (const std::exception& e) { return foundation::Result<void>::Failure(Failure(e)); }
    }
    foundation::Result<std::unique_ptr<IRestoreStage>> StageRestore(const SaveSection& section, const RestoreContext&) override
    {
        try
        {
            if (fail_snapshot_prepare_for_test) throw std::bad_alloc();
            auto snapshot = DecodeParticipantSnapshot<Snapshot>(section, id_);
            auto candidate = Access::Clone(owner_, workspace_);
            auto valid = Access::Restore(*candidate, std::move(snapshot), workspace_);
            if (!valid) return foundation::Result<std::unique_ptr<IRestoreStage>>::Failure(valid.GetError());
            auto stage = std::make_unique<Stage>(*this, std::move(candidate));
            auto [entry, inserted] = workspace_.staged_.emplace(typeid(Owner), &Access::View(*stage->prepared));
            if (!inserted) throw std::runtime_error("owner already has a prepared restore stage");
            return foundation::Result<std::unique_ptr<IRestoreStage>>::Success(std::move(stage));
        }
        catch (const std::exception& e) { return foundation::Result<std::unique_ptr<IRestoreStage>>::Failure(Failure(e)); }
    }
    void CommitRestore(IRestoreStage& abstract_stage) noexcept override
    {
        auto* stage = dynamic_cast<Stage*>(&abstract_stage);
        if (!stage || stage->participant != this || stage->committed) return;
        Access::Commit(owner_, *stage->prepared);
        RemoveStage(*stage); stage->committed = true;
    }
  private:
    static foundation::Error Failure(const std::exception& error)
    { return foundation::Error::Create("gameplay.save.participant_payload", error.what()); }
    void RemoveStage(Stage& stage) noexcept
    {
        const auto found = workspace_.staged_.find(typeid(Owner));
        if (found != workspace_.staged_.end() && found->second == &Access::View(*stage.prepared)) workspace_.staged_.erase(found);
    }
    Owner& owner_;
    SaveRestoreWorkspace& workspace_;
    SaveParticipantId id_;
    std::vector<SaveParticipantId> dependencies_;
};
} // namespace epidemic::gameplay::savegame::detail


namespace epidemic::gameplay::savegame::detail
{
struct WorkspaceTestAccess
{
    template <typename Owner> static void Bind(SaveRestoreWorkspace& workspace, Owner& owner)
    { workspace.Bind(owner); }
    template <typename Owner> static Owner& Require(SaveRestoreWorkspace& workspace)
    { return workspace.Require<Owner>(); }
    template <typename Owner> static Owner& Resolve(SaveRestoreWorkspace& workspace, Owner& live) noexcept
    { return workspace.Resolve(live); }
};
inline void SetSnapshotPrepareFailureForTesting(bool fail) noexcept
{ fail_snapshot_prepare_for_test = fail; }
}

// BEGIN GENERATED GOAL5 VALUE VISITOR
namespace epidemic::gameplay::savegame::detail
{
template <typename T, typename F> void VisitSnapshotFields(T& value, F&& visitor)
{
    constexpr auto n = AggregateArity<T>();
    if constexpr (n == 0) visitor();
    else if constexpr (n == 1) { auto& [f0] = value; visitor(f0); }
    else if constexpr (n == 2) { auto& [f0, f1] = value; visitor(f0, f1); }
    else if constexpr (n == 3) { auto& [f0, f1, f2] = value; visitor(f0, f1, f2); }
    else if constexpr (n == 4) { auto& [f0, f1, f2, f3] = value; visitor(f0, f1, f2, f3); }
    else if constexpr (n == 5) { auto& [f0, f1, f2, f3, f4] = value; visitor(f0, f1, f2, f3, f4); }
    else if constexpr (n == 6) { auto& [f0, f1, f2, f3, f4, f5] = value; visitor(f0, f1, f2, f3, f4, f5); }
    else if constexpr (n == 7) { auto& [f0, f1, f2, f3, f4, f5, f6] = value; visitor(f0, f1, f2, f3, f4, f5, f6); }
    else if constexpr (n == 8) { auto& [f0, f1, f2, f3, f4, f5, f6, f7] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7); }
    else if constexpr (n == 9) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8); }
    else if constexpr (n == 10) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9); }
    else if constexpr (n == 11) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10); }
    else if constexpr (n == 12) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11); }
    else if constexpr (n == 13) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12); }
    else if constexpr (n == 14) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13); }
    else if constexpr (n == 15) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14); }
    else if constexpr (n == 16) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15); }
    else if constexpr (n == 17) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16); }
    else if constexpr (n == 18) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17); }
    else if constexpr (n == 19) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18); }
    else if constexpr (n == 20) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19); }
    else if constexpr (n == 21) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20); }
    else if constexpr (n == 22) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21); }
    else if constexpr (n == 23) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22); }
    else if constexpr (n == 24) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23); }
    else if constexpr (n == 25) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24); }
    else if constexpr (n == 26) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25); }
    else if constexpr (n == 27) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26); }
    else if constexpr (n == 28) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27); }
    else if constexpr (n == 29) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28); }
    else if constexpr (n == 30) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29); }
    else if constexpr (n == 31) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30); }
    else if constexpr (n == 32) { auto& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31] = value; visitor(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31); }
}
}
// END GENERATED GOAL5 VALUE VISITOR
