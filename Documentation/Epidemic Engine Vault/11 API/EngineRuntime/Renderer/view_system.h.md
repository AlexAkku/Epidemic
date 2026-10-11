---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/view_system.h"
---

# Renderer: view_system.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/view_system.h`.

**Модуль:** [[05 EngineRuntime/Модуль Renderer|Renderer]].
**Размер исходного файла:** 807 байт.
**Подключение:** `#include <Epidemic/Runtime/Renderer/view_system.h>`.

## Объявления типов (лексический индекс)
`IViewSystem`

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
class IViewSystem
{
  public:
    virtual ~IViewSystem() = default;

    [[nodiscard]] virtual foundation::Result<ViewId> CreateView(const ViewDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyView(ViewId view) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetMainView(ViewId view) = 0;
    [[nodiscard]] virtual ViewId GetMainView() const = 0;
    [[nodiscard]] virtual ViewLifecycle GetViewLifecycle(ViewId view) const = 0;
};
} // namespace epidemic::runtime::renderer
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Renderer/render_types.h`.

### Состав типов и операции

#### `IViewSystem` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IViewSystem() = default;
[[nodiscard]] virtual foundation::Result<ViewId> CreateView(const ViewDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyView(ViewId view) = 0;
[[nodiscard]] virtual foundation::Result<void> SetMainView(ViewId view) = 0;
[[nodiscard]] virtual ViewId GetMainView() const = 0;
[[nodiscard]] virtual ViewLifecycle GetViewLifecycle(ViewId view) const = 0;
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
| `CreateView` | `RendererRuntime::CreateView` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:377` |
| `DestroyView` | `RendererRuntime::DestroyView` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:420` |
| `SetMainView` | `RendererRuntime::SetMainView` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:437` |
| `GetMainView` | `RendererRuntime::GetMainView` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:454` |
| `GetViewLifecycle` | `RendererRuntime::GetViewLifecycle` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:459` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `CreateView`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:374`
- `SetMainView`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:375`
- `DestroyView`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:379`
- `GetViewLifecycle`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:380`
- `GetMainView`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:1087`

