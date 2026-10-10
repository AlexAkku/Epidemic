---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "support"]
source_path: "EngineBase/Support"
---

# EngineBase / Support

Модуль: **Support**. Композиция сервисов и adapters.

**Реализация:** `EngineBase/Support/`.
**CMake targets:** `EpidemicEngineBaseSupport`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Support/engine_base_support.h|engine_base_support.h]]: `EngineBase/Support/include/Epidemic/EngineBase/engine_base_support.h`; объявленные типы: `ILogger`, `EngineBaseOptions`, `GraphicsRuntimeOptions`, `GraphicsRuntimeServices`, `FramePlatformEvents`
- [[11 API/EngineBase/Support/graphics_backend.h|graphics_backend.h]]: `EngineBase/Support/include/Epidemic/EngineBase/graphics_backend.h`; объявленные типы: `GraphicsBackend`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/support|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
