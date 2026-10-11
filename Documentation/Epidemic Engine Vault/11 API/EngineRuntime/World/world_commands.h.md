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

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/Foundation/runtime_time.h`, `Epidemic/Runtime/World/object_materialization.h`, `Epidemic/Runtime/World/world_object.h`.

### Состав типов и операции

#### `WorldCommandResult` (строка заголовка 13)

Публичные поля и значения по умолчанию:

- `std::optional<WorldObjectSnapshot> before`
- `std::optional<WorldObjectSnapshot> after`

#### `ChangePlacementCommand` (строка заголовка 25)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`

#### `ChangeResidencyCommand` (строка заголовка 32)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`
- `ResidencyState residency = ResidencyState::Unloaded`

#### `PromotePersistenceTierCommand` (строка заголовка 39)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`
- `PersistenceTier tier = PersistenceTier::Disposable`
- `std::optional<PersistentObjectId> persistent_id`

#### `MaterializeObjectCommand` (строка заголовка 48)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`

#### `DemoteObjectCommand` (строка заголовка 54)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`

#### `DestroyObjectCommand` (строка заголовка 60)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

