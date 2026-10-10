---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "queries"]
source_path: "EngineFramework/BaseInfrastructure/Queries"
---

# EngineFramework / Queries

Модуль: **Queries**. Read-model queries.

**Реализация:** `EngineFramework/BaseInfrastructure/Queries/`.
**CMake targets:** `EpidemicGameplayQueries`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Queries/gameplay_queries.h|gameplay_queries.h]]: `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h`; объявленные типы: `QueryConsistency`, `QueryCoverage`, `QueryAccuracy`, `QueryRequirements`, `QueryBudget`, `QueryContext`, `QuerySnapshotReadEpoch`, `IQuerySnapshotCoordinator`, `QueryMetadata`, `QueryResponse`, `QueryProviderSnapshot`, `QueryProviderCapabilities`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/queries|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
