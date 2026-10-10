---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h"
---

# Serialization: migration_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 1106 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/migration_registry.h>`.

## Объявления типов (лексический индекс)
`IMigrationRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/migration.h"

#include <memory>
#include <vector>

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IMigrationRegistry
{
  public:
    virtual ~IMigrationRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterMigration(std::shared_ptr<const IMigration> migration) = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<const IMigration> FindMigration(const MigrationKey& key) const = 0;
    [[nodiscard]] virtual bool HasMigration(const MigrationKey& key) const = 0;
    [[nodiscard]] virtual foundation::Result<std::vector<std::shared_ptr<const IMigration>>> FindMigrationPath(
        foundation::StringId type_id,
        SchemaVersion from,
        SchemaVersion to) const = 0;
};
} // namespace epidemic::runtime
```
