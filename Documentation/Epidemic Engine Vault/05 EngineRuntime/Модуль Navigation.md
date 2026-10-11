---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "navigation"]
source_path: "EngineRuntime/Navigation"
---

# EngineRuntime / Navigation

Модуль: **Navigation**. Пути, данные навигации и запросы.

**Реализация:** `EngineRuntime/Navigation/`.
**CMake targets:** `EpidemicRuntimeNavigation`
**Локальные test targets:** `EpidemicRuntimeNavigationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Navigation/navigation.h|navigation.h]]: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation.h`
- [[11 API/EngineRuntime/Navigation/navigation_runtime.h|navigation_runtime.h]]: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h`; объявленные типы: `INavCostProvider`, `INavigationBackend`, `INavigationDataSource`, `INavigationObstacleSource`, `INavTileRegistry`, `INavigationRuntime`, `NavigationServices`, `NavigationDependencies`
- [[11 API/EngineRuntime/Navigation/navigation_types.h|navigation_types.h]]: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h`; объявленные типы: `NavTileId`, `PathQueryId`, `PathQueryHandle`, `DynamicObstacleId`, `NavTileState`, `PathQueryState`, `PathRequest`, `PathResult`, `NavigationRevision`, `NavCostQuery`, `DynamicObstacle`, `NavigationOptions`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/navigation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Navigation выполняет budgeted path queries по данным tiles, costs и dynamic obstacles. Он не определяет поведение NPC, не хранит gameplay goals и не владеет Environment -> Navigation projection adapter. Этот adapter проверяется в Runtime Support.

#### Tiles и revisions

`INavTileRegistry` принимает только valid `NavTileId`. Публичная регистрация допускает начальные состояния `Queued`, `Loading`, `Ready` и `Unloaded`; `Missing`, `Dirty`, `Rebuilding`, `Failed` и неизвестные enum values не являются допустимым initial state. Duplicate tile rejected.

`MarkTileDirty()` переводит `Ready -> Dirty`. Повторный `MarkTileDirty()` для уже `Dirty` является defined no-op и не увеличивает navigation revision. Rebuild выполняется детерминированно по возрастанию `NavTileId`: `Dirty -> Rebuilding -> Ready`. `max_items` ограничивает число state transitions за вызов. Tile rebuild не материализует byte payload, поэтому `max_bytes` сам по себе не ограничивает эти transitions. Navigation revision exhaustion проверяется до publication; при exhaustion tile state не меняется.

#### Path query identity и lifecycle

`INavigationRuntime` создаёт только handle-based queries. `PathQueryId` и generation являются monotonic non-zero identities; exhaustion rejected до публикации record. Неверная generation, released handle и уже удалённый TTL record являются stale handle.

Query проходит `Pending -> Running -> PartiallyComplete`, после чего становится `Completed`, `Failed`, `Cancelled` либо `Stale`. Cancel разрешён для non-terminal query; повторный cancel уже `Cancelled` является no-op. Другие terminal states не отменяются повторно. `Failed`, `Cancelled` и сохранённый `Stale` читаются локально без повторных external callbacks.

Completed result может иметь TTL. После expiration state читается как `Stale`; физическое удаление record происходит в `Tick()`. Tick сначала полностью готовит purge list и processing list. Если подготовка любого списка получает allocation failure, live query set остаётся без изменений.

#### Providers и backend boundary

`INavigationDataSource::CurrentRevision`, `INavCostProvider`, `INavigationObstacleSource` и `INavigationBackend` являются внешними neutral ports. Их exceptions не выходят как semantic success и не повреждают другие queries. Cost должен быть finite и non-negative. Reference path использует один подготовленный shape plan: blocking obstacle задаёт detour midpoint; если blocker отсутствует, cost > 1 добавляет deterministic midpoint. Один и тот же plan используется и для byte-budget decision, и для последующей публикации.

`INavigationBackend::BuildPath()` является observational/read-replay-safe query callback. Backend success принимается Runtime ровно один раз и сохраняется как durable pending result до локального commit. Поэтому недостаточный byte budget или Runtime-local allocation failure после callback не вызывают повторный `BuildPath()` для того же logical result. Backend failure/exception означает отсутствие принятого path result.

Backend result публикуется только если state = `Completed`, source revision совпадает с expected revision, path содержит минимум start и target, все points finite, первый point равен request start, последний point равен request target. Runtime переписывает handle и result revision своей authoritative identity.

#### Budget semantics

`Tick()` обрабатывает queries детерминированно по возрастанию `PathQueryId`. `max_items` ограничивает accepted state transitions. `max_bytes` применяется к фактически подготовленному path shape. Для reference path это сохранённый point count. Для backend path используется фактическое число возвращённых points. Если path не помещается в текущий byte budget, query остаётся `PartiallyComplete`, а подготовленный provider/backend result сохраняется для последующего Tick без повторного external callback.

`PathQueryState::PartiallyComplete` означает deferred local completion. Отдельного `NoPath` enum нет: backend/provider semantic inability to produce a valid path приводит к `Failed`. Изменение captured source revision делает старый query/result `Stale`.

#### Failure atomicity и scope

Runtime-owned allocation failure до semantic commit не превращается в `provider_exception` и не переводит query в ложный `Failed`. Подготовленный successful backend result или reference plan сохраняет ownership до publication. Result revision exhaustion проверяется до внешнего callback и не допускает side effect, который невозможно локально commit.

Navigation не имеет local `Shutdown()` API. Lifetime backend/providers принадлежит переданным dependencies. Environment -> Navigation projection и cross-major reconciliation относятся к `3.17 Support`, а не к локальному Navigation audit.

#### Статус freeze

Этот документ описывает проверенный локальный Goal 3 contract. После прохождения `EpidemicRuntimeNavigationTests` и canonical evidence convergence модуль имеет статус `LOCAL_READY`. `SYSTEM_READY` и `FROZEN` присваиваются только после последующих system-wide Goals 5–9.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `navigation.h`: factory-функции или backend implementation без отдельного публичного типа.
- `navigation_runtime.h`: `INavCostProvider`, `INavigationBackend`, `INavigationDataSource`, `INavigationObstacleSource`, `INavTileRegistry`, `INavigationRuntime`, `NavigationServices`, `NavigationDependencies`.
- `navigation_types.h`: `NavTileId`, `PathQueryId`, `PathQueryHandle`, `DynamicObstacleId`, `NavTileState`, `PathQueryState`, `PathRequest`, `PathResult`, `NavigationRevision`, `NavCostQuery`, `DynamicObstacle`, `NavigationOptions`.

Источник контрактов: `Documentation/EngineRuntime/modules/navigation.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `NavTileState` (`EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h`): `Missing`, `Queued`, `Loading`, `Ready`, `Dirty`, `Rebuilding`, `Failed`, `Unloaded`.
- `PathQueryState` (`EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h`): `Pending`, `Running`, `PartiallyComplete`, `Completed`, `Failed`, `Cancelled`, `Stale`.

### Операции и хранилища состояния

#### `INavigationRuntime`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:69`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavigationRuntime() = default;
[[nodiscard]] virtual foundation::Result<PathQueryHandle> RequestPathHandle(const PathRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> CancelPath(PathQueryHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<PathQueryState> GetPathState(PathQueryHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<PathResult> GetPathResult(PathQueryHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<void> ReleasePathResult(PathQueryHandle handle) = 0;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `max_bytes gates final path`.

#### `INavTileRegistry`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:55`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavTileRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterTile(NavTileId tile, NavTileState state) = 0;
[[nodiscard]] virtual NavTileState GetTileState(NavTileId tile) const = 0;
[[nodiscard]] virtual foundation::Result<void> MarkTileDirty(NavTileId tile) = 0;
[[nodiscard]] virtual std::size_t RebuildDirtyTiles(RuntimeBudget budget) = 0;
```

#### `INavCostProvider`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavCostProvider() = default;
```

#### `INavigationBackend`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:25`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavigationBackend() = default;
```

#### `INavigationDataSource`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:36`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavigationDataSource() = default;
```

#### `INavigationObstacleSource`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavigationObstacleSource() = default;
```

#### `NavigationServices`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h:88`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<INavigationRuntime> runtime`; `std::shared_ptr<INavTileRegistry> tiles`.

#### `NavTileId`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `PathQueryId`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h:25`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `PathQueryHandle`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h:33`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t generation = 0`.

#### `DynamicObstacleId`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h:42`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `hash`

Объявлен в `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h:120`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::navigation::NavTileId value) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Navigation/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Navigation/src/navigation.cpp` (35 строк): `Error::Create`
- `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp` (953 строк): `Error::Create`, `NavigationRuntime::NavigationRuntime`, `NavigationRuntime::RegisterTile`, `NavigationRuntime::GetTileState`, `NavigationRuntime::MarkTileDirty`, `NavigationRuntime::RebuildDirtyTiles`, `NavigationRuntime::RequestPathHandle`, `NavigationRuntime::CancelPath`, `NavigationRuntime::Tick`, `NavigationRuntime::GetPathState`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `navigation.backend_missing`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:179`
- `navigation.invalid_tile`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:103`
- `navigation.invalid_tile_state`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:62`
- `navigation.duplicate_tile`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:67`
- `navigation.out_of_memory`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:222`
- `navigation.tile_not_found`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:110`
- `navigation.invalid_tile_transition`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:115`
- `navigation.query_id_exhausted`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:183`
- `navigation.query_generation_exhausted`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:187`
- `navigation.duplicate_query_id`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:217`
- `navigation.stale_handle`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:405`
- `navigation.query_terminal`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:242`
- `navigation.stale_result`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:393`
- `navigation.query_not_completed`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:382`
- `navigation.provider_exception`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:877`
- `navigation.invalid_region`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:596`
- `navigation.invalid_coordinate`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:892`
- `navigation.invalid_ttl`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:606`
- `navigation.revision_exhausted`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:616`
- `navigation.result_revision_exhausted`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:626`
- `navigation.backend_failed`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:749`
- `navigation.invalid_provider_data`: `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:884`

### Конкретные тестовые якоря

- `EngineRuntime/Navigation/tests/navigation_tests.cpp`: `TestRequestPathCompletesThroughBudgetedStates`, `TestPathBudgetOrderIsDeterministic`, `TestCancelledQueryDoesNotProducePath`, `TestCancelPartiallyCompletedQuery`, `TestTileDirtyRebuildStates`, `TestCostAndObstacleProjectionsStayExternal`, `TestInvalidInputsReturnFailures`, `TestFactoryBackendSelectionAndFailure`, `TestHandleReleaseTtlAndStaleRevision`, `TestSourceRevisionZeroBecomesStaleAfterAdvance`, `TestInvalidNumericPathRequestsAreRejected`, `TestProviderExceptionsBecomeFailures`, `TestRepeatedDirtyIsNoOp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `navigation.backend_missing` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:179`)

```cpp
177     {
  178         return foundation::Result<PathQueryHandle>::Failure(
  179             foundation::Error::Create("navigation.backend_missing", "navigation backend is required when reference queries are disabled"));
  180     }
  181     if (!CanAllocateMonotonicId(next_query_value_))
```

#### `navigation.invalid_tile` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:103`)

```cpp
101     {
  102         return foundation::Result<void>::Failure(
  103             foundation::Error::Create("navigation.invalid_tile", "navigation tile id must be valid before it can be dirtied"));
  104     }
  105
```

#### `navigation.invalid_tile_state` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:62`)

```cpp
60     {
   61         return foundation::Result<void>::Failure(
   62             foundation::Error::Create("navigation.invalid_tile_state", "navigation tile initial state is not valid for registration"));
   63     }
   64     if (tiles_.contains(tile))
```

#### `navigation.duplicate_tile` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:67`)

```cpp
65     {
   66         return foundation::Result<void>::Failure(
   67             foundation::Error::Create("navigation.duplicate_tile", "navigation tile is already registered"));
   68     }
   69     if (auto revision = PreflightGlobalRevision(); !revision)
```

#### `navigation.out_of_memory` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:222`)

```cpp
220     catch (const std::bad_alloc&)
  221     {
  222         return NavFailure<PathQueryHandle>("navigation.out_of_memory", "navigation query registry could not allocate query record");
  223     }
  224
```

#### `navigation.tile_not_found` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:110`)

```cpp
108     {
  109         return foundation::Result<void>::Failure(
  110             foundation::Error::Create("navigation.tile_not_found", "navigation tile was not registered"));
  111     }
  112     if (!CanTransition(iterator->second, NavTileState::Dirty))
```

#### `navigation.invalid_tile_transition` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:115`)

```cpp
113     {
  114         return foundation::Result<void>::Failure(
  115             foundation::Error::Create("navigation.invalid_tile_transition", "navigation tile cannot be marked dirty from its current state"));
  116     }
  117     if (iterator->second == NavTileState::Dirty)
```

#### `navigation.query_id_exhausted` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:183`)

```cpp
181     if (!CanAllocateMonotonicId(next_query_value_))
  182     {
  183         return NavFailure<PathQueryHandle>("navigation.query_id_exhausted", "path query id allocator is exhausted");
  184     }
  185     if (!CanAllocateMonotonicId(next_generation_))
```

#### `navigation.query_generation_exhausted` (`EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:187`)

```cpp
185     if (!CanAllocateMonotonicId(next_generation_))
  186     {
  187         return NavFailure<PathQueryHandle>("navigation.query_generation_exhausted", "path query generation allocator is exhausted");
  188     }
  189
```
