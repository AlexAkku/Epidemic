---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h"
---

# Persistence: persistent_object_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 719 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistent_object_store.h>`.

## Объявления типов (лексический индекс)
`IPersistentObjectStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/persistence_location.h"
#include "Epidemic/Runtime/Persistence/persistent_record.h"

#include <optional>
#include <vector>

namespace epidemic::runtime
{
class IPersistentObjectStore
{
  public:
    virtual ~IPersistentObjectStore() = default;

    [[nodiscard]] virtual std::optional<PersistentObjectRecord> FindObject(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<PersistentObjectRecord> FindByLocation(const PersistenceLocation& location) const = 0;
    [[nodiscard]] virtual std::vector<PersistentObjectRecord> ListObjects() const = 0;
};
} // namespace epidemic::runtime
```
