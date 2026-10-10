---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_invariants.h"
---

# World: world_invariants.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_invariants.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 1717 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_invariants.h>`.

## Объявления типов (лексический индекс)
_Именованные `class`/`struct`/`enum` в этом заголовке не обнаружены простой лексической проверкой._

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_states.h"
#include "Epidemic/Runtime/World/chunk.h"
#include "Epidemic/Runtime/World/region.h"
#include "Epidemic/Runtime/World/world_query.h"

namespace epidemic::runtime
{
[[nodiscard]] constexpr bool IsValidResidencyState(ResidencyState state) noexcept
{
    switch (state)
    {
    case ResidencyState::Unloaded:
    case ResidencyState::Loading:
    case ResidencyState::Resident:
    case ResidencyState::Active:
    case ResidencyState::Sleeping:
    case ResidencyState::Unloading:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr bool IsValidObjectRealityLevel(ObjectRealityLevel level) noexcept
{
    switch (level)
    {
    case ObjectRealityLevel::AbstractFact:
    case ObjectRealityLevel::Logical:
    case ObjectRealityLevel::Physical:
        return true;
    }
    return false;
}

[[nodiscard]] constexpr bool IsValidPersistenceTier(PersistenceTier tier) noexcept
{
    switch (tier)
    {
    case PersistenceTier::Disposable:
    case PersistenceTier::TemporaryObserved:
    case PersistenceTier::PlayerTouched:
    case PersistenceTier::Protected:
    case PersistenceTier::QuestCritical:
        return true;
    }
    return false;
}

[[nodiscard]] foundation::Result<void> ValidateRealityResidencyCombination(ObjectRealityLevel reality, ResidencyState residency);

[[nodiscard]] foundation::Result<void> ValidateWorldObjectInvariant(
    const WorldObjectSnapshot& object,
    const IRegionRegistry& regions,
    const IChunkRegistry& chunks,
    const IWorldQuery& objects);
} // namespace epidemic::runtime
```
