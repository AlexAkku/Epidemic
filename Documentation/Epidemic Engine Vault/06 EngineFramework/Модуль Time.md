---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "time"]
source_path: "EngineFramework/BaseInfrastructure/Time"
---

# EngineFramework / Time

Модуль: **Time**. Runtime game delta и clock.

**Реализация:** `EngineFramework/BaseInfrastructure/Time/`.
**CMake targets:** `EpidemicGameplayTime`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Time/gameplay_time.h|gameplay_time.h]]: `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h`; объявленные типы: `CalendarDefinition`, `CalendarDate`, `RecurrenceKind`, `CatchUpPolicy`, `SchedulePersistence`, `CalendarPattern`, `RecurrenceRule`, `ClockDefinition`, `ClockState`, `ScheduleEntry`, `ScheduledTrigger`, `SchedulerBudget`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/time|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `RecurrenceKind` (`EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h`): `Once`, `FixedInterval`, `CalendarPattern`.
- `CatchUpPolicy` (`EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h`): `FireEach`, `FireOnce`, `SkipMissed`, `Aggregate`.
- `SchedulePersistence` (`EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h`): `Session`, `Persistent`.

### Операции и хранилища состояния

#### `GameplayTimeService`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:139`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `, CatchUpPolicy catch_up = CatchUpPolicy::FireOnce, SchedulePersistence persistence = SchedulePersistence::Session)`; `std::unordered_map<ClockId, ClockDefinition> clock_definitions_`; `std::unordered_map<ClockId, ClockState> clocks_`; `std::unordered_map<ClockId, Revision> synchronization_revisions_`; `std::set<ClockId> synchronization_rebind_required_`; `std::unordered_map<ActionTypeId, ActionTypeInfo> action_types_`; `std::unordered_map<ScheduleId, ScheduleEntry> schedules_`; `std::unordered_map<ClockId, std::set<ScheduleKey>> schedule_index_`.

#### `CalendarDate`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t year = 1`; `std::uint32_t month = 1`; `std::uint32_t day = 1`; `std::uint32_t hour = 0`; `std::uint32_t minute = 0`; `std::uint32_t second = 0`.

#### `CalendarDefinition`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t hours_per_day = 24`; `std::uint32_t days_per_month = 30`; `std::uint32_t months_per_year = 12`.

#### `CalendarPattern`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t every_days = 1`; `std::uint32_t hour = 0`; `std::uint32_t minute = 0`; `std::uint32_t second = 0`.

#### `RecurrenceRule`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:66`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `RecurrenceKind kind = RecurrenceKind::Once`.

#### `ClockDefinition`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:73`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string canonical_name`.

#### `ClockState`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:80`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool paused = false`; `std::uint32_t time_scale_milli = 1000`; `std::uint32_t fractional_milli = 0`.

#### `ScheduleEntry`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:90`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `CatchUpPolicy catch_up = CatchUpPolicy::FireOnce`; `SchedulePersistence persistence = SchedulePersistence::Session`.

#### `ScheduledTrigger`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:103`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t occurrence_count = 1`.

#### `SchedulerBudget`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:114`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t max_triggers = 10000`; `std::uint64_t max_catch_up_occurrences = 1000000`.

#### `GameplayTimeDiagnostics`

Объявлен в `EngineFramework/BaseInfrastructure/Time/include/Epidemic/GameFramework/Time/gameplay_time.h:120`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t active_schedules = 0`; `std::uint64_t persistent_schedules = 0`; `std::uint64_t emitted_triggers = 0`; `std::uint64_t cancelled_schedules = 0`; `std::uint64_t catch_up_occurrences = 0`; `std::uint64_t budget_exhaustions = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/BaseInfrastructure/Time/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp` (1057 строк): `GameplayTimeService::RegisterClock`, `Error::Create`, `ClockId::FromString`, `GameplayTimeService::RegisterAction`, `ActionTypeId::FromString`, `GameplayTimeService::AdvanceClock`, `GameplayTimeService::AdvanceTo`, `GameplayTimeService::SetPaused`, `GameplayTimeService::SetTimeScale`, `GameplayTimeService::SynchronizeClock`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.registry_frozen`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:88`
- `gameplay.invalid_clock`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:54`
- `gameplay.invalid_calendar`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:58`
- `gameplay.id_collision`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:103`
- `gameplay.already_registered`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:106`
- `clock_registration.publish`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:75`
- `gameplay.invalid_action`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:93`
- `gameplay.clock_unknown`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:777`
- `gameplay.clock_negative_delta`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:124`
- `gameplay.time_overflow`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:766`
- `gameplay.revision_exhausted`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:278`
- `gameplay.clock_rewind`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:268`
- `gameplay.clock_invalid_scale`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:235`
- `gameplay.clock_sync_revision_invalid`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:257`
- `gameplay.clock_sync_stale`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:265`
- `clock_sync.publish`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:285`
- `gameplay.calendar_unavailable`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:391`
- `gameplay.calendar_date_invalid`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:332`
- `gameplay.calendar_time_invalid`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:396`
- `gameplay.schedule_invalid_interval`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:692`
- `gameplay.schedule_calendar_unavailable`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:700`
- `gameplay.schedule_invalid_calendar_pattern`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:536`
- `gameplay.schedule_invalid_recurrence`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:460`
- `gameplay.action_unknown`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:474`
- `gameplay.schedule_owner_mismatch`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:477`
- `gameplay.schedule_revision_exhausted`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:908`
- `gameplay.schedule_id_exhausted`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:489`
- `schedule.record_publish`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:494`
- `gameplay.schedule_id_collision`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:498`
- `schedule.index_publish`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:500`
- `gameplay.schedule_index_collision`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:507`
- `gameplay.schedule_allocation_failed`: `EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:515`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp`
- `EngineRuntime/Time/tests/time_tests.cpp`: `TestGameTimeArithmeticWorks`, `TestInitialSnapshotIsStable`, `TestDeterministicAccumulationKeepsFractionalRemainder`, `TestDifferentFrameSplitsProduceSameGameTime`, `TestNegativeAdvancePreservesObservableState`, `TestRationalAccumulatorAvoidsFloatingDrift`, `TestPauseResume`, `TestInvalidTimeScaleIsRejected`, `TestTimeScaleChangesDeltaMultiplier`, `TestTimeScaleIsNormalized`, `TestTimeScaleChangeDropsFractionalRemainder`, `TestSkipDropsFractionalRemainder`, `TestTimeSkipProducesJumpEvent`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.registry_frozen` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:88`)

```cpp
86     {
   87         return foundation::Result<ActionTypeId>::Failure(foundation::Error::Create(
   88             "gameplay.registry_frozen", "schedule action registry is frozen", std::string(canonical_name)));
   89     }
   90     if (canonical_name.empty() || !owner_domain.IsValid())
```

#### `gameplay.invalid_clock` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:54`)

```cpp
52     if (canonical_name.empty())
   53         return foundation::Result<ClockId>::Failure(
   54             foundation::Error::Create("gameplay.invalid_clock", "clock canonical name must not be empty"));
   55     if (calendar.has_value() &&
   56         (calendar->hours_per_day == 0 || calendar->days_per_month == 0 || calendar->months_per_year == 0))
```

#### `gameplay.invalid_calendar` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:58`)

```cpp
56         (calendar->hours_per_day == 0 || calendar->days_per_month == 0 || calendar->months_per_year == 0))
   57         return foundation::Result<ClockId>::Failure(
   58             foundation::Error::Create("gameplay.invalid_calendar", "calendar units must be non-zero"));
   59     const ClockId id = ClockId::FromString(canonical_name);
   60     const auto found = clock_definitions_.find(id);
```

#### `gameplay.id_collision` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:103`)

```cpp
101         {
  102             return foundation::Result<ActionTypeId>::Failure(foundation::Error::Create(
  103                 "gameplay.id_collision", "action type id collision", std::string(canonical_name)));
  104         }
  105         return foundation::Result<ActionTypeId>::Failure(foundation::Error::Create(
```

#### `gameplay.already_registered` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:106`)

```cpp
104         }
  105         return foundation::Result<ActionTypeId>::Failure(foundation::Error::Create(
  106             "gameplay.already_registered", "schedule action already registered", std::string(canonical_name)));
  107     }
  108
```

#### `clock_registration.publish` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:75`)

```cpp
73     staged_clocks.emplace(id, ClockState{id, {}, {}});
   74     staged_index.emplace(id, std::set<ScheduleKey>{});
   75     detail::MaybeFailTimePublication("clock_registration.publish");
   76     clock_definitions_.swap(staged_definitions);
   77     clocks_.swap(staged_clocks);
```

#### `gameplay.invalid_action` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:93`)

```cpp
91     {
   92         return foundation::Result<ActionTypeId>::Failure(foundation::Error::Create(
   93             "gameplay.invalid_action", "action canonical name and owner domain must be valid"));
   94     }
   95
```

#### `gameplay.clock_unknown` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:777`)

```cpp
775     if (state == clocks_.end() || index == schedule_index_.end())
  776         return foundation::Result<std::vector<ScheduledTrigger>>::Failure(
  777             foundation::Error::Create("gameplay.clock_unknown", "cannot collect schedules for unknown clock"));
  778     if (budget.max_triggers == 0 || budget.max_catch_up_occurrences == 0)
  779         return foundation::Result<std::vector<ScheduledTrigger>>::Failure(
```

#### `gameplay.clock_negative_delta` (`EngineFramework/BaseInfrastructure/Time/src/gameplay_time.cpp:124`)

```cpp
122     {
  123         return foundation::Result<void>::Failure(foundation::Error::Create(
  124             "gameplay.clock_negative_delta", "gameplay clock cannot advance by negative delta"));
  125     }
  126     if (found->second.paused || real_delta.ticks == 0 || found->second.time_scale_milli == 0)
```
