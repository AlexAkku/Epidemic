---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/time_scale.h"
---

# Time: time_scale.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_scale.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 288 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/time_scale.h>`.

## Объявления типов (лексический индекс)
`TimeScale`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>

namespace epidemic::runtime
{
struct TimeScale
{
    std::int64_t numerator = 1;
    std::int64_t denominator = 1;

    [[nodiscard]] constexpr bool operator==(const TimeScale&) const noexcept = default;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `TimeScale` (строка заголовка 7)

Публичные поля и значения по умолчанию:

- `std::int64_t numerator = 1`
- `std::int64_t denominator = 1`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const TimeScale&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

