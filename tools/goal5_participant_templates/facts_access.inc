namespace epidemic::gameplay
{
template <> struct SnapshotRestoreAccess<facts::GameplayFactsService>
{
    using Owner = facts::GameplayFactsService;
    using Snapshot = facts::FactsPersistenceSnapshot;
    static foundation::Result<Snapshot> Capture(const Owner& owner) { return owner.CapturePersistenceSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& owner, savegame::SaveRestoreWorkspace&)
    {
        if (owner.dispatching_) throw std::runtime_error("cannot stage facts during dispatch");
        auto copy = std::make_unique<Owner>();
        copy->event_types_ = owner.event_types_; copy->fact_types_ = owner.fact_types_;
        copy->event_codecs_ = owner.event_codecs_; copy->fact_codecs_ = owner.fact_codecs_;
        copy->frozen_ = owner.frozen_; copy->fact_changed_event_type_ = owner.fact_changed_event_type_;
        return copy;
    }
    static foundation::Result<void> Restore(Owner& candidate, Snapshot snapshot, savegame::SaveRestoreWorkspace&)
    { return candidate.RestorePersistenceSnapshot(std::move(snapshot)); }
    static Owner& View(Owner& candidate) noexcept { return candidate; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        // Immutable registrations, subscribers, mutex and operational counters
        // stay in the live service. All decoding/allocation was done in Clone/Restore.
        live.facts_.swap(candidate.facts_); live.history_.swap(candidate.history_);
        using std::swap;
        swap(live.fact_ids_, candidate.fact_ids_); swap(live.event_ids_, candidate.event_ids_);
        swap(live.fact_revision_, candidate.fact_revision_); swap(live.next_event_sequence_, candidate.next_event_sequence_);
        live.pending_events_.clear(); live.submitted_batches_.clear();
    }
};
}
