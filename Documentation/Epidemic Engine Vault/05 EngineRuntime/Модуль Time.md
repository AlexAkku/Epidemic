---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "time"]
source_path: "EngineRuntime/Time"
---

# EngineRuntime / Time

Модуль: **Time**. Runtime game delta и clock.

**Реализация:** `EngineRuntime/Time/`.
**CMake targets:** `EpidemicRuntimeTime`
**Локальные test targets:** `EpidemicRuntimeTimeTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Time/game_calendar.h|game_calendar.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_calendar.h`; объявленные типы: `CalendarDefinition`, `CalendarDate`
- [[11 API/EngineRuntime/Time/game_time.h|game_time.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_time.h`
- [[11 API/EngineRuntime/Time/time_events.h|time_events.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_events.h`; объявленные типы: `TimeEventKind`, `TimeEvent`
- [[11 API/EngineRuntime/Time/time_runtime.h|time_runtime.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h`; объявленные типы: `TimeAdvanceResult`, `PhaseBoundary`, `TimeOptions`, `TimeCheckpoint`, `IGameClock`, `ITimeRuntime`, `TimeServices`
- [[11 API/EngineRuntime/Time/time_scale.h|time_scale.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_scale.h`; объявленные типы: `TimeScale`
- [[11 API/EngineRuntime/Time/time_snapshot.h|time_snapshot.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_snapshot.h`; объявленные типы: `TimeSnapshot`
- [[11 API/EngineRuntime/Time/time_state.h|time_state.h]]: `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_state.h`; объявленные типы: `DayPhase`, `TimeRuntimeState`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/time|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Time превращает реальную длительность кадра в authoritative игровое время, календарь, фазу дня и события переходов. Это единственный источник game delta для Runtime.

#### Контракты

`IGameClock` выдает текущую точку, последний delta и frame-facing `TimeSnapshot`. `TimeSnapshot` является наблюдением текущего кадра и не является persistence-format.

`ITimeRuntime` выполняет `Advance`, Pause, Resume, изменение `TimeScale` и Skip. Для persistence используется `TimeCheckpoint`: он сохраняет authoritative clock state, нормализованный `TimeScale`, paused state, fractional remainder, revision и immutable configuration identity. `RestoreCheckpoint` сначала полностью проверяет candidate и совместимость конфигурации, затем выполняет один no-fail commit. Ошибка validation или allocation до commit не изменяет live clock. `last_delta`, текущий event buffer и operation-only state в checkpoint не входят.

`TimeAdvanceResult` содержит previous/current snapshot и список `TimeEvent`. `CalendarDefinition` задает длину дня, месяца и года. `PhaseBoundary` связывает минуту дня с `DayPhase`; effective phase boundaries хранятся в canonical sorted form, а пустой список использует стандартные boundaries.

#### Поведение

Дробные game ticks накапливаются integer/rational remainder без floating drift. Изменение remainder является authoritative mutation и увеличивает revision даже если целый game tick ещё не получен. Изменение scale и ненулевой ручной Skip сбрасывают remainder по зафиксированному контракту. `last_delta` является transient полем и сам по себе authoritative revision не увеличивает.

Pause/Resume и повторные no-op операции сохраняют deterministic semantics. Большой `Advance` или Skip публикует обязательные `DayChanged` и `DayPhaseChanged`, если соответствующие границы были пересечены, даже когда конечная фаза совпадает с начальной. Event order для одной операции стабилен: основной operation event, затем `DayChanged`, затем `DayPhaseChanged`.

#### Стабильность

Модуль прошёл локальный Goal 3 Time audit и canonical evidence convergence и имеет статус `LOCAL_READY` для перехода к системным Goals 5–9. Статус `FROZEN` здесь не утверждается до whole-engine qualification. Environment, Simulation и gameplay получают время через clock/result, а не рассчитывают собственную альтернативную шкалу.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `game_calendar.h`: `CalendarDefinition`, `CalendarDate`.
- `game_time.h`: alias/helpers для game time.
- `time_events.h`: `TimeEventKind`, `TimeEvent`.
- `time_runtime.h`: `TimeAdvanceResult`, `PhaseBoundary`, `TimeOptions`, `TimeCheckpoint`, `IGameClock`, `ITimeRuntime`, `TimeServices`.
- `time_scale.h`: `TimeScale`.
- `time_snapshot.h`: `TimeSnapshot`.
- `time_state.h`: `DayPhase`, `TimeRuntimeState`.

Источник контрактов: `Documentation/EngineRuntime/modules/time.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `TimeEventKind` (`EngineRuntime/Time/include/Epidemic/Runtime/Time/time_events.h`): `TimeAdvanced`, `TimeScaleChanged`, `Paused`, `Resumed`, `TimeJumped`, `DayChanged`, `DayPhaseChanged`.
- `DayPhase` (`EngineRuntime/Time/include/Epidemic/Runtime/Time/time_state.h`): `Dawn`, `Day`, `Dusk`, `Night`.
- `TimeRuntimeState` (`EngineRuntime/Time/include/Epidemic/Runtime/Time/time_state.h`): `Running`, `Paused`, `TimeScaleChanged`, `TimeJumped`, `DayChanged`, `PhaseChanged`.

### Операции и хранилища состояния

#### `ITimeRuntime`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h:67`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ITimeRuntime() = default;
[[nodiscard]] virtual foundation::Result<TimeAdvanceResult> Advance(std::chrono::microseconds real_delta) = 0;
[[nodiscard]] virtual foundation::Result<void> Pause() = 0;
[[nodiscard]] virtual foundation::Result<void> Resume() = 0;
[[nodiscard]] virtual foundation::Result<void> SetTimeScale(TimeScale scale) = 0;
[[nodiscard]] virtual foundation::Result<TimeAdvanceResult> Skip(GameDuration duration) = 0;
```

#### `IGameClock`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h:57`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IGameClock() = default;
[[nodiscard]] virtual GameTimePoint Now() const = 0;
[[nodiscard]] virtual GameDuration LastDelta() const = 0;
[[nodiscard]] virtual TimeSnapshot GetSnapshot() const = 0;
```

#### `TimeServices`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h:84`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IGameClock> clock`; `std::shared_ptr<ITimeRuntime> runtime`.

#### `TimeEvent`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_events.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `TimeEventKind kind = TimeEventKind::TimeAdvanced`.

#### `CalendarDate`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_calendar.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t year = 1`; `std::uint32_t month = 1`; `std::uint32_t day = 1`; `std::uint32_t hour = 0`; `std::uint32_t minute = 0`; `std::uint32_t second = 0`.

#### `TimeSnapshot`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_snapshot.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool paused = false`; `DayPhase day_phase = DayPhase::Night`; `std::uint64_t revision = 0`.

#### `TimeScale`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_scale.h:7`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t numerator = 1`; `std::int64_t denominator = 1`.

#### `PhaseBoundary`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h:24`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t start_minute = 0`; `DayPhase phase = DayPhase::Night`.

#### `TimeOptions`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t game_ticks_per_real_second = 1`.

#### `TimeCheckpoint`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/time_runtime.h:44`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool paused = false`; `std::int64_t tick_remainder_numerator = 0`; `std::uint64_t revision = 0`; `std::int64_t game_ticks_per_real_second = 1`.

#### `CalendarDefinition`

Объявлен в `EngineRuntime/Time/include/Epidemic/Runtime/Time/game_calendar.h:7`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t hours_per_day = 24`; `std::uint32_t days_per_month = 30`; `std::uint32_t months_per_year = 12`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Time/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Time/src/time_runtime_impl.cpp` (928 строк): `Error::Create`, `TimeRuntime::TimeRuntime`, `TimeRuntime::Now`, `TimeRuntime::LastDelta`, `TimeRuntime::GetSnapshot`, `TimeRuntime::CaptureCheckpoint`, `TimeRuntime::RestoreCheckpoint`, `TimeRuntime::Advance`, `TimeRuntime::Pause`, `TimeRuntime::Resume`
- `EngineRuntime/Time/src/time.cpp` (121 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `time.invalid_delta`: `EngineRuntime/Time/src/time_runtime_impl.cpp:336`
- `time.overflow`: `EngineRuntime/Time/src/time.cpp:75`
- `time.invalid_scale`: `EngineRuntime/Time/src/time.cpp:56`
- `time.invalid_skip`: `EngineRuntime/Time/src/time_runtime_impl.cpp:531`
- `time.invalid_options`: `EngineRuntime/Time/src/time.cpp:50`
- `time.invalid_calendar`: `EngineRuntime/Time/src/time.cpp:63`
- `time.invalid_phase`: `EngineRuntime/Time/src/time.cpp:89`
- `time.invalid_phase_boundary`: `EngineRuntime/Time/src/time.cpp:94`
- `time.duplicate_phase_boundary`: `EngineRuntime/Time/src/time.cpp:100`
- `time.incompatible_checkpoint`: `EngineRuntime/Time/src/time_runtime_impl.cpp:704`
- `time.invalid_checkpoint`: `EngineRuntime/Time/src/time_runtime_impl.cpp:737`
- `time.invalid_date`: `EngineRuntime/Time/src/time_runtime_impl.cpp:763`
- `time.revision_exhausted`: `EngineRuntime/Time/src/time_runtime_impl.cpp:800`
- `time.out_of_memory`: `EngineRuntime/Time/src/time_runtime_impl.cpp:832`

### Конкретные тестовые якоря

- `EngineRuntime/Time/tests/time_tests.cpp`: `TestGameTimeArithmeticWorks`, `TestInitialSnapshotIsStable`, `TestDeterministicAccumulationKeepsFractionalRemainder`, `TestDifferentFrameSplitsProduceSameGameTime`, `TestNegativeAdvancePreservesObservableState`, `TestRationalAccumulatorAvoidsFloatingDrift`, `TestPauseResume`, `TestInvalidTimeScaleIsRejected`, `TestTimeScaleChangesDeltaMultiplier`, `TestTimeScaleIsNormalized`, `TestTimeScaleChangeDropsFractionalRemainder`, `TestSkipDropsFractionalRemainder`, `TestTimeSkipProducesJumpEvent`
- `EngineFramework/DevelopmentInfrastructure/Tests/time_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `time.invalid_delta` (`EngineRuntime/Time/src/time_runtime_impl.cpp:336`)

```cpp
334     {
  335         return foundation::Result<TimeAdvanceResult>::Failure(
  336             MakeTimeError("time.invalid_delta", "real delta must not be negative"));
  337     }
  338
```

#### `time.overflow` (`EngineRuntime/Time/src/time.cpp:75`)

```cpp
73     {
   74         return foundation::Result<void>::Failure(
   75             MakeTimeError("time.overflow", "calendar year length overflows time conversion"));
   76     }
   77
```

#### `time.invalid_scale` (`EngineRuntime/Time/src/time.cpp:56`)

```cpp
54     {
   55         return foundation::Result<void>::Failure(
   56             MakeTimeError("time.invalid_scale", "initial time scale numerator and denominator must be positive"));
   57     }
   58
```

#### `time.invalid_skip` (`EngineRuntime/Time/src/time_runtime_impl.cpp:531`)

```cpp
529     {
  530         return foundation::Result<TimeAdvanceResult>::Failure(
  531             MakeTimeError("time.invalid_skip", "time skip duration must not be negative"));
  532     }
  533
```

#### `time.invalid_options` (`EngineRuntime/Time/src/time.cpp:50`)

```cpp
48     {
   49         return foundation::Result<void>::Failure(
   50             MakeTimeError("time.invalid_options", "game ticks per real second must be positive"));
   51     }
   52
```

#### `time.invalid_calendar` (`EngineRuntime/Time/src/time.cpp:63`)

```cpp
61     {
   62         return foundation::Result<void>::Failure(
   63             MakeTimeError("time.invalid_calendar", "calendar units must be positive"));
   64     }
   65
```

#### `time.invalid_phase` (`EngineRuntime/Time/src/time.cpp:89`)

```cpp
87         {
   88             return foundation::Result<void>::Failure(
   89                 MakeTimeError("time.invalid_phase", "day phase boundary phase is outside the DayPhase enum domain"));
   90         }
   91         if (boundaries[index].start_minute >= minutes_per_day)
```

#### `time.invalid_phase_boundary` (`EngineRuntime/Time/src/time.cpp:94`)

```cpp
92         {
   93             return foundation::Result<void>::Failure(
   94                 MakeTimeError("time.invalid_phase_boundary", "day phase boundary is outside the configured day"));
   95         }
   96
```

#### `time.duplicate_phase_boundary` (`EngineRuntime/Time/src/time.cpp:100`)

```cpp
98         {
   99             return foundation::Result<void>::Failure(
  100                 MakeTimeError("time.duplicate_phase_boundary", "day phase boundaries must have unique start minutes"));
  101         }
  102     }
```
