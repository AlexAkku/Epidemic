---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Input/include/Epidemic/Input/input_snapshot.h"
---

# Input: input_snapshot.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Input/include/Epidemic/Input/input_snapshot.h`.

**Модуль:** [[04 EngineBase/Модуль Input|Input]].
**Размер исходного файла:** 846 байт.
**Подключение:** `#include <Epidemic/Input/input_snapshot.h>`.

## Объявления типов (лексический индекс)
`InputSnapshot`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Input/keyboard_state.h>
#include <Epidemic/Input/mouse_state.h>

#include <cstdint>

namespace epidemic::input
{
// This file defines the frame-stable input snapshot published by InputSystem.
// Consumers read this snapshot after PublishSnapshot() to observe normalized keyboard and mouse state.

struct InputSnapshot
{
    std::uint64_t update_index{};
    KeyboardState keyboard;
    MouseState mouse;

    // Returns whether the owning window had focus when this snapshot was published.
    [[nodiscard]] bool HasFocus() const noexcept
    {
        return mouse.HasFocus();
    }

    // Returns whether the owning window had mouse capture when this snapshot was published.
    [[nodiscard]] bool HasCapture() const noexcept
    {
        return mouse.HasCapture();
    }
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Input/keyboard_state.h`, `Epidemic/Input/mouse_state.h`.

### Состав типов и операции

#### `InputSnapshot` (строка заголовка 13)

Публичные поля и значения по умолчанию:

- `KeyboardState keyboard`
- `MouseState mouse`

Публичные сигнатуры операций:

```cpp
// Returns whether the owning window had focus when this snapshot was published. [[nodiscard]] bool HasFocus() const noexcept;
// Returns whether the owning window had mouse capture when this snapshot was published. [[nodiscard]] bool HasCapture() const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

