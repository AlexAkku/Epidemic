---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "time"]
source_path: "EngineRuntime/Time"
---

# EngineRuntime / Time

Модуль: **Time**. Runtime game delta и clock.

**Реализация:** `EngineRuntime/Time/`.
**CMake targets:** `EpidemicRuntimeTime`
**Локальные test targets:** `EpidemicRuntimeTimeTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Time/game_calendar.h|game_calendar.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_calendar.h`; объявленные типы: `CalendarDefinition`, `CalendarDate`
- [[11 API/EngineRuntime/Time/game_time.h|game_time.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_time.h`
- [[11 API/EngineRuntime/Time/time_events.h|time_events.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_events.h`; объявленные типы: `TimeEventKind`, `TimeEvent`
- [[11 API/EngineRuntime/Time/time_runtime.h|time_runtime.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h`; объявленные типы: `TimeAdvanceResult`, `PhaseBoundary`, `TimeOptions`, `TimeCheckpoint`, `IGameClock`, `ITimeRuntime`, `TimeServices`
- [[11 API/EngineRuntime/Time/time_scale.h|time_scale.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_scale.h`; объявленные типы: `TimeScale`
- [[11 API/EngineRuntime/Time/time_snapshot.h|time_snapshot.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_snapshot.h`; объявленные типы: `TimeSnapshot`
- [[11 API/EngineRuntime/Time/time_state.h|time_state.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_state.h`; объявленные типы: `DayPhase`, `TimeRuntimeState`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/time|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
