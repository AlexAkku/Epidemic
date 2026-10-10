---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_backend.h"
---

# Persistence: persistence_backend.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_backend.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 1784 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_backend.h>`.

## Объявления типов (лексический индекс)
`PersistenceSnapshot`, `PersistenceDurability`, `IPersistenceBackend`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/lazy_rule_record.h"
#include "Epidemic/Runtime/Persistence/persistent_record.h"
#include "Epidemic/Runtime/Persistence/save_transaction.h"
#include "Epidemic/Runtime/Persistence/tombstone_store.h"
#include "Epidemic/Runtime/Persistence/zone_override_store.h"

#include <cstdint>
#include <vector>

namespace epidemic::runtime
{
struct PersistenceSnapshot
{
    PersistenceRevision current_revision = 0;
    std::vector<PersistentObjectRecord> objects;
    std::vector<TombstoneRecord> tombstones;
    std::vector<LazyRuleRecord> lazy_rules;
    std::vector<ZoneOverrideSnapshot> zone_overrides;
};

// Semantic validation shared by service composition and durable backend adapters.
[[nodiscard]] foundation::Result<void> ValidatePersistenceSnapshot(const PersistenceSnapshot& snapshot);

// Closed enum domain. Callers must use one of these three durability levels.
enum class PersistenceDurability
{
    MemoryOnly,
    SaveRequired,
    SaveAndFlushRequired,
};

class IPersistenceBackend
{
  public:
    virtual ~IPersistenceBackend() = default;

    [[nodiscard]] virtual foundation::Result<PersistenceSnapshot> Load() = 0;
    // Atomic backend contract: Result failure or exception must leave the previously committed
    // durable snapshot authoritative. A successful return accepts the complete snapshot exactly
    // once at the requested durability level. Separate Save()/Flush() transaction APIs are not part
    // of the Runtime Persistence contract.
    [[nodiscard]] virtual foundation::Result<void> CommitSnapshot(const PersistenceSnapshot& snapshot, PersistenceDurability durability) = 0;
};
} // namespace epidemic::runtime
```
