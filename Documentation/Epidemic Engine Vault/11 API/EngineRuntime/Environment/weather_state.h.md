---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/weather_state.h"
---

# Environment: weather_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/weather_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 612 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/weather_state.h>`.

## Объявления типов (лексический индекс)
`WeatherKind`, `WeatherState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
enum class WeatherKind
{
    Clear,
    Cloudy,
    Rain,
    Storm,
    Snow,
    Fog,
    Transitioning,
};

struct WeatherState
{
    WeatherKind kind = WeatherKind::Clear;
    float intensity = 0.0f;
    float cloudiness = 0.0f;
    float precipitation = 0.0f;
    float wind_speed = 0.0f;
    float wind_direction_degrees = 0.0f;
    float current_temperature = 0.0f;
    float current_humidity = 0.0f;

    [[nodiscard]] constexpr bool operator==(const WeatherState&) const noexcept = default;
};
} // namespace epidemic::runtime
```
