---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "simulation"]
source_path: "EngineRuntime/Simulation"
---

# EngineRuntime / Simulation

Модуль: **Simulation**. Budgeted proposals и commits.

**Реализация:** `EngineRuntime/Simulation/`.
**CMake targets:** `EpidemicRuntimeSimulation`
**Локальные test targets:** `EpidemicRuntimeSimulationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Simulation/simulation.h|simulation.h]]: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation.h`
- [[11 API/EngineRuntime/Simulation/simulation_runtime.h|simulation_runtime.h]]: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_runtime.h`; объявленные типы: `ISimulationJob`, `ISimulationScheduler`, `ISimulationRuntime`, `IAttentionSystem`, `IWorldMemory`, `IRelevancePolicy`, `ISimulationCommitTarget`, `IAbstractFactStore`, `ISimulationProposalQueue`, `IScheduledSimulationTasks`, `ISimulationClock`, `SimulationDependencies`…
- [[11 API/EngineRuntime/Simulation/simulation_types.h|simulation_types.h]]: `EngineRuntime/Simulation/include/Epidemic/Runtime/Simulation/simulation_types.h`; объявленные типы: `SimulationJobId`, `WorldMemoryEventId`, `ScheduledSimulationTaskId`, `SimulationJobHandle`, `SimulationLane`, `SimulationZoneState`, `SimulationJobState`, `MemoryLifetime`, `ObservationState`, `AttentionScore`, `SimulationBudget`, `SimulationJobDesc`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/simulation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
