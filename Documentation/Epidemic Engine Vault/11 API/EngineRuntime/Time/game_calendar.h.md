---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/game_calendar.h"
---

# Time: game_calendar.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_calendar.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 559 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/game_calendar.h>`.

## Объявления типов (лексический индекс)
`CalendarDefinition`, `CalendarDate`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>

namespace epidemic::runtime
{
struct CalendarDefinition
{
    std::uint32_t hours_per_day = 24;
    std::uint32_t days_per_month = 30;
    std::uint32_t months_per_year = 12;
};

struct CalendarDate
{
    std::int64_t year = 1;
    std::uint32_t month = 1;
    std::uint32_t day = 1;
    std::uint32_t hour = 0;
    std::uint32_t minute = 0;
    std::uint32_t second = 0;

    [[nodiscard]] constexpr bool operator==(const CalendarDate&) const noexcept = default;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `CalendarDefinition` (строка заголовка 7)

Публичные поля и значения по умолчанию:

- `std::uint32_t hours_per_day = 24`
- `std::uint32_t days_per_month = 30`
- `std::uint32_t months_per_year = 12`

#### `CalendarDate` (строка заголовка 14)

Публичные поля и значения по умолчанию:

- `std::int64_t year = 1`
- `std::uint32_t month = 1`
- `std::uint32_t day = 1`
- `std::uint32_t hour = 0`
- `std::uint32_t minute = 0`
- `std::uint32_t second = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const CalendarDate&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

