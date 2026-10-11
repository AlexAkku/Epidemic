---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI/include/Epidemic/RHI/irhi_command_context.h"
---

# RHI: irhi_command_context.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI/include/Epidemic/RHI/irhi_command_context.h`.

**Модуль:** [[04 EngineBase/Модуль RHI|RHI]].
**Размер исходного файла:** 976 байт.
**Подключение:** `#include <Epidemic/RHI/irhi_command_context.h>`.

## Объявления типов (лексический индекс)
`IRhiCommandContext`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/result.h>
#include <Epidemic/RHI/descriptors.h>

namespace epidemic::rhi
{
// This file defines the command-context abstraction used by the baseline RHI.
// The current API is intentionally minimal: begin frame, clear, end frame.

class IRhiCommandContext
{
  public:
    virtual ~IRhiCommandContext() = default;

    // Begins recording or issuing commands for one frame.
    [[nodiscard]] virtual epidemic::foundation::Result<void> BeginFrame() = 0;

    // Executes a clear operation described by clear_desc.
    [[nodiscard]] virtual epidemic::foundation::Result<void> Clear(const RhiClearDesc &clear_desc) = 0;

    // Ends the active frame command scope.
    [[nodiscard]] virtual epidemic::foundation::Result<void> EndFrame() = 0;

    // Returns whether a frame is currently active on this context.
    [[nodiscard]] virtual bool IsFrameActive() const noexcept = 0;
};
} // namespace epidemic::rhi
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/RHI/descriptors.h`.

### Состав типов и операции

#### `IRhiCommandContext` (строка заголовка 11)

Публичные сигнатуры операций:

```cpp
virtual ~IRhiCommandContext() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

