---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/time_events.h"
---

# Time: time_events.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_events.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 495 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/time_events.h>`.

## Объявления типов (лексический индекс)
`TimeEventKind`, `TimeEvent`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Time/time_snapshot.h"

namespace epidemic::runtime
{
enum class TimeEventKind
{
    TimeAdvanced,
    TimeScaleChanged,
    Paused,
    Resumed,
    TimeJumped,
    DayChanged,
    DayPhaseChanged,
};

struct TimeEvent
{
    TimeEventKind kind = TimeEventKind::TimeAdvanced;
    TimeSnapshot snapshot{};

    [[nodiscard]] constexpr bool operator==(const TimeEvent&) const noexcept = default;
};
} // namespace epidemic::runtime
```
