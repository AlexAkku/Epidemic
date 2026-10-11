---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "streaming"]
source_path: "EngineRuntime/Streaming"
---

# EngineRuntime / Streaming

Модуль: **Streaming**. Загрузка и выгрузка chunks по budget.

**Реализация:** `EngineRuntime/Streaming/`.
**CMake targets:** `EpidemicRuntimeStreaming`
**Локальные test targets:** `EpidemicRuntimeStreamingTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Streaming/residency_controller.h|residency_controller.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h`; объявленные типы: `IResidencyController`
- [[11 API/EngineRuntime/Streaming/streaming_priority_resolver.h|streaming_priority_resolver.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h`; объявленные типы: `IStreamingPriorityResolver`
- [[11 API/EngineRuntime/Streaming/streaming_runtime.h|streaming_runtime.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h`; объявленные типы: `IStreamingRuntime`, `IStreamingQuery`, `IStreamingController`, `StreamingServices`, `StreamingDependencies`
- [[11 API/EngineRuntime/Streaming/streaming_sources.h|streaming_sources.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h`; объявленные типы: `IStreamingWorldSource`, `IStreamingPersistenceSource`, `IStreamingResourceSource`, `IStreamingDataSource`, `IStreamingCommitTarget`, `IStreamingPriorityProvider`
- [[11 API/EngineRuntime/Streaming/streaming_types.h|streaming_types.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_types.h`; объявленные типы: `StreamingState`, `StreamingPriorityClass`, `StreamingRequestId`, `StreamingRequestHandle`, `StreamingDemandId`, `StreamingDemandHandle`, `ChunkStreamingTarget`, `RegionStreamingTarget`, `AssetStreamingTarget`, `ObjectStreamingTarget`, `ResourceGroupStreamingTarget`, `StreamingPlanStep`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/streaming|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Streaming координирует residency targets под byte/CPU budget. Он объединяет demands нескольких consumers, выполняет progressive load plan, commit/rollback и управляет жизненным циклом request от `Requested` до unload. Сам major не знает World, Resources или Persistence.

#### Контракты

`IStreamingRuntime` принимает demands, выполняет Tick и предоставляет progress/statistics. `IStreamingQuery` является read-only view, `IStreamingController` отвечает за cancellation и terminal shutdown. `StreamingDependencies` содержит data/commit/priority/residency/world/persistence/resource ports; concrete Support adapter передается через обычный public factory.

Несколько consumers одного target получают отдельные demand handles, но разделяют request. Во время необратимого `Unloading` существует не более одного successor. Последний released demand отменяет waiting successor. Terminal history не удаляет mapping, если он уже принадлежит более новому request.

`ProgressiveLoadPlan` начинает выполнение с cursor zero и не принимает pre-processed steps. `Commit` может отсутствовать, но если присутствует, он обязан быть единственным и последним шагом. Это гарантирует, что external commit не произойдёт до полного завершения progressive plan.

`IStreamingDataSource::ExecuteStep` имеет явную acceptance semantics. Failure/exception означает, что logical step unit не был принят. Success принимается Runtime ровно один раз. Принятый `StreamingStepResult` сначала сохраняется во внутреннем fixed-size pending state вместе с cursor и budget identity. Если после этого локальная publication не завершилась, следующий Tick продолжает suffix из pending result и не вызывает `ExecuteStep` повторно. `completed=false` разрешает следующий callback того же cursor только после успешного accounting предыдущего partial result.

Runtime-owned allocation failure при подготовке Tick, включая work list и result capacity, пробрасывается как `std::bad_alloc` до mutation request/statistics. Такой fault не конвертируется в `streaming.runtime_exception` и не увеличивает semantic `failed`. External port exceptions по-прежнему содержатся на своих узких callback boundaries.

#### Standard Support integration

Standard Support adapter связывает Streaming с World, Resources и Persistence через neutral manifest. Он начинает загрузку только тогда, когда сам переводит World chunk `Unloaded → Loading`, поэтому rollback не отменяет чужой transition. Подготовленные Resource leases становятся active только после commit. При unload сначала выполняется переход в `Unloading`, затем освобождаются active leases и только после этого World становится `Unloaded`.

Persistence override читается как detached data и доступен через Support `IStreamingPreparedChunkDataQuery`, пока chunk находится в подготовленном/загруженном lifetime. Streaming major не интерпретирует gameplay payload persistence records.

#### Shutdown

После начала shutdown новые demands отклоняются. Failed rollback/unload сохраняет ownership для следующего вызова. Успешный shutdown означает отсутствие live demands/requests и временного/resident ownership. Resource release и residency unload имеют durable phase markers, поэтому успешно завершённый cleanup prefix не выполняется повторно после более позднего failure.

#### Стабильность

Модуль прошёл локальный Goal 3 Streaming audit и canonical evidence convergence и имеет статус `LOCAL_READY` для перехода к системным Goals 5–9. Статус `FROZEN` здесь не утверждается до whole-engine qualification. Cross-major policy остаётся в Support.

Источник контрактов: `Documentation/EngineRuntime/modules/streaming.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `StreamingState` (`EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_types.h`): `NotRequested`, `Requested`, `Queued`, `Loading`, `Loaded`, `Activating`, `Active`, `Resident`, `Deactivating`, `Unloading`, `WaitingForPredecessor`, `Unloaded`, `Cancelled`, `Failed` и другие значения.
- `StreamingPriorityClass` (`EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_types.h`): `Critical`, `High`, `Normal`, `Low`, `Background`.
- `StreamingPlanStep` (`EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_types.h`): `ResolveTarget`, `PrepareData`, `PrepareResources`, `Commit`, `Rollback`, `Release`.

### Операции и хранилища состояния

#### `IStreamingRuntime`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingRuntime() = default;
[[nodiscard]] virtual foundation::Result<StreamingDemandHandle> Request( const StreamingTarget& target, StreamingPriorityClass priority) = 0;
[[nodiscard]] virtual foundation::Result<void> ReleaseDemand(StreamingDemandHandle demand) = 0;
[[nodiscard]] virtual StreamingState GetChunkState(ChunkId chunk) const = 0;
virtual void SetBudget(const StreamingBudget& budget) = 0;
[[nodiscard]] virtual std::optional<StreamingProgress> GetProgress(StreamingRequestHandle request) const = 0;
[[nodiscard]] virtual StreamingStatistics GetStatistics() const = 0;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `an already accepted external step is retained in`.

#### `IStreamingQuery`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h:35`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingQuery() = default;
[[nodiscard]] virtual StreamingState GetChunkState(ChunkId chunk) const = 0;
[[nodiscard]] virtual std::optional<StreamingProgress> GetProgress(StreamingRequestHandle request) const = 0;
[[nodiscard]] virtual StreamingStatistics GetStatistics() const = 0;
```

#### `IResidencyController`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IResidencyController() = default;
[[nodiscard]] virtual foundation::Result<void> ActivateChunk(ChunkId chunk) = 0;
[[nodiscard]] virtual foundation::Result<void> DeactivateChunk(ChunkId chunk) = 0;
[[nodiscard]] virtual foundation::Result<void> UnloadChunk(ChunkId chunk) = 0;
```

#### `IStreamingController`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingController() = default;
[[nodiscard]] virtual foundation::Result<void> CancelRequest(StreamingRequestHandle request) = 0;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

#### `IStreamingPriorityResolver`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h:9`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingPriorityResolver() = default;
[[nodiscard]] virtual StreamingPriorityClass ResolvePriority(ChunkId chunk) const = 0;
```

#### `IStreamingWorldSource`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingWorldSource() = default;
[[nodiscard]] virtual std::optional<RegionId> ResolveRegion(ChunkId chunk) const = 0;
```

#### `IStreamingPersistenceSource`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingPersistenceSource() = default;
[[nodiscard]] virtual foundation::Result<void> PrepareChunkData(const StreamingRequest& request) = 0;
```

#### `IStreamingResourceSource`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h:28`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] virtual foundation::Result<void> PrepareChunkResources(const StreamingRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> ReleaseChunkResources(ChunkId chunk) = 0;
```

#### `IStreamingDataSource`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h:39`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] virtual foundation::Result<ProgressiveLoadPlan> BuildLoadPlan(const StreamingRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<StreamingStepResult> ExecuteStep( const StreamingRequest& request, StreamingPlanStepRecord step, const RuntimeBudget& available_budget) = 0;
```

#### `IStreamingCommitTarget`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] virtual foundation::Result<void> Commit(const StreamingRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> Rollback(const StreamingRequest& request) = 0;
```

#### `IStreamingPriorityProvider`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingPriorityProvider() = default;
[[nodiscard]] virtual StreamingPriorityClass GetPriority(const StreamingTarget& target) const = 0;
```

#### `StreamingServices`

Объявлен в `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h:54`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IStreamingRuntime> runtime`; `std::shared_ptr<IStreamingQuery> query`; `std::shared_ptr<IStreamingController> controller`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Streaming/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp` (1696 строк): `Error::Create`, `InMemoryResidencyController::ActivateChunk`, `InMemoryResidencyController::DeactivateChunk`, `InMemoryResidencyController::UnloadChunk`, `InMemoryResidencyController::GetResidencyState`, `StreamingRuntime::StreamingRuntime`, `StreamingRuntime::Request`, `StreamingRuntime::ReleaseDemand`, `StreamingRuntime::ReleaseLastDemand`, `StreamingRuntime::Shutdown`
- `EngineRuntime/Streaming/src/streaming.cpp` (30 строк)

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `streaming.completion_sequence_overflow`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1549`
- `streaming.revision_overflow`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1073`
- `streaming.unsupported_target`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1507`
- `streaming.invalid_chunk`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:174`
- `streaming.invalid_residency_transition`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:138`
- `streaming.allocation_failed`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1516`
- `streaming.shutdown`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:160`
- `streaming.invalid_priority`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:195`
- `streaming.priority_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:191`
- `streaming.invalid_successor_state`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:212`
- `streaming.id_overflow`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:236`
- `streaming.data_source_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1371`
- `streaming.duplicate_request_id`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:311`
- `streaming.invalid_predecessor_state`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:382`
- `streaming.duplicate_demand_id`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:408`
- `streaming.backend_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:442`
- `streaming.residency_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1531`
- `streaming.internal_error`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:472`
- `streaming.invalid_handle`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:733`
- `streaming.stale_handle`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:738`
- `streaming.reference_underflow`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:500`
- `streaming.shutdown_allocation_failed`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:665`
- `streaming.shutdown_incomplete`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:722`
- `streaming.active_demands`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1566`
- `streaming.runtime_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:879`
- `streaming.byte_counter_overflow`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1434`
- `streaming.invalid_plan`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1057`
- `streaming.invalid_plan_step`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1356`
- `streaming.resource_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1274`
- `streaming.extension_exception`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1402`
- `streaming.pending_step_mismatch`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1417`
- `streaming.step_budget_violation`: `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1428`

### Конкретные тестовые якоря

- `EngineRuntime/Streaming/tests/streaming_tests.cpp`: `TestTwoConsumersRequestOneTarget`, `TestOneConsumerReleaseKeepsLoad`, `TestLastConsumerReleaseCancelsBeforeLoad`, `TestLastConsumerReleaseUnloadsResident`, `TestCancelDuringPartialLoadRollsBack`, `TestRollbackOnFailedStep`, `TestBudgetsLimitItemsAndBytes`, `TestByteBudgetUsesEstimatedBytes`, `TestIncompleteStepDoesNotAdvanceCursor`, `TestPartialStepUsesRemainingByteBudget`, `TestZeroProcessedIncompleteStepDoesNotAutoComplete`, `TestCommitRunsOnlyAfterCompletedStepAndOnlyOnce`, `TestCommitBudgetValidationRunsBeforeExternalCommit`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `streaming.completion_sequence_overflow` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1549`)

```cpp
1547     if (!CanAllocateMonotonicId(next_completion_sequence_))
 1548     {
 1549         return StreamingFailureValue<PreparedTerminalTransition>("streaming.completion_sequence_overflow", "streaming completion sequence allocator is exhausted");
 1550     }
 1551     return foundation::Result<PreparedTerminalTransition>::Success(PreparedTerminalTransition{next_completion_sequence_});
```

#### `streaming.revision_overflow` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1073`)

```cpp
1071     if (record.revision > std::numeric_limits<std::uint64_t>::max() - count)
 1072     {
 1073         return StreamingFailure("streaming.revision_overflow", "streaming request revision cannot advance beyond UINT64_MAX");
 1074     }
 1075     return foundation::Result<void>::Success();
```

#### `streaming.unsupported_target` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1507`)

```cpp
1505     if (!revision) return revision;
 1506     const auto chunk = GetChunkTarget(record.request.target);
 1507     if (!chunk) return StreamingFailure("streaming.unsupported_target", "reference streaming runtime only deactivates chunk targets");
 1508     if (!chunk_states_.contains(*chunk))
 1509     {
```

#### `streaming.invalid_chunk` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:174`)

```cpp
172     if (!chunk.IsValid())
  173     {
  174         return StreamingFailureValue<StreamingDemandHandle>("streaming.invalid_chunk", "chunk id must be valid before requesting streaming");
  175     }
  176
```

#### `streaming.invalid_residency_transition` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:138`)

```cpp
136     if (iterator->second != StreamingState::Deactivating)
  137     {
  138         return StreamingFailure("streaming.invalid_residency_transition", "chunk can only be unloaded after deactivation");
  139     }
  140     iterator->second = StreamingState::Unloaded;
```

#### `streaming.allocation_failed` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1516`)

```cpp
1514         catch (...)
 1515         {
 1516             return StreamingFailure("streaming.allocation_failed", "chunk state could not be prepared before deactivation");
 1517         }
 1518     }
```

#### `streaming.shutdown` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:160`)

```cpp
158     if (lifecycle_ != Lifecycle::Running)
  159     {
  160         return StreamingFailureValue<StreamingDemandHandle>("streaming.shutdown", "streaming runtime is not accepting new requests");
  161     }
  162     if (!IsValidPriority(priority))
```

#### `streaming.invalid_priority` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:195`)

```cpp
193     if (!IsValidPriority(resolved_priority))
  194     {
  195         return StreamingFailureValue<StreamingDemandHandle>("streaming.invalid_priority", "streaming priority provider returned an invalid enum value");
  196     }
  197
```

#### `streaming.priority_exception` (`EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:191`)

```cpp
189     catch (...)
  190     {
  191         return StreamingFailureValue<StreamingDemandHandle>("streaming.priority_exception", "streaming priority provider threw an exception");
  192     }
  193     if (!IsValidPriority(resolved_priority))
```
