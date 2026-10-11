---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "simulation"]
source_path: "EngineRuntime/Simulation"
---

# EngineRuntime / Simulation

Модуль: **Simulation**. Budgeted proposals и commits.

**Реализация:** `EngineRuntime/Simulation/`.
**CMake targets:** `EpidemicRuntimeSimulation`
**Локальные test targets:** `EpidemicRuntimeSimulationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Simulation/simulation.h|simulation.h]]: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation.h`
- [[11 API/EngineRuntime/Simulation/simulation_runtime.h|simulation_runtime.h]]: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h`; объявленные типы: `ISimulationJob`, `ISimulationScheduler`, `ISimulationRuntime`, `IAttentionSystem`, `IWorldMemory`, `IRelevancePolicy`, `ISimulationCommitTarget`, `IAbstractFactStore`, `ISimulationProposalQueue`, `IScheduledSimulationTasks`, `ISimulationClock`, `SimulationDependencies`…
- [[11 API/EngineRuntime/Simulation/simulation_types.h|simulation_types.h]]: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`; объявленные типы: `SimulationJobId`, `WorldMemoryEventId`, `ScheduledSimulationTaskId`, `SimulationJobHandle`, `SimulationLane`, `SimulationZoneState`, `SimulationJobState`, `MemoryLifetime`, `ObservationState`, `AttentionScore`, `SimulationBudget`, `SimulationJobDesc`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/simulation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Simulation предоставляет budgeted выполнение независимых jobs, attention/relevance, world memory, abstract facts, scheduled tasks и очередь proposals. Он не содержит конкретную жизнь NPC, экономику или gameplay rules.

#### Jobs

Caller передает `shared_ptr<ISimulationJob>` и metadata. Scheduler выполняет `ExecuteStep()` под `RuntimeBudget`, контролирует generation handle и state transitions. Job может завершиться, ожидать main-thread publication или продолжить работу. Scheduled task активирует существующий job в `SimulationTime`.

`CancelJob()` удаляет связанное schedule. Shutdown запрещает новую работу и retry-ит failed cancellation.

#### Proposals

Job не меняет authoritative domain напрямую. `SimulationProposalBatch` публикуется только от `Completed` или `WaitingForMainThread` job. Queue commit-ит через `ISimulationCommitTarget`; terminal job удерживается до исчезновения последнего pending batch. `CommitNext()` и `DiscardAll(reason)` сразу запускают pruning.

#### Memory и facts

World memory event имеет неотрицательное `happened_at`; TTL меньше нуля запрещен, ноль означает отсутствие автоматического expiration. Capacity является жестким пределом и не вытесняет active events. Abstract facts валидируют domain, kind, type, zone, subject и time. Attention finite и находится в `[0,1]`.

#### Стабильность

Модуль прошёл локальную квалификацию Goal 3, обязательный MSVC Debug/Release pass и canonical evidence convergence и имеет статус `LOCAL_READY`. Статус `FROZEN` не присваивается до whole-engine qualification. Конкретные NPC и economy jobs реализуются в GameFramework.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `simulation.h`: factory-функции или backend implementation без отдельного публичного типа.
- `simulation_runtime.h`: `ISimulationJob`, `ISimulationScheduler`, `ISimulationRuntime`, `IAttentionSystem`, `IWorldMemory`, `IRelevancePolicy`, `ISimulationCommitTarget`, `IAbstractFactStore`, `ISimulationProposalQueue`, `IScheduledSimulationTasks`, `ISimulationClock`, `SimulationDependencies`, `SimulationServices`.
- `simulation_types.h`: `SimulationJobId`, `WorldMemoryEventId`, `ScheduledSimulationTaskId`, `SimulationJobHandle`, `SimulationLane`, `SimulationZoneState`, `SimulationJobState`, `MemoryLifetime`, `ObservationState`, `AttentionScore`, `SimulationBudget`, `SimulationJobDesc`, `SimulationStepInput`, `SimulationProposal`, `SimulationProposalBatch`, `SimulationStepResult`, `SimulationTickFailure`, `SimulationTickResult`, `WorldMemoryEvent`, `AbstractFact`, `ScheduledSimulationTask`, `ScheduledTaskFailure`, `ScheduledTaskExecutionResult`, `ProposalDiscardReason`, `WorldMemoryQuery`, `SimulationOptions`.

Источник контрактов: `Documentation/EngineRuntime/modules/simulation.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `SimulationLane` (`EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`): `Background`, `Zone`, `Object`, `MainThread`.
- `SimulationZoneState` (`EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`): `Dormant`, `Abstract`, `Scheduled`, `Relevant`, `Observed`, `Active`.
- `SimulationJobState` (`EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`): `Pending`, `Scheduled`, `Running`, `PartiallyComplete`, `WaitingForMainThread`, `Completed`, `Failed`, `Cancelled`.
- `MemoryLifetime` (`EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`): `Disposable`, `Temporary`, `Persistent`.
- `ObservationState` (`EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`): `Unobserved`, `Observed`, `PlayerAffected`.
- `ProposalDiscardReason` (`EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`): `Shutdown`, `AdministrativeReset`.

### Операции и хранилища состояния

#### `ISimulationScheduler`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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

#### `IAttentionSystem`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAttentionSystem() = default;
[[nodiscard]] virtual foundation::Result<void> SetAttention(RuntimeObjectId object, AttentionScore score) = 0;
[[nodiscard]] virtual AttentionScore GetAttention(RuntimeObjectId object) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetRegionAttention(RegionId region, AttentionScore score) = 0;
[[nodiscard]] virtual AttentionScore GetRegionAttention(RegionId region) const = 0;
[[nodiscard]] virtual SimulationZoneState GetRegionZoneState(RegionId region) const = 0;
```

#### `IWorldMemory`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:63`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IWorldMemory() = default;
[[nodiscard]] virtual foundation::Result<WorldMemoryEventId> RecordEvent(const WorldMemoryEvent& event) = 0;
[[nodiscard]] virtual std::vector<WorldMemoryEvent> QueryEvents(const WorldMemoryQuery& query) const = 0;
virtual void ExpireOldEvents(SimulationTime now) = 0;
[[nodiscard]] virtual std::size_t ExpireOldEvents(SimulationTime now, std::uint32_t max_events) = 0;
```

#### `ISimulationProposalQueue`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:118`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISimulationProposalQueue() = default;
[[nodiscard]] virtual foundation::Result<void> Publish(const SimulationProposalBatch& batch) = 0;
[[nodiscard]] virtual std::span<const SimulationProposalBatch> PendingBatches() const = 0;
[[nodiscard]] virtual foundation::Result<void> CommitNext() = 0;
[[nodiscard]] virtual foundation::Result<void> DiscardAll(ProposalDiscardReason reason) = 0;
```

#### `IScheduledSimulationTasks`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:129`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IScheduledSimulationTasks() = default;
[[nodiscard]] virtual foundation::Result<ScheduledSimulationTaskId> Schedule(ScheduledSimulationTask task) = 0;
[[nodiscard]] virtual foundation::Result<void> Cancel(ScheduledSimulationTaskId id) = 0;
[[nodiscard]] virtual std::vector<ScheduledSimulationTask> QueryDue(SimulationTime now) const = 0;
[[nodiscard]] virtual ScheduledTaskExecutionResult ExecuteDueWithinBudget(SimulationTime now, const SimulationBudget& budget) = 0;
```

#### `IAbstractFactStore`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:108`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAbstractFactStore() = default;
[[nodiscard]] virtual foundation::Result<void> RecordFact(const AbstractFact& fact) = 0;
[[nodiscard]] virtual std::vector<AbstractFact> QueryFacts(SimulationZoneId zone) const = 0;
[[nodiscard]] virtual std::uint64_t Revision() const = 0;
```

#### `IRelevancePolicy`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:77`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRelevancePolicy() = default;
[[nodiscard]] virtual SimulationZoneState ClassifyZone(RegionId region, AttentionScore attention) const = 0;
```

#### `ISimulationJob`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:85`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISimulationJob() = default;
[[nodiscard]] virtual foundation::Result<void> Cancel() = 0;
```

#### `ISimulationClock`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:140`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISimulationClock() = default;
[[nodiscard]] virtual SimulationTime Now() const = 0;
```

#### `ISimulationRuntime`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:41`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
~ISimulationRuntime() override = default;
```

#### `ISimulationCommitTarget`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:98`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISimulationCommitTarget() = default;
```

#### `SimulationServices`

Объявлен в `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h:155`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<ISimulationRuntime> runtime`; `std::shared_ptr<ISimulationScheduler> scheduler`; `std::shared_ptr<IWorldMemory> memory`; `std::shared_ptr<IAbstractFactStore> facts`; `std::shared_ptr<ISimulationProposalQueue> proposals`; `std::shared_ptr<IScheduledSimulationTasks> scheduled_tasks`; `std::shared_ptr<IAttentionSystem> attention`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Simulation/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp` (1409 строк): `Error::Create`, `SimulationRuntime::SimulationRuntime`, `SimulationRuntime::SubmitJob`, `SimulationRuntime::CancelJob`, `SimulationRuntime::GetJobState`, `SimulationRuntime::SetBudget`, `SimulationRuntime::Tick`, `SimulationRuntime::ProcessMainThreadCommits`, `SimulationRuntime::Shutdown`, `SimulationRuntime::SetAttention`
- `EngineRuntime/Simulation/src/simulation.cpp` (21 строк)

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `simulation.callback_exception`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:302`
- `simulation.allocation_failed`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:101`
- `simulation.shutdown`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:908`
- `simulation.job_missing`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:134`
- `simulation.job_id_overflow`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:142`
- `simulation.duplicate_job_id`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:164`
- `simulation.stale_handle`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:1248`
- `simulation.job_terminal`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:1303`
- `simulation.work_budget_exceeded`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:335`
- `simulation.proposal_queue_full`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:851`
- `simulation.pending_result_missing`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:439`
- `simulation.invalid_object`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:541`
- `simulation.invalid_attention`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:586`
- `simulation.invalid_region`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:581`
- `simulation.invalid_zone_state`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:603`
- `simulation.invalid_memory_region`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:635`
- `simulation.invalid_memory_ttl`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:640`
- `simulation.invalid_memory_time`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:645`
- `simulation.invalid_memory_enum`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:650`
- `simulation.memory_expiration_overflow`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:655`
- `simulation.memory_store_full`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:660`
- `simulation.memory_event_id_overflow`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:666`
- `simulation.memory_event_already_exists`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:687`
- `simulation.invalid_discard_reason`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:772`
- `simulation.invalid_fact_metadata`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:784`
- `simulation.invalid_fact`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:789`
- `simulation.revision_overflow`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:795`
- `simulation.fact_store_full`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:800`
- `simulation.commit_target_missing`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:884`
- `simulation.invalid_lane`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:1222`
- `simulation.invalid_due_time`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:918`
- `simulation.job_already_scheduled`: `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:934`

### Конкретные тестовые якоря

- `EngineRuntime/Simulation/tests/simulation_tests.cpp`: `TestSubmitJobAndCompletion`, `TestPartialCompletionByBudget`, `TestJobBudgetOrderIsDeterministic`, `TestCancellation`, `TestWorldMemoryTtlAndStates`, `TestWorldMemoryExpirationBudget`, `TestWorldMemoryIdAndTtlValidation`, `TestAttentionAndValidationFailures`, `TestBackendOrientedContractsAndServices`, `TestProposalCommitAtomicityAndFaultIsolation`, `TestProposalPublicationStateMatrix`, `TestAbstractFactRevision`, `TestScheduledTaskBudget`
- `EngineFramework/DevelopmentInfrastructure/Tests/simulation_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `simulation.callback_exception` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:302`)

```cpp
300         {
  301             foundation::Result<SimulationStepResult> step = foundation::Result<SimulationStepResult>::Failure(
  302                 foundation::Error::Create("simulation.callback_exception", "simulation job callback did not run"));
  303             try
  304             {
```

#### `simulation.allocation_failed` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:101`)

```cpp
99 [[nodiscard]] foundation::Error AllocationError(std::string_view operation)
  100 {
  101     return foundation::Error::Create("simulation.allocation_failed", operation);
  102 }
  103
```

#### `simulation.shutdown` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:908`)

```cpp
906     {
  907         return foundation::Result<ScheduledSimulationTaskId>::Failure(
  908             foundation::Error::Create("simulation.shutdown", "simulation runtime is not accepting scheduled work during shutdown"));
  909     }
  910     if (!IsValid(task.lane))
```

#### `simulation.job_missing` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:134`)

```cpp
132     {
  133         return foundation::Result<SimulationJobHandle>::Failure(
  134             foundation::Error::Create("simulation.job_missing", "simulation job executable must be provided"));
  135     }
  136
```

#### `simulation.job_id_overflow` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:142`)

```cpp
140     {
  141         return foundation::Result<SimulationJobHandle>::Failure(
  142             foundation::Error::Create("simulation.job_id_overflow", "simulation job identity allocator is exhausted"));
  143     }
  144
```

#### `simulation.duplicate_job_id` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:164`)

```cpp
162         {
  163             return foundation::Result<SimulationJobHandle>::Failure(
  164                 foundation::Error::Create("simulation.duplicate_job_id", "allocated simulation job id already exists"));
  165         }
  166     }
```

#### `simulation.stale_handle` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:1248`)

```cpp
1246     {
 1247         return foundation::Result<void>::Failure(
 1248             foundation::Error::Create("simulation.stale_handle", "proposal batch must reference an existing job handle"));
 1249     }
 1250     if (batch.zone != job->desc.zone)
```

#### `simulation.job_terminal` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:1303`)

```cpp
1301     {
 1302         return foundation::Result<void>::Failure(
 1303             foundation::Error::Create("simulation.job_terminal", "terminal simulation jobs cannot execute steps"));
 1304     }
 1305     if (!result.proposals.proposals.empty())
```

#### `simulation.work_budget_exceeded` (`EngineRuntime/Simulation/src/simulation_runtime_impl.cpp:335`)

```cpp
333             foundation::Error error = !validated
  334                 ? validated.GetError()
  335                 : foundation::Error::Create("simulation.work_budget_exceeded", "simulation job consumed more work than granted");
  336             result.failures.push_back(SimulationTickFailure{job.handle, std::move(error)});
  337             job.pending_step_result.reset();
```
