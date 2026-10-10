---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_update.h"
---

# Environment: environment_update.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_update.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 988 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/environment_update.h>`.

## Объявления типов (лексический индекс)
`EnvironmentUpdateInput`, `EnvironmentStateUpdate`

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
#include "Epidemic/Runtime/Foundation/runtime_time.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace epidemic::runtime
{
struct EnvironmentUpdateInput
{
    GameTimePoint game_time{};
    GameDuration game_delta{};
    RegionId region_id{};

    [[nodiscard]] constexpr bool operator==(const EnvironmentUpdateInput&) const noexcept = default;
};

struct EnvironmentStateUpdate
{
    RegionId region{};
    std::uint64_t source_revision = 0;
    std::optional<WeatherState> weather;
    std::optional<SeasonState> season;
    std::optional<ClimateProfile> climate;
    std::vector<SurfaceState> surfaces;
};
} // namespace epidemic::runtime
```
