---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h"
---

# Time: gameplay_time.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h`.

**Модуль:** [[06 EngineFramework/Модуль Time|Time]].
**Размер исходного файла:** 8701 байт.
**Подключение:** `#include <Epidemic/GameFramework/Time/gameplay_time.h>`.

## Объявления типов (лексический индекс)
`CalendarDefinition`, `CalendarDate`, `RecurrenceKind`, `CatchUpPolicy`, `SchedulePersistence`, `CalendarPattern`, `RecurrenceRule`, `ClockDefinition`, `ClockState`, `ScheduleEntry`, `ScheduledTrigger`, `SchedulerBudget`, `GameplayTimeDiagnostics`, `GameplayTimeSnapshot`, `GameplayTimeService`, `ActionTypeInfo`, `ScheduleKey`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/error.h"
#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::time
{
struct CalendarDefinition
{
    std::uint32_t hours_per_day = 24;
    std::uint32_t days_per_month = 30;
    std::uint32_t months_per_year = 12;
};

struct CalendarDate
{
    std::int64_t year = 1;
    std::uint32_t month = 1;
    std::uint32_t day = 1;
    std::uint32_t hour = 0;
    std::uint32_t minute = 0;
    std::uint32_t second = 0;

    [[nodiscard]] constexpr bool operator==(const CalendarDate&) const noexcept = default;
};
enum class RecurrenceKind
{
    Once,
    FixedInterval,
    CalendarPattern,
};

enum class CatchUpPolicy
{
    FireEach,
    FireOnce,
    SkipMissed,
    Aggregate,
};

enum class SchedulePersistence
{
    Session,
    Persistent,
};

struct CalendarPattern
{
    std::uint32_t every_days = 1;
    std::uint32_t hour = 0;
    std::uint32_t minute = 0;
    std::uint32_t second = 0;
};

struct RecurrenceRule
{
    RecurrenceKind kind = RecurrenceKind::Once;
    GameplayDuration interval{};
    CalendarPattern calendar{};
};

struct ClockDefinition
{
    ClockId id{};
    std::string canonical_name;
    std::optional<CalendarDefinition> calendar{};
};

struct ClockState
{
    ClockId id{};
    GameplayTimePoint now{};
    Revision revision{};
    bool paused = false;
    std::uint32_t time_scale_milli = 1000;
    std::uint32_t fractional_milli = 0; // accumulated sub-tick numerator remainder, [0, 999]
};

struct ScheduleEntry
{
    ScheduleId id{};
    ClockId clock{};
    GameplayTimePoint due{};
    GameplayObjectRef owner{};
    ActionTypeId action{};
    RecurrenceRule recurrence{};
    CatchUpPolicy catch_up = CatchUpPolicy::FireOnce;
    SchedulePersistence persistence = SchedulePersistence::Session;
    Revision revision{};
};

struct ScheduledTrigger
{
    ScheduleId schedule{};
    ClockId clock{};
    GameplayObjectRef owner{};
    ActionTypeId action{};
    GameplayTimePoint scheduled_for{};
    GameplayTimePoint observed_at{};
    std::uint64_t occurrence_count = 1;
};

struct SchedulerBudget
{
    std::uint64_t max_triggers = 10000;
    std::uint64_t max_catch_up_occurrences = 1000000;
};

struct GameplayTimeDiagnostics
{
    std::uint64_t active_schedules = 0;
    std::uint64_t persistent_schedules = 0;
    std::uint64_t emitted_triggers = 0;
    std::uint64_t cancelled_schedules = 0;
    std::uint64_t catch_up_occurrences = 0;
    std::uint64_t budget_exhaustions = 0;
};

struct GameplayTimeSnapshot
{
    std::vector<ClockState> clocks;
    std::vector<ScheduleEntry> schedules;
    std::vector<ClockId> externally_synchronized_clocks;
    MonotonicIdGenerator<ScheduleId>::Snapshot schedule_ids{};
    Revision scheduler_revision{};
};

class GameplayTimeService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<GameplayTimeService>;

  public:
    GameplayTimeService() = default;

    [[nodiscard]] foundation::Result<ClockId> RegisterClock(
        std::string_view canonical_name,
        std::optional<CalendarDefinition> calendar = std::nullopt);

    [[nodiscard]] foundation::Result<ActionTypeId> RegisterAction(std::string_view canonical_name, GameplayDomainId owner_domain);

    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }
    [[nodiscard]] foundation::Result<void> AdvanceClock(ClockId clock, GameplayDuration real_delta);
    [[nodiscard]] foundation::Result<void> AdvanceTo(ClockId clock, GameplayTimePoint now);
    [[nodiscard]] foundation::Result<void> SetPaused(ClockId clock, bool paused);
    [[nodiscard]] foundation::Result<void> SetTimeScale(ClockId clock, std::uint32_t milli_scale);
    [[nodiscard]] foundation::Result<void> SynchronizeClock(ClockId clock, GameplayTimePoint now, Revision source_revision);
    [[nodiscard]] bool NeedsSynchronizationRebind(ClockId clock) const noexcept;
    [[nodiscard]] std::vector<ClockId> ClocksRequiringSynchronizationRebind() const;

    [[nodiscard]] std::optional<ClockState> GetClock(ClockId clock) const noexcept;
    [[nodiscard]] std::optional<ClockDefinition> GetClockDefinition(ClockId clock) const noexcept;
    [[nodiscard]] const ClockDefinition* FindClockDefinition(ClockId clock) const noexcept;
    [[nodiscard]] foundation::Result<void> ValidateCalendarDate(ClockId clock, const CalendarDate& date) const;
    [[nodiscard]] foundation::Result<GameplayTimePoint> ToGameplayTime(ClockId clock, const CalendarDate& date) const;
    [[nodiscard]] foundation::Result<CalendarDate> ToCalendarDate(ClockId clock, GameplayTimePoint time) const;

    [[nodiscard]] foundation::Result<ScheduleId> Schedule(
        ClockId clock,
        GameplayTimePoint due,
        GameplayObjectRef owner,
        ActionTypeId action,
        RecurrenceRule recurrence = {},
        CatchUpPolicy catch_up = CatchUpPolicy::FireOnce,
        SchedulePersistence persistence = SchedulePersistence::Session);

    [[nodiscard]] foundation::Result<ScheduleId> ScheduleCalendar(
        ClockId clock,
        GameplayObjectRef owner,
        ActionTypeId action,
        CalendarPattern pattern,
        CatchUpPolicy catch_up = CatchUpPolicy::FireOnce,
        SchedulePersistence persistence = SchedulePersistence::Session);

    [[nodiscard]] foundation::Result<void> Cancel(ScheduleId schedule);
    [[nodiscard]] std::uint64_t CancelOwnedBy(GameplayObjectRef owner);
    [[nodiscard]] foundation::Result<void> Reschedule(ScheduleId schedule, GameplayTimePoint new_due);
    [[nodiscard]] bool HasSchedule(ScheduleId schedule) const noexcept;
    [[nodiscard]] std::optional<ScheduleEntry> GetSchedule(ScheduleId schedule) const noexcept;

    [[nodiscard]] foundation::Result<std::vector<ScheduledTrigger>> CollectDue(
        ClockId clock,
        SchedulerBudget budget = {});

    [[nodiscard]] GameplayTimeSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(GameplayTimeSnapshot snapshot);
    [[nodiscard]] GameplayTimeDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision SchedulerRevision() const noexcept { return scheduler_revision_; }

  private:
    struct ActionTypeInfo
    {
        ActionTypeId id{};
        std::string canonical_name;
        GameplayDomainId owner_domain{};
    };

    struct ScheduleKey
    {
        GameplayTimePoint due{};
        ScheduleId id{};

        [[nodiscard]] bool operator<(const ScheduleKey& other) const noexcept
        {
            if (due != other.due)
            {
                return due < other.due;
            }
            return id < other.id;
        }
    };

    [[nodiscard]] foundation::Result<void> ValidateRecurrence(ClockId clock, const RecurrenceRule& recurrence) const;
    [[nodiscard]] foundation::Result<GameplayTimePoint> NextCalendarDue(ClockId clock, CalendarPattern pattern) const;
    [[nodiscard]] foundation::Result<std::uint64_t> CalculateOccurrences(const ScheduleEntry& entry, GameplayTimePoint now) const;
    [[nodiscard]] foundation::Result<GameplayTimePoint> AdvanceDue(const ScheduleEntry& entry, std::uint64_t occurrences) const;
    [[nodiscard]] foundation::Result<std::int64_t> RecurrenceIntervalTicks(const ScheduleEntry& entry) const;
    void InsertScheduleIndex(const ScheduleEntry& entry);
    void RemoveScheduleIndex(const ScheduleEntry& entry);

    std::unordered_map<ClockId, ClockDefinition> clock_definitions_;
    std::unordered_map<ClockId, ClockState> clocks_;
    std::unordered_map<ClockId, Revision> synchronization_revisions_;
    std::set<ClockId> synchronization_rebind_required_;
    std::unordered_map<ActionTypeId, ActionTypeInfo> action_types_;
    std::unordered_map<ScheduleId, ScheduleEntry> schedules_;
    std::unordered_map<ClockId, std::set<ScheduleKey>> schedule_index_;

    MonotonicIdGenerator<ScheduleId> schedule_ids_{ScheduleId::FromString("framework.gameplay_time").High()};
    Revision scheduler_revision_{};
    bool frozen_ = false;

    std::uint64_t emitted_triggers_ = 0;
    std::uint64_t cancelled_schedules_ = 0;
    std::uint64_t catch_up_occurrences_ = 0;
    std::uint64_t budget_exhaustions_ = 0;
};
} // namespace epidemic::gameplay::time
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/error.h`, `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `RecurrenceKind`: `Once`, `FixedInterval`, `CalendarPattern`
- `CatchUpPolicy`: `FireEach`, `FireOnce`, `SkipMissed`, `Aggregate`
- `SchedulePersistence`: `Session`, `Persistent`

### Состав типов и операции

#### `CalendarDefinition` (строка заголовка 19)

Публичные поля и значения по умолчанию:

- `std::uint32_t hours_per_day = 24`
- `std::uint32_t days_per_month = 30`
- `std::uint32_t months_per_year = 12`

#### `CalendarDate` (строка заголовка 26)

Публичные поля и значения по умолчанию:

- `std::int64_t year = 1`
- `std::uint32_t month = 1`
- `std::uint32_t day = 1`
- `std::uint32_t hour = 0`
- `std::uint32_t minute = 0`
- `std::uint32_t second = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const CalendarDate&) const noexcept = default;
```

#### `CalendarPattern` (строка заголовка 58)

Публичные поля и значения по умолчанию:

- `std::uint32_t every_days = 1`
- `std::uint32_t hour = 0`
- `std::uint32_t minute = 0`
- `std::uint32_t second = 0`

#### `RecurrenceRule` (строка заголовка 66)

Публичные поля и значения по умолчанию:

- `RecurrenceKind kind = RecurrenceKind::Once`

#### `ClockDefinition` (строка заголовка 73)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`

#### `ClockState` (строка заголовка 80)

Публичные поля и значения по умолчанию:

- `bool paused = false`
- `std::uint32_t time_scale_milli = 1000`
- `std::uint32_t fractional_milli = 0`

#### `ScheduleEntry` (строка заголовка 90)

Публичные поля и значения по умолчанию:

- `CatchUpPolicy catch_up = CatchUpPolicy::FireOnce`
- `SchedulePersistence persistence = SchedulePersistence::Session`

#### `ScheduledTrigger` (строка заголовка 103)

Публичные поля и значения по умолчанию:

- `std::uint64_t occurrence_count = 1`

#### `SchedulerBudget` (строка заголовка 114)

Публичные поля и значения по умолчанию:

- `std::uint64_t max_triggers = 10000`
- `std::uint64_t max_catch_up_occurrences = 1000000`

#### `GameplayTimeDiagnostics` (строка заголовка 120)

Публичные поля и значения по умолчанию:

- `std::uint64_t active_schedules = 0`
- `std::uint64_t persistent_schedules = 0`
- `std::uint64_t emitted_triggers = 0`
- `std::uint64_t cancelled_schedules = 0`
- `std::uint64_t catch_up_occurrences = 0`
- `std::uint64_t budget_exhaustions = 0`

#### `GameplayTimeSnapshot` (строка заголовка 130)

Публичные поля и значения по умолчанию:

- `std::vector<ClockState> clocks`
- `std::vector<ScheduleEntry> schedules`
- `std::vector<ClockId> externally_synchronized_clocks`

#### `GameplayTimeService` (строка заголовка 139)

Публичные поля и значения по умолчанию:

- `, CatchUpPolicy catch_up = CatchUpPolicy::FireOnce, SchedulePersistence persistence = SchedulePersistence::Session)`

Внутреннее состояние (не часть публичного API): `std::unordered_map<ClockId, ClockDefinition> clock_definitions_`, `std::unordered_map<ClockId, ClockState> clocks_`, `std::unordered_map<ClockId, Revision> synchronization_revisions_`, `std::set<ClockId> synchronization_rebind_required_`, `std::unordered_map<ActionTypeId, ActionTypeInfo> action_types_`, `std::unordered_map<ScheduleId, ScheduleEntry> schedules_`, `std::unordered_map<ClockId, std::set<ScheduleKey>> schedule_index_`, `bool frozen_ = false`, `std::uint64_t emitted_triggers_ = 0`.

Публичные сигнатуры операций:

```cpp
GameplayTimeService() = default;
[[nodiscard]] foundation::Result<ClockId> RegisterClock( std::string_view canonical_name, std::optional<CalendarDefinition> calendar = std::nullopt);
[[nodiscard]] foundation::Result<ActionTypeId> RegisterAction(std::string_view canonical_name, GameplayDomainId owner_domain);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] foundation::Result<void> AdvanceClock(ClockId clock, GameplayDuration real_delta);
[[nodiscard]] foundation::Result<void> AdvanceTo(ClockId clock, GameplayTimePoint now);
[[nodiscard]] foundation::Result<void> SetPaused(ClockId clock, bool paused);
[[nodiscard]] foundation::Result<void> SetTimeScale(ClockId clock, std::uint32_t milli_scale);
[[nodiscard]] foundation::Result<void> SynchronizeClock(ClockId clock, GameplayTimePoint now, Revision source_revision);
[[nodiscard]] bool NeedsSynchronizationRebind(ClockId clock) const noexcept;
[[nodiscard]] std::vector<ClockId> ClocksRequiringSynchronizationRebind() const;
[[nodiscard]] std::optional<ClockState> GetClock(ClockId clock) const noexcept;
[[nodiscard]] std::optional<ClockDefinition> GetClockDefinition(ClockId clock) const noexcept;
[[nodiscard]] const ClockDefinition* FindClockDefinition(ClockId clock) const noexcept;
[[nodiscard]] foundation::Result<void> ValidateCalendarDate(ClockId clock, const CalendarDate& date) const;
[[nodiscard]] foundation::Result<GameplayTimePoint> ToGameplayTime(ClockId clock, const CalendarDate& date) const;
[[nodiscard]] foundation::Result<CalendarDate> ToCalendarDate(ClockId clock, GameplayTimePoint time) const;
[[nodiscard]] foundation::Result<ScheduleId> ScheduleCalendar( ClockId clock, GameplayObjectRef owner, ActionTypeId action, CalendarPattern pattern, CatchUpPolicy catch_up = CatchUpPolicy::FireOnce, SchedulePersistence persistence = SchedulePersistence::Session);
[[nodiscard]] foundation::Result<void> Cancel(ScheduleId schedule);
[[nodiscard]] std::uint64_t CancelOwnedBy(GameplayObjectRef owner);
[[nodiscard]] foundation::Result<void> Reschedule(ScheduleId schedule, GameplayTimePoint new_due);
[[nodiscard]] bool HasSchedule(ScheduleId schedule) const noexcept;
[[nodiscard]] std::optional<ScheduleEntry> GetSchedule(ScheduleId schedule) const noexcept;
[[nodiscard]] GameplayTimeSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(GameplayTimeSnapshot snapshot);
[[nodiscard]] GameplayTimeDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision SchedulerRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `ActionTypeInfo` (строка заголовка 202)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`

#### `ScheduleKey` (строка заголовка 209)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator<(const ScheduleKey& other) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterClock` | `GameplayTimeService::RegisterClock` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:46` |
| `RegisterAction` | `GameplayTimeService::RegisterAction` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:82` |
| `AdvanceClock` | `GameplayTimeService::AdvanceClock` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:113` |
| `AdvanceTo` | `GameplayTimeService::AdvanceTo` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:172` |
| `SetPaused` | `GameplayTimeService::SetPaused` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:201` |
| `SetTimeScale` | `GameplayTimeService::SetTimeScale` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:224` |
| `SynchronizeClock` | `GameplayTimeService::SynchronizeClock` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:252` |
| `NeedsSynchronizationRebind` | `GameplayTimeService::NeedsSynchronizationRebind` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:292` |
| `ClocksRequiringSynchronizationRebind` | `GameplayTimeService::ClocksRequiringSynchronizationRebind` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:297` |
| `GetClock` | `GameplayTimeService::GetClock` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:302` |
| `GetClockDefinition` | `GameplayTimeService::GetClockDefinition` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:307` |
| `FindClockDefinition` | `GameplayTimeService::FindClockDefinition` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:312` |
| `ValidateCalendarDate` | `GameplayTimeService::ValidateCalendarDate` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:318` |
| `ToGameplayTime` | `GameplayTimeService::ToGameplayTime` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:337` |
| `ToCalendarDate` | `GameplayTimeService::ToCalendarDate` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:385` |
| `ScheduleCalendar` | `GameplayTimeService::ScheduleCalendar` | `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:564` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterClock`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:19`
- `RegisterAction`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:20`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:25`
- `SynchronizeClock`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:27`
- `GetSchedule`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:51`
- `ScheduleCalendar`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:57`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:64`
- `Cancel`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:69`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:73`
- `SetTimeScale`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:155`
- `AdvanceClock`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:161`
- `GetClock`: `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp:166`

