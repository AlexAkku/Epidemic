---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h"
---

# Time: time_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Time|Time]].
**Размер исходного файла:** 3121 байт.
**Подключение:** `#include <Epidemic/Runtime/Time/time_runtime.h>`.

## Объявления типов (лексический индекс)
`TimeAdvanceResult`, `PhaseBoundary`, `TimeOptions`, `TimeCheckpoint`, `IGameClock`, `ITimeRuntime`, `TimeServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Time/game_calendar.h"
#include "Epidemic/Runtime/Time/game_time.h"
#include "Epidemic/Runtime/Time/time_events.h"
#include "Epidemic/Runtime/Time/time_scale.h"
#include "Epidemic/Runtime/Time/time_snapshot.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace epidemic::runtime
{
struct TimeAdvanceResult
{
    TimeSnapshot previous{};
    TimeSnapshot current{};
    std::vector<TimeEvent> events{};
};

struct PhaseBoundary
{
    std::uint32_t start_minute = 0;
    DayPhase phase = DayPhase::Night;
};

struct TimeOptions
{
    CalendarDefinition calendar{};
    std::int64_t game_ticks_per_real_second = 1;
    TimeScale initial_time_scale{};
    std::vector<PhaseBoundary> phase_boundaries{};
};

// Persistence-complete checkpoint for the authoritative Runtime clock.
//
// TimeSnapshot is intentionally frame-facing/observational and does not contain
// the fractional accumulator remainder. Persist this checkpoint instead. The
// configuration fields identify the immutable clock configuration that must
// match the destination runtime before RestoreCheckpoint() may commit.
struct TimeCheckpoint
{
    GameTimePoint now{};
    TimeScale time_scale{};
    bool paused = false;
    std::int64_t tick_remainder_numerator = 0;
    std::uint64_t revision = 0;

    std::int64_t game_ticks_per_real_second = 1;
    CalendarDefinition calendar{};
    std::vector<PhaseBoundary> phase_boundaries{};
};

class IGameClock
{
  public:
    virtual ~IGameClock() = default;

    [[nodiscard]] virtual GameTimePoint Now() const = 0;
    [[nodiscard]] virtual GameDuration LastDelta() const = 0;
    [[nodiscard]] virtual TimeSnapshot GetSnapshot() const = 0;
};

class ITimeRuntime
{
  public:
    virtual ~ITimeRuntime() = default;

    // CaptureCheckpoint may allocate while copying immutable configuration identity.
    [[nodiscard]] virtual TimeCheckpoint CaptureCheckpoint() const = 0;
    // Restore validates the complete candidate before a single no-fail commit.
    // Validation failure or std::bad_alloc leaves the live clock unchanged.
    [[nodiscard]] virtual foundation::Result<void> RestoreCheckpoint(const TimeCheckpoint& checkpoint) = 0;
    [[nodiscard]] virtual foundation::Result<TimeAdvanceResult> Advance(std::chrono::microseconds real_delta) = 0;
    [[nodiscard]] virtual foundation::Result<void> Pause() = 0;
    [[nodiscard]] virtual foundation::Result<void> Resume() = 0;
    [[nodiscard]] virtual foundation::Result<void> SetTimeScale(TimeScale scale) = 0;
    [[nodiscard]] virtual foundation::Result<TimeAdvanceResult> Skip(GameDuration duration) = 0;
};

struct TimeServices
{
    std::shared_ptr<IGameClock> clock;
    std::shared_ptr<ITimeRuntime> runtime;
};

[[nodiscard]] foundation::Result<void> ValidateTimeOptions(const TimeOptions& options);
[[nodiscard]] foundation::Result<TimeServices> CreateTimeServices(const TimeOptions& options = {});
} // namespace epidemic::runtime
```
