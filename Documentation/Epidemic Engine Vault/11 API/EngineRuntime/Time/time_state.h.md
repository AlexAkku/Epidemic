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

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `DayPhase`: `Dawn`, `Day`, `Dusk`, `Night`
- `TimeRuntimeState`: `Running`, `Paused`, `TimeScaleChanged`, `TimeJumped`, `DayChanged`, `PhaseChanged`

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

