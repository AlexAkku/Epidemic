namespace epidemic::gameplay::savegame::detail
{
template <> struct SnapshotValueCodec<materials::SubstanceAmount>
{
    template <class IO> static void Transfer(IO& io, materials::SubstanceAmount& amount)
    { TransferSnapshot(io, amount.micro); }
};
}
namespace epidemic::gameplay
{
template <> struct SnapshotRestoreAccess<state_integration::StateIntegrationPersistence>
{
    using Owner = state_integration::StateIntegrationPersistence;
    using Snapshot = state_integration::StateIntegrationCheckpoint;
    using FactsAccess = SnapshotRestoreAccess<state_integration::StateFactsAdapter>;
    using LifecycleAccess = SnapshotRestoreAccess<state_integration::StateLifecycleAdapter>;
    using DeliveryAccess = SnapshotRestoreAccess<state_integration::ConditionEffectsAdapter>;
    using TimeAccess = SnapshotRestoreAccess<state_integration::StateTimeAdapter>;
    struct Prepared
    {
        std::unique_ptr<state_integration::StateFactsAdapter> facts;
        std::unique_ptr<state_integration::StateLifecycleAdapter> lifecycle;
        std::unique_ptr<state_integration::ConditionEffectsAdapter> delivery;
        std::unique_ptr<state_integration::StateTimeAdapter> time;
        Owner state;
        Prepared(const Owner& owner, savegame::SaveRestoreWorkspace& workspace)
            : facts(FactsAccess::Clone(owner.facts_, workspace)),
              lifecycle(LifecycleAccess::Clone(owner.lifecycle_, workspace)),
              delivery(DeliveryAccess::Clone(owner.condition_effects_, workspace)),
              time(TimeAccess::Clone(owner.time_, workspace)), state(*facts, *lifecycle, *delivery, *time) {}
    };
    static Snapshot Capture(const Owner& owner) { return owner.CaptureCheckpoint(); }
    static std::unique_ptr<Prepared> Clone(const Owner& owner, savegame::SaveRestoreWorkspace& workspace)
    { return std::make_unique<Prepared>(owner, workspace); }
    static foundation::Result<void> Restore(Prepared& candidate, Snapshot snapshot, savegame::SaveRestoreWorkspace&)
    { return candidate.state.RestoreCheckpoint(std::move(snapshot)); }
    static Owner& View(Prepared& candidate) noexcept { return candidate.state; }
    static void Commit(Owner& live, Prepared& candidate) noexcept
    {
        FactsAccess::Commit(live.facts_, *candidate.facts);
        LifecycleAccess::Commit(live.lifecycle_, *candidate.lifecycle);
        DeliveryAccess::Commit(live.condition_effects_, *candidate.delivery);
        TimeAccess::Commit(live.time_, *candidate.time);
    }
};
}
