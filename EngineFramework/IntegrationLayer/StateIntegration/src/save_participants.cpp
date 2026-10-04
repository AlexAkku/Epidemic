// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/StateIntegration/state_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay
{
template <> struct SnapshotRestoreAccess<epidemic::gameplay::conditions::ConditionService>
{
    using Owner = epidemic::gameplay::conditions::ConditionService;
    using Snapshot = epidemic::gameplay::conditions::ConditionsSnapshot;
    static Snapshot Capture(const Owner& o) { return o.CaptureSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w; return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreSnapshot(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.applied_, candidate.applied_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.expired_, candidate.expired_);
        savegame::detail::SwapPreparedMember(live.id_to_index_, candidate.id_to_index_);
        savegame::detail::SwapPreparedMember(live.ids_, candidate.ids_);
        savegame::detail::SwapPreparedMember(live.instances_, candidate.instances_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.periodic_triggers_, candidate.periodic_triggers_);
        savegame::detail::SwapPreparedMember(live.removed_, candidate.removed_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.stack_merges_, candidate.stack_merges_);
        savegame::detail::SwapPreparedMember(live.subject_index_, candidate.subject_index_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::effects::EffectService>
{
    using Owner = epidemic::gameplay::effects::EffectService;
    using Snapshot = epidemic::gameplay::effects::EffectsSnapshot;
    static Snapshot Capture(const Owner& o) { return o.CaptureSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w; return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreSnapshot(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.budget_exhaustions_, candidate.budget_exhaustions_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.deferred_, candidate.deferred_);
        savegame::detail::SwapPreparedMember(live.deferred_by_schedule_, candidate.deferred_by_schedule_);
        savegame::detail::SwapPreparedMember(live.deferred_ids_, candidate.deferred_ids_);
        savegame::detail::SwapPreparedMember(live.derived_effects_, candidate.derived_effects_);
        savegame::detail::SwapPreparedMember(live.execution_ids_, candidate.execution_ids_);
        savegame::detail::SwapPreparedMember(live.executions_, candidate.executions_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.operations_, candidate.operations_);
        savegame::detail::SwapPreparedMember(live.rejections_, candidate.rejections_);
        savegame::detail::SwapPreparedMember(live.targets_, candidate.targets_);
        savegame::detail::SwapPreparedMember(live.waves_, candidate.waves_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::entities::EntityService>
{
    using Owner = epidemic::gameplay::entities::EntityService;
    using Snapshot = epidemic::gameplay::entities::EntitySnapshot;
    static Snapshot Capture(const Owner& o) { return o.CaptureSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w; return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreSnapshot(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.creates_, candidate.creates_);
        savegame::detail::SwapPreparedMember(live.destroys_, candidate.destroys_);
        savegame::detail::SwapPreparedMember(live.free_slots_, candidate.free_slots_);
        savegame::detail::SwapPreparedMember(live.id_to_slot_, candidate.id_to_slot_);
        savegame::detail::SwapPreparedMember(live.ids_, candidate.ids_);
        savegame::detail::SwapPreparedMember(live.invalid_handle_lookups_, candidate.invalid_handle_lookups_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.last_change_sequence_, candidate.last_change_sequence_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.pending_destroy_, candidate.pending_destroy_);
        savegame::detail::SwapPreparedMember(live.pending_destroy_contexts_, candidate.pending_destroy_contexts_);
        savegame::detail::SwapPreparedMember(live.pending_destroy_reasons_, candidate.pending_destroy_reasons_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.slots_, candidate.slots_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::materials::MaterialService>
{
    using Owner = epidemic::gameplay::materials::MaterialService;
    using Snapshot = epidemic::gameplay::materials::MaterialsSnapshot;
    static Snapshot Capture(const Owner& o) { return o.CaptureSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w; return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreSnapshot(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.last_change_sequence_, candidate.last_change_sequence_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.reaction_evaluations_, candidate.reaction_evaluations_);
        savegame::detail::SwapPreparedMember(live.reactions_triggered_, candidate.reactions_triggered_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.states_, candidate.states_);
        savegame::detail::SwapPreparedMember(live.stimuli_, candidate.stimuli_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::state_integration::StateFactsAdapter>
{
    using Owner = epidemic::gameplay::state_integration::StateFactsAdapter;
    using Snapshot = epidemic::gameplay::state_integration::StateFactsCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.entities_), w.Resolve(o.materials_), w.Resolve(o.conditions_), w.Resolve(o.effects_), w.Resolve(o.facts_));
        copy->entity_changed_ = o.entity_changed_;
        copy->material_changed_ = o.material_changed_;
        copy->condition_changed_ = o.condition_changed_;
        copy->effect_changed_ = o.effect_changed_;
        copy->active_condition_fact_ = o.active_condition_fact_;
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.entity_cursor_, candidate.entity_cursor_);
        savegame::detail::SwapPreparedMember(live.material_cursor_, candidate.material_cursor_);
        savegame::detail::SwapPreparedMember(live.condition_cursor_, candidate.condition_cursor_);
        savegame::detail::SwapPreparedMember(live.effect_cursor_, candidate.effect_cursor_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::state_integration::StateLifecycleAdapter>
{
    using Owner = epidemic::gameplay::state_integration::StateLifecycleAdapter;
    using Snapshot = epidemic::gameplay::state_integration::StateLifecycleCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.entities_), w.Resolve(o.materials_), w.Resolve(o.conditions_), w.Resolve(o.effects_));
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.cursor_, candidate.cursor_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::state_integration::ConditionEffectsAdapter>
{
    using Owner = epidemic::gameplay::state_integration::ConditionEffectsAdapter;
    using Snapshot = epidemic::gameplay::state_integration::ConditionEffectsCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.conditions_), w.Resolve(o.effects_));
        copy->routes_ = o.routes_;
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.cursor_, candidate.cursor_);
        savegame::detail::SwapPreparedMember(live.deliveries_, candidate.deliveries_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::state_integration::StateTimeAdapter>
{
    using Owner = epidemic::gameplay::state_integration::StateTimeAdapter;
    using Snapshot = epidemic::gameplay::state_integration::StateTimeCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.conditions_), w.Resolve(o.effects_), w.Resolve(o.time_));
        copy->expire_action_ = o.expire_action_;
        copy->periodic_action_ = o.periodic_action_;
        copy->deferred_effect_action_ = o.deferred_effect_action_;
        copy->dispatcher_ = o.dispatcher_;
        copy->effect_budget_ = o.effect_budget_;
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.condition_cursor_, candidate.condition_cursor_);
        savegame::detail::SwapPreparedMember(live.effect_cursor_, candidate.effect_cursor_);
        savegame::detail::SwapPreparedMember(live.deferred_reconciliations_, candidate.deferred_reconciliations_);
    }
};
}
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

namespace epidemic::gameplay::savegame
{
std::unique_ptr<ISaveParticipant> CreateConditionServiceSaveParticipant(epidemic::gameplay::conditions::ConditionService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::conditions::ConditionService>>(owner, workspace, "framework.conditions", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateEffectServiceSaveParticipant(epidemic::gameplay::effects::EffectService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::effects::EffectService>>(owner, workspace, "framework.effects", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateEntityServiceSaveParticipant(epidemic::gameplay::entities::EntityService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::entities::EntityService>>(owner, workspace, "framework.entities", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateMaterialServiceSaveParticipant(epidemic::gameplay::materials::MaterialService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::materials::MaterialService>>(owner, workspace, "framework.materials", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framework.conditions"), SaveParticipantId::FromString("framework.effects"), SaveParticipantId::FromString("framework.facts"), SaveParticipantId::FromString("framework.time")});
}
}
