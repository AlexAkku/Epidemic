---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h"
---

# Combat: combat.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`.

**Модуль:** [[06 EngineFramework/Модуль Combat|Combat]].
**Размер исходного файла:** 14912 байт.
**Подключение:** `#include <Epidemic/GameFramework/Combat/combat.h>`.

## Объявления типов (лексический индекс)
`DamageTypeId`, `DamageProfileId`, `CombatResourceTypeId`, `CombatModifierTypeId`, `CombatResolutionId`, `CombatResourceReservationId`, `IdHash`, `CombatResourceDepletionEffect`, `CombatResourceDefinition`, `DamageProfile`, `CombatLifeState`, `CombatEngagementState`, `CombatResourceState`, `CombatResourceReservation`, `CombatantRecord`, `CombatModifierPhase`, `CombatModifierOperation`, `CombatModifier`, `DamageRequest`, `ICombatModifierProvider`, `CombatOutcome`, `CombatPlan`, `CombatPreparePolicy`, `CombatResult`, `CombatChangeKind`, `CombatChange`, `CombatChangeBatch`, `CombatSnapshot`, `CombatDiagnostics`, `CombatService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"
#include "Epidemic/GameFramework/SupportRandom/deterministic_random.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::combat
{
struct DamageTypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr DamageTypeId FromString(std::string_view name) noexcept { return {TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr auto operator<=>(const DamageTypeId&) const noexcept = default;
};
struct DamageProfileId
{
    TypeId value{};
    [[nodiscard]] static constexpr DamageProfileId FromString(std::string_view name) noexcept { return {TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr auto operator<=>(const DamageProfileId&) const noexcept = default;
};
struct CombatResourceTypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr CombatResourceTypeId FromString(std::string_view name) noexcept { return {TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr auto operator<=>(const CombatResourceTypeId&) const noexcept = default;
};
struct CombatModifierTypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr CombatModifierTypeId FromString(std::string_view name) noexcept { return {TypeId::FromString(name)}; }
    [[nodiscard]] constexpr auto operator<=>(const CombatModifierTypeId&) const noexcept = default;
};
struct CombatResolutionId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr auto operator<=>(const CombatResolutionId&) const noexcept = default;
};
struct CombatResourceReservationId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr auto operator<=>(const CombatResourceReservationId&) const noexcept = default;
};
struct IdHash
{
    template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept
    {
        if constexpr (requires { id.value.Raw(); })
            return std::hash<TypeId>{}(id.value);
        else
            return std::hash<GameplayObjectId>{}(id.value);
    }
};

enum class CombatResourceDepletionEffect { None, Downed, Dead };

struct CombatResourceDefinition
{
    CombatResourceTypeId id{};
    std::string canonical_name;
    std::int64_t minimum_micro = 0;
    std::int64_t default_maximum_micro = 100'000'000;
    CombatResourceDepletionEffect depletion_effect = CombatResourceDepletionEffect::None;
};

struct DamageProfile
{
    DamageProfileId id{};
    std::string canonical_name;
    CombatResourceTypeId target_resource{};
    bool can_miss = true;
    bool can_evade = true;
    bool can_block = true;
    bool can_critical = true;
    std::uint32_t hit_chance_micro = 1'000'000;
    std::uint32_t evade_chance_micro = 0;
    std::uint32_t block_chance_micro = 0;
    std::uint32_t critical_chance_micro = 0;
    std::int64_t blocked_multiplier_micro = 500'000;
    std::int64_t critical_multiplier_micro = 1'500'000;
    std::int64_t flat_mitigation_micro = 0;
};

enum class CombatLifeState { Alive, Downed, Dead, Disabled };
enum class CombatEngagementState { OutOfCombat, Engaged, Disengaging };

struct CombatResourceState
{
    CombatResourceTypeId type{};
    std::int64_t current_micro = 0;
    std::int64_t maximum_micro = 0;
};

struct CombatResourceReservation
{
    CombatResourceReservationId id{};
    GameplayObjectRef subject{};
    CombatResourceTypeId resource{};
    std::int64_t amount_micro = 0;
    GameplayContext context{};
};

struct CombatantRecord
{
    GameplayObjectRef subject{};
    CombatLifeState life_state = CombatLifeState::Alive;
    CombatEngagementState engagement = CombatEngagementState::OutOfCombat;
    std::vector<CombatResourceState> resources;
    Revision revision{};
};

enum class CombatModifierPhase { Base, Attacker, Target, Mitigation, Amplification, Final };
enum class CombatModifierOperation { Add, Multiply, Override, ClampMin, ClampMax };
struct CombatModifier
{
    CombatModifierPhase phase = CombatModifierPhase::Base;
    CombatModifierOperation operation = CombatModifierOperation::Add;
    CombatModifierTypeId type{};
    std::int32_t priority = 0;
    std::int64_t value_micro = 0;
    GameplayObjectRef source{};
};

struct DamageRequest
{
    GameplayObjectRef source{};
    GameplayObjectRef instigator{};
    GameplayObjectRef target{};
    DamageTypeId damage_type{};
    DamageProfileId profile{};
    std::int64_t base_amount_micro = 0;
    random::RandomSeed seed{};
    GameplayContext context{};
};

class ICombatModifierProvider
{
  public:
    virtual ~ICombatModifierProvider() = default;
    [[nodiscard]] virtual std::vector<CombatModifier> Collect(const DamageRequest& request) const = 0;
    [[nodiscard]] virtual Revision RevisionFor(const DamageRequest& request) const noexcept = 0;
};

enum class CombatOutcome : std::uint32_t
{
    None = 0,
    Missed = 1u << 0u,
    Evaded = 1u << 1u,
    Blocked = 1u << 2u,
    Critical = 1u << 3u,
    Killed = 1u << 4u,
    Downed = 1u << 5u,
};
[[nodiscard]] constexpr CombatOutcome operator|(CombatOutcome a, CombatOutcome b) noexcept { return static_cast<CombatOutcome>(static_cast<std::uint32_t>(a)|static_cast<std::uint32_t>(b)); }
[[nodiscard]] constexpr bool HasOutcome(CombatOutcome value, CombatOutcome flag) noexcept { return (static_cast<std::uint32_t>(value)&static_cast<std::uint32_t>(flag))!=0; }

struct CombatPlan
{
    CombatResolutionId id{};
    DamageRequest request;
    CombatResourceTypeId resource{};
    Revision expected_target_revision{};
    Revision expected_modifier_revision{};
    std::uint64_t expected_provider_epoch = 0;
    std::int64_t requested_amount_micro = 0;
    std::int64_t final_amount_micro = 0;
    CombatOutcome outcomes = CombatOutcome::None;
    GameplayTimePoint prepared_at{};
    GameplayTimePoint expires_at{};
};

struct CombatPreparePolicy
{
    GameplayDuration lifetime{1};
};

struct CombatResult
{
    CombatResolutionId id{};
    GameplayObjectRef target{};
    CombatResourceTypeId resource{};
    std::int64_t requested_amount_micro = 0;
    std::int64_t final_amount_micro = 0;
    std::int64_t resource_before_micro = 0;
    std::int64_t resource_after_micro = 0;
    CombatLifeState before = CombatLifeState::Alive;
    CombatLifeState after = CombatLifeState::Alive;
    CombatOutcome outcomes = CombatOutcome::None;
    GameplayContext context{};
};

enum class CombatChangeKind { CombatantRegistered, CombatantRemoved, EngagementChanged, DamageResolved, ResourceChanged, LifeStateChanged };
struct CombatChange
{
    std::uint64_t sequence = 0;
    CombatChangeKind kind = CombatChangeKind::CombatantRegistered;
    GameplayObjectRef subject{};
    CombatResolutionId resolution{};
    CombatResourceTypeId resource{};
    CombatLifeState life_state = CombatLifeState::Alive;
    Revision revision{};
    GameplayContext context{};
};

struct CombatChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::vector<CombatChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct CombatSnapshot
{
    std::vector<CombatantRecord> combatants;
    std::vector<CombatResourceReservation> resource_reservations;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot resolution_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot resource_reservation_ids{};
    std::vector<CombatChange> journal;
    std::uint64_t next_change_sequence = 1;

    std::uint64_t change_epoch = 1;
};
struct CombatDiagnostics
{
    std::uint64_t combatants = 0;
    std::uint64_t damage_requests = 0;
    std::uint64_t damage_resolved = 0;
    std::uint64_t criticals = 0;
    std::uint64_t blocks = 0;
    std::uint64_t evades = 0;
    std::uint64_t deaths = 0;
    std::uint64_t stale_commits = 0;
};

class CombatService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<CombatService>;

  public:
    CombatService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept { return GameplayDomainId::FromString("framework.combat"); }
    [[nodiscard]] foundation::Result<CombatResourceTypeId> RegisterResource(CombatResourceDefinition definition);
    [[nodiscard]] foundation::Result<DamageTypeId> RegisterDamageType(std::string canonical_name);
    [[nodiscard]] foundation::Result<DamageProfileId> RegisterDamageProfile(DamageProfile profile);
    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }
    void SetModifierProvider(const ICombatModifierProvider* provider) noexcept;

    [[nodiscard]] foundation::Result<void> RegisterCombatant(GameplayObjectRef subject, std::vector<CombatResourceState> resources, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveCombatant(GameplayObjectRef subject, GameplayContext context = {});
    [[nodiscard]] const CombatantRecord* FindCombatant(GameplayObjectRef subject) const noexcept;
    [[nodiscard]] foundation::Result<CombatResourceState> GetResource(GameplayObjectRef subject, CombatResourceTypeId type) const;
    [[nodiscard]] foundation::Result<void> SetResourceMaximum(GameplayObjectRef subject, CombatResourceTypeId type, std::int64_t maximum_micro, bool preserve_ratio, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ModifyResource(GameplayObjectRef subject, CombatResourceTypeId type, std::int64_t delta_micro, GameplayContext context = {});
    // Resource reservations are authoritative Combat state. Reserve performs the tentative debit,
    // while Commit only finalizes it and Release restores it. Active reservations are persisted.
    [[nodiscard]] foundation::Result<CombatResourceReservationId> ReserveResource(
        GameplayObjectRef subject, CombatResourceTypeId type, std::int64_t amount_micro, GameplayContext context = {});
    void CommitResourceReservation(CombatResourceReservationId reservation) noexcept;
    void ReleaseResourceReservation(CombatResourceReservationId reservation, GameplayContext context = {}) noexcept;
    [[nodiscard]] const CombatResourceReservation* FindResourceReservation(CombatResourceReservationId reservation) const noexcept;
    [[nodiscard]] foundation::Result<void> SetEngagement(GameplayObjectRef subject, CombatEngagementState state, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> TransitionLifeState(GameplayObjectRef subject, CombatLifeState state, GameplayContext context = {});

    [[nodiscard]] foundation::Result<void> SetPreparePolicy(CombatPreparePolicy policy);
    [[nodiscard]] CombatPreparePolicy GetPreparePolicy() const noexcept { return prepare_policy_; }
    [[nodiscard]] foundation::Result<CombatPlan> PrepareDamage(DamageRequest request);
    [[nodiscard]] foundation::Result<CombatResult> CommitDamage(const CombatPlan& plan, GameplayTimePoint now);
    [[nodiscard]] foundation::Result<void> CancelDamagePlan(CombatResolutionId id);
    [[nodiscard]] std::size_t ExpirePreparedPlans(GameplayTimePoint now) noexcept;

    private:
        [[nodiscard]] std::vector<CombatChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] CombatChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] CombatChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] CombatSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(CombatSnapshot snapshot);
    [[nodiscard]] CombatDiagnostics GetDiagnostics() const noexcept;

  private:
    [[nodiscard]] static std::optional<Revision> NextRevision(const CombatantRecord& record) noexcept;
    void Record(CombatChange change) noexcept;
    [[nodiscard]] bool ReconcileLifeState(CombatantRecord& record, CombatResourceTypeId changed_resource) noexcept;
    [[nodiscard]] static bool IsAllowedLifeTransition(CombatLifeState from, CombatLifeState to) noexcept;
    [[nodiscard]] static std::int64_t ApplyModifiers(std::int64_t amount, std::vector<CombatModifier> modifiers) noexcept;

    std::unordered_map<CombatResourceTypeId, CombatResourceDefinition, IdHash> resources_;
    std::unordered_map<DamageTypeId, std::string, IdHash> damage_types_;
    std::unordered_map<DamageProfileId, DamageProfile, IdHash> damage_profiles_;
    std::unordered_map<GameplayObjectRef, CombatantRecord> combatants_;
    const ICombatModifierProvider* modifier_provider_ = nullptr;
    std::uint64_t modifier_provider_epoch_ = 1;
    MonotonicIdGenerator<GameplayObjectId> resolution_ids_;
    MonotonicIdGenerator<GameplayObjectId> resource_reservation_ids_;
    std::unordered_map<CombatResourceReservationId, CombatResourceReservation, IdHash> resource_reservations_;
    std::unordered_map<CombatResolutionId, CombatPlan, IdHash> prepared_plans_;
    CombatPreparePolicy prepare_policy_{};
    bool frozen_ = false;
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    static constexpr std::size_t kPreparedPlanCapacity = 4096;
    std::deque<CombatChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    CombatDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::combat
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`, `Epidemic/GameFramework/SupportRandom/deterministic_random.h`.

### Перечисления и допустимые категории

- `CombatResourceDepletionEffect`: `None`, `Downed`, `Dead`
- `CombatLifeState`: `Alive`, `Downed`, `Dead`, `Disabled`
- `CombatEngagementState`: `OutOfCombat`, `Engaged`, `Disengaging`
- `CombatModifierPhase`: `Base`, `Attacker`, `Target`, `Mitigation`, `Amplification`, `Final`
- `CombatModifierOperation`: `Add`, `Multiply`, `Override`, `ClampMin`, `ClampMax`
- `CombatOutcome`: `None`, `Missed`, `Evaded`, `Blocked`, `Critical`, `Killed`, `Downed`
- `CombatChangeKind`: `CombatantRegistered`, `CombatantRemoved`, `EngagementChanged`, `DamageResolved`, `ResourceChanged`, `LifeStateChanged`

### Состав типов и операции

#### `DamageTypeId` (строка заголовка 17)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr DamageTypeId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const DamageTypeId&) const noexcept = default;
```

#### `DamageProfileId` (строка заголовка 24)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr DamageProfileId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const DamageProfileId&) const noexcept = default;
```

#### `CombatResourceTypeId` (строка заголовка 31)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr CombatResourceTypeId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const CombatResourceTypeId&) const noexcept = default;
```

#### `CombatModifierTypeId` (строка заголовка 38)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr CombatModifierTypeId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr auto operator<=>(const CombatModifierTypeId&) const noexcept = default;
```

#### `CombatResolutionId` (строка заголовка 44)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const CombatResolutionId&) const noexcept = default;
```

#### `CombatResourceReservationId` (строка заголовка 50)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const CombatResourceReservationId&) const noexcept = default;
```

#### `IdHash` (строка заголовка 56)

Публичные сигнатуры операций:

```cpp
template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept;
```

#### `CombatResourceDefinition` (строка заголовка 69)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::int64_t minimum_micro = 0`
- `std::int64_t default_maximum_micro = 100'000'000`
- `CombatResourceDepletionEffect depletion_effect = CombatResourceDepletionEffect::None`

#### `CombatResourceState` (строка заголовка 99)

Публичные поля и значения по умолчанию:

- `std::int64_t current_micro = 0`
- `std::int64_t maximum_micro = 0`

#### `CombatResourceReservation` (строка заголовка 106)

Публичные поля и значения по умолчанию:

- `std::int64_t amount_micro = 0`

#### `CombatantRecord` (строка заголовка 115)

Публичные поля и значения по умолчанию:

- `CombatLifeState life_state = CombatLifeState::Alive`
- `CombatEngagementState engagement = CombatEngagementState::OutOfCombat`
- `std::vector<CombatResourceState> resources`

#### `CombatModifier` (строка заголовка 126)

Публичные поля и значения по умолчанию:

- `CombatModifierPhase phase = CombatModifierPhase::Base`
- `CombatModifierOperation operation = CombatModifierOperation::Add`
- `std::int32_t priority = 0`
- `std::int64_t value_micro = 0`

#### `DamageRequest` (строка заголовка 136)

Публичные поля и значения по умолчанию:

- `std::int64_t base_amount_micro = 0`

#### `ICombatModifierProvider` (строка заголовка 148)

Публичные сигнатуры операций:

```cpp
virtual ~ICombatModifierProvider() = default;
[[nodiscard]] virtual std::vector<CombatModifier> Collect(const DamageRequest& request) const = 0;
[[nodiscard]] virtual Revision RevisionFor(const DamageRequest& request) const noexcept = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `CombatPlan` (строка заголовка 169)

Публичные поля и значения по умолчанию:

- `DamageRequest request`
- `std::uint64_t expected_provider_epoch = 0`
- `std::int64_t requested_amount_micro = 0`
- `std::int64_t final_amount_micro = 0`
- `CombatOutcome outcomes = CombatOutcome::None`

#### `CombatResult` (строка заголовка 189)

Публичные поля и значения по умолчанию:

- `std::int64_t requested_amount_micro = 0`
- `std::int64_t final_amount_micro = 0`
- `std::int64_t resource_before_micro = 0`
- `std::int64_t resource_after_micro = 0`
- `CombatLifeState before = CombatLifeState::Alive`
- `CombatLifeState after = CombatLifeState::Alive`
- `CombatOutcome outcomes = CombatOutcome::None`

#### `CombatChange` (строка заголовка 205)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `CombatChangeKind kind = CombatChangeKind::CombatantRegistered`
- `CombatLifeState life_state = CombatLifeState::Alive`

#### `CombatChangeBatch` (строка заголовка 217)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::vector<CombatChange> changes`

#### `CombatSnapshot` (строка заголовка 226)

Публичные поля и значения по умолчанию:

- `std::vector<CombatantRecord> combatants`
- `std::vector<CombatResourceReservation> resource_reservations`
- `std::vector<CombatChange> journal`
- `std::uint64_t next_change_sequence = 1`
- `std::uint64_t change_epoch = 1`

#### `CombatDiagnostics` (строка заголовка 237)

Публичные поля и значения по умолчанию:

- `std::uint64_t combatants = 0`
- `std::uint64_t damage_requests = 0`
- `std::uint64_t damage_resolved = 0`
- `std::uint64_t criticals = 0`
- `std::uint64_t blocks = 0`
- `std::uint64_t evades = 0`
- `std::uint64_t deaths = 0`
- `std::uint64_t stale_commits = 0`

#### `CombatService` (строка заголовка 249)

Внутреннее состояние (не часть публичного API): `std::unordered_map<CombatResourceTypeId, CombatResourceDefinition, IdHash> resources_`, `std::unordered_map<DamageTypeId, std::string, IdHash> damage_types_`, `std::unordered_map<DamageProfileId, DamageProfile, IdHash> damage_profiles_`, `std::unordered_map<GameplayObjectRef, CombatantRecord> combatants_`, `const ICombatModifierProvider* modifier_provider_ = nullptr`, `std::uint64_t modifier_provider_epoch_ = 1`, `MonotonicIdGenerator<GameplayObjectId> resolution_ids_`, `MonotonicIdGenerator<GameplayObjectId> resource_reservation_ids_`, `std::unordered_map<CombatResourceReservationId, CombatResourceReservation, IdHash> resource_res`.

Публичные сигнатуры операций:

```cpp
CombatService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<CombatResourceTypeId> RegisterResource(CombatResourceDefinition definition);
[[nodiscard]] foundation::Result<DamageTypeId> RegisterDamageType(std::string canonical_name);
[[nodiscard]] foundation::Result<DamageProfileId> RegisterDamageProfile(DamageProfile profile);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
void SetModifierProvider(const ICombatModifierProvider* provider) noexcept;
[[nodiscard]] const CombatantRecord* FindCombatant(GameplayObjectRef subject) const noexcept;
[[nodiscard]] foundation::Result<CombatResourceState> GetResource(GameplayObjectRef subject, CombatResourceTypeId type) const;
void CommitResourceReservation(CombatResourceReservationId reservation) noexcept;
[[nodiscard]] const CombatResourceReservation* FindResourceReservation(CombatResourceReservationId reservation) const noexcept;
[[nodiscard]] foundation::Result<void> SetPreparePolicy(CombatPreparePolicy policy);
[[nodiscard]] CombatPreparePolicy GetPreparePolicy() const noexcept;
[[nodiscard]] foundation::Result<CombatPlan> PrepareDamage(DamageRequest request);
[[nodiscard]] foundation::Result<CombatResult> CommitDamage(const CombatPlan& plan, GameplayTimePoint now);
[[nodiscard]] foundation::Result<void> CancelDamagePlan(CombatResolutionId id);
[[nodiscard]] std::size_t ExpirePreparedPlans(GameplayTimePoint now) noexcept;
[[nodiscard]] CombatChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] CombatSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(CombatSnapshot snapshot);
[[nodiscard]] CombatDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `CombatService` | `CombatService::CombatService` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:140` |
| `SetModifierProvider` | `CombatService::SetModifierProvider` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:145` |
| `RegisterResource` | `CombatService::RegisterResource` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:154` |
| `RegisterDamageType` | `CombatService::RegisterDamageType` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:172` |
| `RegisterDamageProfile` | `CombatService::RegisterDamageProfile` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:183` |
| `FindCombatant` | `CombatService::FindCombatant` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:337` |
| `GetResource` | `CombatService::GetResource` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:342` |
| `CommitResourceReservation` | `CombatService::CommitResourceReservation` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:484` |
| `FindResourceReservation` | `CombatService::FindResourceReservation` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:520` |
| `SetPreparePolicy` | `CombatService::SetPreparePolicy` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:587` |
| `PrepareDamage` | `CombatService::PrepareDamage` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:631` |
| `CommitDamage` | `CombatService::CommitDamage` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:720` |
| `CancelDamagePlan` | `CombatService::CancelDamagePlan` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:804` |
| `ExpirePreparedPlans` | `CombatService::ExpirePreparedPlans` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:814` |
| `CaptureSnapshot` | `CombatService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:837` |
| `RestoreSnapshot` | `CombatService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:860` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterResource`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:47`
- `RegisterDamageType`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:48`
- `RegisterDamageProfile`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:54`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:56`
- `PrepareDamage`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:61`
- `CommitDamage`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:63`
- `SetPreparePolicy`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:119`
- `CancelDamagePlan`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:167`
- `ExpirePreparedPlans`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:210`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:213`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:223`
- `GetResource`: `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp:225`

