---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog.h"
---

# Assets: asset_catalog.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_catalog.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 2082 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_catalog.h>`.

## Объявления типов (лексический индекс)
`IAssetCatalog`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"

#include "Epidemic/Runtime/Assets/asset_dependency_manifest.h"
#include "Epidemic/Runtime/Assets/asset_metadata.h"

#include <optional>
#include <vector>

namespace epidemic::runtime
{
// Read-only asset catalog contract. Mutation is intentionally split into IAssetCatalogWriter
// so runtime consumers do not need write access after bootstrap. Queries perform no hidden
// mutation; concurrent read/write remains outside the contract.
class IAssetCatalog
{
  public:
    virtual ~IAssetCatalog() = default;

    // Returns a detached metadata copy. Invalid and missing ids both observe as
    // no value; mutating the returned copy never mutates the catalog.
    [[nodiscard]] virtual std::optional<AssetMetadata> FindById(AssetId id) const = 0;

    // Returns detached copies sorted by AssetId::Raw(). Invalid selectors return
    // an empty result. Allocation failure may propagate without mutating catalog state.
    [[nodiscard]] virtual std::vector<AssetMetadata> FindByType(AssetType type) const = 0;
    [[nodiscard]] virtual std::vector<AssetMetadata> FindByTag(foundation::StringId tag) const = 0;

    // Invalid ids are never contained.
    [[nodiscard]] virtual bool Contains(AssetId id) const = 0;

    // Reports the monotonic catalog lifecycle state. Seal cannot be undone.
    [[nodiscard]] virtual bool IsSealed() const = 0;

    // Builds the transitive dependency set without mutating the catalog. Required
    // missing dependencies fail the query; optional missing dependencies remain in
    // the manifest. Diamond duplicates are collapsed by AssetId and requiredness is
    // the logical OR of every incoming edge. Output is sorted by AssetId::Raw().
    // Invalid root and missing root are distinct controlled failures. Allocation
    // failure may propagate and leaves the catalog unchanged.
    [[nodiscard]] virtual foundation::Result<AssetDependencyManifest> BuildDependencyManifest(AssetId root) const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Assets/asset_dependency_manifest.h`, `Epidemic/Runtime/Assets/asset_metadata.h`.

### Состав типов и операции

#### `IAssetCatalog` (строка заголовка 16)

Публичные поля и значения по умолчанию:

- `optional missing dependencies remain in`

Публичные сигнатуры операций:

```cpp
virtual ~IAssetCatalog() = default;
mutating the returned copy never mutates the catalog. [[nodiscard]] virtual std::optional<AssetMetadata> FindById(AssetId id) const = 0;
[[nodiscard]] virtual std::vector<AssetMetadata> FindByTag(foundation::StringId tag) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindById` | `InMemoryAssetCatalog::FindById` | `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:204` |
| `FindByTag` | `InMemoryAssetCatalog::FindByTag` | `EngineRuntime/Assets/src/in_memory_asset_catalog.cpp:243` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `FindById`: `EngineRuntime/Assets/tests/assets_tests.cpp:84`
- `FindByTag`: `EngineRuntime/Assets/tests/assets_tests.cpp:261`

