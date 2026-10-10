---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h"
---

# Physics: physics_event_buffer.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h`.

**Модуль:** [[05 EngineRuntime/Модуль Physics|Physics]].
**Размер исходного файла:** 478 байт.
**Подключение:** `#include <Epidemic/Runtime/Physics/physics_event_buffer.h>`.

## Объявления типов (лексический индекс)
`IPhysicsEventBuffer`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Physics/physics_types.h"

#include <span>

namespace epidemic::runtime::physics
{
// Event boundary contract for publishing contacts out of Physics without mutating World directly.
class IPhysicsEventBuffer
{
  public:
    virtual ~IPhysicsEventBuffer() = default;

    [[nodiscard]] virtual std::span<const ContactEvent> Contacts() const = 0;
    virtual void Clear() = 0;
};
} // namespace epidemic::runtime::physics
```
