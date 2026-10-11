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

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Time/game_calendar.h`, `Epidemic/Runtime/Time/game_time.h`, `Epidemic/Runtime/Time/time_events.h`, `Epidemic/Runtime/Time/time_scale.h`, `Epidemic/Runtime/Time/time_snapshot.h`.

### Состав типов и операции

#### `PhaseBoundary` (строка заголовка 24)

Публичные поля и значения по умолчанию:

- `std::uint32_t start_minute = 0`
- `DayPhase phase = DayPhase::Night`

#### `TimeOptions` (строка заголовка 30)

Публичные поля и значения по умолчанию:

- `std::int64_t game_ticks_per_real_second = 1`

#### `TimeCheckpoint` (строка заголовка 44)

Публичные поля и значения по умолчанию:

- `bool paused = false`
- `std::int64_t tick_remainder_numerator = 0`
- `std::uint64_t revision = 0`
- `std::int64_t game_ticks_per_real_second = 1`

#### `IGameClock` (строка заголовка 57)

Публичные сигнатуры операций:

```cpp
virtual ~IGameClock() = default;
[[nodiscard]] virtual GameTimePoint Now() const = 0;
[[nodiscard]] virtual GameDuration LastDelta() const = 0;
[[nodiscard]] virtual TimeSnapshot GetSnapshot() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ITimeRuntime` (строка заголовка 67)

Публичные сигнатуры операций:

```cpp
virtual ~ITimeRuntime() = default;
[[nodiscard]] virtual foundation::Result<TimeAdvanceResult> Advance(std::chrono::microseconds real_delta) = 0;
[[nodiscard]] virtual foundation::Result<void> Pause() = 0;
[[nodiscard]] virtual foundation::Result<void> Resume() = 0;
[[nodiscard]] virtual foundation::Result<void> SetTimeScale(TimeScale scale) = 0;
[[nodiscard]] virtual foundation::Result<TimeAdvanceResult> Skip(GameDuration duration) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `TimeServices` (строка заголовка 84)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IGameClock> clock`
- `std::shared_ptr<ITimeRuntime> runtime`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Now` | `TimeRuntime::Now` | `EngineRuntime/Time/src/time_runtime_impl.cpp:261` |
| `LastDelta` | `TimeRuntime::LastDelta` | `EngineRuntime/Time/src/time_runtime_impl.cpp:266` |
| `GetSnapshot` | `TimeRuntime::GetSnapshot` | `EngineRuntime/Time/src/time_runtime_impl.cpp:271` |
| `Advance` | `TimeRuntime::Advance` | `EngineRuntime/Time/src/time_runtime_impl.cpp:325` |
| `Pause` | `TimeRuntime::Pause` | `EngineRuntime/Time/src/time_runtime_impl.cpp:420` |
| `Resume` | `TimeRuntime::Resume` | `EngineRuntime/Time/src/time_runtime_impl.cpp:453` |
| `SetTimeScale` | `TimeRuntime::SetTimeScale` | `EngineRuntime/Time/src/time_runtime_impl.cpp:486` |
| `Skip` | `TimeRuntime::Skip` | `EngineRuntime/Time/src/time_runtime_impl.cpp:521` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetSnapshot`: `EngineRuntime/Time/tests/time_tests.cpp:64`
- `Now`: `EngineRuntime/Time/tests/time_tests.cpp:66`
- `Advance`: `EngineRuntime/Time/tests/time_tests.cpp:77`
- `Skip`: `EngineRuntime/Time/tests/time_tests.cpp:105`
- `Pause`: `EngineRuntime/Time/tests/time_tests.cpp:139`
- `Resume`: `EngineRuntime/Time/tests/time_tests.cpp:141`
- `LastDelta`: `EngineRuntime/Time/tests/time_tests.cpp:145`
- `SetTimeScale`: `EngineRuntime/Time/tests/time_tests.cpp:151`

