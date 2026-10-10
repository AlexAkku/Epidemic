---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "foundation"]
source_path: "EngineBase/Foundation"
---

# EngineBase / Foundation

Модуль: **Foundation**. Базовые типы, Result, errors и ID.

**Реализация:** `EngineBase/Foundation/`.
**CMake targets:** `EpidemicFoundation`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Foundation/error.h|error.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/error.h`; объявленные типы: `Error`
- [[11 API/EngineBase/Foundation/handle.h|handle.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/handle.h`; объявленные типы: `Handle`, `hash`
- [[11 API/EngineBase/Foundation/path.h|path.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/path.h`; объявленные типы: `Path`
- [[11 API/EngineBase/Foundation/result.h|result.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/result.h`; объявленные типы: `Result`, `Storage`
- [[11 API/EngineBase/Foundation/string_id.h|string_id.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/string_id.h`; объявленные типы: `BasicId`, `StringIdTag`, `NameIdTag`, `ModuleIdTag`, `ServiceIdTag`, `EventTypeIdTag`, `hash`
- [[11 API/EngineBase/Foundation/time.h|time.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/time.h`; объявленные типы: `FrameTime`, `FrameIndex`, `hash`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/foundation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
