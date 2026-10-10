---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "narrative"]
source_path: "EngineFramework/GameplayWorldStateOwners/Narrative"
---

# EngineFramework / Narrative

Модуль: **Narrative**. Повествовательные структуры и состояния.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Narrative/`.
**CMake targets:** `EpidemicGameFrameworkNarrative`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Narrative/narrative.h|narrative.h]]: `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`; объявленные типы: `NarrativeThreadId`, `NarrativeArcId`, `NarrativeBeatId`, `NarrativeObjectiveId`, `NarrativeConditionId`, `NarrativeConsequenceId`, `NarrativeConsequenceExecutionId`, `JournalEntryId`, `ClueId`, `RumorId`, `StoryletId`, `NarrativeGateId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/narrative|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
