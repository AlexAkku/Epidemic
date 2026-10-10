---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h"
---

# Scene: spatial_index.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 780 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/spatial_index.h>`.

## Объявления типов (лексический индекс)
`ISpatialIndex`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/bounds.h"
#include "Epidemic/Runtime/Scene/scene_node.h"

#include <optional>

namespace epidemic::runtime
{
class ISpatialIndex
{
  public:
    virtual ~ISpatialIndex() = default;

    [[nodiscard]] virtual foundation::Result<void> SetLocalBounds(SceneNodeId node, const Aabb& bounds) = 0;
    [[nodiscard]] virtual std::optional<Aabb> GetLocalBounds(SceneNodeId node) const = 0;
    [[nodiscard]] virtual std::optional<Aabb> GetWorldBounds(SceneNodeId node) const = 0;
    [[nodiscard]] virtual foundation::Result<void> MarkBoundsClean(SceneNodeId node) = 0;
    [[nodiscard]] virtual bool IsBoundsDirty(SceneNodeId node) const = 0;
};
} // namespace epidemic::runtime
```
