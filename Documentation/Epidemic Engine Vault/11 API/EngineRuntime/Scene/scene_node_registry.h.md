---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h"
---

# Scene: scene_node_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1600 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_node_registry.h>`.

## Объявления типов (лексический индекс)
`ReparentMode`, `ISceneNodeRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Scene/scene_node.h"

#include <optional>
#include <vector>

namespace epidemic::runtime
{
enum class ReparentMode
{
    KeepWorld,
    KeepLocal,
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ISceneNodeRegistry
{
  public:
    virtual ~ISceneNodeRegistry() = default;

    [[nodiscard]] virtual foundation::Result<SceneNodeId> CreateNode() = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyNode(SceneNodeId node) = 0;
    [[nodiscard]] virtual bool Exists(SceneNodeId node) const = 0;
    [[nodiscard]] virtual std::optional<SceneNode> GetNode(SceneNodeId node) const = 0;
    [[nodiscard]] virtual foundation::Result<void> AttachNode(SceneNodeId child, SceneNodeId parent, ReparentMode mode = ReparentMode::KeepWorld) = 0;
    [[nodiscard]] virtual foundation::Result<void> DetachNode(SceneNodeId child, ReparentMode mode = ReparentMode::KeepWorld) = 0;
    [[nodiscard]] virtual std::optional<SceneNodeId> GetParent(SceneNodeId child) const = 0;
    [[nodiscard]] virtual std::vector<SceneNodeId> GetChildren(SceneNodeId parent) const = 0;
    [[nodiscard]] virtual foundation::Result<void> SetMobility(SceneNodeId node, SceneMobility mobility) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetVisibility(SceneNodeId node, SceneVisibilityState visibility) = 0;
    [[nodiscard]] virtual std::uint64_t GetRevision() const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Scene/scene_node.h`.

### Перечисления и допустимые категории

- `ReparentMode`: `KeepWorld`, `KeepLocal`

### Состав типов и операции

#### `ISceneNodeRegistry` (строка заголовка 19)

Публичные сигнатуры операций:

```cpp
virtual ~ISceneNodeRegistry() = default;
[[nodiscard]] virtual foundation::Result<SceneNodeId> CreateNode() = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyNode(SceneNodeId node) = 0;
[[nodiscard]] virtual bool Exists(SceneNodeId node) const = 0;
[[nodiscard]] virtual std::optional<SceneNode> GetNode(SceneNodeId node) const = 0;
[[nodiscard]] virtual foundation::Result<void> AttachNode(SceneNodeId child, SceneNodeId parent, ReparentMode mode = ReparentMode::KeepWorld) = 0;
[[nodiscard]] virtual foundation::Result<void> DetachNode(SceneNodeId child, ReparentMode mode = ReparentMode::KeepWorld) = 0;
[[nodiscard]] virtual std::optional<SceneNodeId> GetParent(SceneNodeId child) const = 0;
[[nodiscard]] virtual std::vector<SceneNodeId> GetChildren(SceneNodeId parent) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetMobility(SceneNodeId node, SceneMobility mobility) = 0;
[[nodiscard]] virtual foundation::Result<void> SetVisibility(SceneNodeId node, SceneVisibilityState visibility) = 0;
[[nodiscard]] virtual std::uint64_t GetRevision() const = 0;
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
| `CreateNode` | `SceneRuntime::CreateNode` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:141` |
| `DestroyNode` | `SceneRuntime::DestroyNode` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:181` |
| `Exists` | `SceneRuntime::Exists` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:255` |
| `GetNode` | `SceneRuntime::GetNode` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:260` |
| `AttachNode` | `SceneRuntime::AttachNode` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:266` |
| `DetachNode` | `SceneRuntime::DetachNode` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:381` |
| `GetParent` | `SceneRuntime::GetParent` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:448` |
| `GetChildren` | `SceneRuntime::GetChildren` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:458` |
| `SetMobility` | `SceneRuntime::SetMobility` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:470` |
| `SetVisibility` | `SceneRuntime::SetVisibility` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:496` |
| `GetRevision` | `SceneRuntime::GetRevision` | `EngineRuntime/Scene/src/scene_runtime_impl.cpp:522` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `CreateNode`: `EngineRuntime/Scene/tests/scene_tests.cpp:88`
- `GetRevision`: `EngineRuntime/Scene/tests/scene_tests.cpp:94`
- `DestroyNode`: `EngineRuntime/Scene/tests/scene_tests.cpp:95`
- `Exists`: `EngineRuntime/Scene/tests/scene_tests.cpp:96`
- `AttachNode`: `EngineRuntime/Scene/tests/scene_tests.cpp:111`
- `GetChildren`: `EngineRuntime/Scene/tests/scene_tests.cpp:114`
- `GetParent`: `EngineRuntime/Scene/tests/scene_tests.cpp:115`
- `DetachNode`: `EngineRuntime/Scene/tests/scene_tests.cpp:116`
- `SetVisibility`: `EngineRuntime/Scene/tests/scene_tests.cpp:395`
- `GetNode`: `EngineRuntime/Scene/tests/scene_tests.cpp:493`
- `SetMobility`: `EngineRuntime/Scene/tests/scene_tests.cpp:637`

