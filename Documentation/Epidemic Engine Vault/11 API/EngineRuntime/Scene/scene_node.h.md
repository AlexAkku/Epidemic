---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h"
---

# Scene: scene_node.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1351 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_node.h>`.

## Объявления типов (лексический индекс)
`SceneNodeId`, `SceneNode`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Scene/scene_state.h"

#include <cstdint>
#include <functional>

namespace epidemic::runtime
{
struct SceneNodeId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value != 0;
    }

    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept
    {
        return value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const SceneNodeId&) const noexcept = default;
};

struct SceneNode
{
    SceneNodeId id{};
    SceneNodeId parent_id{};
    SceneAttachmentState attachment_state = SceneAttachmentState::Detached;
    SceneMobility mobility = SceneMobility::Dynamic;
    SceneVisibilityState visibility = SceneVisibilityState::Visible;
    SceneDirtyMask dirty_flags = 0;
    std::uint64_t revision = 0;

    [[nodiscard]] constexpr bool operator==(const SceneNode&) const noexcept = default;
};
} // namespace epidemic::runtime

namespace std
{
template <> struct hash<epidemic::runtime::SceneNodeId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::SceneNodeId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.Raw());
    }
};
} // namespace std
```
