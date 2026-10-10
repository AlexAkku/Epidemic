---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/time_state.h"
---

# Time: time_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 295 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/time_state.h>`.

## Объявления типов (лексический индекс)
`DayPhase`, `TimeRuntimeState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
enum class DayPhase
{
    Dawn,
    Day,
    Dusk,
    Night,
};

enum class TimeRuntimeState
{
    Running,
    Paused,
    TimeScaleChanged,
    TimeJumped,
    DayChanged,
    PhaseChanged,
};
} // namespace epidemic::runtime
```
