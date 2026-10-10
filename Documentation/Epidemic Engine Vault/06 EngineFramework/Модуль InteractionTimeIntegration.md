---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "interaction_time_integration"]
source_path: "EngineFramework/IntegrationLayer/InteractionTimeIntegration"
---

# EngineFramework / InteractionTimeIntegration

Модуль: **InteractionTimeIntegration**. Связь взаимодействий и времени.

**Реализация:** `EngineFramework/IntegrationLayer/InteractionTimeIntegration/`.
**CMake targets:** `EpidemicGameFrameworkInteractionTimeIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/InteractionTimeIntegration`. Admission: 8 public API, 16 mutation obligations, 0 lifecycle, 4 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/InteractionTimeIntegration/interaction_time_adapter.h|interaction_time_adapter.h]]: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h`; объявленные типы: `InteractionTimeReconciliationKind`, `InteractionTimeReconciliationRecord`, `InteractionTimeAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/InteractionTimeIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
