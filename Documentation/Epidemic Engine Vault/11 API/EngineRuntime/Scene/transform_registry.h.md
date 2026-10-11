---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h"
---

# Scene: transform_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1269 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/transform_registry.h>`.

## Объявления типов (лексический индекс)
`WorldTransformWriteMode`, `ITransformRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/scene_node.h"
#include "Epidemic/Runtime/Scene/transform.h"

#include <optional>

namespace epidemic::runtime
{
enum class WorldTransformWriteMode
{
    RejectNonInvertibleParent,
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ITransformRegistry
{
  public:
    virtual ~ITransformRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> SetLocalTransform(SceneNodeId node, const Transform& transform) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetWorldTransform(
        SceneNodeId node,
        const Transform& transform,
        WorldTransformWriteMode mode = WorldTransformWriteMode::RejectNonInvertibleParent) = 0;
    [[nodiscard]] virtual std::optional<Transform> GetLocalTransform(SceneNodeId node) const = 0;
    [[nodiscard]] virtual std::optional<Transform> GetWorldTransform(SceneNodeId node) const = 0;
    [[nodiscard]] virtual foundation::Result<void> MarkTransformClean(SceneNodeId node) = 0;
    [[nodiscard]] virtual bool IsTransformDirty(SceneNodeId node) const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Scene/scene_node.h`, `Epidemic/Runtime/Scene/transform.h`.

### Перечисления и допустимые категории

- `WorldTransformWriteMode`: `RejectNonInvertibleParent`

### Состав типов и операции

#### `ITransformRegistry` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
virtual ~ITransformRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> SetLocalTransform(SceneNodeId node, const Transform& transform) = 0;
[[nodiscard]] virtual foundation::Result<void> SetWorldTransform( SceneNodeId node, const Transform& transform, WorldTransformWriteMode mode = WorldTransformWriteMode::RejectNonInvertibleParent) = 0;
[[nodiscard]] virtual std::optional<Transform> GetLocalTransform(SceneNodeId node) const = 0;
[[nodiscard]] virtual std::optional<Transform> GetWorldTransform(SceneNodeId node) const = 0;
[[nodiscard]] virtual foundation::Result<void> MarkTransformClean(SceneNodeId node) = 0;
[[nodiscard]] virtual bool IsTransformDirty(SceneNodeId node) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `SetLocalTransform` | `SceneRuntime::SetLocalTransform` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:527` |
| `SetWorldTransform` | `SceneRuntime::SetWorldTransform` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:567` |
| `GetLocalTransform` | `SceneRuntime::GetLocalTransform` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:627` |
| `GetWorldTransform` | `SceneRuntime::GetWorldTransform` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:633` |
| `MarkTransformClean` | `SceneRuntime::MarkTransformClean` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:638` |
| `IsTransformDirty` | `SceneRuntime::IsTransformDirty` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:649` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetLocalTransform`: `EngineRuntime/Scene/tests/scene_tests.cpp:135`
- `GetLocalTransform`: `EngineRuntime/Scene/tests/scene_tests.cpp:140`
- `GetWorldTransform`: `EngineRuntime/Scene/tests/scene_tests.cpp:141`
- `SetWorldTransform`: `EngineRuntime/Scene/tests/scene_tests.cpp:157`
- `IsTransformDirty`: `EngineRuntime/Scene/tests/scene_tests.cpp:348`
- `MarkTransformClean`: `EngineRuntime/Scene/tests/scene_tests.cpp:353`

