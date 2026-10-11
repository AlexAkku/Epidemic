---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "materials"]
source_path: "EngineFramework/GameplayWorldStateOwners/Materials"
---

# EngineFramework / Materials

Модуль: **Materials**. Игровые материалы.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Materials/`.
**CMake targets:** `EpidemicGameFrameworkMaterials`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Materials/materials.h|materials.h]]: `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`; объявленные типы: `MaterialId`, `SubstanceId`, `MaterialSlotId`, `MaterialReactionId`, `MaterialResponseTypeId`, `TypeHash`, `MaterialPhysicalProfile`, `MaterialThermalProfile`, `MaterialChemicalProfile`, `MaterialDefinition`, `SubstancePhase`, `SubstanceDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Materials|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility

Owns material, substance, slot and reaction definitions plus per-subject material slot state, composition, contained substances, exposures and reaction-derived responses.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`.

#### Dependency list

Direct production links are `EpidemicGameFrameworkFoundation` and `EpidemicFoundation`. No peer GameplayWorldStateOwner or Runtime module dependency is added by this delta.

#### External ports/callbacks/providers/backends

No external backend or retained callback. GameplayTagRegistry is read during tag-qualified reaction evaluation.

#### Authoritative state

Four registries (materials, substances, slots, reactions), deterministic reaction order, subject-slot state map, revision and bounded material change journal.

#### Derived/cache/index state

reaction_order_ is a sorted derived index over reaction definitions; subject/slot key mapping is authoritative for material state. Query projections are rebuilt from these containers.

#### ID spaces, generations, revisions and cursors

Definition IDs are canonical-name-derived typed IDs. Per-state mutations share a module Revision and ChangeCursor epoch/sequence.

#### State machines

Registries are mutable until Freeze, then definition registration is rejected. Material slot state appears on first successful mutation and can be removed by slot or subject.

Источник контрактов: `Documentation/EngineFramework/Materials.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `SubstancePhase` (`EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`): `Solid`, `Liquid`, `Gas`, `Powder`.
- `MaterialStimulusType` (`EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`): `Heat`, `Cold`, `Electricity`, `Moisture`, `Drying`, `Corrosive`, `Chemical`.
- `ReactionThresholdField` (`EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`): `StimulusMagnitude`, `Temperature`, `Saturation`, `Corrosion`, `ExposureAmount`.
- `MaterialChangeKind` (`EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h`): `Assigned`, `CompositionChanged`, `StateChanged`, `ExposureAdded`, `ExposureRemoved`, `ReactionTriggered`, `Removed`.

### Операции и хранилища состояния

#### `MaterialService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:325`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<MaterialId, MaterialDefinition, TypeHash> materials_`; `std::unordered_map<SubstanceId, SubstanceDefinition, TypeHash> substances_`; `std::unordered_map<MaterialSlotId, std::string, TypeHash> slots_`; `std::unordered_map<MaterialReactionId, MaterialReactionRule, TypeHash> reactions_`; `std::vector<MaterialReactionId> reaction_order_`; `std::unordered_map<MaterialSlotKey, MaterialSlotState, MaterialSlotKeyHash> states_`; `bool frozen_ = false`; `std::vector<MaterialChange> changes_`.

#### `MaterialId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `SubstanceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `MaterialSlotId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:40`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `MaterialReactionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:50`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `MaterialResponseTypeId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:60`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `SubstanceAmount`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:156`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
constexpr SubstanceAmount() noexcept = default;
constexpr SubstanceAmount(std::int64_t value_micro) noexcept : micro(value_micro);
[[nodiscard]] constexpr bool IsPositive() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t micro = 0`.

#### `TypeHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept;
```

#### `MaterialConstituent`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:140`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t weight_ppm = 0`.

#### `SubstanceExposure`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:175`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t coverage_ppm = 0`.

#### `DynamicMaterialState`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Materials/include/Epidemic/GameFramework/Materials/materials.h:184`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t temperature_micro = 0`; `std::uint32_t saturation_ppm = 0`; `std::uint32_t corrosion_ppm = 0`; `std::vector<SubstanceExposure> exposures`; `std::vector<SubstanceQuantity> contents`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Materials/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp` (857 строк): `Error::Create`, `MaterialService::RegisterMaterial`, `MaterialId::FromString`, `MaterialService::RegisterSubstance`, `SubstanceId::FromString`, `MaterialService::RegisterSlot`, `MaterialSlotId::FromString`, `MaterialService::RegisterReaction`, `MaterialReactionId::FromString`, `MaterialService::FindMaterial`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:150`
- `gameplay.material_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:80`
- `gameplay.material_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:70`
- `gameplay.already_registered`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:169`
- `gameplay.substance_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:116`
- `gameplay.substance_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:109`
- `gameplay.material_slot_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:135`
- `gameplay.material_reaction_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:156`
- `gameplay.material_reaction_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:165`
- `gameplay.material_composition_empty`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:202`
- `gameplay.material_composition_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:213`
- `gameplay.material_composition_duplicate`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:217`
- `gameplay.material_composition_weight`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:224`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:234`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:778`
- `gameplay.material_assignment_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:258`
- `gameplay.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:570`
- `gameplay.material_state_unknown`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:554`
- `gameplay.substance_exposure_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:355`
- `gameplay.substance_quantity_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:379`
- `gameplay.substance_quantity_unknown`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:404`
- `gameplay.substance_exposure_unknown`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:422`
- `gameplay.material_stimulus_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:552`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:777`
- `gameplay.material_snapshot_invalid`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:815`
- `gameplay.material_storage_failed`: `EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:827`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/materials_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:150`)

```cpp
148     if (frozen_)
  149     {
  150         return foundation::Result<MaterialReactionId>::Failure(Error("gameplay.registry_frozen", "material reaction registry is frozen"));
  151     }
  152     if (rule.canonical_name.empty() || !rule.response_type.IsValid() ||
```

#### `gameplay.material_invalid` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:80`)

```cpp
78         definition.chemical.toxicity_micro < 0)
   79     {
   80         return foundation::Result<MaterialId>::Failure(Error("gameplay.material_invalid", "material non-negative properties must not be negative"));
   81     }
   82     if (materials_.contains(definition.id))
```

#### `gameplay.material_id_mismatch` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:70`)

```cpp
68     if (definition.id != expected)
   69     {
   70         return foundation::Result<MaterialId>::Failure(Error("gameplay.material_id_mismatch", "material id does not match canonical name"));
   71     }
   72     if (definition.physical.density_micro < 0 || definition.physical.hardness_micro < 0 ||
```

#### `gameplay.already_registered` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:169`)

```cpp
167     if (reactions_.contains(rule.id))
  168     {
  169         return foundation::Result<MaterialReactionId>::Failure(Error("gameplay.already_registered", "material reaction is already registered"));
  170     }
  171     const auto id = rule.id;
```

#### `gameplay.substance_invalid` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:116`)

```cpp
114         definition.conductivity_micro < 0 || definition.freeze_point_micro > definition.boiling_point_micro)
  115     {
  116         return foundation::Result<SubstanceId>::Failure(Error("gameplay.substance_invalid", "substance properties are invalid"));
  117     }
  118     if (substances_.contains(definition.id))
```

#### `gameplay.substance_id_mismatch` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:109`)

```cpp
107     if (definition.id != expected)
  108     {
  109         return foundation::Result<SubstanceId>::Failure(Error("gameplay.substance_id_mismatch", "substance id does not match canonical name"));
  110     }
  111     if (!EnumInRange(definition.phase, SubstancePhase::Powder) ||
```

#### `gameplay.material_slot_invalid` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:135`)

```cpp
133     if (canonical_name.empty())
  134     {
  135         return foundation::Result<MaterialSlotId>::Failure(Error("gameplay.material_slot_invalid", "material slot name must not be empty"));
  136     }
  137     const auto id = MaterialSlotId::FromString(canonical_name);
```

#### `gameplay.material_reaction_invalid` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:156`)

```cpp
154         !EnumInRange(rule.threshold_field, ReactionThresholdField::ExposureAmount))
  155     {
  156         return foundation::Result<MaterialReactionId>::Failure(Error("gameplay.material_reaction_invalid", "reaction name, enum domains and response type must be valid"));
  157     }
  158     const auto expected = MaterialReactionId::FromString(rule.canonical_name);
```

#### `gameplay.material_reaction_id_mismatch` (`EngineFramework/GameplayWorldStateOwners/Materials/src/materials.cpp:165`)

```cpp
163     if (rule.id != expected)
  164     {
  165         return foundation::Result<MaterialReactionId>::Failure(Error("gameplay.material_reaction_id_mismatch", "reaction id does not match canonical name"));
  166     }
  167     if (reactions_.contains(rule.id))
```
