---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h"
---

# Renderer: render_types.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`.

**Модуль:** [[05 EngineRuntime/Модуль Renderer|Renderer]].
**Размер исходного файла:** 3676 байт.
**Подключение:** `#include <Epidemic/Runtime/Renderer/render_types.h>`.

## Объявления типов (лексический индекс)
`RenderProxyId`, `ViewId`, `IRenderMeshResource`, `IRenderMaterialResource`, `RenderProxyLifecycle`, `RenderProxyReadiness`, `RenderProxyVisibility`, `RenderProxyDirtyFlags`, `RenderLayer`, `RenderFrameState`, `ViewLifecycle`, `RenderProxyDesc`, `ViewDesc`, `RenderResourcePayloads`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <cstdint>
#include <functional>
#include <memory>

namespace epidemic::runtime::renderer
{
struct RenderProxyId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const RenderProxyId&) const noexcept = default;
};

struct ViewId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const ViewId&) const noexcept = default;
};

using RenderTransformId = RuntimeObjectId;

class IRenderMeshResource
{
  public:
    virtual ~IRenderMeshResource() = default;
};

class IRenderMaterialResource
{
  public:
    virtual ~IRenderMaterialResource() = default;
};

using RenderMeshResourcePtr = std::shared_ptr<const IRenderMeshResource>;
using RenderMaterialResourcePtr = std::shared_ptr<const IRenderMaterialResource>;

enum class RenderProxyLifecycle
{
    Unregistered,
    Registered,
    DestroyPending,
    Destroyed,
};

enum class RenderProxyReadiness
{
    MissingResources,
    Loading,
    Failed,
    Ready,
};

enum class RenderProxyVisibility
{
    Hidden,
    Visible,
    Culled,
};

enum class RenderProxyDirtyFlags : std::uint32_t
{
    None = 0,
    Transform = 1u << 0,
    Material = 1u << 1,
    Visibility = 1u << 2,
};

using RenderProxyDirtyMask = std::uint32_t;

[[nodiscard]] constexpr RenderProxyDirtyMask ToRenderDirtyMask(RenderProxyDirtyFlags flag) noexcept
{
    return static_cast<RenderProxyDirtyMask>(flag);
}

[[nodiscard]] constexpr bool HasRenderDirtyFlag(RenderProxyDirtyMask mask, RenderProxyDirtyFlags flag) noexcept
{
    return (mask & ToRenderDirtyMask(flag)) != 0u;
}

[[nodiscard]] constexpr RenderProxyDirtyMask AddRenderDirtyFlag(RenderProxyDirtyMask mask, RenderProxyDirtyFlags flag) noexcept
{
    return mask | ToRenderDirtyMask(flag);
}

[[nodiscard]] constexpr RenderProxyDirtyMask ClearRenderDirtyFlag(RenderProxyDirtyMask mask, RenderProxyDirtyFlags flag) noexcept
{
    return mask & ~ToRenderDirtyMask(flag);
}

enum class RenderLayer
{
    Opaque,
    Transparent,
    Sky,
    Water,
    Debug
};

enum class RenderFrameState
{
    NotPrepared,
    Preparing,
    ReadyToRender,
    Rendering,
    Submitted,
    Failed
};

enum class ViewLifecycle
{
    Active,
    DestroyPending,
    Destroyed,
};

struct RenderProxyDesc
{
    RuntimeObjectId owner{};
    ResourceId mesh{};
    ResourceId material{};
    RenderTransformId transform_node{};
    RenderLayer layer = RenderLayer::Opaque;
    RenderProxyVisibility visibility = RenderProxyVisibility::Visible;
};

struct ViewDesc
{
    RenderTransformId transform_node{};
    float vertical_fov = 60.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
};

struct RenderResourcePayloads
{
    RenderMeshResourcePtr mesh{};
    RenderMaterialResourcePtr material{};
};
} // namespace epidemic::runtime::renderer

namespace std
{
template <> struct hash<epidemic::runtime::renderer::RenderProxyId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::renderer::RenderProxyId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::renderer::ViewId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::renderer::ViewId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Перечисления и допустимые категории

- `RenderProxyLifecycle`: `Unregistered`, `Registered`, `DestroyPending`, `Destroyed`
- `RenderProxyReadiness`: `MissingResources`, `Loading`, `Failed`, `Ready`
- `RenderProxyVisibility`: `Hidden`, `Visible`, `Culled`
- `RenderProxyDirtyFlags`: `None`, `Transform`, `Material`, `Visibility`
- `RenderLayer`: `Opaque`, `Transparent`, `Sky`, `Water`, `Debug`
- `RenderFrameState`: `NotPrepared`, `Preparing`, `ReadyToRender`, `Rendering`, `Submitted`, `Failed`
- `ViewLifecycle`: `Active`, `DestroyPending`, `Destroyed`

### Состав типов и операции

#### `RenderProxyId` (строка заголовка 11)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const RenderProxyId&) const noexcept = default;
```

#### `ViewId` (строка заголовка 19)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ViewId&) const noexcept = default;
```

#### `IRenderMeshResource` (строка заголовка 29)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderMeshResource() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IRenderMaterialResource` (строка заголовка 35)

Публичные сигнатуры операций:

```cpp
virtual ~IRenderMaterialResource() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `RenderProxyDesc` (строка заголовка 123)

Публичные поля и значения по умолчанию:

- `RenderLayer layer = RenderLayer::Opaque`
- `RenderProxyVisibility visibility = RenderProxyVisibility::Visible`

#### `ViewDesc` (строка заголовка 133)

Публичные поля и значения по умолчанию:

- `float vertical_fov = 60.0f`
- `float near_plane = 0.1f`
- `float far_plane = 1000.0f`

#### `hash` (строка заголовка 150)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::renderer::RenderProxyId value) const noexcept;
```

#### `hash` (строка заголовка 158)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::renderer::ViewId value) const noexcept;
```

### Интеграция и безопасность вызовов

- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

