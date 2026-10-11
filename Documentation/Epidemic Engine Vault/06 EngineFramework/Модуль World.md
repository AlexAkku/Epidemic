---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "world"]
source_path: "EngineFramework/GameplayWorldStateOwners/World"
---

# EngineFramework / World

Модуль: **World**. World ownership, placement, residency.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/World/`.
**CMake targets:** `EpidemicGameFrameworkWorld`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/World/world.h|world.h]]: `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h`; объявленные типы: `WorldRegionId`, `WorldAreaId`, `LocationId`, `WorldFeatureId`, `WorldAlterationId`, `WorldAlterationTypeId`, `WorldFeatureTypeId`, `WorldPosition`, `WorldAabb`, `WorldRegionDefinition`, `WorldAreaDefinition`, `LocationDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/world|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `WorldAlterationState` (`EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h`): `Active`, `Superseded`, `Expired`, `Removed`, `Compacted`.
- `WorldAlterationPersistence` (`EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h`): `Transient`, `Session`, `Persistent`.
- `WorldChangeKind` (`EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h`): `AlterationCreated`, `AlterationUpdated`, `AlterationExpired`, `AlterationRemoved`, `AlterationCompacted`, `FeatureChanged`, `FeatureRemoved`, `ObjectPlaced`, `ObjectPlacementRemoved`.
- `Kind` (`EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h`): `Create`, `Update`, `Remove`.

### Операции и хранилища состояния

#### `WorldService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h:310`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
WorldService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterRegion(WorldRegionDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterArea(WorldAreaDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterLocation(LocationDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterFeatureType(WorldFeatureTypeId id, std::string canonical_name);
[[nodiscard]] foundation::Result<void> RegisterAlterationType(WorldAlterationTypeId id, std::string canonical_name);
[[nodiscard]] foundation::Result<void> AddStaticFeature(WorldFeatureRecord feature);
[[nodiscard]] foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const WorldRegionDefinition *FindRegion(WorldRegionId id) const noexcept;
[[nodiscard]] const WorldAreaDefinition *FindArea(WorldAreaId id) const noexcept;
[[nodiscard]] const LocationDefinition *FindLocation(LocationId id) const noexcept;
[[nodiscard]] std::optional<WorldFeatureRecord> FindFeature(WorldFeatureId id) const;
[[nodiscard]] std::optional<WorldAlterationRecord> FindAlteration(WorldAlterationId id) const;
[[nodiscard]] std::optional<ObjectPlacementRecord> FindObjectPlacement(GameplayObjectRef object) const noexcept;
[[nodiscard]] bool IsAlterationTypeRegistered(WorldAlterationTypeId id) const noexcept;
[[nodiscard]] std::vector<WorldAreaDefinition> FindAreasAt(WorldPosition position) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::size_t kChangeJournalCapacity = 4096`; `static constexpr std::int64_t kSpatialCellMm = 100000`; `static constexpr std::uint64_t kMaxIndexedCells = 4096`; `struct IdHash`; `std::unordered_map<WorldRegionId, WorldRegionDefinition, IdHash> regions_`; `std::unordered_map<WorldAreaId, WorldAreaDefinition, IdHash> areas_`; `std::unordered_map<LocationId, LocationDefinition, IdHash> locations_`; `std::unordered_map<WorldFeatureId, WorldFeatureRecord, IdHash> features_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h:441`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> std::size_t operator()(const T &id) const noexcept;
```

#### `WorldTransaction`

Объявлен в `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h:274`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
WorldTransaction(WorldService &owner, GameplayContext context);
[[nodiscard]] foundation::Result<WorldAlterationId> Create(WorldAlterationRecord record);
[[nodiscard]] foundation::Result<void> Update(WorldAlterationId id, WorldAlterationUpdate update);
[[nodiscard]] foundation::Result<void> Remove(WorldAlterationId id);
[[nodiscard]] foundation::Result<void> Commit();
void Cancel() noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `MonotonicIdGenerator<GameplayObjectId> staged_alteration_ids_`; `std::vector<Mutation> mutations_`; `bool committed_ = false, cancelled_ = false`.

#### `WorldAlterationId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h:74`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr WorldAlterationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `WorldAabb`

Объявлен в `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h:122`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool Contains(WorldPosition p) const noexcept;
```

#### `WorldPosition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h:117`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/World/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp` (1200 строк): `Error::Create`, `WorldTransaction::WorldTransaction`, `WorldTransaction::Create`, `WorldTransaction::Update`, `WorldTransaction::Remove`, `WorldTransaction::Commit`, `WorldService::WorldService`, `GameplayObjectId::FromString`, `WorldService::CellFor`, `WorldService::CellsFor`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.world.transaction_closed`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:159`
- `gameplay.world.invalid_alteration`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:875`
- `gameplay.world.alteration_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:102`
- `gameplay.world.transaction_duplicate_target`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:864`
- `gameplay.world.transaction_invalid`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:141`
- `framework.world.alteration.seed`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:169`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:315`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:387`
- `gameplay.world.invalid_region`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:332`
- `gameplay.already_registered`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:358`
- `gameplay.world.invalid_area`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:343`
- `gameplay.world.invalid_location`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:356`
- `gameplay.world.invalid_feature_type`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:370`
- `gameplay.world.invalid_alteration_type`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:380`
- `gameplay.world.invalid_feature`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:602`
- `gameplay.world.topology_invalid`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:427`
- `gameplay.world.topology_cycle`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:434`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1072`
- `gameplay.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1009`
- `dynamic_feature.publish`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:575`
- `gameplay.world.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1154`
- `gameplay.world.dynamic_feature_missing`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:644`
- `gameplay.world.invalid_object_placement`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:683`
- `gameplay.world.object_placement_missing`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:729`
- `gameplay.world.alteration_terminal`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:901`
- `gameplay.world.invalid_alteration_transition`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:813`
- `gameplay.world.alteration_missing`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1003`
- `alteration_transaction.publish`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:934`
- `gameplay.world.alteration_not_due`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:963`
- `gameplay.world.alteration_not_compactable`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1006`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1069`
- `gameplay.world.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:1135`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp`: `CHECK(`
- `EngineRuntime/World/tests/world_tests.cpp`: `TestRegisterAndFindRegion`, `TestDuplicateRegionIsRejected`, `TestRegisterAndFindChunk`, `TestChunkRegistrationValidation`, `TestChunkStateDefaultsToUnloadedAndCanChange`, `TestCreateObjectReturnsValidRuntimeId`, `TestFindObjectReturnsSnapshot`, `TestSetPlacementUpdatesPlacement`, `TestInvalidPlacementIsRejected`, `TestDuplicatePersistentIdIsRejected`, `TestSetResidencyUpdatesState`, `TestInvalidResidencyTransitionIsRejected`, `TestCommandRevisionConflictIsRejected`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.world.transaction_closed` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:159`)

```cpp
157     if (!owner_ || committed_ || cancelled_)
  158         return foundation::Result<void>::Failure(
  159             Error("gameplay.world.transaction_closed", "world transaction is closed"));
  160     auto result = owner_->CommitMutations(mutations_, context_);
  161     if (result)
```

#### `gameplay.world.invalid_alteration` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:875`)

```cpp
873                 if (alterations_.contains(id))
  874                     return foundation::Result<void>::Failure(
  875                         Error("gameplay.world.invalid_alteration", "alteration already exists"));
  876                 auto after = m.record;
  877                 after.revision = rev.Value();
```

#### `gameplay.world.alteration_id_exhausted` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:102`)

```cpp
100     if (!record.id.IsValid())
  101         return foundation::Result<WorldAlterationId>::Failure(
  102             Error("gameplay.world.alteration_id_exhausted", "world alteration id generator is exhausted"));
  103     for (const auto &mutation : mutations_)
  104     {
```

#### `gameplay.world.transaction_duplicate_target` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:864`)

```cpp
862             const auto id = m.kind == WorldTransaction::Kind::Create ? m.record.id : m.id;
  863             if (!targets.insert(id).second)
  864                 return foundation::Result<void>::Failure(Error("gameplay.world.transaction_duplicate_target",
  865                                                                "world transaction may mutate an alteration only once"));
  866             if (m.kind == WorldTransaction::Kind::Create)
```

#### `gameplay.world.transaction_invalid` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:141`)

```cpp
139     if (!owner_ || committed_ || cancelled_ || !id.IsValid())
  140         return foundation::Result<void>::Failure(
  141             Error("gameplay.world.transaction_invalid", "world remove is invalid"));
  142     for (const auto &mutation : mutations_)
  143     {
```

#### `framework.world.alteration.seed` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:169`)

```cpp
167 }
  168 
  169 WorldService::WorldService() : alteration_ids_(GameplayObjectId::FromString("framework.world.alteration.seed").High())
  170 {
  171 }
```

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:315`)

```cpp
313     if (!next)
  314         return foundation::Result<Revision>::Failure(
  315             Error("gameplay.revision_exhausted", "world revision counter is exhausted"));
  316     return foundation::Result<Revision>::Success(*next);
  317 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:387`)

```cpp
385 {
  386     if (frozen_)
  387         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "world registry is frozen"));
  388     if (!f.id.IsValid() || !f.type.IsValid() || !f.bounds.IsValid() || !feature_types_.contains(f.type) ||
  389         features_.contains(f.id))
```

#### `gameplay.world.invalid_region` (`EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:332`)

```cpp
330         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "world registry is frozen"));
  331     if (!d.id.IsValid() || d.canonical_name.empty())
  332         return foundation::Result<void>::Failure(Error("gameplay.world.invalid_region", "invalid region"));
  333     if (regions_.contains(d.id))
  334         return foundation::Result<void>::Failure(Error("gameplay.already_registered", "region already registered"));
```
