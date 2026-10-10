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
