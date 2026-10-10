---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h"
---

# Serialization: serializer_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 887 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/serializer_registry.h>`.

## Объявления типов (лексический индекс)
`ISerializerRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/serializer.h"

#include <memory>

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ISerializerRegistry
{
  public:
    virtual ~ISerializerRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterSerializer(std::shared_ptr<const ISerializer> serializer) = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<const ISerializer> FindSerializer(foundation::StringId type_id) const = 0;
    [[nodiscard]] virtual bool HasSerializer(foundation::StringId type_id) const = 0;
};
} // namespace epidemic::runtime
```
