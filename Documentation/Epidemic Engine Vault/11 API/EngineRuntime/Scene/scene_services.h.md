---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h"
---

# Scene: scene_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 772 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_services.h>`.

## Объявления типов (лексический индекс)
`SceneOptions`, `SceneServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/scene_node_registry.h"
#include "Epidemic/Runtime/Scene/scene_query.h"
#include "Epidemic/Runtime/Scene/spatial_index.h"
#include "Epidemic/Runtime/Scene/transform_registry.h"

#include <memory>

namespace epidemic::runtime
{
struct SceneOptions
{
};

struct SceneServices
{
    std::shared_ptr<ISceneNodeRegistry> nodes;
    std::shared_ptr<ITransformRegistry> transforms;
    std::shared_ptr<ISpatialIndex> spatial;
    std::shared_ptr<ISceneQuery> queries;
    std::shared_ptr<ISceneSnapshotProvider> snapshots;
};

[[nodiscard]] foundation::Result<SceneServices> CreateSceneServices(const SceneOptions& options = {});
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Scene/scene_node_registry.h`, `Epidemic/Runtime/Scene/scene_query.h`, `Epidemic/Runtime/Scene/spatial_index.h`, `Epidemic/Runtime/Scene/transform_registry.h`.

### Состав типов и операции

#### `SceneServices` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<ISceneNodeRegistry> nodes`
- `std::shared_ptr<ITransformRegistry> transforms`
- `std::shared_ptr<ISpatialIndex> spatial`
- `std::shared_ptr<ISceneQuery> queries`
- `std::shared_ptr<ISceneSnapshotProvider> snapshots`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

