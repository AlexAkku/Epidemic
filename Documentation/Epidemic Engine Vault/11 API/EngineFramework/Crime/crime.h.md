---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h"
---

# Crime: crime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`.

**Модуль:** [[06 EngineFramework/Модуль Crime|Crime]].
**Размер исходного файла:** 23339 байт.
**Подключение:** `#include <Epidemic/GameFramework/Crime/crime.h>`.

## Объявления типов (лексический индекс)
`LawId`, `CrimeTypeId`, `JurisdictionId`, `CrimeRecordId`, `EvidenceId`, `WitnessRecordId`, `BountyRecordId`, `AuthorityId`, `LawResponseId`, `LawResponseTypeId`, `EvidenceTypeId`, `WitnessTypeId`, `IdHash`, `CrimeProofState`, `CrimeCaseState`, `BountyState`, `CrimeChangeKind`, `CrimeCandidatePolicy`, `LawResponseDefinition`, `LawDefinition`, `JurisdictionRecord`, `AuthorityRecord`, `CrimeCandidateWitness`, `CrimeCandidate`, `WitnessRecord`, `EvidenceRecord`, `CrimeRecord`, `BountyRecord`, `LawResponseRequest`, `LawResponseRecord`, `CrimeEvaluationResult`, `LegalityPreview`, `CrimeChange`, `CrimeSnapshot`, `CrimeChangeBatch`, `CrimeDiagnostics`, `CrimeService`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::crime
{
struct LawId
{
    TypeId value{};
    static constexpr LawId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const LawId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const LawId &) const noexcept = default;
};
struct CrimeTypeId
{
    TypeId value{};
    static constexpr CrimeTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const CrimeTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const CrimeTypeId &) const noexcept = default;
};
struct JurisdictionId
{
    GameplayObjectId value{};
    static constexpr JurisdictionId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr JurisdictionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const JurisdictionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JurisdictionId &) const noexcept = default;
};
struct CrimeRecordId
{
    GameplayObjectId value{};
    static constexpr CrimeRecordId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr CrimeRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const CrimeRecordId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const CrimeRecordId &) const noexcept = default;
};
struct EvidenceId
{
    GameplayObjectId value{};
    static constexpr EvidenceId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr EvidenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const EvidenceId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EvidenceId &) const noexcept = default;
};
struct WitnessRecordId
{
    GameplayObjectId value{};
    static constexpr WitnessRecordId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr WitnessRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WitnessRecordId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WitnessRecordId &) const noexcept = default;
};
struct BountyRecordId
{
    GameplayObjectId value{};
    static constexpr BountyRecordId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr BountyRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const BountyRecordId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const BountyRecordId &) const noexcept = default;
};
struct AuthorityId
{
    GameplayObjectId value{};
    static constexpr AuthorityId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr AuthorityId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const AuthorityId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const AuthorityId &) const noexcept = default;
};
struct LawResponseId
{
    GameplayObjectId value{};
    static constexpr LawResponseId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr LawResponseId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const LawResponseId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const LawResponseId &) const noexcept = default;
};
struct LawResponseTypeId
{
    TypeId value{};
    static constexpr LawResponseTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const LawResponseTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const LawResponseTypeId &) const noexcept = default;
};
struct EvidenceTypeId
{
    TypeId value{};
    static constexpr EvidenceTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const EvidenceTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EvidenceTypeId &) const noexcept = default;
};
struct WitnessTypeId
{
    TypeId value{};
    static constexpr WitnessTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WitnessTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WitnessTypeId &) const noexcept = default;
};
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

enum class CrimeProofState
{
    Unknown,
    Suspected,
    Witnessed,
    Reported,
    EvidenceFound,
    Confirmed,
    FalseAccusation,
    Resolved
};
enum class CrimeCaseState
{
    Open,
    Reported,
    UnderInvestigation,
    Wanted,
    Punished,
    Forgiven,
    Expired,
    Dismissed,
    Resolved
};
enum class BountyState
{
    Active,
    Paid,
    Forgiven,
    Expired,
    Transferred
};
enum class CrimeChangeKind
{
    CrimeCandidateCreated,
    CrimeRecorded,
    CrimeReported,
    WitnessAdded,
    WitnessRemoved,
    EvidenceAdded,
    EvidenceRemoved,
    ProofStateChanged,
    CaseStateChanged,
    BountyCreated,
    BountyChanged,
    BountyResolved,
    LawResponseGenerated,
    CrimeExpired,
    CrimeDismissed,
    CrimePruned
};
enum class CrimeCandidatePolicy
{
    RecordAlways,
    RecordIfWitnessed,
    IgnoreWithoutJurisdiction
};

struct LawResponseDefinition
{
    LawResponseTypeId id{};
    std::uint32_t required_authority_capabilities = 0;
    std::optional<CrimeCaseState> resulting_case_state;
    Revision revision{};
};
struct LawDefinition
{
    LawId id{};
    CrimeTypeId crime_type{};
    GameplayTagSet tags;
    std::int64_t severity_micro = 0;
    std::int64_t default_bounty = 0;
    GameplayDuration statute_of_limitations{};
    Revision revision{};
};
struct JurisdictionRecord
{
    JurisdictionId id{};
    GameplayObjectRef authority_group{};
    GameplayObjectRef area{};
    std::vector<LawId> active_laws;
    int priority = 0;
    Revision revision{};
};
struct AuthorityRecord
{
    AuthorityId id{};
    GameplayObjectRef authority_group{};
    JurisdictionId jurisdiction{};
    std::uint32_t capabilities = 0;
    Revision revision{};
};
struct CrimeCandidateWitness
{
    GameplayObjectRef witness{};
    WitnessTypeId witness_type{};
    std::int64_t confidence_micro = 0;
    GameplayTimePoint observed_at{};
    std::vector<std::byte> payload;
};
struct CrimeCandidate
{
    CrimeTypeId type{};
    GameplayObjectRef offender{};
    GameplayObjectRef victim{};
    GameplayObjectRef target_property{};
    GameplayObjectRef area{};
    GameplayTimePoint time{};
    GameplayContext context{};
    std::vector<std::byte> payload;
    std::optional<CrimeCandidateWitness> initial_witness;
};
struct WitnessRecord
{
    WitnessRecordId id{};
    CrimeRecordId crime{};
    GameplayObjectRef witness{};
    WitnessTypeId witness_type{};
    std::int64_t confidence_micro = 0;
    GameplayTimePoint observed_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct EvidenceRecord
{
    EvidenceId id{};
    CrimeRecordId crime{};
    EvidenceTypeId type{};
    GameplayObjectRef source{};
    std::int64_t strength_micro = 0;
    GameplayTimePoint discovered_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct CrimeRecord
{
    CrimeRecordId id{};
    CrimeTypeId type{};
    GameplayObjectRef offender{};
    GameplayObjectRef victim{};
    GameplayObjectRef target_property{};
    JurisdictionId jurisdiction{};
    std::int64_t severity_micro = 0;
    CrimeProofState proof_state = CrimeProofState::Unknown;
    CrimeCaseState state = CrimeCaseState::Open;
    GameplayTimePoint committed_at{};
    std::optional<GameplayTimePoint> expires_at;
    std::vector<WitnessRecordId> witnesses;
    std::vector<EvidenceId> evidence;
    std::vector<std::byte> payload;
    Revision revision{};
};
struct BountyRecord
{
    BountyRecordId id{};
    GameplayObjectRef offender{};
    JurisdictionId jurisdiction{};
    std::int64_t amount = 0;
    BountyState state = BountyState::Active;
    std::optional<GameplayTimePoint> expires_at;
    std::vector<CrimeRecordId> source_crimes;
    Revision revision{};
};
struct LawResponseRequest
{
    CrimeRecordId crime{};
    LawResponseTypeId response_type{};
    GameplayObjectRef authority{};
    GameplayObjectRef target{};
    GameplayContext context{};
};
struct LawResponseRecord
{
    LawResponseId id{};
    CrimeRecordId crime{};
    LawResponseTypeId response_type{};
    GameplayObjectRef authority{};
    GameplayObjectRef target{};
    Revision revision{};
};
struct CrimeEvaluationResult
{
    std::optional<CrimeRecordId> crime;
    CrimeProofState proof_state = CrimeProofState::Unknown;
    JurisdictionId jurisdiction{};
    std::vector<TypeId> reasons;
    Revision revision{};
};
struct LegalityPreview
{
    bool illegal = false;
    CrimeTypeId crime_type{};
    JurisdictionId jurisdiction{};
    std::int64_t severity_micro = 0;
    Revision revision{};
};
struct CrimeChange
{
    std::uint64_t sequence = 0;
    CrimeChangeKind kind = CrimeChangeKind::CrimeCandidateCreated;
    CrimeRecordId crime{};
    GameplayObjectRef offender{};
    GameplayObjectRef victim{};
    JurisdictionId jurisdiction{};
    WitnessRecordId witness{};
    EvidenceId evidence{};
    BountyRecordId bounty{};
    LawResponseId response{};
    GameplayContext context{};
    Revision revision{};
};
struct CrimeSnapshot
{
    std::vector<LawDefinition> laws;
    std::vector<LawResponseDefinition> response_definitions;
    std::vector<JurisdictionRecord> jurisdictions;
    std::vector<AuthorityRecord> authorities;
    std::vector<CrimeRecord> crimes;
    std::vector<WitnessRecord> witnesses;
    std::vector<EvidenceRecord> evidence;
    std::vector<BountyRecord> bounties;
    std::vector<LawResponseRecord> responses;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot crime_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot witness_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot evidence_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot bounty_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot authority_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot response_ids{};
    Revision revision{};
    bool definitions_frozen = false;

    std::uint64_t change_epoch = 1;
};
struct CrimeChangeBatch
{
    std::vector<CrimeChange> changes;
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct CrimeDiagnostics
{
    std::uint64_t crime_candidates = 0, crime_records = 0, open_cases = 0, wanted_subjects = 0, witnesses = 0,
                  evidence_records = 0, bounties = 0, law_responses = 0, jurisdiction_resolutions = 0,
                  expired_crimes = 0, dismissed_crimes = 0;
};

class CrimeService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<CrimeService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.crime");
    }
    [[nodiscard]] foundation::Result<void> RegisterLaw(LawDefinition law);
    [[nodiscard]] foundation::Result<void> RegisterLawResponseDefinition(LawResponseDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterJurisdiction(JurisdictionRecord jurisdiction);
    [[nodiscard]] foundation::Result<AuthorityId> RegisterAuthority(AuthorityRecord authority);
    [[nodiscard]] foundation::Result<void> FreezeDefinitions();
    [[nodiscard]] bool DefinitionsFrozen() const noexcept { return definitions_frozen_; }
    [[nodiscard]] std::optional<JurisdictionRecord> GetApplicableJurisdiction(GameplayObjectRef area) const;
    [[nodiscard]] foundation::Result<CrimeEvaluationResult> EvaluateCrimeCandidate(
        CrimeCandidate candidate, CrimeCandidatePolicy policy = CrimeCandidatePolicy::RecordAlways);
    [[nodiscard]] foundation::Result<WitnessRecordId> AddWitness(WitnessRecord witness, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveWitness(WitnessRecordId witness, GameplayContext context = {});
    [[nodiscard]] foundation::Result<EvidenceId> AddEvidence(EvidenceRecord evidence, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveEvidence(EvidenceId evidence, GameplayContext context = {});
    [[nodiscard]] foundation::Result<BountyRecordId> CreateBounty(BountyRecord bounty);
    [[nodiscard]] foundation::Result<void> ResolveBounty(BountyRecordId id, BountyState state,
                                                         GameplayContext context = {});
    [[nodiscard]] foundation::Result<LawResponseId> GenerateLawResponse(LawResponseRequest request);
    [[nodiscard]] foundation::Result<void> ChangeCaseState(CrimeRecordId id, CrimeCaseState state,
                                                           GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ChangeProofState(CrimeRecordId id, CrimeProofState state,
                                                            GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ExpireCrime(CrimeRecordId id, GameplayContext context = {});
    [[nodiscard]] std::size_t ExpireDue(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> PruneTerminalCrime(CrimeRecordId id, GameplayContext context = {});
    [[nodiscard]] LegalityPreview PreviewLegality(GameplayObjectRef offender, CrimeTypeId type,
                                                  GameplayObjectRef area) const;
    [[nodiscard]] std::vector<CrimeRecord> FindCrimesByOffender(GameplayObjectRef offender) const;
    [[nodiscard]] std::vector<CrimeRecord> FindCrimesByVictim(GameplayObjectRef victim) const;
    [[nodiscard]] std::vector<CrimeRecord> FindCrimesByJurisdiction(JurisdictionId jurisdiction) const;
    [[nodiscard]] std::vector<CrimeRecord> FindOpenCases() const;
    [[nodiscard]] std::vector<WitnessRecord> FindWitnesses(CrimeRecordId crime) const;
    [[nodiscard]] std::vector<EvidenceRecord> FindEvidence(CrimeRecordId crime) const;
    [[nodiscard]] std::optional<BountyRecord> GetBounty(GameplayObjectRef offender, JurisdictionId jurisdiction) const;
    private:
        [[nodiscard]] std::vector<CrimeChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] CrimeChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] CrimeChangeBatch ReadChangesSince(ChangeCursor cursor) const
    {
        auto batch = ReadChangesSinceSequence(cursor.sequence);
        batch.oldest_available_cursor = {journal_epoch_, batch.oldest_available_sequence};
        batch.latest_cursor = {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                                    : next_change_sequence_ - 1};
        if ((!cursor.IsValid() && cursor.sequence != 0) || (cursor.IsValid() && cursor.epoch != journal_epoch_))
        {
            batch.changes.clear();
            batch.snapshot_required = true;
        }
        return batch;
    }
    [[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept
    {
        return {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                          : next_change_sequence_ - 1};
    }
    void PruneChangesThrough(std::uint64_t sequence);
    void SetChangeJournalCapacity(std::size_t capacity) noexcept;
    [[nodiscard]] CrimeSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(CrimeSnapshot snapshot);
    [[nodiscard]] CrimeDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] const CrimeRecord *FindCrime(CrimeRecordId id) const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept
    {
        return revision_;
    }

  private:
    [[nodiscard]] std::optional<Revision> NextRevision() const noexcept;
    void Record(CrimeChange change) noexcept;
    [[nodiscard]] CrimeRecord *FindMutableCrime(CrimeRecordId id) noexcept;
    [[nodiscard]] const LawDefinition *FindLawFor(CrimeTypeId type, JurisdictionId jurisdiction) const noexcept;
    [[nodiscard]] const LawResponseDefinition *FindResponseDefinition(LawResponseTypeId id) const noexcept;
    [[nodiscard]] CrimeProofState AggregateProofState(CrimeRecordId crime) const noexcept;
    void RecalculateProofState(CrimeRecord &crime, GameplayContext context = {});
    void RebuildDerivedIndexes();
    Revision revision_{};
    bool definitions_frozen_ = false;
    MonotonicIdGenerator<GameplayObjectId> crime_ids_{0x2500};
    MonotonicIdGenerator<GameplayObjectId> witness_ids_{0x2501};
    MonotonicIdGenerator<GameplayObjectId> evidence_ids_{0x2502};
    MonotonicIdGenerator<GameplayObjectId> bounty_ids_{0x2503};
    MonotonicIdGenerator<GameplayObjectId> authority_ids_{0x2504};
    MonotonicIdGenerator<GameplayObjectId> response_ids_{0x2505};
    std::unordered_map<LawId, LawDefinition, IdHash> laws_;
    std::unordered_map<LawResponseTypeId, LawResponseDefinition, IdHash> response_definitions_;
    std::unordered_map<JurisdictionId, JurisdictionRecord, IdHash> jurisdictions_;
    std::unordered_map<AuthorityId, AuthorityRecord, IdHash> authorities_;
    std::unordered_map<CrimeRecordId, CrimeRecord, IdHash> crimes_;
    std::unordered_map<WitnessRecordId, WitnessRecord, IdHash> witnesses_;
    std::unordered_map<EvidenceId, EvidenceRecord, IdHash> evidence_;
    std::unordered_map<BountyRecordId, BountyRecord, IdHash> bounties_;
    std::unordered_map<LawResponseId, LawResponseRecord, IdHash> responses_;
    std::unordered_map<GameplayObjectRef, std::unordered_map<JurisdictionId, BountyRecordId, IdHash>> active_bounty_index_;
    std::deque<CrimeChange> changes_;
    std::size_t change_journal_capacity_ = 4096;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    mutable CrimeDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::crime

namespace std
{
template <> struct hash<epidemic::gameplay::crime::JurisdictionId>
{
    size_t operator()(const epidemic::gameplay::crime::JurisdictionId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::crime::CrimeRecordId>
{
    size_t operator()(const epidemic::gameplay::crime::CrimeRecordId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::crime::WitnessRecordId>
{
    size_t operator()(const epidemic::gameplay::crime::WitnessRecordId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::crime::EvidenceId>
{
    size_t operator()(const epidemic::gameplay::crime::EvidenceId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::crime::BountyRecordId>
{
    size_t operator()(const epidemic::gameplay::crime::BountyRecordId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::crime::AuthorityId>
{
    size_t operator()(const epidemic::gameplay::crime::AuthorityId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::crime::LawResponseId>
{
    size_t operator()(const epidemic::gameplay::crime::LawResponseId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `CrimeProofState`: `Unknown`, `Suspected`, `Witnessed`, `Reported`, `EvidenceFound`, `Confirmed`, `FalseAccusation`, `Resolved`
- `CrimeCaseState`: `Open`, `Reported`, `UnderInvestigation`, `Wanted`, `Punished`, `Forgiven`, `Expired`, `Dismissed`, `Resolved`
- `BountyState`: `Active`, `Paid`, `Forgiven`, `Expired`, `Transferred`
- `CrimeChangeKind`: `CrimeCandidateCreated`, `CrimeRecorded`, `CrimeReported`, `WitnessAdded`, `WitnessRemoved`, `EvidenceAdded`, `EvidenceRemoved`, `ProofStateChanged`, `CaseStateChanged`, `BountyCreated`, `BountyChanged`, `BountyResolved`, `LawResponseGenerated`, `CrimeExpired`, `CrimeDismissed`, `CrimePruned`
- `CrimeCandidatePolicy`: `RecordAlways`, `RecordIfWitnessed`, `IgnoreWithoutJurisdiction`

### Состав типов и операции

#### `LawId` (строка заголовка 19)

Публичные сигнатуры операций:

```cpp
static constexpr LawId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const LawId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const LawId &) const noexcept = default;
```

#### `CrimeTypeId` (строка заголовка 33)

Публичные сигнатуры операций:

```cpp
static constexpr CrimeTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const CrimeTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const CrimeTypeId &) const noexcept = default;
```

#### `JurisdictionId` (строка заголовка 47)

Публичные сигнатуры операций:

```cpp
static constexpr JurisdictionId FromString(std::string_view s) noexcept;
static constexpr JurisdictionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const JurisdictionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const JurisdictionId &) const noexcept = default;
```

#### `CrimeRecordId` (строка заголовка 65)

Публичные сигнатуры операций:

```cpp
static constexpr CrimeRecordId FromString(std::string_view s) noexcept;
static constexpr CrimeRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const CrimeRecordId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const CrimeRecordId &) const noexcept = default;
```

#### `EvidenceId` (строка заголовка 83)

Публичные сигнатуры операций:

```cpp
static constexpr EvidenceId FromString(std::string_view s) noexcept;
static constexpr EvidenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EvidenceId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EvidenceId &) const noexcept = default;
```

#### `WitnessRecordId` (строка заголовка 101)

Публичные сигнатуры операций:

```cpp
static constexpr WitnessRecordId FromString(std::string_view s) noexcept;
static constexpr WitnessRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WitnessRecordId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WitnessRecordId &) const noexcept = default;
```

#### `BountyRecordId` (строка заголовка 119)

Публичные сигнатуры операций:

```cpp
static constexpr BountyRecordId FromString(std::string_view s) noexcept;
static constexpr BountyRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const BountyRecordId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const BountyRecordId &) const noexcept = default;
```

#### `AuthorityId` (строка заголовка 137)

Публичные сигнатуры операций:

```cpp
static constexpr AuthorityId FromString(std::string_view s) noexcept;
static constexpr AuthorityId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AuthorityId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const AuthorityId &) const noexcept = default;
```

#### `LawResponseId` (строка заголовка 155)

Публичные сигнатуры операций:

```cpp
static constexpr LawResponseId FromString(std::string_view s) noexcept;
static constexpr LawResponseId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const LawResponseId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const LawResponseId &) const noexcept = default;
```

#### `LawResponseTypeId` (строка заголовка 173)

Публичные сигнатуры операций:

```cpp
static constexpr LawResponseTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const LawResponseTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const LawResponseTypeId &) const noexcept = default;
```

#### `EvidenceTypeId` (строка заголовка 187)

Публичные сигнатуры операций:

```cpp
static constexpr EvidenceTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EvidenceTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EvidenceTypeId &) const noexcept = default;
```

#### `WitnessTypeId` (строка заголовка 201)

Публичные сигнатуры операций:

```cpp
static constexpr WitnessTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WitnessTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WitnessTypeId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 215)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 217)

#### `LawResponseDefinition` (строка заголовка 280)

Публичные поля и значения по умолчанию:

- `std::uint32_t required_authority_capabilities = 0`
- `std::optional<CrimeCaseState> resulting_case_state`

#### `LawDefinition` (строка заголовка 287)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `std::int64_t severity_micro = 0`
- `std::int64_t default_bounty = 0`

#### `JurisdictionRecord` (строка заголовка 297)

Публичные поля и значения по умолчанию:

- `std::vector<LawId> active_laws`
- `int priority = 0`

#### `AuthorityRecord` (строка заголовка 306)

Публичные поля и значения по умолчанию:

- `std::uint32_t capabilities = 0`

#### `CrimeCandidateWitness` (строка заголовка 314)

Публичные поля и значения по умолчанию:

- `std::int64_t confidence_micro = 0`
- `std::vector<std::byte> payload`

#### `CrimeCandidate` (строка заголовка 322)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`
- `std::optional<CrimeCandidateWitness> initial_witness`

#### `WitnessRecord` (строка заголовка 334)

Публичные поля и значения по умолчанию:

- `std::int64_t confidence_micro = 0`
- `std::vector<std::byte> payload`

#### `EvidenceRecord` (строка заголовка 345)

Публичные поля и значения по умолчанию:

- `std::int64_t strength_micro = 0`
- `std::vector<std::byte> payload`

#### `CrimeRecord` (строка заголовка 356)

Публичные поля и значения по умолчанию:

- `std::int64_t severity_micro = 0`
- `CrimeProofState proof_state = CrimeProofState::Unknown`
- `CrimeCaseState state = CrimeCaseState::Open`
- `std::optional<GameplayTimePoint> expires_at`
- `std::vector<WitnessRecordId> witnesses`
- `std::vector<EvidenceId> evidence`
- `std::vector<std::byte> payload`

#### `BountyRecord` (строка заголовка 374)

Публичные поля и значения по умолчанию:

- `std::int64_t amount = 0`
- `BountyState state = BountyState::Active`
- `std::optional<GameplayTimePoint> expires_at`
- `std::vector<CrimeRecordId> source_crimes`

#### `CrimeEvaluationResult` (строка заголовка 402)

Публичные поля и значения по умолчанию:

- `std::optional<CrimeRecordId> crime`
- `CrimeProofState proof_state = CrimeProofState::Unknown`
- `std::vector<TypeId> reasons`

#### `LegalityPreview` (строка заголовка 410)

Публичные поля и значения по умолчанию:

- `bool illegal = false`
- `std::int64_t severity_micro = 0`

#### `CrimeChange` (строка заголовка 418)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `CrimeChangeKind kind = CrimeChangeKind::CrimeCandidateCreated`

#### `CrimeSnapshot` (строка заголовка 433)

Публичные поля и значения по умолчанию:

- `std::vector<LawDefinition> laws`
- `std::vector<LawResponseDefinition> response_definitions`
- `std::vector<JurisdictionRecord> jurisdictions`
- `std::vector<AuthorityRecord> authorities`
- `std::vector<CrimeRecord> crimes`
- `std::vector<WitnessRecord> witnesses`
- `std::vector<EvidenceRecord> evidence`
- `std::vector<BountyRecord> bounties`
- `std::vector<LawResponseRecord> responses`
- `bool definitions_frozen = false`
- `std::uint64_t change_epoch = 1`

#### `CrimeChangeBatch` (строка заголовка 455)

Публичные поля и значения по умолчанию:

- `std::vector<CrimeChange> changes`
- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`

#### `CrimeService` (строка заголовка 472)

Внутреннее состояние (не часть публичного API): `bool definitions_frozen_ = false`, `std::unordered_map<LawId, LawDefinition, IdHash> laws_`, `std::unordered_map<LawResponseTypeId, LawResponseDefinition, IdHash> response_definitions_`, `std::unordered_map<JurisdictionId, JurisdictionRecord, IdHash> jurisdictions_`, `std::unordered_map<AuthorityId, AuthorityRecord, IdHash> authorities_`, `std::unordered_map<CrimeRecordId, CrimeRecord, IdHash> crimes_`, `std::unordered_map<WitnessRecordId, WitnessRecord, IdHash> witnesses_`, `std::unordered_map<EvidenceId, EvidenceRecord, IdHash> evidence_`, `std::unordered_map<BountyRecordId, BountyRecord, IdHash> bounties_`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterLaw(LawDefinition law);
[[nodiscard]] foundation::Result<void> RegisterLawResponseDefinition(LawResponseDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterJurisdiction(JurisdictionRecord jurisdiction);
[[nodiscard]] foundation::Result<AuthorityId> RegisterAuthority(AuthorityRecord authority);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] std::optional<JurisdictionRecord> GetApplicableJurisdiction(GameplayObjectRef area) const;
[[nodiscard]] foundation::Result<CrimeEvaluationResult> EvaluateCrimeCandidate( CrimeCandidate candidate, CrimeCandidatePolicy policy = CrimeCandidatePolicy::RecordAlways);
[[nodiscard]] foundation::Result<BountyRecordId> CreateBounty(BountyRecord bounty);
[[nodiscard]] foundation::Result<LawResponseId> GenerateLawResponse(LawResponseRequest request);
[[nodiscard]] LegalityPreview PreviewLegality(GameplayObjectRef offender, CrimeTypeId type, GameplayObjectRef area) const;
[[nodiscard]] std::vector<CrimeRecord> FindCrimesByOffender(GameplayObjectRef offender) const;
[[nodiscard]] std::vector<CrimeRecord> FindCrimesByVictim(GameplayObjectRef victim) const;
[[nodiscard]] std::vector<CrimeRecord> FindCrimesByJurisdiction(JurisdictionId jurisdiction) const;
[[nodiscard]] std::vector<CrimeRecord> FindOpenCases() const;
[[nodiscard]] std::vector<WitnessRecord> FindWitnesses(CrimeRecordId crime) const;
[[nodiscard]] std::vector<EvidenceRecord> FindEvidence(CrimeRecordId crime) const;
[[nodiscard]] std::optional<BountyRecord> GetBounty(GameplayObjectRef offender, JurisdictionId jurisdiction) const;
[[nodiscard]] CrimeChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesThrough(std::uint64_t sequence);
void SetChangeJournalCapacity(std::size_t capacity) noexcept;
[[nodiscard]] CrimeSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(CrimeSnapshot snapshot);
[[nodiscard]] CrimeDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] const CrimeRecord *FindCrime(CrimeRecordId id) const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `hash` (строка заголовка 584)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::JurisdictionId &v) const noexcept;
```

#### `hash` (строка заголовка 591)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::CrimeRecordId &v) const noexcept;
```

#### `hash` (строка заголовка 598)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::WitnessRecordId &v) const noexcept;
```

#### `hash` (строка заголовка 605)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::EvidenceId &v) const noexcept;
```

#### `hash` (строка заголовка 612)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::BountyRecordId &v) const noexcept;
```

#### `hash` (строка заголовка 619)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::AuthorityId &v) const noexcept;
```

#### `hash` (строка заголовка 626)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::crime::LawResponseId &v) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterLaw` | `CrimeService::RegisterLaw` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:147` |
| `RegisterLawResponseDefinition` | `CrimeService::RegisterLawResponseDefinition` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:171` |
| `RegisterJurisdiction` | `CrimeService::RegisterJurisdiction` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:196` |
| `RegisterAuthority` | `CrimeService::RegisterAuthority` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:227` |
| `FreezeDefinitions` | `CrimeService::FreezeDefinitions` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:269` |
| `GetApplicableJurisdiction` | `CrimeService::GetApplicableJurisdiction` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:294` |
| `EvaluateCrimeCandidate` | `CrimeService::EvaluateCrimeCandidate` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:334` |
| `FindCrime` | `CrimeService::FindCrime` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:457` |
| `CreateBounty` | `CrimeService::CreateBounty` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:645` |
| `GenerateLawResponse` | `CrimeService::GenerateLawResponse` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:751` |
| `PreviewLegality` | `CrimeService::PreviewLegality` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:929` |
| `FindCrimesByOffender` | `CrimeService::FindCrimesByOffender` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:948` |
| `FindCrimesByVictim` | `CrimeService::FindCrimesByVictim` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:961` |
| `FindCrimesByJurisdiction` | `CrimeService::FindCrimesByJurisdiction` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:974` |
| `FindOpenCases` | `CrimeService::FindOpenCases` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:987` |
| `FindWitnesses` | `CrimeService::FindWitnesses` | `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:1001` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterLaw`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:48`
- `RegisterLawResponseDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:54`
- `RegisterJurisdiction`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:62`
- `RegisterAuthority`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:68`
- `FreezeDefinitions`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:88`
- `PreviewLegality`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:112`
- `EvaluateCrimeCandidate`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:115`
- `FindCrimesByOffender`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:119`
- `FindCrime`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:125`
- `FindWitnesses`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:149`
- `FindEvidence`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:154`
- `GenerateLawResponse`: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp:187`

