---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_snapshot.h"
---

# Environment: environment_snapshot.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_snapshot.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 871 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/environment_snapshot.h>`.

## Объявления типов (лексический индекс)
`EnvironmentSnapshot`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Environment/climate_profile.h"
#include "Epidemic/Runtime/Environment/season_state.h"
#include "Epidemic/Runtime/Environment/surface_state.h"
#include "Epidemic/Runtime/Environment/weather_state.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <cstdint>
#include <vector>

namespace epidemic::runtime
{
// Detached runtime observation for one region. This type is not a persistence/restore format.
struct EnvironmentSnapshot
{
    RegionId region_id{};
    std::uint64_t revision = 0;
    WeatherState weather{};
    SeasonState season{};
    ClimateProfile climate{};
    float temperature = 0.0f;
    float humidity = 0.0f;
    std::vector<SurfaceState> surfaces;

    [[nodiscard]] bool operator==(const EnvironmentSnapshot&) const = default;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Environment/climate_profile.h`, `Epidemic/Runtime/Environment/season_state.h`, `Epidemic/Runtime/Environment/surface_state.h`, `Epidemic/Runtime/Environment/weather_state.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Состав типов и операции

#### `EnvironmentSnapshot` (строка заголовка 15)

Публичные поля и значения по умолчанию:

- `std::uint64_t revision = 0`
- `float temperature = 0.0f`
- `float humidity = 0.0f`
- `std::vector<SurfaceState> surfaces`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const EnvironmentSnapshot&) const = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

