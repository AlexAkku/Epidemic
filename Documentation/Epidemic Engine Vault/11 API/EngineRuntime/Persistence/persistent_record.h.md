---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_record.h"
---

# Persistence: persistent_record.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_record.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 1081 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistent_record.h>`.

## Объявления типов (лексический индекс)
`PersistentObjectRecord`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_states.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include "Epidemic/Runtime/Persistence/persistence_location.h"
#include "Epidemic/Runtime/Persistence/persistence_payload.h"
#include "Epidemic/Runtime/Persistence/persistence_policy.h"
#include "Epidemic/Runtime/Persistence/persistence_state.h"

#include <cstdint>

namespace epidemic::runtime
{
struct PersistentObjectRecord
{
    PersistentObjectId persistent_id{};
    AssetId asset_id{};
    PersistentObjectKind kind = PersistentObjectKind::Unknown;
    PersistenceTier tier = PersistenceTier::Disposable;
    PersistenceState state = PersistenceState::Clean;
    PersistenceLocation location{};
    PersistencePayload payload{};
    GameTimePoint created_game_time{};
    GameTimePoint last_observed_game_time{};
    ObjectProtectionMask protection_flags{};
    std::uint64_t condition_hash = 0;
    std::uint64_t revision = 0;
};
} // namespace epidemic::runtime
```
