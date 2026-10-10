---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI/include/Epidemic/RHI/irhi_swap_chain.h"
---

# RHI: irhi_swap_chain.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI/include/Epidemic/RHI/irhi_swap_chain.h`.

**Модуль:** [[04 EngineBase/Модуль RHI|RHI]].
**Размер исходного файла:** 1227 байт.
**Подключение:** `#include <Epidemic/RHI/irhi_swap_chain.h>`.

## Объявления типов (лексический индекс)
`IRhiSwapChain`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/result.h>
#include <Epidemic/RHI/descriptors.h>

namespace epidemic::rhi
{
// This file defines the swap-chain abstraction used by the baseline RHI.
// Swap chains own presentation buffers and expose resize/present operations through Result.

class IRhiSwapChain
{
  public:
    virtual ~IRhiSwapChain() = default;

    // Presents the current back buffer to the presentation surface.
    [[nodiscard]] virtual epidemic::foundation::Result<void> Present() = 0;

    // Resizes the swap-chain buffers.
    [[nodiscard]] virtual epidemic::foundation::Result<void> Resize(std::uint32_t width, std::uint32_t height) = 0;

    // Returns the current swap-chain width.
    [[nodiscard]] virtual std::uint32_t Width() const noexcept = 0;

    // Returns the current swap-chain height.
    [[nodiscard]] virtual std::uint32_t Height() const noexcept = 0;

    // Returns the number of buffers owned by the swap chain.
    [[nodiscard]] virtual std::uint32_t BufferCount() const noexcept = 0;

    // Returns the current color format of the swap chain.
    [[nodiscard]] virtual RhiPixelFormat ColorFormat() const noexcept = 0;
};
} // namespace epidemic::rhi
```
