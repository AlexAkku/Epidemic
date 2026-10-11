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

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Core — microkernel EngineBase. Он определяет жизненный цикл приложения, контейнер сервисов, реестр модулей, frame phases, event bus, worker scheduler и возврат результатов на main thread. Core координирует работу, но не знает о Windows, input, graphics, world или gameplay.

#### Application

`Application` проходит состояния Constructed, Bootstrapped, Initialized, Running и ShutDown. `Bootstrap()` дает модулям возможность зарегистрировать сервисы. `Initialize()` проверяет зависимости и закрывает `ServiceContainer`. `Tick()` выполняет один кадр, `Run()` повторяет его до stop request, а `Shutdown()` завершает модули.

Frame handlers регистрируются до запуска и исполняются в фиксированных `FramePhase`. `FrameContext` содержит индекс и время кадра, но не хранит данные верхних systems.

#### Сервисы и модули

`ServiceContainer` хранит long-lived services по interface type. Duplicate registration считается ошибкой composition root. `ModuleRegistry` упорядочивает `IModule` по `ModuleManifest`, проверяет отсутствующие и циклические зависимости и вызывает lifecycle methods.

`IEventBus` поддерживает синхронную доставку и queued events. Он подходит для небольших notifications, но не заменяет authoritative command API. `ITaskScheduler` выполняет CPU tasks и позволяет ждать handle или group. `IMainThreadDispatcher` переносит commit обратно в основной поток.

#### Инварианты

Core не зависит от Platform, Input, RHI или EngineRuntime. Background tasks не меняют main-thread-only services напрямую. Service container не открывается повторно после Initialize. Event handlers и frame callbacks не должны хранить borrowed references дольше вызова.

#### Ограничения и стабильность

Core не является ECS, renderer loop или gameplay framework. Его lifecycle, service lookup и task contracts заморожены. Upper layer подключается через composition root и frame handler, не модифицируя Core.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `application.h`: `ILogger`, `ApplicationOptions`, `Application`.
- `basic_configuration.h`: `BasicConfiguration`.
- `configuration.h`: `IConfiguration`.
- `event_bus.h`: `EventDispatchMode`, `IEventBus`, `EventBus`.
- `frame_context.h`: `FrameContext`.
- `frame_phase.h`: `FramePhase`.
- `imodule.h`: `IModule`.
- `main_thread_dispatcher.h`: `IMainThreadDispatcher`, `MainThreadDispatcher`.
- `module_manifest.h`: `ModuleManifest`.
- `module_registry.h`: `ModuleRegistry`, `LifecycleState`.
- `service_container.h`: `ServiceContainer`.
- `task_scheduler.h`: `TaskDiagnostics`, `TaskHandle`, `SimpleTaskScheduler`, `TaskGroup`, `ITaskScheduler`.

Источник контрактов: `Documentation/EngineBase/modules/core.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `State` (`EngineBase/Core/include/Epidemic/Core/application.h`): `Constructed`, `Bootstrapped`, `Initialized`, `Running`, `Failed`, `ShutDown`.
- `EventDispatchMode` (`EngineBase/Core/include/Epidemic/Core/event_bus.h`): `Sync`, `Queued`.
- `LifecycleState` (`EngineBase/Core/include/Epidemic/Core/module_registry.h`): `Empty`, `Registered`, `Bootstrapped`, `Initialized`, `Failed`, `ShutDown`.
- `FramePhase` (`EngineBase/Core/include/Epidemic/Core/frame_phase.h`): `BeginFrame`, `PumpPlatformEvents`, `UpdateInput`, `DrainEvents`, `RunScheduledMainThreadTasks`, `TickModules`, `RhiBeginFrame`, `RhiEndFrame`, `Present`, `EndFrame`, `Count`.

### Операции и хранилища состояния

#### `IConfiguration`

Объявлен в `EngineBase/Core/include/Epidemic/Core/configuration.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IConfiguration() = default;
// Returns the canonical key for the runtime name. [[nodiscard]] static constexpr std::string_view RuntimeNameKey() noexcept;
// Returns the canonical key for worker-thread count. [[nodiscard]] static constexpr std::string_view WorkerCountKey() noexcept;
// Returns the canonical key for memory tracking enablement. [[nodiscard]] static constexpr std::string_view MemoryTrackingEnabledKey() noexcept;
// Returns the canonical key for RHI debug validation enablement. [[nodiscard]] static constexpr std::string_view RhiDebugEnabledKey() noexcept;
// Returns the canonical key for the default window width. [[nodiscard]] static constexpr std::string_view DefaultWindowWidthKey() noexcept;
// Returns the canonical key for the default window height. [[nodiscard]] static constexpr std::string_view DefaultWindowHeightKey() noexcept;
// Convenience accessor for the configured runtime name. [[nodiscard]] std::optional<std::string> GetRuntimeName() const;
// Convenience mutator for the runtime name. void SetRuntimeName(std::string runtime_name);
// Returns the configured worker count when present and non-negative. [[nodiscard]] std::optional<std::size_t> GetWorkerCount() const;
// Convenience mutator for worker count. void SetWorkerCount(std::size_t worker_count);
// Returns whether memory tracking is enabled when the key is present. [[nodiscard]] std::optional<bool> GetMemoryTrackingEnabled() const;
// Convenience mutator for memory tracking enablement. void SetMemoryTrackingEnabled(bool enabled);
// Returns whether RHI debug validation is enabled when the key is present. [[nodiscard]] std::optional<bool> GetRhiDebugEnabled() const;
// Convenience mutator for RHI debug validation enablement. void SetRhiDebugEnabled(bool enabled);
// Returns the configured default window width when present. [[nodiscard]] std::optional<std::int32_t> GetDefaultWindowWidth() const;
// Convenience mutator for default window width. void SetDefaultWindowWidth(std::int32_t width);
// Returns the configured default window height when present. [[nodiscard]] std::optional<std::int32_t> GetDefaultWindowHeight() const;
```

#### `ServiceContainer`

Объявлен в `EngineBase/Core/include/Epidemic/Core/service_container.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Constructs and registers a service implementation under the requested interface type. // Relationship: validation happens before construction so sealed or duplicate failures do not build the instance. template <typename TService, typename TImplementation, typename... TArgs> std::shared_ptr<TService> Emplace(TArgs &&...args);
// Registers an externally created service instance under the requested interface type. template <typename TService> void RegisterInstance(std::shared_ptr<TService> instance);
// Atomically registers a bundle of externally created service instances. // Relationship: a candidate map is built under the container lock and swapped into place only after // every insertion succeeds, so duplicate/sealed/allocation failures leave the live graph unchanged. template <typename... TServices> void RegisterInstancesAtomic(std::shared_ptr<TServices>... instances);
// Stages a service bundle, runs one external strong-guarantee pre-commit action, then publishes the // service map with a no-allocation swap. The callback must not re-enter this ServiceContainer. template <typename TPreCommit, typename... TServices> void RegisterInstancesAtomicWithPreCommit(TPreCommit &&pre_commit, std::shared_ptr<TServices>... instances);
// Returns the registered service instance for TService. // TODO: If optional lookup becomes common, consider a non-throwing TryGet instead of forcing exception control flow. template <typename TService> [[nodiscard]] std::shared_ptr<TService> Get() const;
// Returns whether a service for TService is currently registered. template <typename TService> [[nodiscard]] bool Contains() const;
// Prevents further service registration. void Seal() noexcept;
// Returns whether the container has been sealed. [[nodiscard]] bool IsSealed() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `mutable std::shared_mutex mutex_`; `std::unordered_map<std::type_index, std::shared_ptr<void>> services_`.

#### `IEventBus`

Объявлен в `EngineBase/Core/include/Epidemic/Core/event_bus.h:29`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEventBus() = default;
// Registers a synchronous handler for EventT. template <typename EventT, typename HandlerT> HandlerToken SubscribeSync(HandlerT &&handler);
// Registers a queued handler for EventT. template <typename EventT, typename HandlerT> HandlerToken SubscribeQueued(HandlerT &&handler);
// Immediately dispatches one event to synchronous subscribers of EventT. template <typename EventT> void PublishSync(const EventT &event);
// Queues one event for later delivery to queued subscribers of EventT. template <typename EventT> void Enqueue(const EventT &event);
```

#### `IMainThreadDispatcher`

Объявлен в `EngineBase/Core/include/Epidemic/Core/main_thread_dispatcher.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IMainThreadDispatcher() = default;
// Queues a task for later execution on the owning main thread. virtual void Post(std::function<void()> task, std::string debug_name =;
```

#### `IModule`

Объявлен в `EngineBase/Core/include/Epidemic/Core/imodule.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IModule() = default;
```

#### `ITaskScheduler`

Объявлен в `EngineBase/Core/include/Epidemic/Core/task_scheduler.h:73`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ITaskScheduler() = default;
```

#### `ModuleRegistry`

Объявлен в `EngineBase/Core/include/Epidemic/Core/module_registry.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::unique_ptr<IModule>> modules_`; `std::unordered_map<foundation::ModuleId, std::size_t> module_index_by_id_`; `std::unordered_map<foundation::ModuleId, std::string> canonical_module_name_by_id_`; `std::vector<std::size_t> execution_plan_`; `std::vector<std::uint8_t> shutdown_completed_`.

#### `TaskGroup`

Объявлен в `EngineBase/Core/include/Epidemic/Core/task_scheduler.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
TaskGroup(const TaskGroup &) = default;
TaskGroup(TaskGroup &&) noexcept = default;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `struct State`; `std::shared_ptr<State> state_`.

#### `FrameContext`

Объявлен в `EngineBase/Core/include/Epidemic/Core/frame_context.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
foundation::FrameTime delta_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
foundation::FrameTime absolute_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
foundation::FrameTime raw_delta_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
// Returns true for the very first executed frame. [[nodiscard]] bool IsFirstFrame() const noexcept;
```

#### `Application`

Объявлен в `EngineBase/Core/include/Epidemic/Core/application.h:38`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ApplicationOptions options_`; `ServiceContainer services_`; `ModuleRegistry modules_`.

#### `QueuedEvent`

Объявлен в `EngineBase/Core/include/Epidemic/Core/event_bus.h:112`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES std::uint64_t auditor_correlation;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::any payload`.

#### `TaskHandle`

Объявлен в `EngineBase/Core/include/Epidemic/Core/task_scheduler.h:29`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
TaskHandle() = default;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `struct State`; `std::shared_ptr<State> state_`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Core/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/Core/src/event_bus.cpp` (231 строк): `EventBus::SubscribeImpl`, `EventBus::PublishSyncImpl`, `EventBus::EnqueueImpl`, `EventBus::DrainQueued`, `EventBus::Unsubscribe`, `EventBus::DispatchSync`, `EventBus::DispatchQueued`
- `EngineBase/Core/src/module_registry.cpp` (416 строк): `ModuleId::FromString`, `ModuleRegistry::Register`, `ModuleRegistry::BootstrapAll`, `ModuleRegistry::InitializeAll`, `ModuleRegistry::TickAll`, `ModuleRegistry::ShutdownAll`, `ModuleRegistry::Size`, `ModuleRegistry::EnsureExecutionPlan`
- `EngineBase/Core/src/task_scheduler.cpp` (599 строк): `TaskHandle::IsValid`, `TaskHandle::TaskHandle`, `TaskGroup::TaskGroup`, `TaskGroup::IsValid`, `SimpleTaskScheduler::SimpleTaskScheduler`, `SimpleTaskScheduler::Schedule`, `SimpleTaskScheduler::Cancel`, `SimpleTaskScheduler::Wait`, `SimpleTaskScheduler::WaitIdle`, `SimpleTaskScheduler::RequestStop`
- `EngineBase/Core/src/main_thread_dispatcher.cpp` (93 строк): `MainThreadDispatcher::MainThreadDispatcher`, `MainThreadDispatcher::Post`, `MainThreadDispatcher::Drain`, `MainThreadDispatcher::IsMainThread`
- `EngineBase/Core/src/basic_configuration.cpp` (70 строк): `BasicConfiguration::GetString`, `BasicConfiguration::SetString`, `BasicConfiguration::GetInt`, `BasicConfiguration::SetInt`, `BasicConfiguration::GetBool`, `BasicConfiguration::SetBool`
- `EngineBase/Core/src/application.cpp` (613 строк): `Application::Application`, `Application::Modules`, `Application::Services`, `Application::Bootstrap`, `Application::Initialize`, `Application::Tick`, `Application::Run`, `Application::Shutdown`, `Application::RequestStop`, `Application::ResetStopRequest`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `event.publish`: `EngineBase/Core/src/event_bus.cpp:174`
- `event.handle`: `EngineBase/Core/src/event_bus.cpp:176`
- `event.handler.submit`: `EngineBase/Core/src/event_bus.cpp:199`
- `event.handler.execute`: `EngineBase/Core/src/event_bus.cpp:201`
- `job.submit`: `EngineBase/Core/src/task_scheduler.cpp:448`
- `job.execute`: `EngineBase/Core/src/task_scheduler.cpp:505`
- `job.cancel`: `EngineBase/Core/src/task_scheduler.cpp:505`
- `async.submit`: `EngineBase/Core/src/main_thread_dispatcher.cpp:36`
- `async.execute`: `EngineBase/Core/src/main_thread_dispatcher.cpp:62`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `event.publish` (`EngineBase/Core/src/event_bus.cpp:174`)

```cpp
172     auto event_context = auditor::shadow::child_context();
  173     const auto event_id = auditor::shadow::identity();
  174     event_context.cause = auditor::shadow::emit_causal("event.publish", event_id.data());
  175     const auditor::shadow::ContextScope event_scope(event_context);
  176     auditor::shadow::flow("event.handle", event_id.data());
```

#### `event.handle` (`EngineBase/Core/src/event_bus.cpp:176`)

```cpp
174     event_context.cause = auditor::shadow::emit_causal("event.publish", event_id.data());
  175     const auditor::shadow::ContextScope event_scope(event_context);
  176     auditor::shadow::flow("event.handle", event_id.data());
  177 #endif
  178     std::vector<AnyEventHandler> handlers;
```

#### `event.handler.submit` (`EngineBase/Core/src/event_bus.cpp:199`)

```cpp
197         auto handler_context = auditor::shadow::child_context();
  198         const auto handler_id = auditor::shadow::identity();
  199         handler_context.cause = auditor::shadow::emit_causal("event.handler.submit", handler_id.data());
  200         const auditor::shadow::ContextScope handler_scope(handler_context);
  201         auditor::shadow::flow("event.handler.execute", handler_id.data());
```

#### `event.handler.execute` (`EngineBase/Core/src/event_bus.cpp:201`)

```cpp
199         handler_context.cause = auditor::shadow::emit_causal("event.handler.submit", handler_id.data());
  200         const auditor::shadow::ContextScope handler_scope(handler_context);
  201         auditor::shadow::flow("event.handler.execute", handler_id.data());
  202 #endif
  203         handler(event);
```

#### `job.submit` (`EngineBase/Core/src/task_scheduler.cpp:448`)

```cpp
446         state_->tasks.push(QueuedTask{std::move(task), handle_state, group_state});
  447 #if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
  448         handle_state->auditor_context.cause = auditor::shadow::emit_causal("job.submit", handle_state->auditor_job.data());
  449 #endif
  450         if (group_state)
```

#### `job.execute` (`EngineBase/Core/src/task_scheduler.cpp:505`)

```cpp
503 #if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
  504         const auditor::shadow::ContextScope auditor_scope(queued_task.handle_state->auditor_context);
  505         auditor::shadow::flow(queued_task.task ? "job.execute" : "job.cancel", queued_task.handle_state->auditor_job.data());
  506 #endif
  507         std::exception_ptr task_exception;
```

#### `job.cancel` (`EngineBase/Core/src/task_scheduler.cpp:505`)

```cpp
503 #if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
  504         const auditor::shadow::ContextScope auditor_scope(queued_task.handle_state->auditor_context);
  505         auditor::shadow::flow(queued_task.task ? "job.execute" : "job.cancel", queued_task.handle_state->auditor_job.data());
  506 #endif
  507         std::exception_ptr task_exception;
```

#### `async.submit` (`EngineBase/Core/src/main_thread_dispatcher.cpp:36`)

```cpp
34 #if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
   35     auto& accepted = tasks_.back();
   36     accepted.auditor_context.cause = auditor::shadow::emit_causal("async.submit", accepted.auditor_correlation.data());
   37 #endif
   38 }
```

#### `async.execute` (`EngineBase/Core/src/main_thread_dispatcher.cpp:62`)

```cpp
60 #if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
   61         const auditor::shadow::ContextScope auditor_scope(task.auditor_context);
   62         auditor::shadow::flow("async.execute", task.auditor_correlation.data());
   63 #endif
   64         try
```
