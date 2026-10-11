---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h"
---

# Materials: materials.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`.

**Модуль:** [[06 EngineFramework/Модуль Materials|Materials]].
**Размер исходного файла:** 16541 байт.
**Подключение:** `#include <Epidemic/GameFramework/Materials/materials.h>`.

## Объявления типов (лексический индекс)
`MaterialId`, `SubstanceId`, `MaterialSlotId`, `MaterialReactionId`, `MaterialResponseTypeId`, `TypeHash`, `MaterialPhysicalProfile`, `MaterialThermalProfile`, `MaterialChemicalProfile`, `MaterialDefinition`, `SubstancePhase`, `SubstanceDefinition`, `MaterialConstituent`, `MaterialComposition`, `SubstanceAmount`, `SubstanceQuantity`, `SubstanceExposure`, `DynamicMaterialState`, `MaterialSlotKey`, `MaterialSlotKeyHash`, `MaterialSlotState`, `MaterialStimulusType`, `MaterialStimulus`, `ReactionThresholdField`, `MaterialReactionRule`, `MaterialResponse`, `MaterialChangeKind`, `MaterialChange`, `MaterialChangeBatch`, `MaterialsSnapshot`, `MaterialsDiagnostics`, `MaterialService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::materials
{
constexpr std::int64_t kFixedOne = 1'000'000;
constexpr std::uint32_t kCompositionOnePpm = 1'000'000;

struct MaterialId
{
    TypeId value{};
    [[nodiscard]] static constexpr MaterialId FromString(std::string_view name) noexcept { return MaterialId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const MaterialId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const MaterialId&) const noexcept = default;
};

struct SubstanceId
{
    TypeId value{};
    [[nodiscard]] static constexpr SubstanceId FromString(std::string_view name) noexcept { return SubstanceId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const SubstanceId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const SubstanceId&) const noexcept = default;
};

struct MaterialSlotId
{
    TypeId value{};
    [[nodiscard]] static constexpr MaterialSlotId FromString(std::string_view name) noexcept { return MaterialSlotId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const MaterialSlotId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const MaterialSlotId&) const noexcept = default;
};

struct MaterialReactionId
{
    TypeId value{};
    [[nodiscard]] static constexpr MaterialReactionId FromString(std::string_view name) noexcept { return MaterialReactionId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const MaterialReactionId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const MaterialReactionId&) const noexcept = default;
};

struct MaterialResponseTypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr MaterialResponseTypeId FromString(std::string_view name) noexcept { return MaterialResponseTypeId{TypeId::FromString(name)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr bool operator==(const MaterialResponseTypeId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const MaterialResponseTypeId&) const noexcept = default;
};

struct TypeHash
{
    template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept
    {
        return std::hash<TypeId>{}(id.value);
    }
};

struct MaterialPhysicalProfile
{
    std::int64_t density_micro = 0;
    std::int64_t hardness_micro = 0;
    std::int64_t toughness_micro = 0;
    std::int64_t brittleness_micro = 0;
};

struct MaterialThermalProfile
{
    std::int64_t flammability_micro = 0;
    std::int64_t ignition_threshold_micro = 0;
    std::int64_t burn_rate_micro = 0;
    std::int64_t thermal_capacity_micro = 0;
    std::int64_t thermal_conductivity_micro = 0;
};

struct MaterialChemicalProfile
{
    std::int64_t electrical_conductivity_micro = 0;
    std::int64_t porosity_micro = 0;
    std::int64_t absorption_micro = 0;
    std::int64_t corrosion_resistance_micro = 0;
    std::int64_t toxicity_micro = 0;
};

struct MaterialDefinition
{
    MaterialId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    MaterialPhysicalProfile physical{};
    MaterialThermalProfile thermal{};
    MaterialChemicalProfile chemical{};
    Revision revision{};
};

enum class SubstancePhase
{
    Solid,
    Liquid,
    Gas,
    Powder,
};

struct SubstanceDefinition
{
    SubstanceId id{};
    std::string canonical_name;
    SubstancePhase phase = SubstancePhase::Solid;
    GameplayTagSet tags;
    std::int64_t density_micro = 0;
    std::int64_t viscosity_micro = 0;
    std::int64_t flammability_micro = 0;
    std::int64_t volatility_micro = 0;
    std::int64_t toxicity_micro = 0;
    std::int64_t corrosiveness_micro = 0;
    std::int64_t conductivity_micro = 0;
    std::int64_t freeze_point_micro = 0;
    std::int64_t boiling_point_micro = 0;
};

struct MaterialConstituent
{
    MaterialId material{};
    std::uint32_t weight_ppm = 0;

    [[nodiscard]] constexpr bool operator==(const MaterialConstituent&) const noexcept = default;
};

struct MaterialComposition
{
    std::vector<MaterialConstituent> constituents;
};

// Semantic substance quantity in micro-units of the base quantity unit agreed for the
// substance/game data. This is deliberately not an implicit SI unit; every producer and
// consumer of a SubstanceId must use the same authored quantity unit.
struct SubstanceAmount
{
    std::int64_t micro = 0;

    constexpr SubstanceAmount() noexcept = default;
    constexpr SubstanceAmount(std::int64_t value_micro) noexcept : micro(value_micro) {}

    [[nodiscard]] constexpr bool IsPositive() const noexcept { return micro > 0; }
    [[nodiscard]] constexpr bool operator==(const SubstanceAmount&) const noexcept = default;
};

struct SubstanceQuantity
{
    SubstanceId substance{};
    SubstanceAmount amount{};

    [[nodiscard]] constexpr bool operator==(const SubstanceQuantity&) const noexcept = default;
};

struct SubstanceExposure
{
    SubstanceId substance{};
    SubstanceAmount amount{};
    std::uint32_t coverage_ppm = 0;

    [[nodiscard]] constexpr bool operator==(const SubstanceExposure&) const noexcept = default;
};

struct DynamicMaterialState
{
    std::int64_t temperature_micro = 0;
    std::uint32_t saturation_ppm = 0;
    std::uint32_t corrosion_ppm = 0;
    std::vector<SubstanceExposure> exposures;
    std::vector<SubstanceQuantity> contents;

    [[nodiscard]] constexpr bool operator==(const DynamicMaterialState&) const noexcept = default;
};

struct MaterialSlotKey
{
    GameplayObjectRef subject{};
    MaterialSlotId slot{};
    [[nodiscard]] constexpr bool operator==(const MaterialSlotKey&) const noexcept = default;
};

struct MaterialSlotKeyHash
{
    [[nodiscard]] std::size_t operator()(const MaterialSlotKey& key) const noexcept
    {
        auto seed = std::hash<GameplayObjectRef>{}(key.subject);
        const auto slot = std::hash<TypeId>{}(key.slot.value);
        seed ^= slot + 0x9E3779B97F4A7C15ull + (seed << 6u) + (seed >> 2u);
        return seed;
    }
};

struct MaterialSlotState
{
    MaterialSlotKey key{};
    MaterialComposition composition;
    DynamicMaterialState dynamic{};
    Revision revision{};
};

enum class MaterialStimulusType
{
    Heat,
    Cold,
    Electricity,
    Moisture,
    Drying,
    Corrosive,
    Chemical,
};

struct MaterialStimulus
{
    GameplayObjectRef target{};
    MaterialSlotId slot{};
    MaterialStimulusType type = MaterialStimulusType::Heat;
    std::int64_t magnitude_micro = 0;
    GameplayContext context{};
};

enum class ReactionThresholdField
{
    StimulusMagnitude,
    Temperature,
    Saturation,
    Corrosion,
    ExposureAmount,
};

struct MaterialReactionRule
{
    // Reaction rules describe discrete semantic responses to an explicitly applied
    // stimulus. Materials intentionally does not run a continuous thermal, fluid,
    // combustion, corrosion, or chemistry solver. Higher-level simulation/integration
    // code is responsible for deciding when stimuli are applied over gameplay time.
    MaterialReactionId id{};
    std::string canonical_name;
    MaterialStimulusType stimulus = MaterialStimulusType::Heat;
    std::optional<TagId> required_material_tag{};
    std::optional<TagId> required_substance_tag{};
    ReactionThresholdField threshold_field = ReactionThresholdField::StimulusMagnitude;
    std::int64_t min_threshold_micro = 0;
    MaterialResponseTypeId response_type{};
    std::int64_t response_magnitude_micro = 0;
    int priority = 0;
};

struct MaterialResponse
{
    MaterialResponseTypeId type{};
    MaterialReactionId reaction{};
    GameplayObjectRef subject{};
    MaterialSlotId slot{};
    std::int64_t magnitude_micro = 0;
    GameplayContext context{};
};

enum class MaterialChangeKind
{
    Assigned,
    CompositionChanged,
    StateChanged,
    ExposureAdded,
    ExposureRemoved,
    ReactionTriggered,
    Removed,
};

struct MaterialChange
{
    std::uint64_t sequence = 0;
    MaterialChangeKind kind = MaterialChangeKind::Assigned;
    MaterialSlotKey key{};
    SubstanceId substance{};
    MaterialResponseTypeId response{};
    MaterialReactionId reaction{};
    Revision revision{};
    GameplayContext context{};
};

struct MaterialChangeBatch
{
    std::vector<MaterialChange> changes;
    bool snapshot_required = false;
    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct MaterialsSnapshot
{
    std::vector<MaterialSlotState> states;
    Revision revision{};
    std::uint64_t change_epoch = 1;
};

struct MaterialsDiagnostics
{
    std::uint64_t material_states = 0;
    std::uint64_t substance_exposures = 0;
    std::uint64_t stimuli = 0;
    std::uint64_t reaction_evaluations = 0;
    std::uint64_t reactions_triggered = 0;
};

class MaterialService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<MaterialService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.materials");
    }

    [[nodiscard]] foundation::Result<MaterialId> RegisterMaterial(MaterialDefinition definition);
    [[nodiscard]] foundation::Result<SubstanceId> RegisterSubstance(SubstanceDefinition definition);
    [[nodiscard]] foundation::Result<MaterialSlotId> RegisterSlot(std::string_view canonical_name);
    [[nodiscard]] foundation::Result<MaterialReactionId> RegisterReaction(MaterialReactionRule rule);
    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }

    [[nodiscard]] const MaterialDefinition* FindMaterial(MaterialId id) const noexcept;
    [[nodiscard]] const SubstanceDefinition* FindSubstance(SubstanceId id) const noexcept;
    [[nodiscard]] bool IsSlotRegistered(MaterialSlotId id) const noexcept { return slots_.contains(id); }

    [[nodiscard]] foundation::Result<void> AssignComposition(
        GameplayObjectRef subject,
        MaterialSlotId slot,
        MaterialComposition composition,
        GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveSlot(GameplayObjectRef subject, MaterialSlotId slot, GameplayContext context = {});
    [[nodiscard]] foundation::Result<std::uint64_t> RemoveSubject(GameplayObjectRef subject, GameplayContext context = {});

    [[nodiscard]] foundation::Result<void> ApplySubstanceExposure(
        GameplayObjectRef subject,
        MaterialSlotId slot,
        SubstanceId substance,
        SubstanceAmount amount,
        std::uint32_t coverage_ppm,
        GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SetContainedSubstance(
        GameplayObjectRef subject,
        MaterialSlotId slot,
        SubstanceId substance,
        SubstanceAmount amount,
        GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveContainedSubstance(
        GameplayObjectRef subject,
        MaterialSlotId slot,
        SubstanceId substance,
        GameplayContext context = {});    [[nodiscard]] foundation::Result<void> RemoveSubstanceExposure(
        GameplayObjectRef subject,
        MaterialSlotId slot,
        SubstanceId substance,
        GameplayContext context = {});

    [[nodiscard]] foundation::Result<std::vector<MaterialResponse>> EvaluateStimulus(
        const MaterialStimulus& stimulus,
        const GameplayTagRegistry& tags) const;
    [[nodiscard]] foundation::Result<std::vector<MaterialResponse>> ApplyStimulus(
        const MaterialStimulus& stimulus,
        const GameplayTagRegistry& tags);

    [[nodiscard]] std::optional<MaterialSlotState> FindState(GameplayObjectRef subject, MaterialSlotId slot) const;
    [[nodiscard]] std::vector<MaterialSlotState> FindSlots(GameplayObjectRef subject) const;
    [[nodiscard]] std::vector<MaterialSlotState> FindExposedTo(SubstanceId substance) const;
    [[nodiscard]] std::vector<MaterialSlotState> FindByMaterialTag(TagId tag, const GameplayTagRegistry& tags) const;

    private:

        [[nodiscard]] std::vector<MaterialChange> ChangesSinceSequence(std::uint64_t sequence) const;

    public:
    [[nodiscard]] MaterialChangeBatch ReadChangesSince(ChangeCursor cursor) const;
    [[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept { return {journal_epoch_, last_change_sequence_}; }
    void PruneChangesBefore(std::uint64_t sequence);

    [[nodiscard]] MaterialsSnapshot CaptureSnapshot() const;
    // Captures only states owned by the supplied gameplay subjects. This overload is
    // intended for persistence orchestration that has already decided which subjects
    // belong to the save scope without introducing a dependency from Materials to
    // Entities or another persistence owner. The returned snapshot is deterministic
    // regardless of the input order and contains no state for other subjects.
    [[nodiscard]] MaterialsSnapshot CaptureSnapshot(std::span<const GameplayObjectRef> subjects) const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(MaterialsSnapshot snapshot);
    [[nodiscard]] MaterialsDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }

  private:
    [[nodiscard]] foundation::Result<void> ValidateComposition(MaterialComposition& composition) const;
    [[nodiscard]] bool HasMaterialTag(const MaterialSlotState& state, TagId required, const GameplayTagRegistry& tags) const;
    [[nodiscard]] bool HasSubstanceTag(const MaterialSlotState& state, TagId required, const GameplayTagRegistry& tags) const;
    [[nodiscard]] std::int64_t ThresholdValue(const MaterialSlotState& state, const MaterialStimulus& stimulus, const MaterialReactionRule& rule, const GameplayTagRegistry& tags) const noexcept;
    [[nodiscard]] std::vector<MaterialResponse> EvaluateRules(
        const MaterialSlotState& state,
        const MaterialStimulus& stimulus,
        const GameplayTagRegistry& tags) const;
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    [[nodiscard]] bool CanRecordChanges(std::size_t count) const noexcept;
    void RecordChange(MaterialChange change) noexcept;

    std::unordered_map<MaterialId, MaterialDefinition, TypeHash> materials_;
    std::unordered_map<SubstanceId, SubstanceDefinition, TypeHash> substances_;
    std::unordered_map<MaterialSlotId, std::string, TypeHash> slots_;
    std::unordered_map<MaterialReactionId, MaterialReactionRule, TypeHash> reactions_;
    std::vector<MaterialReactionId> reaction_order_;
    std::unordered_map<MaterialSlotKey, MaterialSlotState, MaterialSlotKeyHash> states_;

    Revision revision_{};
    bool frozen_ = false;
    std::vector<MaterialChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t last_change_sequence_ = 0;
    std::uint64_t journal_epoch_ = 1;

    mutable std::uint64_t reaction_evaluations_ = 0;
    std::uint64_t stimuli_ = 0;
    std::uint64_t reactions_triggered_ = 0;
};
} // namespace epidemic::gameplay::materials
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `SubstancePhase`: `Solid`, `Liquid`, `Gas`, `Powder`
- `MaterialStimulusType`: `Heat`, `Cold`, `Electricity`, `Moisture`, `Drying`, `Corrosive`, `Chemical`
- `ReactionThresholdField`: `StimulusMagnitude`, `Temperature`, `Saturation`, `Corrosion`, `ExposureAmount`
- `MaterialChangeKind`: `Assigned`, `CompositionChanged`, `StateChanged`, `ExposureAdded`, `ExposureRemoved`, `ReactionTriggered`, `Removed`

### Состав типов и операции

#### `MaterialId` (строка заголовка 20)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr MaterialId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const MaterialId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const MaterialId&) const noexcept = default;
```

#### `SubstanceId` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr SubstanceId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const SubstanceId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const SubstanceId&) const noexcept = default;
```

#### `MaterialSlotId` (строка заголовка 40)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr MaterialSlotId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const MaterialSlotId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const MaterialSlotId&) const noexcept = default;
```

#### `MaterialReactionId` (строка заголовка 50)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr MaterialReactionId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const MaterialReactionId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const MaterialReactionId&) const noexcept = default;
```

#### `MaterialResponseTypeId` (строка заголовка 60)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr MaterialResponseTypeId FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr bool operator==(const MaterialResponseTypeId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const MaterialResponseTypeId&) const noexcept = default;
```

#### `TypeHash` (строка заголовка 70)

Публичные сигнатуры операций:

```cpp
template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept;
```

#### `MaterialPhysicalProfile` (строка заголовка 78)

Публичные поля и значения по умолчанию:

- `std::int64_t density_micro = 0`
- `std::int64_t hardness_micro = 0`
- `std::int64_t toughness_micro = 0`
- `std::int64_t brittleness_micro = 0`

#### `MaterialThermalProfile` (строка заголовка 86)

Публичные поля и значения по умолчанию:

- `std::int64_t flammability_micro = 0`
- `std::int64_t ignition_threshold_micro = 0`
- `std::int64_t burn_rate_micro = 0`
- `std::int64_t thermal_capacity_micro = 0`
- `std::int64_t thermal_conductivity_micro = 0`

#### `MaterialChemicalProfile` (строка заголовка 95)

Публичные поля и значения по умолчанию:

- `std::int64_t electrical_conductivity_micro = 0`
- `std::int64_t porosity_micro = 0`
- `std::int64_t absorption_micro = 0`
- `std::int64_t corrosion_resistance_micro = 0`
- `std::int64_t toxicity_micro = 0`

#### `MaterialDefinition` (строка заголовка 104)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`

#### `SubstanceDefinition` (строка заголовка 123)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `SubstancePhase phase = SubstancePhase::Solid`
- `GameplayTagSet tags`
- `std::int64_t density_micro = 0`
- `std::int64_t viscosity_micro = 0`
- `std::int64_t flammability_micro = 0`
- `std::int64_t volatility_micro = 0`
- `std::int64_t toxicity_micro = 0`
- `std::int64_t corrosiveness_micro = 0`
- `std::int64_t conductivity_micro = 0`
- `std::int64_t freeze_point_micro = 0`
- `std::int64_t boiling_point_micro = 0`

#### `MaterialConstituent` (строка заголовка 140)

Публичные поля и значения по умолчанию:

- `std::uint32_t weight_ppm = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const MaterialConstituent&) const noexcept = default;
```

#### `MaterialComposition` (строка заголовка 148)

Публичные поля и значения по умолчанию:

- `std::vector<MaterialConstituent> constituents`

#### `SubstanceAmount` (строка заголовка 156)

Публичные поля и значения по умолчанию:

- `std::int64_t micro = 0`

Публичные сигнатуры операций:

```cpp
constexpr SubstanceAmount() noexcept = default;
constexpr SubstanceAmount(std::int64_t value_micro) noexcept : micro(value_micro);
[[nodiscard]] constexpr bool IsPositive() const noexcept;
[[nodiscard]] constexpr bool operator==(const SubstanceAmount&) const noexcept = default;
```

#### `SubstanceQuantity` (строка заголовка 167)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const SubstanceQuantity&) const noexcept = default;
```

#### `SubstanceExposure` (строка заголовка 175)

Публичные поля и значения по умолчанию:

- `std::uint32_t coverage_ppm = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const SubstanceExposure&) const noexcept = default;
```

#### `DynamicMaterialState` (строка заголовка 184)

Публичные поля и значения по умолчанию:

- `std::int64_t temperature_micro = 0`
- `std::uint32_t saturation_ppm = 0`
- `std::uint32_t corrosion_ppm = 0`
- `std::vector<SubstanceExposure> exposures`
- `std::vector<SubstanceQuantity> contents`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const DynamicMaterialState&) const noexcept = default;
```

#### `MaterialSlotKey` (строка заголовка 195)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const MaterialSlotKey&) const noexcept = default;
```

#### `MaterialSlotKeyHash` (строка заголовка 202)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const MaterialSlotKey& key) const noexcept;
```

#### `MaterialSlotState` (строка заголовка 213)

Публичные поля и значения по умолчанию:

- `MaterialComposition composition`

#### `MaterialStimulus` (строка заголовка 232)

Публичные поля и значения по умолчанию:

- `MaterialStimulusType type = MaterialStimulusType::Heat`
- `std::int64_t magnitude_micro = 0`

#### `MaterialReactionRule` (строка заголовка 250)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `MaterialStimulusType stimulus = MaterialStimulusType::Heat`
- `ReactionThresholdField threshold_field = ReactionThresholdField::StimulusMagnitude`
- `std::int64_t min_threshold_micro = 0`
- `std::int64_t response_magnitude_micro = 0`
- `int priority = 0`

#### `MaterialResponse` (строка заголовка 268)

Публичные поля и значения по умолчанию:

- `std::int64_t magnitude_micro = 0`

#### `MaterialChange` (строка заголовка 289)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `MaterialChangeKind kind = MaterialChangeKind::Assigned`

#### `MaterialChangeBatch` (строка заголовка 301)

Публичные поля и значения по умолчанию:

- `std::vector<MaterialChange> changes`
- `bool snapshot_required = false`

#### `MaterialsSnapshot` (строка заголовка 309)

Публичные поля и значения по умолчанию:

- `std::vector<MaterialSlotState> states`
- `std::uint64_t change_epoch = 1`

#### `MaterialsDiagnostics` (строка заголовка 316)

Публичные поля и значения по умолчанию:

- `std::uint64_t material_states = 0`
- `std::uint64_t substance_exposures = 0`
- `std::uint64_t stimuli = 0`
- `std::uint64_t reaction_evaluations = 0`
- `std::uint64_t reactions_triggered = 0`

#### `MaterialService` (строка заголовка 325)

Внутреннее состояние (не часть публичного API): `std::unordered_map<MaterialId, MaterialDefinition, TypeHash> materials_`, `std::unordered_map<SubstanceId, SubstanceDefinition, TypeHash> substances_`, `std::unordered_map<MaterialSlotId, std::string, TypeHash> slots_`, `std::unordered_map<MaterialReactionId, MaterialReactionRule, TypeHash> reactions_`, `std::vector<MaterialReactionId> reaction_order_`, `std::unordered_map<MaterialSlotKey, MaterialSlotState, MaterialSlotKeyHash> states_`, `bool frozen_ = false`, `std::vector<MaterialChange> changes_`, `std::uint64_t next_change_sequence_ = 1`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<MaterialId> RegisterMaterial(MaterialDefinition definition);
[[nodiscard]] foundation::Result<SubstanceId> RegisterSubstance(SubstanceDefinition definition);
[[nodiscard]] foundation::Result<MaterialSlotId> RegisterSlot(std::string_view canonical_name);
[[nodiscard]] foundation::Result<MaterialReactionId> RegisterReaction(MaterialReactionRule rule);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const MaterialDefinition* FindMaterial(MaterialId id) const noexcept;
[[nodiscard]] const SubstanceDefinition* FindSubstance(SubstanceId id) const noexcept;
[[nodiscard]] bool IsSlotRegistered(MaterialSlotId id) const noexcept;
[[nodiscard]] foundation::Result<std::vector<MaterialResponse>> EvaluateStimulus( const MaterialStimulus& stimulus, const GameplayTagRegistry& tags) const;
[[nodiscard]] foundation::Result<std::vector<MaterialResponse>> ApplyStimulus( const MaterialStimulus& stimulus, const GameplayTagRegistry& tags);
[[nodiscard]] std::optional<MaterialSlotState> FindState(GameplayObjectRef subject, MaterialSlotId slot) const;
[[nodiscard]] std::vector<MaterialSlotState> FindSlots(GameplayObjectRef subject) const;
[[nodiscard]] std::vector<MaterialSlotState> FindExposedTo(SubstanceId substance) const;
[[nodiscard]] std::vector<MaterialSlotState> FindByMaterialTag(TagId tag, const GameplayTagRegistry& tags) const;
[[nodiscard]] MaterialChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesBefore(std::uint64_t sequence);
[[nodiscard]] MaterialsSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(MaterialsSnapshot snapshot);
[[nodiscard]] MaterialsDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterMaterial` | `MaterialService::RegisterMaterial` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:53` |
| `RegisterSubstance` | `MaterialService::RegisterSubstance` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:92` |
| `RegisterSlot` | `MaterialService::RegisterSlot` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:127` |
| `RegisterReaction` | `MaterialService::RegisterReaction` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:146` |
| `FindMaterial` | `MaterialService::FindMaterial` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:186` |
| `FindSubstance` | `MaterialService::FindSubstance` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:192` |
| `EvaluateStimulus` | `MaterialService::EvaluateStimulus` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:530` |
| `ApplyStimulus` | `MaterialService::ApplyStimulus` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:547` |
| `FindState` | `MaterialService::FindState` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:592` |
| `FindSlots` | `MaterialService::FindSlots` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:602` |
| `FindExposedTo` | `MaterialService::FindExposedTo` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:616` |
| `FindByMaterialTag` | `MaterialService::FindByMaterialTag` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:639` |
| `ReadChangesSince` | `MaterialService::ReadChangesSince` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:691` |
| `PruneChangesBefore` | `MaterialService::PruneChangesBefore` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:710` |
| `CaptureSnapshot` | `MaterialService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:718` |
| `RestoreSnapshot` | `MaterialService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:771` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:16`
- `RegisterMaterial`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:23`
- `RegisterSubstance`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:32`
- `RegisterSlot`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:34`
- `RegisterReaction`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:43`
- `FindState`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:60`
- `ApplyStimulus`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:64`
- `CurrentRevision`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:69`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:73`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:86`
- `GetDiagnostics`: `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp:99`

