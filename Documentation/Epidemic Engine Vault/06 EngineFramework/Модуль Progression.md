---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "progression"]
source_path: "EngineFramework/GameplayWorldStateOwners/Progression"
---

# EngineFramework / Progression

Модуль: **Progression**. Gameplay progression.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Progression/`.
**CMake targets:** `EpidemicGameFrameworkProgression`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Progression/progression.h|progression.h]]: `EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h`; объявленные типы: `ProgressionChange`, `AttributeTypeId`, `ProgressionTrackId`, `PerkDefinitionId`, `UnlockTypeId`, `MilestoneId`, `UnlockDefinitionId`, `ProgressionModifierId`, `ProgressionGrantReservationId`, `IdHash`, `DerivedTerm`, `AttributeDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Progression|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns progression profiles, attributes, tracks/ranks, modifiers, perks, unlocks, milestones, progress-grant reservations, ID generators and change journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h`. B06 changes no public header/signature.

#### Dependency list

Direct production links are GameFramework Foundation and EngineBase Foundation only.

#### External ports/callbacks/providers/backends

No durable external backend. Prerequisites and rewards are evaluated against service-owned profile/definition state.

#### Authoritative state

Definition registries, profiles, modifier and reservation generators, pending progress reservations, global/profile revisions and bounded journal.

#### Derived/cache/index state

Rank is derived deterministically from track thresholds. Milestone ordering is canonicalized before reward evaluation.

#### ID spaces, generations, revisions and cursors

Modifier and progress-reservation IDs use monotonic generators. Global/profile/track revisions and journal cursor are checked at mutation boundaries.

#### State machines

Profile mutation and reserve/commit/release progress grant form local transactions. Milestones are terminal achievements and reward deduplication is preserved on retry.

Источник контрактов: `Documentation/EngineFramework/Progression.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ProgressionPrerequisiteKind` (`EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h`): `TrackProgressAtLeast`, `TrackRankAtLeast`, `HasPerk`, `HasUnlock`.
- `ModifierOperation` (`EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h`): `BaseAdd`, `BaseMultiply`, `FinalAdd`, `FinalMultiply`, `Override`, `ClampMin`, `ClampMax`.
- `ProgressionChangeKind` (`EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h`): `ProfileCreated`, `ProfileRemoved`, `AttributeChanged`, `ModifierAdded`, `ModifierRemoved`, `TrackProgressChanged`, `TrackRankChanged`, `PerkGranted`, `PerkRevoked`, `UnlockGranted`, `UnlockRevoked`, `MilestoneReached`.

### Операции и хранилища состояния

#### `ProgressionService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h:278`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
ProgressionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<AttributeTypeId> RegisterAttribute(AttributeDefinition definition);
[[nodiscard]] foundation::Result<ProgressionTrackId> RegisterTrack(ProgressionTrackDefinition definition);
[[nodiscard]] foundation::Result<PerkDefinitionId> RegisterPerk(PerkDefinition definition);
[[nodiscard]] foundation::Result<UnlockDefinitionId> RegisterUnlockDefinition(UnlockDefinition definition);
[[nodiscard]] foundation::Result<MilestoneId> RegisterMilestone(MilestoneDefinition definition);
[[nodiscard]] foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] bool HasProfile(GameplayObjectRef subject) const noexcept;
[[nodiscard]] foundation::Result<std::int64_t> GetAttribute(GameplayObjectRef subject, AttributeTypeId attribute) const;
[[nodiscard]] foundation::Result<ProgressionProfileSnapshot> GetProfileSnapshot(GameplayObjectRef subject) const;
[[nodiscard]] foundation::Result<ProgressionTrackState> GetTrack(GameplayObjectRef subject, ProgressionTrackId track) const;
void CommitProgressGrant(ProgressionGrantReservationId reservation) noexcept;
void ReleaseProgressGrant(ProgressionGrantReservationId reservation) noexcept;
[[nodiscard]] bool HasPerk(GameplayObjectRef subject, PerkDefinitionId perk) const noexcept;
[[nodiscard]] bool HasUnlock(GameplayObjectRef subject, UnlockTypeId type, TypeId value) const noexcept;
[[nodiscard]] PrerequisiteEvaluation QueryPrerequisites(GameplayObjectRef subject, const std::vector<ProgressionPrerequisite>& prerequisites) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<AttributeTypeId, AttributeDefinition, IdHash> attributes_`; `std::unordered_map<ProgressionTrackId, ProgressionTrackDefinition, IdHash> tracks_`; `std::unordered_map<PerkDefinitionId, PerkDefinition, IdHash> perks_`; `std::unordered_map<UnlockDefinitionId, UnlockDefinition, IdHash> unlock_definitions_`; `std::unordered_map<MilestoneId, MilestoneDefinition, IdHash> milestones_`; `std::unordered_map<GameplayObjectRef, Profile> profiles_`; `MonotonicIdGenerator<GameplayObjectId> modifier_ids_`; `MonotonicIdGenerator<GameplayObjectId> grant_reservation_ids_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h:75`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept;
```

#### `ProgressionTrackPolicy`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Progression/include/Epidemic/GameFramework/Progression/progression.h:101`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
std::int64_t max_progress_micro = std::numeric_limits<std::int64_t>::max();
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t min_progress_micro = 0`; `bool allow_decrease = false`; `bool allow_direct_set = false`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Progression/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp` (1611 строк): `Error::Create`, `ProgressionService::ProgressionService`, `GameplayObjectId::FromString`, `ProgressionService::RegisterAttribute`, `AttributeTypeId::FromString`, `ProgressionService::RegisterTrack`, `ProgressionTrackId::FromString`, `ProgressionService::RegisterPerk`, `PerkDefinitionId::FromString`, `ProgressionService::RegisterUnlockDefinition`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.progression.modifiers`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:126`
- `framework.progression.grant_reservations`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:127`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:221`
- `gameplay.progression.attribute_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:148`
- `gameplay.progression.track_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:173`
- `gameplay.progression.perk_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:192`
- `gameplay.progression.unlock_definition_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:211`
- `gameplay.progression.milestone_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:284`
- `gameplay.progression.attribute_cycle`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:454`
- `gameplay.progression.prerequisite_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:287`
- `gameplay.progression.unlock_definition_missing`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1353`
- `gameplay.progression.subject_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:301`
- `gameplay.progression.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1377`
- `gameplay.progression.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1589`
- `gameplay.progression.profile_missing`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1405`
- `gameplay.progression.profile_reserved`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1252`
- `gameplay.progression.attribute_missing`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:451`
- `gameplay.progression.modifier_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:632`
- `gameplay.progression.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:822`
- `gameplay.already_registered`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:651`
- `gameplay.progression.modifier_missing`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:580`
- `gameplay.progression.track_missing`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1339`
- `gameplay.progression.decrease_forbidden`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:815`
- `gameplay.progression.direct_set_forbidden`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:745`
- `gameplay.progression.perk_missing`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1062`
- `gameplay.progression.unlock_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1118`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1494`
- `gameplay.progression.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:1585`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/progression_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.progression.modifiers` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:126`)

```cpp
124 
  125 ProgressionService::ProgressionService()
  126     : modifier_ids_(GameplayObjectId::FromString("framework.progression.modifiers").High()),
  127       grant_reservation_ids_(GameplayObjectId::FromString("framework.progression.grant_reservations").High())
  128 {
```

#### `framework.progression.grant_reservations` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:127`)

```cpp
125 ProgressionService::ProgressionService()
  126     : modifier_ids_(GameplayObjectId::FromString("framework.progression.modifiers").High()),
  127       grant_reservation_ids_(GameplayObjectId::FromString("framework.progression.grant_reservations").High())
  128 {
  129 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:221`)

```cpp
219     if (frozen_)
  220         return foundation::Result<MilestoneId>::Failure(
  221             Error("gameplay.registry_frozen", "progression registry is frozen"));
  222     if (definition.canonical_name.empty() || !definition.track.IsValid())
  223         return foundation::Result<MilestoneId>::Failure(
```

#### `gameplay.progression.attribute_invalid` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:148`)

```cpp
146         if (!term.source.IsValid())
  147             return foundation::Result<AttributeTypeId>::Failure(
  148                 Error("gameplay.progression.attribute_invalid", "derived term has an invalid source"));
  149     const auto id = definition.id;
  150     attributes_.emplace(id, std::move(definition));
```

#### `gameplay.progression.track_invalid` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:173`)

```cpp
171         if (threshold < policy.min_progress_micro || threshold > policy.max_progress_micro)
  172             return foundation::Result<ProgressionTrackId>::Failure(
  173                 Error("gameplay.progression.track_invalid", "rank threshold is outside the track policy range"));
  174     const auto id = definition.id;
  175     tracks_.emplace(id, std::move(definition));
```

#### `gameplay.progression.perk_invalid` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:192`)

```cpp
190     if (definition.id != expected || perks_.contains(definition.id))
  191         return foundation::Result<PerkDefinitionId>::Failure(
  192             Error("gameplay.progression.perk_invalid", "invalid or duplicate perk"));
  193     const auto id = definition.id;
  194     perks_.emplace(id, std::move(definition));
```

#### `gameplay.progression.unlock_definition_invalid` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:211`)

```cpp
209     if (definition.id != expected || unlock_definitions_.contains(definition.id))
  210         return foundation::Result<UnlockDefinitionId>::Failure(
  211             Error("gameplay.progression.unlock_definition_invalid", "invalid or duplicate unlock definition"));
  212     const auto id = definition.id;
  213     unlock_definitions_.emplace(id, std::move(definition));
```

#### `gameplay.progression.milestone_invalid` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:284`)

```cpp
282             milestone.threshold_micro > track->second.policy.max_progress_micro)
  283             return foundation::Result<void>::Failure(
  284                 Error("gameplay.progression.milestone_invalid", "milestone references an invalid track or threshold"));
  285         if (!std::all_of(milestone.prerequisites.begin(), milestone.prerequisites.end(), prerequisite_valid))
  286             return foundation::Result<void>::Failure(
```

#### `gameplay.progression.attribute_cycle` (`EngineFramework/GameplayWorldStateOwners/Progression/src/progression.cpp:454`)

```cpp
452     if (!visiting.insert(attribute).second)
  453         return foundation::Result<std::int64_t>::Failure(
  454             Error("gameplay.progression.attribute_cycle", "attribute cycle during evaluation"));
  455 
  456     std::int64_t value = def->second.default_micro;
```
