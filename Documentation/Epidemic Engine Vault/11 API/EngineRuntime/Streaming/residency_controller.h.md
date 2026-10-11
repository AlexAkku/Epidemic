---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h"
---

# Streaming: residency_controller.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h`.

**Модуль:** [[05 EngineRuntime/Модуль Streaming|Streaming]].
**Размер исходного файла:** 891 байт.
**Подключение:** `#include <Epidemic/Runtime/Streaming/residency_controller.h>`.

## Объявления типов (лексический индекс)
`IResidencyController`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

namespace epidemic::runtime::streaming
{
// Residency transition contract used by Streaming to ask an external controller to
// activate, deactivate or unload chunk-owned runtime state. A successful operation is
// never repeated by Streaming for the same request phase. Backends with ambiguous
// failures should use the chunk/request-side identity as an idempotency key.
class IResidencyController
{
  public:
    virtual ~IResidencyController() = default;

    [[nodiscard]] virtual foundation::Result<void> ActivateChunk(ChunkId chunk) = 0;
    [[nodiscard]] virtual foundation::Result<void> DeactivateChunk(ChunkId chunk) = 0;
    [[nodiscard]] virtual foundation::Result<void> UnloadChunk(ChunkId chunk) = 0;
};
} // namespace epidemic::runtime::streaming
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Состав типов и операции

#### `IResidencyController` (строка заголовка 12)

Публичные сигнатуры операций:

```cpp
virtual ~IResidencyController() = default;
[[nodiscard]] virtual foundation::Result<void> ActivateChunk(ChunkId chunk) = 0;
[[nodiscard]] virtual foundation::Result<void> DeactivateChunk(ChunkId chunk) = 0;
[[nodiscard]] virtual foundation::Result<void> UnloadChunk(ChunkId chunk) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `ActivateChunk` | `InMemoryResidencyController::ActivateChunk` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:67` |
| `DeactivateChunk` | `InMemoryResidencyController::DeactivateChunk` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:98` |
| `UnloadChunk` | `InMemoryResidencyController::UnloadChunk` | `EngineRuntime/Streaming/src/streaming_runtime_impl.cpp:121` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `DeactivateChunk`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:1162`
- `ActivateChunk`: `EngineRuntime/Streaming/tests/streaming_tests.cpp:1164`

