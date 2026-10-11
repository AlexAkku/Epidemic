---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h"
---

# RolesJobs: roles_jobs.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h`.

**Модуль:** [[06 EngineFramework/Модуль RolesJobs|RolesJobs]].
**Размер исходного файла:** 17441 байт.
**Подключение:** `#include <Epidemic/GameFramework/RolesJobs/roles_jobs.h>`.

## Объявления типов (лексический индекс)
`JobDefinitionId`, `JobAssignmentId`, `WorkplaceId`, `WorkScheduleId`, `WorkShiftId`, `DutyId`, `RoleFunctionId`, `JobTaskTypeId`, `IdHash`, `WorkplaceState`, `AssignmentState`, `DutyState`, `RolesJobsChangeKind`, `JobDefinition`, `Workplace`, `JobAssignment`, `WorkShift`, `WorkSchedule`, `Duty`, `RolesJobsChange`, `RolesJobsChangeBatch`, `RolesJobsSnapshot`, `RolesJobsDiagnostics`, `RolesJobsService`, `DutyOccurrenceKey`, `DutyOccurrenceKeyHash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::roles_jobs
{
struct JobDefinitionId
{
    TypeId value{};
    static constexpr JobDefinitionId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const JobDefinitionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobDefinitionId &) const noexcept = default;
};
struct JobAssignmentId
{
    GameplayObjectId value{};
    static constexpr JobAssignmentId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr JobAssignmentId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const JobAssignmentId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobAssignmentId &) const noexcept = default;
};
struct WorkplaceId
{
    GameplayObjectId value{};
    static constexpr WorkplaceId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr WorkplaceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorkplaceId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorkplaceId &) const noexcept = default;
};
struct WorkScheduleId
{
    GameplayObjectId value{};
    static constexpr WorkScheduleId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr WorkScheduleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorkScheduleId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorkScheduleId &) const noexcept = default;
};
struct WorkShiftId
{
    GameplayObjectId value{};
    static constexpr WorkShiftId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr WorkShiftId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorkShiftId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorkShiftId &) const noexcept = default;
};
struct DutyId
{
    GameplayObjectId value{};
    static constexpr DutyId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr DutyId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const DutyId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const DutyId &) const noexcept = default;
};
struct RoleFunctionId
{
    TypeId value{};
    static constexpr RoleFunctionId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const RoleFunctionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const RoleFunctionId &) const noexcept = default;
};
struct JobTaskTypeId
{
    TypeId value{};
    static constexpr JobTaskTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const JobTaskTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JobTaskTypeId &) const noexcept = default;
};
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

enum class WorkplaceState
{
    Active,
    Closed,
    Destroyed,
    Unavailable,
    Seasonal,
    Suspended
};
enum class AssignmentState
{
    Active,
    Paused,
    Suspended,
    Completed,
    Cancelled,
    Invalid
};
enum class DutyState
{
    Scheduled,
    Available,
    Active,
    Completed,
    Failed,
    Skipped,
    Cancelled
};
enum class RolesJobsChangeKind
{
    JobAssigned,
    JobRemoved,
    JobPaused,
    JobResumed,
    WorkplaceCreated,
    WorkplaceStateChanged,
    ShiftScheduled,
    ShiftStarted,
    ShiftEnded,
    DutyCreated,
    DutyActivated,
    DutyCompleted,
    DutyFailed,
    DutySkipped,
    DutyCancelled
};

struct JobDefinition
{
    JobDefinitionId id{};
    GameplayTagSet tags{};
    RoleFunctionId function{};
    std::vector<JobTaskTypeId> default_tasks;
    std::vector<std::byte> payload;
    Revision revision{};
};
struct Workplace
{
    WorkplaceId id{};
    GameplayObjectRef owner{};
    GameplayObjectRef area{};
    GameplayObjectRef property{};
    GameplayTagSet tags{};
    WorkplaceState state = WorkplaceState::Active;
    Revision revision{};
};
struct JobAssignment
{
    JobAssignmentId id{};
    GameplayObjectRef worker{};
    JobDefinitionId job{};
    WorkplaceId workplace{};
    AssignmentState state = AssignmentState::Active;
    GameplayTimePoint assigned_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct WorkShift
{
    WorkShiftId id{};
    GameplayTimePoint start{};
    GameplayDuration duration{};
    // Zero duration means a one-shot shift. Positive duration creates recurring occurrences at this interval.
    GameplayDuration recurrence{};
    JobTaskTypeId task_type{};
    GameplayObjectRef target_area{};
    GameplayObjectRef target_object{};
    std::vector<std::byte> payload;
};
struct WorkSchedule
{
    WorkScheduleId id{};
    JobAssignmentId assignment{};
    std::vector<WorkShift> shifts;
    std::int32_t priority = 0;
    Revision revision{};
};
struct Duty
{
    DutyId id{};
    GameplayObjectRef subject{};
    TypeId type{};
    std::int32_t priority = 0;
    DutyState state = DutyState::Scheduled;
    JobAssignmentId assignment{};
    WorkShiftId shift{};
    GameplayTimePoint scheduled_start{};
    GameplayTimePoint scheduled_end{};
    std::vector<JobTaskTypeId> tasks;
    std::vector<std::byte> payload;
    Revision revision{};
};
struct RolesJobsChange
{
    std::uint64_t sequence = 0;
    RolesJobsChangeKind kind = RolesJobsChangeKind::JobAssigned;
    GameplayObjectRef subject{};
    JobAssignmentId assignment{};
    WorkplaceId workplace{};
    DutyId duty{};
    GameplayContext context{};
    Revision revision{};
};
struct RolesJobsChangeBatch
{
    std::vector<RolesJobsChange> changes;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    bool snapshot_required = false;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct RolesJobsSnapshot
{
    std::vector<JobDefinition> definitions;
    std::vector<Workplace> workplaces;
    std::vector<JobAssignment> assignments;
    std::vector<WorkSchedule> schedules;
    std::vector<Duty> duties;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot assignment_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot workplace_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot schedule_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot shift_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot duty_ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};
struct RolesJobsDiagnostics
{
    std::uint64_t definitions = 0, workplaces = 0, assignments = 0, schedules = 0, active_duties = 0, shift_events = 0,
                  duty_failures = 0, automatic_suspension_failures = 0, retained_changes = 0,
                  retained_terminal_duties = 0;
};

class RolesJobsService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<RolesJobsService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.roles_jobs");
    }
    [[nodiscard]] foundation::Result<void> RegisterJobDefinition(JobDefinition definition);
    [[nodiscard]] foundation::Result<WorkplaceId> CreateWorkplace(Workplace workplace);
    [[nodiscard]] foundation::Result<void> SetWorkplaceState(WorkplaceId id, WorkplaceState state,
                                                             GameplayContext context = {});
    [[nodiscard]] foundation::Result<JobAssignmentId> AssignJob(JobAssignment assignment, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> PauseAssignment(JobAssignmentId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ResumeAssignment(JobAssignmentId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SuspendAssignment(JobAssignmentId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompleteAssignment(JobAssignmentId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CancelAssignment(JobAssignmentId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<WorkScheduleId> CreateSchedule(WorkSchedule schedule);
    [[nodiscard]] foundation::Result<DutyId> CreateDuty(Duty duty, GameplayContext context = {});
    [[nodiscard]] std::vector<DutyId> ActivateDueShifts(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] std::vector<DutyId> ActivateDueShifts(GameplayTimePoint from, GameplayTimePoint to,
                                                        GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ActivateDuty(DutyId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompleteDuty(DutyId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> FailDuty(DutyId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SkipDuty(DutyId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CancelDuty(DutyId id, GameplayContext context = {});
    [[nodiscard]] const JobAssignment *GetJobAssignment(JobAssignmentId id) const noexcept;
    [[nodiscard]] const Workplace *GetWorkplace(WorkplaceId id) const noexcept;
    [[nodiscard]] const Duty *FindDuty(DutyId id) const noexcept;
    [[nodiscard]] const Duty *GetCurrentDuty(GameplayObjectRef subject) const noexcept;
    [[nodiscard]] std::vector<JobAssignment> FindJobsOfSubject(GameplayObjectRef subject) const;
    [[nodiscard]] std::vector<JobAssignment> FindWorkersAtWorkplace(WorkplaceId workplace) const;
    [[nodiscard]] std::vector<Workplace> FindWorkplacesInArea(GameplayObjectRef area) const;
    [[nodiscard]] std::vector<Duty> FindDutiesForAssignment(JobAssignmentId assignment) const;
    [[nodiscard]] std::vector<Duty> FindActiveDuties() const;
    private:
        [[nodiscard]] std::vector<RolesJobsChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] RolesJobsChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] RolesJobsChangeBatch ReadChangesSince(ChangeCursor cursor) const
    {
        auto batch = ReadChangesSinceSequence(cursor.sequence);
        batch.oldest_available_cursor = {journal_epoch_, batch.oldest_available_sequence};
        batch.latest_cursor = {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                                    : next_change_sequence_ - 1};
        if ((!cursor.IsValid() && cursor.sequence != 0) || (cursor.IsValid() && cursor.epoch != journal_epoch_))
        {
            batch.changes.clear();
            batch.snapshot_required = true;
        }
        return batch;
    }
    [[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept
    {
        return {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                          : next_change_sequence_ - 1};
    }
    [[nodiscard]] std::uint64_t OldestChangeSequence() const noexcept;
    void PruneChangesBefore(std::uint64_t sequence) noexcept;
    [[nodiscard]] RolesJobsSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(RolesJobsSnapshot snapshot);
    [[nodiscard]] RolesJobsDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept
    {
        return revision_;
    }

  private:
    struct DutyOccurrenceKey
    {
        JobAssignmentId assignment{};
        WorkShiftId shift{};
        GameplayTimePoint start{};
        [[nodiscard]] bool operator==(const DutyOccurrenceKey &) const noexcept = default;
    };
    struct DutyOccurrenceKeyHash
    {
        [[nodiscard]] std::size_t operator()(const DutyOccurrenceKey &key) const noexcept;
    };

    [[nodiscard]] foundation::Result<Revision> NextRevision() const;
    void Bump(Revision next) noexcept
    {
        revision_ = next;
    }
    void Record(RolesJobsChange change);
    void RebuildIndexes();
    void PruneTerminalDuties();
    [[nodiscard]] bool IsWorkplaceOperational(WorkplaceId id) const noexcept;
    [[nodiscard]] bool IsWorkplaceStateOperational(WorkplaceState state) const noexcept;
    [[nodiscard]] bool IsTerminalAssignment(AssignmentState state) const noexcept;
    [[nodiscard]] bool IsTerminalDuty(DutyState state) const noexcept;
    [[nodiscard]] bool IsLiveDuty(DutyState state) const noexcept;
    [[nodiscard]] bool HasDutyOccurrence(JobAssignmentId assignment, WorkShiftId shift, GameplayTimePoint start) const;
    [[nodiscard]] foundation::Result<DutyId> CreateDutyInternal(Duty duty, RolesJobsChangeKind initial_shift_change,
                                                                GameplayContext context);
    [[nodiscard]] foundation::Result<void> TransitionDuty(DutyId id, DutyState state, RolesJobsChangeKind change,
                                                          GameplayContext context);
    [[nodiscard]] foundation::Result<void> TransitionAssignment(JobAssignmentId id, AssignmentState state,
                                                                RolesJobsChangeKind change, GameplayContext context);
    [[nodiscard]] Duty *FindMutableDuty(DutyId id) noexcept;
    [[nodiscard]] JobAssignment *FindMutableAssignment(JobAssignmentId id) noexcept;
    Revision revision_{};
    MonotonicIdGenerator<GameplayObjectId> assignment_ids_{0x2800};
    MonotonicIdGenerator<GameplayObjectId> workplace_ids_{0x2801};
    MonotonicIdGenerator<GameplayObjectId> schedule_ids_{0x2802};
    MonotonicIdGenerator<GameplayObjectId> shift_ids_{0x2803};
    MonotonicIdGenerator<GameplayObjectId> duty_ids_{0x2804};
    std::unordered_map<JobDefinitionId, JobDefinition, IdHash> definitions_;
    std::unordered_map<WorkplaceId, Workplace, IdHash> workplaces_;
    std::unordered_map<JobAssignmentId, JobAssignment, IdHash> assignments_;
    std::unordered_map<WorkScheduleId, WorkSchedule, IdHash> schedules_;
    std::unordered_map<DutyId, Duty, IdHash> duties_;
    std::unordered_map<GameplayObjectRef, std::vector<JobAssignmentId>> assignments_by_worker_;
    std::unordered_map<WorkplaceId, std::vector<JobAssignmentId>, IdHash> assignments_by_workplace_;
    std::unordered_map<JobAssignmentId, std::vector<WorkScheduleId>, IdHash> schedules_by_assignment_;
    std::unordered_map<JobAssignmentId, std::vector<DutyId>, IdHash> duties_by_assignment_;
    std::unordered_map<GameplayObjectRef, std::vector<DutyId>> duties_by_subject_;
    std::deque<RolesJobsChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    std::size_t change_retention_capacity_ = 4096;
    std::size_t terminal_duty_retention_capacity_ = 2048;
    mutable RolesJobsDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::roles_jobs
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `WorkplaceState`: `Active`, `Closed`, `Destroyed`, `Unavailable`, `Seasonal`, `Suspended`
- `AssignmentState`: `Active`, `Paused`, `Suspended`, `Completed`, `Cancelled`, `Invalid`
- `DutyState`: `Scheduled`, `Available`, `Active`, `Completed`, `Failed`, `Skipped`, `Cancelled`
- `RolesJobsChangeKind`: `JobAssigned`, `JobRemoved`, `JobPaused`, `JobResumed`, `WorkplaceCreated`, `WorkplaceStateChanged`, `ShiftScheduled`, `ShiftStarted`, `ShiftEnded`, `DutyCreated`, `DutyActivated`, `DutyCompleted`, `DutyFailed`, `DutySkipped`, `DutyCancelled`

### Состав типов и операции

#### `JobDefinitionId` (строка заголовка 16)

Публичные сигнатуры операций:

```cpp
static constexpr JobDefinitionId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const JobDefinitionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const JobDefinitionId &) const noexcept = default;
```

#### `JobAssignmentId` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
static constexpr JobAssignmentId FromString(std::string_view s) noexcept;
static constexpr JobAssignmentId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const JobAssignmentId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const JobAssignmentId &) const noexcept = default;
```

#### `WorkplaceId` (строка заголовка 48)

Публичные сигнатуры операций:

```cpp
static constexpr WorkplaceId FromString(std::string_view s) noexcept;
static constexpr WorkplaceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorkplaceId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorkplaceId &) const noexcept = default;
```

#### `WorkScheduleId` (строка заголовка 66)

Публичные сигнатуры операций:

```cpp
static constexpr WorkScheduleId FromString(std::string_view s) noexcept;
static constexpr WorkScheduleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorkScheduleId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorkScheduleId &) const noexcept = default;
```

#### `WorkShiftId` (строка заголовка 84)

Публичные сигнатуры операций:

```cpp
static constexpr WorkShiftId FromString(std::string_view s) noexcept;
static constexpr WorkShiftId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorkShiftId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorkShiftId &) const noexcept = default;
```

#### `DutyId` (строка заголовка 102)

Публичные сигнатуры операций:

```cpp
static constexpr DutyId FromString(std::string_view s) noexcept;
static constexpr DutyId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const DutyId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const DutyId &) const noexcept = default;
```

#### `RoleFunctionId` (строка заголовка 120)

Публичные сигнатуры операций:

```cpp
static constexpr RoleFunctionId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const RoleFunctionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const RoleFunctionId &) const noexcept = default;
```

#### `JobTaskTypeId` (строка заголовка 134)

Публичные сигнатуры операций:

```cpp
static constexpr JobTaskTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const JobTaskTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const JobTaskTypeId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 148)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 150)

#### `JobDefinition` (строка заголовка 203)

Публичные поля и значения по умолчанию:

- `std::vector<JobTaskTypeId> default_tasks`
- `std::vector<std::byte> payload`

#### `Workplace` (строка заголовка 212)

Публичные поля и значения по умолчанию:

- `WorkplaceState state = WorkplaceState::Active`

#### `JobAssignment` (строка заголовка 222)

Публичные поля и значения по умолчанию:

- `AssignmentState state = AssignmentState::Active`
- `std::vector<std::byte> payload`

#### `WorkShift` (строка заголовка 233)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`

#### `WorkSchedule` (строка заголовка 245)

Публичные поля и значения по умолчанию:

- `std::vector<WorkShift> shifts`
- `std::int32_t priority = 0`

#### `Duty` (строка заголовка 253)

Публичные поля и значения по умолчанию:

- `std::int32_t priority = 0`
- `DutyState state = DutyState::Scheduled`
- `std::vector<JobTaskTypeId> tasks`
- `std::vector<std::byte> payload`

#### `RolesJobsChange` (строка заголовка 268)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `RolesJobsChangeKind kind = RolesJobsChangeKind::JobAssigned`

#### `RolesJobsChangeBatch` (строка заголовка 279)

Публичные поля и значения по умолчанию:

- `std::vector<RolesJobsChange> changes`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `bool snapshot_required = false`

#### `RolesJobsSnapshot` (строка заголовка 289)

Публичные поля и значения по умолчанию:

- `std::vector<JobDefinition> definitions`
- `std::vector<Workplace> workplaces`
- `std::vector<JobAssignment> assignments`
- `std::vector<WorkSchedule> schedules`
- `std::vector<Duty> duties`
- `std::uint64_t change_epoch = 1`

#### `RolesJobsService` (строка заголовка 312)

Внутреннее состояние (не часть публичного API): `std::unordered_map<JobDefinitionId, JobDefinition, IdHash> definitions_`, `std::unordered_map<WorkplaceId, Workplace, IdHash> workplaces_`, `std::unordered_map<JobAssignmentId, JobAssignment, IdHash> assignments_`, `std::unordered_map<WorkScheduleId, WorkSchedule, IdHash> schedules_`, `std::unordered_map<DutyId, Duty, IdHash> duties_`, `std::unordered_map<GameplayObjectRef, std::vector<JobAssignmentId>> assignments_by_worker_`, `std::unordered_map<WorkplaceId, std::vector<JobAssignmentId>, IdHash> assignments_by_workplace_`, `std::unordered_map<JobAssignmentId, std::vector<WorkScheduleId>, IdHash> schedules_by_assignmen`, `std::unordered_map<JobAssignmentId, std::vector<DutyId>, IdHash> duties_by_assignment_`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterJobDefinition(JobDefinition definition);
[[nodiscard]] foundation::Result<WorkplaceId> CreateWorkplace(Workplace workplace);
[[nodiscard]] foundation::Result<WorkScheduleId> CreateSchedule(WorkSchedule schedule);
[[nodiscard]] const JobAssignment *GetJobAssignment(JobAssignmentId id) const noexcept;
[[nodiscard]] const Workplace *GetWorkplace(WorkplaceId id) const noexcept;
[[nodiscard]] const Duty *FindDuty(DutyId id) const noexcept;
[[nodiscard]] const Duty *GetCurrentDuty(GameplayObjectRef subject) const noexcept;
[[nodiscard]] std::vector<JobAssignment> FindJobsOfSubject(GameplayObjectRef subject) const;
[[nodiscard]] std::vector<JobAssignment> FindWorkersAtWorkplace(WorkplaceId workplace) const;
[[nodiscard]] std::vector<Workplace> FindWorkplacesInArea(GameplayObjectRef area) const;
[[nodiscard]] std::vector<Duty> FindDutiesForAssignment(JobAssignmentId assignment) const;
[[nodiscard]] std::vector<Duty> FindActiveDuties() const;
[[nodiscard]] RolesJobsChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] std::uint64_t OldestChangeSequence() const noexcept;
void PruneChangesBefore(std::uint64_t sequence) noexcept;
[[nodiscard]] RolesJobsSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(RolesJobsSnapshot snapshot);
[[nodiscard]] RolesJobsDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `DutyOccurrenceKey` (строка заголовка 383)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const DutyOccurrenceKey &) const noexcept = default;
```

#### `DutyOccurrenceKeyHash` (строка заголовка 390)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const DutyOccurrenceKey &key) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterJobDefinition` | `RolesJobsService::RegisterJobDefinition` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:336` |
| `CreateWorkplace` | `RolesJobsService::CreateWorkplace` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:359` |
| `CreateSchedule` | `RolesJobsService::CreateSchedule` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:588` |
| `GetJobAssignment` | `RolesJobsService::GetJobAssignment` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:933` |
| `GetWorkplace` | `RolesJobsService::GetWorkplace` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:939` |
| `FindDuty` | `RolesJobsService::FindDuty` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:945` |
| `GetCurrentDuty` | `RolesJobsService::GetCurrentDuty` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:951` |
| `FindJobsOfSubject` | `RolesJobsService::FindJobsOfSubject` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:972` |
| `FindWorkersAtWorkplace` | `RolesJobsService::FindWorkersAtWorkplace` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:988` |
| `FindWorkplacesInArea` | `RolesJobsService::FindWorkplacesInArea` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1008` |
| `FindDutiesForAssignment` | `RolesJobsService::FindDutiesForAssignment` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1021` |
| `FindActiveDuties` | `RolesJobsService::FindActiveDuties` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1041` |
| `OldestChangeSequence` | `RolesJobsService::OldestChangeSequence` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1089` |
| `PruneChangesBefore` | `RolesJobsService::PruneChangesBefore` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1094` |
| `CaptureSnapshot` | `RolesJobsService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1100` |
| `RestoreSnapshot` | `RolesJobsService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1147` |

