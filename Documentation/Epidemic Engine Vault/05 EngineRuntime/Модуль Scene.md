---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "scene"]
source_path: "EngineRuntime/Scene"
---

# EngineRuntime / Scene

Модуль: **Scene**. Hierarchy и spatial representation.

**Реализация:** `EngineRuntime/Scene/`.
**CMake targets:** `EpidemicRuntimeScene`
**Локальные test targets:** `EpidemicRuntimeSceneTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Scene/bounds.h|bounds.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/bounds.h`
- [[11 API/EngineRuntime/Scene/scene_node.h|scene_node.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h`; объявленные типы: `SceneNodeId`, `SceneNode`, `hash`
- [[11 API/EngineRuntime/Scene/scene_node_registry.h|scene_node_registry.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h`; объявленные типы: `ReparentMode`, `ISceneNodeRegistry`
- [[11 API/EngineRuntime/Scene/scene_query.h|scene_query.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h`; объявленные типы: `SceneNodeSnapshot`, `SceneSnapshot`, `ISceneQuery`, `ISceneSnapshotProvider`
- [[11 API/EngineRuntime/Scene/scene_services.h|scene_services.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h`; объявленные типы: `SceneOptions`, `SceneServices`
- [[11 API/EngineRuntime/Scene/scene_state.h|scene_state.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`; объявленные типы: `SceneAttachmentState`, `SceneMobility`, `SceneVisibilityState`, `SceneDirtyFlags`
- [[11 API/EngineRuntime/Scene/spatial_index.h|spatial_index.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h`; объявленные типы: `ISpatialIndex`
- [[11 API/EngineRuntime/Scene/transform.h|transform.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform.h`
- [[11 API/EngineRuntime/Scene/transform_registry.h|transform_registry.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h`; объявленные типы: `WorldTransformWriteMode`, `ITransformRegistry`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/scene|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
