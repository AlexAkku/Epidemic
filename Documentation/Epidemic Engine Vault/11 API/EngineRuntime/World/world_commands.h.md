---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_commands.h"
---

# World: world_commands.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_commands.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 1570 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_commands.h>`.

## Объявления типов (лексический индекс)
`WorldCommandResult`, `CreateObjectCommand`, `ChangePlacementCommand`, `ChangeResidencyCommand`, `PromotePersistenceTierCommand`, `MaterializeObjectCommand`, `DemoteObjectCommand`, `DestroyObjectCommand`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include "Epidemic/Runtime/World/object_materialization.h"
#include "Epidemic/Runtime/World/world_object.h"

#include <cstdint>
#include <optional>

namespace epidemic::runtime
{
struct WorldCommandResult
{
    RuntimeObjectId runtime_id{};
    std::optional<WorldObjectSnapshot> before;
    std::optional<WorldObjectSnapshot> after;
};

struct CreateObjectCommand
{
    WorldObjectRecord record{};
};

struct ChangePlacementCommand
{
    RuntimeObjectId runtime_id{};
    std::uint64_t expected_revision = 0;
    ObjectPlacement placement{HiddenPlacement{}};
};

struct ChangeResidencyCommand
{
    RuntimeObjectId runtime_id{};
    std::uint64_t expected_revision = 0;
    ResidencyState residency = ResidencyState::Unloaded;
};

struct PromotePersistenceTierCommand
{
    RuntimeObjectId runtime_id{};
    std::uint64_t expected_revision = 0;
    PersistenceTier tier = PersistenceTier::Disposable;
    std::optional<PersistentObjectId> persistent_id;
};


struct MaterializeObjectCommand
{
    MaterializationRequest request{};
    std::uint64_t expected_revision = 0;
};

struct DemoteObjectCommand
{
    DemotionRequest request{};
    std::uint64_t expected_revision = 0;
};

struct DestroyObjectCommand
{
    RuntimeObjectId runtime_id{};
    std::uint64_t expected_revision = 0;
    GameTimePoint destroyed_at{};
    foundation::StringId reason{};
};
} // namespace epidemic::runtime
```
