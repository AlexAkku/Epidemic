---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_services.h"
---

# Persistence: persistence_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 975 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_services.h>`.

## Объявления типов (лексический индекс)
`PersistenceOptions`, `PersistenceServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/persistence_backend.h"
#include "Epidemic/Runtime/Persistence/persistence_store.h"

#include <memory>

namespace epidemic::runtime
{
// Persistence backend/schema composition is fixed when services are created.
// Runtime object data remains mutable transactionally; there is no hot schema/migration registry in this module.
struct PersistenceOptions
{
    std::shared_ptr<IPersistenceBackend> backend;
    // CreatePersistenceServices rejects values outside the PersistenceDurability enum domain.
    PersistenceDurability durability = PersistenceDurability::MemoryOnly;
};

struct PersistenceServices
{
    std::shared_ptr<IPersistenceStore> store;
    std::shared_ptr<IPersistenceQuery> query;
};

[[nodiscard]] foundation::Result<PersistenceServices> CreatePersistenceServices(const PersistenceOptions& options = {});
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Persistence/persistence_backend.h`, `Epidemic/Runtime/Persistence/persistence_store.h`.

### Состав типов и операции

#### `PersistenceOptions` (строка заголовка 13)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IPersistenceBackend> backend`

#### `PersistenceServices` (строка заголовка 20)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IPersistenceStore> store`
- `std::shared_ptr<IPersistenceQuery> query`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

