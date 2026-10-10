---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h"
---

# Persistence: save_transaction.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 2476 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/save_transaction.h>`.

## Объявления типов (лексический индекс)
`SaveTransactionState`, `ISaveTransaction`, `IPersistenceAdministrativeTransaction`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/lazy_rule_record.h"
#include "Epidemic/Runtime/Persistence/persistent_record.h"
#include "Epidemic/Runtime/Persistence/tombstone_store.h"
#include "Epidemic/Runtime/Persistence/zone_override_store.h"

namespace epidemic::runtime
{
using PersistenceRevision = std::uint64_t;

enum class SaveTransactionState
{
    NotStarted,
    Open,
    Committing,
    Committed,
    RolledBack,
    Failed,
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ISaveTransaction
{
  public:
    virtual ~ISaveTransaction() = default;

    [[nodiscard]] virtual SaveTransactionState GetState() const = 0;
    [[nodiscard]] virtual PersistenceRevision GetBaseRevision() const = 0;
    [[nodiscard]] virtual foundation::Result<void> UpsertObject(PersistentObjectRecord record) = 0;
    [[nodiscard]] virtual foundation::Result<void> DeleteObject(TombstoneRecord tombstone) = 0;
    [[nodiscard]] virtual foundation::Result<void> UpsertLazyRule(LazyRuleRecord record) = 0;
    [[nodiscard]] virtual foundation::Result<void> UpdateLazyRule(LazyRuleRecord record) = 0;
    [[nodiscard]] virtual foundation::Result<void> RemoveLazyRule(LazyRuleId id) = 0;
    [[nodiscard]] virtual foundation::Result<void> UpsertZoneOverride(ZoneOverrideSnapshot snapshot) = 0;
    [[nodiscard]] virtual foundation::Result<void> RemoveZoneOverride(const PersistenceLocation& location) = 0;
    // Commit is terminal on success. A failed commit enters Failed; it cannot be retried on the
    // same transaction and may only be closed by Rollback. Repeating Commit after Committed is an
    // idempotent success. An empty transaction is still a commit and advances the store revision.
    [[nodiscard]] virtual foundation::Result<void> Commit() = 0;
    // Rollback discards staged operations while Open or Failed. It is a no-op after a terminal
    // Committed or RolledBack state.
    virtual void Rollback() = 0;
};

class IPersistenceAdministrativeTransaction
{
  public:
    virtual ~IPersistenceAdministrativeTransaction() = default;

    [[nodiscard]] virtual foundation::Result<void> AdminRemoveObject(PersistentObjectId id) = 0;
    [[nodiscard]] virtual foundation::Result<void> AdminAddTombstone(TombstoneRecord tombstone) = 0;
};
} // namespace epidemic::runtime
```
