---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_services.h"
---

# World: world_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 1098 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_services.h>`.

## Объявления типов (лексический индекс)
`WorldOptions`, `WorldServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/World/chunk.h"
#include "Epidemic/Runtime/World/object_materialization.h"
#include "Epidemic/Runtime/World/region.h"
#include "Epidemic/Runtime/World/world_object_registry.h"
#include "Epidemic/Runtime/World/world_query.h"

#include <memory>

namespace epidemic::runtime
{
struct WorldOptions
{
};

using IWorldObjectQuery = IWorldQuery;
using IWorldObjectWriter = IWorldObjectRegistry;
using IWorldMaterializationRuntime = IObjectMaterializer;
using IWorldDemotionAuthority = IDemotionCommitAuthority;

struct WorldServices
{
    std::shared_ptr<IRegionRegistry> regions;
    std::shared_ptr<IChunkRegistry> chunks;
    std::shared_ptr<IWorldObjectQuery> query;
    std::shared_ptr<IWorldObjectWriter> writer;
    std::shared_ptr<IWorldMaterializationRuntime> materialization;
    std::shared_ptr<IWorldDemotionAuthority> demotion_authority;
};

[[nodiscard]] foundation::Result<WorldServices> CreateWorldServices(const WorldOptions& options = {});
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/World/chunk.h`, `Epidemic/Runtime/World/object_materialization.h`, `Epidemic/Runtime/World/region.h`, `Epidemic/Runtime/World/world_object_registry.h`, `Epidemic/Runtime/World/world_query.h`.

### Состав типов и операции

#### `WorldServices` (строка заголовка 23)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IRegionRegistry> regions`
- `std::shared_ptr<IChunkRegistry> chunks`
- `std::shared_ptr<IWorldObjectQuery> query`
- `std::shared_ptr<IWorldObjectWriter> writer`
- `std::shared_ptr<IWorldMaterializationRuntime> materialization`
- `std::shared_ptr<IWorldDemotionAuthority> demotion_authority`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

