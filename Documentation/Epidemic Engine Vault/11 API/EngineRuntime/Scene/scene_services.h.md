---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h"
---

# Scene: scene_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 772 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_services.h>`.

## Объявления типов (лексический индекс)
`SceneOptions`, `SceneServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/scene_node_registry.h"
#include "Epidemic/Runtime/Scene/scene_query.h"
#include "Epidemic/Runtime/Scene/spatial_index.h"
#include "Epidemic/Runtime/Scene/transform_registry.h"

#include <memory>

namespace epidemic::runtime
{
struct SceneOptions
{
};

struct SceneServices
{
    std::shared_ptr<ISceneNodeRegistry> nodes;
    std::shared_ptr<ITransformRegistry> transforms;
    std::shared_ptr<ISpatialIndex> spatial;
    std::shared_ptr<ISceneQuery> queries;
    std::shared_ptr<ISceneSnapshotProvider> snapshots;
};

[[nodiscard]] foundation::Result<SceneServices> CreateSceneServices(const SceneOptions& options = {});
} // namespace epidemic::runtime
```
