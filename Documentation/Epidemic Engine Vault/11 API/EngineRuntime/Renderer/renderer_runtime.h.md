---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_runtime.h"
---

# Renderer: renderer_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Renderer|Renderer]].
**Размер исходного файла:** 1092 байт.
**Подключение:** `#include <Epidemic/Runtime/Renderer/renderer_runtime.h>`.

## Объявления типов (лексический индекс)
`IRendererRuntime`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Renderer/render_types.h"

namespace epidemic::runtime::renderer
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IRendererRuntime
{
  public:
    virtual ~IRendererRuntime() = default;

    // PrepareFrame resolves/stages fallible scene/resource/pose observations without opening a backend frame.
    // Calling it again before RenderFrame replaces the staged frame.
    [[nodiscard]] virtual foundation::Result<void> PrepareFrame() = 0;
    // RenderFrame consumes a successfully prepared frame. If AbortFrame could not confirm closure after
    // Submit/End failure, the next PrepareFrame/RenderFrame/Shutdown first retries reconciliation.
    [[nodiscard]] virtual foundation::Result<void> RenderFrame() = 0;
    [[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
    [[nodiscard]] virtual RenderFrameState GetFrameState() const = 0;
};
} // namespace epidemic::runtime::renderer
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Renderer/render_types.h`.

### Состав типов и операции

#### `IRendererRuntime` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IRendererRuntime() = default;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
[[nodiscard]] virtual RenderFrameState GetFrameState() const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Shutdown` | `RendererRuntime::Shutdown` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:734` |
| `GetFrameState` | `RendererRuntime::GetFrameState` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:793` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetFrameState`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:435`
- `Shutdown`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:556`

