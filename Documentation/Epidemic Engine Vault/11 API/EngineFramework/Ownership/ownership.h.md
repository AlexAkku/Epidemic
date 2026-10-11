---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h"
---

# Ownership: ownership.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`.

**Модуль:** [[06 EngineFramework/Модуль Ownership|Ownership]].
**Размер исходного файла:** 17381 байт.
**Подключение:** `#include <Epidemic/GameFramework/Ownership/ownership.h>`.

## Объявления типов (лексический индекс)
`OwnershipRecordId`, `PropertyClaimId`, `AccessRuleId`, `PermissionGrantId`, `PropertyRightId`, `PropertyDomainId`, `IdHash`, `RefHash`, `OwnershipStrength`, `PermissionStrength`, `AccessDecision`, `PermissionDecision`, `TransferReason`, `OwnershipChangeKind`, `OwnershipRecord`, `PermissionGrant`, `AccessRule`, `PropertyClaim`, `OwnershipPermissionQuery`, `OwnershipPermissionResult`, `TransferOwnershipRequest`, `OwnershipChange`, `OwnershipChangeBatch`, `OwnershipSnapshot`, `OwnershipDiagnostics`, `IOwnershipRoleProvider`, `OwnershipService`, `OwnerKey`, `OwnerKeyHash`, `hash`

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
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::ownership
{
struct OwnershipRecordId
{
    GameplayObjectId value{};
    static constexpr OwnershipRecordId FromString(std::string_view s) noexcept { return {GameplayObjectId::FromString(s)}; }
    static constexpr OwnershipRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept { return {GameplayObjectId::FromRaw(h, l)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const OwnershipRecordId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const OwnershipRecordId &) const noexcept = default;
};
struct PropertyClaimId
{
    GameplayObjectId value{};
    static constexpr PropertyClaimId FromString(std::string_view s) noexcept { return {GameplayObjectId::FromString(s)}; }
    static constexpr PropertyClaimId FromRaw(std::uint64_t h, std::uint64_t l) noexcept { return {GameplayObjectId::FromRaw(h, l)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PropertyClaimId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PropertyClaimId &) const noexcept = default;
};
struct AccessRuleId
{
    GameplayObjectId value{};
    static constexpr AccessRuleId FromString(std::string_view s) noexcept { return {GameplayObjectId::FromString(s)}; }
    static constexpr AccessRuleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept { return {GameplayObjectId::FromRaw(h, l)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const AccessRuleId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const AccessRuleId &) const noexcept = default;
};
struct PermissionGrantId
{
    GameplayObjectId value{};
    static constexpr PermissionGrantId FromString(std::string_view s) noexcept { return {GameplayObjectId::FromString(s)}; }
    static constexpr PermissionGrantId FromRaw(std::uint64_t h, std::uint64_t l) noexcept { return {GameplayObjectId::FromRaw(h, l)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PermissionGrantId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PermissionGrantId &) const noexcept = default;
};
struct PropertyRightId
{
    TypeId value{};
    static constexpr PropertyRightId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PropertyRightId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PropertyRightId &) const noexcept = default;
};
struct PropertyDomainId
{
    TypeId value{};
    static constexpr PropertyDomainId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PropertyDomainId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PropertyDomainId &) const noexcept = default;
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
    [[nodiscard]] std::size_t operator()(GameplayObjectRef r) const noexcept { return std::hash<GameplayObjectRef>{}(r); }
};

enum class OwnershipStrength
{
    Owned,
    Claimed,
    Borrowed,
    Leased,
    Reserved,
    Stolen,
    Contested,
    Abandoned,
    Public,
    Unowned
};
enum class PermissionStrength
{
    Weak,
    Normal,
    Strong,
    Absolute
};
enum class AccessDecision
{
    Allow,
    Deny,
    RequirePermission,
    RequireRole,
    RequireOwner,
    Public,
    Private,
    TrespassIfViolated,
    CrimeIfViolated
};
enum class PermissionDecision
{
    Allowed,
    Denied,
    Trespass,
    CrimeCandidate,
    Unknown,
    NotApplicable
};
enum class TransferReason
{
    Trade,
    Gift,
    Reward,
    Loot,
    Theft,
    Confiscation,
    Inheritance,
    Abandonment,
    ConstructionCreated,
    GameSpecific
};
enum class OwnershipChangeKind
{
    OwnershipAssigned,
    OwnershipTransferred,
    OwnershipRemoved,
    PermissionGranted,
    PermissionRevoked,
    AccessRuleAdded,
    AccessRuleChanged,
    AccessRuleRemoved,
    PropertyClaimCreated,
    PropertyClaimResolved
};

struct OwnershipRecord
{
    OwnershipRecordId id{};
    GameplayObjectRef property{};
    GameplayObjectRef owner{};
    PropertyDomainId domain{};
    OwnershipStrength strength = OwnershipStrength::Owned;
    GameplayTimePoint acquired_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct PermissionGrant
{
    PermissionGrantId id{};
    GameplayObjectRef subject{};
    GameplayObjectRef property{};
    PropertyRightId right{};
    TypeId source{};
    GameplayTimePoint granted_at{};
    std::optional<GameplayTimePoint> expires_at;
    PermissionStrength strength = PermissionStrength::Normal;
    Revision revision{};
};
struct AccessRule
{
    AccessRuleId id{};
    GameplayObjectRef property{};
    PropertyRightId right{};
    AccessDecision decision = AccessDecision::RequirePermission;
    int priority = 0;
    GameplayObjectRef required_subject{};
    TypeId required_role{};
    Revision revision{};
    PermissionStrength minimum_permission_strength = PermissionStrength::Weak;
    PropertyDomainId domain{};
};
struct PropertyClaim
{
    PropertyClaimId id{};
    GameplayObjectRef claimant{};
    GameplayObjectRef property{};
    std::int64_t strength_micro = 0;
    GameplayTimePoint created_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct OwnershipPermissionQuery
{
    GameplayObjectRef actor{};
    GameplayObjectRef property{};
    PropertyRightId right{};
    GameplayContext context{};
    PropertyDomainId domain{};
};
struct OwnershipPermissionResult
{
    PermissionDecision decision = PermissionDecision::Unknown;
    GameplayObjectRef effective_owner{};
    std::vector<TypeId> reasons;
    Revision revision{};
    AccessRuleId matched_rule{};
    PermissionGrantId matched_grant{};
};
struct TransferOwnershipRequest
{
    GameplayObjectRef property{};
    GameplayObjectRef from_owner{};
    GameplayObjectRef to_owner{};
    TransferReason reason = TransferReason::Trade;
    GameplayContext context{};
    PropertyDomainId domain{};
    Revision expected_revision{};
};
struct OwnershipChange
{
    std::uint64_t sequence = 0;
    OwnershipChangeKind kind = OwnershipChangeKind::OwnershipAssigned;
    GameplayObjectRef property{};
    GameplayObjectRef actor{};
    GameplayObjectRef owner{};
    PropertyRightId right{};
    GameplayContext context{};
    Revision revision{};
    PropertyDomainId domain{};
    GameplayObjectRef previous_owner{};
    TransferReason transfer_reason = TransferReason::GameSpecific;
};
struct OwnershipChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    std::vector<OwnershipChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct OwnershipSnapshot
{
    std::vector<OwnershipRecord> records;
    std::vector<PermissionGrant> grants;
    std::vector<AccessRule> rules;
    std::vector<PropertyClaim> claims;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot ownership_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot grant_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot rule_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot claim_ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};
struct OwnershipDiagnostics
{
    std::uint64_t ownership_records = 0, permission_grants = 0, access_rules = 0, property_claims = 0,
                  permission_queries = 0, denied_permissions = 0, crime_candidate_permissions = 0,
                  ownership_transfers = 0, claim_conflicts = 0, expired_permissions = 0,
                  journal_dropped_changes = 0;
};

// Optional neutral boundary used only for AccessDecision::RequireRole. The provider is non-owning
// and must outlive the OwnershipService. It must not mutate Ownership from HasRole().
class IOwnershipRoleProvider
{
  public:
    virtual ~IOwnershipRoleProvider() = default;
    [[nodiscard]] virtual bool HasRole(GameplayObjectRef subject, TypeId role,
                                       const GameplayContext &context) const noexcept = 0;
};

class OwnershipService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<OwnershipService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.ownership");
    }
    [[nodiscard]] foundation::Result<OwnershipRecordId> AssignOwnership(OwnershipRecord record);
    [[nodiscard]] foundation::Result<void> RemoveOwnership(OwnershipRecordId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> TransferOwnership(TransferOwnershipRequest request);
    [[nodiscard]] foundation::Result<PermissionGrantId> GrantPermission(PermissionGrant grant);
    [[nodiscard]] foundation::Result<void> RevokePermission(PermissionGrantId id, GameplayContext context = {});
    [[nodiscard]] std::size_t SweepExpiredPermissions(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] foundation::Result<AccessRuleId> AddAccessRule(AccessRule rule);
    [[nodiscard]] foundation::Result<void> UpdateAccessRule(AccessRule rule, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveAccessRule(AccessRuleId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<PropertyClaimId> CreateClaim(PropertyClaim claim);
    [[nodiscard]] foundation::Result<void> ResolveClaim(PropertyClaimId id, GameplayContext context = {});
    [[nodiscard]] OwnershipPermissionResult EvaluatePermission(OwnershipPermissionQuery query) const;
    [[nodiscard]] const OwnershipRecord *GetOwner(GameplayObjectRef property) const noexcept;
    [[nodiscard]] const OwnershipRecord *GetOwner(GameplayObjectRef property, PropertyDomainId domain) const noexcept;
    [[nodiscard]] std::vector<OwnershipRecord> GetOwnershipRecords(GameplayObjectRef property) const;
    [[nodiscard]] std::vector<OwnershipRecord> FindPropertiesOwnedBy(GameplayObjectRef owner) const;
    [[nodiscard]] std::vector<PermissionGrant> FindPermissions(GameplayObjectRef subject) const;
    [[nodiscard]] std::vector<PropertyClaim> FindClaims(GameplayObjectRef property) const;
    [[nodiscard]] std::vector<AccessRule> FindAccessRules(GameplayObjectRef property) const;
    private:
        [[nodiscard]] std::vector<OwnershipChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] OwnershipChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] OwnershipChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] OwnershipSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(OwnershipSnapshot snapshot);
    void SetRoleProvider(const IOwnershipRoleProvider *provider) noexcept { role_provider_ = provider; }
    [[nodiscard]] OwnershipDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }

  private:
    struct OwnerKey
    {
        GameplayObjectRef property{};
        PropertyDomainId domain{};
        [[nodiscard]] bool operator==(const OwnerKey &) const noexcept = default;
    };
    struct OwnerKeyHash
    {
        [[nodiscard]] std::size_t operator()(const OwnerKey &key) const noexcept
        {
            auto h = std::hash<GameplayObjectRef>{}(key.property);
            const auto d = std::hash<TypeId>{}(key.domain.value);
            return h ^ (d + 0x9e3779b97f4a7c15ULL + (h << 6U) + (h >> 2U));
        }
    };

    static constexpr std::size_t kChangeRetention = 4096;
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    [[nodiscard]] bool CanRecordChanges(std::size_t count = 1) const noexcept;
    void Record(OwnershipChange change);
    [[nodiscard]] OwnershipRecord *FindMutableOwnership(OwnershipRecordId id) noexcept;
    [[nodiscard]] const PermissionGrant *FindBestActiveGrant(GameplayObjectRef subject, GameplayObjectRef property,
                                                              PropertyRightId right, GameplayTimePoint now) const noexcept;
    [[nodiscard]] bool IsPublicProperty(GameplayObjectRef property, PropertyDomainId domain) const noexcept;
    void IndexOwnership(const OwnershipRecord &record);
    void UnindexOwnership(const OwnershipRecord &record);
    void IndexGrant(const PermissionGrant &grant);
    void UnindexGrant(const PermissionGrant &grant);
    void IndexRule(const AccessRule &rule);
    void UnindexRule(const AccessRule &rule);
    void IndexClaim(const PropertyClaim &claim);
    void UnindexClaim(const PropertyClaim &claim);

    Revision revision_{};
    MonotonicIdGenerator<GameplayObjectId> ownership_ids_{0x2300};
    MonotonicIdGenerator<GameplayObjectId> grant_ids_{0x2301};
    MonotonicIdGenerator<GameplayObjectId> rule_ids_{0x2302};
    MonotonicIdGenerator<GameplayObjectId> claim_ids_{0x2303};
    std::unordered_map<OwnershipRecordId, OwnershipRecord, IdHash> records_;
    std::unordered_map<PermissionGrantId, PermissionGrant, IdHash> grants_;
    std::unordered_map<AccessRuleId, AccessRule, IdHash> rules_;
    std::unordered_map<PropertyClaimId, PropertyClaim, IdHash> claims_;
    std::unordered_map<OwnerKey, OwnershipRecordId, OwnerKeyHash> canonical_owners_;
    std::unordered_map<GameplayObjectRef, std::vector<OwnershipRecordId>, RefHash> ownership_by_property_;
    std::unordered_map<GameplayObjectRef, std::vector<OwnershipRecordId>, RefHash> ownership_by_owner_;
    std::unordered_map<GameplayObjectRef, std::vector<PermissionGrantId>, RefHash> grants_by_subject_;
    std::unordered_map<GameplayObjectRef, std::vector<AccessRuleId>, RefHash> rules_by_property_;
    std::unordered_map<GameplayObjectRef, std::vector<PropertyClaimId>, RefHash> claims_by_property_;
    std::deque<OwnershipChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    const IOwnershipRoleProvider *role_provider_ = nullptr;
    mutable OwnershipDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::ownership

namespace std
{
template <> struct hash<epidemic::gameplay::ownership::OwnershipRecordId>
{
    size_t operator()(const epidemic::gameplay::ownership::OwnershipRecordId &v) const noexcept { return hash<epidemic::gameplay::GameplayObjectId>{}(v.value); }
};
template <> struct hash<epidemic::gameplay::ownership::PermissionGrantId>
{
    size_t operator()(const epidemic::gameplay::ownership::PermissionGrantId &v) const noexcept { return hash<epidemic::gameplay::GameplayObjectId>{}(v.value); }
};
template <> struct hash<epidemic::gameplay::ownership::AccessRuleId>
{
    size_t operator()(const epidemic::gameplay::ownership::AccessRuleId &v) const noexcept { return hash<epidemic::gameplay::GameplayObjectId>{}(v.value); }
};
template <> struct hash<epidemic::gameplay::ownership::PropertyClaimId>
{
    size_t operator()(const epidemic::gameplay::ownership::PropertyClaimId &v) const noexcept { return hash<epidemic::gameplay::GameplayObjectId>{}(v.value); }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `OwnershipStrength`: `Owned`, `Claimed`, `Borrowed`, `Leased`, `Reserved`, `Stolen`, `Contested`, `Abandoned`, `Public`, `Unowned`
- `PermissionStrength`: `Weak`, `Normal`, `Strong`, `Absolute`
- `AccessDecision`: `Allow`, `Deny`, `RequirePermission`, `RequireRole`, `RequireOwner`, `Public`, `Private`, `TrespassIfViolated`, `CrimeIfViolated`
- `PermissionDecision`: `Allowed`, `Denied`, `Trespass`, `CrimeCandidate`, `Unknown`, `NotApplicable`
- `TransferReason`: `Trade`, `Gift`, `Reward`, `Loot`, `Theft`, `Confiscation`, `Inheritance`, `Abandonment`, `ConstructionCreated`, `GameSpecific`
- `OwnershipChangeKind`: `OwnershipAssigned`, `OwnershipTransferred`, `OwnershipRemoved`, `PermissionGranted`, `PermissionRevoked`, `AccessRuleAdded`, `AccessRuleChanged`, `AccessRuleRemoved`, `PropertyClaimCreated`, `PropertyClaimResolved`

### Состав типов и операции

#### `OwnershipRecordId` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
static constexpr OwnershipRecordId FromString(std::string_view s) noexcept;
static constexpr OwnershipRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const OwnershipRecordId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const OwnershipRecordId &) const noexcept = default;
```

#### `PropertyClaimId` (строка заголовка 27)

Публичные сигнатуры операций:

```cpp
static constexpr PropertyClaimId FromString(std::string_view s) noexcept;
static constexpr PropertyClaimId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PropertyClaimId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PropertyClaimId &) const noexcept = default;
```

#### `AccessRuleId` (строка заголовка 36)

Публичные сигнатуры операций:

```cpp
static constexpr AccessRuleId FromString(std::string_view s) noexcept;
static constexpr AccessRuleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AccessRuleId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const AccessRuleId &) const noexcept = default;
```

#### `PermissionGrantId` (строка заголовка 45)

Публичные сигнатуры операций:

```cpp
static constexpr PermissionGrantId FromString(std::string_view s) noexcept;
static constexpr PermissionGrantId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PermissionGrantId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PermissionGrantId &) const noexcept = default;
```

#### `PropertyRightId` (строка заголовка 54)

Публичные сигнатуры операций:

```cpp
static constexpr PropertyRightId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PropertyRightId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PropertyRightId &) const noexcept = default;
```

#### `PropertyDomainId` (строка заголовка 62)

Публичные сигнатуры операций:

```cpp
static constexpr PropertyDomainId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PropertyDomainId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PropertyDomainId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 70)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 72)

#### `RefHash` (строка заголовка 77)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef r) const noexcept;
```

#### `OwnershipRecord` (строка заголовка 150)

Публичные поля и значения по умолчанию:

- `OwnershipStrength strength = OwnershipStrength::Owned`
- `std::vector<std::byte> payload`

#### `PermissionGrant` (строка заголовка 161)

Публичные поля и значения по умолчанию:

- `std::optional<GameplayTimePoint> expires_at`
- `PermissionStrength strength = PermissionStrength::Normal`

#### `AccessRule` (строка заголовка 173)

Публичные поля и значения по умолчанию:

- `AccessDecision decision = AccessDecision::RequirePermission`
- `int priority = 0`
- `PermissionStrength minimum_permission_strength = PermissionStrength::Weak`

#### `PropertyClaim` (строка заголовка 186)

Публичные поля и значения по умолчанию:

- `std::int64_t strength_micro = 0`
- `std::vector<std::byte> payload`

#### `OwnershipPermissionResult` (строка заголовка 204)

Публичные поля и значения по умолчанию:

- `PermissionDecision decision = PermissionDecision::Unknown`
- `std::vector<TypeId> reasons`

#### `TransferOwnershipRequest` (строка заголовка 213)

Публичные поля и значения по умолчанию:

- `TransferReason reason = TransferReason::Trade`

#### `OwnershipChange` (строка заголовка 223)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `OwnershipChangeKind kind = OwnershipChangeKind::OwnershipAssigned`
- `TransferReason transfer_reason = TransferReason::GameSpecific`

#### `OwnershipChangeBatch` (строка заголовка 237)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `std::vector<OwnershipChange> changes`

#### `OwnershipSnapshot` (строка заголовка 247)

Публичные поля и значения по умолчанию:

- `std::vector<OwnershipRecord> records`
- `std::vector<PermissionGrant> grants`
- `std::vector<AccessRule> rules`
- `std::vector<PropertyClaim> claims`
- `std::uint64_t change_epoch = 1`

#### `IOwnershipRoleProvider` (строка заголовка 271)

Публичные сигнатуры операций:

```cpp
virtual ~IOwnershipRoleProvider() = default;
[[nodiscard]] virtual bool HasRole(GameplayObjectRef subject, TypeId role, const GameplayContext &context) const noexcept = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `OwnershipService` (строка заголовка 279)

Внутреннее состояние (не часть публичного API): `static constexpr std::size_t kChangeRetention = 4096`, `std::unordered_map<OwnershipRecordId, OwnershipRecord, IdHash> records_`, `std::unordered_map<PermissionGrantId, PermissionGrant, IdHash> grants_`, `std::unordered_map<AccessRuleId, AccessRule, IdHash> rules_`, `std::unordered_map<PropertyClaimId, PropertyClaim, IdHash> claims_`, `std::unordered_map<OwnerKey, OwnershipRecordId, OwnerKeyHash> canonical_owners_`, `std::unordered_map<GameplayObjectRef, std::vector<OwnershipRecordId>, RefHash> ownership_by_pro`, `std::unordered_map<GameplayObjectRef, std::vector<OwnershipRecordId>, RefHash> ownership_by_own`, `std::unordered_map<GameplayObjectRef, std::vector<PermissionGrantId>, RefHash> grants_by_subjec`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<OwnershipRecordId> AssignOwnership(OwnershipRecord record);
[[nodiscard]] foundation::Result<void> TransferOwnership(TransferOwnershipRequest request);
[[nodiscard]] foundation::Result<PermissionGrantId> GrantPermission(PermissionGrant grant);
[[nodiscard]] foundation::Result<AccessRuleId> AddAccessRule(AccessRule rule);
[[nodiscard]] foundation::Result<PropertyClaimId> CreateClaim(PropertyClaim claim);
[[nodiscard]] OwnershipPermissionResult EvaluatePermission(OwnershipPermissionQuery query) const;
[[nodiscard]] const OwnershipRecord *GetOwner(GameplayObjectRef property) const noexcept;
[[nodiscard]] const OwnershipRecord *GetOwner(GameplayObjectRef property, PropertyDomainId domain) const noexcept;
[[nodiscard]] std::vector<OwnershipRecord> GetOwnershipRecords(GameplayObjectRef property) const;
[[nodiscard]] std::vector<OwnershipRecord> FindPropertiesOwnedBy(GameplayObjectRef owner) const;
[[nodiscard]] std::vector<PermissionGrant> FindPermissions(GameplayObjectRef subject) const;
[[nodiscard]] std::vector<PropertyClaim> FindClaims(GameplayObjectRef property) const;
[[nodiscard]] std::vector<AccessRule> FindAccessRules(GameplayObjectRef property) const;
[[nodiscard]] OwnershipChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesThrough(std::uint64_t sequence);
[[nodiscard]] OwnershipSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(OwnershipSnapshot snapshot);
void SetRoleProvider(const IOwnershipRoleProvider *provider) noexcept;
[[nodiscard]] OwnershipDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `OwnerKey` (строка заголовка 337)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const OwnerKey &) const noexcept = default;
```

#### `OwnerKeyHash` (строка заголовка 343)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const OwnerKey &key) const noexcept;
```

#### `hash` (строка заголовка 395)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::ownership::OwnershipRecordId &v) const noexcept;
```

#### `hash` (строка заголовка 399)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::ownership::PermissionGrantId &v) const noexcept;
```

#### `hash` (строка заголовка 403)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::ownership::AccessRuleId &v) const noexcept;
```

#### `hash` (строка заголовка 407)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::ownership::PropertyClaimId &v) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `AssignOwnership` | `OwnershipService::AssignOwnership` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:267` |
| `TransferOwnership` | `OwnershipService::TransferOwnership` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:347` |
| `GrantPermission` | `OwnershipService::GrantPermission` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:419` |
| `AddAccessRule` | `OwnershipService::AddAccessRule` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:487` |
| `CreateClaim` | `OwnershipService::CreateClaim` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:534` |
| `EvaluatePermission` | `OwnershipService::EvaluatePermission` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:609` |
| `GetOwner` | `OwnershipService::GetOwner` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:741` |
| `GetOwnershipRecords` | `OwnershipService::GetOwnershipRecords` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:768` |
| `FindPropertiesOwnedBy` | `OwnershipService::FindPropertiesOwnedBy` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:782` |
| `FindPermissions` | `OwnershipService::FindPermissions` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:802` |
| `FindClaims` | `OwnershipService::FindClaims` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:816` |
| `FindAccessRules` | `OwnershipService::FindAccessRules` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:830` |
| `PruneChangesThrough` | `OwnershipService::PruneChangesThrough` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:875` |
| `CaptureSnapshot` | `OwnershipService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:881` |
| `RestoreSnapshot` | `OwnershipService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:921` |
| `GetDiagnostics` | `OwnershipService::GetDiagnostics` | `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:1048` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `AssignOwnership`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:38`
- `GetOwner`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:57`
- `EvaluatePermission`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:75`
- `GrantPermission`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:87`
- `AddAccessRule`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:98`
- `FindPermissions`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:119`
- `FindAccessRules`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:144`
- `SetRoleProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:150`
- `TransferOwnership`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:172`
- `CreateClaim`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:201`
- `FindClaims`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:202`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp:208`

