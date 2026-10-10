---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "population_simulation_integration"]
source_path: "EngineFramework/IntegrationLayer/PopulationSimulationIntegration"
---

# EngineFramework / PopulationSimulationIntegration

Модуль: **PopulationSimulationIntegration**. Связь популяции с симуляцией.

**Реализация:** `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/`.
**CMake targets:** `EpidemicGameFrameworkPopulationSimulationIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration`. Admission: 14 public API, 28 mutation obligations, 0 lifecycle, 7 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/PopulationSimulationIntegration/population_simulation_adapters.h|population_simulation_adapters.h]]: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h`; объявленные типы: `PopulationEncounterPlanState`, `PopulationBackedEncounterSlot`, `PopulationBackedEncounterPlan`, `PopulationEncounterAdapterSnapshot`, `PopulationBackedSpawnResult`, `PopulationEncounterAdapter`, `RolesNeedsAdapter`, `PopulationLifecycleReconciliation`, `PopulationLifecycleAdapterSnapshot`, `PopulationLifecycleAdapter`, `CityMorningResult`, `CityLifeAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/PopulationSimulationIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
