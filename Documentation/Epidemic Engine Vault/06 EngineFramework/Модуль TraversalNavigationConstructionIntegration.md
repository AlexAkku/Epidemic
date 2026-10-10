---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "traversal_navigation_construction_integration"]
source_path: "EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration"
---

# EngineFramework / TraversalNavigationConstructionIntegration

Модуль: **TraversalNavigationConstructionIntegration**. Связь перемещения, навигации и строительства.

**Реализация:** `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/`.
**CMake targets:** `EpidemicGameFrameworkTraversalNavigationConstructionIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration`. Admission: 8 public API, 0 mutation obligations, 0 lifecycle, 0 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h|traversal_navigation_construction_adapters.h]]: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/include/Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h`; объявленные типы: `TraversalNavigationCapabilityProvider`, `TraversalNavigationAdapter`, `ConstructionNavigationOperation`, `ConstructionNavigationLayerPayload`, `ConstructionNavigationAdapter`, `ConstructionTraversalAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/TraversalNavigationConstructionIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
