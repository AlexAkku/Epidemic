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
