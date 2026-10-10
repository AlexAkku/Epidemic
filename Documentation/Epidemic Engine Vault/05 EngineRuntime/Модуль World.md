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
