---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_object_registry.h"
---

# World: world_object_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_object_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 875 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_object_registry.h>`.

## Объявления типов (лексический индекс)
`IWorldObjectRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/World/world_commands.h"
#include "Epidemic/Runtime/World/world_object.h"

namespace epidemic::runtime
{
class IWorldObjectRegistry
{
  public:
    virtual ~IWorldObjectRegistry() = default;

    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const CreateObjectCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangePlacementCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangeResidencyCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const PromotePersistenceTierCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const DestroyObjectCommand& command) = 0;
};
}
```
