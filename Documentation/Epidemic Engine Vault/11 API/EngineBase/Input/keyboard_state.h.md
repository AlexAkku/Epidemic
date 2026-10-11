---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Input/include/Epidemic/Input/keyboard_state.h"
---

# Input: keyboard_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Input/include/Epidemic/Input/keyboard_state.h`.

**Модуль:** [[04 EngineBase/Модуль Input|Input]].
**Размер исходного файла:** 2689 байт.
**Подключение:** `#include <Epidemic/Input/keyboard_state.h>`.

## Объявления типов (лексический индекс)
`KeyboardState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Input/key_code.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace epidemic::input
{
// This file defines the keyboard portion of the input snapshot.
// KeyboardState tracks persistent key-down state plus transient pressed/released flags for the current frame.

class KeyboardState
{
  public:
    static constexpr std::size_t KeyCount = 256;

    // Returns whether the key is currently held down.
    [[nodiscard]] bool IsKeyDown(KeyCode key_code) const noexcept
    {
        return IsKnownKeyCode(key_code) ? keys_down_[ToIndex(key_code)] : false;
    }

    // Returns whether the key transitioned to pressed during the current frame.
    [[nodiscard]] bool WasPressedThisFrame(KeyCode key_code) const noexcept
    {
        return IsKnownKeyCode(key_code) ? keys_pressed_[ToIndex(key_code)] : false;
    }

    // Returns whether the key transitioned to released during the current frame.
    [[nodiscard]] bool WasReleasedThisFrame(KeyCode key_code) const noexcept
    {
        return IsKnownKeyCode(key_code) ? keys_released_[ToIndex(key_code)] : false;
    }

    // Sets the persistent down/up state for a key.
    void SetKeyDown(KeyCode key_code, bool down) noexcept
    {
        if (IsKnownKeyCode(key_code))
        {
            keys_down_[ToIndex(key_code)] = down;
        }
    }

    // Marks a key as newly pressed for the current frame.
    void MarkPressed(KeyCode key_code) noexcept
    {
        if (IsKnownKeyCode(key_code))
        {
            keys_pressed_[ToIndex(key_code)] = true;
        }
    }

    // Marks a key as newly released for the current frame.
    void MarkReleased(KeyCode key_code) noexcept
    {
        if (IsKnownKeyCode(key_code))
        {
            keys_released_[ToIndex(key_code)] = true;
        }
    }

    // Clears only the per-frame pressed/released flags.
    void ClearTransient() noexcept
    {
        keys_pressed_.fill(false);
        keys_released_.fill(false);
    }

    // Clears both persistent and transient key state.
    void ClearAll() noexcept
    {
        keys_down_.fill(false);
        ClearTransient();
    }

  private:
    // Maps a validated key code to its backing-array index.
    [[nodiscard]] static constexpr std::size_t ToIndex(KeyCode key_code) noexcept
    {
        const auto index = static_cast<std::size_t>(static_cast<std::uint16_t>(key_code));
        return index < KeyCount ? index : 0;
    }

    std::array<bool, KeyCount> keys_down_{};
    std::array<bool, KeyCount> keys_pressed_{};
    std::array<bool, KeyCount> keys_released_{};
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Input/key_code.h`.

### Состав типов и операции

#### `KeyboardState` (строка заголовка 14)

Публичные поля и значения по умолчанию:

- `static constexpr std::size_t KeyCount = 256`

Публичные сигнатуры операций:

```cpp
// Returns whether the key is currently held down. [[nodiscard]] bool IsKeyDown(KeyCode key_code) const noexcept;
// Returns whether the key transitioned to pressed during the current frame. [[nodiscard]] bool WasPressedThisFrame(KeyCode key_code) const noexcept;
// Returns whether the key transitioned to released during the current frame. [[nodiscard]] bool WasReleasedThisFrame(KeyCode key_code) const noexcept;
// Sets the persistent down/up state for a key. void SetKeyDown(KeyCode key_code, bool down) noexcept;
// Marks a key as newly pressed for the current frame. void MarkPressed(KeyCode key_code) noexcept;
// Marks a key as newly released for the current frame. void MarkReleased(KeyCode key_code) noexcept;
// Clears only the per-frame pressed/released flags. void ClearTransient() noexcept;
// Clears both persistent and transient key state. void ClearAll() noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

