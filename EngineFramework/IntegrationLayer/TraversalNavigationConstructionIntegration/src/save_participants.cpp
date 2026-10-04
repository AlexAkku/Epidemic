// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
namespace epidemic::gameplay
{
template <> struct SnapshotRestoreAccess<epidemic::gameplay::construction::ConstructionService>
{
    using Owner = epidemic::gameplay::construction::ConstructionService;
    using Snapshot = epidemic::gameplay::construction::ConstructionSnapshot;
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
        savegame::detail::SwapPreparedMember(live.dead_letters_, candidate.dead_letters_);
        savegame::detail::SwapPreparedMember(live.execution_ids_, candidate.execution_ids_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.outbox_, candidate.outbox_);
        savegame::detail::SwapPreparedMember(live.output_ids_, candidate.output_ids_);
        savegame::detail::SwapPreparedMember(live.placed_ids_, candidate.placed_ids_);
        savegame::detail::SwapPreparedMember(live.placed_objects_, candidate.placed_objects_);
        savegame::detail::SwapPreparedMember(live.plan_ids_, candidate.plan_ids_);
        savegame::detail::SwapPreparedMember(live.plans_, candidate.plans_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.site_ids_, candidate.site_ids_);
        savegame::detail::SwapPreparedMember(live.sites_, candidate.sites_);
        savegame::detail::SwapPreparedMember(live.socket_reservation_by_socket_, candidate.socket_reservation_by_socket_);
        savegame::detail::SwapPreparedMember(live.socket_reservation_ids_, candidate.socket_reservation_ids_);
        savegame::detail::SwapPreparedMember(live.socket_reservations_by_id_, candidate.socket_reservations_by_id_);
        savegame::detail::SwapPreparedMember(live.sockets_, candidate.sockets_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::navigation_semantics::NavigationSemanticsService>
{
    using Owner = epidemic::gameplay::navigation_semantics::NavigationSemanticsService;
    using Snapshot = epidemic::gameplay::navigation_semantics::NavigationSnapshot;
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
        savegame::detail::SwapPreparedMember(live.layer_ids_, candidate.layer_ids_);
        savegame::detail::SwapPreparedMember(live.layer_ids_by_area_, candidate.layer_ids_by_area_);
        savegame::detail::SwapPreparedMember(live.layers_, candidate.layers_);
        savegame::detail::SwapPreparedMember(live.link_ids_by_pair_, candidate.link_ids_by_pair_);
        savegame::detail::SwapPreparedMember(live.links_, candidate.links_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.profiles_, candidate.profiles_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    }
};
template <> struct SnapshotRestoreAccess<epidemic::gameplay::traversal::TraversalService>
{
    using Owner = epidemic::gameplay::traversal::TraversalService;
    using Snapshot = epidemic::gameplay::traversal::TraversalSnapshot;
    static Snapshot Capture(const Owner& o) { return o.CaptureSnapshot(); }
    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)
    {
        (void)w; return std::make_unique<Owner>(o);
    }
    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) { (void)w; return o.RestoreSnapshot(std::move(s)); }
    static Owner& View(Owner& o) noexcept { return o; }
    static void Commit(Owner& live, Owner& candidate) noexcept
    {
        savegame::detail::SwapPreparedMember(live.capability_grant_ids_, candidate.capability_grant_ids_);
        savegame::detail::SwapPreparedMember(live.capability_grants_, candidate.capability_grants_);
        savegame::detail::SwapPreparedMember(live.carrier_by_passenger_, candidate.carrier_by_passenger_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_, candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.route_ids_, candidate.route_ids_);
        savegame::detail::SwapPreparedMember(live.routes_, candidate.routes_);
        savegame::detail::SwapPreparedMember(live.session_ids_, candidate.session_ids_);
        savegame::detail::SwapPreparedMember(live.sessions_, candidate.sessions_);
        savegame::detail::SwapPreparedMember(live.states_, candidate.states_);
    }
};
}
namespace epidemic::gameplay::savegame
{
std::unique_ptr<ISaveParticipant> CreateConstructionServiceSaveParticipant(epidemic::gameplay::construction::ConstructionService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::construction::ConstructionService>>(owner, workspace, "framework.construction", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateNavigationSemanticsServiceSaveParticipant(epidemic::gameplay::navigation_semantics::NavigationSemanticsService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::navigation_semantics::NavigationSemanticsService>>(owner, workspace, "framework.navigation_semantics", std::vector<SaveParticipantId>{});
}
std::unique_ptr<ISaveParticipant> CreateTraversalServiceSaveParticipant(epidemic::gameplay::traversal::TraversalService& owner, SaveRestoreWorkspace& workspace)
{
    return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::traversal::TraversalService>>(owner, workspace, "framework.traversal", std::vector<SaveParticipantId>{});
}
}
