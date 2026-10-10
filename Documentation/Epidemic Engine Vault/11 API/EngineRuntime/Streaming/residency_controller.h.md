---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h"
---

# Streaming: residency_controller.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h`.

**Модуль:** [[05 EngineRuntime/Модуль Streaming|Streaming]].
**Размер исходного файла:** 891 байт.
**Подключение:** `#include <Epidemic/Runtime/Streaming/residency_controller.h>`.

## Объявления типов (лексический индекс)
`IResidencyController`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

namespace epidemic::runtime::streaming
{
// Residency transition contract used by Streaming to ask an external controller to
// activate, deactivate or unload chunk-owned runtime state. A successful operation is
// never repeated by Streaming for the same request phase. Backends with ambiguous
// failures should use the chunk/request-side identity as an idempotency key.
class IResidencyController
{
  public:
    virtual ~IResidencyController() = default;

    [[nodiscard]] virtual foundation::Result<void> ActivateChunk(ChunkId chunk) = 0;
    [[nodiscard]] virtual foundation::Result<void> DeactivateChunk(ChunkId chunk) = 0;
    [[nodiscard]] virtual foundation::Result<void> UnloadChunk(ChunkId chunk) = 0;
};
} // namespace epidemic::runtime::streaming
```
