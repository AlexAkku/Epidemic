---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h"
---

# Streaming: streaming_priority_resolver.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h`.

**Модуль:** [[05 EngineRuntime/Модуль Streaming|Streaming]].
**Размер исходного файла:** 526 байт.
**Подключение:** `#include <Epidemic/Runtime/Streaming/streaming_priority_resolver.h>`.

## Объявления типов (лексический индекс)
`IStreamingPriorityResolver`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Streaming/streaming_types.h"

namespace epidemic::runtime::streaming
{
// Optional policy interface that lets integrations derive request priority from external
// context such as player position, camera focus or region importance.
class IStreamingPriorityResolver
{
  public:
    virtual ~IStreamingPriorityResolver() = default;

    [[nodiscard]] virtual StreamingPriorityClass ResolvePriority(ChunkId chunk) const = 0;
};
} // namespace epidemic::runtime::streaming
```
