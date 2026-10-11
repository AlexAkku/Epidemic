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

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/GameFramework/Foundation/change_cursor.h`, `Epidemic/GameFramework/Foundation/gameplay_context.h`, `Epidemic/GameFramework/Foundation/id_generator.h`, `Epidemic/GameFramework/Foundation/ids.h`, `Epidemic/GameFramework/Foundation/tags.h`, `Epidemic/GameFramework/Foundation/time_types.h`, `Epidemic/GameFramework/Foundation/type_registry.h`.

Этот заголовок преимущественно содержит free-functions, type aliases или implementation helpers. Точные сигнатуры смотрите в исходном блоке выше.

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

