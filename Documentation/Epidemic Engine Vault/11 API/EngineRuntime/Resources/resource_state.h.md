---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_state.h"
---

# Resources: resource_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 1658 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_state.h>`.

## Объявления типов (лексический индекс)
`ResourceState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
enum class ResourceState
{
    Unknown,
    Unloaded,
    Queued,
    Loading,
    WaitingForDependencies,
    Ready,
    Failed,
    Evicting,
    Evicted,
    Reloading,
};

[[nodiscard]] constexpr bool CanTransition(ResourceState from, ResourceState to) noexcept
{
    switch (from)
    {
    case ResourceState::Unknown:
        return to == ResourceState::Unloaded || to == ResourceState::Queued || to == ResourceState::Failed;
    case ResourceState::Unloaded:
        return to == ResourceState::Queued || to == ResourceState::Evicted;
    case ResourceState::Queued:
        return to == ResourceState::Loading || to == ResourceState::Evicted || to == ResourceState::Failed;
    case ResourceState::Loading:
        return to == ResourceState::WaitingForDependencies || to == ResourceState::Ready || to == ResourceState::Failed;
    case ResourceState::WaitingForDependencies:
        return to == ResourceState::Queued || to == ResourceState::Ready || to == ResourceState::Failed;
    case ResourceState::Ready:
        return to == ResourceState::Reloading || to == ResourceState::Evicting;
    case ResourceState::Reloading:
        return to == ResourceState::Loading || to == ResourceState::Ready || to == ResourceState::Failed;
    case ResourceState::Evicting:
        return to == ResourceState::Evicted;
    case ResourceState::Evicted:
        return to == ResourceState::Queued;
    case ResourceState::Failed:
        return to == ResourceState::Queued || to == ResourceState::Evicted;
    }
    return false;
}
} // namespace epidemic::runtime
```
