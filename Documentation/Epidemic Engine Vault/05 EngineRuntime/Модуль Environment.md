---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "environment"]
source_path: "EngineRuntime/Environment"
---

# EngineRuntime / Environment

Модуль: **Environment**. Нейтральное состояние окружения.

**Реализация:** `EngineRuntime/Environment/`.
**CMake targets:** `EpidemicRuntimeEnvironment`
**Локальные test targets:** `EpidemicRuntimeEnvironmentTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Environment/climate_profile.h|climate_profile.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/climate_profile.h`; объявленные типы: `ClimateProfile`
- [[11 API/EngineRuntime/Environment/environment_projection.h|environment_projection.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_projection.h`; объявленные типы: `EnvironmentProjection`
- [[11 API/EngineRuntime/Environment/environment_runtime.h|environment_runtime.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h`; объявленные типы: `IEnvironmentQuery`, `IEnvironmentWriter`, `IEnvironmentUpdatePolicy`, `IEnvironmentRuntime`
- [[11 API/EngineRuntime/Environment/environment_services.h|environment_services.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_services.h`; объявленные типы: `EnvironmentOptions`, `EnvironmentServices`
- [[11 API/EngineRuntime/Environment/environment_snapshot.h|environment_snapshot.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_snapshot.h`; объявленные типы: `EnvironmentSnapshot`
- [[11 API/EngineRuntime/Environment/environment_update.h|environment_update.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_update.h`; объявленные типы: `EnvironmentUpdateInput`, `EnvironmentStateUpdate`
- [[11 API/EngineRuntime/Environment/season_state.h|season_state.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/season_state.h`; объявленные типы: `SeasonKind`, `SeasonState`
- [[11 API/EngineRuntime/Environment/surface_state.h|surface_state.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/surface_state.h`; объявленные типы: `SurfaceConditionKind`, `SurfaceState`
- [[11 API/EngineRuntime/Environment/weather_state.h|weather_state.h]]: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/weather_state.h`; объявленные типы: `WeatherKind`, `WeatherState`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/environment|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Environment хранит нейтральное состояние погоды, сезона, климата и поверхностей по регионам и предоставляет snapshots/projections другим systems.

#### Модель

Регион регистрируется атомарно с `WeatherState`, `SeasonState` и `ClimateProfile`. `IEnvironmentWriter` изменяет состояния, `IEnvironmentQuery` читает их, `IEnvironmentUpdatePolicy` проверяет revisioned updates, а `IEnvironmentRuntime` применяет обновления.

Climate averages описывают долгосрочный регион, а current temperature/humidity принадлежат Weather. Surface state имеет immutable region ownership.

#### Revisions

Есть global revision и per-region revision. Cross-region update не создает ложный conflict. No-op setter не увеличивает revision. Перемещение существующего `SurfaceId` в другой region не выполняется обычным `SetSurfaceState`.

#### Registration и ownership

`RegisterRegionEnvironment` публикует регион только после полной проверки weather/season/climate и reservation следующей revision. `FreezeRegistration()` идемпотентен и после freeze запрещает новые `RegionId` и новые `SurfaceId`; mutation уже зарегистрированных region/surface state остаётся разрешена. `SurfaceId` имеет immutable region ownership и не переносится обычным update.

`SurfaceState::condition` является derived output: входное значение не является authoritative и перед публикацией всегда вычисляется через `DeriveSurfaceCondition` из wetness/snow/mud/ice. Поэтому enum-domain guarantee для сохранённого surface state обеспечивается derivation, а не доверием caller-provided condition.

#### Validation и transaction semantics

Weather/season enums валидируются. Нормированные величины ограничены контрактными диапазонами, остальные числовые значения обязаны быть finite; wind speed и surface depths неотрицательны. Wind direction принимает любое finite значение и канонизируется modulo 360 перед сравнением и commit.

`ApplyUpdate` использует per-region `source_revision`. Весь batch, включая ownership и duplicate `SurfaceId`, валидируется до commit. Invalid entry, revision conflict, revision exhaustion и allocation failure сохраняют authoritative region/surface state без partial publication. Semantic no-op не увеличивает global или per-region revision.

`IEnvironmentUpdatePolicy` является внешней callback boundary. Result failure передаётся вызывающему коду как controlled failure, exception преобразуется в `environment.update_policy_exception`, а returned update проходит обычную полную `ApplyUpdate` validation до mutation.

#### Snapshot и projection

`EnvironmentSnapshot` является detached runtime observation одного региона. Surface collection сортируется по `SurfaceId`, поэтому observable order не зависит от `unordered_map` iteration. Snapshot содержит region revision, weather, season, climate, derived temperature/humidity и все surfaces региона. `EnvironmentProjection` строится из того же authoritative observation и несёт ту же region revision.

Локального restore API у Environment нет. В Goal 3 `EnvironmentSnapshot` не объявляется persistence format; persistence mapping решается отдельно в Goal 5.

#### Граница

Environment не решает, как рисовать дождь, рассчитывать навигацию или управлять поведением NPC. Он предоставляет состояние и projection. После Goal 3 и canonical evidence convergence модуль имеет статус `LOCAL_READY`; до системных Goals 5–9 он не должен называться `FROZEN`.

#### Goal 3.14 audit defect

`G3-ENV-AUDIT-001`: `FreezeRegistration()` блокировал регистрацию нового региона, но не блокировал первое появление нового `SurfaceId` через `SetSurfaceState()` или `ApplyUpdate()`. Это нарушало registration contract Goal 3.14: после freeze новые region/surface identities не публикуются, при этом runtime updates уже зарегистрированных identities остаются разрешены. Причина: freeze guard находился только в `RegisterRegionEnvironment()`. Исправление: reject нового surface до revision/allocation/mutation как в direct, так и batch path; существующий surface по-прежнему обновляется. Regression: direct и batch creation после freeze возвращают `environment.registration_frozen`, global/per-region revisions и maps не меняются, existing surface update проходит.

Источник контрактов: `Documentation/EngineRuntime/modules/environment.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `SurfaceConditionKind` (`EngineRuntime/Environment/include/Epidemic/Runtime/Environment/surface_state.h`): `Dry`, `Wet`, `Muddy`, `SnowCovered`, `Icy`, `Frozen`, `Thawing`, `Drying`.
- `SeasonKind` (`EngineRuntime/Environment/include/Epidemic/Runtime/Environment/season_state.h`): `Spring`, `Summer`, `Autumn`, `Winter`, `Transitioning`.
- `WeatherKind` (`EngineRuntime/Environment/include/Epidemic/Runtime/Environment/weather_state.h`): `Clear`, `Cloudy`, `Rain`, `Storm`, `Snow`, `Fog`, `Transitioning`.

### Операции и хранилища состояния

#### `IEnvironmentQuery`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEnvironmentQuery() = default;
[[nodiscard]] virtual foundation::Result<WeatherState> GetWeather(RegionId region) const = 0;
[[nodiscard]] virtual foundation::Result<SeasonState> GetSeason(RegionId region) const = 0;
[[nodiscard]] virtual foundation::Result<ClimateProfile> GetClimateProfile(RegionId region) const = 0;
[[nodiscard]] virtual foundation::Result<SurfaceState> GetSurfaceState(SurfaceId surface) const = 0;
[[nodiscard]] virtual foundation::Result<EnvironmentSnapshot> BuildSnapshot(RegionId region) const = 0;
[[nodiscard]] virtual foundation::Result<EnvironmentProjection> BuildProjection(RegionId region) const = 0;
[[nodiscard]] virtual std::uint64_t GetRevision() const = 0;
[[nodiscard]] virtual foundation::Result<std::uint64_t> GetRegionRevision(RegionId region) const = 0;
```

#### `IEnvironmentWriter`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h:34`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEnvironmentWriter() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterRegionEnvironment( RegionId region, WeatherState weather, SeasonState season, ClimateProfile climate) = 0;
[[nodiscard]] virtual foundation::Result<void> SetWeather(RegionId region, WeatherState weather) = 0;
[[nodiscard]] virtual foundation::Result<void> SetSeason(RegionId region, SeasonState season) = 0;
[[nodiscard]] virtual foundation::Result<void> SetClimateProfile(RegionId region, ClimateProfile climate) = 0;
[[nodiscard]] virtual foundation::Result<void> SetSurfaceState(SurfaceState state) = 0;
[[nodiscard]] virtual foundation::Result<void> ApplyUpdate(const EnvironmentStateUpdate& update) = 0;
[[nodiscard]] virtual foundation::Result<void> Update(const EnvironmentUpdateInput& input) = 0;
```

#### `IEnvironmentRuntime`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h:63`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
~IEnvironmentRuntime() override = default;
virtual void SetUpdatePolicy(std::shared_ptr<const IEnvironmentUpdatePolicy> policy) = 0;
[[nodiscard]] virtual bool IsRegistrationFrozen() const noexcept = 0;
```

#### `IEnvironmentUpdatePolicy`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h:52`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEnvironmentUpdatePolicy() = default;
```

#### `EnvironmentServices`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_services.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IEnvironmentRuntime> runtime`; `std::shared_ptr<IEnvironmentQuery> query`; `std::shared_ptr<IEnvironmentWriter> writer`.

#### `EnvironmentSnapshot`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_snapshot.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t revision = 0`; `float temperature = 0.0f`; `float humidity = 0.0f`; `std::vector<SurfaceState> surfaces`.

#### `SurfaceState`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/surface_state.h:22`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `SurfaceConditionKind condition = SurfaceConditionKind::Dry`; `float wetness = 0.0f`; `float snow_depth = 0.0f`; `float mud_depth = 0.0f`; `float ice_thickness = 0.0f`; `float temperature = 0.0f`; `std::uint64_t revision = 0`.

#### `SeasonState`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/season_state.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `SeasonKind kind = SeasonKind::Spring`; `float progress = 0.0f`.

#### `EnvironmentProjection`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_projection.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t revision = 0`; `float temperature = 0.0f`; `float humidity = 0.0f`.

#### `ClimateProfile`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/climate_profile.h:5`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `float average_temperature = 0.0f`; `float average_humidity = 0.0f`; `float average_wind_speed = 0.0f`; `float annual_precipitation = 0.0f`.

#### `WeatherState`

Объявлен в `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/weather_state.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `WeatherKind kind = WeatherKind::Clear`; `float intensity = 0.0f`; `float cloudiness = 0.0f`; `float precipitation = 0.0f`; `float wind_speed = 0.0f`; `float wind_direction_degrees = 0.0f`; `float current_temperature = 0.0f`; `float current_humidity = 0.0f`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Environment/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Environment/src/environment.cpp` (18 строк)
- `EngineRuntime/Environment/src/environment_runtime_impl.cpp` (751 строк): `Error::Create`, `EnvironmentRuntime::GetWeather`, `EnvironmentRuntime::GetSeason`, `EnvironmentRuntime::GetClimateProfile`, `EnvironmentRuntime::GetSurfaceState`, `EnvironmentRuntime::BuildSnapshot`, `EnvironmentRuntime::BuildProjection`, `EnvironmentRuntime::GetRevision`, `EnvironmentRuntime::GetRegionRevision`, `EnvironmentRuntime::RegisterRegionEnvironment`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `environment.invalid_weather_kind`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:72`
- `environment.invalid_weather`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:78`
- `environment.invalid_season_kind`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:87`
- `environment.invalid_season`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:91`
- `environment.invalid_climate`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:101`
- `environment.invalid_surface`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:189`
- `environment.invalid_region`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:695`
- `environment.invalid_surface_state`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:119`
- `environment.region_unknown`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:699`
- `environment.surface_unknown`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:194`
- `environment.registration_frozen`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:723`
- `environment.region_already_registered`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:308`
- `environment.allocation_failed`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:609`
- `environment.revision_conflict`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:468`
- `environment.duplicate_surface_update`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:550`
- `environment.invalid_delta`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:625`
- `environment.update_policy_exception`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:652`
- `environment.revision_overflow`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:714`
- `environment.surface_region_mismatch`: `EngineRuntime/Environment/src/environment_runtime_impl.cpp:727`

### Конкретные тестовые якоря

- `EngineRuntime/Environment/tests/environment_tests.cpp`: `TestUnknownRegionAndSurfaceFail`, `TestSetGetWeatherSeasonClimateByRegion`, `TestDuplicateRegionRegistrationRejected`, `TestNoOpSettersDoNotBumpRevision`, `TestSurfaceStateStoresRegionAndMixedConditions`, `TestSurfaceRegionOwnershipIsImmutable`, `TestValidationRejectsInvalidValues`, `TestPartialRegionInitializationRejected`, `TestSnapshotAndProjectionAreRevisionedCopies`, `TestUpdateWithoutPolicyDoesNotMutateSurface`, `TestOptionalPolicyCanMutateSurface`, `TestApplyUpdateRejectsRevisionConflict`, `TestBatchNormalizesWindDirection`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `environment.invalid_weather_kind` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:72`)

```cpp
70     if (!IsValid(weather.kind))
   71     {
   72         return EnvironmentFailure("environment.invalid_weather_kind", "weather kind is outside the declared enum domain");
   73     }
   74     if (!Unit(weather.intensity) || !Unit(weather.cloudiness) || !Unit(weather.precipitation) ||
```

#### `environment.invalid_weather` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:78`)

```cpp
76         !std::isfinite(weather.wind_direction_degrees))
   77     {
   78         return EnvironmentFailure("environment.invalid_weather", "weather values must be finite and normalized where required");
   79     }
   80     return foundation::Result<void>::Success();
```

#### `environment.invalid_season_kind` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:87`)

```cpp
85     if (!IsValid(season.kind))
   86     {
   87         return EnvironmentFailure("environment.invalid_season_kind", "season kind is outside the declared enum domain");
   88     }
   89     if (!Unit(season.progress))
```

#### `environment.invalid_season` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:91`)

```cpp
89     if (!Unit(season.progress))
   90     {
   91         return EnvironmentFailure("environment.invalid_season", "season progress must be in [0, 1]");
   92     }
   93     return foundation::Result<void>::Success();
```

#### `environment.invalid_climate` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:101`)

```cpp
99         !NonNegative(climate.annual_precipitation))
  100     {
  101         return EnvironmentFailure("environment.invalid_climate", "climate values must be finite and normalized where required");
  102     }
  103     return foundation::Result<void>::Success();
```

#### `environment.invalid_surface` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:189`)

```cpp
187     if (!surface.IsValid())
  188     {
  189         return EnvironmentFailureValue<SurfaceState>("environment.invalid_surface", "surface id must be valid");
  190     }
  191     const auto iterator = surface_states_.find(surface);
```

#### `environment.invalid_region` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:695`)

```cpp
693     if (!region.IsValid())
  694     {
  695         return EnvironmentFailure("environment.invalid_region", "region id must be valid");
  696     }
  697     if (!IsRegionRegistered(region))
```

#### `environment.invalid_surface_state` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:119`)

```cpp
117         !NonNegative(surface.ice_thickness) || !std::isfinite(surface.temperature))
  118     {
  119         return EnvironmentFailure("environment.invalid_surface_state", "surface wet/snow/mud/ice values must be finite and valid");
  120     }
  121     // SurfaceConditionKind is a derived output. Caller-provided condition is intentionally ignored.
```

#### `environment.region_unknown` (`EngineRuntime/Environment/src/environment_runtime_impl.cpp:699`)

```cpp
697     if (!IsRegionRegistered(region))
  698     {
  699         return EnvironmentFailure("environment.region_unknown", "region environment is not registered");
  700     }
  701     return foundation::Result<void>::Success();
```
