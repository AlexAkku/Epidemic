---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "core"]
source_path: "EngineBase/Core"
---

# EngineBase / Core

Модуль: **Core**. Application, services, frame lifecycle, scheduler, events.

**Реализация:** `EngineBase/Core/`.
**CMake targets:** `EpidemicCore`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Core/application.h|application.h]]: `EngineBase/Core/include/Epidemic/Core/application.h`; объявленные типы: `ILogger`, `ApplicationOptions`, `Application`, `FramePhaseHandlerRegistration`, `PhaseHandler`, `State`
- [[11 API/EngineBase/Core/basic_configuration.h|basic_configuration.h]]: `EngineBase/Core/include/Epidemic/Core/basic_configuration.h`; объявленные типы: `BasicConfiguration`
- [[11 API/EngineBase/Core/configuration.h|configuration.h]]: `EngineBase/Core/include/Epidemic/Core/configuration.h`; объявленные типы: `IConfiguration`
- [[11 API/EngineBase/Core/event_bus.h|event_bus.h]]: `EngineBase/Core/include/Epidemic/Core/event_bus.h`; объявленные типы: `EventDispatchMode`, `IEventBus`, `EventBus`, `Subscription`, `QueuedEvent`
- [[11 API/EngineBase/Core/frame_context.h|frame_context.h]]: `EngineBase/Core/include/Epidemic/Core/frame_context.h`; объявленные типы: `FrameContext`
- [[11 API/EngineBase/Core/frame_phase.h|frame_phase.h]]: `EngineBase/Core/include/Epidemic/Core/frame_phase.h`; объявленные типы: `FramePhase`
- [[11 API/EngineBase/Core/imodule.h|imodule.h]]: `EngineBase/Core/include/Epidemic/Core/imodule.h`; объявленные типы: `IModule`
- [[11 API/EngineBase/Core/main_thread_dispatcher.h|main_thread_dispatcher.h]]: `EngineBase/Core/include/Epidemic/Core/main_thread_dispatcher.h`; объявленные типы: `IMainThreadDispatcher`, `MainThreadDispatcher`, `PendingTask`
- [[11 API/EngineBase/Core/module_manifest.h|module_manifest.h]]: `EngineBase/Core/include/Epidemic/Core/module_manifest.h`; объявленные типы: `ModuleManifest`
- [[11 API/EngineBase/Core/module_registry.h|module_registry.h]]: `EngineBase/Core/include/Epidemic/Core/module_registry.h`; объявленные типы: `ModuleRegistry`, `LifecycleState`
- [[11 API/EngineBase/Core/service_container.h|service_container.h]]: `EngineBase/Core/include/Epidemic/Core/service_container.h`; объявленные типы: `ServiceContainer`
- [[11 API/EngineBase/Core/task_scheduler.h|task_scheduler.h]]: `EngineBase/Core/include/Epidemic/Core/task_scheduler.h`; объявленные типы: `TaskDiagnostics`, `TaskHandle`, `State`, `TaskGroup`, `ITaskScheduler`, `SimpleTaskScheduler`, `SharedState`, `QueuedTask`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/core|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
