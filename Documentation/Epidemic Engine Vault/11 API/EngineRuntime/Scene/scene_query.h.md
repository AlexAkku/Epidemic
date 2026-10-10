---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h"
---

# Scene: scene_query.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1508 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_query.h>`.

## Объявления типов (лексический индекс)
`SceneNodeSnapshot`, `SceneSnapshot`, `ISceneQuery`, `ISceneSnapshotProvider`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Scene/bounds.h"
#include "Epidemic/Runtime/Scene/scene_node.h"
#include "Epidemic/Runtime/Scene/transform.h"

#include <optional>
#include <vector>

namespace epidemic::runtime
{
struct SceneNodeSnapshot
{
    SceneNodeId id{};
    SceneNodeId parent{};
    Transform local_transform{};
    Transform world_transform{};
    std::optional<Aabb> local_bounds{};
    std::optional<Aabb> world_bounds{};
    SceneAttachmentState attachment = SceneAttachmentState::Detached;
    SceneMobility mobility = SceneMobility::Dynamic;
    SceneVisibilityState visibility = SceneVisibilityState::Visible;
    std::uint64_t revision = 0;
};

struct SceneSnapshot
{
    std::uint64_t revision = 0;
    std::vector<SceneNodeSnapshot> nodes;
};

class ISceneQuery
{
  public:
    virtual ~ISceneQuery() = default;

    [[nodiscard]] virtual std::vector<SceneNodeId> QueryAabb(const Aabb& bounds) const = 0;
    [[nodiscard]] virtual std::vector<SceneNodeId> QuerySphere(const Vec3& center, float radius) const = 0;
};

class ISceneSnapshotProvider
{
  public:
    virtual ~ISceneSnapshotProvider() = default;

    // Scene publishes simplified TRS snapshots. Shear-producing combinations, such as non-uniform parent
    // scale with rotated children, are represented by the deterministic RuntimeFoundation TRS approximation.
    [[nodiscard]] virtual SceneSnapshot CaptureSnapshot() const = 0;
};
} // namespace epidemic::runtime
```
