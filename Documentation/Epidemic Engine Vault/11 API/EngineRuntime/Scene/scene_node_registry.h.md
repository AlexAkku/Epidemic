---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h"
---

# Scene: scene_node_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1600 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_node_registry.h>`.

## Объявления типов (лексический индекс)
`ReparentMode`, `ISceneNodeRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/scene_node.h"

#include <optional>
#include <vector>

namespace epidemic::runtime
{
enum class ReparentMode
{
    KeepWorld,
    KeepLocal,
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ISceneNodeRegistry
{
  public:
    virtual ~ISceneNodeRegistry() = default;

    [[nodiscard]] virtual foundation::Result<SceneNodeId> CreateNode() = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyNode(SceneNodeId node) = 0;
    [[nodiscard]] virtual bool Exists(SceneNodeId node) const = 0;
    [[nodiscard]] virtual std::optional<SceneNode> GetNode(SceneNodeId node) const = 0;
    [[nodiscard]] virtual foundation::Result<void> AttachNode(SceneNodeId child, SceneNodeId parent, ReparentMode mode = ReparentMode::KeepWorld) = 0;
    [[nodiscard]] virtual foundation::Result<void> DetachNode(SceneNodeId child, ReparentMode mode = ReparentMode::KeepWorld) = 0;
    [[nodiscard]] virtual std::optional<SceneNodeId> GetParent(SceneNodeId child) const = 0;
    [[nodiscard]] virtual std::vector<SceneNodeId> GetChildren(SceneNodeId parent) const = 0;
    [[nodiscard]] virtual foundation::Result<void> SetMobility(SceneNodeId node, SceneMobility mobility) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetVisibility(SceneNodeId node, SceneVisibilityState visibility) = 0;
    [[nodiscard]] virtual std::uint64_t GetRevision() const = 0;
};
} // namespace epidemic::runtime
```
