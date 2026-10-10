---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_foundation.h"
---

# Foundation: gameplay_foundation.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

**Модуль:** [[06 EngineFramework/Модуль Foundation|Foundation]].
**Размер исходного файла:** 684 байт.
**Подключение:** `#include <Epidemic/GameFramework/Foundation/gameplay_foundation.h>`.

## Объявления типов (лексический индекс)
`SnapshotRestoreAccess`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/GameFramework/Foundation/change_cursor.h"
#include "Epidemic/GameFramework/Foundation/gameplay_context.h"
#include "Epidemic/GameFramework/Foundation/id_generator.h"
#include "Epidemic/GameFramework/Foundation/ids.h"
#include "Epidemic/GameFramework/Foundation/tags.h"
#include "Epidemic/GameFramework/Foundation/time_types.h"
#include "Epidemic/GameFramework/Foundation/type_registry.h"

namespace epidemic::gameplay
{
// Ownership-local prepare/publication customization. Defined by upper-layer
// adapters; this declaration creates no dependency on SaveGame or peer owners.
template <typename Owner> struct SnapshotRestoreAccess;
}
```
