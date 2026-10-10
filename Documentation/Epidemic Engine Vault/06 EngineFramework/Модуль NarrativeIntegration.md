---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "narrative_integration"]
source_path: "EngineFramework/IntegrationLayer/NarrativeIntegration"
---

# EngineFramework / NarrativeIntegration

Модуль: **NarrativeIntegration**. Связь narrative services.

**Реализация:** `EngineFramework/IntegrationLayer/NarrativeIntegration/`.
**CMake targets:** `EpidemicGameFrameworkNarrativeIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local integration contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/NarrativeIntegration/narrative_adapters.h|narrative_adapters.h]]: `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h`; объявленные типы: `NarrativeEventContract`, `NarrativeIntegrationContractRegistry`, `NarrativeSemanticEventAdapter`, `ExternalConsequenceDeliveryState`, `NarrativeExternalConsequenceDelivery`, `NarrativeExternalConsequenceSnapshot`, `NarrativeExternalConsequenceOutbox`, `NarrativeExternalConsequenceSaveParticipant`, `KnowledgeNarrativeReference`, `KnowledgeNarrativeAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/narrative_integration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
