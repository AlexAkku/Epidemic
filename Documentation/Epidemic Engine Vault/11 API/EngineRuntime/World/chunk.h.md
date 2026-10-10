---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h"
---

# World: chunk.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 2394 байт.
**Подключение:** `#include <Epidemic/Runtime/World/chunk.h>`.

## Объявления типов (лексический индекс)
`ChunkDescriptor`, `ChangeChunkStateCommand`, `ChunkSnapshot`, `IChunkRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/World/world_state.h"
#include <cstdint>
#include <optional>

namespace epidemic::runtime
{
struct ChunkDescriptor
{
    ChunkId id{};
    RegionId region_id{};
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    [[nodiscard]] constexpr bool operator==(const ChunkDescriptor&) const noexcept = default;
};

struct ChangeChunkStateCommand
{
    ChunkId chunk{};
    std::uint64_t expected_revision = 0;
    ChunkState state = ChunkState::Unloaded;
};

struct ChunkSnapshot
{
    ChunkDescriptor descriptor{};
    ChunkState state = ChunkState::Unloaded;
    std::uint64_t revision = 0;
};

[[nodiscard]] constexpr bool CanTransition(ChunkState from, ChunkState to) noexcept
{
    switch (from)
    {
    case ChunkState::Unloaded:
        return to == ChunkState::Unloaded || to == ChunkState::Loading;
    case ChunkState::Loading:
        return to == ChunkState::Loading || to == ChunkState::Resident || to == ChunkState::Unloaded;
    case ChunkState::Resident:
        return to == ChunkState::Resident || to == ChunkState::Active || to == ChunkState::Sleeping ||
               to == ChunkState::Unloading;
    case ChunkState::Active:
        return to == ChunkState::Active || to == ChunkState::Resident || to == ChunkState::Sleeping ||
               to == ChunkState::Unloading;
    case ChunkState::Sleeping:
        return to == ChunkState::Sleeping || to == ChunkState::Active || to == ChunkState::Resident ||
               to == ChunkState::Unloading;
    case ChunkState::Unloading:
        return to == ChunkState::Unloading || to == ChunkState::Unloaded;
    }
    return false;
}

class IChunkRegistry
{
  public:
    virtual ~IChunkRegistry() = default;
    [[nodiscard]] virtual foundation::Result<void> RegisterChunk(ChunkDescriptor chunk) = 0;
    [[nodiscard]] virtual std::optional<ChunkDescriptor> FindChunk(ChunkId id) const = 0;
    [[nodiscard]] virtual foundation::Result<ChunkSnapshot> GetChunkSnapshot(ChunkId id) const = 0;
    [[nodiscard]] virtual foundation::Result<void> SetChunkState(ChangeChunkStateCommand command) = 0;
    virtual void Freeze() noexcept = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};
}
```
