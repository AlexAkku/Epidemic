---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "environment"]
source_path: "EngineRuntime/Environment"
---

# EngineRuntime / Environment

Модуль: **Environment**. Нейтральное состояние окружения.

**Реализация:** `EngineRuntime/Environment/`.
**CMake targets:** `EpidemicRuntimeEnvironment`
**Локальные test targets:** `EpidemicRuntimeEnvironmentTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Environment/climate_profile.h|climate_profile.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/climate_profile.h`; объявленные типы: `ClimateProfile`
- [[11 API/EngineRuntime/Environment/environment_projection.h|environment_projection.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_projection.h`; объявленные типы: `EnvironmentProjection`
- [[11 API/EngineRuntime/Environment/environment_runtime.h|environment_runtime.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h`; объявленные типы: `IEnvironmentQuery`, `IEnvironmentWriter`, `IEnvironmentUpdatePolicy`, `IEnvironmentRuntime`
- [[11 API/EngineRuntime/Environment/environment_services.h|environment_services.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_services.h`; объявленные типы: `EnvironmentOptions`, `EnvironmentServices`
- [[11 API/EngineRuntime/Environment/environment_snapshot.h|environment_snapshot.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_snapshot.h`; объявленные типы: `EnvironmentSnapshot`
- [[11 API/EngineRuntime/Environment/environment_update.h|environment_update.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_update.h`; объявленные типы: `EnvironmentUpdateInput`, `EnvironmentStateUpdate`
- [[11 API/EngineRuntime/Environment/season_state.h|season_state.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/season_state.h`; объявленные типы: `SeasonKind`, `SeasonState`
- [[11 API/EngineRuntime/Environment/surface_state.h|surface_state.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/surface_state.h`; объявленные типы: `SurfaceConditionKind`, `SurfaceState`
- [[11 API/EngineRuntime/Environment/weather_state.h|weather_state.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/weather_state.h`; объявленные типы: `WeatherKind`, `WeatherState`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/environment|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
