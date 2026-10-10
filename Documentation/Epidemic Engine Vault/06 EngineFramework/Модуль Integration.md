---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "integration"]
source_path: "EngineFramework/IntegrationLayer/Integration"
---

# EngineFramework / Integration

Модуль: **Integration**. Core gameplay adapters.

**Реализация:** `EngineFramework/IntegrationLayer/Integration/`.
**CMake targets:** `EpidemicGameFrameworkCoreIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/Integration`. Admission: 34 public API, 68 mutation obligations, 1 lifecycle, 14 stale-identity, 2 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Integration/core_adapters.h|core_adapters.h]]: `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h`; объявленные типы: `FileSavePackageStore`, `FindFactsQuery`, `FindHistoryQuery`, `FactsQueryAdapter`, `RuntimeTimeAdapter`, `ScheduledTriggerDisposition`, `ScheduledTriggerActionDeliveryMode`, `ScheduledTriggerDispatcherPolicy`, `ScheduledTriggerDeliveryRecord`, `ScheduledTriggerDispatcherSnapshot`, `ScheduledTriggerPumpReport`, `ScheduledTriggerDispatcherSaveParticipant`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Integration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
