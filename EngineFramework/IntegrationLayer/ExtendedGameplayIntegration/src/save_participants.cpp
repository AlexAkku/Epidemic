// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"

namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<epidemic::gameplay::dialogue::DialogueService> {
    using Owner = epidemic::gameplay::dialogue::DialogueService;
    using Snapshot = epidemic::gameplay::dialogue::DialogueSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.consequence_ids_, candidate.consequence_ids_);
        savegame::detail::SwapPreparedMember(live.consequences_, candidate.consequences_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.session_ids_, candidate.session_ids_);
        savegame::detail::SwapPreparedMember(live.sessions_, candidate.sessions_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::economy::EconomyService> {
    using Owner = epidemic::gameplay::economy::EconomyService;
    using Snapshot = epidemic::gameplay::economy::EconomySnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.account_ids_, candidate.account_ids_);
        savegame::detail::SwapPreparedMember(live.accounts_, candidate.accounts_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.contract_ids_, candidate.contract_ids_);
        savegame::detail::SwapPreparedMember(live.contracts_, candidate.contracts_);
        savegame::detail::SwapPreparedMember(live.debt_ids_, candidate.debt_ids_);
        savegame::detail::SwapPreparedMember(live.debts_, candidate.debts_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.indicators_, candidate.indicators_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.market_ids_, candidate.market_ids_);
        savegame::detail::SwapPreparedMember(live.markets_, candidate.markets_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.offer_ids_, candidate.offer_ids_);
        savegame::detail::SwapPreparedMember(live.offers_, candidate.offers_);
        savegame::detail::SwapPreparedMember(live.reservation_ids_, candidate.reservation_ids_);
        savegame::detail::SwapPreparedMember(live.reservations_, candidate.reservations_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.transaction_ids_, candidate.transaction_ids_);
        savegame::detail::SwapPreparedMember(live.transactions_, candidate.transactions_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::equipment::EquipmentService> {
    using Owner = epidemic::gameplay::equipment::EquipmentService;
    using Snapshot = epidemic::gameplay::equipment::EquipmentSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.binding_ids_, candidate.binding_ids_);
        savegame::detail::SwapPreparedMember(live.bindings_, candidate.bindings_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.loadout_ids_, candidate.loadout_ids_);
        savegame::detail::SwapPreparedMember(live.loadouts_, candidate.loadouts_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.operation_ids_, candidate.operation_ids_);
        savegame::detail::SwapPreparedMember(live.profile_by_subject_,
                                             candidate.profile_by_subject_);
        savegame::detail::SwapPreparedMember(live.profile_ids_, candidate.profile_ids_);
        savegame::detail::SwapPreparedMember(live.profiles_, candidate.profiles_);
        savegame::detail::SwapPreparedMember(live.reconciliation_state_,
                                             candidate.reconciliation_state_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.slot_ids_, candidate.slot_ids_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::items::ItemsInventoryService> {
    using Owner = epidemic::gameplay::items::ItemsInventoryService;
    using Snapshot = epidemic::gameplay::items::ItemsSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return std::make_unique<Owner>(o);
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.active_reserved_quantities_,
                                             candidate.active_reserved_quantities_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.container_ids_, candidate.container_ids_);
        savegame::detail::SwapPreparedMember(live.container_items_, candidate.container_items_);
        savegame::detail::SwapPreparedMember(live.containers_, candidate.containers_);
        savegame::detail::SwapPreparedMember(live.definition_items_, candidate.definition_items_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.indexes_valid_, candidate.indexes_valid_);
        savegame::detail::SwapPreparedMember(live.item_ids_, candidate.item_ids_);
        savegame::detail::SwapPreparedMember(live.items_, candidate.items_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.reservation_ids_, candidate.reservation_ids_);
        savegame::detail::SwapPreparedMember(live.reservations_, candidate.reservations_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.transfer_ids_, candidate.transfer_ids_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::integration::TradeCoordinator> {
    using Owner = epidemic::gameplay::integration::TradeCoordinator;
    using Snapshot = epidemic::gameplay::integration::TradeCoordinatorSnapshot;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureSnapshot();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        auto copy = std::make_unique<Owner>(w.Resolve(o.economy_), w.Resolve(o.items_),
                                            w.Resolve(o.ownership_));
        return copy;
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreSnapshot(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.execution_ids_, candidate.execution_ids_);
        savegame::detail::SwapPreparedMember(live.executions_, candidate.executions_);
    }

};

} // namespace epidemic::gameplay

namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant>
CreateDialogueServiceSaveParticipant(epidemic::gameplay::dialogue::DialogueService &owner,
                                     SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::dialogue::DialogueService>>(owner, workspace, "framework.dialogue",
                                                        std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateEconomyServiceSaveParticipant(epidemic::gameplay::economy::EconomyService &owner,
                                    SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::economy::EconomyService>>(owner, workspace, "framework.economy",
                                                      std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateEquipmentServiceSaveParticipant(epidemic::gameplay::equipment::EquipmentService &owner,
                                      SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::equipment::EquipmentService>>(owner, workspace, "framework.equipment",
                                                          std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateItemsInventoryServiceSaveParticipant(epidemic::gameplay::items::ItemsInventoryService &owner,
                                           SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::items::ItemsInventoryService>>(owner, workspace, "framework.items",
                                                           std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateTradeCoordinatorSaveParticipant(epidemic::gameplay::integration::TradeCoordinator &owner,
                                      SaveRestoreWorkspace &workspace) {
    return std::make_unique<
        detail::SnapshotParticipant<epidemic::gameplay::integration::TradeCoordinator>>(
        owner, workspace, "integration.trade",
        std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.economy"),
                                       SaveParticipantId::FromString("framework.items"),
                                       SaveParticipantId::FromString("framework.ownership")});
}

} // namespace epidemic::gameplay::savegame
