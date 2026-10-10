---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/region.h"
---

# World: region.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/region.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 778 байт.
**Подключение:** `#include <Epidemic/Runtime/World/region.h>`.

## Объявления типов (лексический индекс)
`RegionDescriptor`, `IRegionRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include <optional>

namespace epidemic::runtime
{
struct RegionDescriptor
{
    RegionId id{};
    foundation::StringId name{};

    [[nodiscard]] constexpr bool operator==(const RegionDescriptor&) const noexcept = default;
};

class IRegionRegistry
{
  public:
    virtual ~IRegionRegistry() = default;
    [[nodiscard]] virtual foundation::Result<void> RegisterRegion(RegionDescriptor region) = 0;
    [[nodiscard]] virtual std::optional<RegionDescriptor> FindRegion(RegionId id) const = 0;
    virtual void Freeze() noexcept = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};
}
```
