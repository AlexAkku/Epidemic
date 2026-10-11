---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h"
---

# Simulation: simulation_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Simulation|Simulation]].
**Размер исходного файла:** 6315 байт.
**Подключение:** `#include <Epidemic/Runtime/Simulation/simulation_runtime.h>`.

## Объявления типов (лексический индекс)
`ISimulationJob`, `ISimulationScheduler`, `ISimulationRuntime`, `IAttentionSystem`, `IWorldMemory`, `IRelevancePolicy`, `ISimulationCommitTarget`, `IAbstractFactStore`, `ISimulationProposalQueue`, `IScheduledSimulationTasks`, `ISimulationClock`, `SimulationDependencies`, `SimulationServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Simulation/simulation_types.h"

#include <memory>
#include <span>
#include <cstdint>
#include <vector>

namespace epidemic::runtime::simulation
{
// Public Simulation contracts. These interfaces expose scheduling, attention, world-memory and effect buffering
// as infrastructure, leaving authoritative state changes to downstream resolvers/composition code.

class ISimulationJob;

class ISimulationScheduler
{
public:
    virtual ~ISimulationScheduler() = default;

    [[nodiscard]] virtual foundation::Result<SimulationJobHandle> SubmitJob(
        std::shared_ptr<ISimulationJob> job,
        SimulationJobDesc metadata) = 0;

    [[nodiscard]] virtual foundation::Result<void> CancelJob(SimulationJobHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<SimulationJobState> GetJobState(SimulationJobHandle handle) const = 0;

    virtual void SetBudget(const SimulationBudget& budget) = 0;

    [[nodiscard]] virtual foundation::Result<SimulationTickResult> Tick() = 0;
    [[nodiscard]] virtual foundation::Result<SimulationTickResult> ProcessMainThreadCommits(std::uint32_t max_jobs = 0) = 0;

    // Once shutdown has entered its retryable cleanup state, new scheduler/proposal work is rejected.
    // A failed Shutdown may be called again to continue cleanup; completed shutdown is idempotent.
    [[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
};

class ISimulationRuntime : public ISimulationScheduler
{
public:
    ~ISimulationRuntime() override = default;
};

class IAttentionSystem
{
public:
    virtual ~IAttentionSystem() = default;

    [[nodiscard]] virtual foundation::Result<void> SetAttention(RuntimeObjectId object, AttentionScore score) = 0;

    [[nodiscard]] virtual AttentionScore GetAttention(RuntimeObjectId object) const = 0;

    [[nodiscard]] virtual foundation::Result<void> SetRegionAttention(RegionId region, AttentionScore score) = 0;

    [[nodiscard]] virtual AttentionScore GetRegionAttention(RegionId region) const = 0;

    [[nodiscard]] virtual SimulationZoneState GetRegionZoneState(RegionId region) const = 0;
};

class IWorldMemory
{
public:
    virtual ~IWorldMemory() = default;

    [[nodiscard]] virtual foundation::Result<WorldMemoryEventId> RecordEvent(const WorldMemoryEvent& event) = 0;

    [[nodiscard]] virtual std::vector<WorldMemoryEvent> QueryEvents(const WorldMemoryQuery& query) const = 0;

    virtual void ExpireOldEvents(SimulationTime now) = 0;

    [[nodiscard]] virtual std::size_t ExpireOldEvents(SimulationTime now, std::uint32_t max_events) = 0;
};

class IRelevancePolicy
{
public:
    virtual ~IRelevancePolicy() = default;

    [[nodiscard]] virtual SimulationZoneState ClassifyZone(RegionId region, AttentionScore attention) const = 0;
};

class ISimulationJob
{
public:
    virtual ~ISimulationJob() = default;

    // Result failure or exception is not accepted as a completed logical step. Result success is accepted
    // by Runtime exactly once: if later Runtime-local publication must be retried, the successful result is
    // retained internally and ExecuteStep is not called again for that logical step.
    // Runtime never leaves a synthetic Running state published across the callback boundary.
    [[nodiscard]] virtual foundation::Result<SimulationStepResult> ExecuteStep(const RuntimeBudget& budget) = 0;
    [[nodiscard]] virtual foundation::Result<void> Cancel() = 0;
};

class ISimulationCommitTarget
{
public:
    virtual ~ISimulationCommitTarget() = default;

    // Failure must mean that no externally visible commit occurred. Implementations with non-transactional
    // side effects must deduplicate retries by the stable batch job/source_revision identity.
    [[nodiscard]] virtual foundation::Result<void> Commit(const SimulationProposalBatch& batch) = 0;
};

class IAbstractFactStore
{
public:
    virtual ~IAbstractFactStore() = default;

    [[nodiscard]] virtual foundation::Result<void> RecordFact(const AbstractFact& fact) = 0;
    [[nodiscard]] virtual std::vector<AbstractFact> QueryFacts(SimulationZoneId zone) const = 0;
    [[nodiscard]] virtual std::uint64_t Revision() const = 0;
};

class ISimulationProposalQueue
{
public:
    virtual ~ISimulationProposalQueue() = default;

    [[nodiscard]] virtual foundation::Result<void> Publish(const SimulationProposalBatch& batch) = 0;
    [[nodiscard]] virtual std::span<const SimulationProposalBatch> PendingBatches() const = 0;
    [[nodiscard]] virtual foundation::Result<void> CommitNext() = 0;
    [[nodiscard]] virtual foundation::Result<void> DiscardAll(ProposalDiscardReason reason) = 0;
};

class IScheduledSimulationTasks
{
public:
    virtual ~IScheduledSimulationTasks() = default;

    [[nodiscard]] virtual foundation::Result<ScheduledSimulationTaskId> Schedule(ScheduledSimulationTask task) = 0;
    [[nodiscard]] virtual foundation::Result<void> Cancel(ScheduledSimulationTaskId id) = 0;
    [[nodiscard]] virtual std::vector<ScheduledSimulationTask> QueryDue(SimulationTime now) const = 0;
    [[nodiscard]] virtual ScheduledTaskExecutionResult ExecuteDueWithinBudget(SimulationTime now, const SimulationBudget& budget) = 0;
};

class ISimulationClock
{
public:
    virtual ~ISimulationClock() = default;

    [[nodiscard]] virtual SimulationTime Now() const = 0;
};

struct SimulationDependencies
{
    std::shared_ptr<IRelevancePolicy> relevance;
    std::shared_ptr<ISimulationCommitTarget> commit_target;
    std::shared_ptr<ISimulationClock> clock;
};

struct SimulationServices
{
    std::shared_ptr<ISimulationRuntime> runtime;
    std::shared_ptr<ISimulationScheduler> scheduler;
    std::shared_ptr<IWorldMemory> memory;
    std::shared_ptr<IAbstractFactStore> facts;
    std::shared_ptr<ISimulationProposalQueue> proposals;
    std::shared_ptr<IScheduledSimulationTasks> scheduled_tasks;
    std::shared_ptr<IAttentionSystem> attention;
};

[[nodiscard]] foundation::Result<SimulationServices> CreateSimulationServices(
    SimulationOptions options = {},
    SimulationDependencies dependencies = {});
} // namespace epidemic::runtime::simulation
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Simulation/simulation_types.h`.

### Состав типов и операции

#### `ISimulationScheduler` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
virtual ~ISimulationScheduler() = default;
[[nodiscard]] virtual foundation::Result<SimulationJobHandle> SubmitJob( std::shared_ptr<ISimulationJob> job, SimulationJobDesc metadata) = 0;
[[nodiscard]] virtual foundation::Result<void> CancelJob(SimulationJobHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<SimulationJobState> GetJobState(SimulationJobHandle handle) const = 0;
virtual void SetBudget(const SimulationBudget& budget) = 0;
[[nodiscard]] virtual foundation::Result<SimulationTickResult> Tick() = 0;
[[nodiscard]] virtual foundation::Result<SimulationTickResult> ProcessMainThreadCommits(std::uint32_t max_jobs = 0) = 0;
completed shutdown is idempotent. [[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ISimulationRuntime` (строка заголовка 41)

Публичные сигнатуры операций:

```cpp
~ISimulationRuntime() override = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAttentionSystem` (строка заголовка 47)

Публичные сигнатуры операций:

```cpp
virtual ~IAttentionSystem() = default;
[[nodiscard]] virtual foundation::Result<void> SetAttention(RuntimeObjectId object, AttentionScore score) = 0;
[[nodiscard]] virtual AttentionScore GetAttention(RuntimeObjectId object) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetRegionAttention(RegionId region, AttentionScore score) = 0;
[[nodiscard]] virtual AttentionScore GetRegionAttention(RegionId region) const = 0;
[[nodiscard]] virtual SimulationZoneState GetRegionZoneState(RegionId region) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IWorldMemory` (строка заголовка 63)

Публичные сигнатуры операций:

```cpp
virtual ~IWorldMemory() = default;
[[nodiscard]] virtual foundation::Result<WorldMemoryEventId> RecordEvent(const WorldMemoryEvent& event) = 0;
[[nodiscard]] virtual std::vector<WorldMemoryEvent> QueryEvents(const WorldMemoryQuery& query) const = 0;
virtual void ExpireOldEvents(SimulationTime now) = 0;
[[nodiscard]] virtual std::size_t ExpireOldEvents(SimulationTime now, std::uint32_t max_events) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IRelevancePolicy` (строка заголовка 77)

Публичные сигнатуры операций:

```cpp
virtual ~IRelevancePolicy() = default;
[[nodiscard]] virtual SimulationZoneState ClassifyZone(RegionId region, AttentionScore attention) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ISimulationJob` (строка заголовка 85)

Публичные сигнатуры операций:

```cpp
virtual ~ISimulationJob() = default;
[[nodiscard]] virtual foundation::Result<void> Cancel() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ISimulationCommitTarget` (строка заголовка 98)

Публичные сигнатуры операций:

```cpp
virtual ~ISimulationCommitTarget() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAbstractFactStore` (строка заголовка 108)

Публичные сигнатуры операций:

```cpp
virtual ~IAbstractFactStore() = default;
[[nodiscard]] virtual foundation::Result<void> RecordFact(const AbstractFact& fact) = 0;
[[nodiscard]] virtual std::vector<AbstractFact> QueryFacts(SimulationZoneId zone) const = 0;
[[nodiscard]] virtual std::uint64_t Revision() const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ISimulationProposalQueue` (строка заголовка 118)

Публичные сигнатуры операций:

```cpp
virtual ~ISimulationProposalQueue() = default;
[[nodiscard]] virtual foundation::Result<void> Publish(const SimulationProposalBatch& batch) = 0;
[[nodiscard]] virtual std::span<const SimulationProposalBatch> PendingBatches() const = 0;
[[nodiscard]] virtual foundation::Result<void> CommitNext() = 0;
[[nodiscard]] virtual foundation::Result<void> DiscardAll(ProposalDiscardReason reason) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IScheduledSimulationTasks` (строка заголовка 129)

Публичные сигнатуры операций:

```cpp
virtual ~IScheduledSimulationTasks() = default;
[[nodiscard]] virtual foundation::Result<ScheduledSimulationTaskId> Schedule(ScheduledSimulationTask task) = 0;
[[nodiscard]] virtual foundation::Result<void> Cancel(ScheduledSimulationTaskId id) = 0;
[[nodiscard]] virtual std::vector<ScheduledSimulationTask> QueryDue(SimulationTime now) const = 0;
[[nodiscard]] virtual ScheduledTaskExecutionResult ExecuteDueWithinBudget(SimulationTime now, const SimulationBudget& budget) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ISimulationClock` (строка заголовка 140)

Публичные сигнатуры операций:

```cpp
virtual ~ISimulationClock() = default;
[[nodiscard]] virtual SimulationTime Now() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `SimulationDependencies` (строка заголовка 148)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IRelevancePolicy> relevance`
- `std::shared_ptr<ISimulationCommitTarget> commit_target`
- `std::shared_ptr<ISimulationClock> clock`

#### `SimulationServices` (строка заголовка 155)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<ISimulationRuntime> runtime`
- `std::shared_ptr<ISimulationScheduler> scheduler`
- `std::shared_ptr<IWorldMemory> memory`
- `std::shared_ptr<IAbstractFactStore> facts`
- `std::shared_ptr<ISimulationProposalQueue> proposals`
- `std::shared_ptr<IScheduledSimulationTasks> scheduled_tasks`
- `std::shared_ptr<IAttentionSystem> attention`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `SubmitJob` | `SimulationRuntime::SubmitJob` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:117` |
| `CancelJob` | `SimulationRuntime::CancelJob` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:177` |
| `GetJobState` | `SimulationRuntime::GetJobState` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:218` |
| `SetBudget` | `SimulationRuntime::SetBudget` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:230` |
| `Tick` | `SimulationRuntime::Tick` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:235` |
| `ProcessMainThreadCommits` | `SimulationRuntime::ProcessMainThreadCommits` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:408` |
| `Shutdown` | `SimulationRuntime::Shutdown` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:485` |
| `SetAttention` | `SimulationRuntime::SetAttention` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:536` |
| `GetAttention` | `SimulationRuntime::GetAttention` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:565` |
| `SetRegionAttention` | `SimulationRuntime::SetRegionAttention` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:576` |
| `GetRegionAttention` | `SimulationRuntime::GetRegionAttention` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:618` |
| `GetRegionZoneState` | `SimulationRuntime::GetRegionZoneState` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:624` |
| `RecordEvent` | `SimulationRuntime::RecordEvent` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:630` |
| `QueryEvents` | `SimulationRuntime::QueryEvents` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:706` |
| `ExpireOldEvents` | `SimulationRuntime::ExpireOldEvents` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:728` |
| `DiscardAll` | `SimulationRuntime::DiscardAll` | `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:763` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetBudget`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:280`
- `SubmitJob`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:282`
- `GetJobState`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:284`
- `Tick`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:285`
- `CancelJob`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:329`
- `RecordEvent`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:339`
- `QueryEvents`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:342`
- `ExpireOldEvents`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:343`
- `SetAttention`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:386`
- `GetAttention`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:387`
- `SetRegionAttention`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:388`
- `GetRegionAttention`: `EngineRuntime/Simulation/tests/simulation_tests.cpp:389`

