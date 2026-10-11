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

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Diagnostics делает работу EngineBase наблюдаемой через structured logging, числовые counters, scope profiling и диагностические имена потоков. Diagnostics не владеет authoritative engine/game state и не используется как event bus, control channel или условие успешности engine mutation.

#### Logging

`ILogger` является безопасной публичной границей. Все `Log`/`Trace`/`Debug`/`Info`/`Warn`/`Error`/`Fatal` вызовы являются `noexcept`: исключение конкретного sink перехватывается внутри `ILogger` и не может заменить исходную ошибку либо изменить control flow движка.

Конкретный sink реализует private virtual `Write`. `ConsoleLogger` пишет в stdout и best-effort зеркалирует строку в `logs/epidemic.log`. Через `SetLoggingEnabled(false)` dispatch можно отключить полностью; пропущенные записи не replay-ятся после повторного включения.

`LogMessage` заимствует module/category/message только на время синхронного вызова. `thread_name` хранится как owning `std::string`.

#### Counters

`DiagnosticsCounters` хранит фиксированный массив атомарных `int64_t`. `Increment` и `Decrement` используют saturating arithmetic и не wrap-around при достижении `INT64_MIN/MAX`. `CounterId::Count` и поврежденные enum values являются invalid: mutation для них является no-op, `Get` возвращает `0`.

Counters предназначены только для наблюдения. Они не являются revision, generation, ownership token или механизмом синхронизации.

#### Profiling

`SetProfileCollector` задает process-wide collector для новых scopes. `SetProfilingEnabled(false)` отключает создание profiling events; disabled path возвращается до копирования имени scope и не делает диагностических allocations.

`ProfileScope` копирует имя scope и захватывает `shared_ptr` на текущий collector при создании. Замена global collector во время уже активного scope не перенаправляет его событие. Деструктор `ProfileScope` всегда `noexcept`, работает на normal return и exception unwinding и подавляет исключения collector, сохраняя исходную ошибку.

`InMemoryProfileCollector::Snapshot()` возвращает detached copy диагностических samples. Это не persistence snapshot и restore API у Diagnostics отсутствует.

Пример:

```cpp
epidemic::diagnostics::SetProfileCollector(collector);
epidemic::diagnostics::SetProfilingEnabled(true);
{
    epidemic::diagnostics::ProfileScope scope("Resources.ProcessPendingLoads");
    ProcessPendingLoads();
}
```

#### Thread context

`SetCurrentThreadName` меняет только `thread_local` имя текущего потока. `GetCurrentThreadName` возвращает owning `std::string`, поэтому полученное значение не становится dangling после последующего rename или завершения исходного потока.

#### Threading

Counters и enable flags атомарны. Global collector защищен mutex. `InMemoryProfileCollector` и `ConsoleLogger` сериализуют доступ к внутреннему состоянию. Thread names являются `thread_local`.

Полная cross-module concurrency qualification выполняется в Goal 6; локальный Diagnostics audit фиксирует только собственные synchronization contracts.

#### Карта публичных заголовков

- `console_logger.h`: `ConsoleLogger`.
- `counters.h`: `CounterId`, `DiagnosticsCounters`, counter helpers.
- `logger.h`: `LogLevel`, `LogMessage`, `ILogger`, logging enable state.
- `profiling.h`: `ProfileEvent`, `IProfileCollector`, `InMemoryProfileCollector`, `ProfileScope`.
- `thread_context.h`: current-thread diagnostic name helpers.

Источник контрактов: `Documentation/EngineBase/modules/diagnostics.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `CounterId` (`EngineBase/Diagnostics/include/Epidemic/Diagnostics/counters.h`): `Frames`, `TasksScheduled`, `TasksCompleted`, `WorkerCount`, `CurrentFrameIndex`, `FrameTimeMicros`, `PlatformPumpTimeMicros`, `InputUpdateTimeMicros`, `ModuleTickTimeMicros`, `PresentTimeMicros`, `MainThreadTasksExecuted`, `EventBusQueuedEvents`, `EventBusDispatchedEvents`, `PlatformEventsThisFrame` и другие значения.
- `LogLevel` (`EngineBase/Diagnostics/include/Epidemic/Diagnostics/logger.h`): `Trace`, `Debug`, `Info`, `Warning`, `Error`, `Fatal`.

### Операции и хранилища состояния

#### `ILogger`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/logger.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ILogger() = default;
// Writes a fully prepared log message if logging is enabled. Sink failures are contained. void Log(const LogMessage &message) noexcept;
// Builds a structured message from individual fields and forwards it to the contained sink boundary. void Log(LogLevel level, std::string_view module_name, std::string_view category, std::string_view message) noexcept;
// Convenience overload for uncategorized log entries. void Log(LogLevel level, std::string_view module_name, std::string_view message) noexcept;
void Trace(std::string_view module_name, std::string_view category, std::string_view message) noexcept;
void Trace(std::string_view module_name, std::string_view message) noexcept;
void Debug(std::string_view module_name, std::string_view category, std::string_view message) noexcept;
void Debug(std::string_view module_name, std::string_view message) noexcept;
void Info(std::string_view module_name, std::string_view category, std::string_view message) noexcept;
void Info(std::string_view module_name, std::string_view message) noexcept;
void Warn(std::string_view module_name, std::string_view category, std::string_view message) noexcept;
void Warn(std::string_view module_name, std::string_view message) noexcept;
void Error(std::string_view module_name, std::string_view category, std::string_view message) noexcept;
void Error(std::string_view module_name, std::string_view message) noexcept;
void Fatal(std::string_view module_name, std::string_view category, std::string_view message) noexcept;
void Fatal(std::string_view module_name, std::string_view message) noexcept;
```

#### `InMemoryProfileCollector`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
void Record(ProfileEvent event) override;
[[nodiscard]] std::vector<ProfileEvent> Snapshot() const;
void Reset();
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `mutable std::mutex mutex_`; `std::vector<ProfileEvent> events_`.

#### `IProfileCollector`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IProfileCollector() = default;
virtual void Record(ProfileEvent event) = 0;
```

#### `ProfileScope`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h:52`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ProfileScope(std::string_view scope_name);
~ProfileScope() noexcept;
ProfileScope(const ProfileScope &) = delete;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string name_`; `std::shared_ptr<IProfileCollector> collector_`.

#### `ProfileEvent`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string name`; `std::string thread_name`.

#### `ConsoleLogger`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/console_logger.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::mutex mutex_`; `std::ofstream file_`.

#### `LogMessage`

Объявлен в `EngineBase/Diagnostics/include/Epidemic/Diagnostics/logger.h:50`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string_view module_name`; `std::string_view category`; `std::string_view message`; `std::string thread_name`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Diagnostics/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/Diagnostics/src/profiling.cpp` (96 строк): `InMemoryProfileCollector::Record`, `InMemoryProfileCollector::Snapshot`, `InMemoryProfileCollector::Reset`, `ProfileScope::ProfileScope`
- `EngineBase/Diagnostics/src/thread_context.cpp` (31 строк)
- `EngineBase/Diagnostics/src/logger.cpp` (21 строк)
- `EngineBase/Diagnostics/src/counters.cpp` (104 строк): `DiagnosticsCounters::Increment`, `DiagnosticsCounters::Decrement`, `DiagnosticsCounters::Set`, `DiagnosticsCounters::Get`, `DiagnosticsCounters::Reset`
- `EngineBase/Diagnostics/src/console_logger.cpp` (123 строк): `ConsoleLogger::EnsureLogFileInitialized`, `ConsoleLogger::Write`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `epidemic.log`: `EngineBase/Diagnostics/src/console_logger.cpp:70`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `epidemic.log` (`EngineBase/Diagnostics/src/console_logger.cpp:70`)

```cpp
68     }
   69 
   70     file_.open(logs_directory / "epidemic.log", std::ios::out | std::ios::app);
   71 }
   72
```
