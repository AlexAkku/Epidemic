---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Input/include/Epidemic/Input/iinput_system.h"
---

# Input: iinput_system.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Input/include/Epidemic/Input/iinput_system.h`.

**Модуль:** [[04 EngineBase/Модуль Input|Input]].
**Размер исходного файла:** 1420 байт.
**Подключение:** `#include <Epidemic/Input/iinput_system.h>`.

## Объявления типов (лексический индекс)
`IInputSystem`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Input/input_event.h>
#include <Epidemic/Input/input_snapshot.h>
#include <Epidemic/Platform/platform_event.h>

#include <span>

namespace epidemic::input
{
// This file defines the normalized input pipeline interface used by EngineBase.
// Implementations ingest raw platform events, publish a frame-stable snapshot, and expose the
// transient event list generated during the most recent publish.

class IInputSystem
{
  public:
    virtual ~IInputSystem() = default;

    // Queues one raw platform event for later normalization.
    virtual void QueuePlatformEvent(const epidemic::platform::PlatformEvent &event) = 0;

    // Queues a batch of raw platform events for later normalization.
    virtual void QueuePlatformEvents(std::span<const epidemic::platform::PlatformEvent> events) = 0;

    // Consumes queued platform events and publishes a new snapshot plus transient event list.
    virtual void PublishSnapshot() = 0;

    // Clears all cached state and queued events.
    virtual void Reset() noexcept = 0;

    // Returns the most recently published frame snapshot.
    [[nodiscard]] virtual const InputSnapshot &CurrentSnapshot() const noexcept = 0;

    // Returns the transient events produced by the most recent PublishSnapshot() call.
    [[nodiscard]] virtual std::span<const InputEvent> CurrentEvents() const noexcept = 0;
};
}
```
