---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_operation.h"
---

# RuntimeFoundation: runtime_operation.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_operation.h`.

**Модуль:** [[05 EngineRuntime/Модуль RuntimeFoundation|RuntimeFoundation]].
**Размер исходного файла:** 620 байт.
**Подключение:** `#include <Epidemic/Runtime/Foundation/runtime_operation.h>`.

## Объявления типов (лексический индекс)
`AsyncOperationStatus`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
enum class AsyncOperationStatus
{
    Pending,
    Running,
    WaitingForMainThread,
    PartiallyComplete,
    Completed,
    Failed,
    Cancelled,
};

[[nodiscard]] constexpr bool IsTerminalOperationStatus(AsyncOperationStatus status) noexcept
{
    return status == AsyncOperationStatus::Completed || status == AsyncOperationStatus::Failed ||
           status == AsyncOperationStatus::Cancelled;
}

[[nodiscard]] constexpr bool IsActiveOperationStatus(AsyncOperationStatus status) noexcept
{
    return !IsTerminalOperationStatus(status);
}
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `AsyncOperationStatus`: `Pending`, `Running`, `WaitingForMainThread`, `PartiallyComplete`, `Completed`, `Failed`, `Cancelled`

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

