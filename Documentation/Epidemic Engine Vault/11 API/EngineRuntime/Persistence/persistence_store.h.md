---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h"
---

# Persistence: persistence_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 1555 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_store.h>`.

## Объявления типов (лексический индекс)
`IPersistenceQuery`, `IPersistenceStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/dirty_tracker.h"
#include "Epidemic/Runtime/Persistence/lazy_rule_record.h"
#include "Epidemic/Runtime/Persistence/persistent_object_store.h"
#include "Epidemic/Runtime/Persistence/save_transaction.h"
#include "Epidemic/Runtime/Persistence/tombstone_store.h"
#include "Epidemic/Runtime/Persistence/zone_override_store.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace epidemic::runtime
{
class IPersistenceQuery : public IPersistentObjectStore, public IDirtyTracker, public ITombstoneStore, public IZoneOverrideStore
{
  public:
    virtual ~IPersistenceQuery() = default;

    [[nodiscard]] virtual std::optional<LazyRuleRecord> FindLazyRule(LazyRuleId id) const = 0;
    [[nodiscard]] virtual std::vector<LazyRuleRecord> FindLazyRules(PersistentObjectId target_id) const = 0;
    [[nodiscard]] virtual std::vector<LazyRuleRecord> QueryDueLazyRules(GameTimePoint now) const = 0;
    [[nodiscard]] virtual std::vector<LazyRuleRecord> ListLazyRules() const = 0;
    [[nodiscard]] virtual PersistenceRevision GetRevision() const = 0;
};

class IPersistenceStore : public IPersistenceQuery
{
  public:
    virtual ~IPersistenceStore() = default;

    [[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction() = 0;
    [[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction(PersistenceRevision base_revision) = 0;
};
} // namespace epidemic::runtime
```
