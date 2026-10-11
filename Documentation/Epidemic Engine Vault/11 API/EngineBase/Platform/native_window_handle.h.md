---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Platform/include/Epidemic/Platform/native_window_handle.h"
---

# Platform: native_window_handle.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Platform/include/Epidemic/Platform/native_window_handle.h`.

**Модуль:** [[04 EngineBase/Модуль Platform|Platform]].
**Размер исходного файла:** 1279 байт.
**Подключение:** `#include <Epidemic/Platform/native_window_handle.h>`.

## Объявления типов (лексический индекс)
`NativeWindowHandle`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::platform
{
// This file defines the opaque carrier used to pass native window handles across module boundaries.
// The wrapper keeps platform-specific pointer types out of higher-level headers.

class NativeWindowHandle
{
  public:
    // Creates an empty handle.
    constexpr NativeWindowHandle() noexcept = default;

    // Wraps a raw native window pointer value.
    explicit constexpr NativeWindowHandle(void *value) noexcept : value_(value)
    {
    }

    // Returns the wrapped raw pointer value.
    [[nodiscard]] constexpr void *Value() const noexcept
    {
        return value_;
    }

    // Returns the wrapped raw pointer cast to the requested native handle type.
    template <typename T> [[nodiscard]] constexpr T As() const noexcept
    {
        return static_cast<T>(value_);
    }

    // Returns whether the handle contains a non-null native pointer.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value_ != nullptr;
    }

    // Compares two wrapped native handle values.
    [[nodiscard]] constexpr bool operator==(const NativeWindowHandle &) const noexcept = default;

  private:
    void *value_{nullptr};
};
} // namespace epidemic::platform
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `NativeWindowHandle` (строка заголовка 8)

Публичные сигнатуры операций:

```cpp
// Wraps a raw native window pointer value. explicit constexpr NativeWindowHandle(void *value) noexcept : value_(value);
// Returns the wrapped raw pointer value. [[nodiscard]] constexpr void *Value() const noexcept;
// Returns the wrapped raw pointer cast to the requested native handle type. template <typename T> [[nodiscard]] constexpr T As() const noexcept;
// Returns whether the handle contains a non-null native pointer. [[nodiscard]] constexpr bool IsValid() const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

