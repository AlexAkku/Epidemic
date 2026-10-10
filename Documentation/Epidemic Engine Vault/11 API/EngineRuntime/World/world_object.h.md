---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_object.h"
---

# World: world_object.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_object.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 818 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_object.h>`.

## Объявления типов (лексический индекс)
`WorldObjectRecord`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_states.h"
#include "Epidemic/Runtime/World/object_placement.h"

#include <cstdint>

namespace epidemic::runtime
{
struct WorldObjectRecord
{
    RuntimeObjectId runtime_id{};
    PersistentObjectId persistent_id{};
    AssetId asset_id{};
    ObjectRealityLevel reality = ObjectRealityLevel::Logical;
    ResidencyState residency = ResidencyState::Unloaded;
    PersistenceTier persistence_tier = PersistenceTier::Disposable;
    ObjectPlacement placement{HiddenPlacement{}};
    std::uint64_t revision = 0;

    [[nodiscard]] bool operator==(const WorldObjectRecord&) const noexcept = default;
};

using WorldObjectSnapshot = WorldObjectRecord;
} // namespace epidemic::runtime
```
