---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h"
---

# Streaming: streaming_sources.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h`.

**Модуль:** [[05 EngineRuntime/Модуль Streaming|Streaming]].
**Размер исходного файла:** 3066 байт.
**Подключение:** `#include <Epidemic/Runtime/Streaming/streaming_sources.h>`.

## Объявления типов (лексический индекс)
`IStreamingWorldSource`, `IStreamingPersistenceSource`, `IStreamingResourceSource`, `IStreamingDataSource`, `IStreamingCommitTarget`, `IStreamingPriorityProvider`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Streaming/streaming_types.h"

#include <optional>

namespace epidemic::runtime::streaming
{
// Adapter interfaces that let Streaming request chunk-related data from other majors
// through stable contracts, while still being able to run in mock mode when absent.
class IStreamingWorldSource
{
  public:
    virtual ~IStreamingWorldSource() = default;

    [[nodiscard]] virtual std::optional<RegionId> ResolveRegion(ChunkId chunk) const = 0;
};

class IStreamingPersistenceSource
{
  public:
    virtual ~IStreamingPersistenceSource() = default;

    [[nodiscard]] virtual foundation::Result<void> PrepareChunkData(const StreamingRequest& request) = 0;
};

class IStreamingResourceSource
{
  public:
    // ReleaseChunkResources may be retried only when the previous call did not report
    // success. Streaming itself never repeats a successfully completed release phase.
    virtual ~IStreamingResourceSource() = default;

    [[nodiscard]] virtual foundation::Result<void> PrepareChunkResources(const StreamingRequest& request) = 0;
    [[nodiscard]] virtual foundation::Result<void> ReleaseChunkResources(ChunkId chunk) = 0;
};

class IStreamingDataSource
{
  public:
    // BuildLoadPlan is a planning operation. A returned plan is detached Runtime input.
    // ExecuteStep has an acceptance boundary: Result failure or exception means that the
    // logical step unit was not accepted. Result success is accepted by Runtime exactly
    // once. If Runtime-local publication must be retried afterwards, the accepted result
    // is retained and ExecuteStep is not called again for that unit. completed=false may
    // still produce a later ExecuteStep call for the same plan cursor after the previous
    // partial result has been durably accounted.
    virtual ~IStreamingDataSource() = default;

    [[nodiscard]] virtual foundation::Result<ProgressiveLoadPlan> BuildLoadPlan(const StreamingRequest& request) = 0;
    [[nodiscard]] virtual foundation::Result<StreamingStepResult> ExecuteStep(
        const StreamingRequest& request,
        StreamingPlanStepRecord step,
        const RuntimeBudget& available_budget) = 0;
};

class IStreamingCommitTarget
{
  public:
    // request.handle is the stable operation identity. Implementations that can have
    // ambiguous transport/backend failures must use it as an idempotency key so a
    // retry cannot commit/rollback the same request twice.
    virtual ~IStreamingCommitTarget() = default;

    [[nodiscard]] virtual foundation::Result<void> Commit(const StreamingRequest& request) = 0;
    [[nodiscard]] virtual foundation::Result<void> Rollback(const StreamingRequest& request) = 0;
};

class IStreamingPriorityProvider
{
  public:
    virtual ~IStreamingPriorityProvider() = default;

    [[nodiscard]] virtual StreamingPriorityClass GetPriority(const StreamingTarget& target) const = 0;
};
} // namespace epidemic::runtime::streaming
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Streaming/streaming_types.h`.

### Состав типов и операции

#### `IStreamingWorldSource` (строка заголовка 12)

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingWorldSource() = default;
[[nodiscard]] virtual std::optional<RegionId> ResolveRegion(ChunkId chunk) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingPersistenceSource` (строка заголовка 20)

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingPersistenceSource() = default;
[[nodiscard]] virtual foundation::Result<void> PrepareChunkData(const StreamingRequest& request) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingResourceSource` (строка заголовка 28)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] virtual foundation::Result<void> PrepareChunkResources(const StreamingRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> ReleaseChunkResources(ChunkId chunk) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingDataSource` (строка заголовка 39)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] virtual foundation::Result<ProgressiveLoadPlan> BuildLoadPlan(const StreamingRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<StreamingStepResult> ExecuteStep( const StreamingRequest& request, StreamingPlanStepRecord step, const RuntimeBudget& available_budget) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingCommitTarget` (строка заголовка 58)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] virtual foundation::Result<void> Commit(const StreamingRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> Rollback(const StreamingRequest& request) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IStreamingPriorityProvider` (строка заголовка 70)

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingPriorityProvider() = default;
[[nodiscard]] virtual StreamingPriorityClass GetPriority(const StreamingTarget& target) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Rollback` | `StreamingRuntime::Rollback` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:1473` |

