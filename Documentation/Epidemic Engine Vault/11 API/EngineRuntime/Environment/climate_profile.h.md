---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/climate_profile.h"
---

# Environment: climate_profile.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/climate_profile.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 364 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/climate_profile.h>`.

## Объявления типов (лексический индекс)
`ClimateProfile`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
struct ClimateProfile
{
    float average_temperature = 0.0f;
    float average_humidity = 0.0f;
    float average_wind_speed = 0.0f;
    float annual_precipitation = 0.0f;

    [[nodiscard]] constexpr bool operator==(const ClimateProfile&) const noexcept = default;
};
} // namespace epidemic::runtime
```
