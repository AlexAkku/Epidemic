---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Diagnostics/include/Epidemic/Diagnostics/counters.h"
---

# Diagnostics: counters.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/counters.h`.

**Модуль:** [[04 EngineBase/Модуль Diagnostics|Diagnostics]].
**Размер исходного файла:** 4032 байт.
**Подключение:** `#include <Epidemic/Diagnostics/counters.h>`.

## Объявления типов (лексический индекс)
`CounterId`, `DiagnosticsCounters`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace epidemic::diagnostics
{
// Process-wide numeric diagnostics counters. Arithmetic saturates instead of wrapping so observability
// cannot publish nonsensical values after an integer boundary is reached.
enum class CounterId : std::uint8_t
{
    Frames,
    TasksScheduled,
    TasksCompleted,
    WorkerCount,
    CurrentFrameIndex,
    FrameTimeMicros,
    PlatformPumpTimeMicros,
    InputUpdateTimeMicros,
    ModuleTickTimeMicros,
    PresentTimeMicros,
    MainThreadTasksExecuted,
    EventBusQueuedEvents,
    EventBusDispatchedEvents,
    PlatformEventsThisFrame,
    InputEventsThisFrame,
    MemoryUsedBytes,
    MemoryPeakBytes,
    RhiFrames,
    RhiPresents,
    RhiResizeCount,
    Count,
};

[[nodiscard]] constexpr std::size_t CounterCount() noexcept
{
    return static_cast<std::size_t>(CounterId::Count);
}

[[nodiscard]] constexpr bool IsValidCounterId(CounterId counter_id) noexcept
{
    return static_cast<std::size_t>(counter_id) < CounterCount();
}

[[nodiscard]] inline std::string_view ToString(CounterId counter_id) noexcept
{
    switch (counter_id)
    {
    case CounterId::Frames:
        return "frames";
    case CounterId::TasksScheduled:
        return "tasks_scheduled";
    case CounterId::TasksCompleted:
        return "tasks_completed";
    case CounterId::WorkerCount:
        return "worker_count";
    case CounterId::CurrentFrameIndex:
        return "current_frame_index";
    case CounterId::FrameTimeMicros:
        return "frame_time_micros";
    case CounterId::PlatformPumpTimeMicros:
        return "platform_pump_time_micros";
    case CounterId::InputUpdateTimeMicros:
        return "input_update_time_micros";
    case CounterId::ModuleTickTimeMicros:
        return "module_tick_time_micros";
    case CounterId::PresentTimeMicros:
        return "present_time_micros";
    case CounterId::MainThreadTasksExecuted:
        return "main_thread_tasks_executed";
    case CounterId::EventBusQueuedEvents:
        return "event_bus_queued_events";
    case CounterId::EventBusDispatchedEvents:
        return "event_bus_dispatched_events";
    case CounterId::PlatformEventsThisFrame:
        return "platform_events_this_frame";
    case CounterId::InputEventsThisFrame:
        return "input_events_this_frame";
    case CounterId::MemoryUsedBytes:
        return "memory_used_bytes";
    case CounterId::MemoryPeakBytes:
        return "memory_peak_bytes";
    case CounterId::RhiFrames:
        return "rhi_frames";
    case CounterId::RhiPresents:
        return "rhi_presents";
    case CounterId::RhiResizeCount:
        return "rhi_resize_count";
    case CounterId::Count:
        break;
    }

    return "unknown";
}

class DiagnosticsCounters
{
  public:
    // Adds delta with signed saturation. Invalid CounterId values are rejected as a no-op.
    void Increment(CounterId counter_id, std::int64_t delta = 1) noexcept;

    // Subtracts delta with signed saturation. Invalid CounterId values are rejected as a no-op.
    void Decrement(CounterId counter_id, std::int64_t delta = 1) noexcept;

    // Replaces one valid counter with an exact value. Invalid CounterId values are rejected as a no-op.
    void Set(CounterId counter_id, std::int64_t value) noexcept;

    // Returns a valid counter value, or zero for an invalid CounterId.
    [[nodiscard]] std::int64_t Get(CounterId counter_id) const noexcept;

    // Resets every valid counter to zero.
    void Reset() noexcept;

  private:
    [[nodiscard]] static constexpr std::size_t ToIndex(CounterId counter_id) noexcept
    {
        return static_cast<std::size_t>(counter_id);
    }

    std::array<std::atomic<std::int64_t>, CounterCount()> values_{};
};

[[nodiscard]] DiagnosticsCounters &GlobalCounters() noexcept;
} // namespace epidemic::diagnostics
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `CounterId`: `Frames`, `TasksScheduled`, `TasksCompleted`, `WorkerCount`, `CurrentFrameIndex`, `FrameTimeMicros`, `PlatformPumpTimeMicros`, `InputUpdateTimeMicros`, `ModuleTickTimeMicros`, `PresentTimeMicros`, `MainThreadTasksExecuted`, `EventBusQueuedEvents`, `EventBusDispatchedEvents`, `PlatformEventsThisFrame`, `InputEventsThisFrame`, `MemoryUsedBytes`, `MemoryPeakBytes`, `RhiFrames`, `RhiPresents`, `RhiResizeCount`, `Count`

### Состав типов и операции

#### `DiagnosticsCounters` (строка заголовка 99)

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

