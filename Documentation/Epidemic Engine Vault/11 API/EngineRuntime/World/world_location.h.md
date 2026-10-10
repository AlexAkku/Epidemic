---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_location.h"
---

# World: world_location.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_location.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 282 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_location.h>`.

## Объявления типов (лексический индекс)
`WorldLocation`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
namespace epidemic::runtime
{
struct WorldLocation
{
    RegionId region_id{};
    ChunkId chunk_id{};

    [[nodiscard]] constexpr bool operator==(const WorldLocation&) const noexcept = default;
};
}
```
