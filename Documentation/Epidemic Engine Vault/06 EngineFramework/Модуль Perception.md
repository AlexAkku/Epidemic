---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "perception"]
source_path: "EngineFramework/GameplayWorldStateOwners/Perception"
---

# EngineFramework / Perception

Модуль: **Perception**. Gameplay perception.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Perception/`.
**CMake targets:** `EpidemicGameFrameworkPerception`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Perception/perception.h|perception.h]]: `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`; объявленные типы: `SenseTypeId`, `PerceiverProfileId`, `SenseEvaluatorId`, `PerceptionStimulusId`, `PerceptionObservationId`, `AwarenessStateId`, `IdHash`, `RefHash`, `WorldPosition`, `WorldDirection`, `PerceptionMaterializationPolicy`, `SenseEvaluationModel`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Perception|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
