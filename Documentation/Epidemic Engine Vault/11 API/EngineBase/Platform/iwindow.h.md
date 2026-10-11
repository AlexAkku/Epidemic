---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Platform/include/Epidemic/Platform/iwindow.h"
---

# Platform: iwindow.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Platform/include/Epidemic/Platform/iwindow.h`.

**Модуль:** [[04 EngineBase/Модуль Platform|Platform]].
**Размер исходного файла:** 1860 байт.
**Подключение:** `#include <Epidemic/Platform/iwindow.h>`.

## Объявления типов (лексический индекс)
`IWindow`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Platform/native_window_handle.h>
#include <Epidemic/Platform/platform_event.h>

#include <string_view>

namespace epidemic::platform
{
// This file defines the platform window abstraction consumed by EngineBase and smoke apps.
// A window exposes identity, size/focus state, and a minimal set of imperative lifecycle actions.

class IWindow
{
  public:
    virtual ~IWindow() = default;

    // Returns the EngineBase-assigned logical window identifier.
    [[nodiscard]] virtual WindowId Id() const noexcept = 0;

    // Returns the current UTF-8 title.
    [[nodiscard]] virtual std::string_view Title() const noexcept = 0;

    // Returns the native OS window handle wrapped in a portable carrier type.
    [[nodiscard]] virtual NativeWindowHandle GetNativeHandle() const noexcept = 0;

    // Returns the current client-area width in pixels.
    [[nodiscard]] virtual std::uint32_t ClientWidth() const noexcept = 0;

    // Returns the current client-area height in pixels.
    [[nodiscard]] virtual std::uint32_t ClientHeight() const noexcept = 0;

    // Returns the current DPI associated with the native window.
    [[nodiscard]] virtual std::uint32_t Dpi() const noexcept = 0;

    // Returns whether the window currently has input focus.
    [[nodiscard]] virtual bool HasFocus() const noexcept = 0;

    // Returns whether the window is currently minimized.
    [[nodiscard]] virtual bool IsMinimized() const noexcept = 0;

    // Returns whether a close request has been observed for this window.
    [[nodiscard]] virtual bool IsCloseRequested() const noexcept = 0;

    // Makes the window visible to the user.
    virtual void Show() = 0;

    // Requests final destruction of the native window.
    virtual void Close() = 0;
};
} // namespace epidemic::platform
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Platform/native_window_handle.h`, `Epidemic/Platform/platform_event.h`.

### Состав типов и операции

#### `IWindow` (строка заголовка 13)

Публичные сигнатуры операций:

```cpp
virtual ~IWindow() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

