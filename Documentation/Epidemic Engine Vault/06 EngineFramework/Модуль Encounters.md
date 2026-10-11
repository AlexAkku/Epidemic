---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "encounters"]
source_path: "EngineFramework/GameplayWorldStateOwners/Encounters"
---

# EngineFramework / Encounters

Модуль: **Encounters**. Встречи и встречи персонажей.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Encounters/`.
**CMake targets:** `EpidemicGameFrameworkEncounters`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Encounters/encounters.h|encounters.h]]: `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`; объявленные типы: `EncounterDefinitionId`, `EncounterInstanceId`, `SpawnTableId`, `SpawnEntryId`, `SpawnPointId`, `SpawnRequestId`, `SpawnedEntityRecordId`, `RespawnRuleId`, `IdHash`, `EncounterState`, `SpawnPointState`, `SpawnRollPolicy`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Encounters|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Freeze unit: `EngineFramework/GameplayWorldStateOwners/Encounters`. Admission: 70 public callables, 68 mutation obligations, 2 lifecycle candidates, 14 stale-identity candidates, 0 external-boundary candidates.

#### Public contract review

- Definition/spawn-table registration and validation.
- Encounter active/terminal lifecycle and spawn request lifecycle.
- Spawn point/entity records, respawn rules and budget limits.
- Deterministic spawn selection through explicit random streams.
- Terminal pruning, stale identities and journal gap semantics.
- Snapshot/restore validates generators/references before swap.

#### Failure atomicity and boundary review

All revision-bearing mutations were reviewed for revision/generator exhaustion before authoritative mutation. Multi-container mutations were reviewed for rollback or staged publication. Snapshot restore builds/validates candidate state before live-state replacement. Allocation-failure evidence for this block no longer depends on the process-global Framework allocator helper; owned tests use module-local failure seams where allocation failure is evidence.

#### Persistence and deterministic reads

The module snapshot surface is treated as local in-memory persistence evidence for Goal 4. Non-empty roundtrip, invalid restore, generator/revision continuity, journal epoch/cursor behavior and failed-restore pre-state preservation are covered by the module test executable. Whole-engine ordered restore and replay remain Goal 5.

#### Test evidence

Primary regression source: `EngineFramework/DevelopmentInfrastructure/Tests/encounters_tests.cpp`.
Registered target: `EpidemicGameFrameworkEncountersTests`.
Local Linux verification in the worker environment used direct C++23 compilation with `-Wall -Wextra -Wpedantic -Werror` and executed the module test binary. The repository top-level CMake configure is Windows-only and therefore cannot be used in this Linux worker environment.

#### Goal 4 result

Block-local review result: LOCAL_READY candidate for serial integration. This document does not claim whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN`.

Источник контрактов: `Documentation/EngineFramework/Encounters.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `EncounterState` (`EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`): `AwaitingPopulationBinding`, `Active`, `Completed`, `Failed`, `Despawning`, `Expired`.
- `SpawnPointState` (`EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`): `Available`, `Reserved`, `Disabled`, `Occupied`, `Consumed`.
- `SpawnRollPolicy` (`EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`): `GuaranteedAll`, `WeightedOne`, `WeightedMany`, `IndependentChance`, `PickNWithoutReplacement`, `PopulationBacked`.
- `SpawnPersistencePolicy` (`EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`): `Transient`, `Session`, `Persistent`, `PopulationBacked`.
- `SpawnResultState` (`EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`): `Succeeded`, `Deferred`, `Rejected`, `Failed`.
- `EncounterChangeKind` (`EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h`): `EncounterCreated`, `EncounterActivated`, `EncounterCompleted`, `EncounterFailed`, `EncounterExpired`, `SpawnRequested`, `SpawnSucceeded`, `SpawnFailed`, `EntitySpawned`, `EntityDespawned`, `RespawnScheduled`, `RespawnTriggered`, `SpawnPointAdded`.

### Операции и хранилища состояния

#### `EncountersService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:379`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterSpawnTable(SpawnTable table);
[[nodiscard]] foundation::Result<void> RegisterEncounterDefinition(EncounterDefinition definition);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] foundation::Result<SpawnPointId> AddSpawnPoint(SpawnPoint point);
void SetBudgets(EncounterBudgets budgets) noexcept;
[[nodiscard]] SpawnResult SpawnEncounter(SpawnRequest request);
[[nodiscard]] foundation::Result<std::vector<TypeId>> PreviewSpawnArchetypes(SpawnRequest request) const;
[[nodiscard]] std::size_t PruneTerminalEncounters(std::size_t max_to_prune);
[[nodiscard]] std::vector<EncounterDefinition> FindDefinitionsByTag(TagId tag) const;
[[nodiscard]] const EncounterInstance *GetEncounterInstance(EncounterInstanceId id) const noexcept;
[[nodiscard]] const SpawnedEntityRecord *GetSpawnedEntityRecord(SpawnedEntityRecordId id) const noexcept;
[[nodiscard]] const SpawnTable *GetSpawnTable(SpawnTableId id) const noexcept;
[[nodiscard]] std::vector<EncounterInstance> FindActiveEncountersInArea(GameplayObjectRef area) const;
[[nodiscard]] std::vector<EncounterInstance> FindEncountersByState(EncounterState state) const;
[[nodiscard]] std::vector<SpawnPoint> FindSpawnPointsInArea(GameplayObjectRef area) const;
[[nodiscard]] bool CanSpawnEncounter(SpawnRequest request) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<EncounterDefinitionId, EncounterDefinition, IdHash> definitions_`; `std::unordered_map<SpawnTableId, SpawnTable, IdHash> tables_`; `std::unordered_map<SpawnPointId, SpawnPoint, IdHash> points_`; `std::unordered_map<EncounterInstanceId, EncounterInstance, IdHash> instances_`; `std::unordered_map<SpawnedEntityRecordId, SpawnedEntityRecord, IdHash> spawned_`; `std::unordered_map<RespawnRuleId, RespawnRule, IdHash> respawns_`; `std::unordered_map<SpawnRequestId, SpawnRequestRecord, IdHash> requests_`; `std::unordered_map<GameplayObjectRef, std::vector<EncounterInstanceId>> instances_by_area_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:149`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `EncounterInstanceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:31`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr EncounterInstanceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `SpawnPointId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:77`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr SpawnPointId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `SpawnRequestId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:95`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr SpawnRequestId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `SpawnedEntityRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:113`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr SpawnedEntityRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `RespawnRuleId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:131`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr RespawnRuleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `SpawnEntry`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Encounters/include/Epidemic/GameFramework/Encounters/encounters.h:214`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t weight = 1`; `std::uint32_t min_count = 1`; `std::uint32_t max_count = 1`; `std::vector<std::byte> payload`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Encounters/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp` (704 строк): `Error::Create`, `EncountersService::PrepareRevision`, `EncountersService::RegisterSpawnTable`, `EncountersService::RegisterEncounterDefinition`, `EncountersService::FreezeDefinitions`, `EncountersService::AddSpawnPoint`, `EncountersService::EntryMatches`, `EncountersService::RequestMatches`, `EncountersService::CanSpawnEncounter`, `EncountersService::ResolveEntries`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:80`
- `gameplay.encounters.definitions_frozen`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:115`
- `gameplay.encounters.invalid_table`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:89`
- `gameplay.encounters.duplicate_table`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:91`
- `gameplay.encounters.invalid_roll_policy`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:93`
- `gameplay.encounters.invalid_selection_count`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:98`
- `gameplay.encounters.invalid_entry`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:102`
- `gameplay.encounters.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:117`
- `gameplay.encounters.duplicate_definition`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:119`
- `gameplay.encounters.freeze_invalid_definition`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:133`
- `gameplay.encounters.invalid_spawn_point`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:141`
- `gameplay.encounters.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:529`
- `gameplay.encounters.duplicate_spawn_point`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:151`
- `encounter.spawn`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:210`
- `gameplay.encounters.selection_impossible`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:230`
- `gameplay.encounters.random_exhausted`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:285`
- `gameplay.encounters.invalid_weights`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:245`
- `gameplay.encounters.spawn_preview_rejected`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:266`
- `encounter.count`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:349`
- `gameplay.encounters.spawn_preview_overflow`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:290`
- `gameplay.encounters.spawn_preview_budget`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:294`
- `gameplay.encounters.spawned_record_missing`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:459`
- `gameplay.encounters.population_binding_not_expected`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:427`
- `gameplay.encounters.population_already_bound`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:429`
- `gameplay.encounters.population_unit_in_use`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:431`
- `gameplay.encounters.instance_missing`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:493`
- `gameplay.encounters.invalid_state`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:494`
- `gameplay.encounters.population_binding_incomplete`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:448`
- `gameplay.encounters.spawned_record_already_bound`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:460`
- `gameplay.encounters.invalid_respawn`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:518`
- `gameplay.encounters.invalid_respawn_point`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:523`
- `gameplay.encounters.respawn_time_overflow`: `EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:525`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/encounters_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:80`)

```cpp
78     const auto next = CheckedNext(revision_);
   79     if (!next)
   80         return foundation::Result<Revision>::Failure(Error("gameplay.revision_exhausted", "encounters revision counter is exhausted"));
   81     return foundation::Result<Revision>::Success(*next);
   82 }
```

#### `gameplay.encounters.definitions_frozen` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:115`)

```cpp
113 {
  114     if (definitions_frozen_)
  115         return foundation::Result<void>::Failure(Error("gameplay.encounters.definitions_frozen", "encounter definitions are frozen"));
  116     if (!d.id.IsValid() || !d.spawn_table.IsValid() || !tables_.contains(d.spawn_table) || !IsValid(d.persistence))
  117         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_definition", "invalid encounter definition"));
```

#### `gameplay.encounters.invalid_table` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:89`)

```cpp
87         return foundation::Result<void>::Failure(Error("gameplay.encounters.definitions_frozen", "encounter definitions are frozen"));
   88     if (!t.id.IsValid())
   89         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_table", "invalid spawn table"));
   90     if (tables_.contains(t.id))
   91         return foundation::Result<void>::Failure(Error("gameplay.encounters.duplicate_table", "duplicate spawn table"));
```

#### `gameplay.encounters.duplicate_table` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:91`)

```cpp
89         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_table", "invalid spawn table"));
   90     if (tables_.contains(t.id))
   91         return foundation::Result<void>::Failure(Error("gameplay.encounters.duplicate_table", "duplicate spawn table"));
   92     if (!IsValid(t.roll_policy))
   93         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_roll_policy", "invalid spawn roll policy"));
```

#### `gameplay.encounters.invalid_roll_policy` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:93`)

```cpp
91         return foundation::Result<void>::Failure(Error("gameplay.encounters.duplicate_table", "duplicate spawn table"));
   92     if (!IsValid(t.roll_policy))
   93         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_roll_policy", "invalid spawn roll policy"));
   94     if (t.selection_min_count > t.selection_max_count)
   95         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_selection_count", "invalid spawn table selection count"));
```

#### `gameplay.encounters.invalid_selection_count` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:98`)

```cpp
96     if ((t.roll_policy == SpawnRollPolicy::WeightedMany || t.roll_policy == SpawnRollPolicy::PickNWithoutReplacement) &&
   97         t.selection_max_count == 0)
   98         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_selection_count", "selection policy requires a positive count"));
   99     for (const auto& e : t.entries)
  100     {
```

#### `gameplay.encounters.invalid_entry` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:102`)

```cpp
100     {
  101         if (!e.id.IsValid() || !e.archetype.IsValid() || e.min_count > e.max_count)
  102             return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_entry", "invalid spawn entry"));
  103     }
  104     const auto next_revision = PrepareRevision();
```

#### `gameplay.encounters.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:117`)

```cpp
115         return foundation::Result<void>::Failure(Error("gameplay.encounters.definitions_frozen", "encounter definitions are frozen"));
  116     if (!d.id.IsValid() || !d.spawn_table.IsValid() || !tables_.contains(d.spawn_table) || !IsValid(d.persistence))
  117         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_definition", "invalid encounter definition"));
  118     if (definitions_.contains(d.id))
  119         return foundation::Result<void>::Failure(Error("gameplay.encounters.duplicate_definition", "dup…
```

#### `gameplay.encounters.duplicate_definition` (`EngineFramework/GameplayWorldStateOwners/Encounters/src/encounters.cpp:119`)

```cpp
117         return foundation::Result<void>::Failure(Error("gameplay.encounters.invalid_definition", "invalid encounter definition"));
  118     if (definitions_.contains(d.id))
  119         return foundation::Result<void>::Failure(Error("gameplay.encounters.duplicate_definition", "duplicate encounter definition"));
  120     const auto next_revision = PrepareRevision();
  121     if (!next_revision) return foundation::Result<void>::Failure(next_revision.GetError());
```
