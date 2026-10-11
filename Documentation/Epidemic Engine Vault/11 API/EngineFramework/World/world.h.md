---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h"
---

# World: world.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/World/include/Epidemic/GameFramework/World/world.h`.

**Модуль:** [[06 EngineFramework/Модуль World|World]].
**Размер исходного файла:** 18597 байт.
**Подключение:** `#include <Epidemic/GameFramework/World/world.h>`.

## Объявления типов (лексический индекс)
`WorldRegionId`, `WorldAreaId`, `LocationId`, `WorldFeatureId`, `WorldAlterationId`, `WorldAlterationTypeId`, `WorldFeatureTypeId`, `WorldPosition`, `WorldAabb`, `WorldRegionDefinition`, `WorldAreaDefinition`, `LocationDefinition`, `ObjectPlacementRecord`, `WorldFeatureRecord`, `WorldAlterationState`, `WorldAlterationPersistence`, `WorldAlterationRecord`, `WorldAlterationUpdate`, `WorldSnapshot`, `WorldChangeKind`, `WorldChange`, `WorldChangeBatch`, `WorldDiagnostics`, `WorldService`, `WorldTransaction`, `Kind`, `Mutation`, `CellKey`, `CellHash`, `IdHash`, `TypeHash`

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
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::world
{
struct WorldRegionId
{
    GameplayObjectId value{};
    static constexpr WorldRegionId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorldRegionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorldRegionId &) const noexcept = default;
};
struct WorldAreaId
{
    GameplayObjectId value{};
    static constexpr WorldAreaId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorldAreaId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorldAreaId &) const noexcept = default;
};
struct LocationId
{
    GameplayObjectId value{};
    static constexpr LocationId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const LocationId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const LocationId &) const noexcept = default;
};
struct WorldFeatureId
{
    GameplayObjectId value{};
    static constexpr WorldFeatureId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorldFeatureId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorldFeatureId &) const noexcept = default;
};
struct WorldAlterationId
{
    GameplayObjectId value{};
    static constexpr WorldAlterationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorldAlterationId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorldAlterationId &) const noexcept = default;
};
struct WorldAlterationTypeId
{
    TypeId value{};
    static constexpr WorldAlterationTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorldAlterationTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorldAlterationTypeId &) const noexcept = default;
};
struct WorldFeatureTypeId
{
    TypeId value{};
    static constexpr WorldFeatureTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const WorldFeatureTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const WorldFeatureTypeId &) const noexcept = default;
};

struct WorldPosition
{
    std::int64_t x_mm = 0, y_mm = 0, z_mm = 0;
    [[nodiscard]] constexpr bool operator==(const WorldPosition &) const noexcept = default;
};
struct WorldAabb
{
    WorldPosition min{};
    WorldPosition max{};
    [[nodiscard]] constexpr bool Contains(WorldPosition p) const noexcept
    {
        return p.x_mm >= min.x_mm && p.x_mm <= max.x_mm && p.y_mm >= min.y_mm && p.y_mm <= max.y_mm &&
               p.z_mm >= min.z_mm && p.z_mm <= max.z_mm;
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return min.x_mm <= max.x_mm && min.y_mm <= max.y_mm && min.z_mm <= max.z_mm;
    }
    [[nodiscard]] constexpr bool operator==(const WorldAabb &) const noexcept = default;
};

struct WorldRegionDefinition
{
    WorldRegionId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    Revision revision{};
};
struct WorldAreaDefinition
{
    WorldAreaId id{};
    std::string canonical_name;
    WorldAabb bounds{};
    std::vector<WorldRegionId> regions;
    std::vector<WorldAreaId> adjacent;
    GameplayTagSet tags;
    Revision revision{};
};
struct LocationDefinition
{
    LocationId id{};
    std::string canonical_name;
    WorldPosition position{};
    std::vector<WorldRegionId> regions;
    std::vector<WorldAreaId> areas;
    std::optional<LocationId> parent_location{};
    GameplayTagSet tags;
    Revision revision{};
};
struct ObjectPlacementRecord
{
    GameplayObjectRef object{};
    std::optional<LocationId> location{};
    std::optional<WorldAreaId> area{};
    WorldPosition position{};
    GameplayContext context{};
    Revision revision{};
};
struct WorldFeatureRecord
{
    WorldFeatureId id{};
    WorldFeatureTypeId type{};
    WorldAabb bounds{};
    GameplayTagSet tags;
    Revision revision{};
    bool dynamic = false;
};

enum class WorldAlterationState
{
    Active,
    Superseded,
    Expired,
    Removed,
    Compacted
};
enum class WorldAlterationPersistence
{
    Transient,
    Session,
    Persistent
};
struct WorldAlterationRecord
{
    WorldAlterationId id{};
    WorldAlterationTypeId type{};
    GameplayObjectRef subject{};
    WorldAabb affected_area{};
    GameplayContext context{};
    GameplayTimePoint created_at{};
    WorldAlterationPersistence persistence = WorldAlterationPersistence::Persistent;
    std::optional<GameplayTimePoint> expires_at{};
    WorldAlterationState state = WorldAlterationState::Active;
    std::vector<std::byte> payload;
    Revision revision{};
};

struct WorldAlterationUpdate
{
    std::optional<WorldAabb> affected_area{};
    bool update_expires_at = false;
    std::optional<GameplayTimePoint> expires_at{};
    std::optional<WorldAlterationState> state{};
    std::optional<std::vector<std::byte>> payload{};
    std::optional<GameplayContext> context{};
};

struct WorldSnapshot
{
    std::vector<WorldFeatureRecord> dynamic_features;
    std::vector<ObjectPlacementRecord> object_placements;
    std::vector<WorldAlterationRecord> alterations;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot alteration_ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};

enum class WorldChangeKind
{
    AlterationCreated,
    AlterationUpdated,
    AlterationExpired,
    AlterationRemoved,
    AlterationCompacted,
    FeatureChanged,
    FeatureRemoved,
    ObjectPlaced,
    ObjectPlacementRemoved
};
struct WorldChange
{
    std::uint64_t sequence = 0;
    WorldChangeKind kind = WorldChangeKind::AlterationCreated;
    WorldAlterationId alteration{};
    WorldFeatureId feature{};
    GameplayObjectRef object{};
    Revision revision{};
    GameplayContext context{};
};
struct WorldChangeBatch
{
    std::vector<WorldChange> changes;
    std::uint64_t oldest_available_sequence = 0;
    bool snapshot_required = false;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct WorldDiagnostics
{
    std::uint64_t regions = 0, areas = 0, locations = 0, features = 0, active_alterations = 0,
                  persistent_alterations = 0, transactions = 0;
};

class WorldService;
class WorldTransaction
{
  public:
    WorldTransaction(WorldService &owner, GameplayContext context);
    [[nodiscard]] foundation::Result<WorldAlterationId> Create(WorldAlterationRecord record);
    [[nodiscard]] foundation::Result<void> Update(WorldAlterationId id, WorldAlterationUpdate update);
    [[nodiscard]] foundation::Result<void> Remove(WorldAlterationId id);
    [[nodiscard]] foundation::Result<void> Commit();
    void Cancel() noexcept
    {
        cancelled_ = true;
    }

  public:
    enum class Kind
    {
        Create,
        Update,
        Remove
    };
    struct Mutation
    {
        Kind kind;
        WorldAlterationRecord record{};
        WorldAlterationUpdate update{};
        WorldAlterationId id{};
    };

  private:
    WorldService *owner_{};
    GameplayContext context_{};
    MonotonicIdGenerator<GameplayObjectId> staged_alteration_ids_;
    std::vector<Mutation> mutations_;
    bool committed_ = false, cancelled_ = false;
};

class WorldService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<WorldService>;

  public:
    WorldService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.world");
    }
    [[nodiscard]] foundation::Result<void> RegisterRegion(WorldRegionDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterArea(WorldAreaDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterLocation(LocationDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterFeatureType(WorldFeatureTypeId id, std::string canonical_name);
    [[nodiscard]] foundation::Result<void> RegisterAlterationType(WorldAlterationTypeId id, std::string canonical_name);
    [[nodiscard]] foundation::Result<void> AddStaticFeature(WorldFeatureRecord feature);
    [[nodiscard]] foundation::Result<void> Freeze();
    [[nodiscard]] bool IsFrozen() const noexcept
    {
        return frozen_;
    }

    [[nodiscard]] const WorldRegionDefinition *FindRegion(WorldRegionId id) const noexcept;
    [[nodiscard]] const WorldAreaDefinition *FindArea(WorldAreaId id) const noexcept;
    [[nodiscard]] const LocationDefinition *FindLocation(LocationId id) const noexcept;
    [[nodiscard]] std::optional<WorldFeatureRecord> FindFeature(WorldFeatureId id) const;
    [[nodiscard]] std::optional<WorldAlterationRecord> FindAlteration(WorldAlterationId id) const;
    [[nodiscard]] std::optional<ObjectPlacementRecord> FindObjectPlacement(GameplayObjectRef object) const noexcept;
    [[nodiscard]] bool IsAlterationTypeRegistered(WorldAlterationTypeId id) const noexcept
    {
        return alteration_types_.contains(id);
    }

    [[nodiscard]] std::vector<WorldAreaDefinition> FindAreasAt(WorldPosition position) const;
    [[nodiscard]] std::vector<LocationDefinition> FindLocationsInArea(WorldAreaId area) const;
    [[nodiscard]] std::vector<LocationDefinition> FindChildLocations(LocationId parent) const;
    [[nodiscard]] std::vector<WorldAlterationRecord> FindAlterations(
        const WorldAabb &bounds, std::optional<WorldAlterationTypeId> type = std::nullopt) const;

    [[nodiscard]] foundation::Result<WorldFeatureId> AddDynamicFeature(WorldFeatureRecord feature);
    [[nodiscard]] foundation::Result<void> UpdateDynamicFeature(WorldFeatureId id, WorldAabb bounds,
                                                                GameplayTagSet tags, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveDynamicFeature(WorldFeatureId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> PlaceObject(ObjectPlacementRecord placement);
    [[nodiscard]] foundation::Result<void> RemoveObjectPlacement(GameplayObjectRef object,
                                                                 GameplayContext context = {});
    [[nodiscard]] std::vector<ObjectPlacementRecord> FindObjectsInArea(WorldAreaId area) const;
    [[nodiscard]] std::vector<ObjectPlacementRecord> FindObjectsInLocation(LocationId location) const;
    [[nodiscard]] WorldTransaction BeginTransaction(GameplayContext context = {})
    {
        return WorldTransaction(*this, context);
    }
    [[nodiscard]] foundation::Result<void> ExpireAlteration(WorldAlterationId id, GameplayTimePoint now,
                                                            GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SweepExpired(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompactAlteration(WorldAlterationId id, GameplayContext context = {});

    [[nodiscard]] WorldSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(WorldSnapshot snapshot);

  private:
    [[nodiscard]] std::vector<WorldChange> ChangesSinceSequence(std::uint64_t sequence) const;
    [[nodiscard]] WorldChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;

  public:
    [[nodiscard]] WorldChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
        return {journal_epoch_,
                next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max() : next_change_sequence_ - 1};
    }
    void PruneChangesBefore(std::uint64_t sequence);
    [[nodiscard]] Revision CurrentRevision() const noexcept
    {
        return revision_;
    }
    [[nodiscard]] WorldDiagnostics GetDiagnostics() const noexcept;

  private:
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    friend class WorldTransaction;
    [[nodiscard]] foundation::Result<void> CommitMutations(std::span<const WorldTransaction::Mutation> mutations,
                                                           GameplayContext context);
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    [[nodiscard]] bool CanRecordChanges(std::size_t count) const noexcept;
    [[nodiscard]] foundation::Result<void> ValidateTopology() const;
    [[nodiscard]] foundation::Result<void> ValidateAlteration(const WorldAlterationRecord &record) const;
    [[nodiscard]] foundation::Result<WorldAlterationRecord> ApplyAlterationUpdate(
        const WorldAlterationRecord &current, const WorldAlterationUpdate &update) const;
    [[nodiscard]] foundation::Result<void> SynchronizeRequestedAlterationId(WorldAlterationId id);
    void RebuildTopologyIndexes();

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
    static constexpr std::int64_t kSpatialCellMm = 100000;
    static constexpr std::uint64_t kMaxIndexedCells = 4096;
    struct IdHash;
    void IndexArea(const WorldAreaDefinition &area);
    void IndexLocation(const LocationDefinition &location);
    void BuildAlterationIndex(const std::unordered_map<WorldAlterationId, WorldAlterationRecord, IdHash> &alterations,
                              std::unordered_map<CellKey, std::vector<WorldAlterationId>, CellHash> &alteration_index,
                              std::vector<WorldAlterationId> &large_alterations) const;
    void IndexAlteration(const WorldAlterationRecord &alteration);
    void UnindexAlteration(const WorldAlterationRecord &alteration);
    [[nodiscard]] std::vector<CellKey> CellsFor(const WorldAabb &bounds) const;
    [[nodiscard]] CellKey CellFor(WorldPosition position) const noexcept;
    struct IdHash
    {
        template <class T> std::size_t operator()(const T &id) const noexcept
        {
            return std::hash<GameplayObjectId>{}(id.value);
        }
    };
    struct TypeHash
    {
        template <class T> std::size_t operator()(const T &id) const noexcept
        {
            return std::hash<TypeId>{}(id.value);
        }
    };
    std::unordered_map<WorldRegionId, WorldRegionDefinition, IdHash> regions_;
    std::unordered_map<WorldAreaId, WorldAreaDefinition, IdHash> areas_;
    std::unordered_map<LocationId, LocationDefinition, IdHash> locations_;
    std::unordered_map<WorldFeatureId, WorldFeatureRecord, IdHash> features_;
    std::unordered_map<WorldFeatureTypeId, std::string, TypeHash> feature_types_;
    std::unordered_map<WorldAlterationTypeId, std::string, TypeHash> alteration_types_;
    std::unordered_map<WorldAlterationId, WorldAlterationRecord, IdHash> alterations_;
    std::unordered_map<GameplayObjectRef, ObjectPlacementRecord> object_placements_;
    std::unordered_map<CellKey, std::vector<WorldAreaId>, CellHash> area_index_;
    std::unordered_map<WorldAreaId, std::vector<LocationId>, IdHash> locations_by_area_;
    std::unordered_map<LocationId, std::vector<LocationId>, IdHash> child_locations_;
    std::unordered_map<CellKey, std::vector<WorldAlterationId>, CellHash> alteration_index_;
    std::vector<WorldAreaId> large_areas_;
    std::vector<WorldAlterationId> large_alterations_;
    MonotonicIdGenerator<GameplayObjectId> alteration_ids_;
    Revision revision_{};
    bool frozen_ = false;
    std::deque<WorldChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    std::uint64_t last_change_sequence_ = 0;
    std::uint64_t transactions_ = 0;
};
} // namespace epidemic::gameplay::world
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `WorldAlterationState`: `Active`, `Superseded`, `Expired`, `Removed`, `Compacted`
- `WorldAlterationPersistence`: `Transient`, `Session`, `Persistent`
- `WorldChangeKind`: `AlterationCreated`, `AlterationUpdated`, `AlterationExpired`, `AlterationRemoved`, `AlterationCompacted`, `FeatureChanged`, `FeatureRemoved`, `ObjectPlaced`, `ObjectPlacementRemoved`
- `Kind`: `Create`, `Update`, `Remove`

### Состав типов и операции

#### `WorldRegionId` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
static constexpr WorldRegionId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldRegionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorldRegionId &) const noexcept = default;
```

#### `WorldAreaId` (строка заголовка 32)

Публичные сигнатуры операций:

```cpp
static constexpr WorldAreaId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldAreaId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorldAreaId &) const noexcept = default;
```

#### `LocationId` (строка заголовка 46)

Публичные сигнатуры операций:

```cpp
static constexpr LocationId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const LocationId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const LocationId &) const noexcept = default;
```

#### `WorldFeatureId` (строка заголовка 60)

Публичные сигнатуры операций:

```cpp
static constexpr WorldFeatureId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldFeatureId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorldFeatureId &) const noexcept = default;
```

#### `WorldAlterationId` (строка заголовка 74)

Публичные сигнатуры операций:

```cpp
static constexpr WorldAlterationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldAlterationId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorldAlterationId &) const noexcept = default;
```

#### `WorldAlterationTypeId` (строка заголовка 88)

Публичные сигнатуры операций:

```cpp
static constexpr WorldAlterationTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldAlterationTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorldAlterationTypeId &) const noexcept = default;
```

#### `WorldFeatureTypeId` (строка заголовка 102)

Публичные сигнатуры операций:

```cpp
static constexpr WorldFeatureTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldFeatureTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const WorldFeatureTypeId &) const noexcept = default;
```

#### `WorldPosition` (строка заголовка 117)

Публичные поля и значения по умолчанию:

- `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const WorldPosition &) const noexcept = default;
```

#### `WorldAabb` (строка заголовка 122)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool Contains(WorldPosition p) const noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const WorldAabb &) const noexcept = default;
```

#### `WorldRegionDefinition` (строка заголовка 138)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`

#### `WorldAreaDefinition` (строка заголовка 145)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::vector<WorldRegionId> regions`
- `std::vector<WorldAreaId> adjacent`
- `GameplayTagSet tags`

#### `LocationDefinition` (строка заголовка 155)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::vector<WorldRegionId> regions`
- `std::vector<WorldAreaId> areas`
- `GameplayTagSet tags`

#### `WorldFeatureRecord` (строка заголовка 175)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `bool dynamic = false`

#### `WorldAlterationRecord` (строка заголовка 199)

Публичные поля и значения по умолчанию:

- `WorldAlterationPersistence persistence = WorldAlterationPersistence::Persistent`
- `WorldAlterationState state = WorldAlterationState::Active`
- `std::vector<std::byte> payload`

#### `WorldAlterationUpdate` (строка заголовка 214)

Публичные поля и значения по умолчанию:

- `bool update_expires_at = false`

#### `WorldSnapshot` (строка заголовка 224)

Публичные поля и значения по умолчанию:

- `std::vector<WorldFeatureRecord> dynamic_features`
- `std::vector<ObjectPlacementRecord> object_placements`
- `std::vector<WorldAlterationRecord> alterations`
- `std::uint64_t change_epoch = 1`

#### `WorldChange` (строка заголовка 247)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `WorldChangeKind kind = WorldChangeKind::AlterationCreated`

#### `WorldChangeBatch` (строка заголовка 257)

Публичные поля и значения по умолчанию:

- `std::vector<WorldChange> changes`
- `std::uint64_t oldest_available_sequence = 0`
- `bool snapshot_required = false`

#### `WorldDiagnostics` (строка заголовка 267)

Публичные поля и значения по умолчанию:

- `std::uint64_t regions = 0, areas = 0, locations = 0, features = 0, active_alterations = 0, persistent_alterations = 0, transactions = 0`

#### `WorldTransaction` (строка заголовка 274)

Публичные сигнатуры операций:

```cpp
WorldTransaction(WorldService &owner, GameplayContext context);
[[nodiscard]] foundation::Result<WorldAlterationId> Create(WorldAlterationRecord record);
[[nodiscard]] foundation::Result<void> Update(WorldAlterationId id, WorldAlterationUpdate update);
[[nodiscard]] foundation::Result<void> Remove(WorldAlterationId id);
[[nodiscard]] foundation::Result<void> Commit();
void Cancel() noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `Mutation` (строка заголовка 294)

Публичные поля и значения по умолчанию:

- `Kind kind`

#### `WorldService` (строка заголовка 310)

Внутреннее состояние (не часть публичного API): `static constexpr std::size_t kChangeJournalCapacity = 4096`, `static constexpr std::int64_t kSpatialCellMm = 100000`, `static constexpr std::uint64_t kMaxIndexedCells = 4096`, `struct IdHash`, `std::unordered_map<WorldRegionId, WorldRegionDefinition, IdHash> regions_`, `std::unordered_map<WorldAreaId, WorldAreaDefinition, IdHash> areas_`, `std::unordered_map<LocationId, LocationDefinition, IdHash> locations_`, `std::unordered_map<WorldFeatureId, WorldFeatureRecord, IdHash> features_`, `std::unordered_map<WorldFeatureTypeId, std::string, TypeHash> feature_types_`.

Публичные сигнатуры операций:

```cpp
WorldService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterRegion(WorldRegionDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterArea(WorldAreaDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterLocation(LocationDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterFeatureType(WorldFeatureTypeId id, std::string canonical_name);
[[nodiscard]] foundation::Result<void> RegisterAlterationType(WorldAlterationTypeId id, std::string canonical_name);
[[nodiscard]] foundation::Result<void> AddStaticFeature(WorldFeatureRecord feature);
[[nodiscard]] foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const WorldRegionDefinition *FindRegion(WorldRegionId id) const noexcept;
[[nodiscard]] const WorldAreaDefinition *FindArea(WorldAreaId id) const noexcept;
[[nodiscard]] const LocationDefinition *FindLocation(LocationId id) const noexcept;
[[nodiscard]] std::optional<WorldFeatureRecord> FindFeature(WorldFeatureId id) const;
[[nodiscard]] std::optional<WorldAlterationRecord> FindAlteration(WorldAlterationId id) const;
[[nodiscard]] std::optional<ObjectPlacementRecord> FindObjectPlacement(GameplayObjectRef object) const noexcept;
[[nodiscard]] bool IsAlterationTypeRegistered(WorldAlterationTypeId id) const noexcept;
[[nodiscard]] std::vector<WorldAreaDefinition> FindAreasAt(WorldPosition position) const;
[[nodiscard]] std::vector<LocationDefinition> FindLocationsInArea(WorldAreaId area) const;
[[nodiscard]] std::vector<LocationDefinition> FindChildLocations(LocationId parent) const;
[[nodiscard]] std::vector<WorldAlterationRecord> FindAlterations( const WorldAabb &bounds, std::optional<WorldAlterationTypeId> type = std::nullopt) const;
[[nodiscard]] foundation::Result<WorldFeatureId> AddDynamicFeature(WorldFeatureRecord feature);
[[nodiscard]] foundation::Result<void> PlaceObject(ObjectPlacementRecord placement);
[[nodiscard]] std::vector<ObjectPlacementRecord> FindObjectsInArea(WorldAreaId area) const;
[[nodiscard]] std::vector<ObjectPlacementRecord> FindObjectsInLocation(LocationId location) const;
[[nodiscard]] WorldSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(WorldSnapshot snapshot);
[[nodiscard]] WorldChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesBefore(std::uint64_t sequence);
[[nodiscard]] Revision CurrentRevision() const noexcept;
[[nodiscard]] WorldDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `CellKey` (строка заголовка 414)

Публичные поля и значения по умолчанию:

- `std::int64_t x = 0, y = 0, z = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const CellKey &) const noexcept = default;
```

#### `CellHash` (строка заголовка 419)

Публичные сигнатуры операций:

```cpp
std::size_t operator()(const CellKey &c) const noexcept;
```

#### `IdHash` (строка заголовка 441)

Публичные сигнатуры операций:

```cpp
template <class T> std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 443)

#### `TypeHash` (строка заголовка 448)

Публичные сигнатуры операций:

```cpp
template <class T> std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 450)

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `WorldTransaction` | `WorldTransaction::WorldTransaction` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:75` |
| `Create` | `WorldTransaction::Create` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:79` |
| `Update` | `WorldTransaction::Update` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:118` |
| `Remove` | `WorldTransaction::Remove` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:137` |
| `Commit` | `WorldTransaction::Commit` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:155` |
| `WorldService` | `WorldService::WorldService` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:169` |
| `RegisterRegion` | `WorldService::RegisterRegion` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:327` |
| `RegisterArea` | `WorldService::RegisterArea` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:338` |
| `RegisterLocation` | `WorldService::RegisterLocation` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:351` |
| `RegisterFeatureType` | `WorldService::RegisterFeatureType` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:364` |
| `RegisterAlterationType` | `WorldService::RegisterAlterationType` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:374` |
| `AddStaticFeature` | `WorldService::AddStaticFeature` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:384` |
| `Freeze` | `WorldService::Freeze` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:440` |
| `FindRegion` | `WorldService::FindRegion` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:451` |
| `FindArea` | `WorldService::FindArea` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:456` |
| `FindLocation` | `WorldService::FindLocation` | `EngineFramework/GameplayWorldStateOwners/World/src/world.cpp:461` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterAlterationType`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:26`
- `RegisterFeatureType`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:27`
- `RegisterRegion`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:41`
- `RegisterArea`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:48`
- `RegisterLocation`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:56`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:67`
- `FindAreasAt`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:68`
- `FindLocationsInArea`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:69`
- `FindChildLocations`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:70`
- `Create`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:84`
- `Commit`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:88`
- `FindAlteration`: `EngineFramework/DevelopmentInfrastructure/Tests/world_tests.cpp:89`

