---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h"
---

# Environment: environment_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 3449 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/environment_runtime.h>`.

## Объявления типов (лексический индекс)
`IEnvironmentQuery`, `IEnvironmentWriter`, `IEnvironmentUpdatePolicy`, `IEnvironmentRuntime`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Environment/climate_profile.h"
#include "Epidemic/Runtime/Environment/environment_projection.h"
#include "Epidemic/Runtime/Environment/environment_snapshot.h"
#include "Epidemic/Runtime/Environment/environment_update.h"
#include "Epidemic/Runtime/Environment/season_state.h"
#include "Epidemic/Runtime/Environment/surface_state.h"
#include "Epidemic/Runtime/Environment/weather_state.h"

#include <cstdint>
#include <memory>

namespace epidemic::runtime
{
class IEnvironmentQuery
{
  public:
    virtual ~IEnvironmentQuery() = default;

    [[nodiscard]] virtual foundation::Result<WeatherState> GetWeather(RegionId region) const = 0;
    [[nodiscard]] virtual foundation::Result<SeasonState> GetSeason(RegionId region) const = 0;
    [[nodiscard]] virtual foundation::Result<ClimateProfile> GetClimateProfile(RegionId region) const = 0;
    [[nodiscard]] virtual foundation::Result<SurfaceState> GetSurfaceState(SurfaceId surface) const = 0;
    [[nodiscard]] virtual foundation::Result<EnvironmentSnapshot> BuildSnapshot(RegionId region) const = 0;
    [[nodiscard]] virtual foundation::Result<EnvironmentProjection> BuildProjection(RegionId region) const = 0;
    [[nodiscard]] virtual std::uint64_t GetRevision() const = 0;
    [[nodiscard]] virtual foundation::Result<std::uint64_t> GetRegionRevision(RegionId region) const = 0;
};

// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IEnvironmentWriter
{
  public:
    virtual ~IEnvironmentWriter() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterRegionEnvironment(
        RegionId region,
        WeatherState weather,
        SeasonState season,
        ClimateProfile climate) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetWeather(RegionId region, WeatherState weather) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetSeason(RegionId region, SeasonState season) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetClimateProfile(RegionId region, ClimateProfile climate) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetSurfaceState(SurfaceState state) = 0;
    [[nodiscard]] virtual foundation::Result<void> ApplyUpdate(const EnvironmentStateUpdate& update) = 0;
    [[nodiscard]] virtual foundation::Result<void> Update(const EnvironmentUpdateInput& input) = 0;
};

class IEnvironmentUpdatePolicy
{
  public:
    virtual ~IEnvironmentUpdatePolicy() = default;

    // Result failure and exceptions are contained by Update(). Returned state is fully validated before commit.
    [[nodiscard]] virtual foundation::Result<EnvironmentStateUpdate> BuildUpdate(
        const EnvironmentUpdateInput& input,
        const IEnvironmentQuery& query) const = 0;
};

class IEnvironmentRuntime : public IEnvironmentQuery, public IEnvironmentWriter
{
  public:
    ~IEnvironmentRuntime() override = default;

    virtual void SetUpdatePolicy(std::shared_ptr<const IEnvironmentUpdatePolicy> policy) = 0;

    // Region and surface identities are boot-static once frozen. Runtime state updates for existing identities remain available.
    virtual void FreezeRegistration() noexcept = 0;
    [[nodiscard]] virtual bool IsRegistrationFrozen() const noexcept = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Environment/climate_profile.h`, `Epidemic/Runtime/Environment/environment_projection.h`, `Epidemic/Runtime/Environment/environment_snapshot.h`, `Epidemic/Runtime/Environment/environment_update.h`, `Epidemic/Runtime/Environment/season_state.h`, `Epidemic/Runtime/Environment/surface_state.h`, `Epidemic/Runtime/Environment/weather_state.h`.

### Состав типов и операции

#### `IEnvironmentQuery` (строка заголовка 17)

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IEnvironmentWriter` (строка заголовка 34)

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IEnvironmentUpdatePolicy` (строка заголовка 52)

Публичные сигнатуры операций:

```cpp
virtual ~IEnvironmentUpdatePolicy() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IEnvironmentRuntime` (строка заголовка 63)

Публичные сигнатуры операций:

```cpp
~IEnvironmentRuntime() override = default;
virtual void SetUpdatePolicy(std::shared_ptr<const IEnvironmentUpdatePolicy> policy) = 0;
[[nodiscard]] virtual bool IsRegistrationFrozen() const noexcept = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `GetWeather` | `EnvironmentRuntime::GetWeather` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:143` |
| `GetSeason` | `EnvironmentRuntime::GetSeason` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:157` |
| `GetClimateProfile` | `EnvironmentRuntime::GetClimateProfile` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:171` |
| `GetSurfaceState` | `EnvironmentRuntime::GetSurfaceState` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:185` |
| `BuildSnapshot` | `EnvironmentRuntime::BuildSnapshot` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:199` |
| `BuildProjection` | `EnvironmentRuntime::BuildProjection` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:233` |
| `GetRevision` | `EnvironmentRuntime::GetRevision` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:249` |
| `GetRegionRevision` | `EnvironmentRuntime::GetRegionRevision` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:254` |
| `RegisterRegionEnvironment` | `EnvironmentRuntime::RegisterRegionEnvironment` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:264` |
| `SetWeather` | `EnvironmentRuntime::SetWeather` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:319` |
| `SetSeason` | `EnvironmentRuntime::SetSeason` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:348` |
| `SetClimateProfile` | `EnvironmentRuntime::SetClimateProfile` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:376` |
| `SetSurfaceState` | `EnvironmentRuntime::SetSurfaceState` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:404` |
| `ApplyUpdate` | `EnvironmentRuntime::ApplyUpdate` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:454` |
| `Update` | `EnvironmentRuntime::Update` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:617` |
| `SetUpdatePolicy` | `EnvironmentRuntime::SetUpdatePolicy` | `EngineRuntime/Environment/src/environment_runtime_impl.cpp:656` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterRegionEnvironment`: `EngineRuntime/Environment/tests/environment_tests.cpp:46`
- `GetSurfaceState`: `EngineRuntime/Environment/tests/environment_tests.cpp:71`
- `GetRegionRevision`: `EngineRuntime/Environment/tests/environment_tests.cpp:85`
- `GetWeather`: `EngineRuntime/Environment/tests/environment_tests.cpp:99`
- `BuildSnapshot`: `EngineRuntime/Environment/tests/environment_tests.cpp:101`
- `GetSeason`: `EngineRuntime/Environment/tests/environment_tests.cpp:116`
- `GetClimateProfile`: `EngineRuntime/Environment/tests/environment_tests.cpp:117`
- `GetRevision`: `EngineRuntime/Environment/tests/environment_tests.cpp:120`
- `SetSurfaceState`: `EngineRuntime/Environment/tests/environment_tests.cpp:144`
- `SetWeather`: `EngineRuntime/Environment/tests/environment_tests.cpp:162`
- `SetSeason`: `EngineRuntime/Environment/tests/environment_tests.cpp:163`
- `SetClimateProfile`: `EngineRuntime/Environment/tests/environment_tests.cpp:164`

