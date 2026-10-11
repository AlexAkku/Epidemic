---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/time_snapshot.h"
---

# Time: time_snapshot.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_snapshot.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 999 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/time_snapshot.h>`.

## Объявления типов (лексический индекс)
`TimeSnapshot`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Time/game_calendar.h"
#include "Epidemic/Runtime/Time/game_time.h"
#include "Epidemic/Runtime/Time/time_scale.h"
#include "Epidemic/Runtime/Time/time_state.h"

#include <cstdint>

namespace epidemic::runtime
{
struct TimeSnapshot
{
    // Frame-facing observation only. Persistence must use TimeCheckpoint from
    // time_runtime.h because the deterministic fractional accumulator remainder
    // is intentionally not exposed here.
    GameTimePoint now{};
    // Transient result of the latest runtime operation. Snapshot revision tracks
    // authoritative clock state changes, not last_delta-only refreshes.
    GameDuration last_delta{};
    TimeScale time_scale{};
    bool paused = false;
    CalendarDate calendar{};
    DayPhase day_phase = DayPhase::Night;
    std::uint64_t revision = 0;

    [[nodiscard]] constexpr bool operator==(const TimeSnapshot&) const noexcept = default;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Time/game_calendar.h`, `Epidemic/Runtime/Time/game_time.h`, `Epidemic/Runtime/Time/time_scale.h`, `Epidemic/Runtime/Time/time_state.h`.

### Состав типов и операции

#### `TimeSnapshot` (строка заголовка 12)

Публичные поля и значения по умолчанию:

- `bool paused = false`
- `DayPhase day_phase = DayPhase::Night`
- `std::uint64_t revision = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const TimeSnapshot&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

