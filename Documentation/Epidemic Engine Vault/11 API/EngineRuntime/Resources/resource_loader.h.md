---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h"
---

# Resources: resource_loader.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 826 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_loader.h>`.

## Объявления типов (лексический индекс)
`ResourceLoadArtifact`, `IResourceLoader`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Resources/resource_dependency_graph.h"
#include "Epidemic/Runtime/Resources/resource_payload.h"
#include "Epidemic/Runtime/Resources/resource_request.h"
#include "Epidemic/Runtime/Resources/resource_type.h"

#include <vector>

namespace epidemic::runtime
{
struct ResourceLoadArtifact
{
    ResourceId resource_id{};
    ResourceType type{};
    ResourcePayloadPtr payload{};
    std::vector<ResourceDependency> dependencies;
};

class IResourceLoader
{
  public:
    virtual ~IResourceLoader() = default;

    [[nodiscard]] virtual ResourceType GetResourceType() const = 0;
    [[nodiscard]] virtual foundation::Result<ResourceLoadArtifact> Load(ResourceRequest request) = 0;
};
} // namespace epidemic::runtime
```
