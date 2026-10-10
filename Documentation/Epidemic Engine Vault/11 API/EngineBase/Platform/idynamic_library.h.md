---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h"
---

# Platform: idynamic_library.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h`.

**Модуль:** [[04 EngineBase/Модуль Platform|Platform]].
**Размер исходного файла:** 860 байт.
**Подключение:** `#include <Epidemic/Platform/idynamic_library.h>`.

## Объявления типов (лексический индекс)
`IDynamicLibrary`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/result.h>

#include <memory>
#include <string_view>

namespace epidemic::platform
{
// This file defines the minimal dynamic-library abstraction returned by the platform runtime.
// The abstraction keeps symbol lookup behind Result-based error reporting instead of exceptions.

class IDynamicLibrary
{
  public:
    virtual ~IDynamicLibrary() = default;

    // Returns the library name or origin label used for diagnostics.
    [[nodiscard]] virtual std::string_view Name() const = 0;

    // Resolves a symbol and returns either its address or an Error describing the failure.
    [[nodiscard]] virtual epidemic::foundation::Result<void *> FindSymbol(std::string_view symbol_name) const = 0;
};

using DynamicLibraryPtr = std::shared_ptr<IDynamicLibrary>;
} // namespace epidemic::platform
```
