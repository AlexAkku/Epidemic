---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h"
---

# Physics: physics_query.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h`.

**Модуль:** [[05 EngineRuntime/Модуль Physics|Physics]].
**Размер исходного файла:** 563 байт.
**Подключение:** `#include <Epidemic/Runtime/Physics/physics_query.h>`.

## Объявления типов (лексический индекс)
`IPhysicsQuery`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Physics/physics_types.h"

namespace epidemic::runtime::physics
{
// Public query contract for deterministic raycast and overlap requests.
class IPhysicsQuery
{
  public:
    virtual ~IPhysicsQuery() = default;

    [[nodiscard]] virtual foundation::Result<RaycastHit> Raycast(const RaycastQuery& query) const = 0;
    [[nodiscard]] virtual foundation::Result<OverlapResult> Overlap(const OverlapQuery& query) const = 0;
};
} // namespace epidemic::runtime::physics
```
