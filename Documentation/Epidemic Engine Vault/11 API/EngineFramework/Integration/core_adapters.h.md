---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h"
---

# Integration: core_adapters.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h`.

**Модуль:** [[06 EngineFramework/Модуль Integration|Integration]].
**Размер исходного файла:** 20796 байт.
**Подключение:** `#include <Epidemic/GameFramework/Integration/core_adapters.h>`.

## Объявления типов (лексический индекс)
`FileSavePackageStore`, `FindFactsQuery`, `FindHistoryQuery`, `FactsQueryAdapter`, `RuntimeTimeAdapter`, `ScheduledTriggerDisposition`, `ScheduledTriggerActionDeliveryMode`, `ScheduledTriggerDispatcherPolicy`, `ScheduledTriggerDeliveryRecord`, `ScheduledTriggerDispatcherSnapshot`, `ScheduledTriggerPumpReport`, `ScheduledTriggerDispatcherSaveParticipant`, `ScheduledTriggerDispatcher`, `RegisteredHandler`, `TimeFactsAdapter`, `SaveGenerationId`, `SaveGenerationLimits`, `RuntimeSaveGenerationCapture`, `IRuntimeSaveGenerationPort`, `CallbackRuntimeSaveGenerationPort`, `SaveGenerationPackage`, `ISaveGenerationStore`, `FileSaveGenerationStore`, `SaveGenerationBarrierCallbacks`, `SaveGenerationBarrier`, `State`, `SaveGenerationCoordinator`, `IRuntimeGenerationRestoreStage`, `IRuntimeGenerationRestorePort`, `WholeRestorePhase`, `WholeRestoreCallbacks`, `WholeEngineRestoreCoordinator`, `ThreadIdentity`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/path.h"
#include "Epidemic/GameFramework/Facts/gameplay_facts.h"
#include "Epidemic/GameFramework/Queries/gameplay_queries.h"
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/Time/gameplay_time.h"
#include "Epidemic/Runtime/Time/time_runtime.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::integration
{
// Windows production package store. Externally synchronized, one writer per
// path. Temporary files share the destination directory. File data is flushed
// before write-through replacement; no hardware/power-loss guarantee is implied.
class FileSavePackageStore final : public savegame::ISavePackageStore
{
  public:
    explicit FileSavePackageStore(std::filesystem::path path, savegame::SaveWireLimits limits = {})
        : path_(foundation::Path(std::move(path)).Native()), limits_(limits) {}
    [[nodiscard]] foundation::Result<std::vector<std::byte>> Load() override;
    [[nodiscard]] foundation::Result<void> Commit(std::span<const std::byte> package) override;
  private:
    std::filesystem::path path_;
    savegame::SaveWireLimits limits_;
};

struct FindFactsQuery
{
    using ResultType = std::vector<facts::FactRecord>;
    FactTypeId type{};
    GameplayObjectRef subject{};
    GameplayObjectRef scope{};

    [[nodiscard]] static constexpr QueryTypeId Type() noexcept
    {
        return QueryTypeId::FromString("framework.query.facts.find");
    }
};

struct FindHistoryQuery
{
    using ResultType = std::vector<facts::EventRecord>;
    EventTypeId type{};
    GameplayObjectRef subject{};

    [[nodiscard]] static constexpr QueryTypeId Type() noexcept
    {
        return QueryTypeId::FromString("framework.query.history.find");
    }
};

class FactsQueryAdapter
{
  public:
    FactsQueryAdapter(facts::GameplayFactsService& facts_service, queries::GameplayQueryService& query_service)
        : facts_(facts_service), queries_(query_service)
    {
    }

    // Composition contract: register once before Queries::Freeze(). Repeated calls on the same adapter are idempotent.
    [[nodiscard]] foundation::Result<void> RegisterProviders();
    [[nodiscard]] bool IsRegistered() const noexcept { return registered_; }

  private:
    facts::GameplayFactsService& facts_;
    queries::GameplayQueryService& queries_;
    bool registered_ = false;
};

class RuntimeTimeAdapter
{
  public:
    RuntimeTimeAdapter(runtime::IGameClock& runtime_clock, time::GameplayTimeService& gameplay_time, ClockId clock)
        : runtime_clock_(runtime_clock), gameplay_time_(gameplay_time), clock_(clock)
    {
    }

    [[nodiscard]] foundation::Result<void> Synchronize();

  private:
    runtime::IGameClock& runtime_clock_;
    time::GameplayTimeService& gameplay_time_;
    ClockId clock_{};
};

using ScheduledTriggerHandlerId = TypeId;

enum class ScheduledTriggerDisposition
{
    Ack,
    Retry,
    DiscardTerminal,
};

enum class ScheduledTriggerActionDeliveryMode : std::uint8_t
{
    RequiresActionHandler = 1,
    ObserverOnly = 2,
};

struct ScheduledTriggerDispatcherPolicy
{
    std::size_t max_pending_deliveries = 16384;
    std::uint64_t max_delivery_attempts_per_pump = 16384;
};

struct ScheduledTriggerDeliveryRecord
{
    time::ScheduledTrigger trigger{};
    GameplayContext context{};
    bool action_declared = false;
    ScheduledTriggerActionDeliveryMode action_mode = ScheduledTriggerActionDeliveryMode::RequiresActionHandler;
    ScheduledTriggerHandlerId required_action_handler{};
    std::vector<ScheduledTriggerHandlerId> completed_handlers;
};

struct ScheduledTriggerDispatcherSnapshot
{
    // Frozen delivery registry captured with the inbox. Restore requires an exact match so a
    // previously unknown action cannot become complete merely because the runtime manifest changed.
    std::vector<ScheduledTriggerHandlerId> observer_handlers;
    std::vector<ScheduledTriggerDeliveryRecord> pending;
};

struct ScheduledTriggerPumpReport
{
    std::uint64_t collected = 0;
    std::uint64_t acknowledged = 0;
    std::uint64_t discarded_terminal = 0;
    std::uint64_t retry_requests = 0;
    std::uint64_t handler_failures = 0;
    std::uint64_t unhandled = 0;
    std::uint64_t pending = 0;
};

class ScheduledTriggerDispatcherSaveParticipant;

class ScheduledTriggerDispatcher
{
  public:
    using Handler = std::function<foundation::Result<ScheduledTriggerDisposition>(const time::ScheduledTrigger&, const GameplayContext&)>;

    ScheduledTriggerDispatcher(
        time::GameplayTimeService& time_service,
        ScheduledTriggerDispatcherPolicy policy = {})
        : time_(time_service), policy_(policy)
    {
    }

    // Observers receive every collected trigger. Action handlers receive only their registered action.
    // Handler registrations are immutable after Freeze(). IDs are stable delivery-leg identities used by snapshots/retries.
    [[nodiscard]] foundation::Result<void> RegisterObserver(ScheduledTriggerHandlerId id, int priority, Handler handler);
    [[nodiscard]] foundation::Result<void> DeclareRequiresActionHandler(ActionTypeId action);
    [[nodiscard]] foundation::Result<void> DeclareObserverOnlyAction(ActionTypeId action);
    [[nodiscard]] foundation::Result<void> RegisterActionHandler(ActionTypeId action, ScheduledTriggerHandlerId id, Handler handler);
    foundation::Result<void> Freeze();
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }

    // This is the only Integration-level API that destructively collects from GameplayTimeService.
    // Collected triggers are first stored in pending delivery state before any handler is invoked.
    [[nodiscard]] foundation::Result<std::uint64_t> CollectDue(
        ClockId clock,
        GameplayContext context,
        time::SchedulerBudget budget = {});

    [[nodiscard]] ScheduledTriggerPumpReport DispatchPending(
        std::uint64_t max_attempts = 0);

    [[nodiscard]] foundation::Result<ScheduledTriggerPumpReport> Pump(
        ClockId clock,
        GameplayContext context,
        time::SchedulerBudget budget = {});

    [[nodiscard]] ScheduledTriggerDispatcherSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(ScheduledTriggerDispatcherSnapshot snapshot);

    [[nodiscard]] std::size_t PendingCount() const noexcept { return pending_.size(); }

  private:
    struct RegisteredHandler
    {
        ScheduledTriggerHandlerId id{};
        int priority = 0;
        Handler callback;
    };

    [[nodiscard]] bool HandlerExists(ScheduledTriggerHandlerId id) const noexcept;
    [[nodiscard]] const RegisteredHandler* FindHandler(ScheduledTriggerHandlerId id) const noexcept;
    [[nodiscard]] foundation::Result<void> DeclareActionDelivery(
        ActionTypeId action, ScheduledTriggerActionDeliveryMode mode);
    [[nodiscard]] std::vector<ScheduledTriggerHandlerId> RequiredHandlersFor(
        const ScheduledTriggerDeliveryRecord& delivery) const;
    [[nodiscard]] foundation::Result<void> ValidateSnapshot(const ScheduledTriggerDispatcherSnapshot& snapshot) const;
    void CommitRestoredSnapshot(ScheduledTriggerDispatcherSnapshot&& snapshot) noexcept;

    time::GameplayTimeService& time_;
    ScheduledTriggerDispatcherPolicy policy_{};
    std::vector<RegisteredHandler> observers_;
    std::unordered_map<ActionTypeId, ScheduledTriggerActionDeliveryMode> action_manifest_;
    std::unordered_map<ActionTypeId, RegisteredHandler> action_handlers_;
    std::vector<ScheduledTriggerDeliveryRecord> pending_;
    bool frozen_ = false;

    friend class ScheduledTriggerDispatcherSaveParticipant;
};

class ScheduledTriggerDispatcherSaveParticipant final : public savegame::ISaveParticipant
{
  public:
    explicit ScheduledTriggerDispatcherSaveParticipant(ScheduledTriggerDispatcher& dispatcher) noexcept
        : dispatcher_(dispatcher)
    {
    }

    [[nodiscard]] savegame::SaveParticipantId Id() const noexcept override;
    [[nodiscard]] savegame::SaveSchemaVersion SchemaVersion() const noexcept override { return 1; }
    [[nodiscard]] std::vector<savegame::SaveParticipantId> Dependencies() const override { return {}; }
    [[nodiscard]] foundation::Result<savegame::SaveSection> CaptureSnapshot(
        const savegame::SaveContext& context) const override;
    [[nodiscard]] foundation::Result<void> ValidateSnapshot(
        const savegame::SaveSection& section, const savegame::RestoreContext& context) const override;
    [[nodiscard]] foundation::Result<std::unique_ptr<savegame::IRestoreStage>> StageRestore(
        const savegame::SaveSection& section, const savegame::RestoreContext& context) override;
    void CommitRestore(savegame::IRestoreStage& stage) noexcept override;

  private:
    ScheduledTriggerDispatcher& dispatcher_;
};

class TimeFactsAdapter
{
  public:
    TimeFactsAdapter(facts::GameplayFactsService& facts_service)
        : facts_(facts_service)
    {
    }

    // Register once before Facts::Freeze(). Repeated calls on the same adapter are idempotent.
    [[nodiscard]] foundation::Result<EventTypeId> RegisterContracts();

    // Registers as a dispatcher observer. It no longer calls Time::CollectDue directly.
    [[nodiscard]] foundation::Result<void> RegisterWithDispatcher(
        ScheduledTriggerDispatcher& dispatcher,
        int priority = 0);

    [[nodiscard]] foundation::Result<ScheduledTriggerDisposition> PublishTrigger(
        const time::ScheduledTrigger& trigger,
        const GameplayContext& context);

    [[nodiscard]] foundation::Result<std::uint64_t> ExpireTimedFacts(GameplayContext context);

    [[nodiscard]] EventTypeId ScheduledDueEventType() const noexcept { return scheduled_due_event_type_; }

  private:
    facts::GameplayFactsService& facts_;
    EventTypeId scheduled_due_event_type_{};
    bool dispatcher_registered_ = false;
};
} // namespace epidemic::gameplay::integration

// BEGIN GENERATED GOAL5 PARTICIPANTS
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/Facts/gameplay_facts.h"
#include "Epidemic/GameFramework/Time/gameplay_time.h"
namespace epidemic::gameplay::savegame
{
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateGameplayFactsServiceSaveParticipant(epidemic::gameplay::facts::GameplayFactsService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateGameplayTimeServiceSaveParticipant(epidemic::gameplay::time::GameplayTimeService& owner, SaveRestoreWorkspace& workspace);
}
// END GENERATED GOAL5 PARTICIPANTS

// BEGIN GOAL5 SAVE GENERATION COORDINATION
namespace epidemic::gameplay::integration
{
// Stable identity for one whole-engine save generation. Zero is never valid.
struct SaveGenerationId
{
    std::uint64_t value = 0;

    [[nodiscard]] static constexpr SaveGenerationId FromRaw(std::uint64_t raw) noexcept { return {raw}; }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const SaveGenerationId&) const noexcept = default;
};

struct SaveGenerationLimits
{
    std::size_t max_framework_savegame_bytes = 600ull * 1024ull * 1024ull;
    std::size_t max_runtime_time_bytes = 64ull * 1024ull * 1024ull;
    std::size_t max_runtime_persistence_bytes = 64ull * 1024ull * 1024ull;
    std::size_t max_wire_bytes = 768ull * 1024ull * 1024ull;
};

struct RuntimeSaveGenerationCapture
{
    std::vector<std::byte> time_package;
    std::vector<std::byte> persistence_package;
};

// Narrow composition boundary. The implementation lives where Runtime services
// and Framework Integration are both owned. Runtime modules never depend on this interface.
class IRuntimeSaveGenerationPort
{
  public:
    virtual ~IRuntimeSaveGenerationPort() = default;
    [[nodiscard]] virtual foundation::Result<RuntimeSaveGenerationCapture> Capture(
        SaveGenerationId generation) = 0;
};

// Production composition adapter for Runtime capture without a Framework ->
// Runtime Persistence/Support link. Callbacks must return canonical validated
// Runtime v1 package bytes while the outer generation lease is held.
class CallbackRuntimeSaveGenerationPort final : public IRuntimeSaveGenerationPort
{
  public:
    using PlaneCapture = std::function<foundation::Result<std::vector<std::byte>>(SaveGenerationId)>;

    CallbackRuntimeSaveGenerationPort(PlaneCapture time_capture, PlaneCapture persistence_capture)
        : time_capture_(std::move(time_capture)), persistence_capture_(std::move(persistence_capture)) {}

    [[nodiscard]] foundation::Result<RuntimeSaveGenerationCapture> Capture(
        SaveGenerationId generation) override;

  private:
    PlaneCapture time_capture_;
    PlaneCapture persistence_capture_;
};

struct SaveGenerationPackage
{
    SaveGenerationId generation{};
    std::vector<std::byte> framework_savegame;
    std::vector<std::byte> runtime_time;
    std::vector<std::byte> runtime_persistence;
};

[[nodiscard]] foundation::Result<std::vector<std::byte>> EncodeSaveGenerationPackage(
    const SaveGenerationPackage& package, SaveGenerationLimits limits = {});
[[nodiscard]] foundation::Result<SaveGenerationPackage> DecodeSaveGenerationPackage(
    std::span<const std::byte> bytes, SaveGenerationLimits limits = {});

// Semantic current-generation store. Missing current data is a successful empty
// optional and therefore allocates generation 1. A malformed existing package is
// an error and must never silently restart the lineage.
class ISaveGenerationStore
{
  public:
    virtual ~ISaveGenerationStore() = default;
    [[nodiscard]] virtual foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() = 0;
    [[nodiscard]] virtual foundation::Result<void> CommitCurrent(std::span<const std::byte> package) = 0;
};

class FileSaveGenerationStore final : public ISaveGenerationStore
{
  public:
    explicit FileSaveGenerationStore(std::filesystem::path path, SaveGenerationLimits limits = {})
        : path_(foundation::Path(std::move(path)).Native()), limits_(limits) {}
    [[nodiscard]] foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() override;
    [[nodiscard]] foundation::Result<void> CommitCurrent(std::span<const std::byte> package) override;

  private:
    std::filesystem::path path_;
    SaveGenerationLimits limits_;
};

struct SaveGenerationBarrierCallbacks
{
    using LeaseFactory = std::function<foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>() >;

    // The factory establishes quiescence before preflight executes. Preflight
    // then checks Streaming/RuntimeBridge and other transient blockers while the
    // safe point is stable.
    LeaseFactory acquire_capture_quiescence;
    std::function<foundation::Result<void>()> capture_preflight;
    LeaseFactory acquire_restore_quiescence;
};

// Production SaveGame barrier used by the outer generation coordinator. A
// SaveGameOrchestrator capture is allowed only as a nested acquisition from the
// same thread while one outer SaveGenerationId lease is active.
class SaveGenerationBarrier final : public savegame::ISaveBarrier
{
  public:
    explicit SaveGenerationBarrier(SaveGenerationBarrierCallbacks callbacks);
    ~SaveGenerationBarrier() override;
    SaveGenerationBarrier(const SaveGenerationBarrier&) = delete;
    SaveGenerationBarrier& operator=(const SaveGenerationBarrier&) = delete;

    [[nodiscard]] foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> AcquireGenerationLease(
        SaveGenerationId generation);
    [[nodiscard]] foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> AcquireCaptureLease() override;
    [[nodiscard]] foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> AcquireRestoreLease() override;
    [[nodiscard]] SaveGenerationId ActiveGeneration() const noexcept;
    [[nodiscard]] bool IsCaptureActive() const noexcept;

  private:
    struct State;
    std::shared_ptr<State> state_;
};

class SaveGenerationCoordinator
{
  public:
    SaveGenerationCoordinator(
        savegame::SaveGameOrchestrator& framework,
        IRuntimeSaveGenerationPort& runtime,
        SaveGenerationBarrier& barrier,
        ISaveGenerationStore& store,
        SaveGenerationLimits limits = {},
        savegame::SaveWireLimits framework_limits = {}) noexcept;

    [[nodiscard]] foundation::Result<SaveGenerationId> CaptureAndCommit(
        const savegame::SaveContext& context);

  private:
    savegame::SaveGameOrchestrator& framework_;
    IRuntimeSaveGenerationPort& runtime_;
    SaveGenerationBarrier& barrier_;
    ISaveGenerationStore& store_;
    SaveGenerationLimits limits_;
    savegame::SaveWireLimits framework_limits_;
};
} // namespace epidemic::gameplay::integration
// END GOAL5 SAVE GENERATION COORDINATION

namespace epidemic::gameplay::integration
{
// Preparation owns all Runtime allocation/validation. Commit only publishes
// prepared semantic state; external/transient work belongs to reconciliation.
class IRuntimeGenerationRestoreStage
{
  public:
    virtual ~IRuntimeGenerationRestoreStage() = default;
    virtual void Commit() noexcept = 0;
};

class IRuntimeGenerationRestorePort
{
  public:
    virtual ~IRuntimeGenerationRestorePort() = default;
    [[nodiscard]] virtual foundation::Result<std::unique_ptr<IRuntimeGenerationRestoreStage>> Prepare(
        const SaveGenerationPackage& package) = 0;
};

enum class WholeRestorePhase
{
    AwaitLoad,
    Precommit,
    CommittedSemantic,
    RebuildTransient,
    ReconcileExternal,
    MaterializeRuntime,
    ReconciliationRequired,
    ReadyToResume,
};

struct WholeRestoreCallbacks
{
    // Each step must retain its own accepted-work cursor on partial failure.
    // Successful steps are not replayed by the coordinator on retry.
    std::function<foundation::Result<void>()> rebuild_transient;
    std::function<foundation::Result<void>()> reconcile_external;
    std::function<foundation::Result<void>()> materialize_runtime;
};

// Owner-thread only, one fresh-composition restore cycle. Framework dependency
// order/staging/no-fail publication remain owned by SaveGameOrchestrator.
// Callers must gate simulation and Time advance on CanResume().
class WholeEngineRestoreCoordinator
{
  public:
    WholeEngineRestoreCoordinator(savegame::SaveGameOrchestrator& framework,
        IRuntimeGenerationRestorePort& runtime, SaveGenerationBarrier& barrier,
        ISaveGenerationStore& store, WholeRestoreCallbacks callbacks,
        SaveGenerationLimits limits = {}, savegame::SaveWireLimits framework_limits = {});
    ~WholeEngineRestoreCoordinator();
    WholeEngineRestoreCoordinator(const WholeEngineRestoreCoordinator&) = delete;
    WholeEngineRestoreCoordinator& operator=(const WholeEngineRestoreCoordinator&) = delete;

    [[nodiscard]] foundation::Result<SaveGenerationId> LoadAndRestore(const savegame::RestoreContext& context);
    [[nodiscard]] foundation::Result<void> RetryReconciliation();
    [[nodiscard]] bool CanResume() const noexcept { return phase_ == WholeRestorePhase::ReadyToResume && !lease_; }
    [[nodiscard]] WholeRestorePhase Phase() const noexcept { return phase_; }
    [[nodiscard]] SaveGenerationId RestoredGeneration() const noexcept { return restored_generation_; }

  private:
    [[nodiscard]] foundation::Result<void> ContinueReconciliation();
    [[nodiscard]] bool OnOwnerThread() const noexcept;
    savegame::SaveGameOrchestrator& framework_;
    IRuntimeGenerationRestorePort& runtime_;
    SaveGenerationBarrier& barrier_;
    ISaveGenerationStore& store_;
    WholeRestoreCallbacks callbacks_;
    SaveGenerationLimits limits_;
    savegame::SaveWireLimits framework_limits_;
    std::unique_ptr<savegame::ISaveBarrierLease> lease_;
    WholeRestorePhase phase_ = WholeRestorePhase::AwaitLoad;
    SaveGenerationId restored_generation_{};
    std::size_t completed_steps_ = 0;
    bool operation_active_ = false;
    struct ThreadIdentity;
    std::unique_ptr<ThreadIdentity> owner_thread_;
};
} // namespace epidemic::gameplay::integration
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Foundation/path.h`, `Epidemic/GameFramework/Facts/gameplay_facts.h`, `Epidemic/GameFramework/Queries/gameplay_queries.h`, `Epidemic/GameFramework/SaveGame/save_game.h`, `Epidemic/GameFramework/Time/gameplay_time.h`, `Epidemic/Runtime/Time/time_runtime.h`, `Epidemic/GameFramework/SaveGame/save_game.h`, `Epidemic/GameFramework/Facts/gameplay_facts.h`, `Epidemic/GameFramework/Time/gameplay_time.h`.

### Перечисления и допустимые категории

- `ScheduledTriggerDisposition`: `Ack`, `Retry`, `DiscardTerminal`
- `ScheduledTriggerActionDeliveryMode`: `RequiresActionHandler`, `ObserverOnly`
- `WholeRestorePhase`: `AwaitLoad`, `Precommit`, `CommittedSemantic`, `RebuildTransient`, `ReconcileExternal`, `MaterializeRuntime`, `ReconciliationRequired`, `ReadyToResume`

### Состав типов и операции

#### `FileSavePackageStore` (строка заголовка 24)

Внутреннее состояние (не часть публичного API): `std::filesystem::path path_`, `savegame::SaveWireLimits limits_`.

Публичные сигнатуры операций:

```cpp
) : path_(foundation::Path(std::move(path)).Native()), limits_(limits);
[[nodiscard]] foundation::Result<std::vector<std::byte>> Load() override;
[[nodiscard]] foundation::Result<void> Commit(std::span<const std::byte> package) override;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `FindFactsQuery` (строка заголовка 36)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `FindHistoryQuery` (строка заголовка 49)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `FactsQueryAdapter` (строка заголовка 61)

Публичные сигнатуры операций:

```cpp
FactsQueryAdapter(facts::GameplayFactsService& facts_service, queries::GameplayQueryService& query_service) : facts_(facts_service), queries_(query_service);
[[nodiscard]] bool IsRegistered() const noexcept;
```

#### `RuntimeTimeAdapter` (строка заголовка 79)

Публичные сигнатуры операций:

```cpp
RuntimeTimeAdapter(runtime::IGameClock& runtime_clock, time::GameplayTimeService& gameplay_time, ClockId clock) : runtime_clock_(runtime_clock), gameplay_time_(gameplay_time), clock_(clock);
[[nodiscard]] foundation::Result<void> Synchronize();
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `ScheduledTriggerDispatcherPolicy` (строка заголовка 110)

Публичные поля и значения по умолчанию:

- `std::size_t max_pending_deliveries = 16384`
- `std::uint64_t max_delivery_attempts_per_pump = 16384`

#### `ScheduledTriggerDeliveryRecord` (строка заголовка 116)

Публичные поля и значения по умолчанию:

- `bool action_declared = false`
- `ScheduledTriggerActionDeliveryMode action_mode = ScheduledTriggerActionDeliveryMode::RequiresActionHandler`
- `std::vector<ScheduledTriggerHandlerId> completed_handlers`

#### `ScheduledTriggerDispatcherSnapshot` (строка заголовка 126)

Публичные поля и значения по умолчанию:

- `std::vector<ScheduledTriggerDeliveryRecord> pending`

#### `ScheduledTriggerPumpReport` (строка заголовка 134)

Публичные поля и значения по умолчанию:

- `std::uint64_t collected = 0`
- `std::uint64_t acknowledged = 0`
- `std::uint64_t discarded_terminal = 0`
- `std::uint64_t retry_requests = 0`
- `std::uint64_t handler_failures = 0`
- `std::uint64_t unhandled = 0`
- `std::uint64_t pending = 0`

#### `ScheduledTriggerDispatcher` (строка заголовка 147)

Публичные сигнатуры операций:

```cpp
) : time_(time_service), policy_(policy);
[[nodiscard]] foundation::Result<void> DeclareRequiresActionHandler(ActionTypeId action);
[[nodiscard]] foundation::Result<void> DeclareObserverOnlyAction(ActionTypeId action);
[[nodiscard]] foundation::Result<void> RegisterActionHandler(ActionTypeId action, ScheduledTriggerHandlerId id, Handler handler);
foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] ScheduledTriggerPumpReport DispatchPending( std::uint64_t max_attempts = 0);
[[nodiscard]] ScheduledTriggerDispatcherSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(ScheduledTriggerDispatcherSnapshot snapshot);
[[nodiscard]] std::size_t PendingCount() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `RegisteredHandler` (строка заголовка 189)

Публичные поля и значения по умолчанию:

- `int priority = 0`
- `Handler callback`

#### `ScheduledTriggerDispatcherSaveParticipant` (строка заголовка 216)

Публичные сигнатуры операций:

```cpp
explicit ScheduledTriggerDispatcherSaveParticipant(ScheduledTriggerDispatcher& dispatcher) noexcept : dispatcher_(dispatcher);
[[nodiscard]] savegame::SaveParticipantId Id() const noexcept override;
[[nodiscard]] savegame::SaveSchemaVersion SchemaVersion() const noexcept override;
[[nodiscard]] std::vector<savegame::SaveParticipantId> Dependencies() const override;
[[nodiscard]] foundation::Result<savegame::SaveSection> CaptureSnapshot( const savegame::SaveContext& context) const override;
[[nodiscard]] foundation::Result<void> ValidateSnapshot( const savegame::SaveSection& section, const savegame::RestoreContext& context) const override;
[[nodiscard]] foundation::Result<std::unique_ptr<savegame::IRestoreStage>> StageRestore( const savegame::SaveSection& section, const savegame::RestoreContext& context) override;
void CommitRestore(savegame::IRestoreStage& stage) noexcept override;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `TimeFactsAdapter` (строка заголовка 239)

Публичные сигнатуры операций:

```cpp
TimeFactsAdapter(facts::GameplayFactsService& facts_service) : facts_(facts_service);
[[nodiscard]] foundation::Result<ScheduledTriggerDisposition> PublishTrigger( const time::ScheduledTrigger& trigger, const GameplayContext& context);
[[nodiscard]] foundation::Result<std::uint64_t> ExpireTimedFacts(GameplayContext context);
[[nodiscard]] EventTypeId ScheduledDueEventType() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `SaveGenerationId` (строка заголовка 285)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr SaveGenerationId FromRaw(std::uint64_t raw) noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const SaveGenerationId&) const noexcept = default;
```

#### `SaveGenerationLimits` (строка заголовка 295)

Публичные поля и значения по умолчанию:

- `std::size_t max_framework_savegame_bytes = 600ull * 1024ull * 1024ull`
- `std::size_t max_runtime_time_bytes = 64ull * 1024ull * 1024ull`
- `std::size_t max_runtime_persistence_bytes = 64ull * 1024ull * 1024ull`
- `std::size_t max_wire_bytes = 768ull * 1024ull * 1024ull`

#### `RuntimeSaveGenerationCapture` (строка заголовка 303)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> time_package`
- `std::vector<std::byte> persistence_package`

#### `IRuntimeSaveGenerationPort` (строка заголовка 311)

Публичные сигнатуры операций:

```cpp
virtual ~IRuntimeSaveGenerationPort() = default;
[[nodiscard]] virtual foundation::Result<RuntimeSaveGenerationCapture> Capture( SaveGenerationId generation) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `CallbackRuntimeSaveGenerationPort` (строка заголовка 322)

Публичные сигнатуры операций:

```cpp
CallbackRuntimeSaveGenerationPort(PlaneCapture time_capture, PlaneCapture persistence_capture) : time_capture_(std::move(time_capture)), persistence_capture_(std::move(persistence_capture));
[[nodiscard]] foundation::Result<RuntimeSaveGenerationCapture> Capture( SaveGenerationId generation) override;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `SaveGenerationPackage` (строка заголовка 338)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> framework_savegame`
- `std::vector<std::byte> runtime_time`
- `std::vector<std::byte> runtime_persistence`

#### `ISaveGenerationStore` (строка заголовка 354)

Публичные сигнатуры операций:

```cpp
virtual ~ISaveGenerationStore() = default;
[[nodiscard]] virtual foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() = 0;
[[nodiscard]] virtual foundation::Result<void> CommitCurrent(std::span<const std::byte> package) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `FileSaveGenerationStore` (строка заголовка 362)

Внутреннее состояние (не часть публичного API): `std::filesystem::path path_`, `SaveGenerationLimits limits_`.

Публичные сигнатуры операций:

```cpp
) : path_(foundation::Path(std::move(path)).Native()), limits_(limits);
[[nodiscard]] foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() override;
[[nodiscard]] foundation::Result<void> CommitCurrent(std::span<const std::byte> package) override;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `SaveGenerationBarrierCallbacks` (строка заголовка 375)

Публичные поля и значения по умолчанию:

- `LeaseFactory acquire_restore_quiescence`

Публичные сигнатуры операций:

```cpp
std::function<foundation::Result<void>()> capture_preflight;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `SaveGenerationBarrier` (строка заголовка 390)

Публичные сигнатуры операций:

```cpp
explicit SaveGenerationBarrier(SaveGenerationBarrierCallbacks callbacks);
~SaveGenerationBarrier() override;
SaveGenerationBarrier(const SaveGenerationBarrier&) = delete;
SaveGenerationBarrier& operator=(const SaveGenerationBarrier&) = delete;
[[nodiscard]] foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> AcquireGenerationLease( SaveGenerationId generation);
[[nodiscard]] foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> AcquireCaptureLease() override;
[[nodiscard]] foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> AcquireRestoreLease() override;
[[nodiscard]] SaveGenerationId ActiveGeneration() const noexcept;
[[nodiscard]] bool IsCaptureActive() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `SaveGenerationCoordinator` (строка заголовка 410)

Внутреннее состояние (не часть публичного API): `savegame::SaveGameOrchestrator& framework_`, `IRuntimeSaveGenerationPort& runtime_`, `SaveGenerationBarrier& barrier_`, `ISaveGenerationStore& store_`, `SaveGenerationLimits limits_`, `savegame::SaveWireLimits framework_limits_`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] foundation::Result<SaveGenerationId> CaptureAndCommit( const savegame::SaveContext& context);
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `IRuntimeGenerationRestoreStage` (строка заголовка 439)

Публичные сигнатуры операций:

```cpp
virtual ~IRuntimeGenerationRestoreStage() = default;
virtual void Commit() noexcept = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IRuntimeGenerationRestorePort` (строка заголовка 446)

Публичные сигнатуры операций:

```cpp
virtual ~IRuntimeGenerationRestorePort() = default;
[[nodiscard]] virtual foundation::Result<std::unique_ptr<IRuntimeGenerationRestoreStage>> Prepare( const SaveGenerationPackage& package) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `WholeRestoreCallbacks` (строка заголовка 466)

Публичные сигнатуры операций:

```cpp
std::function<foundation::Result<void>()> reconcile_external;
std::function<foundation::Result<void>()> materialize_runtime;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `WholeEngineRestoreCoordinator` (строка заголовка 478)

Внутреннее состояние (не часть публичного API): `savegame::SaveGameOrchestrator& framework_`, `IRuntimeGenerationRestorePort& runtime_`, `SaveGenerationBarrier& barrier_`, `ISaveGenerationStore& store_`, `WholeRestoreCallbacks callbacks_`, `SaveGenerationLimits limits_`, `savegame::SaveWireLimits framework_limits_`, `std::unique_ptr<savegame::ISaveBarrierLease> lease_`, `WholeRestorePhase phase_ = WholeRestorePhase::AwaitLoad`.

Публичные сигнатуры операций:

```cpp
~WholeEngineRestoreCoordinator();
WholeEngineRestoreCoordinator(const WholeEngineRestoreCoordinator&) = delete;
WholeEngineRestoreCoordinator& operator=(const WholeEngineRestoreCoordinator&) = delete;
[[nodiscard]] foundation::Result<SaveGenerationId> LoadAndRestore(const savegame::RestoreContext& context);
[[nodiscard]] foundation::Result<void> RetryReconciliation();
[[nodiscard]] bool CanResume() const noexcept;
[[nodiscard]] WholeRestorePhase Phase() const noexcept;
[[nodiscard]] SaveGenerationId RestoredGeneration() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Load` | `FileSavePackageStore::Load` | `EngineFramework/IntegrationLayer/Integration/src/save_package_store.cpp:18` |
| `Commit` | `FileSavePackageStore::Commit` | `EngineFramework/IntegrationLayer/Integration/src/save_package_store.cpp:20` |
| `Synchronize` | `RuntimeTimeAdapter::Synchronize` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:471` |
| `DeclareRequiresActionHandler` | `ScheduledTriggerDispatcher::DeclareRequiresActionHandler` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:550` |
| `DeclareObserverOnlyAction` | `ScheduledTriggerDispatcher::DeclareObserverOnlyAction` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:555` |
| `RegisterActionHandler` | `ScheduledTriggerDispatcher::RegisterActionHandler` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:560` |
| `Freeze` | `ScheduledTriggerDispatcher::Freeze` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:590` |
| `DispatchPending` | `ScheduledTriggerDispatcher::DispatchPending` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:741` |
| `CaptureSnapshot` | `ScheduledTriggerDispatcher::CaptureSnapshot` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:915` |
| `ValidateSnapshot` | `ScheduledTriggerDispatcher::ValidateSnapshot` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:928` |
| `RestoreSnapshot` | `ScheduledTriggerDispatcher::RestoreSnapshot` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1025` |
| `Id` | `ScheduledTriggerDispatcherSaveParticipant::Id` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1037` |
| `CaptureSnapshot` | `ScheduledTriggerDispatcherSaveParticipant::CaptureSnapshot` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1042` |
| `ValidateSnapshot` | `ScheduledTriggerDispatcherSaveParticipant::ValidateSnapshot` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1083` |
| `StageRestore` | `ScheduledTriggerDispatcherSaveParticipant::StageRestore` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1117` |
| `CommitRestore` | `ScheduledTriggerDispatcherSaveParticipant::CommitRestore` | `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1166` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Commit`: `EngineFramework/DevelopmentInfrastructure/Tests/integration_save_participant_tests.cpp:21`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/integration_save_participant_tests.cpp:38`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/integration_save_participant_tests.cpp:39`

