---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Core/include/Epidemic/Core/module_manifest.h"
---

# Core: module_manifest.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Core/include/Epidemic/Core/module_manifest.h`.

**Модуль:** [[04 EngineBase/Модуль Core|Core]].
**Размер исходного файла:** 403 байт.
**Подключение:** `#include <Epidemic/Core/module_manifest.h>`.

## Объявления типов (лексический индекс)
`ModuleManifest`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <string>
#include <vector>

namespace epidemic::core
{
// This file defines the static metadata used by ModuleRegistry.
// Dependencies are expressed by module id and are resolved into an execution order during bootstrap.

struct ModuleManifest
{
    std::string id;
    std::string name;
    std::vector<std::string> dependencies;
};
} // namespace epidemic::core
```
