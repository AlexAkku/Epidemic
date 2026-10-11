---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/game_time.h"
---

# Time: game_time.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_time.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 282 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/game_time.h>`.

## Объявления типов (лексический индекс)
_Именованные `class`/`struct`/`enum` в этом заголовке не обнаружены простой лексической проверкой._

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_time.h"

namespace epidemic::runtime
{
using GameTime = GameTimePoint;

[[nodiscard]] constexpr bool IsZero(GameDuration duration) noexcept
{
    return duration.ticks == 0;
}
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_time.h`.

Этот заголовок преимущественно содержит free-functions, type aliases или implementation helpers. Точные сигнатуры смотрите в исходном блоке выше.

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

