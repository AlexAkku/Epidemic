---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/object_placement.h"
---

# World: object_placement.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/object_placement.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 2289 байт.
**Подключение:** `#include <Epidemic/Runtime/World/object_placement.h>`.

## Объявления типов (лексический индекс)
`WorldSurfacePlacement`, `ContainerPlacement`, `InventoryPlacement`, `EquippedPlacement`, `HiddenPlacement`, `DestroyedPlacement`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include "Epidemic/Runtime/Foundation/spatial.h"

#include <optional>
#include <variant>

namespace epidemic::runtime
{
struct WorldSurfacePlacement
{
    RegionId region{};
    ChunkId chunk{};
    Transform transform{};

    [[nodiscard]] constexpr bool operator==(const WorldSurfacePlacement&) const noexcept = default;
};

struct ContainerPlacement
{
    RuntimeObjectId container{};
    foundation::StringId slot{};

    [[nodiscard]] constexpr bool operator==(const ContainerPlacement&) const noexcept = default;
};

struct InventoryPlacement
{
    RuntimeObjectId owner{};

    [[nodiscard]] constexpr bool operator==(const InventoryPlacement&) const noexcept = default;
};

struct EquippedPlacement
{
    RuntimeObjectId owner{};
    foundation::StringId slot{};

    [[nodiscard]] constexpr bool operator==(const EquippedPlacement&) const noexcept = default;
};

struct HiddenPlacement
{
    RegionId region{};

    [[nodiscard]] constexpr bool operator==(const HiddenPlacement&) const noexcept = default;
};

struct DestroyedPlacement
{
    GameTimePoint destroyed_at{};
    foundation::StringId reason{};

    [[nodiscard]] constexpr bool operator==(const DestroyedPlacement&) const noexcept = default;
};

using ObjectPlacement = std::variant<
    WorldSurfacePlacement,
    ContainerPlacement,
    InventoryPlacement,
    EquippedPlacement,
    HiddenPlacement,
    DestroyedPlacement>;

[[nodiscard]] inline std::optional<RegionId> GetPlacementRegion(const ObjectPlacement& placement)
{
    if (const auto* world = std::get_if<WorldSurfacePlacement>(&placement))
    {
        return world->region;
    }
    if (const auto* hidden = std::get_if<HiddenPlacement>(&placement))
    {
        return hidden->region;
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<ChunkId> GetPlacementChunk(const ObjectPlacement& placement)
{
    if (const auto* world = std::get_if<WorldSurfacePlacement>(&placement))
    {
        return world->chunk;
    }
    return std::nullopt;
}
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/runtime_time.h`, `Epidemic/Runtime/Foundation/spatial.h`.

### Состав типов и операции

#### `WorldSurfacePlacement` (строка заголовка 13)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const WorldSurfacePlacement&) const noexcept = default;
```

#### `ContainerPlacement` (строка заголовка 22)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const ContainerPlacement&) const noexcept = default;
```

#### `InventoryPlacement` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const InventoryPlacement&) const noexcept = default;
```

#### `EquippedPlacement` (строка заголовка 37)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const EquippedPlacement&) const noexcept = default;
```

#### `HiddenPlacement` (строка заголовка 45)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const HiddenPlacement&) const noexcept = default;
```

#### `DestroyedPlacement` (строка заголовка 52)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const DestroyedPlacement&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

