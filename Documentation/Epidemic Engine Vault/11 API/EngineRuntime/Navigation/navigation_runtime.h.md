---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h"
---

# Navigation: navigation_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Navigation|Navigation]].
**Размер исходного файла:** 4055 байт.
**Подключение:** `#include <Epidemic/Runtime/Navigation/navigation_runtime.h>`.

## Объявления типов (лексический индекс)
`INavCostProvider`, `INavigationBackend`, `INavigationDataSource`, `INavigationObstacleSource`, `INavTileRegistry`, `INavigationRuntime`, `NavigationServices`, `NavigationDependencies`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Navigation/navigation_types.h"

#include <cstddef>
#include <memory>
#include <span>

namespace epidemic::runtime::navigation
{
// Public service contracts for the Navigation major. Implementations may be mock or backed by real nav data,
// but callers only see path queries, tile states and projection interfaces.

class INavCostProvider
{
public:
    virtual ~INavCostProvider() = default;

    // Observational callback. Implementations return a finite, non-negative traversal cost.
    // Exceptions are contained by Navigation and fail only the affected query.
    [[nodiscard]] virtual float GetTraversalCost(const NavCostQuery& query) const = 0;
};

class INavigationBackend
{
public:
    virtual ~INavigationBackend() = default;

    // Query callback is observational/read-replay-safe. A successful result is accepted by Runtime once and may be
    // held internally until byte budget permits publication; Runtime-local publication failure must not require a
    // second BuildPath call. Result failure or exception means no path result was accepted.
    [[nodiscard]] virtual foundation::Result<PathResult> BuildPath(const PathRequest& request, NavigationRevision revision) const = 0;
};

class INavigationDataSource
{
public:
    virtual ~INavigationDataSource() = default;

    // Observational revision read. Exceptions are contained at the Navigation boundary.
    [[nodiscard]] virtual NavigationRevision CurrentRevision(RegionId region) const = 0;
};

class INavigationObstacleSource
{
public:
    virtual ~INavigationObstacleSource() = default;

    // Observational projection. The returned span must remain valid while the current Navigation call consumes it.
    // Exceptions are contained and fail only the affected path query.
    [[nodiscard]] virtual std::span<const DynamicObstacle> ObstaclesForRegion(RegionId region) const = 0;
};

class INavTileRegistry
{
public:
    virtual ~INavTileRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterTile(NavTileId tile, NavTileState state) = 0;

    [[nodiscard]] virtual NavTileState GetTileState(NavTileId tile) const = 0;

    [[nodiscard]] virtual foundation::Result<void> MarkTileDirty(NavTileId tile) = 0;

    [[nodiscard]] virtual std::size_t RebuildDirtyTiles(RuntimeBudget budget) = 0;
};

class INavigationRuntime
{
public:
    virtual ~INavigationRuntime() = default;

    [[nodiscard]] virtual foundation::Result<PathQueryHandle> RequestPathHandle(const PathRequest& request) = 0;

    [[nodiscard]] virtual foundation::Result<void> CancelPath(PathQueryHandle handle) = 0;

    // Advances queries in deterministic handle order. max_items limits state transitions; max_bytes gates final path
    // publication by the actual prepared point count. Zero budget fields are unlimited.
    [[nodiscard]] virtual std::size_t Tick(RuntimeBudget budget) = 0;

    [[nodiscard]] virtual foundation::Result<PathQueryState> GetPathState(PathQueryHandle handle) const = 0;

    [[nodiscard]] virtual foundation::Result<PathResult> GetPathResult(PathQueryHandle handle) const = 0;
    [[nodiscard]] virtual foundation::Result<void> ReleasePathResult(PathQueryHandle handle) = 0;
};

struct NavigationServices
{
    std::shared_ptr<INavigationRuntime> runtime;
    std::shared_ptr<INavTileRegistry> tiles;
};

struct NavigationDependencies
{
    std::shared_ptr<INavigationBackend> backend;
    std::shared_ptr<INavigationDataSource> data_source;
    std::shared_ptr<INavCostProvider> cost_provider;
    std::shared_ptr<INavigationObstacleSource> obstacle_source;
};

[[nodiscard]] foundation::Result<NavigationServices> CreateNavigationServices(
    NavigationOptions options = {},
    NavigationDependencies dependencies = {});
[[nodiscard]] NavigationServices CreateMockNavigationServices(NavigationOptions options = {});
} // namespace epidemic::runtime::navigation
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Navigation/navigation_types.h`.

### Состав типов и операции

#### `INavCostProvider` (строка заголовка 15)

Публичные сигнатуры операций:

```cpp
virtual ~INavCostProvider() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `INavigationBackend` (строка заголовка 25)

Публичные сигнатуры операций:

```cpp
virtual ~INavigationBackend() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `INavigationDataSource` (строка заголовка 36)

Публичные сигнатуры операций:

```cpp
virtual ~INavigationDataSource() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `INavigationObstacleSource` (строка заголовка 45)

Публичные сигнатуры операций:

```cpp
virtual ~INavigationObstacleSource() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `INavTileRegistry` (строка заголовка 55)

Публичные сигнатуры операций:

```cpp
virtual ~INavTileRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterTile(NavTileId tile, NavTileState state) = 0;
[[nodiscard]] virtual NavTileState GetTileState(NavTileId tile) const = 0;
[[nodiscard]] virtual foundation::Result<void> MarkTileDirty(NavTileId tile) = 0;
[[nodiscard]] virtual std::size_t RebuildDirtyTiles(RuntimeBudget budget) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `INavigationRuntime` (строка заголовка 69)

Публичные поля и значения по умолчанию:

- `max_bytes gates final path`

Публичные сигнатуры операций:

```cpp
virtual ~INavigationRuntime() = default;
[[nodiscard]] virtual foundation::Result<PathQueryHandle> RequestPathHandle(const PathRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> CancelPath(PathQueryHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<PathQueryState> GetPathState(PathQueryHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<PathResult> GetPathResult(PathQueryHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<void> ReleasePathResult(PathQueryHandle handle) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `NavigationServices` (строка заголовка 88)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<INavigationRuntime> runtime`
- `std::shared_ptr<INavTileRegistry> tiles`

#### `NavigationDependencies` (строка заголовка 94)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<INavigationBackend> backend`
- `std::shared_ptr<INavigationDataSource> data_source`
- `std::shared_ptr<INavCostProvider> cost_provider`
- `std::shared_ptr<INavigationObstacleSource> obstacle_source`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterTile` | `NavigationRuntime::RegisterTile` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:52` |
| `GetTileState` | `NavigationRuntime::GetTileState` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:87` |
| `MarkTileDirty` | `NavigationRuntime::MarkTileDirty` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:98` |
| `RebuildDirtyTiles` | `NavigationRuntime::RebuildDirtyTiles` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:131` |
| `RequestPathHandle` | `NavigationRuntime::RequestPathHandle` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:169` |
| `CancelPath` | `NavigationRuntime::CancelPath` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:230` |
| `GetPathState` | `NavigationRuntime::GetPathState` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:333` |
| `GetPathResult` | `NavigationRuntime::GetPathResult` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:365` |
| `ReleasePathResult` | `NavigationRuntime::ReleasePathResult` | `EngineRuntime/Navigation/src/navigation_runtime_impl.cpp:399` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetPathState`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:287`
- `RequestPathHandle`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:301`
- `GetPathResult`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:315`
- `CancelPath`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:353`
- `GetTileState`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:383`
- `RegisterTile`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:384`
- `MarkTileDirty`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:386`
- `RebuildDirtyTiles`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:389`
- `ReleasePathResult`: `EngineRuntime/Navigation/tests/navigation_tests.cpp:503`

