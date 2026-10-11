---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_manager.h"
---

# Resources: resource_manager.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_manager.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 2296 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_manager.h>`.

## Объявления типов (лексический индекс)
`ResourceProcessingStats`, `ResourceMemoryStats`, `IResourceManager`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Resources/resource_handle.h"
#include "Epidemic/Runtime/Resources/resource_payload.h"
#include "Epidemic/Runtime/Resources/resource_request.h"
#include "Epidemic/Runtime/Resources/resource_state.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace epidemic::runtime
{
struct ResourceProcessingStats
{
    std::size_t attempted_jobs = 0;
    std::size_t processed_jobs = 0;
    std::size_t skipped_jobs = 0;
    std::size_t loaded_resources = 0;
    std::size_t failed_resources = 0;
    std::size_t bytes_loaded = 0;
};

struct ResourceMemoryStats
{
    std::size_t resident_bytes = 0;
    std::size_t budget_bytes = 0;
    std::size_t slot_count = 0;
    std::size_t ready_count = 0;
    std::size_t ready_bytes = 0;
    std::size_t cached_unreferenced_bytes = 0;
    std::size_t resource_count = 0;
    std::size_t invariant_failure_count = 0;
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IResourceManager
{
  public:
    virtual ~IResourceManager() = default;

    [[nodiscard]] virtual foundation::Result<ResourceLease> RequestLease(ResourceRequest request) = 0;
    [[nodiscard]] virtual foundation::Result<ResourceProcessingStats> ProcessPendingLoads(RuntimeBudget budget = {}) = 0;
    [[nodiscard]] virtual foundation::Result<void> Release(ResourceLease lease) = 0;
    [[nodiscard]] virtual foundation::Result<void> Evict(ResourceId id) = 0;
    [[nodiscard]] virtual std::size_t EvictUnreferenced() = 0;

    [[nodiscard]] virtual foundation::Result<void> ValidateHandle(ResourceHandle handle) const = 0;
    [[nodiscard]] virtual ResourceState GetState(ResourceHandle handle) const = 0;
    [[nodiscard]] virtual bool IsReady(ResourceHandle handle) const = 0;
    [[nodiscard]] virtual std::optional<ResourceId> GetResourceId(ResourceHandle handle) const = 0;
    [[nodiscard]] virtual ResourcePayloadPtr GetPayload(ResourceHandle handle) const = 0;

    virtual void SetMemoryBudgetBytes(std::size_t bytes) = 0;
    [[nodiscard]] virtual ResourceMemoryStats GetMemoryStatistics() const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Resources/resource_handle.h`, `Epidemic/Runtime/Resources/resource_payload.h`, `Epidemic/Runtime/Resources/resource_request.h`, `Epidemic/Runtime/Resources/resource_state.h`.

### Состав типов и операции

#### `ResourceProcessingStats` (строка заголовка 15)

Публичные поля и значения по умолчанию:

- `std::size_t attempted_jobs = 0`
- `std::size_t processed_jobs = 0`
- `std::size_t skipped_jobs = 0`
- `std::size_t loaded_resources = 0`
- `std::size_t failed_resources = 0`
- `std::size_t bytes_loaded = 0`

#### `ResourceMemoryStats` (строка заголовка 25)

Публичные поля и значения по умолчанию:

- `std::size_t resident_bytes = 0`
- `std::size_t budget_bytes = 0`
- `std::size_t slot_count = 0`
- `std::size_t ready_count = 0`
- `std::size_t ready_bytes = 0`
- `std::size_t cached_unreferenced_bytes = 0`
- `std::size_t resource_count = 0`
- `std::size_t invariant_failure_count = 0`

#### `IResourceManager` (строка заголовка 39)

Публичные сигнатуры операций:

```cpp
virtual ~IResourceManager() = default;
[[nodiscard]] virtual foundation::Result<ResourceLease> RequestLease(ResourceRequest request) = 0;
[[nodiscard]] virtual foundation::Result<void> Release(ResourceLease lease) = 0;
[[nodiscard]] virtual foundation::Result<void> Evict(ResourceId id) = 0;
[[nodiscard]] virtual std::size_t EvictUnreferenced() = 0;
[[nodiscard]] virtual foundation::Result<void> ValidateHandle(ResourceHandle handle) const = 0;
[[nodiscard]] virtual ResourceState GetState(ResourceHandle handle) const = 0;
[[nodiscard]] virtual bool IsReady(ResourceHandle handle) const = 0;
[[nodiscard]] virtual std::optional<ResourceId> GetResourceId(ResourceHandle handle) const = 0;
[[nodiscard]] virtual ResourcePayloadPtr GetPayload(ResourceHandle handle) const = 0;
virtual void SetMemoryBudgetBytes(std::size_t bytes) = 0;
[[nodiscard]] virtual ResourceMemoryStats GetMemoryStatistics() const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RequestLease` | `ResourceManager::RequestLease` | `EngineRuntime/Resources/src/resource_manager.cpp:213` |
| `Release` | `ResourceManager::Release` | `EngineRuntime/Resources/src/resource_manager.cpp:475` |
| `Evict` | `ResourceManager::Evict` | `EngineRuntime/Resources/src/resource_manager.cpp:500` |
| `EvictUnreferenced` | `ResourceManager::EvictUnreferenced` | `EngineRuntime/Resources/src/resource_manager.cpp:540` |
| `ValidateHandle` | `ResourceManager::ValidateHandle` | `EngineRuntime/Resources/src/resource_manager.cpp:572` |
| `GetState` | `ResourceManager::GetState` | `EngineRuntime/Resources/src/resource_manager.cpp:586` |
| `IsReady` | `ResourceManager::IsReady` | `EngineRuntime/Resources/src/resource_manager.cpp:596` |
| `GetResourceId` | `ResourceManager::GetResourceId` | `EngineRuntime/Resources/src/resource_manager.cpp:601` |
| `GetPayload` | `ResourceManager::GetPayload` | `EngineRuntime/Resources/src/resource_manager.cpp:611` |
| `SetMemoryBudgetBytes` | `ResourceManager::SetMemoryBudgetBytes` | `EngineRuntime/Resources/src/resource_manager.cpp:621` |
| `GetMemoryStatistics` | `ResourceManager::GetMemoryStatistics` | `EngineRuntime/Resources/src/resource_manager.cpp:626` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RequestLease`: `EngineRuntime/Resources/tests/resources_tests.cpp:308`
- `GetState`: `EngineRuntime/Resources/tests/resources_tests.cpp:309`
- `GetPayload`: `EngineRuntime/Resources/tests/resources_tests.cpp:315`
- `GetMemoryStatistics`: `EngineRuntime/Resources/tests/resources_tests.cpp:316`
- `IsReady`: `EngineRuntime/Resources/tests/resources_tests.cpp:318`
- `Release`: `EngineRuntime/Resources/tests/resources_tests.cpp:421`
- `Evict`: `EngineRuntime/Resources/tests/resources_tests.cpp:428`
- `SetMemoryBudgetBytes`: `EngineRuntime/Resources/tests/resources_tests.cpp:499`
- `EvictUnreferenced`: `EngineRuntime/Resources/tests/resources_tests.cpp:806`
- `ValidateHandle`: `EngineRuntime/Resources/tests/resources_tests.cpp:829`
- `GetResourceId`: `EngineRuntime/Resources/tests/resources_tests.cpp:1335`

