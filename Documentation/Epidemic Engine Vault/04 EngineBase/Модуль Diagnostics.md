---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "diagnostics"]
source_path: "EngineBase/Diagnostics"
---

# EngineBase / Diagnostics

Модуль: **Diagnostics**. Логирование, counters, profiling.

**Реализация:** `EngineBase/Diagnostics/`.
**CMake targets:** `EpidemicDiagnostics`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Diagnostics/console_logger.h|console_logger.h]]: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/console_logger.h`; объявленные типы: `ConsoleLogger`
- [[11 API/EngineBase/Diagnostics/counters.h|counters.h]]: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/counters.h`; объявленные типы: `CounterId`, `DiagnosticsCounters`
- [[11 API/EngineBase/Diagnostics/logger.h|logger.h]]: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/logger.h`; объявленные типы: `LogLevel`, `LogMessage`, `ILogger`
- [[11 API/EngineBase/Diagnostics/profiling.h|profiling.h]]: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h`; объявленные типы: `ProfileEvent`, `IProfileCollector`, `InMemoryProfileCollector`, `ProfileScope`
- [[11 API/EngineBase/Diagnostics/thread_context.h|thread_context.h]]: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/thread_context.h`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/diagnostics|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
