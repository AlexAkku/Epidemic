---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_request.h"
---

# Resources: resource_request.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_request.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 306 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_request.h>`.

## Объявления типов (лексический индекс)
`ResourceRequest`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_budget.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Resources/resource_type.h"
namespace epidemic::runtime
{
struct ResourceRequest
{
    ResourceId resource_id{};
    ResourceType type{};
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_budget.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Resources/resource_type.h`.

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

