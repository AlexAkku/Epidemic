---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_projection.h"
---

# Environment: environment_projection.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_projection.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 591 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/environment_projection.h>`.

## Объявления типов (лексический индекс)
`EnvironmentProjection`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Environment/season_state.h"
#include "Epidemic/Runtime/Environment/weather_state.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <cstdint>

namespace epidemic::runtime
{
struct EnvironmentProjection
{
    RegionId region_id{};
    std::uint64_t revision = 0;
    WeatherState weather{};
    SeasonState season{};
    float temperature = 0.0f;
    float humidity = 0.0f;

    [[nodiscard]] constexpr bool operator==(const EnvironmentProjection&) const noexcept = default;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Environment/season_state.h`, `Epidemic/Runtime/Environment/weather_state.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Состав типов и операции

#### `EnvironmentProjection` (строка заголовка 11)

Публичные поля и значения по умолчанию:

- `std::uint64_t revision = 0`
- `float temperature = 0.0f`
- `float humidity = 0.0f`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const EnvironmentProjection&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

