---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location.h"
---

# Assets: asset_location.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_location.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 1816 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_location.h>`.

## Объявления типов (лексический индекс)
`AssetLocationKind`, `AssetLocation`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"

#include <string>
#include <utility>

namespace epidemic::runtime
{
enum class AssetLocationKind
{
    FilePath,
    PackageEntry,
    VirtualPath,
    Generated,
};

[[nodiscard]] constexpr bool IsValidAssetLocationKind(AssetLocationKind value) noexcept
{
    switch (value)
    {
    case AssetLocationKind::FilePath:
    case AssetLocationKind::PackageEntry:
    case AssetLocationKind::VirtualPath:
    case AssetLocationKind::Generated:
        return true;
    }
    return false;
}

struct AssetLocation
{
    AssetLocationKind kind = AssetLocationKind::FilePath;
    foundation::StringId mount_id{};
    std::string path;
    foundation::StringId generator_id{};

    AssetLocation() = default;
    AssetLocation(AssetLocationKind location_kind, std::string location_path)
        : kind(location_kind), path(std::move(location_path))
    {
    }

    [[nodiscard]] bool Empty() const noexcept { return path.empty(); }
    [[nodiscard]] bool operator==(const AssetLocation&) const = default;
};

// Canonicalizes only lexical separators and dot segments. It performs no I/O
// and does not make an invalid location valid; call IsValidAssetLocation() at
// authoritative mutation boundaries.
[[nodiscard]] AssetLocation CanonicalizeAssetLocation(AssetLocation location);

// A valid location is a non-empty engine-root-relative path with no embedded
// NUL and no host-rooted, UNC/device or Windows drive-relative form. Package
// and virtual locations require mount_id. Generated locations require
// generator_id. Extra fields are metadata only and do not imply I/O ownership.
[[nodiscard]] bool IsValidAssetLocation(const AssetLocation& location) noexcept;
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`.

### Перечисления и допустимые категории

- `AssetLocationKind`: `FilePath`, `PackageEntry`, `VirtualPath`, `Generated`

### Состав типов и операции

#### `AssetLocation` (строка заголовка 31)

Публичные поля и значения по умолчанию:

- `AssetLocationKind kind = AssetLocationKind::FilePath`
- `std::string path`

Публичные сигнатуры операций:

```cpp
AssetLocation() = default;
AssetLocation(AssetLocationKind location_kind, std::string location_path) : kind(location_kind), path(std::move(location_path));
[[nodiscard]] bool Empty() const noexcept;
[[nodiscard]] bool operator==(const AssetLocation&) const = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Empty`: `EngineRuntime/Assets/tests/assets_tests.cpp:73`

