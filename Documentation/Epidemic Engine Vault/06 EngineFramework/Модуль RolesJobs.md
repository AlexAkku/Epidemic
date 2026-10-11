---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "roles_jobs"]
source_path: "EngineFramework/GameplayWorldStateOwners/RolesJobs"
---

# EngineFramework / RolesJobs

Модуль: **RolesJobs**. Роли и работа NPC.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/RolesJobs/`.
**CMake targets:** `EpidemicGameFrameworkRolesJobs`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/RolesJobs/roles_jobs.h|roles_jobs.h]]: `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h`; объявленные типы: `JobDefinitionId`, `JobAssignmentId`, `WorkplaceId`, `WorkScheduleId`, `WorkShiftId`, `DutyId`, `RoleFunctionId`, `JobTaskTypeId`, `IdHash`, `WorkplaceState`, `AssignmentState`, `DutyState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/RolesJobs|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Scope

Freeze unit: `EngineFramework/GameplayWorldStateOwners/RolesJobs`. Admission: 73 public callables, 80 mutation obligations, 5 lifecycle candidates, 20 stale-identity candidates, 0 external-boundary candidates.

#### Public contract review

- Role/job definitions and definition freeze semantics.
- Assignment/unassignment, duplicate assignment, capacity and eligibility.
- Workplace/job/duty lifecycle including forbidden transitions.
- Worker/job stale references and deterministic queries.
- Failed reassignment and restore are failure-atomic.
- Snapshot/restore preserves IDs, revisions, indexes and journal epoch/cursor.

#### Failure atomicity and boundary review

All revision-bearing mutations were reviewed for revision/generator exhaustion before authoritative mutation. Multi-container mutations were reviewed for rollback or staged publication. Snapshot restore builds/validates candidate state before live-state replacement. Allocation-failure evidence for this block no longer depends on the process-global Framework allocator helper; owned tests use module-local failure seams where allocation failure is evidence.

#### Persistence and deterministic reads

The module snapshot surface is treated as local in-memory persistence evidence for Goal 4. Non-empty roundtrip, invalid restore, generator/revision continuity, journal epoch/cursor behavior and failed-restore pre-state preservation are covered by the module test executable. Whole-engine ordered restore and replay remain Goal 5.

#### Test evidence

Primary regression source: `EngineFramework/DevelopmentInfrastructure/Tests/roles_jobs_tests.cpp`.
Registered target: `EpidemicGameFrameworkRolesJobsTests`.
Local Linux verification in the worker environment used direct C++23 compilation with `-Wall -Wextra -Wpedantic -Werror` and executed the module test binary. The repository top-level CMake configure is Windows-only and therefore cannot be used in this Linux worker environment.

#### Goal 4 result

Block-local review result: LOCAL_READY candidate for serial integration. This document does not claim whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN`.

Источник контрактов: `Documentation/EngineFramework/RolesJobs.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `WorkplaceState` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h`): `Active`, `Closed`, `Destroyed`, `Unavailable`, `Seasonal`, `Suspended`.
- `AssignmentState` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h`): `Active`, `Paused`, `Suspended`, `Completed`, `Cancelled`, `Invalid`.
- `DutyState` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h`): `Scheduled`, `Available`, `Active`, `Completed`, `Failed`, `Skipped`, `Cancelled`.
- `RolesJobsChangeKind` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h`): `JobAssigned`, `JobRemoved`, `JobPaused`, `JobResumed`, `WorkplaceCreated`, `WorkplaceStateChanged`, `ShiftScheduled`, `ShiftStarted`, `ShiftEnded`, `DutyCreated`, `DutyActivated`, `DutyCompleted`, `DutyFailed`, `DutySkipped` и другие значения.

### Операции и хранилища состояния

#### `RolesJobsService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:312`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<JobDefinitionId, JobDefinition, IdHash> definitions_`; `std::unordered_map<WorkplaceId, Workplace, IdHash> workplaces_`; `std::unordered_map<JobAssignmentId, JobAssignment, IdHash> assignments_`; `std::unordered_map<WorkScheduleId, WorkSchedule, IdHash> schedules_`; `std::unordered_map<DutyId, Duty, IdHash> duties_`; `std::unordered_map<GameplayObjectRef, std::vector<JobAssignmentId>> assignments_by_worker_`; `std::unordered_map<WorkplaceId, std::vector<JobAssignmentId>, IdHash> assignments_by_workplace_`; `std::unordered_map<JobAssignmentId, std::vector<WorkScheduleId>, IdHash> schedules_by_assignment_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:148`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `JobAssignmentId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr JobAssignmentId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `WorkplaceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr WorkplaceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `WorkScheduleId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:66`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr WorkScheduleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `WorkShiftId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:84`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr WorkShiftId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `DutyId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/RolesJobs/include/Epidemic/GameFramework/RolesJobs/roles_jobs.h:102`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr DutyId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/RolesJobs/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp` (1355 строк): `Error::Create`, `RolesJobsService::FindMutableDuty`, `RolesJobsService::FindMutableAssignment`, `RolesJobsService::NextRevision`, `RolesJobsService::IsWorkplaceStateOperational`, `RolesJobsService::IsWorkplaceOperational`, `RolesJobsService::IsTerminalAssignment`, `RolesJobsService::IsTerminalDuty`, `RolesJobsService::IsLiveDuty`, `RolesJobsService::HasDutyOccurrence`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.roles_jobs.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:217`
- `gameplay.roles_jobs.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:348`
- `gameplay.roles_jobs.duplicate_definition`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:343`
- `gameplay.roles_jobs.invalid_workplace`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:363`
- `gameplay.roles_jobs.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:663`
- `gameplay.roles_jobs.duplicate_workplace`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:375`
- `gameplay.roles_jobs.workplace_missing`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:394`
- `gameplay.roles_jobs.invalid_assignment`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:456`
- `gameplay.roles_jobs.workplace_not_operational`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:885`
- `gameplay.roles_jobs.invalid_assignment_state`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:550`
- `gameplay.roles_jobs.duplicate_assignment`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:474`
- `gameplay.roles_jobs.assignment_missing`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:546`
- `gameplay.roles_jobs.invalid_assignment_transition`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:552`
- `gameplay.roles_jobs.assignment_terminal`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:555`
- `gameplay.roles_jobs.invalid_schedule`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:601`
- `gameplay.roles_jobs.duplicate_schedule`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:614`
- `gameplay.roles_jobs.invalid_shift`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:627`
- `gameplay.roles_jobs.duplicate_shift`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:637`
- `gameplay.roles_jobs.duplicate_duty`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:665`
- `gameplay.roles_jobs.invalid_duty`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:733`
- `gameplay.roles_jobs.invalid_duty_state`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:873`
- `gameplay.roles_jobs.duty_missing`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:871`
- `gameplay.roles_jobs.invalid_duty_transition`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:875`
- `gameplay.roles_jobs.duty_terminal`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:878`
- `gameplay.roles_jobs.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1151`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1157`
- `gameplay.roles_jobs.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:1303`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.roles_jobs.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:217`)

```cpp
215     if (!next)
  216         return foundation::Result<Revision>::Failure(
  217             Error("gameplay.roles_jobs.revision_exhausted", "roles/jobs revision is exhausted"));
  218     return foundation::Result<Revision>::Success(*next);
  219 }
```

#### `gameplay.roles_jobs.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:348`)

```cpp
346         if (!task.IsValid())
  347             return foundation::Result<void>::Failure(
  348                 Error("gameplay.roles_jobs.invalid_definition", "invalid default job task"));
  349     }
  350     auto next_revision = NextRevision();
```

#### `gameplay.roles_jobs.duplicate_definition` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:343`)

```cpp
341     if (definitions_.contains(definition.id))
  342         return foundation::Result<void>::Failure(
  343             Error("gameplay.roles_jobs.duplicate_definition", "duplicate job definition"));
  344     for (const auto task : definition.default_tasks)
  345     {
```

#### `gameplay.roles_jobs.invalid_workplace` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:363`)

```cpp
361     if (!workplace.area.IsValid() || !IsValidWorkplaceState(workplace.state))
  362         return foundation::Result<WorkplaceId>::Failure(
  363             Error("gameplay.roles_jobs.invalid_workplace", "invalid workplace"));
  364 
  365     auto staged_ids = workplace_ids_;
```

#### `gameplay.roles_jobs.id_exhausted` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:663`)

```cpp
661         AdvanceGeneratorPastAcceptedId(staged_ids, duty.id);
  662     if (!duty.id.IsValid())
  663         return foundation::Result<DutyId>::Failure(Error("gameplay.roles_jobs.id_exhausted", "duty id is exhausted"));
  664     if (duties_.contains(duty.id))
  665         return foundation::Result<DutyId>::Failure(Error("gameplay.roles_jobs.duplicate_duty", "duplicate duty"));
```

#### `gameplay.roles_jobs.duplicate_workplace` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:375`)

```cpp
373     if (workplaces_.contains(workplace.id))
  374         return foundation::Result<WorkplaceId>::Failure(
  375             Error("gameplay.roles_jobs.duplicate_workplace", "duplicate workplace"));
  376 
  377     auto next_revision = NextRevision();
```

#### `gameplay.roles_jobs.workplace_missing` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:394`)

```cpp
392     auto it = workplaces_.find(id);
  393     if (it == workplaces_.end())
  394         return foundation::Result<void>::Failure(Error("gameplay.roles_jobs.workplace_missing", "workplace missing"));
  395 
  396     if (it->second.state == state)
```

#### `gameplay.roles_jobs.invalid_assignment` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:456`)

```cpp
454         !assignment.workplace.IsValid() || !workplaces_.contains(assignment.workplace))
  455         return foundation::Result<JobAssignmentId>::Failure(
  456             Error("gameplay.roles_jobs.invalid_assignment", "invalid job assignment"));
  457     if (!IsWorkplaceOperational(assignment.workplace))
  458         return foundation::Result<JobAssignmentId>::Failure(
```

#### `gameplay.roles_jobs.workplace_not_operational` (`EngineFramework/GameplayWorldStateOwners/RolesJobs/src/roles_jobs.cpp:885`)

```cpp
883             !IsWorkplaceOperational(assignment->second.workplace))
  884             return foundation::Result<void>::Failure(
  885                 Error("gameplay.roles_jobs.workplace_not_operational", "active duty requires active assignment and workplace"));
  886     }
  887
```
