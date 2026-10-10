---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_state.h"
---

# Assets: asset_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 743 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_state.h>`.

## Объявления типов (лексический индекс)
`AssetState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once


// Header for runtime contracts or module-local helpers. Comments document how each
// function participates in the module API and what state it observes or mutates.
namespace epidemic::runtime
{
enum class AssetState
{
    Unknown,
    Discovered,
    Indexed,
    Validated,
    Missing,
    Invalid,
    Deprecated,
};


[[nodiscard]] constexpr bool IsValidAssetState(AssetState value) noexcept
{
    switch (value)
    {
    case AssetState::Unknown:
    case AssetState::Discovered:
    case AssetState::Indexed:
    case AssetState::Validated:
    case AssetState::Missing:
    case AssetState::Invalid:
    case AssetState::Deprecated:
        return true;
    }
    return false;
}
}
```
