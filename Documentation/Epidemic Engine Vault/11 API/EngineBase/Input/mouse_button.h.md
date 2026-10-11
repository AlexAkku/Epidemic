---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Input/include/Epidemic/Input/mouse_button.h"
---

# Input: mouse_button.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Input/include/Epidemic/Input/mouse_button.h`.

**Модуль:** [[04 EngineBase/Модуль Input|Input]].
**Размер исходного файла:** 1483 байт.
**Подключение:** `#include <Epidemic/Input/mouse_button.h>`.

## Объявления типов (лексический индекс)
`MouseButton`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>
#include <string_view>

namespace epidemic::input
{
// This file defines the normalized mouse buttons recognized by EngineBase input code.
// The enum includes Unknown so platform backends can reject unsupported raw button values explicitly.

enum class MouseButton : std::uint8_t
{
    Left = 0,
    Right = 1,
    Middle = 2,
    X1 = 3,
    X2 = 4,
    Count = 5,
    Unknown = 255,
};

// Returns true only for concrete supported buttons.
[[nodiscard]] constexpr bool IsKnownMouseButton(MouseButton button) noexcept
{
    switch (button)
    {
    case MouseButton::Left:
    case MouseButton::Right:
    case MouseButton::Middle:
    case MouseButton::X1:
    case MouseButton::X2:
        return true;
    case MouseButton::Count:
    case MouseButton::Unknown:
        return false;
    }

    return false;
}

// Converts a normalized mouse button to a stable diagnostic name.
[[nodiscard]] inline std::string_view ToString(MouseButton button) noexcept
{
    switch (button)
    {
    case MouseButton::Left:
        return "Left";
    case MouseButton::Right:
        return "Right";
    case MouseButton::Middle:
        return "Middle";
    case MouseButton::X1:
        return "X1";
    case MouseButton::X2:
        return "X2";
    case MouseButton::Count:
        return "Count";
    case MouseButton::Unknown:
        return "Unknown";
    }

    return "Unknown";
}
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `MouseButton`: `Left`, `Right`, `Middle`, `X1`, `X2`, `Count`, `Unknown`

### Состав типов и операции

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

