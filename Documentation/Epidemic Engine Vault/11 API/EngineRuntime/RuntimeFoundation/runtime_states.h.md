---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h"
---

# RuntimeFoundation: runtime_states.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`.

**Модуль:** [[05 EngineRuntime/Модуль RuntimeFoundation|RuntimeFoundation]].
**Размер исходного файла:** 3604 байт.
**Подключение:** `#include <Epidemic/Runtime/Foundation/runtime_states.h>`.

## Объявления типов (лексический индекс)
`ResidencyState`, `ObjectRealityLevel`, `PersistenceTier`, `SimulationLod`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
enum class ResidencyState
{
    Unloaded,
    Loading,
    Resident,
    Active,
    Sleeping,
    Unloading,
};

[[nodiscard]] constexpr bool CanTransition(ResidencyState from, ResidencyState to) noexcept
{
    if (from == to)
    {
        return true;
    }

    switch (from)
    {
    case ResidencyState::Unloaded:
        return to == ResidencyState::Loading || to == ResidencyState::Resident;
    case ResidencyState::Loading:
        return to == ResidencyState::Resident || to == ResidencyState::Unloaded;
    case ResidencyState::Resident:
        return to == ResidencyState::Active || to == ResidencyState::Sleeping || to == ResidencyState::Unloading;
    case ResidencyState::Active:
        return to == ResidencyState::Resident || to == ResidencyState::Sleeping || to == ResidencyState::Unloading;
    case ResidencyState::Sleeping:
        return to == ResidencyState::Resident || to == ResidencyState::Active || to == ResidencyState::Unloading;
    case ResidencyState::Unloading:
        return to == ResidencyState::Unloaded;
    }

    return false;
}

enum class ObjectRealityLevel
{
    AbstractFact,
    Logical,
    Physical,
};

[[nodiscard]] constexpr int RealityRank(ObjectRealityLevel level) noexcept
{
    switch (level)
    {
    case ObjectRealityLevel::AbstractFact:
        return 0;
    case ObjectRealityLevel::Logical:
        return 1;
    case ObjectRealityLevel::Physical:
        return 2;
    }

    return 0;
}

[[nodiscard]] constexpr bool IsMoreConcreteRealityLevel(ObjectRealityLevel left, ObjectRealityLevel right) noexcept
{
    return RealityRank(left) > RealityRank(right);
}

[[nodiscard]] constexpr bool IsLessConcreteRealityLevel(ObjectRealityLevel left, ObjectRealityLevel right) noexcept
{
    return RealityRank(left) < RealityRank(right);
}

[[nodiscard]] constexpr bool CanPromoteReality(ObjectRealityLevel from, ObjectRealityLevel to) noexcept
{
    return RealityRank(to) >= RealityRank(from);
}

[[nodiscard]] constexpr bool CanDemoteReality(ObjectRealityLevel from, ObjectRealityLevel to) noexcept
{
    return RealityRank(to) <= RealityRank(from);
}

enum class PersistenceTier
{
    Disposable,
    TemporaryObserved,
    PlayerTouched,
    Protected,
    QuestCritical,
};

[[nodiscard]] constexpr int PersistenceTierRank(PersistenceTier tier) noexcept
{
    switch (tier)
    {
    case PersistenceTier::Disposable:
        return 0;
    case PersistenceTier::TemporaryObserved:
        return 1;
    case PersistenceTier::PlayerTouched:
        return 2;
    case PersistenceTier::Protected:
        return 3;
    case PersistenceTier::QuestCritical:
        return 4;
    }

    return 0;
}

[[nodiscard]] constexpr bool CanPromotePersistenceTier(PersistenceTier from, PersistenceTier to) noexcept
{
    return PersistenceTierRank(to) >= PersistenceTierRank(from);
}

enum class SimulationLod
{
    Dormant,
    Abstract,
    Scheduled,
    Relevant,
    Observed,
    Active,
};

[[nodiscard]] constexpr int SimulationLodRank(SimulationLod lod) noexcept
{
    switch (lod)
    {
    case SimulationLod::Dormant:
        return 0;
    case SimulationLod::Abstract:
        return 1;
    case SimulationLod::Scheduled:
        return 2;
    case SimulationLod::Relevant:
        return 3;
    case SimulationLod::Observed:
        return 4;
    case SimulationLod::Active:
        return 5;
    }

    return 0;
}
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `ResidencyState`: `Unloaded`, `Loading`, `Resident`, `Active`, `Sleeping`, `Unloading`
- `ObjectRealityLevel`: `AbstractFact`, `Logical`, `Physical`
- `PersistenceTier`: `Disposable`, `TemporaryObserved`, `PlayerTouched`, `Protected`, `QuestCritical`
- `SimulationLod`: `Dormant`, `Abstract`, `Scheduled`, `Relevant`, `Observed`, `Active`

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

