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
