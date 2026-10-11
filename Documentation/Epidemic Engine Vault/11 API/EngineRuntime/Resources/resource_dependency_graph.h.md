---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h"
---

# Resources: resource_dependency_graph.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 518 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_dependency_graph.h>`.

## Объявления типов (лексический индекс)
`ResourceDependency`, `ResourceDependencySet`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Resources/resource_type.h"


#include <vector>

namespace epidemic::runtime
{
struct ResourceDependency
{
    ResourceId resource_id{};
    ResourceType type{};
    bool required = true;

    [[nodiscard]] constexpr bool operator==(const ResourceDependency&) const noexcept = default;
};

struct ResourceDependencySet
{
    ResourceId root{};
    std::vector<ResourceDependency> dependencies;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Resources/resource_type.h`.

### Состав типов и операции

#### `ResourceDependency` (строка заголовка 11)

Публичные поля и значения по умолчанию:

- `bool required = true`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const ResourceDependency&) const noexcept = default;
```

#### `ResourceDependencySet` (строка заголовка 20)

Публичные поля и значения по умолчанию:

- `std::vector<ResourceDependency> dependencies`

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

