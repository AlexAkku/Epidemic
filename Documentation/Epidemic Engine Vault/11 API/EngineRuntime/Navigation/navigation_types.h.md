---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h"
---

# Navigation: navigation_types.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h`.

**Модуль:** [[05 EngineRuntime/Модуль Navigation|Navigation]].
**Размер исходного файла:** 3433 байт.
**Подключение:** `#include <Epidemic/Runtime/Navigation/navigation_types.h>`.

## Объявления типов (лексический индекс)
`NavTileId`, `PathQueryId`, `PathQueryHandle`, `DynamicObstacleId`, `NavTileState`, `PathQueryState`, `PathRequest`, `PathResult`, `NavigationRevision`, `NavCostQuery`, `DynamicObstacle`, `NavigationOptions`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_budget.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/spatial.h"

#include <cstdint>
#include <functional>
#include <chrono>
#include <vector>

namespace epidemic::runtime::navigation
{
// Public value types for the Navigation major. The types describe navigation tiles,
// path query lifecycle, traversal costs and obstacle projections without encoding AI decisions or movement rules.

struct NavTileId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const NavTileId&) const noexcept = default;
};

struct PathQueryId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const PathQueryId&) const noexcept = default;
};

struct PathQueryHandle
{
    PathQueryId id{};
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return id.IsValid() && generation != 0; }
    [[nodiscard]] constexpr bool operator==(const PathQueryHandle&) const noexcept = default;
};

struct DynamicObstacleId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const DynamicObstacleId&) const noexcept = default;
};

enum class NavTileState
{
    Missing,
    Queued,
    Loading,
    Ready,
    Dirty,
    Rebuilding,
    Failed,
    Unloaded
};

enum class PathQueryState
{
    Pending,
    Running,
    PartiallyComplete,
    Completed,
    Failed,
    Cancelled,
    Stale
};

struct PathRequest
{
    Vec3 start{};
    Vec3 target{};
    RegionId region{};
    std::uint64_t source_revision = 0;
    std::chrono::microseconds result_ttl{};
};

struct PathResult
{
    PathQueryHandle handle{};
    PathQueryState state = PathQueryState::Pending;
    std::vector<Vec3> points;
    std::uint64_t nav_revision = 0;
    std::uint64_t revision = 0;
};

struct NavigationRevision
{
    std::uint64_t value = 0;
};

struct NavCostQuery
{
    RegionId region{};
    SurfaceId surface{};
    Vec3 position{};
};

struct DynamicObstacle
{
    DynamicObstacleId id{};
    RegionId region{};
    Aabb bounds{};
    bool blocks_traversal = true;
    float cost_multiplier = 1.0f;
};

struct NavigationOptions
{
    bool enable_mock_queries = true;
};
} // namespace epidemic::runtime::navigation

namespace std
{
template <> struct hash<epidemic::runtime::navigation::NavTileId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::navigation::NavTileId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::navigation::PathQueryId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::navigation::PathQueryId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::navigation::DynamicObstacleId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::navigation::DynamicObstacleId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_budget.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/spatial.h`.

### Перечисления и допустимые категории

- `NavTileState`: `Missing`, `Queued`, `Loading`, `Ready`, `Dirty`, `Rebuilding`, `Failed`, `Unloaded`
- `PathQueryState`: `Pending`, `Running`, `PartiallyComplete`, `Completed`, `Failed`, `Cancelled`, `Stale`

### Состав типов и операции

#### `NavTileId` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NavTileId&) const noexcept = default;
```

#### `PathQueryId` (строка заголовка 25)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PathQueryId&) const noexcept = default;
```

#### `PathQueryHandle` (строка заголовка 33)

Публичные поля и значения по умолчанию:

- `std::uint32_t generation = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PathQueryHandle&) const noexcept = default;
```

#### `DynamicObstacleId` (строка заголовка 42)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const DynamicObstacleId&) const noexcept = default;
```

#### `PathRequest` (строка заголовка 73)

Публичные поля и значения по умолчанию:

- `std::uint64_t source_revision = 0`

#### `PathResult` (строка заголовка 82)

Публичные поля и значения по умолчанию:

- `PathQueryState state = PathQueryState::Pending`
- `std::vector<Vec3> points`
- `std::uint64_t nav_revision = 0`
- `std::uint64_t revision = 0`

#### `NavigationRevision` (строка заголовка 91)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

#### `DynamicObstacle` (строка заголовка 103)

Публичные поля и значения по умолчанию:

- `bool blocks_traversal = true`
- `float cost_multiplier = 1.0f`

#### `NavigationOptions` (строка заголовка 112)

Публичные поля и значения по умолчанию:

- `bool enable_mock_queries = true`

#### `hash` (строка заголовка 120)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::navigation::NavTileId value) const noexcept;
```

#### `hash` (строка заголовка 128)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::navigation::PathQueryId value) const noexcept;
```

#### `hash` (строка заголовка 136)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::navigation::DynamicObstacleId value) const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

