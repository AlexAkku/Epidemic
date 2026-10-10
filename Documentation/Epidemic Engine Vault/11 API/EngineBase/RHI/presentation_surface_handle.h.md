---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI/include/Epidemic/RHI/presentation_surface_handle.h"
---

# RHI: presentation_surface_handle.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI/include/Epidemic/RHI/presentation_surface_handle.h`.

**Модуль:** [[04 EngineBase/Модуль RHI|RHI]].
**Размер исходного файла:** 1345 байт.
**Подключение:** `#include <Epidemic/RHI/presentation_surface_handle.h>`.

## Объявления типов (лексический индекс)
`PresentationSurfaceHandle`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::rhi
{
// This file defines the opaque handle used by RHI swap chains to reference presentation surfaces.
// Higher layers pass native window handles through this wrapper without exposing backend-specific types here.

class PresentationSurfaceHandle
{
  public:
    // Creates an empty presentation handle.
    constexpr PresentationSurfaceHandle() noexcept = default;

    // Wraps a raw native presentation surface pointer.
    explicit constexpr PresentationSurfaceHandle(void *value) noexcept : value_(value)
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

    // Compares two wrapped presentation handles.
    [[nodiscard]] constexpr bool operator==(const PresentationSurfaceHandle &) const noexcept = default;

  private:
    void *value_{nullptr};
};
} // namespace epidemic::rhi
```
