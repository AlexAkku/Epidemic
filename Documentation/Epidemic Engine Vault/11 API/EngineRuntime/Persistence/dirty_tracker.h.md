---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h"
---

# Persistence: dirty_tracker.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 403 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/dirty_tracker.h>`.

## Объявления типов (лексический индекс)
`IDirtyTracker`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <vector>

namespace epidemic::runtime
{
class IDirtyTracker
{
  public:
    virtual ~IDirtyTracker() = default;

    [[nodiscard]] virtual bool IsDirty(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<PersistentObjectId> CollectDirty() const = 0;
};
} // namespace epidemic::runtime
```
