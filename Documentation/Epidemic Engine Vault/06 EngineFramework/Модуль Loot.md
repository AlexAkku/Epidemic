---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "loot"]
source_path: "EngineFramework/GameplayWorldStateOwners/Loot"
---

# EngineFramework / Loot

Модуль: **Loot**. Drops и rewards.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Loot/`.
**CMake targets:** `EpidemicGameFrameworkLoot`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Loot/loot.h|loot.h]]: `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`; объявленные типы: `RewardTypeId`, `RewardDefinitionId`, `LootTableId`, `LootEntryId`, `LootQualityId`, `RewardExecutionId`, `IdHash`, `RegisteredRewardPayload`, `RewardDefinition`, `LootRollPolicy`, `LootEntry`, `LootTableDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Loot|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
