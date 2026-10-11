---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "processes"]
source_path: "EngineFramework/GameplayWorldStateOwners/Processes"
---

# EngineFramework / Processes

Модуль: **Processes**. Игровые процессы.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Processes/`.
**CMake targets:** `EpidemicGameFrameworkProcesses`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Processes/processes.h|processes.h]]: `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`; объявленные типы: `ProcessDefinitionId`, `ProcessRecipeId`, `ProcessKindId`, `ProcessStepId`, `ProcessInputId`, `ProcessOutputId`, `ProcessInputTypeId`, `ProcessOutputTypeId`, `ProcessStationId`, `ProcessInstanceId`, `ProcessReservationId`, `ProcessQualityId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Processes|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns process definitions, recipes, stations, process instances, reservation/output reconciliation records, timing state, revisions/generators and the process journal.

The service remains the single authoritative owner of the state listed below. Derived indexes and journals are not independent semantic owners.

#### Authoritative state and indexes

Admission inventory: 93 public callables, 88 mutation obligations, 4 lifecycle candidates, 20 stale-identity candidates and 4 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Mutations that span several containers must complete all fallible staging before the no-fail authoritative commit point. Query ordering and index-derived views are required to agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B03 coverage projection. Revision, ID-generator and journal publication is part of the mutation contract. Allocation/publication failure must not expose a partially advanced generator, revision, primary record, derived index or journal entry.

B03 allocation evidence uses source-private module seams rather than the shared process-global allocator override. These seams are test-only implementation details and do not expand the public API.

#### Lifecycle and identity

Lifecycle candidates and stale identity candidates from the admission inventory were reviewed. Invalid, removed, duplicate and stale IDs are required to be rejected or treated as the documented no-op without publishing false state. Terminal records are not silently resurrected by retry.

#### Persistence

Snapshot/restore is reviewed as a candidate-state operation: validate first, build off-state, preserve all live state on failure, then publish the candidate. Successful restore preserves the persistent ID-generator/revision/journal boundary represented by the snapshot contract.

#### External boundaries

Input and output providers are transactional ports. Prepare, commit, cancel, compensation and retry/reconciliation paths are part of the local contract. Four admission external-boundary candidates were reviewed.

External callback/provider failure is converted to the module error/result contract. Retry must not duplicate an already committed semantic side effect, and failed compensation that requires reconciliation stays represented by owned state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B03. These owners are treated as externally serialized/owner-thread services for local correctness evidence. Later Goal 6 performs the engine-wide concurrency qualification.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkProcessesTests` from `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp`.

`G4-PROC-001` is closed by exact integer progress scaling and `TestExactProgressScaling`. The large `INT64_MAX` one-tick-before-completion case yields 999999 without relying on extended floating-point precision.

Источник контрактов: `Documentation/EngineFramework/Processes.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ProcessTimingPolicy` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Instant`, `Timed`, `ExternalCompletion`.
- `ProcessPersistencePolicy` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Transient`, `Session`, `Persistent`.
- `InputConsumptionPolicy` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `ConsumeOnStart`, `ConsumeOnCompletion`, `ReserveThenConsume`, `ToolNotConsumed`, `Catalyst`, `ConditionRequired`.
- `OutputDeliveryPolicy` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Immediate`, `OnCompletion`.
- `ProcessInstanceState` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Prepared`, `Reserved`, `Running`, `Paused`, `Completed`, `Failed`, `Cancelled`, `Expired`, `ReconciliationRequired`.
- `StationState` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Active`, `Disabled`, `Destroyed`, `Occupied`, `Unavailable`.
- `ProcessChangeKind` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Prepared`, `Started`, `Paused`, `Resumed`, `Completed`, `Failed`, `Cancelled`, `InputReserved`, `InputConsumed`, `OutputProduced`, `QualityResolved`, `ReconciliationRequired`, `StationRegistered`, `Pruned`.
- `ProcessInputCommitState` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Reserved`, `Consumed`, `Released`.
- `ProcessOutputCommitState` (`EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`): `Prepared`, `Committed`, `Cancelled`.

### Операции и хранилища состояния

#### `ProcessesService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h:527`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
ProcessesService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<ProcessDefinitionId> RegisterDefinition(ProcessDefinition definition);
[[nodiscard]] foundation::Result<ProcessRecipeId> RegisterRecipe(ProcessRecipe recipe);
[[nodiscard]] foundation::Result<ProcessStationId> RegisterStation(ProcessStation station);
void Freeze() noexcept;
void SetInputProvider(IProcessInputProvider *provider) noexcept;
void AddOutputHandler(IProcessOutputHandler *handler);
void SetQualityProvider(const IProcessQualityProvider *provider) noexcept;
[[nodiscard]] const ProcessDefinition *FindDefinition(ProcessDefinitionId id) const noexcept;
[[nodiscard]] const ProcessRecipe *FindRecipe(ProcessRecipeId id) const noexcept;
[[nodiscard]] const ProcessStation *FindStationByObject(GameplayObjectRef station) const noexcept;
[[nodiscard]] const ProcessInstance *FindInstance(ProcessInstanceId id) const noexcept;
[[nodiscard]] Fixed EvaluateProgress(ProcessInstanceId id, GameplayTimePoint now) const noexcept;
[[nodiscard]] foundation::Result<ProcessInstanceId> StartProcess(StartProcessRequest request);
[[nodiscard]] foundation::Result<ProcessBatchCompletionReport> CompleteDue(GameplayTimePoint now);
[[nodiscard]] std::vector<ProcessInstance> FindDueProcessesForSimulation(GameplayObjectRef simulation_area, GameplayTimePoint from, GameplayTimePoint to) const;
[[nodiscard]] std::vector<ProcessInstance> FindProcessesByActor(GameplayObjectRef actor) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<ProcessDefinitionId, ProcessDefinition, IdHash> definitions_`; `std::unordered_map<ProcessRecipeId, ProcessRecipe, IdHash> recipes_`; `std::unordered_map<ProcessStationId, ProcessStation, IdHash> stations_`; `std::unordered_map<GameplayObjectRef, ProcessStationId> station_by_object_`; `std::unordered_map<ProcessInstanceId, ProcessInstance, IdHash> instances_`; `MonotonicIdGenerator<GameplayObjectId> station_ids_`; `MonotonicIdGenerator<GameplayObjectId> instance_ids_`.

#### `IProcessInputProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h:485`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IProcessInputProvider() = default;
[[nodiscard]] virtual foundation::Result<void> Validate(const ProcessInputDefinition &input, const StartProcessRequest &request, ProcessInstanceId instance) = 0;
[[nodiscard]] virtual foundation::Result<ReservedProcessInput> Reserve(const ProcessInputDefinition &input, const StartProcessRequest &request, ProcessInstanceId instance) = 0;
[[nodiscard]] virtual foundation::Result<void> Release(const ReservedProcessInput &reservation, GameplayContext context) = 0;
```

#### `IProcessOutputHandler`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h:502`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IProcessOutputHandler() = default;
[[nodiscard]] virtual bool Supports(ProcessOutputTypeId type) const noexcept = 0;
[[nodiscard]] virtual foundation::Result<void> Commit(const PreparedProcessOutput &output, const ProcessInstance &instance, GameplayContext context) = 0;
[[nodiscard]] virtual foundation::Result<void> Cancel(const PreparedProcessOutput &output, const ProcessInstance &instance, GameplayContext context) = 0;
```

#### `IProcessQualityProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h:519`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IProcessQualityProvider() = default;
[[nodiscard]] virtual ProcessQualityResult Resolve(const ProcessRecipe &recipe, const StartProcessRequest &request) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h:192`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Processes/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp` (996 строк): `Error::Create`, `ProcessesService::ProcessesService`, `ProcessesService::CanAdvanceRevision`, `ProcessesService::CanRecordChanges`, `ProcessesService::RegisterDefinition`, `ProcessDefinitionId::FromString`, `ProcessesService::RegisterRecipe`, `ProcessRecipeId::FromString`, `ProcessesService::RegisterStation`, `ProcessesService::FindDefinition`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.processes.provider_exception`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:67`
- `gameplay.processes.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:184`
- `gameplay.processes.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:175`
- `gameplay.processes.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:814`
- `gameplay.processes.publication_failed`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:876`
- `gameplay.processes.invalid_recipe`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:212`
- `gameplay.processes.invalid_input`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:200`
- `gameplay.processes.invalid_output`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:207`
- `gameplay.processes.invalid_station`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:226`
- `gameplay.processes.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:626`
- `gameplay.processes.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:814`
- `gameplay.processes.input_provider_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:471`
- `gameplay.processes.reconciliation_required`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:407`
- `gameplay.processes.nonportable_token`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:554`
- `gameplay.processes.output_handler_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:600`
- `gameplay.processes.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:624`
- `gameplay.processes.recipe_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:676`
- `gameplay.processes.definition_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:625`
- `gameplay.processes.station_unavailable`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:625`
- `gameplay.processes.station_capability_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:625`
- `framework.quality.normal`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:627`
- `gameplay.processes.duplicate_instance`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:634`
- `gameplay.processes.instance_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:750`
- `gameplay.processes.invalid_state`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:750`
- `gameplay.processes.not_due`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:750`
- `gameplay.processes.simulation_area_missing`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:749`
- `gameplay.processes.simulation_area_mismatch`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:750`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:852`
- `gameplay.processes.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:942`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp`: `TestReservationIdExhaustionBeforeExternalCall`, `TestProviderReservationIdsAreCanonicalized`, `TestNonPortableInputCompensation`, `TestNonPortableOutputCompensation`, `TestImmediatePrepareFailureWithInputRollbackFailure`, `TestEarlierPreparedOutputCancelFailureIsDurable`, `TestInstantCompletionFailureKeepsOperationIdentity`, `TestCheckedTimeBoundaries`, `TestExactProgressScaling`, `TestJournalSequenceSurvivesSnapshot`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.processes.provider_exception` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:67`)

```cpp
65 [[nodiscard]] foundation::Error CallbackError(std::string_view stage)
   66 {
   67     return foundation::Error::Create("gameplay.processes.provider_exception", stage);
   68 }
   69
```

#### `gameplay.processes.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:184`)

```cpp
182     if (frozen_)
  183         return foundation::Result<ProcessRecipeId>::Failure(
  184             Error("gameplay.processes.registry_frozen", "process registry is frozen"));
  185     if (recipe.canonical_name.empty() || !definitions_.contains(recipe.process))
  186         return foundation::Result<ProcessRecipeId>::Failure(
```

#### `gameplay.processes.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:175`)

```cpp
173         return foundation::Result<ProcessDefinitionId>::Failure(Error("gameplay.processes.revision_exhausted", "process revision is exhausted"));
  174     const Revision next{revision_.value + 1}; definition.revision = next; const auto id = definition.id;
  175     try { if (!definitions_.emplace(id, std::move(definition)).second) return foundation::Result<ProcessDefinitionId>::Failure(Error("gameplay.processes.invalid_definition", "duplicate process definition")); }
  176     catch (...) { return foundation::Result<ProcessDefinitionId>::F…
```

#### `gameplay.processes.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:814`)

```cpp
812 foundation::Result<void> ProcessesService::PruneTerminalProcesses(std::size_t keep_recent)
  813 {
  814     std::vector<ProcessInstanceId> terminal;for(const auto &[id,instance]:instances_)if(IsTerminal(instance.state))terminal.push_back(id);std::sort(terminal.begin(),terminal.end());if(terminal.size()<=keep_recent)return foundation::Result<void>::Success();const auto count=terminal.size()-keep_recent;if(!CanAdvanceRevision()||!CanRecordChanges(count))return foundation::Result<void>::Failure(Error(!CanAdvanceRevision()?"gameplay.processes.…
```

#### `gameplay.processes.publication_failed` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:876`)

```cpp
874     if (internal_test::ConsumeAllocationFault(internal_test::AllocationFaultPoint::RestoreInstances))
  875         return foundation::Result<void>::Failure(
  876             Error("gameplay.processes.publication_failed", "process restore instance staging failed"));
  877     for (auto &instance : snapshot.instances)
  878     {
```

#### `gameplay.processes.invalid_recipe` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:212`)

```cpp
210         return foundation::Result<ProcessRecipeId>::Failure(Error("gameplay.processes.revision_exhausted", "process revision is exhausted"));
  211     const Revision next{revision_.value + 1}; recipe.revision = next; const auto id = recipe.id;
  212     try { if (!recipes_.emplace(id, std::move(recipe)).second) return foundation::Result<ProcessRecipeId>::Failure(Error("gameplay.processes.invalid_recipe", "duplicate process recipe")); }
  213     catch (...) { return foundation::Result<ProcessRecipeId>::Failure(Error("gameplay.processes.pub…
```

#### `gameplay.processes.invalid_input` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:200`)

```cpp
198         if (!input.id.IsValid() || !input.type.IsValid() || input.amount < 0 || !IsValid(input.consumption) || !input_ids.insert(input.id).second)
  199             return foundation::Result<ProcessRecipeId>::Failure(
  200                 Error("gameplay.processes.invalid_input", "recipe has invalid or duplicate input"));
  201     }
  202     std::unordered_set<ProcessOutputId, IdHash> output_ids;
```

#### `gameplay.processes.invalid_output` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:207`)

```cpp
205         if (!output.id.IsValid() || !output.type.IsValid() || output.amount < 0 || !IsValid(output.delivery) || !output_ids.insert(output.id).second)
  206             return foundation::Result<ProcessRecipeId>::Failure(
  207                 Error("gameplay.processes.invalid_output", "recipe has invalid or duplicate output"));
  208     }
  209     if (!CanAdvanceRevision())
```

#### `gameplay.processes.invalid_station` (`EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:226`)

```cpp
224     if (!station.id.IsValid()) station.id = ProcessStationId{staged_ids.Next()};
  225     if (!station.id.IsValid()) return foundation::Result<ProcessStationId>::Failure(Error("gameplay.processes.id_exhausted", "process station id generator is exhausted"));
  226     if (stations_.contains(station.id)) return foundation::Result<ProcessStationId>::Failure(Error("gameplay.processes.invalid_station", "duplicate process station id"));
  227     AdvanceGeneratorPastAcceptedId(staged_ids, station.id);
  228     if (!CanAdvanceRevision() || !CanR…
```
