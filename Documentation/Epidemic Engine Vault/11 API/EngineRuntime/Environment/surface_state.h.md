---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/surface_state.h"
---

# Environment: surface_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/surface_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 1383 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/surface_state.h>`.

## Объявления типов (лексический индекс)
`SurfaceConditionKind`, `SurfaceState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <cstdint>

namespace epidemic::runtime
{
enum class SurfaceConditionKind
{
    Dry,
    Wet,
    Muddy,
    SnowCovered,
    Icy,
    Frozen,
    Thawing,
    Drying,
};

// condition is derived from wetness/snow/mud/ice by Environment mutations; caller-provided condition is not authoritative.
struct SurfaceState
{
    SurfaceId surface_id{};
    RegionId region_id{};
    SurfaceConditionKind condition = SurfaceConditionKind::Dry;
    float wetness = 0.0f;
    float snow_depth = 0.0f;
    float mud_depth = 0.0f;
    float ice_thickness = 0.0f;
    float temperature = 0.0f;
    std::uint64_t revision = 0;

    [[nodiscard]] constexpr bool operator==(const SurfaceState&) const noexcept = default;
};

[[nodiscard]] constexpr SurfaceConditionKind DeriveSurfaceCondition(const SurfaceState& state) noexcept
{
    if (state.ice_thickness > 0.0f)
    {
        return SurfaceConditionKind::Icy;
    }
    if (state.snow_depth > 0.0f)
    {
        return SurfaceConditionKind::SnowCovered;
    }
    if (state.mud_depth > 0.0f)
    {
        return SurfaceConditionKind::Muddy;
    }
    if (state.wetness > 0.0f)
    {
        return SurfaceConditionKind::Wet;
    }
    return SurfaceConditionKind::Dry;
}
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Перечисления и допустимые категории

- `SurfaceConditionKind`: `Dry`, `Wet`, `Muddy`, `SnowCovered`, `Icy`, `Frozen`, `Thawing`, `Drying`

### Состав типов и операции

#### `SurfaceState` (строка заголовка 22)

Публичные поля и значения по умолчанию:

- `SurfaceConditionKind condition = SurfaceConditionKind::Dry`
- `float wetness = 0.0f`
- `float snow_depth = 0.0f`
- `float mud_depth = 0.0f`
- `float ice_thickness = 0.0f`
- `float temperature = 0.0f`
- `std::uint64_t revision = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const SurfaceState&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

