---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "physics"]
source_path: "EngineRuntime/Physics"
---

# EngineRuntime / Physics

Модуль: **Physics**. Physics scene и simulation bodies.

**Реализация:** `EngineRuntime/Physics/`.
**CMake targets:** `EpidemicRuntimePhysics`
**Локальные test targets:** `EpidemicRuntimePhysicsTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Physics/physics_event_buffer.h|physics_event_buffer.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h`; объявленные типы: `IPhysicsEventBuffer`
- [[11 API/EngineRuntime/Physics/physics_query.h|physics_query.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h`; объявленные типы: `IPhysicsQuery`
- [[11 API/EngineRuntime/Physics/physics_scene.h|physics_scene.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h`; объявленные типы: `ICollisionShapeRegistry`, `IPhysicsScene`, `IPhysicsStepper`, `IPhysicsRuntimeLifecycle`, `IPhysicsBackend`, `IPhysicsTransformSource`, `IPhysicsTransformSink`, `PhysicsServices`, `PhysicsDependencies`
- [[11 API/EngineRuntime/Physics/physics_types.h|physics_types.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h`; объявленные типы: `PhysicsBodyId`, `PhysicsBodyHandle`, `CollisionShapeId`, `BackendShapeHandle`, `BackendBodyHandle`, `PhysicsBodyType`, `PhysicsActivityState`, `PhysicsEventState`, `PhysicsDirtyFlags`, `PhysicsBodyDesc`, `PhysicsBackendOptions`, `PhysicsOptions`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/physics|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
