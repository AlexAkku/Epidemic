---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Core/include/Epidemic/Core/imodule.h"
---

# Core: imodule.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Core/include/Epidemic/Core/imodule.h`.

**Модуль:** [[04 EngineBase/Модуль Core|Core]].
**Размер исходного файла:** 1198 байт.
**Подключение:** `#include <Epidemic/Core/imodule.h>`.

## Объявления типов (лексический индекс)
`IModule`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Core/frame_context.h>
#include <Epidemic/Core/module_manifest.h>
#include <Epidemic/Core/service_container.h>

namespace epidemic::core
{
// This file defines the lifecycle contract for EngineBase modules.
// Modules participate in composition through Bootstrap/Initialize/Tick/Shutdown and expose
// static dependency metadata through ModuleManifest.

class IModule
{
  public:
    virtual ~IModule() = default;

    // Returns static identity and dependency metadata for the module.
    [[nodiscard]] virtual const ModuleManifest &Manifest() const = 0;

    // Performs service registration and other dependency-free setup before initialization.
    virtual void Bootstrap(ServiceContainer &services) = 0;

    // Performs runtime initialization after all modules have bootstrapped.
    virtual void Initialize(ServiceContainer &services) = 0;

    // Executes the module's per-frame work.
    virtual void Tick(ServiceContainer &services, const FrameContext &frame_context) = 0;

    // Releases runtime state in reverse dependency order.
    virtual void Shutdown(ServiceContainer &services) = 0;
};
} // namespace epidemic::core
```
