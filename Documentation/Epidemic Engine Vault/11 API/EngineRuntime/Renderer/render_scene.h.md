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
