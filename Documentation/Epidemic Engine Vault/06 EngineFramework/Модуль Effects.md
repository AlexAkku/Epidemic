---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "effects"]
source_path: "EngineFramework/GameplayWorldStateOwners/Effects"
---

# EngineFramework / Effects

Модуль: **Effects**. Эффекты игровых действий.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Effects/`.
**CMake targets:** `EpidemicGameFrameworkEffects`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Effects/effects.h|effects.h]]: `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`; объявленные типы: `EffectTypeId`, `EffectDefinitionId`, `EffectExecutionId`, `DeferredEffectId`, `EffectTypeIdHash`, `EffectDefinitionIdHash`, `DeferredEffectIdHash`, `RegisteredEffectPayload`, `EffectTargetSelector`, `EffectStepDefinition`, `EffectExecutionPolicy`, `EffectDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Effects|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
