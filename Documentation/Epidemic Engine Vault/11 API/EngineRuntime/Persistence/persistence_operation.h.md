---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_operation.h"
---

# Persistence: persistence_operation.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_operation.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 1346 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_operation.h>`.

## Объявления типов (лексический индекс)
`UpsertObjectOperation`, `AdminRemoveObjectOperation`, `DeleteObjectOperation`, `AdminAddTombstoneOperation`, `UpsertLazyRuleOperation`, `UpdateLazyRuleOperation`, `RemoveLazyRuleOperation`, `UpsertZoneOverrideOperation`, `RemoveZoneOverrideOperation`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Persistence/lazy_rule_record.h"
#include "Epidemic/Runtime/Persistence/persistent_record.h"
#include "Epidemic/Runtime/Persistence/tombstone_store.h"
#include "Epidemic/Runtime/Persistence/zone_override_store.h"

#include <variant>

namespace epidemic::runtime
{
struct UpsertObjectOperation
{
    PersistentObjectRecord record{};
};

struct AdminRemoveObjectOperation
{
    PersistentObjectId id{};
};

struct DeleteObjectOperation
{
    TombstoneRecord tombstone{};
};

struct AdminAddTombstoneOperation
{
    TombstoneRecord tombstone{};
};

struct UpsertLazyRuleOperation
{
    LazyRuleRecord record{};
};

struct UpdateLazyRuleOperation
{
    LazyRuleRecord record{};
};

struct RemoveLazyRuleOperation
{
    LazyRuleId id{};
};

struct UpsertZoneOverrideOperation
{
    ZoneOverrideSnapshot snapshot{};
};

struct RemoveZoneOverrideOperation
{
    PersistenceLocation location{};
};

using PersistenceOperation = std::variant<
    UpsertObjectOperation,
    DeleteObjectOperation,
    AdminRemoveObjectOperation,
    AdminAddTombstoneOperation,
    UpsertLazyRuleOperation,
    UpdateLazyRuleOperation,
    RemoveLazyRuleOperation,
    UpsertZoneOverrideOperation,
    RemoveZoneOverrideOperation>;
} // namespace epidemic::runtime
```
