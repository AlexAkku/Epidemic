---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "resources_production"]
source_path: "EngineFramework/GameplayWorldStateOwners/ResourcesProduction"
---

# EngineFramework / ResourcesProduction

Модуль: **ResourcesProduction**. Геймплейное производство.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/`.
**CMake targets:** `EpidemicGameFrameworkResourcesProduction`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ResourcesProduction/resources_production.h|resources_production.h]]: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`; объявленные типы: `ResourceTypeId`, `ResourceUnitId`, `ResourceStockpileId`, `ResourceNodeId`, `ProductionSiteId`, `ProductionRecipeId`, `ResourceTransactionId`, `ResourceSourceId`, `ResourceSinkId`, `ResourceReservationId`, `ProductionCapabilityId`, `ProductionPlanId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ResourcesProduction|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
