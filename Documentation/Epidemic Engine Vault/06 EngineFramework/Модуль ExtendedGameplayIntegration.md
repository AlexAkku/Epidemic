---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "extended_gameplay_integration"]
source_path: "EngineFramework/IntegrationLayer/ExtendedGameplayIntegration"
---

# EngineFramework / ExtendedGameplayIntegration

Модуль: **ExtendedGameplayIntegration**. Расширенная композиция gameplay.

**Реализация:** `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/`.
**CMake targets:** `EpidemicGameFrameworkExtendedGameplayIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration`. Admission: 42 public API, 68 mutation obligations, 0 lifecycle, 17 stale-identity, 1 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h|extended_gameplay_adapters.h]]: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h`; объявленные типы: `EquipmentItemsAdapter`, `ProcessItemInputPayload`, `ProcessItemOutputPayload`, `ProcessItemReservationPayload`, `PreparedProcessItemOutputPayload`, `ItemProcessInputProvider`, `ItemProcessOutputHandler`, `DialogueShareKnowledgePayload`, `DialogueAssertKnowledgePayload`, `DialogueKnowledgeConsequenceHandler`, `TradeExecutionId`, `TradeExecutionIdHash`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ExtendedGameplayIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
