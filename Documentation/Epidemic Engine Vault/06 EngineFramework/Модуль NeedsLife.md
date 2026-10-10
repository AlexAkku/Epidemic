---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "needs_life"]
source_path: "EngineFramework/GameplayWorldStateOwners/NeedsLife"
---

# EngineFramework / NeedsLife

Модуль: **NeedsLife**. Нужды и жизненные потребности.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/NeedsLife/`.
**CMake targets:** `EpidemicGameFrameworkNeedsLife`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/NeedsLife/needs_life.h|needs_life.h]]: `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h`; объявленные типы: `NeedTypeId`, `NeedProfileId`, `LifeRoutineId`, `LifePressureId`, `NeedDecayRuleId`, `LifeSimulationProfileId`, `IdHash`, `NeedStateKey`, `NeedStateKeyHash`, `NeedThreshold`, `NeedMaterializationPolicy`, `LifePressureState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/NeedsLife|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
