// Generated owner bindings; gameplay state remains authoritative in its original owner.
#include "Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h"
#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"

namespace epidemic::gameplay {
template <> struct SnapshotRestoreAccess<epidemic::gameplay::crime::CrimeService> {
    using Owner = epidemic::gameplay::crime::CrimeService;
    using Snapshot = epidemic::gameplay::crime::CrimeSnapshot;

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
        savegame::detail::SwapPreparedMember(live.active_bounty_index_,
                                             candidate.active_bounty_index_);
        savegame::detail::SwapPreparedMember(live.authorities_, candidate.authorities_);
        savegame::detail::SwapPreparedMember(live.authority_ids_, candidate.authority_ids_);
        savegame::detail::SwapPreparedMember(live.bounties_, candidate.bounties_);
        savegame::detail::SwapPreparedMember(live.bounty_ids_, candidate.bounty_ids_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.crime_ids_, candidate.crime_ids_);
        savegame::detail::SwapPreparedMember(live.crimes_, candidate.crimes_);
        savegame::detail::SwapPreparedMember(live.definitions_frozen_,
                                             candidate.definitions_frozen_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.evidence_, candidate.evidence_);
        savegame::detail::SwapPreparedMember(live.evidence_ids_, candidate.evidence_ids_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.jurisdictions_, candidate.jurisdictions_);
        savegame::detail::SwapPreparedMember(live.laws_, candidate.laws_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.response_definitions_,
                                             candidate.response_definitions_);
        savegame::detail::SwapPreparedMember(live.response_ids_, candidate.response_ids_);
        savegame::detail::SwapPreparedMember(live.responses_, candidate.responses_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.witness_ids_, candidate.witness_ids_);
        savegame::detail::SwapPreparedMember(live.witnesses_, candidate.witnesses_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::ownership::OwnershipService> {
    using Owner = epidemic::gameplay::ownership::OwnershipService;
    using Snapshot = epidemic::gameplay::ownership::OwnershipSnapshot;

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
        savegame::detail::SwapPreparedMember(live.canonical_owners_, candidate.canonical_owners_);
        savegame::detail::SwapPreparedMember(live.changes_, candidate.changes_);
        savegame::detail::SwapPreparedMember(live.claim_ids_, candidate.claim_ids_);
        savegame::detail::SwapPreparedMember(live.claims_, candidate.claims_);
        savegame::detail::SwapPreparedMember(live.claims_by_property_,
                                             candidate.claims_by_property_);
        savegame::detail::SwapPreparedMember(live.diagnostics_, candidate.diagnostics_);
        savegame::detail::SwapPreparedMember(live.grant_ids_, candidate.grant_ids_);
        savegame::detail::SwapPreparedMember(live.grants_, candidate.grants_);
        savegame::detail::SwapPreparedMember(live.grants_by_subject_, candidate.grants_by_subject_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.ownership_by_owner_,
                                             candidate.ownership_by_owner_);
        savegame::detail::SwapPreparedMember(live.ownership_by_property_,
                                             candidate.ownership_by_property_);
        savegame::detail::SwapPreparedMember(live.ownership_ids_, candidate.ownership_ids_);
        savegame::detail::SwapPreparedMember(live.records_, candidate.records_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
        savegame::detail::SwapPreparedMember(live.rule_ids_, candidate.rule_ids_);
        savegame::detail::SwapPreparedMember(live.rules_, candidate.rules_);
        savegame::detail::SwapPreparedMember(live.rules_by_property_, candidate.rules_by_property_);
    }

};

template <> struct SnapshotRestoreAccess<epidemic::gameplay::society::SocietyService> {
    using Owner = epidemic::gameplay::society::SocietyService;
    using Snapshot = epidemic::gameplay::society::SocietySnapshot;

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
        savegame::detail::SwapPreparedMember(live.definitions_frozen_,
                                             candidate.definitions_frozen_);
        savegame::detail::SwapPreparedMember(live.groups_, candidate.groups_);
        savegame::detail::SwapPreparedMember(live.journal_epoch_, candidate.journal_epoch_);
        savegame::detail::SwapPreparedMember(live.membership_by_key_, candidate.membership_by_key_);
        savegame::detail::SwapPreparedMember(live.membership_ids_, candidate.membership_ids_);
        savegame::detail::SwapPreparedMember(live.memberships_, candidate.memberships_);
        savegame::detail::SwapPreparedMember(live.memberships_by_group_,
                                             candidate.memberships_by_group_);
        savegame::detail::SwapPreparedMember(live.memberships_by_member_,
                                             candidate.memberships_by_member_);
        savegame::detail::SwapPreparedMember(live.next_change_sequence_,
                                             candidate.next_change_sequence_);
        savegame::detail::SwapPreparedMember(live.relationship_by_key_,
                                             candidate.relationship_by_key_);
        savegame::detail::SwapPreparedMember(live.relationship_ids_, candidate.relationship_ids_);
        savegame::detail::SwapPreparedMember(live.relationship_types_,
                                             candidate.relationship_types_);
        savegame::detail::SwapPreparedMember(live.relationships_, candidate.relationships_);
        savegame::detail::SwapPreparedMember(live.relationships_by_subject_,
                                             candidate.relationships_by_subject_);
        savegame::detail::SwapPreparedMember(live.reputation_tracks_, candidate.reputation_tracks_);
        savegame::detail::SwapPreparedMember(live.reputations_, candidate.reputations_);
        savegame::detail::SwapPreparedMember(live.reputations_by_subject_,
                                             candidate.reputations_by_subject_);
        savegame::detail::SwapPreparedMember(live.revision_, candidate.revision_);
    }

};

template <>
struct SnapshotRestoreAccess<epidemic::gameplay::social_legal_integration::CrimeSocietyAdapter> {
    using Owner = epidemic::gameplay::social_legal_integration::CrimeSocietyAdapter;
    using Snapshot = epidemic::gameplay::social_legal_integration::SocialLegalCheckpoint;

    static Snapshot Capture(const Owner &o) {
        return o.CaptureCheckpoint();
    }

    static std::unique_ptr<Owner> Clone(const Owner &o, savegame::SaveRestoreWorkspace &w) {
        (void)w;
        auto copy =
            std::make_unique<Owner>(w.Resolve(o.crime_), w.Resolve(o.society_), o.mappings_);
        return copy;
    }

    static ::epidemic::foundation::Result<void> Restore(Owner &o, Snapshot s,
                                                        savegame::SaveRestoreWorkspace &w) {
        (void)w;
        return o.RestoreCheckpoint(std::move(s));
    }

    static Owner &View(Owner &o) noexcept {
        return o;
    }

    static void Commit(Owner &live, Owner &candidate) noexcept {
        savegame::detail::SwapPreparedMember(live.applied_relationship_penalties_,
                                             candidate.applied_relationship_penalties_);
        savegame::detail::SwapPreparedMember(live.crime_cursor_, candidate.crime_cursor_);
    }

};

} // namespace epidemic::gameplay

namespace epidemic::gameplay::savegame {
std::unique_ptr<ISaveParticipant>
CreateCrimeServiceSaveParticipant(epidemic::gameplay::crime::CrimeService &owner,
                                  SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::crime::CrimeService>>(owner, workspace, "framework.crime",
                                                  std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateOwnershipServiceSaveParticipant(epidemic::gameplay::ownership::OwnershipService &owner,
                                      SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::ownership::OwnershipService>>(owner, workspace, "framework.ownership",
                                                          std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant>
CreateSocietyServiceSaveParticipant(epidemic::gameplay::society::SocietyService &owner,
                                    SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::society::SocietyService>>(owner, workspace, "framework.society",
                                                      std::vector<SaveParticipantId>{});
}

std::unique_ptr<ISaveParticipant> CreateCrimeSocietyAdapterSaveParticipant(
    epidemic::gameplay::social_legal_integration::CrimeSocietyAdapter &owner,
    SaveRestoreWorkspace &workspace) {
    return std::make_unique<detail::SnapshotParticipant<
        epidemic::gameplay::social_legal_integration::CrimeSocietyAdapter>>(
        owner, workspace, "integration.social_legal",
        std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.crime"),
                                       SaveParticipantId::FromString("framework.society")});
}

} // namespace epidemic::gameplay::savegame
