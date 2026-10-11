---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h"
---

# Abilities: abilities.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`.

**Модуль:** [[06 EngineFramework/Модуль Abilities|Abilities]].
**Размер исходного файла:** 20388 байт.
**Подключение:** `#include <Epidemic/GameFramework/Abilities/abilities.h>`.

## Объявления типов (лексический индекс)
`AbilityDefinitionId`, `AbilityInstanceId`, `AbilityExecutionId`, `CooldownGroupId`, `AbilityResourceTypeId`, `AbilityReservationId`, `IdHash`, `RegisteredAbilityPayload`, `AbilityTargetPolicy`, `AbilityTimingKind`, `AbilityCostPolicy`, `AbilityGrantPersistence`, `AbilityContinuityPolicy`, `AbilityCostDefinition`, `AbilityCooldownDefinition`, `AbilityTimingDefinition`, `AbilityOutputDefinition`, `AbilityDefinition`, `AbilityTargetSet`, `AbilityInstance`, `AbilityExecutionState`, `AbilityResourceReservation`, `AbilityExecution`, `AbilityActivationRequest`, `AbilityAvailability`, `AbilityAvailabilityResult`, `IAbilityRequirementProvider`, `IAbilityMaterializationProvider`, `IAbilityResourceProvider`, `AbilityOutput`, `AbilityChangeKind`, `AbilityChange`, `AbilityCooldownState`, `AbilityChangeBatch`, `AbilitiesSnapshot`, `AbilitiesDiagnostics`, `AbilityService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/error.h"
#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::abilities
{
struct AbilityDefinitionId
{
    TypeId value{};
    [[nodiscard]] static constexpr AbilityDefinitionId FromString(std::string_view n) noexcept
    {
        return {TypeId::FromString(n)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr auto operator<=>(const AbilityDefinitionId &) const noexcept = default;
};
struct AbilityInstanceId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr auto operator<=>(const AbilityInstanceId &) const noexcept = default;
};
struct AbilityExecutionId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr auto operator<=>(const AbilityExecutionId &) const noexcept = default;
};
struct CooldownGroupId
{
    TypeId value{};
    [[nodiscard]] static constexpr CooldownGroupId FromString(std::string_view n) noexcept
    {
        return {TypeId::FromString(n)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr auto operator<=>(const CooldownGroupId &) const noexcept = default;
};
struct AbilityResourceTypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr AbilityResourceTypeId FromString(std::string_view n) noexcept
    {
        return {TypeId::FromString(n)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr auto operator<=>(const AbilityResourceTypeId &) const noexcept = default;
};
struct AbilityReservationId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr auto operator<=>(const AbilityReservationId &) const noexcept = default;
};
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        if constexpr (requires { id.value.Raw(); })
            return std::hash<TypeId>{}(id.value);
        else
            return std::hash<GameplayObjectId>{}(id.value);
    }
};

struct RegisteredAbilityPayload
{
    TypeId type{};
    std::vector<std::byte> bytes;
    template <class T> [[nodiscard]] static RegisteredAbilityPayload FromTrivial(TypeId t, const T &v)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        RegisteredAbilityPayload p;
        p.type = t;
        p.bytes.resize(sizeof(T));
        std::memcpy(p.bytes.data(), &v, sizeof(T));
        return p;
    }
    template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId t) const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (type != t || bytes.size() != sizeof(T))
            return std::nullopt;
        T v{};
        std::memcpy(&v, bytes.data(), sizeof(T));
        return v;
    }
};

enum class AbilityTargetPolicy
{
    Self,
    SingleTarget,
    MultipleTargets,
    Point,
    Area,
    Direction,
    None
};
enum class AbilityTimingKind
{
    Instant,
    CastTime,
    Channel,
    DelayedExecution
};
enum class AbilityCostPolicy
{
    PayOnStart,
    ReserveThenCommit,
    PayOnExecute,
    PayPerChannelTick
};
enum class AbilityGrantPersistence
{
    Permanent,
    Temporary,
    SourceBound
};
enum class AbilityContinuityPolicy
{
    ActivationOnly,
    UntilExecute,
    ThroughoutExecution
};

struct AbilityCostDefinition
{
    AbilityResourceTypeId resource{};
    std::int64_t amount_micro = 0;
    AbilityCostPolicy policy = AbilityCostPolicy::ReserveThenCommit;
};
struct AbilityCooldownDefinition
{
    GameplayDuration duration{};
    CooldownGroupId group{};
    bool starts_on_begin = false;
};
struct AbilityTimingDefinition
{
    AbilityTimingKind kind = AbilityTimingKind::Instant;
    GameplayDuration cast_duration{};
    GameplayDuration channel_interval{};
    GameplayDuration max_channel_duration{};
};
struct AbilityOutputDefinition
{
    ActionTypeId action{};
    std::int64_t magnitude_micro = 0;
    RegisteredAbilityPayload payload;
};
struct AbilityDefinition
{
    AbilityDefinitionId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    AbilityTargetPolicy targeting = AbilityTargetPolicy::None;
    AbilityTimingDefinition timing{};
    AbilityCooldownDefinition cooldown{};
    std::vector<AbilityCostDefinition> costs;
    std::vector<AbilityOutputDefinition> outputs;
    bool requires_materialized_owner = false;
    bool requires_materialized_target = false;
    AbilityContinuityPolicy continuity = AbilityContinuityPolicy::ActivationOnly;
};

struct AbilityTargetSet
{
    GameplayObjectRef primary{};
    std::vector<GameplayObjectRef> targets;
    std::optional<std::array<std::int64_t, 3>> point_micro{};
    std::optional<std::array<std::int64_t, 3>> direction_micro{};
    std::int64_t area_radius_micro = 0;
};

struct AbilityInstance
{
    AbilityInstanceId id{};
    AbilityDefinitionId definition{};
    GameplayObjectRef owner{};
    GameplayObjectRef source{};
    AbilityGrantPersistence persistence = AbilityGrantPersistence::Permanent;
    bool enabled = true;
    Revision revision{};
};

enum class AbilityExecutionState
{
    Casting,
    Channeling,
    Executing,
    Completed,
    Cancelled,
    Interrupted,
    Failed
};
struct AbilityResourceReservation
{
    AbilityReservationId id{};
    AbilityResourceTypeId resource{};
    RegisteredAbilityPayload provider_token;
};
struct AbilityExecution
{
    AbilityExecutionId id{};
    AbilityInstanceId ability{};
    GameplayObjectRef owner{};
    AbilityTargetSet targets;
    AbilityExecutionState state = AbilityExecutionState::Executing;
    GameplayTimePoint started_at{};
    GameplayTimePoint due_at{};
    GameplayTimePoint next_channel_at{};
    std::vector<AbilityResourceReservation> reservations;
    std::optional<ScheduleId> schedule{};
    GameplayContext context{};
    Revision revision{};
};

struct AbilityActivationRequest
{
    AbilityInstanceId ability{};
    AbilityTargetSet targets;
    GameplayTimePoint now{};
    GameplayContext context{};
};
enum class AbilityAvailability
{
    Available,
    Unavailable
};
struct AbilityAvailabilityResult
{
    AbilityAvailability availability = AbilityAvailability::Unavailable;
    TypeId reason{};
};

class IAbilityRequirementProvider
{
  public:
    virtual ~IAbilityRequirementProvider() = default;
    [[nodiscard]] virtual AbilityAvailabilityResult Check(const AbilityDefinition &definition,
                                                          const AbilityInstance &instance,
                                                          const AbilityTargetSet &targets,
                                                          GameplayTimePoint now) const = 0;
};
class IAbilityMaterializationProvider
{
  public:
    virtual ~IAbilityMaterializationProvider() = default;
    [[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef subject) const noexcept = 0;
};
class IAbilityResourceProvider
{
  public:
    virtual ~IAbilityResourceProvider() = default;
    [[nodiscard]] virtual bool CanAfford(GameplayObjectRef owner, AbilityResourceTypeId type,
                                         std::int64_t amount_micro) const = 0;
    [[nodiscard]] virtual foundation::Result<AbilityResourceReservation> Reserve(GameplayObjectRef owner,
                                                                                 AbilityResourceTypeId type,
                                                                                 std::int64_t amount_micro,
                                                                                 GameplayContext context) = 0;
    virtual void Commit(const AbilityResourceReservation &reservation, GameplayContext context) noexcept = 0;
    virtual void Release(const AbilityResourceReservation &reservation, GameplayContext context) noexcept = 0;
    [[nodiscard]] virtual foundation::Result<void> ReconcileReservation(const AbilityResourceReservation &reservation,
                                                                        GameplayObjectRef owner,
                                                                        GameplayContext context)
    {
        (void)reservation;
        (void)owner;
        (void)context;
        return foundation::Result<void>::Failure(
            foundation::Error::Create("gameplay.ability.resource_reconcile_unsupported",
                                      "resource provider does not support reservation reconciliation"));
    }
};

struct AbilityOutput
{
    AbilityExecutionId execution{};
    GameplayTimePoint occurrence_at{};
    std::uint32_t output_index = 0;
    ActionTypeId action{};
    GameplayObjectRef owner{};
    AbilityTargetSet targets;
    std::int64_t magnitude_micro = 0;
    RegisteredAbilityPayload payload;
    GameplayContext context{};
};

enum class AbilityChangeKind
{
    Granted,
    Revoked,
    Started,
    Executed,
    Cancelled,
    Interrupted,
    Failed,
    CooldownStarted,
    CooldownFinished,
    EnabledChanged
};
struct AbilityChange
{
    std::uint64_t sequence = 0;
    AbilityChangeKind kind = AbilityChangeKind::Granted;
    GameplayObjectRef owner{};
    AbilityInstanceId ability{};
    AbilityExecutionId execution{};
    GameplayTimePoint time{};
    GameplayContext context{};
    TypeId reason{};
};
struct AbilityCooldownState
{
    GameplayObjectRef owner{};
    CooldownGroupId group{};
    GameplayTimePoint ends_at{};
};
struct AbilityChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::vector<AbilityChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct AbilitiesSnapshot
{
    std::vector<AbilityInstance> instances;
    std::vector<AbilityExecution> executions;
    std::vector<AbilityCooldownState> cooldowns;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot instance_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot execution_ids{};
    std::vector<AbilityChange> journal;
    std::uint64_t next_change_sequence = 1;

    std::uint64_t change_epoch = 1;
};
struct AbilitiesDiagnostics
{
    std::uint64_t instances = 0, activation_attempts = 0, activations = 0, failed = 0, interrupts = 0, cooldowns = 0,
                  outputs = 0;
};

class AbilityService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<AbilityService>;

  public:
    AbilityService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.abilities");
    }
    [[nodiscard]] foundation::Result<AbilityDefinitionId> RegisterDefinition(AbilityDefinition definition);
    void Freeze() noexcept
    {
        frozen_ = true;
    }
    [[nodiscard]] const AbilityDefinition *FindDefinition(AbilityDefinitionId id) const noexcept;
    void SetRequirementProvider(const IAbilityRequirementProvider *provider) noexcept
    {
        requirements_ = provider;
    }
    void SetResourceProvider(IAbilityResourceProvider *provider) noexcept
    {
        resources_ = provider;
    }
    void SetMaterializationProvider(const IAbilityMaterializationProvider *provider) noexcept
    {
        materialization_ = provider;
    }

    [[nodiscard]] foundation::Result<AbilityInstanceId> Grant(
        GameplayObjectRef owner, AbilityDefinitionId definition, GameplayObjectRef source = {},
        AbilityGrantPersistence persistence = AbilityGrantPersistence::Permanent, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Revoke(AbilityInstanceId instance, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SetAbilityEnabled(AbilityInstanceId instance, bool enabled,
                                                             GameplayContext context = {});
    [[nodiscard]] std::uint64_t RevokeBySource(GameplayObjectRef owner, GameplayObjectRef source,
                                               GameplayContext context = {});
    [[nodiscard]] const AbilityInstance *FindInstance(AbilityInstanceId id) const noexcept;
    [[nodiscard]] std::vector<AbilityInstance> GetAbilities(GameplayObjectRef owner) const;

    [[nodiscard]] AbilityAvailabilityResult CanActivate(AbilityInstanceId ability, const AbilityTargetSet &targets,
                                                        GameplayTimePoint now) const;
    [[nodiscard]] foundation::Result<AbilityExecutionId> BeginActivation(AbilityActivationRequest request);
    [[nodiscard]] foundation::Result<std::vector<AbilityOutput>> CompleteExecution(AbilityExecutionId execution,
                                                                                   GameplayTimePoint now);
    [[nodiscard]] foundation::Result<std::vector<AbilityOutput>> ChannelTick(AbilityExecutionId execution,
                                                                             GameplayTimePoint now,
                                                                             std::uint64_t occurrences = 1);
    [[nodiscard]] foundation::Result<void> Cancel(AbilityExecutionId execution, GameplayTimePoint now,
                                                  GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Interrupt(AbilityExecutionId execution, TypeId reason, GameplayTimePoint now,
                                                     GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> BindSchedule(AbilityExecutionId execution, ScheduleId schedule);
    [[nodiscard]] foundation::Result<void> NotifyScheduleDue(ScheduleId schedule, GameplayTimePoint now,
                                                             std::vector<AbilityOutput> &outputs);

    [[nodiscard]] GameplayDuration CooldownRemaining(GameplayObjectRef owner, CooldownGroupId group,
                                                     GameplayTimePoint now) const noexcept;
    void SweepCooldowns(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] bool NeedsResourceReconciliation() const noexcept
    {
        return resource_reconciliation_required_;
    }
    [[nodiscard]] foundation::Result<void> ReconcileRestoredReservations(GameplayContext context = {});
    [[nodiscard]] const AbilityExecution *FindExecution(AbilityExecutionId id) const noexcept;
    private:
        [[nodiscard]] std::vector<AbilityChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] AbilityChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] AbilityChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] AbilitiesSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(AbilitiesSnapshot snapshot);
    [[nodiscard]] AbilitiesDiagnostics GetDiagnostics() const noexcept;

  private:
    [[nodiscard]] foundation::Result<void> AcquireStartCosts(const AbilityDefinition &def, AbilityExecution &execution,
                                                               std::vector<AbilityResourceReservation> &pay_on_start);
    [[nodiscard]] foundation::Result<void> AcquireExecuteCosts(const AbilityDefinition &def,
                                                               AbilityExecution &execution,
                                                               std::vector<AbilityResourceReservation> &pay_on_execute);
    void CommitReservations(AbilityExecution &execution) noexcept;
    [[nodiscard]] AbilityAvailabilityResult ValidateTargets(const AbilityDefinition &definition, const AbilityInstance &instance,
                                                           const AbilityTargetSet &targets) const;
    [[nodiscard]] static bool IsTerminal(AbilityExecutionState state) noexcept;
    [[nodiscard]] AbilityAvailabilityResult CheckContinuity(const AbilityDefinition &definition,
                                                            const AbilityInstance &instance,
                                                            const AbilityExecution &execution,
                                                            GameplayTimePoint now) const;
    void ReleaseReservations(AbilityExecution &execution, GameplayContext context) noexcept;
    [[nodiscard]] foundation::Result<void> FinalizeExecution(AbilityExecutionId id, AbilityExecutionState terminal_state,
                                                               AbilityChangeKind change_kind, GameplayTimePoint now,
                                                               GameplayContext context, TypeId reason = {});
    void StartCooldown(const AbilityDefinition &def, GameplayObjectRef owner, GameplayTimePoint now,
                       GameplayContext context, AbilityInstanceId ability, AbilityExecutionId execution);
    [[nodiscard]] std::vector<AbilityOutput> BuildOutputs(const AbilityDefinition &def,
                                                          const AbilityExecution &execution,
                                                          GameplayTimePoint occurrence_at) const;
    [[nodiscard]] bool CanRecord(std::size_t count = 1) const noexcept;
    void Record(AbilityChange change) noexcept;

    std::unordered_map<AbilityDefinitionId, AbilityDefinition, IdHash> definitions_;
    std::unordered_map<AbilityInstanceId, AbilityInstance, IdHash> instances_;
    std::unordered_map<AbilityExecutionId, AbilityExecution, IdHash> executions_;
    std::vector<AbilityCooldownState> cooldowns_;
    MonotonicIdGenerator<GameplayObjectId> instance_ids_;
    MonotonicIdGenerator<GameplayObjectId> execution_ids_;
    const IAbilityRequirementProvider *requirements_ = nullptr;
    IAbilityResourceProvider *resources_ = nullptr;
    const IAbilityMaterializationProvider *materialization_ = nullptr;
    std::unordered_map<ScheduleId, AbilityExecutionId> schedule_to_execution_;
    bool frozen_ = false;
    bool resource_reconciliation_required_ = false;
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    static constexpr std::uint64_t kMaxChannelOccurrencesPerCall = 1024;
    std::vector<AbilityChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    std::vector<AbilityReservationId> reconciled_reservations_;
    AbilitiesDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::abilities
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/error.h`, `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `AbilityTargetPolicy`: `Self`, `SingleTarget`, `MultipleTargets`, `Point`, `Area`, `Direction`, `None`
- `AbilityTimingKind`: `Instant`, `CastTime`, `Channel`, `DelayedExecution`
- `AbilityCostPolicy`: `PayOnStart`, `ReserveThenCommit`, `PayOnExecute`, `PayPerChannelTick`
- `AbilityGrantPersistence`: `Permanent`, `Temporary`, `SourceBound`
- `AbilityContinuityPolicy`: `ActivationOnly`, `UntilExecute`, `ThroughoutExecution`
- `AbilityExecutionState`: `Casting`, `Channeling`, `Executing`, `Completed`, `Cancelled`, `Interrupted`, `Failed`
- `AbilityAvailability`: `Available`, `Unavailable`
- `AbilityChangeKind`: `Granted`, `Revoked`, `Started`, `Executed`, `Cancelled`, `Interrupted`, `Failed`, `CooldownStarted`, `CooldownFinished`, `EnabledChanged`

### Состав типов и операции

#### `AbilityDefinitionId` (строка заголовка 21)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr AbilityDefinitionId FromString(std::string_view n) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const AbilityDefinitionId &) const noexcept = default;
```

#### `AbilityInstanceId` (строка заголовка 34)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const AbilityInstanceId &) const noexcept = default;
```

#### `AbilityExecutionId` (строка заголовка 43)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const AbilityExecutionId &) const noexcept = default;
```

#### `CooldownGroupId` (строка заголовка 52)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr CooldownGroupId FromString(std::string_view n) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const CooldownGroupId &) const noexcept = default;
```

#### `AbilityResourceTypeId` (строка заголовка 65)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr AbilityResourceTypeId FromString(std::string_view n) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const AbilityResourceTypeId &) const noexcept = default;
```

#### `AbilityReservationId` (строка заголовка 78)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr auto operator<=>(const AbilityReservationId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 87)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 89)

#### `RegisteredAbilityPayload` (строка заголовка 98)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> bytes`

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] static RegisteredAbilityPayload FromTrivial(TypeId t, const T &v);
template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId t) const;
```

#### `T` (строка заголовка 102)

#### `T` (строка заголовка 111)

#### `AbilityCostDefinition` (строка заголовка 159)

Публичные поля и значения по умолчанию:

- `std::int64_t amount_micro = 0`
- `AbilityCostPolicy policy = AbilityCostPolicy::ReserveThenCommit`

#### `AbilityCooldownDefinition` (строка заголовка 165)

Публичные поля и значения по умолчанию:

- `bool starts_on_begin = false`

#### `AbilityTimingDefinition` (строка заголовка 171)

Публичные поля и значения по умолчанию:

- `AbilityTimingKind kind = AbilityTimingKind::Instant`

#### `AbilityOutputDefinition` (строка заголовка 178)

Публичные поля и значения по умолчанию:

- `std::int64_t magnitude_micro = 0`
- `RegisteredAbilityPayload payload`

#### `AbilityDefinition` (строка заголовка 184)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `AbilityTargetPolicy targeting = AbilityTargetPolicy::None`
- `std::vector<AbilityCostDefinition> costs`
- `std::vector<AbilityOutputDefinition> outputs`
- `bool requires_materialized_owner = false`
- `bool requires_materialized_target = false`
- `AbilityContinuityPolicy continuity = AbilityContinuityPolicy::ActivationOnly`

#### `AbilityTargetSet` (строка заголовка 199)

Публичные поля и значения по умолчанию:

- `std::vector<GameplayObjectRef> targets`
- `std::int64_t area_radius_micro = 0`

#### `AbilityInstance` (строка заголовка 208)

Публичные поля и значения по умолчанию:

- `AbilityGrantPersistence persistence = AbilityGrantPersistence::Permanent`
- `bool enabled = true`

#### `AbilityResourceReservation` (строка заголовка 229)

Публичные поля и значения по умолчанию:

- `RegisteredAbilityPayload provider_token`

#### `AbilityExecution` (строка заголовка 235)

Публичные поля и значения по умолчанию:

- `AbilityTargetSet targets`
- `AbilityExecutionState state = AbilityExecutionState::Executing`
- `std::vector<AbilityResourceReservation> reservations`

#### `AbilityActivationRequest` (строка заголовка 251)

Публичные поля и значения по умолчанию:

- `AbilityTargetSet targets`

#### `AbilityAvailabilityResult` (строка заголовка 263)

Публичные поля и значения по умолчанию:

- `AbilityAvailability availability = AbilityAvailability::Unavailable`

#### `IAbilityRequirementProvider` (строка заголовка 269)

Публичные сигнатуры операций:

```cpp
virtual ~IAbilityRequirementProvider() = default;
[[nodiscard]] virtual AbilityAvailabilityResult Check(const AbilityDefinition &definition, const AbilityInstance &instance, const AbilityTargetSet &targets, GameplayTimePoint now) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAbilityMaterializationProvider` (строка заголовка 278)

Публичные сигнатуры операций:

```cpp
virtual ~IAbilityMaterializationProvider() = default;
[[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef subject) const noexcept = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAbilityResourceProvider` (строка заголовка 284)

Публичные сигнатуры операций:

```cpp
virtual ~IAbilityResourceProvider() = default;
[[nodiscard]] virtual bool CanAfford(GameplayObjectRef owner, AbilityResourceTypeId type, std::int64_t amount_micro) const = 0;
[[nodiscard]] virtual foundation::Result<AbilityResourceReservation> Reserve(GameplayObjectRef owner, AbilityResourceTypeId type, std::int64_t amount_micro, GameplayContext context) = 0;
virtual void Commit(const AbilityResourceReservation &reservation, GameplayContext context) noexcept = 0;
virtual void Release(const AbilityResourceReservation &reservation, GameplayContext context) noexcept = 0;
[[nodiscard]] virtual foundation::Result<void> ReconcileReservation(const AbilityResourceReservation &reservation, GameplayObjectRef owner, GameplayContext context);
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `AbilityOutput` (строка заголовка 309)

Публичные поля и значения по умолчанию:

- `std::uint32_t output_index = 0`
- `AbilityTargetSet targets`
- `std::int64_t magnitude_micro = 0`
- `RegisteredAbilityPayload payload`

#### `AbilityChange` (строка заголовка 335)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `AbilityChangeKind kind = AbilityChangeKind::Granted`

#### `AbilityChangeBatch` (строка заголовка 352)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::vector<AbilityChange> changes`

#### `AbilitiesSnapshot` (строка заголовка 361)

Публичные поля и значения по умолчанию:

- `std::vector<AbilityInstance> instances`
- `std::vector<AbilityExecution> executions`
- `std::vector<AbilityCooldownState> cooldowns`
- `std::vector<AbilityChange> journal`
- `std::uint64_t next_change_sequence = 1`
- `std::uint64_t change_epoch = 1`

#### `AbilitiesDiagnostics` (строка заголовка 373)

Публичные поля и значения по умолчанию:

- `std::uint64_t instances = 0, activation_attempts = 0, activations = 0, failed = 0, interrupts = 0, cooldowns = 0, outputs = 0`

#### `AbilityService` (строка заголовка 379)

Внутреннее состояние (не часть публичного API): `std::unordered_map<AbilityDefinitionId, AbilityDefinition, IdHash> definitions_`, `std::unordered_map<AbilityInstanceId, AbilityInstance, IdHash> instances_`, `std::unordered_map<AbilityExecutionId, AbilityExecution, IdHash> executions_`, `std::vector<AbilityCooldownState> cooldowns_`, `MonotonicIdGenerator<GameplayObjectId> instance_ids_`, `MonotonicIdGenerator<GameplayObjectId> execution_ids_`, `std::unordered_map<ScheduleId, AbilityExecutionId> schedule_to_execution_`, `bool frozen_ = false`, `bool resource_reconciliation_required_ = false`.

Публичные сигнатуры операций:

```cpp
AbilityService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<AbilityDefinitionId> RegisterDefinition(AbilityDefinition definition);
void Freeze() noexcept;
[[nodiscard]] const AbilityDefinition *FindDefinition(AbilityDefinitionId id) const noexcept;
void SetRequirementProvider(const IAbilityRequirementProvider *provider) noexcept;
void SetResourceProvider(IAbilityResourceProvider *provider) noexcept;
void SetMaterializationProvider(const IAbilityMaterializationProvider *provider) noexcept;
[[nodiscard]] const AbilityInstance *FindInstance(AbilityInstanceId id) const noexcept;
[[nodiscard]] std::vector<AbilityInstance> GetAbilities(GameplayObjectRef owner) const;
[[nodiscard]] AbilityAvailabilityResult CanActivate(AbilityInstanceId ability, const AbilityTargetSet &targets, GameplayTimePoint now) const;
[[nodiscard]] foundation::Result<AbilityExecutionId> BeginActivation(AbilityActivationRequest request);
[[nodiscard]] foundation::Result<std::vector<AbilityOutput>> CompleteExecution(AbilityExecutionId execution, GameplayTimePoint now);
[[nodiscard]] foundation::Result<std::vector<AbilityOutput>> ChannelTick(AbilityExecutionId execution, GameplayTimePoint now, std::uint64_t occurrences = 1);
[[nodiscard]] foundation::Result<void> BindSchedule(AbilityExecutionId execution, ScheduleId schedule);
[[nodiscard]] foundation::Result<void> NotifyScheduleDue(ScheduleId schedule, GameplayTimePoint now, std::vector<AbilityOutput> &outputs);
[[nodiscard]] GameplayDuration CooldownRemaining(GameplayObjectRef owner, CooldownGroupId group, GameplayTimePoint now) const noexcept;
[[nodiscard]] bool NeedsResourceReconciliation() const noexcept;
[[nodiscard]] const AbilityExecution *FindExecution(AbilityExecutionId id) const noexcept;
[[nodiscard]] AbilityChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] AbilitiesSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(AbilitiesSnapshot snapshot);
[[nodiscard]] AbilitiesDiagnostics GetDiagnostics() const noexcept;
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
| `AbilityService` | `AbilityService::AbilityService` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:67` |
| `RegisterDefinition` | `AbilityService::RegisterDefinition` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:74` |
| `FindDefinition` | `AbilityService::FindDefinition` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:104` |
| `FindInstance` | `AbilityService::FindInstance` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:233` |
| `GetAbilities` | `AbilityService::GetAbilities` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:239` |
| `CooldownRemaining` | `AbilityService::CooldownRemaining` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:252` |
| `CanActivate` | `AbilityService::CanActivate` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:340` |
| `BeginActivation` | `AbilityService::BeginActivation` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:608` |
| `CompleteExecution` | `AbilityService::CompleteExecution` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:749` |
| `ChannelTick` | `AbilityService::ChannelTick` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:869` |
| `BindSchedule` | `AbilityService::BindSchedule` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1121` |
| `NotifyScheduleDue` | `AbilityService::NotifyScheduleDue` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1160` |
| `FindExecution` | `AbilityService::FindExecution` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1292` |
| `CaptureSnapshot` | `AbilityService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1315` |
| `RestoreSnapshot` | `AbilityService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1350` |
| `GetDiagnostics` | `AbilityService::GetDiagnostics` | `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1478` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetResourceProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:185`
- `RegisterDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:200`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:203`
- `BeginActivation`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:210`
- `CompleteExecution`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:213`
- `FindExecution`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:216`
- `CanActivate`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:218`
- `GetDiagnostics`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:223`
- `ReadChangesSince`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:233`
- `SetMaterializationProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:294`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:334`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp:345`

