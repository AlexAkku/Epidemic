---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h"
---

# Society: society.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h`.

**Модуль:** [[06 EngineFramework/Модуль Society|Society]].
**Размер исходного файла:** 18728 байт.
**Подключение:** `#include <Epidemic/GameFramework/Society/society.h>`.

## Объявления типов (лексический индекс)
`MembershipId`, `RelationshipId`, `SocialRoleId`, `SocialGroupTypeId`, `RelationshipTypeId`, `ReputationTrackId`, `IdHash`, `RefHash`, `PairHash`, `MembershipState`, `RelationshipState`, `SocietyChangeKind`, `RelationshipStateThreshold`, `RelationshipTypeDefinition`, `ReputationStandingThreshold`, `ReputationTrackDefinition`, `SocialGroupDefinition`, `MembershipRecord`, `RelationshipRecord`, `ReputationRecord`, `SocialStandingSnapshot`, `SocialChangeRequest`, `ReputationChangeRequest`, `AttitudeQuery`, `SocietyChange`, `SocietyChangeBatch`, `SocietySnapshot`, `SocietyDiagnostics`, `SocietyService`, `MembershipKey`, `MembershipKeyHash`, `RelationshipKey`, `RelationshipKeyHash`, `ReputationKey`, `ReputationKeyHash`, `hash`

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
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::society
{
struct MembershipId
{
    GameplayObjectId value{};
    static constexpr MembershipId FromString(std::string_view s) noexcept { return {GameplayObjectId::FromString(s)}; }
    static constexpr MembershipId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const MembershipId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const MembershipId &) const noexcept = default;
};

struct RelationshipId
{
    GameplayObjectId value{};
    static constexpr RelationshipId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr RelationshipId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const RelationshipId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const RelationshipId &) const noexcept = default;
};

struct SocialRoleId
{
    TypeId value{};
    static constexpr SocialRoleId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const SocialRoleId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const SocialRoleId &) const noexcept = default;
};

struct SocialGroupTypeId
{
    TypeId value{};
    static constexpr SocialGroupTypeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const SocialGroupTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const SocialGroupTypeId &) const noexcept = default;
};

struct RelationshipTypeId
{
    TypeId value{};
    static constexpr RelationshipTypeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const RelationshipTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const RelationshipTypeId &) const noexcept = default;
};

struct ReputationTrackId
{
    TypeId value{};
    static constexpr ReputationTrackId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const ReputationTrackId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ReputationTrackId &) const noexcept = default;
};

struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

struct RefHash
{
    [[nodiscard]] std::size_t operator()(GameplayObjectRef r) const noexcept
    {
        return std::hash<GameplayObjectRef>{}(r);
    }
};

struct PairHash
{
    [[nodiscard]] std::size_t operator()(const std::pair<GameplayObjectRef, GameplayObjectRef> &p) const noexcept
    {
        return std::hash<GameplayObjectRef>{}(p.first) ^
               (std::hash<GameplayObjectRef>{}(p.second) + 0x9E3779B97F4A7C15ull);
    }
};

enum class MembershipState
{
    Active,
    Suspended,
    Former,
    Banned,
    Applicant,
    Honorary
};

enum class RelationshipState
{
    Neutral,
    Friendly,
    Hostile,
    Allied,
    Feared,
    Trusted,
    Unknown
};

enum class SocietyChangeKind
{
    GroupCreated,
    MembershipAdded,
    MembershipRemoved,
    MembershipChanged,
    RelationshipChanged,
    ReputationChanged,
    SocialRoleAssigned,
    SocialRoleRemoved,
    FactionRelationChanged
};

struct RelationshipStateThreshold
{
    std::int64_t minimum_value_micro = 0;
    RelationshipState state = RelationshipState::Neutral;
};

struct RelationshipTypeDefinition
{
    RelationshipTypeId id{};
    std::int64_t minimum_value_micro = -1'000'000;
    std::int64_t maximum_value_micro = 1'000'000;
    std::int64_t default_value_micro = 0;
    // Sorted ascending at registration. The state of a value is the last threshold whose
    // minimum_value_micro is <= value. The first threshold must cover minimum_value_micro.
    std::vector<RelationshipStateThreshold> state_thresholds;
    // Normalized coefficients in [-1'000'000, 1'000'000].
    std::int64_t direct_attitude_weight_micro = 1'000'000;
    std::int64_t group_attitude_weight_micro = 0;
    Revision revision{};
};

struct ReputationStandingThreshold
{
    std::int64_t minimum_value_micro = 0;
    TagId standing_tag{};
};

struct ReputationTrackDefinition
{
    ReputationTrackId id{};
    std::int64_t minimum_value_micro = -1'000'000;
    std::int64_t maximum_value_micro = 1'000'000;
    std::int64_t default_value_micro = 0;
    // Contribution of this track to GetEffectiveAttitude when the target has this reputation
    // in the observer's own scope or in a group the observer actively belongs to.
    std::int64_t attitude_weight_micro = 0;
    // Sorted ascending. At most one, the highest applicable threshold, contributes a standing tag.
    std::vector<ReputationStandingThreshold> standing_thresholds;
    Revision revision{};
};

struct SocialGroupDefinition
{
    GameplayObjectRef group{};
    GameplayTagSet tags;
    SocialGroupTypeId type{};
    std::vector<std::byte> payload;
    Revision revision{};
};

struct MembershipRecord
{
    MembershipId id{};
    GameplayObjectRef member{};
    GameplayObjectRef group{};
    SocialRoleId role{};
    std::int64_t rank = 0;
    MembershipState state = MembershipState::Active;
    GameplayTimePoint joined_at{};
    Revision revision{};
};

struct RelationshipRecord
{
    RelationshipId id{};
    GameplayObjectRef subject{};
    GameplayObjectRef target{};
    RelationshipTypeId type{};
    std::int64_t value_micro = 0;
    // Derived from RelationshipTypeDefinition::state_thresholds on every mutation/restore.
    RelationshipState state = RelationshipState::Neutral;
    GameplayTimePoint updated_at{};
    Revision revision{};
};

struct ReputationRecord
{
    GameplayObjectRef subject{};
    GameplayObjectRef scope{};
    ReputationTrackId track{};
    std::int64_t value_micro = 0;
    Revision revision{};
};

struct SocialStandingSnapshot
{
    GameplayObjectRef subject{};
    GameplayObjectRef scope{};
    GameplayTagSet standing_tags;
    std::vector<ReputationRecord> reputations;
    Revision revision{};
};

struct SocialChangeRequest
{
    GameplayObjectRef subject{};
    GameplayObjectRef target{};
    RelationshipTypeId type{};
    std::int64_t delta_micro = 0;
    TypeId reason{};
    GameplayContext context{};
};

struct ReputationChangeRequest
{
    GameplayObjectRef subject{};
    GameplayObjectRef scope{};
    ReputationTrackId track{};
    std::int64_t delta_micro = 0;
    TypeId reason{};
    GameplayContext context{};
};

struct AttitudeQuery
{
    GameplayObjectRef observer{};
    GameplayObjectRef target{};
    GameplayContext context{};
};

struct SocietyChange
{
    std::uint64_t sequence = 0;
    SocietyChangeKind kind = SocietyChangeKind::GroupCreated;
    GameplayObjectRef subject{};
    GameplayObjectRef target{};
    RelationshipTypeId relationship_type{};
    ReputationTrackId reputation_track{};
    MembershipId membership{};
    GameplayContext context{};
    Revision revision{};
};

struct SocietyChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    std::vector<SocietyChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct SocietySnapshot
{
    std::vector<SocialGroupDefinition> groups;
    std::vector<RelationshipTypeDefinition> relationship_types;
    std::vector<ReputationTrackDefinition> reputation_tracks;
    bool definitions_frozen = false;
    std::vector<MembershipRecord> memberships;
    std::vector<RelationshipRecord> relationships;
    std::vector<ReputationRecord> reputations;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot membership_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot relationship_ids{};
    Revision revision{};
    std::vector<SocietyChange> journal;
    std::uint64_t next_change_sequence = 1;

    std::uint64_t change_epoch = 1;
};

struct SocietyDiagnostics
{
    std::uint64_t groups = 0;
    std::uint64_t memberships = 0;
    std::uint64_t relationships = 0;
    std::uint64_t reputation_records = 0;
    std::uint64_t relationship_changes = 0;
    std::uint64_t reputation_changes = 0;
    std::uint64_t attitude_queries = 0;
    std::uint64_t social_transactions = 0;
    std::uint64_t journal_gaps = 0;
};

class SocietyService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<SocietyService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.society");
    }

    [[nodiscard]] foundation::Result<void> RegisterGroup(SocialGroupDefinition group);
    [[nodiscard]] foundation::Result<void> RegisterRelationshipType(RelationshipTypeDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterReputationTrack(ReputationTrackDefinition definition);
    [[nodiscard]] foundation::Result<void> FreezeDefinitions();
    [[nodiscard]] bool DefinitionsFrozen() const noexcept { return definitions_frozen_; }

    [[nodiscard]] foundation::Result<MembershipId> AddMembership(MembershipRecord record);
    [[nodiscard]] foundation::Result<void> SetMembershipState(MembershipId id, MembershipState state,
                                                              GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SetMembershipRole(MembershipId id, SocialRoleId role,
                                                             GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveMembership(MembershipId id, GameplayContext context = {});

    [[nodiscard]] foundation::Result<RelationshipId> SetRelationship(RelationshipRecord record,
                                                                     GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ApplySocialChange(SocialChangeRequest request);
    [[nodiscard]] foundation::Result<void> SetReputation(ReputationRecord record, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ApplyReputationChange(ReputationChangeRequest request);

    [[nodiscard]] const SocialGroupDefinition *GetGroup(GameplayObjectRef group) const noexcept;
    [[nodiscard]] const RelationshipTypeDefinition *GetRelationshipType(RelationshipTypeId id) const noexcept;
    [[nodiscard]] const ReputationTrackDefinition *GetReputationTrack(ReputationTrackId id) const noexcept;
    [[nodiscard]] const MembershipRecord *GetMembership(MembershipId id) const noexcept;
    [[nodiscard]] std::vector<MembershipRecord> FindGroupsOf(GameplayObjectRef member) const;
    [[nodiscard]] std::vector<MembershipRecord> FindMembersOf(GameplayObjectRef group) const;
    [[nodiscard]] std::optional<RelationshipRecord> GetRelationship(GameplayObjectRef subject, GameplayObjectRef target,
                                                                    RelationshipTypeId type) const;
    [[nodiscard]] std::int64_t GetEffectiveAttitude(AttitudeQuery query) const;
    [[nodiscard]] std::optional<ReputationRecord> GetReputation(GameplayObjectRef subject, GameplayObjectRef scope,
                                                                ReputationTrackId track) const;
    [[nodiscard]] SocialStandingSnapshot GetSocialStanding(GameplayObjectRef subject, GameplayObjectRef scope) const;
    [[nodiscard]] bool HasRole(GameplayObjectRef subject, SocialRoleId role, GameplayObjectRef scope = {}) const;

    private:
        [[nodiscard]] std::vector<SocietyChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] SocietyChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] SocietyChangeBatch ReadChangesSince(ChangeCursor cursor) const
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

    [[nodiscard]] SocietySnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(SocietySnapshot snapshot);
    [[nodiscard]] SocietyDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }

  private:
    struct MembershipKey
    {
        GameplayObjectRef member{};
        GameplayObjectRef group{};
        [[nodiscard]] bool operator==(const MembershipKey &) const noexcept = default;
    };
    struct MembershipKeyHash
    {
        [[nodiscard]] std::size_t operator()(const MembershipKey &key) const noexcept;
    };
    struct RelationshipKey
    {
        GameplayObjectRef subject{};
        GameplayObjectRef target{};
        RelationshipTypeId type{};
        [[nodiscard]] bool operator==(const RelationshipKey &) const noexcept = default;
    };
    struct RelationshipKeyHash
    {
        [[nodiscard]] std::size_t operator()(const RelationshipKey &key) const noexcept;
    };
    struct ReputationKey
    {
        GameplayObjectRef subject{};
        GameplayObjectRef scope{};
        ReputationTrackId track{};
        [[nodiscard]] bool operator==(const ReputationKey &) const noexcept = default;
    };
    struct ReputationKeyHash
    {
        [[nodiscard]] std::size_t operator()(const ReputationKey &key) const noexcept;
    };

    static constexpr std::size_t kChangeJournalCapacity = 4096;

    bool Bump() noexcept
    {
        if (revision_.value == std::numeric_limits<std::uint64_t>::max())
            return false;
        ++revision_.value;
        return true;
    }
    void Record(SocietyChange change);
    [[nodiscard]] RelationshipRecord *FindMutableRelationship(GameplayObjectRef subject, GameplayObjectRef target,
                                                              RelationshipTypeId type) noexcept;
    [[nodiscard]] RelationshipState ResolveRelationshipState(RelationshipTypeId type, std::int64_t value_micro) const;
    void IndexMembership(const MembershipRecord &record);
    void UnindexMembership(const MembershipRecord &record);
    void IndexRelationship(const RelationshipRecord &record);
    void IndexReputation(const ReputationRecord &record);

    Revision revision_{};
    MonotonicIdGenerator<GameplayObjectId> membership_ids_{0x2400};
    MonotonicIdGenerator<GameplayObjectId> relationship_ids_{0x2401};

    std::unordered_map<GameplayObjectRef, SocialGroupDefinition, RefHash> groups_;
    std::unordered_map<RelationshipTypeId, RelationshipTypeDefinition, IdHash> relationship_types_;
    std::unordered_map<ReputationTrackId, ReputationTrackDefinition, IdHash> reputation_tracks_;
    bool definitions_frozen_ = false;

    std::unordered_map<MembershipId, MembershipRecord, IdHash> memberships_;
    std::unordered_map<MembershipKey, MembershipId, MembershipKeyHash> membership_by_key_;
    std::unordered_map<GameplayObjectRef, std::vector<MembershipId>, RefHash> memberships_by_member_;
    std::unordered_map<GameplayObjectRef, std::vector<MembershipId>, RefHash> memberships_by_group_;

    std::unordered_map<RelationshipId, RelationshipRecord, IdHash> relationships_;
    std::unordered_map<RelationshipKey, RelationshipId, RelationshipKeyHash> relationship_by_key_;
    std::unordered_map<GameplayObjectRef, std::vector<RelationshipId>, RefHash> relationships_by_subject_;

    std::unordered_map<ReputationKey, ReputationRecord, ReputationKeyHash> reputations_;
    std::unordered_map<GameplayObjectRef, std::vector<ReputationKey>, RefHash> reputations_by_subject_;

    std::vector<SocietyChange> changes_ = [] {
        std::vector<SocietyChange> journal;
        journal.reserve(4096);
        return journal;
    }();
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    mutable SocietyDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::society

namespace std
{
template <> struct hash<epidemic::gameplay::society::MembershipId>
{
    size_t operator()(const epidemic::gameplay::society::MembershipId &v) const noexcept
    {
        return hash<epidemic::gameplay::GameplayObjectId>{}(v.value);
    }
};
template <> struct hash<epidemic::gameplay::society::RelationshipId>
{
    size_t operator()(const epidemic::gameplay::society::RelationshipId &v) const noexcept
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

- `MembershipState`: `Active`, `Suspended`, `Former`, `Banned`, `Applicant`, `Honorary`
- `RelationshipState`: `Neutral`, `Friendly`, `Hostile`, `Allied`, `Feared`, `Trusted`, `Unknown`
- `SocietyChangeKind`: `GroupCreated`, `MembershipAdded`, `MembershipRemoved`, `MembershipChanged`, `RelationshipChanged`, `ReputationChanged`, `SocialRoleAssigned`, `SocialRoleRemoved`, `FactionRelationChanged`

### Состав типов и операции

#### `MembershipId` (строка заголовка 17)

Публичные сигнатуры операций:

```cpp
static constexpr MembershipId FromString(std::string_view s) noexcept;
static constexpr MembershipId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const MembershipId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const MembershipId &) const noexcept = default;
```

#### `RelationshipId` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
static constexpr RelationshipId FromString(std::string_view s) noexcept;
static constexpr RelationshipId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const RelationshipId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const RelationshipId &) const noexcept = default;
```

#### `SocialRoleId` (строка заголовка 46)

Публичные сигнатуры операций:

```cpp
static constexpr SocialRoleId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const SocialRoleId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const SocialRoleId &) const noexcept = default;
```

#### `SocialGroupTypeId` (строка заголовка 55)

Публичные сигнатуры операций:

```cpp
static constexpr SocialGroupTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const SocialGroupTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const SocialGroupTypeId &) const noexcept = default;
```

#### `RelationshipTypeId` (строка заголовка 64)

Публичные сигнатуры операций:

```cpp
static constexpr RelationshipTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const RelationshipTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const RelationshipTypeId &) const noexcept = default;
```

#### `ReputationTrackId` (строка заголовка 73)

Публичные сигнатуры операций:

```cpp
static constexpr ReputationTrackId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ReputationTrackId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ReputationTrackId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 82)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 84)

#### `RefHash` (строка заголовка 90)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef r) const noexcept;
```

#### `PairHash` (строка заголовка 98)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const std::pair<GameplayObjectRef, GameplayObjectRef> &p) const noexcept;
```

#### `RelationshipStateThreshold` (строка заголовка 141)

Публичные поля и значения по умолчанию:

- `std::int64_t minimum_value_micro = 0`
- `RelationshipState state = RelationshipState::Neutral`

#### `RelationshipTypeDefinition` (строка заголовка 147)

Публичные поля и значения по умолчанию:

- `std::int64_t minimum_value_micro = -1'000'000`
- `std::int64_t maximum_value_micro = 1'000'000`
- `std::int64_t default_value_micro = 0`
- `std::int64_t group_attitude_weight_micro = 0`

#### `ReputationStandingThreshold` (строка заголовка 162)

Публичные поля и значения по умолчанию:

- `std::int64_t minimum_value_micro = 0`

#### `ReputationTrackDefinition` (строка заголовка 168)

Публичные поля и значения по умолчанию:

- `std::int64_t minimum_value_micro = -1'000'000`
- `std::int64_t maximum_value_micro = 1'000'000`
- `std::int64_t default_value_micro = 0`

#### `SocialGroupDefinition` (строка заголовка 182)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `std::vector<std::byte> payload`

#### `MembershipRecord` (строка заголовка 191)

Публичные поля и значения по умолчанию:

- `std::int64_t rank = 0`
- `MembershipState state = MembershipState::Active`

#### `RelationshipRecord` (строка заголовка 203)

Публичные поля и значения по умолчанию:

- `std::int64_t value_micro = 0`

#### `ReputationRecord` (строка заголовка 216)

Публичные поля и значения по умолчанию:

- `std::int64_t value_micro = 0`

#### `SocialStandingSnapshot` (строка заголовка 225)

Публичные поля и значения по умолчанию:

- `GameplayTagSet standing_tags`
- `std::vector<ReputationRecord> reputations`

#### `SocialChangeRequest` (строка заголовка 234)

Публичные поля и значения по умолчанию:

- `std::int64_t delta_micro = 0`

#### `ReputationChangeRequest` (строка заголовка 244)

Публичные поля и значения по умолчанию:

- `std::int64_t delta_micro = 0`

#### `SocietyChange` (строка заголовка 261)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `SocietyChangeKind kind = SocietyChangeKind::GroupCreated`

#### `SocietyChangeBatch` (строка заголовка 274)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `std::vector<SocietyChange> changes`

#### `SocietySnapshot` (строка заголовка 285)

Публичные поля и значения по умолчанию:

- `std::vector<SocialGroupDefinition> groups`
- `std::vector<RelationshipTypeDefinition> relationship_types`
- `std::vector<ReputationTrackDefinition> reputation_tracks`
- `bool definitions_frozen = false`
- `std::vector<MembershipRecord> memberships`
- `std::vector<RelationshipRecord> relationships`
- `std::vector<ReputationRecord> reputations`
- `std::vector<SocietyChange> journal`
- `std::uint64_t next_change_sequence = 1`
- `std::uint64_t change_epoch = 1`

#### `SocietyDiagnostics` (строка заголовка 303)

Публичные поля и значения по умолчанию:

- `std::uint64_t groups = 0`
- `std::uint64_t memberships = 0`
- `std::uint64_t relationships = 0`
- `std::uint64_t reputation_records = 0`
- `std::uint64_t relationship_changes = 0`
- `std::uint64_t reputation_changes = 0`
- `std::uint64_t attitude_queries = 0`
- `std::uint64_t social_transactions = 0`
- `std::uint64_t journal_gaps = 0`

#### `SocietyService` (строка заголовка 316)

Внутреннее состояние (не часть публичного API): `static constexpr std::size_t kChangeJournalCapacity = 4096`, `std::unordered_map<GameplayObjectRef, SocialGroupDefinition, RefHash> groups_`, `std::unordered_map<RelationshipTypeId, RelationshipTypeDefinition, IdHash> relationship_types_`, `std::unordered_map<ReputationTrackId, ReputationTrackDefinition, IdHash> reputation_tracks_`, `bool definitions_frozen_ = false`, `std::unordered_map<MembershipId, MembershipRecord, IdHash> memberships_`, `std::unordered_map<MembershipKey, MembershipId, MembershipKeyHash> membership_by_key_`, `std::unordered_map<GameplayObjectRef, std::vector<MembershipId>, RefHash> memberships_by_member`, `std::unordered_map<GameplayObjectRef, std::vector<MembershipId>, RefHash> memberships_by_group_`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterGroup(SocialGroupDefinition group);
[[nodiscard]] foundation::Result<void> RegisterRelationshipType(RelationshipTypeDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterReputationTrack(ReputationTrackDefinition definition);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] foundation::Result<MembershipId> AddMembership(MembershipRecord record);
[[nodiscard]] foundation::Result<void> ApplySocialChange(SocialChangeRequest request);
[[nodiscard]] foundation::Result<void> ApplyReputationChange(ReputationChangeRequest request);
[[nodiscard]] const SocialGroupDefinition *GetGroup(GameplayObjectRef group) const noexcept;
[[nodiscard]] const RelationshipTypeDefinition *GetRelationshipType(RelationshipTypeId id) const noexcept;
[[nodiscard]] const ReputationTrackDefinition *GetReputationTrack(ReputationTrackId id) const noexcept;
[[nodiscard]] const MembershipRecord *GetMembership(MembershipId id) const noexcept;
[[nodiscard]] std::vector<MembershipRecord> FindGroupsOf(GameplayObjectRef member) const;
[[nodiscard]] std::vector<MembershipRecord> FindMembersOf(GameplayObjectRef group) const;
[[nodiscard]] std::optional<RelationshipRecord> GetRelationship(GameplayObjectRef subject, GameplayObjectRef target, RelationshipTypeId type) const;
[[nodiscard]] std::int64_t GetEffectiveAttitude(AttitudeQuery query) const;
[[nodiscard]] std::optional<ReputationRecord> GetReputation(GameplayObjectRef subject, GameplayObjectRef scope, ReputationTrackId track) const;
[[nodiscard]] SocialStandingSnapshot GetSocialStanding(GameplayObjectRef subject, GameplayObjectRef scope) const;
[[nodiscard]] SocietyChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesThrough(std::uint64_t sequence);
[[nodiscard]] SocietySnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(SocietySnapshot snapshot);
[[nodiscard]] SocietyDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `MembershipKey` (строка заголовка 389)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const MembershipKey &) const noexcept = default;
```

#### `MembershipKeyHash` (строка заголовка 395)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const MembershipKey &key) const noexcept;
```

#### `RelationshipKey` (строка заголовка 399)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const RelationshipKey &) const noexcept = default;
```

#### `RelationshipKeyHash` (строка заголовка 406)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const RelationshipKey &key) const noexcept;
```

#### `ReputationKey` (строка заголовка 410)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const ReputationKey &) const noexcept = default;
```

#### `ReputationKeyHash` (строка заголовка 417)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const ReputationKey &key) const noexcept;
```

#### `hash` (строка заголовка 474)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::society::MembershipId &v) const noexcept;
```

#### `hash` (строка заголовка 481)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::society::RelationshipId &v) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterGroup` | `SocietyService::RegisterGroup` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:183` |
| `RegisterRelationshipType` | `SocietyService::RegisterRelationshipType` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:208` |
| `RegisterReputationTrack` | `SocietyService::RegisterReputationTrack` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:258` |
| `FreezeDefinitions` | `SocietyService::FreezeDefinitions` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:303` |
| `AddMembership` | `SocietyService::AddMembership` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:309` |
| `ApplySocialChange` | `SocietyService::ApplySocialChange` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:546` |
| `ApplyReputationChange` | `SocietyService::ApplyReputationChange` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:649` |
| `GetGroup` | `SocietyService::GetGroup` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:670` |
| `GetRelationshipType` | `SocietyService::GetRelationshipType` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:676` |
| `GetReputationTrack` | `SocietyService::GetReputationTrack` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:682` |
| `GetMembership` | `SocietyService::GetMembership` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:688` |
| `FindGroupsOf` | `SocietyService::FindGroupsOf` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:694` |
| `FindMembersOf` | `SocietyService::FindMembersOf` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:712` |
| `GetRelationship` | `SocietyService::GetRelationship` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:730` |
| `GetEffectiveAttitude` | `SocietyService::GetEffectiveAttitude` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:742` |
| `GetReputation` | `SocietyService::GetReputation` | `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:794` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterGroup`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:115`
- `RegisterRelationshipType`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:117`
- `RegisterReputationTrack`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:119`
- `AddMembership`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:125`
- `FreezeDefinitions`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:128`
- `ApplySocialChange`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:172`
- `GetRelationship`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:174`
- `ApplyReputationChange`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:209`
- `GetReputation`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:211`
- `GetSocialStanding`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:215`
- `GetEffectiveAttitude`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:219`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp:224`

