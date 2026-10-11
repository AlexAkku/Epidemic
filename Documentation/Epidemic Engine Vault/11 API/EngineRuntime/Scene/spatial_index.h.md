---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h"
---

# Scene: spatial_index.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 780 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/spatial_index.h>`.

## Объявления типов (лексический индекс)
`ISpatialIndex`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/bounds.h"
#include "Epidemic/Runtime/Scene/scene_node.h"

#include <optional>

namespace epidemic::runtime
{
class ISpatialIndex
{
  public:
    virtual ~ISpatialIndex() = default;

    [[nodiscard]] virtual foundation::Result<void> SetLocalBounds(SceneNodeId node, const Aabb& bounds) = 0;
    [[nodiscard]] virtual std::optional<Aabb> GetLocalBounds(SceneNodeId node) const = 0;
    [[nodiscard]] virtual std::optional<Aabb> GetWorldBounds(SceneNodeId node) const = 0;
    [[nodiscard]] virtual foundation::Result<void> MarkBoundsClean(SceneNodeId node) = 0;
    [[nodiscard]] virtual bool IsBoundsDirty(SceneNodeId node) const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Scene/bounds.h`, `Epidemic/Runtime/Scene/scene_node.h`.

### Состав типов и операции

#### `ISpatialIndex` (строка заголовка 11)

Публичные сигнатуры операций:

```cpp
virtual ~ISpatialIndex() = default;
[[nodiscard]] virtual foundation::Result<void> SetLocalBounds(SceneNodeId node, const Aabb& bounds) = 0;
[[nodiscard]] virtual std::optional<Aabb> GetLocalBounds(SceneNodeId node) const = 0;
[[nodiscard]] virtual std::optional<Aabb> GetWorldBounds(SceneNodeId node) const = 0;
[[nodiscard]] virtual foundation::Result<void> MarkBoundsClean(SceneNodeId node) = 0;
[[nodiscard]] virtual bool IsBoundsDirty(SceneNodeId node) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `SetLocalBounds` | `SceneRuntime::SetLocalBounds` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:655` |
| `GetLocalBounds` | `SceneRuntime::GetLocalBounds` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:692` |
| `GetWorldBounds` | `SceneRuntime::GetWorldBounds` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:702` |
| `MarkBoundsClean` | `SceneRuntime::MarkBoundsClean` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:707` |
| `IsBoundsDirty` | `SceneRuntime::IsBoundsDirty` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:718` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetLocalBounds`: `EngineRuntime/Scene/tests/scene_tests.cpp:306`
- `GetWorldBounds`: `EngineRuntime/Scene/tests/scene_tests.cpp:310`
- `IsBoundsDirty`: `EngineRuntime/Scene/tests/scene_tests.cpp:348`
- `MarkBoundsClean`: `EngineRuntime/Scene/tests/scene_tests.cpp:354`

