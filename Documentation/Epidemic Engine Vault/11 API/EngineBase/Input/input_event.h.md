---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Input/include/Epidemic/Input/input_event.h"
---

# Input: input_event.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Input/include/Epidemic/Input/input_event.h`.

**Модуль:** [[04 EngineBase/Модуль Input|Input]].
**Размер исходного файла:** 967 байт.
**Подключение:** `#include <Epidemic/Input/input_event.h>`.

## Объявления типов (лексический индекс)
`InputEventType`, `InputEvent`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Input/key_code.h>
#include <Epidemic/Input/mouse_button.h>

#include <cstdint>

namespace epidemic::input
{
// This file defines the transient input events emitted during one PublishSnapshot() call.
// InputEvent complements InputSnapshot by preserving per-event details such as repeat and wheel deltas.

enum class InputEventType : std::uint8_t
{
    None,
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseWheel,
    FocusChanged,
    CaptureChanged,
};

struct InputEvent
{
    InputEventType type{InputEventType::None};
    KeyCode key_code{KeyCode::Unknown};
    MouseButton mouse_button{MouseButton::Left};
    bool repeated{};
    bool focused{};
    bool captured{};
    std::int32_t mouse_x{};
    std::int32_t mouse_y{};
    std::int32_t mouse_delta_x{};
    std::int32_t mouse_delta_y{};
    std::int32_t wheel_delta{};
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Input/key_code.h`, `Epidemic/Input/mouse_button.h`.

### Перечисления и допустимые категории

- `InputEventType`: `None`, `KeyPressed`, `KeyReleased`, `MouseMoved`, `MouseButtonPressed`, `MouseButtonReleased`, `MouseWheel`, `FocusChanged`, `CaptureChanged`

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

