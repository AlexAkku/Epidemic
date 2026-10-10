---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "process_resource_simulation_integration"]
source_path: "EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration"
---

# EngineFramework / ProcessResourceSimulationIntegration

Модуль: **ProcessResourceSimulationIntegration**. Связь процессов и ресурсов с симуляцией.

**Реализация:** `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/`.
**CMake targets:** `EpidemicGameFrameworkProcessResourceSimulationIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration`. Admission: 22 public API, 32 mutation obligations, 0 lifecycle, 8 stale-identity, 1 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h|process_resource_simulation_adapters.h]]: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/include/Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h`; объявленные типы: `ProcessResourcePayload`, `ProcessResourceReservationPayload`, `PreparedProcessResourceOutputPayload`, `ResourceProcessInputProvider`, `ResourceProcessOutputHandler`, `ProcessesSimulationLayer`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ProcessResourceSimulationIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
