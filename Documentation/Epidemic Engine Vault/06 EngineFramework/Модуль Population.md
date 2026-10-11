---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "population"]
source_path: "EngineFramework/GameplayWorldStateOwners/Population"
---

# EngineFramework / Population

Модуль: **Population**. Популяция и её развитие.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Population/`.
**CMake targets:** `EpidemicGameFrameworkPopulation`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Population/population.h|population.h]]: `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`; объявленные типы: `PopulationGroupId`, `PopulationUnitId`, `PopulationProfileId`, `PopulationTemplateId`, `PopulationResidenceId`, `PopulationMigrationId`, `PopulationAllocationId`, `IdHash`, `PopulationGroupState`, `PopulationUnitState`, `ResidenceState`, `MigrationState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Population|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Scope

Freeze unit: `EngineFramework/GameplayWorldStateOwners/Population`. Admission: 75 public callables, 80 mutation obligations, 2 lifecycle candidates, 19 stale-identity candidates, 0 external-boundary candidates.

#### Public contract review

- Template registry/freeze and group/unit creation.
- Virtual/materialized/dead/removed unit lifecycle and entity binding.
- Residence and area indexes remain consistent with primary records.
- Migration start/finish/cancel/fail is multi-container atomic.
- Allocation reserve/commit/release and terminal pruning.
- Snapshot/restore preserves generators, revisions, indexes and journal epoch/cursor.

#### Failure atomicity and boundary review

All revision-bearing mutations were reviewed for revision/generator exhaustion before authoritative mutation. Multi-container mutations were reviewed for rollback or staged publication. Snapshot restore builds/validates candidate state before live-state replacement. Allocation-failure evidence for this block no longer depends on the process-global Framework allocator helper; owned tests use module-local failure seams where allocation failure is evidence.

#### Persistence and deterministic reads

The module snapshot surface is treated as local in-memory persistence evidence for Goal 4. Non-empty roundtrip, invalid restore, generator/revision continuity, journal epoch/cursor behavior and failed-restore pre-state preservation are covered by the module test executable. Whole-engine ordered restore and replay remain Goal 5.

#### Test evidence

Primary regression source: `EngineFramework/DevelopmentInfrastructure/Tests/population_tests.cpp`.
Registered target: `EpidemicGameFrameworkPopulationTests`.
Local Linux verification in the worker environment used direct C++23 compilation with `-Wall -Wextra -Wpedantic -Werror` and executed the module test binary. The repository top-level CMake configure is Windows-only and therefore cannot be used in this Linux worker environment.

#### Goal 4 result

Block-local review result: LOCAL_READY candidate for serial integration. This document does not claim whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN`.

Источник контрактов: `Documentation/EngineFramework/Population.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PopulationGroupState` (`EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`): `Active`, `Dormant`, `Depleted`, `Removed`.
- `PopulationUnitState` (`EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`): `Latent`, `Abstract`, `Materialized`, `Dead`, `Removed`.
- `ResidenceState` (`EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`): `Assigned`, `Unavailable`, `Abandoned`, `Destroyed`.
- `MigrationState` (`EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`): `Planned`, `Active`, `Completed`, `Cancelled`, `Failed`.
- `PopulationAllocationState` (`EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`): `Active`, `Committed`, `Released`.
- `PopulationChangeKind` (`EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h`): `GroupCreated`, `GroupChanged`, `UnitCreated`, `UnitMaterialized`, `UnitDematerialized`, `UnitDied`, `UnitMigrated`, `UnitRetired`, `ResidenceAssigned`, `MigrationStarted`, `MigrationCompleted`, `MigrationCancelled`, `MigrationFailed`, `EntityBindingChanged` и другие значения.

### Операции и хранилища состояния

#### `PopulationService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:336`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterTemplate(PopulationTemplate definition);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] foundation::Result<PopulationGroupId> CreateGroup(PopulationGroup group);
void PruneTerminalAllocations(GameplayObjectId correlation);
[[nodiscard]] const PopulationGroup *GetGroup(PopulationGroupId id) const noexcept;
[[nodiscard]] const PopulationUnit *GetUnit(PopulationUnitId id) const noexcept;
[[nodiscard]] const PopulationTemplate *GetTemplate(PopulationTemplateId id) const noexcept;
[[nodiscard]] const PopulationAllocation *GetAllocation(PopulationAllocationId id) const noexcept;
[[nodiscard]] const PopulationAllocation *GetActiveAllocationForUnit(PopulationUnitId unit) const noexcept;
[[nodiscard]] const PopulationUnit *FindUnitByEntity(GameplayObjectRef entity) const noexcept;
[[nodiscard]] std::vector<PopulationAllocation> FindAllocationsByCorrelation(GameplayObjectId correlation) const;
[[nodiscard]] static constexpr GameplayObjectRef ResidentRef(PopulationUnitId unit) noexcept;
[[nodiscard]] std::vector<PopulationGroup> FindGroupsInArea(GameplayObjectRef area) const;
[[nodiscard]] std::vector<PopulationUnit> FindUnitsByGroup(PopulationGroupId group) const;
[[nodiscard]] std::vector<PopulationUnit> FindUnitsByState(PopulationUnitState state) const;
[[nodiscard]] std::vector<PopulationUnit> FindUnitsInArea(GameplayObjectRef area) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<PopulationTemplateId, PopulationTemplate, IdHash> templates_`; `std::unordered_map<PopulationGroupId, PopulationGroup, IdHash> groups_`; `std::unordered_map<PopulationUnitId, PopulationUnit, IdHash> units_`; `std::unordered_map<PopulationResidenceId, PopulationResidence, IdHash> residences_`; `std::unordered_map<PopulationMigrationId, PopulationMigration, IdHash> migrations_`; `std::unordered_map<PopulationAllocationId, PopulationAllocation, IdHash> allocations_`; `std::unordered_map<GameplayObjectRef, PopulationUnitId> unit_by_entity_`; `std::unordered_map<PopulationUnitId, PopulationMigrationId, IdHash> active_migration_by_unit_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:136`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `PopulationGroupId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PopulationGroupId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PopulationUnitId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:36`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PopulationUnitId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PopulationResidenceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:82`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PopulationResidenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PopulationMigrationId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:100`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PopulationMigrationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PopulationAllocationId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:118`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PopulationAllocationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PopulationTemplate`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:202`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> generation_payload`; `bool persistent = true`.

#### `PopulationGroup`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Population/include/Epidemic/GameFramework/Population/population.h:211`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `PopulationGroupState state = PopulationGroupState::Active`; `std::uint32_t desired_count = 0`; `std::uint32_t current_known_count = 0`; `std::uint32_t materialized_count = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Population/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp` (1114 строк): `Error::Create`, `PopulationService::CanAdvanceRevision`, `PopulationService::CanRecordChanges`, `PopulationService::FindMutableUnit`, `PopulationService::FindMutableGroup`, `PopulationService::RegisterTemplate`, `PopulationService::FreezeDefinitions`, `PopulationService::CreateGroup`, `PopulationService::CreateUnit`, `PopulationService::BindEntity`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.population.definitions_frozen`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:149`
- `gameplay.population.invalid_template`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:150`
- `gameplay.population.duplicate_template`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:151`
- `gameplay.population.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:472`
- `gameplay.population.publication_failed`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:472`
- `gameplay.population.invalid_group`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:168`
- `gameplay.population.invalid_group_state`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:169`
- `gameplay.population.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:451`
- `gameplay.population.duplicate_group`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:173`
- `gameplay.population.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:472`
- `gameplay.population.invalid_unit_group`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:185`
- `gameplay.population.unknown_template`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:186`
- `gameplay.population.invalid_unit_state`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:187`
- `gameplay.population.invalid_entity_link`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:243`
- `gameplay.population.entity_already_bound`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:464`
- `gameplay.population.invalid_unit_area`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:194`
- `gameplay.population.duplicate_unit`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:198`
- `gameplay.population.unit_missing`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:417`
- `gameplay.population.invalid_entity_link_state`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:242`
- `gameplay.population.invalid_materialize`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:256`
- `gameplay.population.invalid_materialize_state`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:257`
- `gameplay.population.invalid_materialize_binding`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:258`
- `gameplay.population.invalid_dematerialize_state`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:276`
- `gameplay.population.invalid_terminal_transition`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:358`
- `gameplay.population.invalid_residence`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:383`
- `gameplay.population.residence_index_corrupt`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:387`
- `gameplay.population.duplicate_residence`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:392`
- `gameplay.population.invalid_migration`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:405`
- `gameplay.population.migration_already_active`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:402`
- `gameplay.population.migration_source_stale`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:417`
- `gameplay.population.duplicate_migration`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:406`
- `gameplay.population.invalid_migration_state`: `EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:416`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/population_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.population.definitions_frozen` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:149`)

```cpp
147 foundation::Result<void> PopulationService::RegisterTemplate(PopulationTemplate definition)
  148 {
  149     if(definitions_frozen_)return foundation::Result<void>::Failure(Error("gameplay.population.definitions_frozen","population definitions are frozen"));
  150     if(!definition.id.IsValid())return foundation::Result<void>::Failure(Error("gameplay.population.invalid_template","invalid population template"));
  151     if(templates_.contains(definition.id))return foundation::Result<void>::Failure(Error("gameplay.population.duplicate_tem…
```

#### `gameplay.population.invalid_template` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:150`)

```cpp
148 {
  149     if(definitions_frozen_)return foundation::Result<void>::Failure(Error("gameplay.population.definitions_frozen","population definitions are frozen"));
  150     if(!definition.id.IsValid())return foundation::Result<void>::Failure(Error("gameplay.population.invalid_template","invalid population template"));
  151     if(templates_.contains(definition.id))return foundation::Result<void>::Failure(Error("gameplay.population.duplicate_template","duplicate population template"));
  152     if(!CanAdvanceRevision())return foundation::Re…
```

#### `gameplay.population.duplicate_template` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:151`)

```cpp
149     if(definitions_frozen_)return foundation::Result<void>::Failure(Error("gameplay.population.definitions_frozen","population definitions are frozen"));
  150     if(!definition.id.IsValid())return foundation::Result<void>::Failure(Error("gameplay.population.invalid_template","invalid population template"));
  151     if(templates_.contains(definition.id))return foundation::Result<void>::Failure(Error("gameplay.population.duplicate_template","duplicate population template"));
  152     if(!CanAdvanceRevision())return foundation::Result<voi…
```

#### `gameplay.population.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:472`)

```cpp
470 foundation::Result<void> PopulationService::ReleaseAllocation(PopulationAllocationId id, GameplayContext context)
  471 {
  472     auto a=allocations_.find(id);if(a==allocations_.end())return foundation::Result<void>::Failure(Error("gameplay.population.allocation_missing","population allocation missing"));if(a->second.state==PopulationAllocationState::Released)return foundation::Result<void>::Success();if(a->second.state==PopulationAllocationState::Committed)return foundation::Result<void>::Failure(Error("gameplay.population.allocation_alr…
```

#### `gameplay.population.publication_failed` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:472`)

```cpp
470 foundation::Result<void> PopulationService::ReleaseAllocation(PopulationAllocationId id, GameplayContext context)
  471 {
  472     auto a=allocations_.find(id);if(a==allocations_.end())return foundation::Result<void>::Failure(Error("gameplay.population.allocation_missing","population allocation missing"));if(a->second.state==PopulationAllocationState::Released)return foundation::Result<void>::Success();if(a->second.state==PopulationAllocationState::Committed)return foundation::Result<void>::Failure(Error("gameplay.population.allocation_alr…
```

#### `gameplay.population.invalid_group` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:168`)

```cpp
166 foundation::Result<PopulationGroupId> PopulationService::CreateGroup(PopulationGroup group)
  167 {
  168     if(!group.area.IsValid()||!IsValidPopulationGroupState(group.state))return foundation::Result<PopulationGroupId>::Failure(Error("gameplay.population.invalid_group","invalid population group"));
  169     if(group.state==PopulationGroupState::Removed)return foundation::Result<PopulationGroupId>::Failure(Error("gameplay.population.invalid_group_state","cannot create a removed population group"));
  170     auto staged_ids=group_ids_;
```

#### `gameplay.population.invalid_group_state` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:169`)

```cpp
167 {
  168     if(!group.area.IsValid()||!IsValidPopulationGroupState(group.state))return foundation::Result<PopulationGroupId>::Failure(Error("gameplay.population.invalid_group","invalid population group"));
  169     if(group.state==PopulationGroupState::Removed)return foundation::Result<PopulationGroupId>::Failure(Error("gameplay.population.invalid_group_state","cannot create a removed population group"));
  170     auto staged_ids=group_ids_;
  171     if(!group.id.IsValid())group.id=PopulationGroupId{staged_ids.Next()};
```

#### `gameplay.population.id_exhausted` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:451`)

```cpp
449     if(!CanAdvanceRevision()||!CanRecordChanges(canonical.size()))return foundation::Result<PopulationAllocationBatch>::Failure(Error(!CanAdvanceRevision()?"gameplay.population.revision_exhausted":"gameplay.population.change_sequence_exhausted","population mutation metadata is exhausted"));
  450     auto staged_alloc_ids=allocation_ids_;auto staged_corr_ids=allocation_correlation_ids_;if(!request.correlation.IsValid())request.correlation=staged_corr_ids.Next();else AdvanceGeneratorPast(staged_corr_ids,PopulationAllocationId{request.correla…
```

#### `gameplay.population.duplicate_group` (`EngineFramework/GameplayWorldStateOwners/Population/src/population.cpp:173`)

```cpp
171     if(!group.id.IsValid())group.id=PopulationGroupId{staged_ids.Next()};
  172     if(!group.id.IsValid())return foundation::Result<PopulationGroupId>::Failure(Error("gameplay.population.id_exhausted","population group id generator exhausted"));
  173     if(groups_.contains(group.id))return foundation::Result<PopulationGroupId>::Failure(Error("gameplay.population.duplicate_group","duplicate population group"));
  174     AdvanceGeneratorPast(staged_ids,group.id);
  175     if(!CanAdvanceRevision())return foundation::Result<PopulationGroup…
```
