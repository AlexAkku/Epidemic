---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_context.h"
---

# Foundation: gameplay_context.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_context.h`.

**Модуль:** [[06 EngineFramework/Модуль Foundation|Foundation]].
**Размер исходного файла:** 922 байт.
**Подключение:** `#include <Epidemic/GameFramework/Foundation/gameplay_context.h>`.

## Объявления типов (лексический индекс)
`GameplayContext`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/GameFramework/Foundation/ids.h"
#include "Epidemic/GameFramework/Foundation/time_types.h"

namespace epidemic::gameplay
{
struct GameplayContext
{
    GameplayTickId tick{};
    GameplayTimePoint time{};

    // Unique identity of this operation and the wider causal chain it belongs to.
    OperationId operation{};
    CorrelationId correlation{};

    // Actor performs this operation; instigator is the original responsible subject; source is the immediate effect source.
    GameplayObjectRef actor{};
    GameplayObjectRef instigator{};
    GameplayObjectRef source{};

    // Immediate predecessor, if this operation was caused by another operation or event.
    OperationId parent_operation{};
    EventId cause_event{};

    [[nodiscard]] constexpr bool operator==(const GameplayContext&) const noexcept = default;
};
} // namespace epidemic::gameplay
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/GameFramework/Foundation/ids.h`, `Epidemic/GameFramework/Foundation/time_types.h`.

### Состав типов и операции

#### `GameplayContext` (строка заголовка 8)

Публичные поля и значения по умолчанию:

- `instigator is the original responsible subject`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const GameplayContext&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

