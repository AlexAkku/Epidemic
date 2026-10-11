---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "environment"]
source_path: "EngineFramework/GameplayWorldStateOwners/Environment"
---

# EngineFramework / Environment

Модуль: **Environment**. Нейтральное состояние окружения.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Environment/`.
**CMake targets:** `EpidemicGameFrameworkEnvironment`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Environment/environment.h|environment.h]]: `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`; объявленные типы: `EnvironmentLayerId`, `EnvironmentLayerTypeId`, `EnvironmentBlendHandlerId`, `EnvironmentPosition`, `EnvironmentAabb`, `EnvironmentBlendPolicy`, `EnvironmentPersistence`, `EnvironmentValueField`, `EnvironmentValues`, `EnvironmentHazardTypeId`, `EnvironmentHazard`, `EnvironmentLayer`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Environment|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns semantic gameplay environment layer types, blend handlers, hazard types and environment layers, including spatial indexing and semantic sampling. Runtime environment remains non-authoritative for this state.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`.

#### Dependency list

Direct production links are `EpidemicGameFrameworkFoundation` and `EpidemicFoundation`. No peer GameplayWorldStateOwner or Runtime module dependency is added by this delta.

#### External ports/callbacks/providers/backends

Custom blend handlers are registered callbacks. Their failure, invalid result and exception are contained as controlled sample failures; no Runtime Environment object is owned here.

#### Authoritative state

Type/blend/hazard registries; layer map; spatial cell index; global and large-layer lists; layer ID generator; revision; bounded environment change journal and diagnostics.

#### Derived/cache/index state

spatial_index_, global_layers_ and large_layers_ are derived from layers_. Update/remove/restore rebuild or maintain every index consistently.

#### ID spaces, generations, revisions and cursors

EnvironmentLayerId is service-generated or caller-supplied in-scope with generator advancement. Revision and change cursor are monotonic and restore carries a new journal epoch.

#### State machines

Definition registries are mutable until Freeze. Layers support add, update, explicit expire/remove and time-based sweep. Expired layers do not remain authoritative.

Источник контрактов: `Documentation/EngineFramework/Environment.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `EnvironmentBlendPolicy` (`EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`): `Override`, `Add`, `Multiply`, `Min`, `Max`, `CustomRegistered`.
- `EnvironmentPersistence` (`EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`): `Transient`, `Session`, `Persistent`.
- `EnvironmentValueField` (`EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`): `Temperature`, `Humidity`, `Precipitation`, `WindStrength`, `WindDirection`, `Visibility`, `LightExposure`.
- `EnvironmentChangeKind` (`EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`): `Added`, `Changed`, `Removed`, `Expired`.

### Операции и хранилища состояния

#### `EnvironmentService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:219`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
EnvironmentService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterLayerType(EnvironmentLayerTypeId id, std::string canonical_name);
[[nodiscard]] foundation::Result<void> RegisterBlendHandler(EnvironmentBlendHandlerId id, std::string canonical_name, BlendHandler handler);
[[nodiscard]] foundation::Result<void> RegisterHazardType(EnvironmentHazardTypeId id, std::string canonical_name);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] foundation::Result<EnvironmentLayerId> AddLayer(EnvironmentLayer layer);
[[nodiscard]] foundation::Result<void> UpdateLayer(EnvironmentLayer layer);
[[nodiscard]] std::optional<EnvironmentLayer> GetLayer(EnvironmentLayerId id) const noexcept;
[[nodiscard]] std::vector<EnvironmentLayer> FindLayers(EnvironmentPosition position) const;
[[nodiscard]] EnvironmentSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(EnvironmentSnapshot snapshot);
[[nodiscard]] EnvironmentChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
[[nodiscard]] EnvironmentDiagnostics GetDiagnostics() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::int64_t kSpatialCellMm = 100000`; `static constexpr std::uint64_t kMaxIndexedCells = 4096`; `static constexpr std::size_t kChangeJournalCapacity = 4096`; `std::unordered_map<EnvironmentLayerTypeId, std::string, TypeHash> types_`; `std::unordered_map<EnvironmentBlendHandlerId, std::pair<std::string, BlendHandler>, BlendHash> blend_handlers_`; `std::unordered_map<EnvironmentHazardTypeId, std::string, HazardHash> hazard_types_`; `std::unordered_map<EnvironmentLayerId, EnvironmentLayer, IdHash> layers_`; `std::unordered_map<CellKey, std::vector<EnvironmentLayerId>, CellHash> spatial_index_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:304`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
std::size_t operator()(EnvironmentLayerId id) const noexcept;
```

#### `EnvironmentLayerId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr EnvironmentLayerId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `EnvironmentAabb`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:54`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool Contains(EnvironmentPosition p) const noexcept;
```

#### `EnvironmentPosition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`.

#### `EnvironmentValues`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:103`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `EnvironmentFixed temperature_milli_c = 0`; `EnvironmentFixed humidity = 0`; `EnvironmentFixed precipitation = 0`; `EnvironmentFixed wind_strength = 0`; `EnvironmentFixed wind_x = 0, wind_y = 0, wind_z = 0`; `EnvironmentFixed visibility = kEnvironmentOne`; `EnvironmentFixed light_exposure = kEnvironmentOne`; `EnvironmentValueMask present = kEnvironmentAllValues`.

#### `EnvironmentHazard`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:125`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `EnvironmentFixed intensity = 0`; `GameplayTagSet tags`.

#### `EnvironmentLayer`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h:133`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `int priority = 0`; `EnvironmentBlendPolicy blend = EnvironmentBlendPolicy::Override`; `GameplayTagSet tags`; `std::vector<EnvironmentHazard> hazards`; `EnvironmentPersistence persistence = EnvironmentPersistence::Session`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Environment/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp` (839 строк): `Error::Create`, `EnvironmentService::EnvironmentService`, `GameplayObjectId::FromString`, `EnvironmentService::CellFor`, `EnvironmentService::CellsFor`, `EnvironmentService::IndexLayer`, `EnvironmentService::UnindexLayer`, `EnvironmentService::RebuildSpatialIndex`, `EnvironmentService::PrepareRevision`, `EnvironmentService::CanRecordChanges`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.environment.layer.seed`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:97`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:196`
- `gameplay.environment.invalid_values`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:266`
- `gameplay.environment.invalid_layer`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:365`
- `gameplay.environment.invalid_time_range`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:284`
- `gameplay.environment.invalid_hazard`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:297`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:331`
- `gameplay.environment.invalid_type`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:310`
- `gameplay.environment.invalid_blend_handler`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:321`
- `gameplay.environment.invalid_hazard_type`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:334`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:733`
- `gameplay.environment.layer_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:362`
- `gameplay.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:531`
- `gameplay.environment.storage_failed`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:780`
- `gameplay.environment.layer_missing`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:494`
- `gameplay.environment.layer_not_due`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:497`
- `gameplay.environment.blend_handler_missing`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:591`
- `gameplay.environment.invalid_blend_result`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:607`
- `gameplay.environment.blend_exception`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:624`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:729`
- `gameplay.environment.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:762`

### Конкретные тестовые якоря

- `EngineRuntime/Environment/tests/environment_tests.cpp`: `TestUnknownRegionAndSurfaceFail`, `TestSetGetWeatherSeasonClimateByRegion`, `TestDuplicateRegionRegistrationRejected`, `TestNoOpSettersDoNotBumpRevision`, `TestSurfaceStateStoresRegionAndMixedConditions`, `TestSurfaceRegionOwnershipIsImmutable`, `TestValidationRejectsInvalidValues`, `TestPartialRegionInitializationRejected`, `TestSnapshotAndProjectionAreRevisionedCopies`, `TestUpdateWithoutPolicyDoesNotMutateSurface`, `TestOptionalPolicyCanMutateSurface`, `TestApplyUpdateRejectsRevisionConflict`, `TestBatchNormalizesWindDirection`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.environment.layer.seed` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:97`)

```cpp
95 
   96 EnvironmentService::EnvironmentService()
   97     : ids_(GameplayObjectId::FromString("framework.environment.layer.seed").High())
   98 {
   99 }
```

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:196`)

```cpp
194     if (!next)
  195         return foundation::Result<Revision>::Failure(
  196             Error("gameplay.revision_exhausted", "environment revision counter is exhausted"));
  197     return foundation::Result<Revision>::Success(*next);
  198 }
```

#### `gameplay.environment.invalid_values` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:266`)

```cpp
264     {
  265         return foundation::Result<void>::Failure(
  266             Error("gameplay.environment.invalid_values", "environment values are outside supported ranges"));
  267     }
  268
```

#### `gameplay.environment.invalid_layer` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:365`)

```cpp
363     if (layers_.contains(layer.id))
  364         return foundation::Result<EnvironmentLayerId>::Failure(
  365             Error("gameplay.environment.invalid_layer", "duplicate environment layer"));
  366     if (auto valid = ValidateLayer(layer); !valid)
  367         return foundation::Result<EnvironmentLayerId>::Failure(valid.GetError());
```

#### `gameplay.environment.invalid_time_range` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:284`)

```cpp
282 
  283     if (layer.expires_at && layer.expires_at->ticks <= layer.created_at.ticks)
  284         return foundation::Result<void>::Failure(Error("gameplay.environment.invalid_time_range",
  285                                                         "environment layer expiration must be later than creation"));
  286
```

#### `gameplay.environment.invalid_hazard` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:297`)

```cpp
295         {
  296             return foundation::Result<void>::Failure(
  297                 Error("gameplay.environment.invalid_hazard", "environment hazard is invalid or duplicated"));
  298         }
  299     }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:331`)

```cpp
329 {
  330     if (frozen_)
  331         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "environment registry is frozen"));
  332     if (!id.IsValid() || canonical_name.empty() || hazard_types_.contains(id))
  333         return foundation::Result<void>::Failure(
```

#### `gameplay.environment.invalid_type` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:310`)

```cpp
308     if (!id.IsValid() || canonical_name.empty() || types_.contains(id))
  309         return foundation::Result<void>::Failure(
  310             Error("gameplay.environment.invalid_type", "invalid or duplicate environment layer type"));
  311     types_.emplace(id, std::move(canonical_name));
  312     return foundation::Result<void>::Success();
```

#### `gameplay.environment.invalid_blend_handler` (`EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:321`)

```cpp
319         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "environment registry is frozen"));
  320     if (!id.IsValid() || canonical_name.empty() || !handler || blend_handlers_.contains(id))
  321         return foundation::Result<void>::Failure(Error("gameplay.environment.invalid_blend_handler",
  322                                                         "invalid or duplicate environment blend handler"));
  323     blend_handlers_.emplace(id, std::make_pair(std::move(canonical_name), std::move(handler)));
```
