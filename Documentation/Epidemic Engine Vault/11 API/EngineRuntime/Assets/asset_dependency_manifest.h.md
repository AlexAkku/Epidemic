---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_dependency_manifest.h"
---

# Assets: asset_dependency_manifest.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_dependency_manifest.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 511 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_dependency_manifest.h>`.

## Объявления типов (лексический индекс)
`AssetDependencyManifest`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Assets/asset_metadata.h"

#include <vector>

namespace epidemic::runtime
{
struct AssetDependencyManifest
{
    AssetId root{};
    // Deterministically sorted transitive dependencies. Each AssetId appears at
    // most once. required is true when any reachable dependency edge requires it.
    // Optional missing dependencies remain present for diagnostics/planning.
    std::vector<AssetDependency> dependencies;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Assets/asset_metadata.h`.

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

