---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_services.h"
---

# Assets: asset_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 888 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_services.h>`.

## Объявления типов (лексический индекс)
`AssetsOptions`, `AssetServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Assets/asset_catalog.h"
#include "Epidemic/Runtime/Assets/asset_catalog_writer.h"
#include "Epidemic/Runtime/Assets/asset_location_resolver.h"

#include <memory>

namespace epidemic::runtime
{
struct AssetsOptions
{
};

struct AssetServices
{
    std::shared_ptr<IAssetCatalog> catalog;
    std::shared_ptr<IAssetCatalogWriter> writer;
    std::shared_ptr<IAssetLocationResolver> location_resolver;
};

// Creates one in-memory catalog shared through read, write and location-resolver
// interfaces. No filesystem/package/resource side effect is performed here.
// Process-wide allocation failure may propagate according to the engine policy.
[[nodiscard]] foundation::Result<AssetServices> CreateAssetServices(const AssetsOptions& options = {});
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Assets/asset_catalog.h`, `Epidemic/Runtime/Assets/asset_catalog_writer.h`, `Epidemic/Runtime/Assets/asset_location_resolver.h`.

### Состав типов и операции

#### `AssetServices` (строка заголовка 16)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IAssetCatalog> catalog`
- `std::shared_ptr<IAssetCatalogWriter> writer`
- `std::shared_ptr<IAssetLocationResolver> location_resolver`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

