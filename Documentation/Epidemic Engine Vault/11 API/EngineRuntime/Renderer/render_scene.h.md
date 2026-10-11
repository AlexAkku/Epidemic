---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_scene.h"
---

# Renderer: render_scene.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_scene.h`.

**Модуль:** [[05 EngineRuntime/Модуль Renderer|Renderer]].
**Размер исходного файла:** 1515 байт.
**Подключение:** `#include <Epidemic/Runtime/Renderer/render_scene.h>`.

## Объявления типов (лексический индекс)
`IRenderScene`

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
class IRenderScene
{
  public:
    virtual ~IRenderScene() = default;

    [[nodiscard]] virtual foundation::Result<RenderProxyId> RegisterProxy(const RenderProxyDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyProxy(RenderProxyId id) = 0;
    // Retryable prefix drain: successfully released proxies stay removed; a failed release remains DestroyPending for retry.
    [[nodiscard]] virtual foundation::Result<void> FlushDeferredDestroys() = 0;
    [[nodiscard]] virtual foundation::Result<void> MarkTransformDirty(RenderProxyId id) = 0;
    [[nodiscard]] virtual foundation::Result<void> MarkMaterialDirty(RenderProxyId id) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetProxyVisibility(RenderProxyId id, RenderProxyVisibility visibility) = 0;
    [[nodiscard]] virtual RenderProxyLifecycle GetProxyLifecycle(RenderProxyId id) const = 0;
    [[nodiscard]] virtual RenderProxyReadiness GetProxyReadiness(RenderProxyId id) const = 0;
    [[nodiscard]] virtual RenderProxyVisibility GetProxyVisibility(RenderProxyId id) const = 0;
    [[nodiscard]] virtual RenderProxyDirtyMask GetProxyDirtyFlags(RenderProxyId id) const = 0;
};
} // namespace epidemic::runtime::renderer
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Renderer/render_types.h`.

### Состав типов и операции

#### `IRenderScene` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderScene() = default;
[[nodiscard]] virtual foundation::Result<RenderProxyId> RegisterProxy(const RenderProxyDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyProxy(RenderProxyId id) = 0;
a failed release remains DestroyPending for retry. [[nodiscard]] virtual foundation::Result<void> FlushDeferredDestroys() = 0;
[[nodiscard]] virtual foundation::Result<void> MarkTransformDirty(RenderProxyId id) = 0;
[[nodiscard]] virtual foundation::Result<void> MarkMaterialDirty(RenderProxyId id) = 0;
[[nodiscard]] virtual foundation::Result<void> SetProxyVisibility(RenderProxyId id, RenderProxyVisibility visibility) = 0;
[[nodiscard]] virtual RenderProxyLifecycle GetProxyLifecycle(RenderProxyId id) const = 0;
[[nodiscard]] virtual RenderProxyReadiness GetProxyReadiness(RenderProxyId id) const = 0;
[[nodiscard]] virtual RenderProxyVisibility GetProxyVisibility(RenderProxyId id) const = 0;
[[nodiscard]] virtual RenderProxyDirtyMask GetProxyDirtyFlags(RenderProxyId id) const = 0;
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
| `RegisterProxy` | `RendererRuntime::RegisterProxy` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:142` |
| `DestroyProxy` | `RendererRuntime::DestroyProxy` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:218` |
| `FlushDeferredDestroys` | `RendererRuntime::FlushDeferredDestroys` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:235` |
| `MarkTransformDirty` | `RendererRuntime::MarkTransformDirty` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:297` |
| `MarkMaterialDirty` | `RendererRuntime::MarkMaterialDirty` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:314` |
| `SetProxyVisibility` | `RendererRuntime::SetProxyVisibility` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:331` |
| `GetProxyLifecycle` | `RendererRuntime::GetProxyLifecycle` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:353` |
| `GetProxyReadiness` | `RendererRuntime::GetProxyReadiness` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:359` |
| `GetProxyVisibility` | `RendererRuntime::GetProxyVisibility` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:365` |
| `GetProxyDirtyFlags` | `RendererRuntime::GetProxyDirtyFlags` | `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:371` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterProxy`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:306`
- `GetProxyLifecycle`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:307`
- `GetProxyReadiness`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:308`
- `GetProxyVisibility`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:309`
- `GetProxyDirtyFlags`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:310`
- `MarkTransformDirty`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:336`
- `SetProxyVisibility`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:337`
- `DestroyProxy`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:356`
- `FlushDeferredDestroys`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:362`
- `MarkMaterialDirty`: `EngineRuntime/Renderer/tests/renderer_tests.cpp:625`

