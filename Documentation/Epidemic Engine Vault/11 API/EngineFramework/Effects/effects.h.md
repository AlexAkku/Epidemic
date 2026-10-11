---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h"
---

# Effects: effects.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`.

**Модуль:** [[06 EngineFramework/Модуль Effects|Effects]].
**Размер исходного файла:** 15906 байт.
**Подключение:** `#include <Epidemic/GameFramework/Effects/effects.h>`.

## Объявления типов (лексический индекс)
`EffectTypeId`, `EffectDefinitionId`, `EffectExecutionId`, `DeferredEffectId`, `EffectTypeIdHash`, `EffectDefinitionIdHash`, `DeferredEffectIdHash`, `RegisteredEffectPayload`, `EffectTargetSelector`, `EffectStepDefinition`, `EffectExecutionPolicy`, `EffectDefinition`, `EffectRequest`, `EffectOperation`, `EffectHandlerCapabilities`, `EffectPrepareDisposition`, `EffectPrepareResult`, `EffectCommitDisposition`, `EffectCommitResult`, `IEffectHandler`, `EffectTargetState`, `IEffectTargetStateProvider`, `EffectExecutionBudget`, `EffectOperationDisposition`, `EffectOperationResult`, `EffectBatchDisposition`, `EffectExecutionResult`, `EffectChangeKind`, `EffectChange`, `DeferredEffectPersistence`, `DeferredEffectRecord`, `EffectChangeBatch`, `EffectsSnapshot`, `EffectsDiagnostics`, `EffectService`, `HandlerEntry`, `PreparedOperation`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::effects
{
struct EffectTypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr EffectTypeId FromString(std::string_view name) noexcept { return EffectTypeId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const EffectTypeId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EffectTypeId&) const noexcept = default;
};

struct EffectDefinitionId
{
    TypeId value{};
    [[nodiscard]] static constexpr EffectDefinitionId FromString(std::string_view name) noexcept { return EffectDefinitionId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const EffectDefinitionId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EffectDefinitionId&) const noexcept = default;
};

struct EffectExecutionId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EffectExecutionId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EffectExecutionId&) const noexcept = default;
};

struct DeferredEffectId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const DeferredEffectId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const DeferredEffectId&) const noexcept = default;
};

struct EffectTypeIdHash
{
    [[nodiscard]] std::size_t operator()(EffectTypeId id) const noexcept { return std::hash<TypeId>{}(id.value); }
};
struct EffectDefinitionIdHash
{
    [[nodiscard]] std::size_t operator()(EffectDefinitionId id) const noexcept { return std::hash<TypeId>{}(id.value); }
};
struct DeferredEffectIdHash
{
    [[nodiscard]] std::size_t operator()(const DeferredEffectId& id) const noexcept { return std::hash<GameplayObjectId>{}(id.value); }
};

struct RegisteredEffectPayload
{
    TypeId type{};
    std::vector<std::byte> bytes;

    [[nodiscard]] bool Empty() const noexcept { return bytes.empty(); }

    template <typename T>
    [[nodiscard]] static RegisteredEffectPayload FromTrivial(TypeId type_id, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        RegisteredEffectPayload result;
        result.type = type_id;
        result.bytes.resize(sizeof(T));
        std::memcpy(result.bytes.data(), &value, sizeof(T));
        return result;
    }

    template <typename T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected_type) const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (type != expected_type || bytes.size() != sizeof(T))
        {
            return std::nullopt;
        }
        T value{};
        std::memcpy(&value, bytes.data(), sizeof(T));
        return value;
    }
};

enum class EffectTargetSelector
{
    AllTargets,
    FirstTarget,
};

struct EffectStepDefinition
{
    EffectTypeId type{};
    EffectTargetSelector selector = EffectTargetSelector::AllTargets;
    std::int64_t magnitude_micro = 0;
    RegisteredEffectPayload payload;
};

enum class EffectExecutionPolicy
{
    BestEffort,
    RequireAllPrepared,
};

struct EffectDefinition
{
    EffectDefinitionId id{};
    std::string canonical_name;
    std::vector<EffectStepDefinition> steps;
    EffectExecutionPolicy policy = EffectExecutionPolicy::BestEffort;
};

struct EffectRequest
{
    EffectDefinitionId definition{};
    GameplayObjectRef source{};
    GameplayObjectRef instigator{};
    std::vector<GameplayObjectRef> targets;
    std::int64_t scale_micro = 1'000'000;
    GameplayContext context{};
};

struct EffectOperation
{
    EffectExecutionId execution{};
    std::uint32_t wave = 0;
    std::uint32_t step_index = 0;
    std::uint64_t local_sequence = 0;
    EffectTypeId type{};
    GameplayObjectRef target{};
    std::int64_t magnitude_micro = 0;
    RegisteredEffectPayload payload;
    GameplayContext context{};
};

struct EffectHandlerCapabilities
{
    bool abstract_capable = true;
    bool requires_materialized = false;
    bool requires_runtime_projection = false;
    bool thread_safe_prepare = false;
};

enum class EffectPrepareDisposition
{
    Accepted,
    Rejected,
    NoOp,
    Unavailable,
    InvalidTarget,
    Unsupported,
};

struct EffectPrepareResult
{
    EffectPrepareDisposition disposition = EffectPrepareDisposition::Accepted;
    RegisteredEffectPayload commit_token;
};

enum class EffectCommitDisposition
{
    Applied,
    NoOp,
};

struct EffectCommitResult
{
    EffectCommitDisposition disposition = EffectCommitDisposition::Applied;
    std::vector<EffectOperation> derived_effects;
};

class IEffectHandler
{
  public:
    virtual ~IEffectHandler() = default;
    [[nodiscard]] virtual EffectTypeId Type() const noexcept = 0;
    [[nodiscard]] virtual EffectHandlerCapabilities Capabilities() const noexcept = 0;
    [[nodiscard]] virtual foundation::Result<EffectPrepareResult> Prepare(const EffectOperation& operation) const = 0;
    // Commit is the no-throw stage of the two-phase effect contract. Handlers must
    // perform all potentially throwing/fallible preparation in Prepare().
    [[nodiscard]] virtual foundation::Result<EffectCommitResult> Commit(
        const EffectOperation& operation,
        const RegisteredEffectPayload& commit_token) noexcept = 0;
};

struct EffectTargetState
{
    bool known = true;
    bool materialized = false;
    bool runtime_projection = false;
};

class IEffectTargetStateProvider
{
  public:
    virtual ~IEffectTargetStateProvider() = default;
    [[nodiscard]] virtual EffectTargetState Resolve(GameplayObjectRef target) const = 0;
};

struct EffectExecutionBudget
{
    std::uint64_t max_effects = 100000;
    std::uint32_t max_waves = 64;
    std::uint32_t max_derived_per_parent = 256;
};

enum class EffectOperationDisposition
{
    Applied,
    Rejected,
    NoOp,
    Unavailable,
    InvalidTarget,
    Unsupported,
    BudgetExceeded,
    Failed,
};

struct EffectOperationResult
{
    EffectOperation operation;
    EffectOperationDisposition disposition = EffectOperationDisposition::Failed;
};

enum class EffectBatchDisposition
{
    Succeeded,
    PartiallyApplied,
    Rejected,
    Failed,
};

struct EffectExecutionResult
{
    EffectExecutionId execution{};
    EffectBatchDisposition disposition = EffectBatchDisposition::Failed;
    std::vector<EffectOperationResult> operations;
    std::uint32_t waves = 0;
};

enum class EffectChangeKind
{
    ExecutionStarted,
    Applied,
    Rejected,
    Failed,
    ExecutionCompleted,
    BudgetExceeded,
    DeferredCreated,
    DeferredCancelled,
    DeferredExecuted,
};

struct EffectChange
{
    std::uint64_t sequence = 0;
    EffectChangeKind kind = EffectChangeKind::ExecutionStarted;
    EffectExecutionId execution{};
    EffectTypeId type{};
    GameplayObjectRef target{};
    DeferredEffectId deferred{};
    EffectOperationDisposition disposition = EffectOperationDisposition::Applied;
    GameplayContext context{};
    std::optional<ScheduleId> schedule{};
};

enum class DeferredEffectPersistence
{
    Session,
    Persistent,
};

struct DeferredEffectRecord
{
    DeferredEffectId id{};
    EffectRequest request;
    ClockId clock{};
    GameplayTimePoint due{};
    std::optional<ScheduleId> schedule{};
    DeferredEffectPersistence persistence = DeferredEffectPersistence::Session;
};

struct EffectChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::vector<EffectChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct EffectsSnapshot
{
    std::vector<DeferredEffectRecord> deferred;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot execution_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot deferred_ids{};

    std::uint64_t change_epoch = 1;
};

struct EffectsDiagnostics
{
    std::uint64_t executions = 0;
    std::uint64_t operations = 0;
    std::uint64_t targets = 0;
    std::uint64_t derived_effects = 0;
    std::uint64_t waves = 0;
    std::uint64_t rejections = 0;
    std::uint64_t budget_exhaustions = 0;
    std::uint64_t deferred_effects = 0;
};

class EffectService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<EffectService>;

  public:
    using PayloadValidator = std::function<bool(std::span<const std::byte>)>;

    EffectService();

    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.effects");
    }

    [[nodiscard]] foundation::Result<EffectTypeId> RegisterHandler(
        std::string_view canonical_name,
        std::shared_ptr<IEffectHandler> handler,
        TypeId payload_type = {},
        std::size_t max_payload_bytes = 0,
        PayloadValidator validator = {});
    [[nodiscard]] foundation::Result<EffectDefinitionId> RegisterDefinition(EffectDefinition definition);
    void SetTargetStateProvider(const IEffectTargetStateProvider* provider) noexcept { target_state_provider_ = provider; }
    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }

    [[nodiscard]] const EffectDefinition* FindDefinition(EffectDefinitionId id) const noexcept;

    [[nodiscard]] foundation::Result<EffectExecutionResult> Execute(
        EffectRequest request,
        EffectExecutionBudget budget = {});

    [[nodiscard]] foundation::Result<DeferredEffectId> Defer(
        EffectRequest request,
        ClockId clock,
        GameplayTimePoint due,
        DeferredEffectPersistence persistence = DeferredEffectPersistence::Session);
    [[nodiscard]] foundation::Result<void> BindDeferredSchedule(DeferredEffectId id, ScheduleId schedule);
    [[nodiscard]] foundation::Result<void> CancelDeferred(DeferredEffectId id, GameplayContext context = {});
    [[nodiscard]] std::uint64_t CancelDeferredTargeting(GameplayObjectRef target, GameplayContext context = {});
    [[nodiscard]] const DeferredEffectRecord* FindDeferred(DeferredEffectId id) const noexcept;
    [[nodiscard]] std::optional<DeferredEffectRecord> FindDeferredCopy(DeferredEffectId id) const noexcept;
    [[nodiscard]] std::vector<DeferredEffectRecord> AllDeferred() const;
    [[nodiscard]] std::vector<DeferredEffectRecord> UnscheduledDeferred() const;
    [[nodiscard]] foundation::Result<EffectRequest> PeekDeferredBySchedule(ScheduleId schedule, GameplayContext context = {}) const;
    [[nodiscard]] foundation::Result<void> AcknowledgeDeferredBySchedule(ScheduleId schedule, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ClearDeferredSchedule(DeferredEffectId id);
    [[nodiscard]] foundation::Result<EffectRequest> TakeDeferredBySchedule(ScheduleId schedule, GameplayContext context = {});

    private:
        [[nodiscard]] std::vector<EffectChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] EffectChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] EffectChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] std::uint64_t OldestChangeSequence() const noexcept;
    void PruneChangesBefore(std::uint64_t sequence);

    [[nodiscard]] EffectsSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(EffectsSnapshot snapshot);
    [[nodiscard]] EffectsDiagnostics GetDiagnostics() const noexcept;

  private:
    static constexpr std::size_t kChangeJournalCapacity = 4096;

    struct HandlerEntry
    {
        std::string canonical_name;
        std::shared_ptr<IEffectHandler> handler;
        TypeId payload_type{};
        std::size_t max_payload_bytes = 0;
        PayloadValidator validator;
    };

    struct PreparedOperation
    {
        EffectOperation operation;
        EffectPrepareResult prepared;
        EffectOperationDisposition preflight_disposition = EffectOperationDisposition::Failed;
        bool commit = false;
    };

    [[nodiscard]] foundation::Result<void> ValidatePayload(const HandlerEntry& entry, const RegisteredEffectPayload& payload) const;
    [[nodiscard]] foundation::Result<std::vector<EffectOperation>> ExpandRequest(
        const EffectRequest& request,
        EffectExecutionId execution) const;
    [[nodiscard]] EffectOperationDisposition ValidateCapabilities(const HandlerEntry& entry, GameplayObjectRef target) const;
    [[nodiscard]] foundation::Result<std::vector<PreparedOperation>> PrepareWave(
        std::vector<EffectOperation> operations,
        EffectExecutionPolicy policy);
    [[nodiscard]] EffectOperationDisposition MapPrepareDisposition(EffectPrepareDisposition disposition) const noexcept;
    void RecordChange(EffectChange change) noexcept;

    std::unordered_map<EffectTypeId, HandlerEntry, EffectTypeIdHash> handlers_;
    std::unordered_map<EffectDefinitionId, EffectDefinition, EffectDefinitionIdHash> definitions_;
    const IEffectTargetStateProvider* target_state_provider_ = nullptr;
    bool frozen_ = false;

    MonotonicIdGenerator<GameplayObjectId> execution_ids_;
    MonotonicIdGenerator<GameplayObjectId> deferred_ids_;
    std::unordered_map<DeferredEffectId, DeferredEffectRecord, DeferredEffectIdHash> deferred_;
    std::unordered_map<ScheduleId, DeferredEffectId> deferred_by_schedule_;

    std::vector<EffectChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;

    std::uint64_t executions_ = 0;
    std::uint64_t operations_ = 0;
    std::uint64_t targets_ = 0;
    std::uint64_t derived_effects_ = 0;
    std::uint64_t waves_ = 0;
    std::uint64_t rejections_ = 0;
    std::uint64_t budget_exhaustions_ = 0;
};
} // namespace epidemic::gameplay::effects
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `EffectTargetSelector`: `AllTargets`, `FirstTarget`
- `EffectExecutionPolicy`: `BestEffort`, `RequireAllPrepared`
- `EffectPrepareDisposition`: `Accepted`, `Rejected`, `NoOp`, `Unavailable`, `InvalidTarget`, `Unsupported`
- `EffectCommitDisposition`: `Applied`, `NoOp`
- `EffectOperationDisposition`: `Applied`, `Rejected`, `NoOp`, `Unavailable`, `InvalidTarget`, `Unsupported`, `BudgetExceeded`, `Failed`
- `EffectBatchDisposition`: `Succeeded`, `PartiallyApplied`, `Rejected`, `Failed`
- `EffectChangeKind`: `ExecutionStarted`, `Applied`, `Rejected`, `Failed`, `ExecutionCompleted`, `BudgetExceeded`, `DeferredCreated`, `DeferredCancelled`, `DeferredExecuted`
- `DeferredEffectPersistence`: `Session`, `Persistent`

### Состав типов и операции

#### `EffectTypeId` (строка заголовка 22)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr EffectTypeId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const EffectTypeId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EffectTypeId&) const noexcept = default;
```

#### `EffectDefinitionId` (строка заголовка 32)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr EffectDefinitionId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const EffectDefinitionId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EffectDefinitionId&) const noexcept = default;
```

#### `EffectExecutionId` (строка заголовка 42)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EffectExecutionId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EffectExecutionId&) const noexcept = default;
```

#### `DeferredEffectId` (строка заголовка 50)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const DeferredEffectId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const DeferredEffectId&) const noexcept = default;
```

#### `EffectTypeIdHash` (строка заголовка 58)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(EffectTypeId id) const noexcept;
```

#### `EffectDefinitionIdHash` (строка заголовка 62)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(EffectDefinitionId id) const noexcept;
```

#### `DeferredEffectIdHash` (строка заголовка 66)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const DeferredEffectId& id) const noexcept;
```

#### `RegisteredEffectPayload` (строка заголовка 71)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> bytes`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool Empty() const noexcept;
template <typename T> [[nodiscard]] static RegisteredEffectPayload FromTrivial(TypeId type_id, const T& value);
template <typename T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected_type) const;
```

#### `EffectStepDefinition` (строка заголовка 108)

Публичные поля и значения по умолчанию:

- `EffectTargetSelector selector = EffectTargetSelector::AllTargets`
- `std::int64_t magnitude_micro = 0`
- `RegisteredEffectPayload payload`

#### `EffectDefinition` (строка заголовка 122)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::vector<EffectStepDefinition> steps`
- `EffectExecutionPolicy policy = EffectExecutionPolicy::BestEffort`

#### `EffectRequest` (строка заголовка 130)

Публичные поля и значения по умолчанию:

- `std::vector<GameplayObjectRef> targets`
- `std::int64_t scale_micro = 1'000'000`

#### `EffectOperation` (строка заголовка 140)

Публичные поля и значения по умолчанию:

- `std::uint32_t wave = 0`
- `std::uint32_t step_index = 0`
- `std::uint64_t local_sequence = 0`
- `std::int64_t magnitude_micro = 0`
- `RegisteredEffectPayload payload`

#### `EffectHandlerCapabilities` (строка заголовка 153)

Публичные поля и значения по умолчанию:

- `bool abstract_capable = true`
- `bool requires_materialized = false`
- `bool requires_runtime_projection = false`
- `bool thread_safe_prepare = false`

#### `EffectPrepareResult` (строка заголовка 171)

Публичные поля и значения по умолчанию:

- `EffectPrepareDisposition disposition = EffectPrepareDisposition::Accepted`
- `RegisteredEffectPayload commit_token`

#### `EffectCommitResult` (строка заголовка 183)

Публичные поля и значения по умолчанию:

- `EffectCommitDisposition disposition = EffectCommitDisposition::Applied`
- `std::vector<EffectOperation> derived_effects`

#### `IEffectHandler` (строка заголовка 189)

Публичные сигнатуры операций:

```cpp
virtual ~IEffectHandler() = default;
[[nodiscard]] virtual EffectTypeId Type() const noexcept = 0;
[[nodiscard]] virtual EffectHandlerCapabilities Capabilities() const noexcept = 0;
[[nodiscard]] virtual foundation::Result<EffectPrepareResult> Prepare(const EffectOperation& operation) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `EffectTargetState` (строка заголовка 203)

Публичные поля и значения по умолчанию:

- `bool known = true`
- `bool materialized = false`
- `bool runtime_projection = false`

#### `IEffectTargetStateProvider` (строка заголовка 210)

Публичные сигнатуры операций:

```cpp
virtual ~IEffectTargetStateProvider() = default;
[[nodiscard]] virtual EffectTargetState Resolve(GameplayObjectRef target) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `EffectExecutionBudget` (строка заголовка 217)

Публичные поля и значения по умолчанию:

- `std::uint64_t max_effects = 100000`
- `std::uint32_t max_waves = 64`
- `std::uint32_t max_derived_per_parent = 256`

#### `EffectOperationResult` (строка заголовка 236)

Публичные поля и значения по умолчанию:

- `EffectOperation operation`
- `EffectOperationDisposition disposition = EffectOperationDisposition::Failed`

#### `EffectExecutionResult` (строка заголовка 250)

Публичные поля и значения по умолчанию:

- `EffectBatchDisposition disposition = EffectBatchDisposition::Failed`
- `std::vector<EffectOperationResult> operations`
- `std::uint32_t waves = 0`

#### `EffectChange` (строка заголовка 271)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `EffectChangeKind kind = EffectChangeKind::ExecutionStarted`
- `EffectOperationDisposition disposition = EffectOperationDisposition::Applied`

#### `DeferredEffectRecord` (строка заголовка 290)

Публичные поля и значения по умолчанию:

- `EffectRequest request`
- `DeferredEffectPersistence persistence = DeferredEffectPersistence::Session`

#### `EffectChangeBatch` (строка заголовка 300)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::vector<EffectChange> changes`

#### `EffectsSnapshot` (строка заголовка 310)

Публичные поля и значения по умолчанию:

- `std::vector<DeferredEffectRecord> deferred`
- `std::uint64_t change_epoch = 1`

#### `EffectsDiagnostics` (строка заголовка 319)

Публичные поля и значения по умолчанию:

- `std::uint64_t executions = 0`
- `std::uint64_t operations = 0`
- `std::uint64_t targets = 0`
- `std::uint64_t derived_effects = 0`
- `std::uint64_t waves = 0`
- `std::uint64_t rejections = 0`
- `std::uint64_t budget_exhaustions = 0`
- `std::uint64_t deferred_effects = 0`

#### `EffectService` (строка заголовка 331)

Внутреннее состояние (не часть публичного API): `static constexpr std::size_t kChangeJournalCapacity = 4096`, `std::unordered_map<EffectTypeId, HandlerEntry, EffectTypeIdHash> handlers_`, `std::unordered_map<EffectDefinitionId, EffectDefinition, EffectDefinitionIdHash> definitions_`, `const IEffectTargetStateProvider* target_state_provider_ = nullptr`, `bool frozen_ = false`, `MonotonicIdGenerator<GameplayObjectId> execution_ids_`, `MonotonicIdGenerator<GameplayObjectId> deferred_ids_`, `std::unordered_map<DeferredEffectId, DeferredEffectRecord, DeferredEffectIdHash> deferred_`, `std::unordered_map<ScheduleId, DeferredEffectId> deferred_by_schedule_`.

Публичные сигнатуры операций:

```cpp
EffectService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<EffectDefinitionId> RegisterDefinition(EffectDefinition definition);
void SetTargetStateProvider(const IEffectTargetStateProvider* provider) noexcept;
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const EffectDefinition* FindDefinition(EffectDefinitionId id) const noexcept;
[[nodiscard]] foundation::Result<DeferredEffectId> Defer( EffectRequest request, ClockId clock, GameplayTimePoint due, DeferredEffectPersistence persistence = DeferredEffectPersistence::Session);
[[nodiscard]] foundation::Result<void> BindDeferredSchedule(DeferredEffectId id, ScheduleId schedule);
[[nodiscard]] const DeferredEffectRecord* FindDeferred(DeferredEffectId id) const noexcept;
[[nodiscard]] std::optional<DeferredEffectRecord> FindDeferredCopy(DeferredEffectId id) const noexcept;
[[nodiscard]] std::vector<DeferredEffectRecord> AllDeferred() const;
[[nodiscard]] std::vector<DeferredEffectRecord> UnscheduledDeferred() const;
[[nodiscard]] foundation::Result<void> ClearDeferredSchedule(DeferredEffectId id);
[[nodiscard]] EffectChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] std::uint64_t OldestChangeSequence() const noexcept;
void PruneChangesBefore(std::uint64_t sequence);
[[nodiscard]] EffectsSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(EffectsSnapshot snapshot);
[[nodiscard]] EffectsDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `HandlerEntry` (строка заголовка 411)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::shared_ptr<IEffectHandler> handler`
- `std::size_t max_payload_bytes = 0`
- `PayloadValidator validator`

#### `PreparedOperation` (строка заголовка 420)

Публичные поля и значения по умолчанию:

- `EffectOperation operation`
- `EffectPrepareResult prepared`
- `EffectOperationDisposition preflight_disposition = EffectOperationDisposition::Failed`
- `bool commit = false`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `EffectService` | `EffectService::EffectService` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:102` |
| `RegisterDefinition` | `EffectService::RegisterDefinition` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:144` |
| `FindDefinition` | `EffectService::FindDefinition` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:188` |
| `Defer` | `EffectService::Defer` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:687` |
| `BindDeferredSchedule` | `EffectService::BindDeferredSchedule` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:730` |
| `FindDeferred` | `EffectService::FindDeferred` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:789` |
| `FindDeferredCopy` | `EffectService::FindDeferredCopy` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:795` |
| `AllDeferred` | `EffectService::AllDeferred` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:801` |
| `UnscheduledDeferred` | `EffectService::UnscheduledDeferred` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:813` |
| `ClearDeferredSchedule` | `EffectService::ClearDeferredSchedule` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:865` |
| `OldestChangeSequence` | `EffectService::OldestChangeSequence` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:987` |
| `PruneChangesBefore` | `EffectService::PruneChangesBefore` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:992` |
| `CaptureSnapshot` | `EffectService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1000` |
| `RestoreSnapshot` | `EffectService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1019` |
| `GetDiagnostics` | `EffectService::GetDiagnostics` | `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1081` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetTargetStateProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:229`
- `RegisterDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:236`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:241`
- `Defer`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:264`
- `BindDeferredSchedule`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:272`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:281`
- `GetDiagnostics`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:282`
- `LatestChangeCursor`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:283`
- `OldestChangeSequence`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:284`
- `AllDeferred`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:285`
- `UnscheduledDeferred`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:286`
- `IsFrozen`: `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp:399`

