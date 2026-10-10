---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h"
---

# Scene: transform_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1269 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/transform_registry.h>`.

## Объявления типов (лексический индекс)
`WorldTransformWriteMode`, `ITransformRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/scene_node.h"
#include "Epidemic/Runtime/Scene/transform.h"

#include <optional>

namespace epidemic::runtime
{
enum class WorldTransformWriteMode
{
    RejectNonInvertibleParent,
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ITransformRegistry
{
  public:
    virtual ~ITransformRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> SetLocalTransform(SceneNodeId node, const Transform& transform) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetWorldTransform(
        SceneNodeId node,
        const Transform& transform,
        WorldTransformWriteMode mode = WorldTransformWriteMode::RejectNonInvertibleParent) = 0;
    [[nodiscard]] virtual std::optional<Transform> GetLocalTransform(SceneNodeId node) const = 0;
    [[nodiscard]] virtual std::optional<Transform> GetWorldTransform(SceneNodeId node) const = 0;
    [[nodiscard]] virtual foundation::Result<void> MarkTransformClean(SceneNodeId node) = 0;
    [[nodiscard]] virtual bool IsTransformDirty(SceneNodeId node) const = 0;
};
} // namespace epidemic::runtime
```
