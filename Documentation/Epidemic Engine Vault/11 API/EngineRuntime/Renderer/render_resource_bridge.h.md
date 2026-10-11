---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h"
---

# Renderer: render_resource_bridge.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h`.

**Модуль:** [[05 EngineRuntime/Модуль Renderer|Renderer]].
**Размер исходного файла:** 3083 байт.
**Подключение:** `#include <Epidemic/Runtime/Renderer/render_resource_bridge.h>`.

## Объявления типов (лексический индекс)
`RenderTransformSnapshot`, `RenderPoseBuffer`, `IRenderPoseSource`, `IRenderResourceBridge`, `IRenderSceneSource`, `RenderProxySubmission`, `RenderFrameContext`, `IRenderCommandSink`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/spatial.h"
#include "Epidemic/Runtime/Renderer/render_types.h"

#include <memory>
#include <vector>

namespace epidemic::runtime::renderer
{
struct RenderTransformSnapshot
{
    RenderTransformId node{};
    Transform world_transform{};
    std::uint64_t revision = 0;
};

struct RenderPoseBuffer
{
    RuntimeObjectId owner{};
    std::vector<Transform> bone_transforms{};
    std::uint64_t revision = 0;
};

class IRenderPoseSource
{
  public:
    virtual ~IRenderPoseSource() = default;

    // Query boundary: Result failure or exception does not publish renderer state.
    // A successful null pointer means that the owner has no pose and is rendered statically.
    [[nodiscard]] virtual foundation::Result<std::shared_ptr<const RenderPoseBuffer>>
        GetPose(RuntimeObjectId owner) const = 0;
};

class IRenderResourceBridge
{
  public:
    virtual ~IRenderResourceBridge() = default;

    // Ownership boundary: Acquire failure/exception means no hold was acquired.
    // Release failure/exception means the existing hold remains owned by the caller and is retryable.
    [[nodiscard]] virtual foundation::Result<void> AcquirePayloads(ResourceId mesh, ResourceId material) = 0;
    [[nodiscard]] virtual foundation::Result<void> ReleasePayloads(ResourceId mesh, ResourceId material) = 0;
    [[nodiscard]] virtual foundation::Result<RenderResourcePayloads> GetPayloads(ResourceId mesh, ResourceId material) const = 0;
};

class IRenderSceneSource
{
  public:
    virtual ~IRenderSceneSource() = default;

    [[nodiscard]] virtual foundation::Result<RenderTransformSnapshot> GetTransformSnapshot(RenderTransformId node) const = 0;
};

struct RenderProxySubmission
{
    RenderProxyId proxy{};
    RenderTransformSnapshot transform{};
    RenderResourcePayloads payloads{};
    std::shared_ptr<const RenderPoseBuffer> pose{};
    RenderLayer layer = RenderLayer::Opaque;
    RenderProxyVisibility visibility = RenderProxyVisibility::Visible;
};

struct RenderFrameContext
{
    ViewId view{};
    ViewDesc desc{};
    RenderTransformSnapshot transform{};
};

class IRenderCommandSink
{
  public:
    virtual ~IRenderCommandSink() = default;

    // BeginFrame failure/exception means no backend frame was opened. After BeginFrame success,
    // SubmitProxy/EndFrame failure or exception may leave that frame open. AbortFrame success
    // closes/discards it. AbortFrame failure/exception retains backend-frame ownership and must
    // be safely retryable before a later BeginFrame is accepted.
    [[nodiscard]] virtual foundation::Result<void> BeginFrame(const RenderFrameContext& context) = 0;
    [[nodiscard]] virtual foundation::Result<void> SubmitProxy(const RenderProxySubmission& submission) = 0;
    [[nodiscard]] virtual foundation::Result<void> EndFrame() = 0;
    [[nodiscard]] virtual foundation::Result<void> AbortFrame() = 0;
};
} // namespace epidemic::runtime::renderer
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Foundation/spatial.h`, `Epidemic/Runtime/Renderer/render_types.h`.

### Состав типов и операции

#### `RenderTransformSnapshot` (строка заголовка 12)

Публичные поля и значения по умолчанию:

- `std::uint64_t revision = 0`

#### `RenderPoseBuffer` (строка заголовка 19)

Публичные поля и значения по умолчанию:

- `std::uint64_t revision = 0`

#### `IRenderPoseSource` (строка заголовка 26)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderPoseSource() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IRenderResourceBridge` (строка заголовка 37)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderResourceBridge() = default;
[[nodiscard]] virtual foundation::Result<void> ReleasePayloads(ResourceId mesh, ResourceId material) = 0;
[[nodiscard]] virtual foundation::Result<RenderResourcePayloads> GetPayloads(ResourceId mesh, ResourceId material) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IRenderSceneSource` (строка заголовка 49)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderSceneSource() = default;
[[nodiscard]] virtual foundation::Result<RenderTransformSnapshot> GetTransformSnapshot(RenderTransformId node) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `RenderProxySubmission` (строка заголовка 57)

Публичные поля и значения по умолчанию:

- `RenderLayer layer = RenderLayer::Opaque`
- `RenderProxyVisibility visibility = RenderProxyVisibility::Visible`

#### `IRenderCommandSink` (строка заголовка 74)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderCommandSink() = default;
[[nodiscard]] virtual foundation::Result<void> SubmitProxy(const RenderProxySubmission& submission) = 0;
[[nodiscard]] virtual foundation::Result<void> EndFrame() = 0;
[[nodiscard]] virtual foundation::Result<void> AbortFrame() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

