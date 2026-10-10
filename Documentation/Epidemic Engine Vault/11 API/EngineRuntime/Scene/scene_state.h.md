---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h"
---

# Scene: scene_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1146 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_state.h>`.

## Объявления типов (лексический индекс)
`SceneAttachmentState`, `SceneMobility`, `SceneVisibilityState`, `SceneDirtyFlags`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>

namespace epidemic::runtime
{
enum class SceneAttachmentState
{
    Detached,
    Attached,
};

enum class SceneMobility
{
    Static,
    Dynamic,
};

enum class SceneVisibilityState
{
    Visible,
    Hidden,
};

enum class SceneDirtyFlags : std::uint32_t
{
    None = 0,
    Transform = 1u << 0,
    Bounds = 1u << 1,
    Hierarchy = 1u << 2,
    Visibility = 1u << 3,
};

using SceneDirtyMask = std::uint32_t;

[[nodiscard]] constexpr SceneDirtyMask ToSceneDirtyMask(SceneDirtyFlags flag) noexcept
{
    return static_cast<SceneDirtyMask>(flag);
}

[[nodiscard]] constexpr bool HasSceneDirtyFlag(SceneDirtyMask mask, SceneDirtyFlags flag) noexcept
{
    return (mask & ToSceneDirtyMask(flag)) != 0u;
}

[[nodiscard]] constexpr SceneDirtyMask AddSceneDirtyFlag(SceneDirtyMask mask, SceneDirtyFlags flag) noexcept
{
    return mask | ToSceneDirtyMask(flag);
}

[[nodiscard]] constexpr SceneDirtyMask ClearSceneDirtyFlag(SceneDirtyMask mask, SceneDirtyFlags flag) noexcept
{
    return mask & ~ToSceneDirtyMask(flag);
}
} // namespace epidemic::runtime
```
