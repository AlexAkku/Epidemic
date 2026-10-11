---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "simulation"]
source_path: "EngineFramework/GameplayWorldStateOwners/Simulation"
---

# EngineFramework / Simulation

Модуль: **Simulation**. Budgeted proposals и commits.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Simulation/`.
**CMake targets:** `EpidemicGameFrameworkSimulation`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Simulation/simulation.h|simulation.h]]: `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h`; объявленные типы: `SimulationRegionId`, `SimulationLayerId`, `SimulationTaskId`, `SimulationPolicyId`, `SimulationSummaryId`, `IdHash`, `SimulationDetailLevel`, `SimulationTaskState`, `SimulationIntervalState`, `SimulationMaterializationPolicy`, `SimulationChangeKind`, `SimulationBudget`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Simulation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility and ownership

Owns Framework simulation regions, layer definitions/executor bindings, resumable interval/task state, completed summaries, task/summary generators, revisions and the bounded simulation journal.

The service remains the single authoritative owner of that state. Derived indexes, journals and diagnostic/read models are not independent semantic owners.

#### Authoritative state and indexes

Admission/generated B05 inventory: 45 public callables, 40 mutation obligations, 2 lifecycle candidates, 9 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Multi-container mutations must complete fallible staging before authoritative publication. Deterministic query/order semantics must agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B05 coverage projection. Revision, identity-generator and journal publication is part of the mutation contract. Allocation/publication failure may not expose partially advanced primary state, index state, generator, revision or journal.

B05 allocation evidence uses source-private module seams rather than the shared process-global allocator override. The seams are implementation/test details and do not expand the public API.

#### Lifecycle and identity

All emitted lifecycle and stale-identity candidates were reviewed. Invalid, removed, duplicate and stale identities are rejected or handled as the documented no-op without false publication. Retry does not silently resurrect terminal state.

#### Persistence

Snapshot/restore is reviewed as candidate-state publication: validate first, build off-state, preserve the current service on failure, then atomically publish the candidate. Successful restore preserves generator/revision/cursor continuity required by the module snapshot contract.

#### External boundaries

`ISimulationLayerExecutor::Prepare/Commit` is the critical accepted-work boundary. Stable task IDs and idempotent commit remain required. Before an external `Commit`, the service now proves capacity for the task terminal record. If the task is the last unfinished layer, it additionally stages the final interval revision, summary node and `SummaryGenerated` journal record. After accepted work only no-throw local state/journal publication remains.

#### Threading contract

No additional internal synchronization guarantee is introduced by B05. These gameplay owners are treated as externally serialized/owner-thread services for Goal 4 local correctness. Engine-wide concurrency qualification remains Goal 6.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkSimulationTests` from `EngineFramework/DevelopmentInfrastructure/Tests/simulation_tests.cpp`.

`G4-SIM-001` is covered by restored pending-interval regressions for revision `MAX`, revision `MAX-1` on the last unfinished layer, final journal-sequence exhaustion and terminal `SummaryPublication` allocation failure. Each proves external `Commit` is not called unless both task and interval terminal publication can complete, and the allocation case also proves retry succeeds. `G4-SIM-002` is covered by module-local `IntervalPublication`, `SummaryPublication` and `JournalPublication` fault points with complete snapshot equality on failure and retry after summary-publication failure.

Источник контрактов: `Documentation/EngineFramework/Simulation.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `SimulationDetailLevel` (`EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h`): `Disabled`, `Dormant`, `Abstract`, `Coarse`, `Detailed`, `Materialized`.
- `SimulationTaskState` (`EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h`): `Pending`, `Running`, `Completed`, `Failed`, `Skipped`, `Deferred`.
- `SimulationIntervalState` (`EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h`): `Pending`, `Completed`.
- `SimulationMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h`): `AbstractCapable`, `RequiresDetailed`, `RequiresMaterialized`.
- `SimulationChangeKind` (`EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h`): `RegionRegistered`, `LayerRegistered`, `IntervalStarted`, `TaskPrepared`, `TaskCommitted`, `TaskDeferred`, `TaskFailed`, `BudgetExceeded`, `SummaryGenerated`.

### Операции и хранилища состояния

#### `SimulationService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:284`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
SimulationService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
void FreezeDefinitions() noexcept;
void Freeze() noexcept;
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] const SimulationRegion *FindRegion(SimulationRegionId id) const noexcept;
[[nodiscard]] const SimulationLayerDefinition *FindLayer(SimulationLayerId id) const noexcept;
[[nodiscard]] const SimulationIntervalExecution *FindActiveInterval(SimulationRegionId region) const noexcept;
void SetBudget(SimulationBudget budget) noexcept;
void SetRetentionPolicy(SimulationRetentionPolicy policy) noexcept;
[[nodiscard]] std::vector<SimulationSummary> FindSummaries(SimulationRegionId region) const;
[[nodiscard]] SimulationChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] SimulationSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(SimulationSnapshot snapshot);
[[nodiscard]] SimulationDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool definitions_frozen_ = false`; `std::unordered_map<SimulationRegionId, SimulationRegion, IdHash> regions_`; `std::unordered_map<SimulationLayerId, SimulationLayerDefinition, IdHash> layers_`; `std::unordered_map<SimulationLayerId, ISimulationLayerExecutor *, IdHash> executors_`; `std::unordered_map<SimulationRegionId, SimulationIntervalExecution, IdHash> active_intervals_`; `std::list<SimulationSummary> summaries_`; `MonotonicIdGenerator<GameplayObjectId> task_ids_`; `MonotonicIdGenerator<GameplayObjectId> summary_ids_`.

#### `ISimulationLayerExecutor`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:270`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISimulationLayerExecutor() = default;
[[nodiscard]] virtual SimulationLayerId Layer() const noexcept = 0;
[[nodiscard]] virtual foundation::Result<void> Commit(const SimulationTask &task, const SimulationLayerSummary &summary) = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:89`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `SimulationTaskId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr SimulationTaskId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `SimulationSummaryId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:75`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr SimulationSummaryId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `SimulationBudget`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:138`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t max_layer_commits = 1024`; `std::int64_t max_interval_ticks = 86'400`.

#### `SimulationRetentionPolicy`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Simulation/include/Epidemic/GameFramework/Simulation/simulation.h:147`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::size_t max_completed_summaries = 1024`; `std::size_t max_changes = 4096`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Simulation/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp` (1042 строк): `Error::Create`, `SimulationService::SimulationService`, `SimulationService::RegisterRegion`, `SimulationRegionId::FromString`, `SimulationService::RegisterLayer`, `SimulationLayerId::FromString`, `SimulationService::FindRegion`, `SimulationService::FindLayer`, `SimulationService::FindActiveInterval`, `SimulationService::SetRetentionPolicy`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.simulation.journal_exhausted`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:110`
- `gameplay.simulation.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:974`
- `gameplay.simulation.invalid_region`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:171`
- `gameplay.simulation.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:567`
- `gameplay.simulation.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:195`
- `gameplay.simulation.invalid_layer`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:222`
- `gameplay.simulation.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:312`
- `gameplay.simulation.interval_exists`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:362`
- `gameplay.simulation.interval_missing`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:376`
- `gameplay.simulation.layer_missing`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:429`
- `gameplay.simulation.prepare_failed`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:450`
- `gameplay.simulation.executor_exception`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:633`
- `gameplay.simulation.invalid_prepare`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:497`
- `gameplay.simulation.commit_failed`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:620`
- `gameplay.simulation.interval_pending`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:704`
- `gameplay.simulation.region_missing`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:772`
- `gameplay.simulation.invalid_interval`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:775`
- `gameplay.simulation.interval_budget`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:780`
- `gameplay.simulation.interval_in_progress`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:790`
- `gameplay.simulation.interval_conflict`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:796`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:879`
- `gameplay.simulation.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:968`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/simulation_tests.cpp`
- `EngineRuntime/Simulation/tests/simulation_tests.cpp`: `TestSubmitJobAndCompletion`, `TestPartialCompletionByBudget`, `TestJobBudgetOrderIsDeterministic`, `TestCancellation`, `TestWorldMemoryTtlAndStates`, `TestWorldMemoryExpirationBudget`, `TestWorldMemoryIdAndTtlValidation`, `TestAttentionAndValidationFailures`, `TestBackendOrientedContractsAndServices`, `TestProposalCommitAtomicityAndFaultIsolation`, `TestProposalPublicationStateMatrix`, `TestAbstractFactRevision`, `TestScheduledTaskBudget`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.simulation.journal_exhausted` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:110`)

```cpp
108     if (next_sequence == 0)
  109         return foundation::Result<std::uint64_t>::Failure(
  110             Error("gameplay.simulation.journal_exhausted", "simulation change journal sequence exhausted"));
  111     if (internal_test::ConsumeAllocationFault(internal_test::AllocationFaultPoint::JournalPublication))
  112         return foundation::Result<std::uint64_t>::Failure(
```

#### `gameplay.simulation.allocation_failed` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:974`)

```cpp
972     {
  973         return foundation::Result<void>::Failure(
  974             Error("gameplay.simulation.allocation_failed", "simulation restore staging failed"));
  975     }
  976
```

#### `gameplay.simulation.invalid_region` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:171`)

```cpp
169         if (!staged_regions.emplace(id, std::move(region)).second)
  170             return foundation::Result<SimulationRegionId>::Failure(
  171                 Error("gameplay.simulation.invalid_region", "invalid or duplicate region"));
  172     }
  173     catch (...)
```

#### `gameplay.simulation.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:567`)

```cpp
565             if (next.Value().value == std::numeric_limits<std::uint64_t>::max())
  566                 return foundation::Result<SimulationSummaryId>::Failure(
  567                     Error("gameplay.simulation.revision_exhausted", "simulation revision exhausted"));
  568             terminal_revision = Revision{next.Value().value + 1};
  569
```

#### `gameplay.simulation.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:195`)

```cpp
193     if (definitions_frozen_)
  194         return foundation::Result<SimulationLayerId>::Failure(
  195             Error("gameplay.simulation.registry_frozen", "simulation layer definitions frozen"));
  196     if (layer.canonical_name.empty() || executor == nullptr ||
  197         !IsValidSimulationMaterializationPolicy(layer.materialization_policy))
```

#### `gameplay.simulation.invalid_layer` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:222`)

```cpp
220         if (!staged_layers.emplace(id, std::move(layer)).second || !staged_executors.emplace(id, executor).second)
  221             return foundation::Result<SimulationLayerId>::Failure(
  222                 Error("gameplay.simulation.invalid_layer", "invalid or duplicate layer"));
  223     }
  224     catch (...)
```

#### `gameplay.simulation.id_exhausted` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:312`)

```cpp
310             if (!layer_execution.task.id.IsValid())
  311                 return foundation::Result<void>::Failure(
  312                     Error("gameplay.simulation.id_exhausted", "simulation task id generator exhausted"));
  313             layer_execution.task.region = region.id;
  314             layer_execution.task.layer = layer.id;
```

#### `gameplay.simulation.interval_exists` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:362`)

```cpp
360     if (!inserted.inserted)
  361         return foundation::Result<void>::Failure(
  362             Error("gameplay.simulation.interval_exists", "active simulation interval already exists"));
  363     task_ids_ = staged_task_ids;
  364     summary_ids_ = staged_summary_ids;
```

#### `gameplay.simulation.interval_missing` (`EngineFramework/GameplayWorldStateOwners/Simulation/src/simulation.cpp:376`)

```cpp
374     if (execution_it == active_intervals_.end())
  375         return foundation::Result<SimulationSummaryId>::Failure(
  376             Error("gameplay.simulation.interval_missing", "active simulation interval missing"));
  377     auto &execution = execution_it->second;
  378
```
