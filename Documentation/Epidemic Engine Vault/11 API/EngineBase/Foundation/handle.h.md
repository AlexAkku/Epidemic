---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Foundation/include/Epidemic/Foundation/handle.h"
---

# Foundation: handle.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Foundation/include/Epidemic/Foundation/handle.h`.

**Модуль:** [[04 EngineBase/Модуль Foundation|Foundation]].
**Размер исходного файла:** 2518 байт.
**Подключение:** `#include <Epidemic/Foundation/handle.h>`.

## Объявления типов (лексический индекс)
`Handle`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace epidemic::foundation
{
// This file contains the generic typed handle used to reference engine-managed objects.
// Handle<TTag> does not own anything by itself; higher systems are expected to validate
// index/generation pairs against their own storage.

// Stable typed handle made of slot index and generation.
// Relationship: intended to be embedded in higher-level registries and object stores.
template <typename TTag> class Handle
{
  public:
    static constexpr std::uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max();

    // Builds the canonical invalid handle.
    constexpr Handle() noexcept = default;

    // Builds a handle from an explicit slot index and generation pair.
    // An invalid index always canonicalizes to generation 0.
    constexpr Handle(std::uint32_t index, std::uint32_t generation) noexcept
        : index_(index), generation_(index == kInvalidIndex ? 0u : generation)
    {
    }

    // Returns true when the handle points at a real slot index.
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return index_ != kInvalidIndex;
    }

    // Returns the slot index portion of the handle.
    [[nodiscard]] constexpr std::uint32_t Index() const noexcept
    {
        return index_;
    }

    // Returns the generation used to detect recycled slots.
    [[nodiscard]] constexpr std::uint32_t Generation() const noexcept
    {
        return generation_;
    }

    // Packs generation and index into a single 64-bit value.
    // Relationship: used by hashing and quick identity comparisons.
    [[nodiscard]] constexpr std::uint64_t Packed() const noexcept
    {
        return (static_cast<std::uint64_t>(generation_) << 32u) | index_;
    }

    // Handles compare by structural identity only.
    [[nodiscard]] constexpr bool operator==(const Handle &) const noexcept = default;

  private:
    std::uint32_t index_{kInvalidIndex};
    std::uint32_t generation_{0};
};
} // namespace epidemic::foundation

namespace std
{
template <typename TTag> struct hash<epidemic::foundation::Handle<TTag>>
{
    // Hashes the packed handle identity so Handle<TTag> can be used in unordered containers.
    [[nodiscard]] size_t operator()(const epidemic::foundation::Handle<TTag> &value) const noexcept
    {
        return hash<std::uint64_t>{}(value.Packed());
    }
};
} // namespace std
```
