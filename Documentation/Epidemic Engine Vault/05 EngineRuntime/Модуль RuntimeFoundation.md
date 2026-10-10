---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "runtime_foundation"]
source_path: "EngineRuntime/RuntimeFoundation"
---

# EngineRuntime / RuntimeFoundation

Модуль: **RuntimeFoundation**. Общие Runtime IDs, budgets и spatial contracts.

**Реализация:** `EngineRuntime/RuntimeFoundation/`.
**CMake targets:** `EpidemicRuntimeFoundation`
**Локальные test targets:** `EpidemicRuntimeFoundationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/RuntimeFoundation/checked_id_allocator.h|checked_id_allocator.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/checked_id_allocator.h`; объявленные типы: `MonotonicIdReservation`
- [[11 API/EngineRuntime/RuntimeFoundation/numeric_validation.h|numeric_validation.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/numeric_validation.h`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_budget.h|runtime_budget.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_budget.h`; объявленные типы: `RuntimeBudget`, `RuntimeBudgetConsumption`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_foundation.h|runtime_foundation.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_foundation.h`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_handles.h|runtime_handles.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_handles.h`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_ids.h|runtime_ids.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_ids.h`; объявленные типы: `AssetIdTag`, `ResourceIdTag`, `RuntimeObjectIdTag`, `PersistentObjectIdTag`, `LazyRuleIdTag`, `RegionIdTag`, `ChunkIdTag`, `SurfaceIdTag`, `SimulationZoneIdTag`, `NumericRuntimeId`, `AssetId`, `ResourceId`…
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_operation.h|runtime_operation.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_operation.h`; объявленные типы: `AsyncOperationStatus`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_states.h|runtime_states.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`; объявленные типы: `ResidencyState`, `ObjectRealityLevel`, `PersistenceTier`, `SimulationLod`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_time.h|runtime_time.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_time.h`; объявленные типы: `RuntimeFrameDuration`, `GameDuration`, `GameTimePoint`
- [[11 API/EngineRuntime/RuntimeFoundation/spatial.h|spatial.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h`; объявленные типы: `Vec3`, `Quat`, `Transform`, `Aabb`, `Sphere`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/runtime_foundation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
