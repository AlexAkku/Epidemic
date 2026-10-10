---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_metadata.h"
---

# Assets: asset_metadata.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_metadata.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 1649 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_metadata.h>`.

## Объявления типов (лексический индекс)
`AssetDependency`, `AssetMetadata`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Assets/asset_location.h"
#include "Epidemic/Runtime/Assets/asset_state.h"
#include "Epidemic/Runtime/Assets/asset_type.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

// Header for runtime contracts or module-local helpers. Comments document how each
// function participates in the module API and what state it observes or mutates.

#include <cstdint>
#include <cstddef>
#include <vector>

namespace epidemic::runtime
{
// Public mutation limits keep caller-controlled metadata bounded before the catalog
// performs normalization/copies. std::bad_alloc outside these validated limits remains
// a process-level allocation failure; Assets does not invent a module-local OOM policy.
inline constexpr std::size_t kMaxAssetPathBytes = 4096;
inline constexpr std::size_t kMaxAssetDependencies = 4096;
inline constexpr std::size_t kMaxAssetTags = 1024;

struct AssetDependency
{
    AssetId asset_id{};
    bool required = true;

    [[nodiscard]] constexpr bool operator==(const AssetDependency&) const noexcept = default;
};

struct AssetMetadata
{
    AssetId id{};
    AssetType type{};
    AssetLocation location{};
    AssetState state = AssetState::Unknown;
    std::vector<AssetDependency> dependencies;
    std::vector<foundation::StringId> tags;
    std::uint64_t content_hash = 0;
    // Per-asset metadata/content schema version. This is not a catalog-global
    // revision counter and does not change when unrelated assets are registered.
    // Version zero is reserved as invalid for registered metadata.
    std::uint32_t version = 0;
};
}
```
