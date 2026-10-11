---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "assets"]
source_path: "EngineRuntime/Assets"
---

# EngineRuntime / Assets

Модуль: **Assets**. Идентичность, metadata и locations ассетов.

**Реализация:** `EngineRuntime/Assets/`.
**CMake targets:** `EpidemicRuntimeAssets`
**Локальные test targets:** `EpidemicRuntimeAssetsTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Assets/asset_catalog.h|asset_catalog.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog.h`; объявленные типы: `IAssetCatalog`
- [[11 API/EngineRuntime/Assets/asset_catalog_writer.h|asset_catalog_writer.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog_writer.h`; объявленные типы: `IAssetCatalogWriter`
- [[11 API/EngineRuntime/Assets/asset_dependency_manifest.h|asset_dependency_manifest.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_dependency_manifest.h`; объявленные типы: `AssetDependencyManifest`
- [[11 API/EngineRuntime/Assets/asset_location.h|asset_location.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location.h`; объявленные типы: `AssetLocationKind`, `AssetLocation`
- [[11 API/EngineRuntime/Assets/asset_location_resolver.h|asset_location_resolver.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location_resolver.h`; объявленные типы: `IAssetLocationResolver`
- [[11 API/EngineRuntime/Assets/asset_metadata.h|asset_metadata.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_metadata.h`; объявленные типы: `AssetDependency`, `AssetMetadata`
- [[11 API/EngineRuntime/Assets/asset_services.h|asset_services.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_services.h`; объявленные типы: `AssetsOptions`, `AssetServices`
- [[11 API/EngineRuntime/Assets/asset_state.h|asset_state.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_state.h`; объявленные типы: `AssetState`
- [[11 API/EngineRuntime/Assets/asset_type.h|asset_type.h]]: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_type.h`; объявленные типы: `AssetType`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/assets|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Assets хранит immutable после регистрации metadata catalog независимо от того, загружен ли payload в память. Модуль связывает `AssetId`, `AssetType`, `AssetState`, logical `AssetLocation`, tags и dependency metadata. Он не читает payload, не владеет resource residency и не выполняет filesystem/package I/O.

#### Контракты

`IAssetCatalog` предоставляет detached read/query API. `IAssetCatalogWriter` регистрирует новые записи и необратимо переводит catalog в sealed state. Update/replace API отсутствует. `IAssetLocationResolver` возвращает сохранённый logical `AssetLocation`; он не преобразует его в host path и не монтирует package. `AssetServices` предоставляет эти interfaces над одним in-memory catalog.

`AssetMetadata::version` является версией metadata/content конкретного asset. Это не global catalog revision. Значение `0` для зарегистрированной metadata недопустимо.

#### Registration и location validation

Регистрация требует valid `AssetId`, `AssetType`, `AssetState` и `AssetLocationKind`. Path обязан быть непустым engine-root-relative значением без embedded NUL, host absolute/rooted form, UNC/device form, Windows drive-relative form и traversal за engine root. `PackageEntry` и `VirtualPath` требуют valid `mount_id`. `Generated` требует valid `generator_id`.

Raw path, dependency list и tag list ограничены `kMaxAssetPathBytes`, `kMaxAssetDependencies` и `kMaxAssetTags`. Limits проверяются до каталожной mutation. Dependency IDs и tags должны быть valid, self dependency запрещена, duplicate dependency и duplicate tag отклоняются. После validation path lexical-normalized. Allocation failure во время validation, normalization или insertion не публикует partial catalog entry.

#### Dependency manifest

`BuildDependencyManifest` обходит dependency graph итеративно. Required missing dependency завершает query controlled failure. Optional missing dependency остаётся в manifest. Diamond dependencies дедуплицируются по `AssetId`. Если один asset достижим и по optional, и по required edge, итоговый `required` является logical OR всех достижимых incoming edges и поэтому не зависит от registration или traversal order. Cycle возвращает controlled failure. Итоговый manifest сортируется по `AssetId::Raw()`.

#### Queries и lifecycle

`FindById`, `FindByType`, `FindByTag` и `Resolve` возвращают detached copies. Type/tag queries и dependency manifest имеют deterministic ordering. Invalid selector для non-Result query наблюдается как empty/not-contained. Result-producing `Resolve` и `BuildDependencyManifest` отличают invalid ID от valid-but-missing ID.

`Seal()` идемпотентен. После первого successful seal catalog больше не принимает registration и не возвращается в mutable state. Модуль не имеет shutdown/cleanup lifecycle, внешних callbacks/backends, generated revisions, journals или derived indexes.

Mutation выполняется на runtime thread. Concurrent read/write не поддерживается. После завершённого bootstrap и `Seal()` состояние больше не мутирует; const queries не имеют hidden mutation.

#### Persistence boundary

Assets не публикует snapshot/restore API. Goal 3.5 не вводит скрытый catalog persistence contract. Если persistent asset catalog потребуется composition/bootstrap layer, он должен производить обычные validated registrations через существующий writer contract.

#### Статус Goal 3

После локального аудита Goal 3.5, повторной MSVC Debug/Release qualification и canonical evidence convergence модуль имеет статус `LOCAL_READY`. Это не означает `SYSTEM_READY` или `FROZEN`; системные и финальные freeze gates остаются в последующих целях общего плана.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `asset_catalog.h`: `IAssetCatalog`.
- `asset_catalog_writer.h`: `IAssetCatalogWriter`.
- `asset_dependency_manifest.h`: `AssetDependencyManifest`.
- `asset_location.h`: `AssetLocationKind`, `AssetLocation`.
- `asset_location_resolver.h`: `IAssetLocationResolver`.
- `asset_metadata.h`: `AssetDependency`, `AssetMetadata`.
- `asset_services.h`: `AssetsOptions`, `AssetServices`.
- `asset_state.h`: `AssetState`.
- `asset_type.h`: `AssetType`.

Источник контрактов: `Documentation/EngineRuntime/modules/assets.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AssetState` (`EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_state.h`): `Unknown`, `Discovered`, `Indexed`, `Validated`, `Missing`, `Invalid`, `Deprecated`.
- `AssetLocationKind` (`EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location.h`): `FilePath`, `PackageEntry`, `VirtualPath`, `Generated`.

### Операции и хранилища состояния

#### `IAssetCatalog`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAssetCatalog() = default;
mutating the returned copy never mutates the catalog. [[nodiscard]] virtual std::optional<AssetMetadata> FindById(AssetId id) const = 0;
[[nodiscard]] virtual std::vector<AssetMetadata> FindByTag(foundation::StringId tag) const = 0;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `optional missing dependencies remain in`.

#### `IAssetCatalogWriter`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog_writer.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAssetCatalogWriter() = default;
```

#### `IAssetLocationResolver`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location_resolver.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAssetLocationResolver() = default;
```

#### `AssetServices`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_services.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IAssetCatalog> catalog`; `std::shared_ptr<IAssetCatalogWriter> writer`; `std::shared_ptr<IAssetLocationResolver> location_resolver`.

#### `AssetLocation`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location.h:31`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
AssetLocation() = default;
AssetLocation(AssetLocationKind location_kind, std::string location_path) : kind(location_kind), path(std::move(location_path));
[[nodiscard]] bool Empty() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `AssetLocationKind kind = AssetLocationKind::FilePath`; `std::string path`.

#### `AssetDependency`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_metadata.h:24`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool required = true`.

#### `AssetMetadata`

Объявлен в `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_metadata.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `AssetState state = AssetState::Unknown`; `std::vector<AssetDependency> dependencies`; `std::vector<foundation::StringId> tags`; `std::uint64_t content_hash = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Assets/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Assets/src/assets.cpp` (15 строк)
- `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp` (509 строк): `InMemoryAssetCatalog::RegisterAsset`, `Error::Create`, `InMemoryAssetCatalog::Seal`, `InMemoryAssetCatalog::FindById`, `InMemoryAssetCatalog::FindByType`, `InMemoryAssetCatalog::FindByTag`, `InMemoryAssetCatalog::Contains`, `InMemoryAssetCatalog::IsSealed`, `InMemoryAssetCatalog::BuildDependencyManifest`, `InMemoryAssetCatalog::Resolve`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `asset.catalog_sealed`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:173`
- `asset.duplicate`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:185`
- `asset.invalid_id`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:407`
- `asset.not_found`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:396`
- `asset.missing_dependency`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:341`
- `asset.dependency_cycle`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:363`
- `asset.invalid_type`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:413`
- `asset.invalid_state`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:419`
- `asset.invalid_location`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:503`
- `asset.metadata_too_large`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:441`
- `asset.invalid_version`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:453`
- `asset.invalid_dependency`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:483`
- `asset.self_dependency`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:467`
- `asset.invalid_tag`: `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:489`

### Конкретные тестовые якоря

- `EngineRuntime/Assets/tests/assets_tests.cpp`: `TestDefaultMetadataState`, `TestRegisterAssetStoresMetadata`, `TestValidationFailures`, `TestVersionGeneratedDependencyAndTagValidation`, `TestPathValidationRejectsTraversalAndRequiresMounts`, `TestDuplicateAssetIdReturnsError`, `TestSelfAndDuplicateDependenciesFail`, `TestCatalogSeal`, `TestFindByIdReturnsSnapshot`, `TestFindByTypeAndTagWork`, `TestMissingAssetReturnsNulloptAndResolveError`, `TestInvalidQueryInputsAreControlled`, `TestAssetLocationResolverReturnsRegisteredLocation`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `asset.catalog_sealed` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:173`)

```cpp
171     {
  172         return foundation::Result<void>::Failure(
  173             foundation::Error::Create("asset.catalog_sealed", "asset catalog is sealed and cannot be mutated"));
  174     }
  175
```

#### `asset.duplicate` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:185`)

```cpp
183     {
  184         return foundation::Result<void>::Failure(
  185             foundation::Error::Create("asset.duplicate", "asset id is already registered"));
  186     }
  187
```

#### `asset.invalid_id` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:407`)

```cpp
405     {
  406         return foundation::Result<AssetMetadata>::Failure(
  407             foundation::Error::Create("asset.invalid_id", "asset id must be valid before registration"));
  408     }
  409
```

#### `asset.not_found` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:396`)

```cpp
394     {
  395         return foundation::Result<AssetLocation>::Failure(
  396             foundation::Error::Create("asset.not_found", "asset location not registered"));
  397     }
  398
```

#### `asset.missing_dependency` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:341`)

```cpp
339             {
  340                 return foundation::Result<AssetDependencyManifest>::Failure(
  341                     foundation::Error::Create("asset.missing_dependency", "required asset dependency is not registered"));
  342             }
  343             const auto [requirement, inserted] = manifest_requirements.emplace(dependency.asset_id, dependency.required);
```

#### `asset.dependency_cycle` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:363`)

```cpp
361             {
  362                 return foundation::Result<AssetDependencyManifest>::Failure(
  363                     foundation::Error::Create("asset.dependency_cycle", "asset dependency graph contains a cycle"));
  364             }
  365             continue;
```

#### `asset.invalid_type` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:413`)

```cpp
411     {
  412         return foundation::Result<AssetMetadata>::Failure(
  413             foundation::Error::Create("asset.invalid_type", "asset type must be valid before registration"));
  414     }
  415
```

#### `asset.invalid_state` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:419`)

```cpp
417     {
  418         return foundation::Result<AssetMetadata>::Failure(
  419             foundation::Error::Create("asset.invalid_state", "asset state is outside the declared enum domain"));
  420     }
  421
```

#### `asset.invalid_location` (`EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:503`)

```cpp
501     {
  502         return foundation::Result<AssetMetadata>::Failure(
  503             foundation::Error::Create("asset.invalid_location", "asset location must contain a valid canonical path"));
  504     }
  505
```
