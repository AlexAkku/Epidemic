---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h"
---

# Streaming: streaming_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Streaming|Streaming]].
**Размер исходного файла:** 3057 байт.
**Подключение:** `#include <Epidemic/Runtime/Streaming/streaming_runtime.h>`.

## Объявления типов (лексический индекс)
`IStreamingRuntime`, `IStreamingQuery`, `IStreamingController`, `StreamingServices`, `StreamingDependencies`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Streaming/residency_controller.h"
#include "Epidemic/Runtime/Streaming/streaming_sources.h"
#include "Epidemic/Runtime/Streaming/streaming_types.h"

#include <memory>
#include <optional>

namespace epidemic::runtime::streaming
{
// Public Streaming runtime contract. The implementation coordinates chunk residency
// requests and budgeted state transitions without owning world or renderer data.
class IStreamingRuntime
{
  public:
    virtual ~IStreamingRuntime() = default;

    [[nodiscard]] virtual foundation::Result<StreamingDemandHandle> Request(
        const StreamingTarget& target,
        StreamingPriorityClass priority) = 0;
    [[nodiscard]] virtual foundation::Result<void> ReleaseDemand(StreamingDemandHandle demand) = 0;
    [[nodiscard]] virtual StreamingState GetChunkState(ChunkId chunk) const = 0;
    virtual void SetBudget(const StreamingBudget& budget) = 0;
    // Runtime-owned allocation failure during Tick preparation/staging is not converted
    // into a semantic request failure and may surface as std::bad_alloc. Preparation faults
    // happen before request mutation; an already accepted external step is retained in
    // private retry state so retry cannot repeat that external side effect.
    [[nodiscard]] virtual StreamingTickResult Tick() = 0;
    [[nodiscard]] virtual std::optional<StreamingProgress> GetProgress(StreamingRequestHandle request) const = 0;
    [[nodiscard]] virtual StreamingStatistics GetStatistics() const = 0;
};

class IStreamingQuery
{
  public:
    virtual ~IStreamingQuery() = default;

    [[nodiscard]] virtual StreamingState GetChunkState(ChunkId chunk) const = 0;
    [[nodiscard]] virtual std::optional<StreamingProgress> GetProgress(StreamingRequestHandle request) const = 0;
    [[nodiscard]] virtual StreamingStatistics GetStatistics() const = 0;
};

class IStreamingController
{
  public:
    virtual ~IStreamingController() = default;

    [[nodiscard]] virtual foundation::Result<void> CancelRequest(StreamingRequestHandle request) = 0;
    [[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
};

struct StreamingServices
{
    std::shared_ptr<IStreamingRuntime> runtime;
    std::shared_ptr<IStreamingQuery> query;
    std::shared_ptr<IStreamingController> controller;
};

struct StreamingDependencies
{
    std::shared_ptr<IStreamingDataSource> data_source;
    std::shared_ptr<IStreamingCommitTarget> commit_target;
    std::shared_ptr<IStreamingPriorityProvider> priority_provider;
    std::shared_ptr<IResidencyController> residency_controller;
    std::shared_ptr<IStreamingWorldSource> world_source;
    std::shared_ptr<IStreamingPersistenceSource> persistence_source;
    std::shared_ptr<IStreamingResourceSource> resource_source;
};

[[nodiscard]] foundation::Result<StreamingServices> CreateStreamingServices(const StreamingDependencies& dependencies = {});
} // namespace epidemic::runtime::streaming
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Streaming/residency_controller.h`, `Epidemic/Runtime/Streaming/streaming_sources.h`, `Epidemic/Runtime/Streaming/streaming_types.h`.

### Состав типов и операции

#### `IStreamingRuntime` (строка заголовка 15)

Публичные поля и значения по умолчанию:

- `an already accepted external step is retained in`

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingRuntime() = default;
[[nodiscard]] virtual foundation::Result<StreamingDemandHandle> Request( const StreamingTarget& target, StreamingPriorityClass priority) = 0;
[[nodiscard]] virtual foundation::Result<void> ReleaseDemand(StreamingDemandHandle demand) = 0;
[[nodiscard]] virtual StreamingState GetChunkState(ChunkId chunk) const = 0;
virtual void SetBudget(const StreamingBudget& budget) = 0;
[[nodiscard]] virtual std::optional<StreamingProgress> GetProgress(StreamingRequestHandle request) const = 0;
[[nodiscard]] virtual StreamingStatistics GetStatistics() const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingQuery` (строка заголовка 35)

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingQuery() = default;
[[nodiscard]] virtual StreamingState GetChunkState(ChunkId chunk) const = 0;
[[nodiscard]] virtual std::optional<StreamingProgress> GetProgress(StreamingRequestHandle request) const = 0;
[[nodiscard]] virtual StreamingStatistics GetStatistics() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingController` (строка заголовка 45)

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingController() = default;
[[nodiscard]] virtual foundation::Result<void> CancelRequest(StreamingRequestHandle request) = 0;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `StreamingServices` (строка заголовка 54)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IStreamingRuntime> runtime`
- `std::shared_ptr<IStreamingQuery> query`
- `std::shared_ptr<IStreamingController> controller`

#### `StreamingDependencies` (строка заголовка 61)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IStreamingDataSource> data_source`
- `std::shared_ptr<IStreamingCommitTarget> commit_target`
- `std::shared_ptr<IStreamingPriorityProvider> priority_provider`
- `std::shared_ptr<IResidencyController> residency_controller`
- `std::shared_ptr<IStreamingWorldSource> world_source`
- `std::shared_ptr<IStreamingPersistenceSource> persistence_source`
- `std::shared_ptr<IStreamingResourceSource> resource_source`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Request` | `StreamingRuntime::Request` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:156` |
| `ReleaseDemand` | `StreamingRuntime::ReleaseDemand` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:482` |
| `Shutdown` | `StreamingRuntime::Shutdown` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:606` |
| `CancelRequest` | `StreamingRuntime::CancelRequest` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:729` |
| `GetChunkState` | `StreamingRuntime::GetChunkState` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:807` |
| `SetBudget` | `StreamingRuntime::SetBudget` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:813` |
| `GetProgress` | `StreamingRuntime::GetProgress` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:935` |
| `GetStatistics` | `StreamingRuntime::GetStatistics` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:946` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `CancelRequest`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:311`
- `GetProgress`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:316`
- `GetChunkState`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:323`
- `Request`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:336`
- `ReleaseDemand`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:356`
- `GetStatistics`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:429`
- `SetBudget`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:436`
- `Shutdown`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:972`

