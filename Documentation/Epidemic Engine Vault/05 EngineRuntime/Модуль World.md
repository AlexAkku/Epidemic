---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "world"]
source_path: "EngineRuntime/World"
---

# EngineRuntime / World

Модуль: **World**. World ownership, placement, residency.

**Реализация:** `EngineRuntime/World/`.
**CMake targets:** `EpidemicRuntimeWorld`
**Локальные test targets:** `EpidemicRuntimeWorldTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/World/chunk.h|chunk.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h`; объявленные типы: `ChunkDescriptor`, `ChangeChunkStateCommand`, `ChunkSnapshot`, `IChunkRegistry`
- [[11 API/EngineRuntime/World/object_materialization.h|object_materialization.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h`; объявленные типы: `MaterializationState`, `MaterializationRequest`, `DemotionCommitToken`, `DemotionSnapshot`, `DemotionRequest`, `IObjectMaterializer`, `IDemotionCommitAuthority`
- [[11 API/EngineRuntime/World/object_placement.h|object_placement.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/object_placement.h`; объявленные типы: `WorldSurfacePlacement`, `ContainerPlacement`, `InventoryPlacement`, `EquippedPlacement`, `HiddenPlacement`, `DestroyedPlacement`
- [[11 API/EngineRuntime/World/region.h|region.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/region.h`; объявленные типы: `RegionDescriptor`, `IRegionRegistry`
- [[11 API/EngineRuntime/World/world_commands.h|world_commands.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_commands.h`; объявленные типы: `WorldCommandResult`, `CreateObjectCommand`, `ChangePlacementCommand`, `ChangeResidencyCommand`, `PromotePersistenceTierCommand`, `MaterializeObjectCommand`, `DemoteObjectCommand`, `DestroyObjectCommand`
- [[11 API/EngineRuntime/World/world_invariants.h|world_invariants.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_invariants.h`
- [[11 API/EngineRuntime/World/world_location.h|world_location.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_location.h`; объявленные типы: `WorldLocation`
- [[11 API/EngineRuntime/World/world_object.h|world_object.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_object.h`; объявленные типы: `WorldObjectRecord`
- [[11 API/EngineRuntime/World/world_object_registry.h|world_object_registry.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_object_registry.h`; объявленные типы: `IWorldObjectRegistry`
- [[11 API/EngineRuntime/World/world_query.h|world_query.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_query.h`; объявленные типы: `IWorldQuery`
- [[11 API/EngineRuntime/World/world_services.h|world_services.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_services.h`; объявленные типы: `WorldOptions`, `WorldServices`
- [[11 API/EngineRuntime/World/world_state.h|world_state.h]]: `EngineRuntime/World/include/Epidemic/Runtime/World/world_state.h`; объявленные типы: `ChunkState`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/world|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

World хранит логическую модель объектов и пространства мира независимо от их текущего render/physics representation. Он описывает regions, chunks, placement, reality level, residency, persistence tier и materialization.

#### Объекты

`IWorldObjectRegistry` применяет revisioned commands создания, перемещения, изменения residency, продвижения persistence tier и уничтожения. `IWorldQuery` выполняет detached queries. Placement является variant: world surface, container, inventory, equipped, hidden или destroyed.

Destroyed object terminal. Обычная команда не может выставить `DestroyedPlacement` и не может воскресить запись. Объекты уровня `PlayerTouched` и выше требуют уникальный `PersistentObjectId`.

#### Chunks и regions

`IRegionRegistry` и `IChunkRegistry` регистрируют географию. Chunk state меняется через `ChangeChunkStateCommand` с expected revision и разрешенной state transition. Unknown chunk не маскируется состоянием Unloaded.

#### Materialization

`IObjectMaterializer` создает concrete runtime representation только для совместимого placement. Demotion требует runtime-issued `DemotionCommitToken`; tokens можно revoke и они инвалидируются при изменении object revision.

#### Стабильность

После прохождения Goal 3.8 и canonical evidence convergence World имеет статус `LOCAL_READY`. До обязательных системных gates Goals 5–9 модуль не считается `SYSTEM_READY` или `FROZEN`. Он не содержит inventory rules, корабли, NPC или quests; GameFramework строит их поверх neutral records.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `chunk.h`: `ChunkDescriptor`, `ChangeChunkStateCommand`, `ChunkSnapshot`, `IChunkRegistry`.
- `object_materialization.h`: `MaterializationState`, `MaterializationRequest`, `DemotionCommitToken`, `DemotionSnapshot`, `DemotionRequest`, `IObjectMaterializer`, `IDemotionCommitAuthority`.
- `object_placement.h`: `WorldSurfacePlacement`, `ContainerPlacement`, `InventoryPlacement`, `EquippedPlacement`, `HiddenPlacement`, `DestroyedPlacement`.
- `region.h`: `RegionDescriptor`, `IRegionRegistry`.
- `world_commands.h`: `WorldCommandResult`, `CreateObjectCommand`, `ChangePlacementCommand`, `ChangeResidencyCommand`, `PromotePersistenceTierCommand`, `MaterializeObjectCommand`, `DemoteObjectCommand`, `DestroyObjectCommand`.
- `world_invariants.h`: factory-функции или backend implementation без отдельного публичного типа.
- `world_location.h`: `WorldLocation`.
- `world_object.h`: `WorldObjectRecord`.
- `world_object_registry.h`: `IWorldObjectRegistry`.
- `world_query.h`: `IWorldQuery`.
- `world_services.h`: `WorldOptions`, `WorldServices`.
- `world_state.h`: `ChunkState`.

Источник контрактов: `Documentation/EngineRuntime/modules/world.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ChunkState` (`EngineRuntime/World/include/Epidemic/Runtime/World/world_state.h`): `Unloaded`, `Loading`, `Resident`, `Active`, `Sleeping`, `Unloading`.
- `MaterializationState` (`EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h`): `NotMaterialized`, `Requested`, `Materializing`, `Materialized`, `Demoting`, `Demoted`, `Failed`.

### Операции и хранилища состояния

#### `IChunkRegistry`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h:59`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IChunkRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterChunk(ChunkDescriptor chunk) = 0;
[[nodiscard]] virtual std::optional<ChunkDescriptor> FindChunk(ChunkId id) const = 0;
[[nodiscard]] virtual foundation::Result<ChunkSnapshot> GetChunkSnapshot(ChunkId id) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetChunkState(ChangeChunkStateCommand command) = 0;
virtual void Freeze() noexcept = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

#### `IWorldObjectRegistry`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/world_object_registry.h:9`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IWorldObjectRegistry() = default;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const CreateObjectCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangePlacementCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangeResidencyCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const PromotePersistenceTierCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const DestroyObjectCommand& command) = 0;
```

#### `IRegionRegistry`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/region.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRegionRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterRegion(RegionDescriptor region) = 0;
[[nodiscard]] virtual std::optional<RegionDescriptor> FindRegion(RegionId id) const = 0;
virtual void Freeze() noexcept = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

#### `IWorldQuery`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/world_query.h:9`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IWorldQuery() = default;
[[nodiscard]] virtual std::optional<WorldObjectRecord> FindObject(RuntimeObjectId id) const = 0;
[[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInRegion(RegionId region) const = 0;
[[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInChunk(ChunkId chunk) const = 0;
[[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsByReality(ObjectRealityLevel reality) const = 0;
```

#### `IObjectMaterializer`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h:69`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IObjectMaterializer() = default;
[[nodiscard]] virtual foundation::Result<RuntimeObjectId> Materialize(const MaterializationRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> Demote(const DemotionRequest& request) = 0;
```

#### `IDemotionCommitAuthority`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h:78`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IDemotionCommitAuthority() = default;
[[nodiscard]] virtual foundation::Result<DemotionCommitToken> IssueDemotionCommitToken(DemotionSnapshot snapshot) = 0;
[[nodiscard]] virtual foundation::Result<void> RevokeDemotionCommitToken(std::uint64_t token_id) = 0;
```

#### `WorldServices`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/world_services.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IRegionRegistry> regions`; `std::shared_ptr<IChunkRegistry> chunks`; `std::shared_ptr<IWorldObjectQuery> query`; `std::shared_ptr<IWorldObjectWriter> writer`; `std::shared_ptr<IWorldMaterializationRuntime> materialization`; `std::shared_ptr<IWorldDemotionAuthority> demotion_authority`.

#### `DemotionCommitToken`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t token_id = 0`; `ObjectRealityLevel target_reality = ObjectRealityLevel::Logical`; `std::uint64_t source_revision = 0`.

#### `ChunkDescriptor`

Объявлен в `EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int32_t x = 0`; `std::int32_t y = 0`; `std::int32_t z = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/World/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/World/src/world.cpp` (31 строк)
- `EngineRuntime/World/src/world_runtime_impl.cpp` (1002 строк): `Error::Create`, `WorldRuntime::RegisterRegion`, `WorldRuntime::Freeze`, `WorldRuntime::IsFrozen`, `WorldRuntime::FindRegion`, `WorldRuntime::RegisterChunk`, `WorldRuntime::FindChunk`, `WorldRuntime::GetChunkSnapshot`, `WorldRuntime::SetChunkState`, `WorldRuntime::Apply`
- `EngineRuntime/World/src/world_invariants.cpp` (228 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `world.expected_revision_required`: `EngineRuntime/World/src/world_runtime_impl.cpp:227`
- `world.revision_conflict`: `EngineRuntime/World/src/world_runtime_impl.cpp:232`
- `world.revision_overflow`: `EngineRuntime/World/src/world_runtime_impl.cpp:246`
- `world.topology_frozen`: `EngineRuntime/World/src/world_runtime_impl.cpp:164`
- `world.invalid_region`: `EngineRuntime/World/src/world_runtime_impl.cpp:175`
- `world.duplicate_region`: `EngineRuntime/World/src/world_runtime_impl.cpp:132`
- `world.invalid_chunk`: `EngineRuntime/World/src/world_runtime_impl.cpp:169`
- `world.region_not_found`: `EngineRuntime/World/src/world_invariants.cpp:222`
- `world.duplicate_chunk`: `EngineRuntime/World/src/world_runtime_impl.cpp:187`
- `world.chunk_not_found`: `EngineRuntime/World/src/world_invariants.cpp:171`
- `world.invalid_chunk_transition`: `EngineRuntime/World/src/world_runtime_impl.cpp:237`
- `command.dispatch`: `EngineRuntime/World/src/world_runtime_impl.cpp:681`
- `command.execute`: `EngineRuntime/World/src/world_runtime_impl.cpp:683`
- `world.duplicate_persistent_object`: `EngineRuntime/World/src/world_runtime_impl.cpp:515`
- `world.runtime_object_id_exhausted`: `EngineRuntime/World/src/world_runtime_impl.cpp:270`
- `world.duplicate_runtime_object`: `EngineRuntime/World/src/world_runtime_impl.cpp:291`
- `world.allocation_failed`: `EngineRuntime/World/src/world_runtime_impl.cpp:959`
- `world.object_not_found`: `EngineRuntime/World/src/world_runtime_impl.cpp:904`
- `world.destroyed_terminal`: `EngineRuntime/World/src/world_runtime_impl.cpp:909`
- `world.destroyed_requires_destroy_command`: `EngineRuntime/World/src/world_runtime_impl.cpp:357`
- `world.invalid_state_enum`: `EngineRuntime/World/src/world_invariants.cpp:131`
- `world.invalid_residency_transition`: `EngineRuntime/World/src/world_runtime_impl.cpp:405`
- `world.forbidden_persistence_downgrade`: `EngineRuntime/World/src/world_runtime_impl.cpp:458`
- `world.invalid_persistent_object`: `EngineRuntime/World/src/world_runtime_impl.cpp:553`
- `world.persistent_id_immutable`: `EngineRuntime/World/src/world_runtime_impl.cpp:476`
- `world.persistent_id_required`: `EngineRuntime/World/src/world_invariants.cpp:140`
- `world.invalid_reality_promotion`: `EngineRuntime/World/src/world_runtime_impl.cpp:577`
- `world.materialization_placement_incompatible`: `EngineRuntime/World/src/world_runtime_impl.cpp:588`
- `world.invalid_reality_demotion`: `EngineRuntime/World/src/world_runtime_impl.cpp:637`
- `world.demotion_not_confirmed`: `EngineRuntime/World/src/world_runtime_impl.cpp:650`
- `world.invalid_object`: `EngineRuntime/World/src/world_invariants.cpp:127`
- `world.dependent_objects_exist`: `EngineRuntime/World/src/world_runtime_impl.cpp:718`

### Конкретные тестовые якоря

- `EngineRuntime/World/tests/world_tests.cpp`: `TestRegisterAndFindRegion`, `TestDuplicateRegionIsRejected`, `TestRegisterAndFindChunk`, `TestChunkRegistrationValidation`, `TestChunkStateDefaultsToUnloadedAndCanChange`, `TestCreateObjectReturnsValidRuntimeId`, `TestFindObjectReturnsSnapshot`, `TestSetPlacementUpdatesPlacement`, `TestInvalidPlacementIsRejected`, `TestDuplicatePersistentIdIsRejected`, `TestSetResidencyUpdatesState`, `TestInvalidResidencyTransitionIsRejected`, `TestCommandRevisionConflictIsRejected`
- `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp`: `CHECK(`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `world.expected_revision_required` (`EngineRuntime/World/src/world_runtime_impl.cpp:227`)

```cpp
225     {
  226         return foundation::Result<void>::Failure(
  227             MakeWorldError("world.expected_revision_required", "chunk state commands must declare an expected chunk revision"));
  228     }
  229     if (command.expected_revision != iterator->second.revision)
```

#### `world.revision_conflict` (`EngineRuntime/World/src/world_runtime_impl.cpp:232`)

```cpp
230     {
  231         return foundation::Result<void>::Failure(
  232             MakeWorldError("world.revision_conflict", "chunk revision does not match command expectation"));
  233     }
  234     if (!CanTransition(iterator->second.state, command.state))
```

#### `world.revision_overflow` (`EngineRuntime/World/src/world_runtime_impl.cpp:246`)

```cpp
244     {
  245         return foundation::Result<void>::Failure(
  246             MakeWorldError("world.revision_overflow", "chunk revision cannot advance beyond UINT64_MAX"));
  247     }
  248
```

#### `world.topology_frozen` (`EngineRuntime/World/src/world_runtime_impl.cpp:164`)

```cpp
162     if (topology_frozen_)
  163     {
  164         return foundation::Result<void>::Failure(MakeWorldError("world.topology_frozen", "region/chunk topology registry is frozen"));
  165     }
  166     if (!chunk.id.IsValid())
```

#### `world.invalid_region` (`EngineRuntime/World/src/world_runtime_impl.cpp:175`)

```cpp
173     {
  174         return foundation::Result<void>::Failure(
  175             MakeWorldError("world.invalid_region", "chunk region id must be valid before registration"));
  176     }
  177
```

#### `world.duplicate_region` (`EngineRuntime/World/src/world_runtime_impl.cpp:132`)

```cpp
130     {
  131         return foundation::Result<void>::Failure(
  132             MakeWorldError("world.duplicate_region", "region id is already registered"));
  133     }
  134
```

#### `world.invalid_chunk` (`EngineRuntime/World/src/world_runtime_impl.cpp:169`)

```cpp
167     {
  168         return foundation::Result<void>::Failure(
  169             MakeWorldError("world.invalid_chunk", "chunk id must be valid before registration"));
  170     }
  171
```

#### `world.region_not_found` (`EngineRuntime/World/src/world_invariants.cpp:222`)

```cpp
220         if (hidden->region.IsValid() && !regions.FindRegion(hidden->region))
  221         {
  222             return WorldInvariantFailure("world.region_not_found", "hidden placement region was not found");
  223         }
  224     }
```

#### `world.duplicate_chunk` (`EngineRuntime/World/src/world_runtime_impl.cpp:187`)

```cpp
185     {
  186         return foundation::Result<void>::Failure(
  187             MakeWorldError("world.duplicate_chunk", "chunk id is already registered"));
  188     }
  189
```
