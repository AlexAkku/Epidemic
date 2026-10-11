---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_foundation.h"
---

# RuntimeFoundation: runtime_foundation.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_foundation.h`.

**Модуль:** [[05 EngineRuntime/Модуль RuntimeFoundation|RuntimeFoundation]].
**Размер исходного файла:** 475 байт.
**Подключение:** `#include <Epidemic/Runtime/Foundation/runtime_foundation.h>`.

## Объявления типов (лексический индекс)
_Именованные `class`/`struct`/`enum` в этом заголовке не обнаружены простой лексической проверкой._

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/checked_id_allocator.h"
#include "Epidemic/Runtime/Foundation/numeric_validation.h"
#include "Epidemic/Runtime/Foundation/runtime_budget.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_operation.h"
#include "Epidemic/Runtime/Foundation/runtime_states.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include "Epidemic/Runtime/Foundation/spatial.h"
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/checked_id_allocator.h`, `Epidemic/Runtime/Foundation/numeric_validation.h`, `Epidemic/Runtime/Foundation/runtime_budget.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/runtime_operation.h`, `Epidemic/Runtime/Foundation/runtime_states.h`, `Epidemic/Runtime/Foundation/runtime_time.h`, `Epidemic/Runtime/Foundation/spatial.h`.

Этот заголовок преимущественно содержит free-functions, type aliases или implementation helpers. Точные сигнатуры смотрите в исходном блоке выше.

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

