---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "platform"]
source_path: "EngineBase/Platform"
---

# EngineBase / Platform

Модуль: **Platform**. Win32 runtime, окна и события.

**Реализация:** `EngineBase/Platform/`.
**CMake targets:** `EpidemicPlatform`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Platform/idynamic_library.h|idynamic_library.h]]: `EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h`; объявленные типы: `IDynamicLibrary`
- [[11 API/EngineBase/Platform/iplatform_runtime.h|iplatform_runtime.h]]: `EngineBase/Platform/include/Epidemic/Platform/iplatform_runtime.h`; объявленные типы: `ProcessInfo`, `IPlatformRuntime`
- [[11 API/EngineBase/Platform/iwindow.h|iwindow.h]]: `EngineBase/Platform/include/Epidemic/Platform/iwindow.h`; объявленные типы: `IWindow`
- [[11 API/EngineBase/Platform/iwindow_system.h|iwindow_system.h]]: `EngineBase/Platform/include/Epidemic/Platform/iwindow_system.h`; объявленные типы: `WindowCreateInfo`, `IWindowSystem`
- [[11 API/EngineBase/Platform/native_window_handle.h|native_window_handle.h]]: `EngineBase/Platform/include/Epidemic/Platform/native_window_handle.h`; объявленные типы: `NativeWindowHandle`
- [[11 API/EngineBase/Platform/platform_event.h|platform_event.h]]: `EngineBase/Platform/include/Epidemic/Platform/platform_event.h`; объявленные типы: `PlatformEventType`, `PlatformEvent`
- [[11 API/EngineBase/Platform/windows_platform_runtime.h|windows_platform_runtime.h]]: `EngineBase/Platform/include/Epidemic/Platform/windows_platform_runtime.h`; объявленные типы: `WindowsPlatformRuntime`, `Impl`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/platform|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
