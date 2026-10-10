---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h"
---

# Resources: resource_dependency_graph.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 518 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_dependency_graph.h>`.

## Объявления типов (лексический индекс)
`ResourceDependency`, `ResourceDependencySet`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Resources/resource_type.h"


#include <vector>

namespace epidemic::runtime
{
struct ResourceDependency
{
    ResourceId resource_id{};
    ResourceType type{};
    bool required = true;

    [[nodiscard]] constexpr bool operator==(const ResourceDependency&) const noexcept = default;
};

struct ResourceDependencySet
{
    ResourceId root{};
    std::vector<ResourceDependency> dependencies;
};
}
```
