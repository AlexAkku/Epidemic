---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "navigation"]
source_path: "EngineRuntime/Navigation"
---

# EngineRuntime / Navigation

Модуль: **Navigation**. Пути, данные навигации и запросы.

**Реализация:** `EngineRuntime/Navigation/`.
**CMake targets:** `EpidemicRuntimeNavigation`
**Локальные test targets:** `EpidemicRuntimeNavigationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Navigation/navigation.h|navigation.h]]: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation.h`
- [[11 API/EngineRuntime/Navigation/navigation_runtime.h|navigation_runtime.h]]: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_runtime.h`; объявленные типы: `INavCostProvider`, `INavigationBackend`, `INavigationDataSource`, `INavigationObstacleSource`, `INavTileRegistry`, `INavigationRuntime`, `NavigationServices`, `NavigationDependencies`
- [[11 API/EngineRuntime/Navigation/navigation_types.h|navigation_types.h]]: `EngineRuntime/Navigation/include/Epidemic/Runtime/Navigation/navigation_types.h`; объявленные типы: `NavTileId`, `PathQueryId`, `PathQueryHandle`, `DynamicObstacleId`, `NavTileState`, `PathQueryState`, `PathRequest`, `PathResult`, `NavigationRevision`, `NavCostQuery`, `DynamicObstacle`, `NavigationOptions`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/navigation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
