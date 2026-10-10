---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI/include/Epidemic/RHI/null_rhi_device.h"
---

# RHI: null_rhi_device.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI/include/Epidemic/RHI/null_rhi_device.h`.

**Модуль:** [[04 EngineBase/Модуль RHI|RHI]].
**Размер исходного файла:** 570 байт.
**Подключение:** `#include <Epidemic/RHI/null_rhi_device.h>`.

## Объявления типов (лексический индекс)
_Именованные `class`/`struct`/`enum` в этом заголовке не обнаружены простой лексической проверкой._

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/result.h>
#include <Epidemic/RHI/irhi_device.h>

#include <memory>

namespace epidemic::rhi
{
// This file exposes the baseline software/null RHI backend used by tests and headless scenarios.
// The null device validates descriptors and simulates clear/present flow without touching a GPU.

// Creates a null RHI device or returns a validation error.
[[nodiscard]] epidemic::foundation::Result<std::shared_ptr<IRhiDevice>>
CreateNullRhiDevice(const RhiDeviceDesc &device_desc = {});
} // namespace epidemic::rhi
```
