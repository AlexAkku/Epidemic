---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location_resolver.h"
---

# Assets: asset_location_resolver.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location_resolver.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 871 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_location_resolver.h>`.

## Объявления типов (лексический индекс)
`IAssetLocationResolver`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Assets/asset_location.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

namespace epidemic::runtime
{
// Small resolver interface that returns the registered logical location for an
// asset id without exposing the full asset catalog implementation.
class IAssetLocationResolver
{
  public:
    virtual ~IAssetLocationResolver() = default;

    // Resolves the stored logical location for an asset id without opening files,
    // mounting packages, or acquiring resources. Invalid id -> asset.invalid_id;
    // valid-but-unregistered id -> asset.not_found. Success returns a detached copy.
    // Allocation failure may propagate and cannot mutate catalog state.
    [[nodiscard]] virtual foundation::Result<AssetLocation> Resolve(AssetId id) const = 0;
};
}
```
