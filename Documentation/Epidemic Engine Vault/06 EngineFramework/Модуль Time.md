---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "time"]
source_path: "EngineFramework/BaseInfrastructure/Time"
---

# EngineFramework / Time

Модуль: **Time**. Runtime game delta и clock.

**Реализация:** `EngineFramework/BaseInfrastructure/Time/`.
**CMake targets:** `EpidemicGameplayTime`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Time/gameplay_time.h|gameplay_time.h]]: `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h`; объявленные типы: `CalendarDefinition`, `CalendarDate`, `RecurrenceKind`, `CatchUpPolicy`, `SchedulePersistence`, `CalendarPattern`, `RecurrenceRule`, `ClockDefinition`, `ClockState`, `ScheduleEntry`, `ScheduledTrigger`, `SchedulerBudget`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/time|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
