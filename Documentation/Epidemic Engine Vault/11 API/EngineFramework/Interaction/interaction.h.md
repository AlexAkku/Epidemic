---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h"
---

# Interaction: interaction.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`.

**Модуль:** [[06 EngineFramework/Модуль Interaction|Interaction]].
**Размер исходного файла:** 12903 байт.
**Подключение:** `#include <Epidemic/GameFramework/Interaction/interaction.h>`.

## Объявления типов (лексический индекс)
`InteractionTypeId`, `InteractionProviderId`, `InteractionExecutionId`, `InteractionPoint`, `InteractionAvailability`, `InteractionExecutionMode`, `InteractionMaterializationPolicy`, `InteractionSessionState`, `InteractionPersistence`, `InteractionDefinition`, `InteractionContext`, `InteractionCandidate`, `InteractionSession`, `InteractionPlan`, `InteractionResult`, `InteractionSnapshot`, `InteractionChangeKind`, `InteractionChange`, `InteractionChangeBatch`, `InteractionDiagnostics`, `IInteractionProvider`, `IInteractionExecutor`, `IInteractionStateProvider`, `InteractionService`, `TypeHash`, `ProviderHash`, `ExecutionHash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <cstddef>
#include <limits>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::interaction
{
struct InteractionTypeId
{
    TypeId value{};
    static constexpr InteractionTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const InteractionTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const InteractionTypeId &) const noexcept = default;
};
struct InteractionProviderId
{
    TypeId value{};
    static constexpr InteractionProviderId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const InteractionProviderId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const InteractionProviderId &) const noexcept = default;
};
struct InteractionExecutionId
{
    GameplayObjectId value{};
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const InteractionExecutionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const InteractionExecutionId &) const noexcept = default;
};
using InteractionPriority = std::int32_t;
struct InteractionPoint
{
    std::int64_t x_mm = 0, y_mm = 0, z_mm = 0;
    [[nodiscard]] constexpr bool operator==(const InteractionPoint &) const noexcept = default;
};

enum class InteractionAvailability
{
    Available,
    Unavailable,
    Hidden,
    Deferred
};
enum class InteractionExecutionMode
{
    Instant,
    Timed,
    Continuous,
    Channelled
};
enum class InteractionMaterializationPolicy
{
    AbstractAllowed,
    RequiresMaterializedActor,
    RequiresMaterializedTarget,
    RequiresBothMaterialized
};
enum class InteractionSessionState
{
    Preparing,
    Active,
    Completed,
    Cancelled,
    Failed
};
enum class InteractionPersistence
{
    Transient,
    PersistentSession
};
struct InteractionDefinition
{
    InteractionTypeId type{};
    std::string canonical_name;
    GameplayTagSet tags;
    InteractionExecutionMode mode = InteractionExecutionMode::Instant;
    InteractionMaterializationPolicy materialization = InteractionMaterializationPolicy::RequiresBothMaterialized;
    InteractionPersistence persistence = InteractionPersistence::Transient;
    GameplayDuration duration{};
    ActionTypeId action{};
    std::vector<std::byte> payload;
};
struct InteractionContext
{
    GameplayObjectRef actor{};
    GameplayObjectRef target{};
    GameplayContext gameplay{};
    InteractionPoint point{};
    GameplayTagSet context_tags;
    Revision actor_revision{};
    Revision target_revision{};
};
struct InteractionCandidate
{
    InteractionTypeId type{};
    GameplayObjectRef actor{};
    GameplayObjectRef target{};
    InteractionProviderId provider{};
    InteractionPriority priority = 0;
    InteractionAvailability availability = InteractionAvailability::Available;
    TypeId reason{};
    std::vector<std::byte> payload;
};
struct InteractionSession
{
    InteractionExecutionId id{};
    InteractionTypeId type{};
    GameplayObjectRef actor{};
    GameplayObjectRef target{};
    InteractionSessionState state = InteractionSessionState::Preparing;
    GameplayTimePoint started_at{};
    std::optional<GameplayTimePoint> completes_at{};
    Revision actor_revision{};
    Revision target_revision{};
    std::vector<std::byte> payload;
    GameplayContext origin_context{};
    std::optional<ScheduleId> completion_schedule{};
    Revision revision{};
};
struct InteractionPlan
{
    InteractionCandidate candidate{};
    InteractionContext context{};
    Revision definition_revision{};
};
struct InteractionResult
{
    InteractionExecutionId execution{};
    InteractionSessionState state = InteractionSessionState::Failed;
    TypeId reason{};
};
struct InteractionSnapshot
{
    std::vector<InteractionSession> sessions;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};
enum class InteractionChangeKind
{
    Started,
    Completed,
    Cancelled,
    Failed
};
struct InteractionChange
{
    std::uint64_t sequence = 0;
    InteractionChangeKind kind = InteractionChangeKind::Started;
    InteractionExecutionId execution{};
    InteractionTypeId type{};
    GameplayObjectRef actor{}, target{};
    GameplayContext context{};
    Revision revision{};
    TypeId reason{};
    std::optional<ScheduleId> completion_schedule{};
};
struct InteractionChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::vector<InteractionChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct InteractionDiagnostics
{
    std::uint64_t candidate_requests = 0, candidates = 0, sessions = 0, completed = 0, cancelled = 0, failed = 0;
    std::uint64_t callback_failures = 0;
    std::uint64_t dropped_changes = 0;
};

class IInteractionProvider
{
  public:
    virtual ~IInteractionProvider() = default;
    [[nodiscard]] virtual InteractionProviderId Id() const noexcept = 0;
    [[nodiscard]] virtual std::vector<InteractionCandidate> Collect(const InteractionContext &context) const = 0;
};
class IInteractionExecutor
{
  public:
    virtual ~IInteractionExecutor() = default;
    [[nodiscard]] virtual foundation::Result<void> Validate(const InteractionPlan &plan) const = 0;
    // Commit is the irreversible stage. Implementations must prepare all fallible work in Validate/other preflight code.
    [[nodiscard]] virtual foundation::Result<void> Commit(const InteractionPlan &plan,
                                                          InteractionExecutionId execution) noexcept = 0;
};
class IInteractionStateProvider
{
  public:
    virtual ~IInteractionStateProvider() = default;
    [[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef object) const = 0;
    [[nodiscard]] virtual Revision RevisionOf(GameplayObjectRef object) const = 0;
};

class InteractionService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<InteractionService>;

  public:
    InteractionService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.interaction");
    }
    [[nodiscard]] foundation::Result<void> RegisterDefinition(InteractionDefinition definition);
    // Registered providers/executors are non-owning boot-time dependencies and must outlive this service.
    [[nodiscard]] foundation::Result<void> RegisterProvider(const IInteractionProvider &provider);
    [[nodiscard]] foundation::Result<void> RegisterExecutor(InteractionTypeId type, IInteractionExecutor &executor);
    void SetStateProvider(const IInteractionStateProvider *provider) noexcept
    {
        state_provider_ = provider;
    }
    void Freeze() noexcept
    {
        frozen_ = true;
    }
    [[nodiscard]] bool IsFrozen() const noexcept
    {
        return frozen_;
    }
    [[nodiscard]] const InteractionDefinition *FindDefinition(InteractionTypeId type) const noexcept;
    [[nodiscard]] std::vector<InteractionCandidate> GetAvailableInteractions(const InteractionContext &context) const;
    [[nodiscard]] foundation::Result<InteractionPlan> Prepare(const InteractionContext &context,
                                                              InteractionCandidate candidate) const;
    [[nodiscard]] foundation::Result<InteractionResult> Commit(const InteractionPlan &plan);
    [[nodiscard]] foundation::Result<void> BindCompletionSchedule(InteractionExecutionId id, ScheduleId schedule);
    [[nodiscard]] foundation::Result<void> Complete(InteractionExecutionId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Cancel(InteractionExecutionId id, TypeId reason = {},
                                                  GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SweepTimed(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] const InteractionSession *FindSession(InteractionExecutionId id) const noexcept;
    [[nodiscard]] std::optional<InteractionSession> FindSessionCopy(InteractionExecutionId id) const noexcept;
    [[nodiscard]] std::vector<InteractionSession> FindActive(GameplayObjectRef actor) const;
    // Persistent timed sessions restored from a snapshot intentionally have no scheduler handle.
    // Time integration must enumerate and bind these sessions before normal timed processing resumes.
    [[nodiscard]] std::vector<InteractionSession> SessionsRequiringSchedule() const;
    [[nodiscard]] InteractionSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(InteractionSnapshot snapshot);
    private:
        [[nodiscard]] std::vector<InteractionChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] InteractionChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] InteractionChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }
    [[nodiscard]] InteractionDiagnostics GetDiagnostics() const noexcept;

  private:
    struct TypeHash
    {
        std::size_t operator()(InteractionTypeId id) const noexcept
        {
            return std::hash<TypeId>{}(id.value);
        }
    };
    struct ProviderHash
    {
        std::size_t operator()(InteractionProviderId id) const noexcept
        {
            return std::hash<TypeId>{}(id.value);
        }
    };
    struct ExecutionHash
    {
        std::size_t operator()(InteractionExecutionId id) const noexcept
        {
            return std::hash<GameplayObjectId>{}(id.value);
        }
    };
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    void Record(InteractionChange c) noexcept;
    [[nodiscard]] bool MaterializationAllowed(const InteractionDefinition &d, const InteractionContext &c) const;
    std::unordered_map<InteractionTypeId, InteractionDefinition, TypeHash> definitions_;
    std::unordered_map<InteractionProviderId, const IInteractionProvider *, ProviderHash> providers_;
    std::unordered_map<InteractionTypeId, IInteractionExecutor *, TypeHash> executors_;
    std::unordered_map<InteractionExecutionId, InteractionSession, ExecutionHash> sessions_;
    std::unordered_map<ScheduleId, InteractionExecutionId> session_by_schedule_;
    const IInteractionStateProvider *state_provider_ = nullptr;
    MonotonicIdGenerator<GameplayObjectId> ids_;
    Revision revision_{};
    bool frozen_ = false;
    mutable std::uint64_t candidate_requests_ = 0, candidates_ = 0;
    std::uint64_t completed_ = 0, cancelled_ = 0, failed_ = 0;
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    std::deque<InteractionChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    mutable std::uint64_t callback_failures_ = 0;
    std::uint64_t dropped_changes_ = 0;
};
} // namespace epidemic::gameplay::interaction
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `InteractionAvailability`: `Available`, `Unavailable`, `Hidden`, `Deferred`
- `InteractionExecutionMode`: `Instant`, `Timed`, `Continuous`, `Channelled`
- `InteractionMaterializationPolicy`: `AbstractAllowed`, `RequiresMaterializedActor`, `RequiresMaterializedTarget`, `RequiresBothMaterialized`
- `InteractionSessionState`: `Preparing`, `Active`, `Completed`, `Cancelled`, `Failed`
- `InteractionPersistence`: `Transient`, `PersistentSession`
- `InteractionChangeKind`: `Started`, `Completed`, `Cancelled`, `Failed`

### Состав типов и операции

#### `InteractionTypeId` (строка заголовка 21)

Публичные сигнатуры операций:

```cpp
static constexpr InteractionTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const InteractionTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const InteractionTypeId &) const noexcept = default;
```

#### `InteractionProviderId` (строка заголовка 35)

Публичные сигнатуры операций:

```cpp
static constexpr InteractionProviderId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const InteractionProviderId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const InteractionProviderId &) const noexcept = default;
```

#### `InteractionExecutionId` (строка заголовка 49)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const InteractionExecutionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const InteractionExecutionId &) const noexcept = default;
```

#### `InteractionPoint` (строка заголовка 60)

Публичные поля и значения по умолчанию:

- `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const InteractionPoint &) const noexcept = default;
```

#### `InteractionDefinition` (строка заголовка 100)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `InteractionExecutionMode mode = InteractionExecutionMode::Instant`
- `InteractionMaterializationPolicy materialization = InteractionMaterializationPolicy::RequiresBothMaterialized`
- `InteractionPersistence persistence = InteractionPersistence::Transient`
- `std::vector<std::byte> payload`

#### `InteractionContext` (строка заголовка 112)

Публичные поля и значения по умолчанию:

- `GameplayTagSet context_tags`

#### `InteractionCandidate` (строка заголовка 122)

Публичные поля и значения по умолчанию:

- `InteractionPriority priority = 0`
- `InteractionAvailability availability = InteractionAvailability::Available`
- `std::vector<std::byte> payload`

#### `InteractionSession` (строка заголовка 133)

Публичные поля и значения по умолчанию:

- `InteractionSessionState state = InteractionSessionState::Preparing`
- `std::vector<std::byte> payload`

#### `InteractionResult` (строка заголовка 155)

Публичные поля и значения по умолчанию:

- `InteractionSessionState state = InteractionSessionState::Failed`

#### `InteractionSnapshot` (строка заголовка 161)

Публичные поля и значения по умолчанию:

- `std::vector<InteractionSession> sessions`
- `std::uint64_t change_epoch = 1`

#### `InteractionChange` (строка заголовка 176)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `InteractionChangeKind kind = InteractionChangeKind::Started`

#### `InteractionChangeBatch` (строка заголовка 188)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::vector<InteractionChange> changes`

#### `InteractionDiagnostics` (строка заголовка 198)

Публичные поля и значения по умолчанию:

- `std::uint64_t candidate_requests = 0, candidates = 0, sessions = 0, completed = 0, cancelled = 0, failed = 0`
- `std::uint64_t callback_failures = 0`
- `std::uint64_t dropped_changes = 0`

#### `IInteractionProvider` (строка заголовка 205)

Публичные сигнатуры операций:

```cpp
virtual ~IInteractionProvider() = default;
[[nodiscard]] virtual InteractionProviderId Id() const noexcept = 0;
[[nodiscard]] virtual std::vector<InteractionCandidate> Collect(const InteractionContext &context) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IInteractionExecutor` (строка заголовка 212)

Публичные сигнатуры операций:

```cpp
virtual ~IInteractionExecutor() = default;
[[nodiscard]] virtual foundation::Result<void> Validate(const InteractionPlan &plan) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IInteractionStateProvider` (строка заголовка 221)

Публичные сигнатуры операций:

```cpp
virtual ~IInteractionStateProvider() = default;
[[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef object) const = 0;
[[nodiscard]] virtual Revision RevisionOf(GameplayObjectRef object) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `InteractionService` (строка заголовка 229)

Внутреннее состояние (не часть публичного API): `std::unordered_map<InteractionTypeId, InteractionDefinition, TypeHash> definitions_`, `std::unordered_map<InteractionProviderId, const IInteractionProvider *, ProviderHash> providers`, `std::unordered_map<InteractionTypeId, IInteractionExecutor *, TypeHash> executors_`, `std::unordered_map<InteractionExecutionId, InteractionSession, ExecutionHash> sessions_`, `std::unordered_map<ScheduleId, InteractionExecutionId> session_by_schedule_`, `MonotonicIdGenerator<GameplayObjectId> ids_`, `bool frozen_ = false`, `mutable std::uint64_t candidate_requests_ = 0, candidates_ = 0`, `std::uint64_t completed_ = 0, cancelled_ = 0, failed_ = 0`.

Публичные сигнатуры операций:

```cpp
InteractionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterDefinition(InteractionDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterExecutor(InteractionTypeId type, IInteractionExecutor &executor);
void SetStateProvider(const IInteractionStateProvider *provider) noexcept;
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const InteractionDefinition *FindDefinition(InteractionTypeId type) const noexcept;
[[nodiscard]] std::vector<InteractionCandidate> GetAvailableInteractions(const InteractionContext &context) const;
[[nodiscard]] foundation::Result<InteractionPlan> Prepare(const InteractionContext &context, InteractionCandidate candidate) const;
[[nodiscard]] foundation::Result<InteractionResult> Commit(const InteractionPlan &plan);
[[nodiscard]] foundation::Result<void> BindCompletionSchedule(InteractionExecutionId id, ScheduleId schedule);
[[nodiscard]] const InteractionSession *FindSession(InteractionExecutionId id) const noexcept;
[[nodiscard]] std::optional<InteractionSession> FindSessionCopy(InteractionExecutionId id) const noexcept;
[[nodiscard]] std::vector<InteractionSession> FindActive(GameplayObjectRef actor) const;
[[nodiscard]] InteractionSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(InteractionSnapshot snapshot);
[[nodiscard]] InteractionChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
[[nodiscard]] InteractionDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `TypeHash` (строка заголовка 299)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(InteractionTypeId id) const noexcept;
```

#### `ProviderHash` (строка заголовка 306)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(InteractionProviderId id) const noexcept;
```

#### `ExecutionHash` (строка заголовка 313)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(InteractionExecutionId id) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `InteractionService` | `InteractionService::InteractionService` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:66` |
| `RegisterDefinition` | `InteractionService::RegisterDefinition` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:79` |
| `RegisterExecutor` | `InteractionService::RegisterExecutor` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:101` |
| `FindDefinition` | `InteractionService::FindDefinition` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:111` |
| `GetAvailableInteractions` | `InteractionService::GetAvailableInteractions` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:116` |
| `Prepare` | `InteractionService::Prepare` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:206` |
| `Commit` | `InteractionService::Commit` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:262` |
| `BindCompletionSchedule` | `InteractionService::BindCompletionSchedule` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:366` |
| `FindSession` | `InteractionService::FindSession` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:513` |
| `FindSessionCopy` | `InteractionService::FindSessionCopy` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:518` |
| `FindActive` | `InteractionService::FindActive` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:523` |
| `CaptureSnapshot` | `InteractionService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:550` |
| `RestoreSnapshot` | `InteractionService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:571` |
| `GetDiagnostics` | `InteractionService::GetDiagnostics` | `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:678` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:67`
- `RegisterExecutor`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:73`
- `SetStateProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:74`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:75`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:76`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:77`
- `CurrentRevision`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:84`
- `GetAvailableInteractions`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:86`
- `Prepare`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:92`
- `Commit`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:100`
- `FindActive`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_gameplay_tests.cpp:108`
- `FindSession`: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_time_integration_tests.cpp:98`

