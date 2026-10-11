---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_services.h"
---

# Resources: resource_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 620 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_services.h>`.

## Объявления типов (лексический индекс)
`ResourceOptions`, `ResourceServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Resources/resource_loader_registry.h"
#include "Epidemic/Runtime/Resources/resource_manager.h"

#include <cstddef>
#include <memory>

namespace epidemic::runtime
{
struct ResourceOptions
{
    std::size_t memory_budget_bytes = 0;
};

struct ResourceServices
{
    std::shared_ptr<IResourceLoaderRegistry> loaders;
    std::shared_ptr<IResourceManager> manager;
};

[[nodiscard]] foundation::Result<ResourceServices> CreateResourceServices(const ResourceOptions& options = {});
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Resources/resource_loader_registry.h`, `Epidemic/Runtime/Resources/resource_manager.h`.

### Состав типов и операции

#### `ResourceOptions` (строка заголовка 12)

Публичные поля и значения по умолчанию:

- `std::size_t memory_budget_bytes = 0`

#### `ResourceServices` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IResourceLoaderRegistry> loaders`
- `std::shared_ptr<IResourceManager> manager`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

