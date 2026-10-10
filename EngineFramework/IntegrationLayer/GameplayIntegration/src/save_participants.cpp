// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay
{
template <> struct SnapshotRestoreAccess<epidemic::gameplay::abilities::AbilityService>
{
    using Owner = epidemic::gameplay::abilities::AbilityService;
    using Snapshot = epidemic::gameplay::abilities::AbilitiesSnapshot;
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
        savegame::detail::SwapPreparedMember(live.cooldowns_, candidate.cooldowns_);
        savegame::detail::SwapPreparedMember(live.execution_ids_, candidate.execution_ids_);
        savegame::detail::SwapPreparedMember(live.executions_, candidate.executions_);
        savegame::detail::SwapPreparedMember(live.instance_ids_, candidate.instance_ids_);
        savegame::detail::SwapPreparedMember(live.instances_, candidate.instances_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.reconciled_reservations_, candidate.reconciled_reservations_);
        savegame::detail::SwapPreparedMember(live.resource_reconciliation_required_, candidate.resource_reconciliation_required_);
        savegame::detail::SwapPreparedMember(live.schedule_to_execution_, candidate.schedule_to_execution_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::combat::CombatService>
{
    using Owner = epidemic::gameplay::combat::CombatService;
    using Snapshot = epidemic::gameplay::combat::CombatSnapshot;
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
        savegame::detail::SwapPreparedMember(live.combatants_, candidate.combatants_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.prepared_plans_, candidate.prepared_plans_);
        savegame::detail::SwapPreparedMember(live.resolution_ids_, candidate.resolution_ids_);
        savegame::detail::SwapPreparedMember(live.resource_reservation_ids_, candidate.resource_reservation_ids_);
        savegame::detail::SwapPreparedMember(live.resource_reservations_, candidate.resource_reservations_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::loot::LootService>
{
    using Owner = epidemic::gameplay::loot::LootService;
    using Snapshot = epidemic::gameplay::loot::LootSnapshot;
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
        savegame::detail::SwapPreparedMember(live.claimed_, candidate.claimed_);
        savegame::detail::SwapPreparedMember(live.claimed_history_floor_low_, candidate.claimed_history_floor_low_);
        savegame::detail::SwapPreparedMember(live.claimed_order_, candidate.claimed_order_);
        savegame::detail::SwapPreparedMember(live.execution_ids_, candidate.execution_ids_);
        savegame::detail::SwapPreparedMember(live.generated_, candidate.generated_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.pending_, candidate.pending_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::progression::ProgressionService>
{
    using Owner = epidemic::gameplay::progression::ProgressionService;
    using Snapshot = epidemic::gameplay::progression::ProgressionSnapshot;
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
        savegame::detail::SwapPreparedMember(live.grant_reservation_ids_, candidate.grant_reservation_ids_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.modifier_ids_, candidate.modifier_ids_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.pending_progress_grants_, candidate.pending_progress_grants_);
        savegame::detail::SwapPreparedMember(live.profiles_, candidate.profiles_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::integration::AbilityEffectsDispatcher>
{
    using Owner = epidemic::gameplay::integration::AbilityEffectsDispatcher;
    using Snapshot = epidemic::gameplay::integration::AbilityEffectsCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.effects_));
        copy->mappings_ = o.mappings_;
        copy->mappings_frozen_ = o.mappings_frozen_;
        copy->mapping_revision_ = o.mapping_revision_;
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.delivered_, candidate.delivered_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::integration::AbilityTimeAdapter>
{
    using Owner = epidemic::gameplay::integration::AbilityTimeAdapter;
    using Snapshot = epidemic::gameplay::integration::AbilityTimeCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.time_), w.Resolve(o.abilities_), o.clock_, o.action_);
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.pending_, candidate.pending_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::integration::ConditionProgressionAdapter>
{
    using Owner = epidemic::gameplay::integration::ConditionProgressionAdapter;
    using Snapshot = epidemic::gameplay::integration::ConditionProgressionCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.conditions_), w.Resolve(o.progression_));
        copy->mappings_ = o.mappings_;
        copy->mappings_frozen_ = o.mappings_frozen_;
        copy->mapping_revision_ = o.mapping_revision_;
        return copy;
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreCheckpoint(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.cursor_, candidate.cursor_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::integration::DeathRewardAdapter>
{
    using Owner = epidemic::gameplay::integration::DeathRewardAdapter;
    using Snapshot = epidemic::gameplay::integration::DeathRewardCheckpoint;
    static Snapshot Capture(const Owner& o) { return o.CaptureCheckpoint(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.combat_), w.Resolve(o.loot_));
        copy->tables_ = o.tables_;
        copy->mappings_frozen_ = o.mappings_frozen_;
        copy->mapping_revision_ = o.mapping_revision_;
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
}
namespace epidemic::gameplay::savegame
{
std::unique_ptr<ISaveParticipant> CreateAbilityServiceSaveParticipant(epidemic::gameplay::abilities::AbilityService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::abilities::AbilityService>>(owner, workspace, "framework.abilities", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateCombatServiceSaveParticipant(epidemic::gameplay::combat::CombatService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::combat::CombatService>>(owner, workspace, "framework.combat", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateLootServiceSaveParticipant(epidemic::gameplay::loot::LootService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::loot::LootService>>(owner, workspace, "framework.loot", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateProgressionServiceSaveParticipant(epidemic::gameplay::progression::ProgressionService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::progression::ProgressionService>>(owner, workspace, "framework.progression", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateAbilityEffectsDispatcherSaveParticipant(epidemic::gameplay::integration::AbilityEffectsDispatcher& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityEffectsDispatcher>>(owner, workspace, "integration.ability_effects", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.effects")});
}
std::unique_ptr<ISaveParticipant> CreateAbilityTimeAdapterSaveParticipant(epidemic::gameplay::integration::AbilityTimeAdapter& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityTimeAdapter>>(owner, workspace, "integration.ability_time", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.time")});
}
std::unique_ptr<ISaveParticipant> CreateConditionProgressionAdapterSaveParticipant(epidemic::gameplay::integration::ConditionProgressionAdapter& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::ConditionProgressionAdapter>>(owner, workspace, "integration.condition_progression", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.conditions"), SaveParticipantId::FromString("framework.progression")});
}
std::unique_ptr<ISaveParticipant> CreateDeathRewardAdapterSaveParticipant(epidemic::gameplay::integration::DeathRewardAdapter& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::DeathRewardAdapter>>(owner, workspace, "integration.death_reward", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.combat"), SaveParticipantId::FromString("framework.loot")});
}
}
