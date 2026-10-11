---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h"
---

# Environment: environment.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Environment/include/Epidemic/GameFramework/Environment/environment.h`.

**Модуль:** [[06 EngineFramework/Модуль Environment|Environment]].
**Размер исходного файла:** 14207 байт.
**Подключение:** `#include <Epidemic/GameFramework/Environment/environment.h>`.

## Объявления типов (лексический индекс)
`EnvironmentLayerId`, `EnvironmentLayerTypeId`, `EnvironmentBlendHandlerId`, `EnvironmentPosition`, `EnvironmentAabb`, `EnvironmentBlendPolicy`, `EnvironmentPersistence`, `EnvironmentValueField`, `EnvironmentValues`, `EnvironmentHazardTypeId`, `EnvironmentHazard`, `EnvironmentLayer`, `EnvironmentSampleHazard`, `EnvironmentSample`, `EnvironmentChangeKind`, `EnvironmentChange`, `EnvironmentChangeBatch`, `EnvironmentSnapshot`, `EnvironmentDiagnostics`, `EnvironmentService`, `CellKey`, `CellHash`, `IdHash`, `TypeHash`, `HazardHash`, `BlendHash`

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
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::environment
{
struct EnvironmentLayerId
{
    GameplayObjectId value{};
    static constexpr EnvironmentLayerId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EnvironmentLayerId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EnvironmentLayerId &) const noexcept = default;
};

struct EnvironmentLayerTypeId
{
    TypeId value{};
    static constexpr EnvironmentLayerTypeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EnvironmentLayerTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EnvironmentLayerTypeId &) const noexcept = default;
};

struct EnvironmentBlendHandlerId
{
    TypeId value{};
    static constexpr EnvironmentBlendHandlerId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EnvironmentBlendHandlerId &) const noexcept = default;
};

struct EnvironmentPosition
{
    std::int64_t x_mm = 0, y_mm = 0, z_mm = 0;
    [[nodiscard]] constexpr bool operator==(const EnvironmentPosition &) const noexcept = default;
};

struct EnvironmentAabb
{
    EnvironmentPosition min{}, max{};
    [[nodiscard]] constexpr bool Contains(EnvironmentPosition p) const noexcept
    {
        return p.x_mm >= min.x_mm && p.x_mm <= max.x_mm && p.y_mm >= min.y_mm && p.y_mm <= max.y_mm &&
               p.z_mm >= min.z_mm && p.z_mm <= max.z_mm;
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return min.x_mm <= max.x_mm && min.y_mm <= max.y_mm && min.z_mm <= max.z_mm;
    }
    [[nodiscard]] constexpr bool operator==(const EnvironmentAabb &) const noexcept = default;
};

// Fixed-point environment coefficient. 1000 == 1.0. Temperature uses milli-degrees Celsius.
using EnvironmentFixed = std::int32_t;
constexpr EnvironmentFixed kEnvironmentOne = 1000;

enum class EnvironmentBlendPolicy
{
    Override,
    Add,
    Multiply,
    Min,
    Max,
    CustomRegistered
};

enum class EnvironmentPersistence
{
    Transient,
    Session,
    Persistent
};

enum class EnvironmentValueField : std::uint32_t
{
    Temperature = 1u << 0u,
    Humidity = 1u << 1u,
    Precipitation = 1u << 2u,
    WindStrength = 1u << 3u,
    WindDirection = 1u << 4u,
    Visibility = 1u << 5u,
    LightExposure = 1u << 6u,
};
using EnvironmentValueMask = std::uint32_t;
constexpr EnvironmentValueMask kEnvironmentAllValues = (1u << 7u) - 1u;

struct EnvironmentValues
{
    EnvironmentFixed temperature_milli_c = 0;
    EnvironmentFixed humidity = 0;
    EnvironmentFixed precipitation = 0;
    EnvironmentFixed wind_strength = 0;
    EnvironmentFixed wind_x = 0, wind_y = 0, wind_z = 0;
    EnvironmentFixed visibility = kEnvironmentOne;
    EnvironmentFixed light_exposure = kEnvironmentOne;
    EnvironmentValueMask present = kEnvironmentAllValues;
    [[nodiscard]] constexpr bool operator==(const EnvironmentValues &) const noexcept = default;
};

struct EnvironmentHazardTypeId
{
    TypeId value{};
    static constexpr EnvironmentHazardTypeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EnvironmentHazardTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EnvironmentHazardTypeId &) const noexcept = default;
};

struct EnvironmentHazard
{
    EnvironmentHazardTypeId type{};
    EnvironmentFixed intensity = 0;
    GameplayTagSet tags;
    [[nodiscard]] bool operator==(const EnvironmentHazard &) const noexcept = default;
};

struct EnvironmentLayer
{
    EnvironmentLayerId id{};
    EnvironmentLayerTypeId type{};
    std::optional<EnvironmentAabb> bounds{};
    GameplayObjectRef area_ref{};
    int priority = 0;
    EnvironmentBlendPolicy blend = EnvironmentBlendPolicy::Override;
    EnvironmentBlendHandlerId custom_blend{};
    EnvironmentValues values{};
    GameplayTagSet tags;
    std::vector<EnvironmentHazard> hazards;
    GameplayTimePoint created_at{};
    std::optional<GameplayTimePoint> expires_at{};
    EnvironmentPersistence persistence = EnvironmentPersistence::Session;
    Revision revision{};
    GameplayContext context{};
    [[nodiscard]] bool operator==(const EnvironmentLayer &) const noexcept = default;
};

struct EnvironmentSampleHazard
{
    EnvironmentLayerId source_layer{};
    EnvironmentHazard hazard{};
    [[nodiscard]] bool operator==(const EnvironmentSampleHazard &) const noexcept = default;
};

struct EnvironmentSample
{
    EnvironmentPosition position{};
    GameplayTimePoint time{};
    EnvironmentValues values{};
    GameplayTagSet tags;
    Revision revision{};
    std::vector<EnvironmentLayerId> layers;
    // Contributions remain separate and retain their source layer. Consumers decide how equal hazard types aggregate.
    std::vector<EnvironmentSampleHazard> hazards;
};

enum class EnvironmentChangeKind
{
    Added,
    Changed,
    Removed,
    Expired
};

struct EnvironmentChange
{
    std::uint64_t sequence = 0;
    EnvironmentChangeKind kind = EnvironmentChangeKind::Added;
    EnvironmentLayerId layer{};
    Revision revision{};
    GameplayContext context{};
};

struct EnvironmentChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    std::vector<EnvironmentChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct EnvironmentSnapshot
{
    std::vector<EnvironmentLayer> layers;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};

struct EnvironmentDiagnostics
{
    std::uint64_t layers = 0;
    std::uint64_t persistent_layers = 0;
    std::uint64_t samples = 0;
    std::uint64_t expired = 0;
    std::uint64_t changes = 0;
    std::uint64_t blend_failures = 0;
};

class EnvironmentService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<EnvironmentService>;

  public:
    EnvironmentService();

    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.environment");
    }

    // Custom handlers are trusted semantic extensions, but they remain fallible. Exceptions are converted to Result
    // failures by EnvironmentService and returned values are validated before becoming part of a sample.
    using BlendHandler =
        std::function<foundation::Result<EnvironmentValues>(const EnvironmentValues &, const EnvironmentValues &)>;

    [[nodiscard]] foundation::Result<void> RegisterLayerType(EnvironmentLayerTypeId id, std::string canonical_name);
    [[nodiscard]] foundation::Result<void> RegisterBlendHandler(EnvironmentBlendHandlerId id,
                                                                std::string canonical_name, BlendHandler handler);
    [[nodiscard]] foundation::Result<void> RegisterHazardType(EnvironmentHazardTypeId id, std::string canonical_name);

    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }

    [[nodiscard]] foundation::Result<EnvironmentLayerId> AddLayer(EnvironmentLayer layer);
    [[nodiscard]] foundation::Result<void> UpdateLayer(EnvironmentLayer layer);
    [[nodiscard]] foundation::Result<void> RemoveLayer(EnvironmentLayerId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ExpireLayer(EnvironmentLayerId id, GameplayTimePoint now,
                                                       GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SweepExpired(GameplayTimePoint now, GameplayContext context = {});

    [[nodiscard]] std::optional<EnvironmentLayer> GetLayer(EnvironmentLayerId id) const noexcept;
    [[nodiscard]] std::vector<EnvironmentLayer> FindLayers(EnvironmentPosition position) const;
    [[nodiscard]] foundation::Result<EnvironmentSample> Sample(EnvironmentPosition position, GameplayTimePoint time,
                                                               std::span<const GameplayObjectRef> scopes = {}) const;

    [[nodiscard]] EnvironmentSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(EnvironmentSnapshot snapshot);

    // ChangesSince is retained as a compatibility helper. Consumers that require loss detection must use
    // ReadChangesSince and handle snapshot_required.
    private:
        [[nodiscard]] std::vector<EnvironmentChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] EnvironmentChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] EnvironmentChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }
    [[nodiscard]] EnvironmentDiagnostics GetDiagnostics() const noexcept;

  private:
    struct CellKey
    {
        std::int64_t x = 0, y = 0, z = 0;
        [[nodiscard]] bool operator==(const CellKey &) const noexcept = default;
    };

    struct CellHash
    {
        std::size_t operator()(const CellKey &c) const noexcept
        {
            const auto a = std::hash<std::int64_t>{}(c.x);
            const auto b = std::hash<std::int64_t>{}(c.y);
            const auto d = std::hash<std::int64_t>{}(c.z);
            return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6u) + (a >> 2u)) ^ (d << 1u);
        }
    };

    struct IdHash
    {
        std::size_t operator()(EnvironmentLayerId id) const noexcept { return std::hash<GameplayObjectId>{}(id.value); }
    };

    struct TypeHash
    {
        std::size_t operator()(EnvironmentLayerTypeId id) const noexcept { return std::hash<TypeId>{}(id.value); }
    };

    struct HazardHash
    {
        std::size_t operator()(EnvironmentHazardTypeId id) const noexcept { return std::hash<TypeId>{}(id.value); }
    };

    struct BlendHash
    {
        std::size_t operator()(EnvironmentBlendHandlerId id) const noexcept { return std::hash<TypeId>{}(id.value); }
    };

    static constexpr std::int64_t kSpatialCellMm = 100000;
    static constexpr std::uint64_t kMaxIndexedCells = 4096;
    static constexpr std::size_t kChangeJournalCapacity = 4096;

    [[nodiscard]] CellKey CellFor(EnvironmentPosition p) const noexcept;
    [[nodiscard]] std::vector<CellKey> CellsFor(const EnvironmentAabb &bounds) const;
    void IndexLayer(const EnvironmentLayer &layer);
    void UnindexLayer(const EnvironmentLayer &layer) noexcept;
    void RebuildSpatialIndex();

    [[nodiscard]] foundation::Result<void> ValidateValues(const EnvironmentValues &values) const;
    [[nodiscard]] foundation::Result<void> ValidateLayer(const EnvironmentLayer &layer) const;
    [[nodiscard]] foundation::Result<void> Blend(EnvironmentValues &base, const EnvironmentLayer &layer) const;
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    [[nodiscard]] bool CanRecordChanges(std::size_t count) const noexcept;
    void Record(EnvironmentChange change) noexcept;
    void AdvanceGeneratorPast(EnvironmentLayerId id) noexcept;

    std::unordered_map<EnvironmentLayerTypeId, std::string, TypeHash> types_;
    std::unordered_map<EnvironmentBlendHandlerId, std::pair<std::string, BlendHandler>, BlendHash> blend_handlers_;
    std::unordered_map<EnvironmentHazardTypeId, std::string, HazardHash> hazard_types_;
    std::unordered_map<EnvironmentLayerId, EnvironmentLayer, IdHash> layers_;
    std::unordered_map<CellKey, std::vector<EnvironmentLayerId>, CellHash> spatial_index_;
    std::vector<EnvironmentLayerId> global_layers_;
    std::vector<EnvironmentLayerId> large_layers_;
    MonotonicIdGenerator<GameplayObjectId> ids_;
    Revision revision_{};
    bool frozen_ = false;
    mutable std::uint64_t samples_ = 0;
    mutable std::uint64_t blend_failures_ = 0;
    std::uint64_t expired_ = 0;
    std::deque<EnvironmentChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    std::uint64_t last_change_sequence_ = 0;
};
} // namespace epidemic::gameplay::environment
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `EnvironmentBlendPolicy`: `Override`, `Add`, `Multiply`, `Min`, `Max`, `CustomRegistered`
- `EnvironmentPersistence`: `Transient`, `Session`, `Persistent`
- `EnvironmentValueField`: `Temperature`, `Humidity`, `Precipitation`, `WindStrength`, `WindDirection`, `Visibility`, `LightExposure`
- `EnvironmentChangeKind`: `Added`, `Changed`, `Removed`, `Expired`

### Состав типов и операции

#### `EnvironmentLayerId` (строка заголовка 19)

Публичные сигнатуры операций:

```cpp
static constexpr EnvironmentLayerId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EnvironmentLayerId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EnvironmentLayerId &) const noexcept = default;
```

#### `EnvironmentLayerTypeId` (строка заголовка 31)

Публичные сигнатуры операций:

```cpp
static constexpr EnvironmentLayerTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EnvironmentLayerTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EnvironmentLayerTypeId &) const noexcept = default;
```

#### `EnvironmentBlendHandlerId` (строка заголовка 40)

Публичные сигнатуры операций:

```cpp
static constexpr EnvironmentBlendHandlerId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EnvironmentBlendHandlerId &) const noexcept = default;
```

#### `EnvironmentPosition` (строка заголовка 48)

Публичные поля и значения по умолчанию:

- `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const EnvironmentPosition &) const noexcept = default;
```

#### `EnvironmentAabb` (строка заголовка 54)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool Contains(EnvironmentPosition p) const noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EnvironmentAabb &) const noexcept = default;
```

#### `EnvironmentValues` (строка заголовка 103)

Публичные поля и значения по умолчанию:

- `EnvironmentFixed temperature_milli_c = 0`
- `EnvironmentFixed humidity = 0`
- `EnvironmentFixed precipitation = 0`
- `EnvironmentFixed wind_strength = 0`
- `EnvironmentFixed wind_x = 0, wind_y = 0, wind_z = 0`
- `EnvironmentFixed visibility = kEnvironmentOne`
- `EnvironmentFixed light_exposure = kEnvironmentOne`
- `EnvironmentValueMask present = kEnvironmentAllValues`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const EnvironmentValues &) const noexcept = default;
```

#### `EnvironmentHazardTypeId` (строка заголовка 116)

Публичные сигнатуры операций:

```cpp
static constexpr EnvironmentHazardTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const EnvironmentHazardTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EnvironmentHazardTypeId &) const noexcept = default;
```

#### `EnvironmentHazard` (строка заголовка 125)

Публичные поля и значения по умолчанию:

- `EnvironmentFixed intensity = 0`
- `GameplayTagSet tags`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const EnvironmentHazard &) const noexcept = default;
```

#### `EnvironmentLayer` (строка заголовка 133)

Публичные поля и значения по умолчанию:

- `int priority = 0`
- `EnvironmentBlendPolicy blend = EnvironmentBlendPolicy::Override`
- `GameplayTagSet tags`
- `std::vector<EnvironmentHazard> hazards`
- `EnvironmentPersistence persistence = EnvironmentPersistence::Session`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const EnvironmentLayer &) const noexcept = default;
```

#### `EnvironmentSampleHazard` (строка заголовка 153)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const EnvironmentSampleHazard &) const noexcept = default;
```

#### `EnvironmentSample` (строка заголовка 160)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `std::vector<EnvironmentLayerId> layers`

#### `EnvironmentChange` (строка заголовка 180)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `EnvironmentChangeKind kind = EnvironmentChangeKind::Added`

#### `EnvironmentChangeBatch` (строка заголовка 189)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `std::vector<EnvironmentChange> changes`

#### `EnvironmentSnapshot` (строка заголовка 200)

Публичные поля и значения по умолчанию:

- `std::vector<EnvironmentLayer> layers`
- `std::uint64_t change_epoch = 1`

#### `EnvironmentDiagnostics` (строка заголовка 209)

Публичные поля и значения по умолчанию:

- `std::uint64_t layers = 0`
- `std::uint64_t persistent_layers = 0`
- `std::uint64_t samples = 0`
- `std::uint64_t expired = 0`
- `std::uint64_t changes = 0`
- `std::uint64_t blend_failures = 0`

#### `EnvironmentService` (строка заголовка 219)

Внутреннее состояние (не часть публичного API): `static constexpr std::int64_t kSpatialCellMm = 100000`, `static constexpr std::uint64_t kMaxIndexedCells = 4096`, `static constexpr std::size_t kChangeJournalCapacity = 4096`, `std::unordered_map<EnvironmentLayerTypeId, std::string, TypeHash> types_`, `std::unordered_map<EnvironmentBlendHandlerId, std::pair<std::string, BlendHandler>, BlendHash> `, `std::unordered_map<EnvironmentHazardTypeId, std::string, HazardHash> hazard_types_`, `std::unordered_map<EnvironmentLayerId, EnvironmentLayer, IdHash> layers_`, `std::unordered_map<CellKey, std::vector<EnvironmentLayerId>, CellHash> spatial_index_`, `std::vector<EnvironmentLayerId> global_layers_`.

Публичные сигнатуры операций:

```cpp
EnvironmentService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterLayerType(EnvironmentLayerTypeId id, std::string canonical_name);
[[nodiscard]] foundation::Result<void> RegisterBlendHandler(EnvironmentBlendHandlerId id, std::string canonical_name, BlendHandler handler);
[[nodiscard]] foundation::Result<void> RegisterHazardType(EnvironmentHazardTypeId id, std::string canonical_name);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] foundation::Result<EnvironmentLayerId> AddLayer(EnvironmentLayer layer);
[[nodiscard]] foundation::Result<void> UpdateLayer(EnvironmentLayer layer);
[[nodiscard]] std::optional<EnvironmentLayer> GetLayer(EnvironmentLayerId id) const noexcept;
[[nodiscard]] std::vector<EnvironmentLayer> FindLayers(EnvironmentPosition position) const;
[[nodiscard]] EnvironmentSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(EnvironmentSnapshot snapshot);
[[nodiscard]] EnvironmentChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
[[nodiscard]] EnvironmentDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `CellKey` (строка заголовка 287)

Публичные поля и значения по умолчанию:

- `std::int64_t x = 0, y = 0, z = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const CellKey &) const noexcept = default;
```

#### `CellHash` (строка заголовка 293)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(const CellKey &c) const noexcept;
```

#### `IdHash` (строка заголовка 304)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(EnvironmentLayerId id) const noexcept;
```

#### `TypeHash` (строка заголовка 309)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(EnvironmentLayerTypeId id) const noexcept;
```

#### `HazardHash` (строка заголовка 314)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(EnvironmentHazardTypeId id) const noexcept;
```

#### `BlendHash` (строка заголовка 319)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(EnvironmentBlendHandlerId id) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `EnvironmentService` | `EnvironmentService::EnvironmentService` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:96` |
| `RegisterLayerType` | `EnvironmentService::RegisterLayerType` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:304` |
| `RegisterBlendHandler` | `EnvironmentService::RegisterBlendHandler` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:315` |
| `RegisterHazardType` | `EnvironmentService::RegisterHazardType` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:327` |
| `AddLayer` | `EnvironmentService::AddLayer` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:350` |
| `UpdateLayer` | `EnvironmentService::UpdateLayer` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:416` |
| `GetLayer` | `EnvironmentService::GetLayer` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:553` |
| `FindLayers` | `EnvironmentService::FindLayers` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:559` |
| `CaptureSnapshot` | `EnvironmentService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:710` |
| `RestoreSnapshot` | `EnvironmentService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:723` |
| `GetDiagnostics` | `EnvironmentService::GetDiagnostics` | `EngineFramework/GameplayWorldStateOwners/Environment/src/environment.cpp:826` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterLayerType`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:23`
- `RegisterHazardType`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:25`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:49`
- `AddLayer`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:61`
- `CurrentRevision`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:169`
- `UpdateLayer`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:188`
- `RegisterBlendHandler`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:214`
- `GetDiagnostics`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:228`
- `LatestChangeCursor`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:246`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:248`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:253`
- `GetLayer`: `EngineFramework/DevelopmentInfrastructure/Tests/environment_gameplay_tests.cpp:254`

