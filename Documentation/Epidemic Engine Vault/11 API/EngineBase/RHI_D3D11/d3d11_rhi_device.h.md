---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI_D3D11/include/Epidemic/RHI_D3D11/d3d11_rhi_device.h"
---

# RHI_D3D11: d3d11_rhi_device.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI_D3D11/include/Epidemic/RHI_D3D11/d3d11_rhi_device.h`.

**Модуль:** [[04 EngineBase/Модуль RHI_D3D11|RHI_D3D11]].
**Размер исходного файла:** 622 байт.
**Подключение:** `#include <Epidemic/RHI_D3D11/d3d11_rhi_device.h>`.

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

namespace epidemic::rhi::d3d11
{
// This file exposes the D3D11 RHI backend factory used by EngineBase support wiring.
// Backend creation returns Result because device creation and swap-chain setup are expected runtime failures.

// Creates a D3D11 RHI device or returns an Error describing why backend initialization failed.
[[nodiscard]] epidemic::foundation::Result<std::shared_ptr<IRhiDevice>>
CreateD3D11RhiDevice(const RhiDeviceDesc &device_desc = {});
} // namespace epidemic::rhi::d3d11
```
