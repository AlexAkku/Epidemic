---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h"
---

# Persistence: tombstone_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 840 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/tombstone_store.h>`.

## Объявления типов (лексический индекс)
`TombstoneRecord`, `ITombstoneStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace epidemic::runtime
{
struct TombstoneRecord
{
    PersistentObjectId persistent_id{};
    GameTimePoint deleted_game_time{};
    foundation::StringId reason{};
    std::uint64_t revision = 0;
};

class ITombstoneStore
{
  public:
    virtual ~ITombstoneStore() = default;

    [[nodiscard]] virtual std::optional<TombstoneRecord> FindTombstone(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual bool IsTombstoned(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<TombstoneRecord> ListTombstones() const = 0;
};
} // namespace epidemic::runtime
```
