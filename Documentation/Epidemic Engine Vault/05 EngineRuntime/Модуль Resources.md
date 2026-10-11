---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "resources"]
source_path: "EngineRuntime/Resources"
---

# EngineRuntime / Resources

Модуль: **Resources**. Resource requests, payload и leases.

**Реализация:** `EngineRuntime/Resources/`.
**CMake targets:** `EpidemicRuntimeResources`
**Локальные test targets:** `EpidemicRuntimeResourcesTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Resources/resource_dependency_graph.h|resource_dependency_graph.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h`; объявленные типы: `ResourceDependency`, `ResourceDependencySet`
- [[11 API/EngineRuntime/Resources/resource_handle.h|resource_handle.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_handle.h`; объявленные типы: `ResourceHandle`, `ResourceLease`
- [[11 API/EngineRuntime/Resources/resource_loader.h|resource_loader.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h`; объявленные типы: `ResourceLoadArtifact`, `IResourceLoader`
- [[11 API/EngineRuntime/Resources/resource_loader_registry.h|resource_loader_registry.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h`; объявленные типы: `IResourceLoaderRegistry`
- [[11 API/EngineRuntime/Resources/resource_manager.h|resource_manager.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_manager.h`; объявленные типы: `ResourceProcessingStats`, `ResourceMemoryStats`, `IResourceManager`
- [[11 API/EngineRuntime/Resources/resource_payload.h|resource_payload.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h`; объявленные типы: `IResourcePayload`, `ByteResourcePayload`
- [[11 API/EngineRuntime/Resources/resource_request.h|resource_request.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_request.h`; объявленные типы: `ResourceRequest`
- [[11 API/EngineRuntime/Resources/resource_result.h|resource_result.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_result.h`
- [[11 API/EngineRuntime/Resources/resource_services.h|resource_services.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_services.h`; объявленные типы: `ResourceOptions`, `ResourceServices`
- [[11 API/EngineRuntime/Resources/resource_state.h|resource_state.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_state.h`; объявленные типы: `ResourceState`
- [[11 API/EngineRuntime/Resources/resource_type.h|resource_type.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_type.h`; объявленные типы: `ResourceType`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/resources|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Resources загружает immutable payloads по `ResourceId`, управляет dependencies, memory budget, retries, cancellation, eviction и владением consumer. В отличие от Assets, Resource существует в конкретном runtime state и может находиться в очереди, загрузке, Ready, Failed или Evicted.

#### Владение

Consumer получает `ResourceLease`. Lease имеет собственный acquisition ID и освобождается один раз через `IResourceManager::Release()`. `ResourceHandle` используется для query и не дает права освободить чужое удержание. Dependency loads также принадлежат leases, поэтому rollback одной операции не снимает удержание другого consumer.

#### Публичные контракты

`IResourceLoaderRegistry` связывает `ResourceType` с `IResourceLoader`. Loader возвращает `ResourceLoadArtifact` с immutable `IResourcePayload`. `IResourceManager` принимает requests, обрабатывает pending jobs под budget, возвращает state/payload/statistics и освобождает leases. `ResourceDependencyGraph` защищает от cycles и дублирования.

#### Инварианты

Artifact ID и type должны совпадать с request. Null payload запрещен. Byte budget является soft budget текущего frame: начатая операция может завершить минимальную единицу работы, после чего новые jobs не берутся. Memory accounting использует overflow-safe arithmetic. Internal release failure записывается как invariant violation.

#### Статус Goal 3

После module-local квалификации блока C и общего serial convergence модуль имеет статус `LOCAL_READY`. Это не означает `SYSTEM_READY` или `FROZEN`; системные и финальные freeze gates остаются в последующих целях. Concrete decoders и loaders добавляются сверху без изменения manager API.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `resource_dependency_graph.h`: `ResourceDependency`, `ResourceDependencySet`.
- `resource_handle.h`: `ResourceHandle`, `ResourceLease`.
- `resource_loader.h`: `ResourceLoadArtifact`, `IResourceLoader`.
- `resource_loader_registry.h`: `IResourceLoaderRegistry`.
- `resource_manager.h`: `ResourceProcessingStats`, `ResourceMemoryStats`, `IResourceManager`.
- `resource_payload.h`: `IResourcePayload`, `ByteResourcePayload`.
- `resource_request.h`: `ResourceRequest`.
- `resource_result.h`: factory-функции или backend implementation без отдельного публичного типа.
- `resource_services.h`: `ResourceOptions`, `ResourceServices`.
- `resource_state.h`: `ResourceState`.
- `resource_type.h`: `ResourceType`.

Источник контрактов: `Documentation/EngineRuntime/modules/resources.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ResourceState` (`EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_state.h`): `Unknown`, `Unloaded`, `Queued`, `Loading`, `WaitingForDependencies`, `Ready`, `Failed`, `Evicting`, `Evicted`, `Reloading`.

### Операции и хранилища состояния

#### `IResourceManager`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_manager.h:39`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IResourceManager() = default;
[[nodiscard]] virtual foundation::Result<ResourceLease> RequestLease(ResourceRequest request) = 0;
[[nodiscard]] virtual foundation::Result<void> Release(ResourceLease lease) = 0;
[[nodiscard]] virtual foundation::Result<void> Evict(ResourceId id) = 0;
[[nodiscard]] virtual std::size_t EvictUnreferenced() = 0;
[[nodiscard]] virtual foundation::Result<void> ValidateHandle(ResourceHandle handle) const = 0;
[[nodiscard]] virtual ResourceState GetState(ResourceHandle handle) const = 0;
[[nodiscard]] virtual bool IsReady(ResourceHandle handle) const = 0;
[[nodiscard]] virtual std::optional<ResourceId> GetResourceId(ResourceHandle handle) const = 0;
[[nodiscard]] virtual ResourcePayloadPtr GetPayload(ResourceHandle handle) const = 0;
virtual void SetMemoryBudgetBytes(std::size_t bytes) = 0;
[[nodiscard]] virtual ResourceMemoryStats GetMemoryStatistics() const = 0;
```

#### `IResourceLoaderRegistry`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IResourceLoaderRegistry() = default;
[[nodiscard]] virtual IResourceLoader* FindLoader(ResourceType type) = 0;
[[nodiscard]] virtual const IResourceLoader* FindLoader(ResourceType type) const = 0;
[[nodiscard]] virtual bool HasLoader(ResourceType type) const = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

#### `IResourceLoader`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h:21`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IResourceLoader() = default;
[[nodiscard]] virtual ResourceType GetResourceType() const = 0;
[[nodiscard]] virtual foundation::Result<ResourceLoadArtifact> Load(ResourceRequest request) = 0;
```

#### `IResourcePayload`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IResourcePayload() = default;
[[nodiscard]] virtual std::size_t GetSizeBytes() const noexcept = 0;
```

#### `ResourceServices`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_services.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IResourceLoaderRegistry> loaders`; `std::shared_ptr<IResourceManager> manager`.

#### `ByteResourcePayload`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : bytes_(std::move(bytes));
[[nodiscard]] std::size_t GetSizeBytes() const noexcept override;
[[nodiscard]] const std::vector<std::byte>& Bytes() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> bytes_`.

#### `ResourceHandle`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_handle.h:8`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t generation = 0`.

#### `ResourceLease`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_handle.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ResourceAcquisitionId acquisition = 0`.

#### `ResourceDependency`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool required = true`.

#### `ResourceOptions`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_services.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::size_t memory_budget_bytes = 0`.

#### `ResourceDependencySet`

Объявлен в `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<ResourceDependency> dependencies`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Resources/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Resources/src/resources.cpp` (19 строк)
- `EngineRuntime/Resources/src/resource_dependency_graph.cpp` (66 строк): `ResourceDependencyGraph::SetDependencies`, `ResourceDependencyGraph::ReplaceDependencies`, `ResourceDependencyGraph::RemoveDependencies`, `ResourceDependencyGraph::FindDependencies`, `ResourceDependencyGraph::HasDependencies`
- `EngineRuntime/Resources/src/resource_manager.cpp` (1147 строк): `Error::Create`, `ResourceLoadQueue::Enqueue`, `ResourceLoadQueue::PopBack`, `ResourceLoadQueue::Front`, `ResourceLoadQueue::PopFrontNoThrow`, `ResourceLoadQueue::FailNextEnqueueForTesting`, `ResourceLoadQueue::IsEmpty`, `ResourceLoadQueue::Dequeue`, `ResourceLoadQueue::Size`, `ResourceWaitingQueue::Publish`
- `EngineRuntime/Resources/src/resource_loader_registry.cpp` (73 строк): `ResourceLoaderRegistry::RegisterLoader`, `Error::Create`, `ResourceLoaderRegistry::FindLoader`, `ResourceLoaderRegistry::HasLoader`, `ResourceLoaderRegistry::Freeze`, `ResourceLoaderRegistry::IsFrozen`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `resource.invalid_acquisition`: `EngineRuntime/Resources/src/resource_manager.cpp:246`
- `resource.type_mismatch`: `EngineRuntime/Resources/src/resource_manager.cpp:252`
- `resource.evicting`: `EngineRuntime/Resources/src/resource_manager.cpp:256`
- `resource.reference_overflow`: `EngineRuntime/Resources/src/resource_manager.cpp:260`
- `resource.duplicate_slot`: `EngineRuntime/Resources/src/resource_manager.cpp:277`
- `resource.duplicate_acquisition`: `EngineRuntime/Resources/src/resource_manager.cpp:307`
- `resource.allocation_failed`: `EngineRuntime/Resources/src/resource_manager.cpp:1009`
- `resource.acquire_exception`: `EngineRuntime/Resources/src/resource_manager.cpp:349`
- `resource.invalid_lease`: `EngineRuntime/Resources/src/resource_manager.cpp:479`
- `resource.stale_handle`: `EngineRuntime/Resources/src/resource_manager.cpp:581`
- `resource.acquisition_underflow`: `EngineRuntime/Resources/src/resource_manager.cpp:489`
- `resource.reference_underflow`: `EngineRuntime/Resources/src/resource_manager.cpp:493`
- `resource.invalid_id`: `EngineRuntime/Resources/src/resource_manager.cpp:698`
- `resource.not_found`: `EngineRuntime/Resources/src/resource_manager.cpp:510`
- `resource.in_use`: `EngineRuntime/Resources/src/resource_manager.cpp:514`
- `resource.invalid_handle`: `EngineRuntime/Resources/src/resource_manager.cpp:576`
- `resource.acquisition_id_exhausted`: `EngineRuntime/Resources/src/resource_manager.cpp:668`
- `resource.generation_exhausted`: `EngineRuntime/Resources/src/resource_manager.cpp:689`
- `resource.invalid_type`: `EngineRuntime/Resources/src/resource_manager.cpp:702`
- `resource.dependency_release_pending`: `EngineRuntime/Resources/src/resource_manager.cpp:763`
- `resource.dependency_cycle`: `EngineRuntime/Resources/src/resource_manager.cpp:957`
- `resource.loader_registry_missing`: `EngineRuntime/Resources/src/resource_manager.cpp:777`
- `resource.loader_not_found`: `EngineRuntime/Resources/src/resource_manager.cpp:961`
- `resource.loader_exception`: `EngineRuntime/Resources/src/resource_manager.cpp:824`
- `resource.loader_mismatched_id`: `EngineRuntime/Resources/src/resource_manager.cpp:849`
- `resource.loader_mismatched_type`: `EngineRuntime/Resources/src/resource_manager.cpp:855`
- `resource.invalid_payload`: `EngineRuntime/Resources/src/resource_manager.cpp:861`
- `resource.dependency_invalid_id`: `EngineRuntime/Resources/src/resource_manager.cpp:943`
- `resource.dependency_invalid_type`: `EngineRuntime/Resources/src/resource_manager.cpp:951`
- `resource.dependency_failed`: `EngineRuntime/Resources/src/resource_manager.cpp:1024`
- `resource.accounting_overflow`: `EngineRuntime/Resources/src/resource_manager.cpp:1101`
- `resource.memory_budget_exceeded`: `EngineRuntime/Resources/src/resource_manager.cpp:1110`

### Конкретные тестовые якоря

- `EngineRuntime/Resources/tests/resources_tests.cpp`: `TestBasicTypesAndStateTransitions`, `TestDependencyGraphStoresDependencies`, `TestLoaderRegistryNonOwningContract`, `TestRequestQueuesAndProcessLoadsPayload`, `TestProcessBudgetLimitsJobs`, `TestTimeBudgetStopsFurtherJobs`, `TestRepeatedReadyRequestDoesNotReload`, `TestTypeMismatchRejectsConflictingRequest`, `TestDependenciesLoadAndReleaseOnlyOnEvict`, `TestSharedDependencySurvivesUntilBothRootsEvict`, `TestDependencyCycleAcrossChainFails`, `TestMemoryBudgetFailureRollsBackPayload`, `TestMemoryBudgetOverflowCannotFit`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `resource.invalid_acquisition` (`EngineRuntime/Resources/src/resource_manager.cpp:246`)

```cpp
244     if (acquisition == 0)
  245     {
  246         return ResourceFailureValue<ResourceHandle>("resource.invalid_acquisition", "resource acquisition id must be valid");
  247     }
  248
```

#### `resource.type_mismatch` (`EngineRuntime/Resources/src/resource_manager.cpp:252`)

```cpp
250     if (slot != nullptr && slot->type.IsValid() && slot->type != request.type)
  251     {
  252         return ResourceFailureValue<ResourceHandle>("resource.type_mismatch", "resource id is already associated with a different type");
  253     }
  254     if (slot != nullptr && slot->state == ResourceState::Evicting)
```

#### `resource.evicting` (`EngineRuntime/Resources/src/resource_manager.cpp:256`)

```cpp
254     if (slot != nullptr && slot->state == ResourceState::Evicting)
  255     {
  256         return ResourceFailureValue<ResourceHandle>("resource.evicting", "resource cannot be acquired while eviction reconciliation is pending");
  257     }
  258     if (slot != nullptr && slot->reference_count == std::numeric_limits<std::uint32_t>::max())
```

#### `resource.reference_overflow` (`EngineRuntime/Resources/src/resource_manager.cpp:260`)

```cpp
258     if (slot != nullptr && slot->reference_count == std::numeric_limits<std::uint32_t>::max())
  259     {
  260         return ResourceFailureValue<ResourceHandle>("resource.reference_overflow", "resource reference count is exhausted");
  261     }
  262
```

#### `resource.duplicate_slot` (`EngineRuntime/Resources/src/resource_manager.cpp:277`)

```cpp
275             if (!inserted)
  276             {
  277                 return ResourceFailureValue<ResourceHandle>("resource.duplicate_slot", "resource slot unexpectedly already exists");
  278             }
  279             slot = &it->second;
```

#### `resource.duplicate_acquisition` (`EngineRuntime/Resources/src/resource_manager.cpp:307`)

```cpp
305                 cache_.Entries().erase(slot->id);
  306             }
  307             return ResourceFailureValue<ResourceHandle>("resource.duplicate_acquisition", "resource acquisition id already exists");
  308         }
  309
```

#### `resource.allocation_failed` (`EngineRuntime/Resources/src/resource_manager.cpp:1009`)

```cpp
1007                     return foundation::Result<bool>::Failure(released.GetError());
 1008                 }
 1009                 return ResourceFailureValue<bool>("resource.allocation_failed", "resource dependency ownership could not be published");
 1010             }
 1011         }
```

#### `resource.acquire_exception` (`EngineRuntime/Resources/src/resource_manager.cpp:349`)

```cpp
347             }
  348         }
  349         return ResourceFailureValue<ResourceHandle>("resource.acquire_exception", "resource acquisition failed with an unexpected exception");
  350     }
  351 }
```

#### `resource.invalid_lease` (`EngineRuntime/Resources/src/resource_manager.cpp:479`)

```cpp
477     if (!lease.IsValid())
  478     {
  479         return ResourceFailure("resource.invalid_lease", "resource lease must be valid before release");
  480     }
  481
```
