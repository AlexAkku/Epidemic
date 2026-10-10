---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Platform/include/Epidemic/Platform/platform_event.h"
---

# Platform: platform_event.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Platform/include/Epidemic/Platform/platform_event.h`.

**Модуль:** [[04 EngineBase/Модуль Platform|Platform]].
**Размер исходного файла:** 1371 байт.
**Подключение:** `#include <Epidemic/Platform/platform_event.h>`.

## Объявления типов (лексический индекс)
`PlatformEventType`, `PlatformEvent`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>

namespace epidemic::platform
{
// This file defines the raw event payload emitted by the platform layer.
// PlatformEvent is intentionally a flat POD-style structure so the window runtime can queue events cheaply.

using WindowId = std::uint64_t;

constexpr WindowId kInvalidWindowId = 0;

enum class PlatformEventType : std::uint8_t
{
    None,
    WindowCloseRequested,
    WindowResized,
    WindowFocusChanged,
    WindowMinimized,
    WindowRestored,
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseWheel,
    MouseCaptureChanged,
};

// Carries one normalized platform event.
// Relationship: InputSystem consumes the input-related subset, while apps may inspect window events directly.
struct PlatformEvent
{
    PlatformEventType type{PlatformEventType::None};
    WindowId window_id{kInvalidWindowId};
    std::uint32_t client_width{};
    std::uint32_t client_height{};
    bool focused{};
    bool captured{};
    std::uint32_t key_code{};
    std::uint32_t scan_code{};
    bool repeated{};
    std::int32_t mouse_x{};
    std::int32_t mouse_y{};
    std::int32_t mouse_delta_x{};
    std::int32_t mouse_delta_y{};
    std::int32_t wheel_delta{};
    std::uint8_t mouse_button{};
};
} // namespace epidemic::platform
```
