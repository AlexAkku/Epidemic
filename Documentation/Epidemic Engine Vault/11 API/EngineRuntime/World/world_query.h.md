---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_query.h"
---

# World: world_query.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_query.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 649 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_query.h>`.

## Объявления типов (лексический индекс)
`IWorldQuery`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/World/world_object.h"
#include <optional>
#include <vector>

namespace epidemic::runtime
{
class IWorldQuery
{
  public:
    virtual ~IWorldQuery() = default;
    [[nodiscard]] virtual std::optional<WorldObjectRecord> FindObject(RuntimeObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInRegion(RegionId region) const = 0;
    [[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInChunk(ChunkId chunk) const = 0;
    [[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsByReality(ObjectRealityLevel reality) const = 0;
};
}
```
