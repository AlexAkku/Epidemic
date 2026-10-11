---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog_writer.h"
---

# Assets: asset_catalog_writer.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog_writer.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 1356 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_catalog_writer.h>`.

## Объявления типов (лексический индекс)
`IAssetCatalogWriter`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Assets/asset_metadata.h"

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IAssetCatalogWriter
{
  public:
    virtual ~IAssetCatalogWriter() = default;

    // Preconditions: metadata identifiers/enums/location and bounded collection sizes
    // must satisfy the Assets contract. Semantic validation failures return Result failure
    // and leave the catalog unchanged. Process-wide allocation exhaustion (std::bad_alloc)
    // is intentionally not translated into a module-specific recoverable error and also
    // leaves the pre-call catalog observable state unchanged. Registration stores one
    // normalized detached copy. Metadata version is per asset, not a global catalog revision.
    [[nodiscard]] virtual foundation::Result<void> RegisterAsset(const AssetMetadata& metadata) = 0;
    // Seal is idempotent. Once sealed, successful/failed registration attempts cannot
    // reopen or otherwise mutate the catalog. Calling Seal() on an already sealed catalog
    // is a semantic no-op that still returns success.
    [[nodiscard]] virtual foundation::Result<void> Seal() = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Assets/asset_metadata.h`.

### Состав типов и операции

#### `IAssetCatalogWriter` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IAssetCatalogWriter() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

