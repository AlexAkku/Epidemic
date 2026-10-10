---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Input/include/Epidemic/Input/input_system.h"
---

# Input: input_system.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Input/include/Epidemic/Input/input_system.h`.

**Модуль:** [[04 EngineBase/Модуль Input|Input]].
**Размер исходного файла:** 1555 байт.
**Подключение:** `#include <Epidemic/Input/input_system.h>`.

## Объявления типов (лексический индекс)
`InputSystem`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Input/iinput_system.h>

#include <vector>

namespace epidemic::input
{
// This file declares the baseline InputSystem implementation.
// The system buffers raw PlatformEvent values until PublishSnapshot() normalizes them into
// keyboard/mouse state and transient InputEvent records.

class InputSystem final : public IInputSystem
{
  public:
    // Queues one raw platform event for the next publish pass.
    void QueuePlatformEvent(const epidemic::platform::PlatformEvent &event) override;

    // Queues a batch of raw platform events for the next publish pass.
    void QueuePlatformEvents(std::span<const epidemic::platform::PlatformEvent> events) override;

    // Normalizes queued platform events into the current snapshot and current event list.
    void PublishSnapshot() override;

    // Resets all cached state and queued events.
    void Reset() noexcept override;

    // Returns the most recently published input snapshot.
    [[nodiscard]] const InputSnapshot &CurrentSnapshot() const noexcept override;

    // Returns the transient input events produced by the most recent publish.
    [[nodiscard]] std::span<const InputEvent> CurrentEvents() const noexcept override;

  private:
    KeyboardState keyboard_state_{};
    MouseState mouse_state_{};
    InputSnapshot current_snapshot_{};
    std::vector<epidemic::platform::PlatformEvent> pending_platform_events_{};
    std::vector<InputEvent> current_events_{};
    std::uint64_t next_update_index_{};
};
}
```
