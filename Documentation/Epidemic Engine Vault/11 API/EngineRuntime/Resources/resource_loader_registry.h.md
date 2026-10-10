---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h"
---

# Resources: resource_loader_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 1048 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_loader_registry.h>`.

## Объявления типов (лексический индекс)
`IResourceLoaderRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Resources/resource_loader.h"

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IResourceLoaderRegistry
{
  public:
    virtual ~IResourceLoaderRegistry() = default;

    // The loader registry does not own loaders. Registered loaders must outlive the
    // registry and any resource manager that resolves loaders through it.
    [[nodiscard]] virtual foundation::Result<void> RegisterLoader(IResourceLoader& loader) = 0;
    [[nodiscard]] virtual IResourceLoader* FindLoader(ResourceType type) = 0;
    [[nodiscard]] virtual const IResourceLoader* FindLoader(ResourceType type) const = 0;
    [[nodiscard]] virtual bool HasLoader(ResourceType type) const = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};
} // namespace epidemic::runtime
```
