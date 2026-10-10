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
