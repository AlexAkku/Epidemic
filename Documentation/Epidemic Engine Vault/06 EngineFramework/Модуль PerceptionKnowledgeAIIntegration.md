---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "perception_knowledge_a_i_integration"]
source_path: "EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration"
---

# EngineFramework / PerceptionKnowledgeAIIntegration

Модуль: **PerceptionKnowledgeAIIntegration**. Связь восприятия, знаний и AI.

**Реализация:** `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/`.
**CMake targets:** `EpidemicGameFrameworkPerceptionKnowledgeAIIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration`. Admission: 14 public API, 12 mutation obligations, 0 lifecycle, 2 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h|perception_knowledge_ai_adapters.h]]: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h`; объявленные типы: `PerceptionKnowledgeMapping`, `ObservationKnowledgeResult`, `PerceptionKnowledgeAdapter`, `AIExecutionAvailability`, `KnowledgeAIInputKeys`, `KnowledgeAIAdapter`, `AIIntentExecutionRecorder`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/PerceptionKnowledgeAIIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
