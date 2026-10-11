---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_services.h"
---

# Renderer: renderer_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Renderer|Renderer]].
**Размер исходного файла:** 1537 байт.
**Подключение:** `#include <Epidemic/Runtime/Renderer/renderer_services.h>`.

## Объявления типов (лексический индекс)
`RendererOptions`, `RendererDependencies`, `RendererServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Renderer/render_resource_bridge.h"
#include "Epidemic/Runtime/Renderer/render_scene.h"
#include "Epidemic/Runtime/Renderer/renderer_runtime.h"
#include "Epidemic/Runtime/Renderer/view_system.h"

#include <memory>

namespace epidemic::runtime::renderer
{
struct RendererOptions
{
    // Compatibility option only. CreateRendererServices is always strict and never injects mocks;
    // reference/mock behavior is available only through CreateMockRendererServices().
    bool enable_mock_dependencies = false;
};

struct RendererDependencies
{
    std::shared_ptr<IRenderResourceBridge> resource_bridge;
    std::shared_ptr<IRenderSceneSource> scene_source;
    std::shared_ptr<IRenderPoseSource> pose_source;
    std::shared_ptr<IRenderCommandSink> command_sink;
};

struct RendererServices
{
    std::shared_ptr<IRenderResourceBridge> resource_bridge;
    std::shared_ptr<IRenderSceneSource> scene_source;
    std::shared_ptr<IRenderPoseSource> pose_source;
    std::shared_ptr<IRenderCommandSink> command_sink;
    std::shared_ptr<IRenderScene> scene;
    std::shared_ptr<IViewSystem> views;
    std::shared_ptr<IRendererRuntime> runtime;
};

[[nodiscard]] foundation::Result<RendererServices> CreateRendererServices(const RendererOptions& options, RendererDependencies dependencies = {});
[[nodiscard]] foundation::Result<RendererServices> CreateMockRendererServices();
} // namespace epidemic::runtime::renderer
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Renderer/render_resource_bridge.h`, `Epidemic/Runtime/Renderer/render_scene.h`, `Epidemic/Runtime/Renderer/renderer_runtime.h`, `Epidemic/Runtime/Renderer/view_system.h`.

### Состав типов и операции

#### `RendererDependencies` (строка заголовка 20)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IRenderResourceBridge> resource_bridge`
- `std::shared_ptr<IRenderSceneSource> scene_source`
- `std::shared_ptr<IRenderPoseSource> pose_source`
- `std::shared_ptr<IRenderCommandSink> command_sink`

#### `RendererServices` (строка заголовка 28)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IRenderResourceBridge> resource_bridge`
- `std::shared_ptr<IRenderSceneSource> scene_source`
- `std::shared_ptr<IRenderPoseSource> pose_source`
- `std::shared_ptr<IRenderCommandSink> command_sink`
- `std::shared_ptr<IRenderScene> scene`
- `std::shared_ptr<IViewSystem> views`
- `std::shared_ptr<IRendererRuntime> runtime`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

