---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h"
---

# ResourcesProduction: resources_production.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`.

**Модуль:** [[06 EngineFramework/Модуль ResourcesProduction|ResourcesProduction]].
**Размер исходного файла:** 22624 байт.
**Подключение:** `#include <Epidemic/GameFramework/ResourcesProduction/resources_production.h>`.

## Объявления типов (лексический индекс)
`ResourceTypeId`, `ResourceUnitId`, `ResourceStockpileId`, `ResourceNodeId`, `ProductionSiteId`, `ProductionRecipeId`, `ResourceTransactionId`, `ResourceSourceId`, `ResourceSinkId`, `ResourceReservationId`, `ProductionCapabilityId`, `ProductionPlanId`, `IdHash`, `ResourceStoragePolicy`, `ResourceDecayPolicy`, `StockpileState`, `ResourceNodeState`, `ProductionSiteState`, `ResourceReservationState`, `ProductionPlanState`, `ResourceChangeKind`, `ResourceType`, `ResourceQuantity`, `ResourceStockpile`, `ResourceNode`, `ProductionSite`, `ProductionRecipe`, `ResourceReservation`, `ProductionCapability`, `ProductionPlan`, `ResourceTransaction`, `ResourceChange`, `ResourceChangeBatch`, `ResourcesSnapshot`, `ResourcesDiagnostics`, `ResourcesProductionService`, `AmountKey`, `AmountKeyHash`, `StockpileOperation`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::resources
{
using Fixed = std::int64_t;
struct ResourceTypeId
{
    TypeId value{};
    static constexpr ResourceTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceTypeId &) const noexcept = default;
};
struct ResourceUnitId
{
    TypeId value{};
    static constexpr ResourceUnitId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceUnitId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceUnitId &) const noexcept = default;
};
struct ResourceStockpileId
{
    GameplayObjectId value{};
    static constexpr ResourceStockpileId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceStockpileId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceStockpileId &) const noexcept = default;
};
struct ResourceNodeId
{
    GameplayObjectId value{};
    static constexpr ResourceNodeId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceNodeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceNodeId &) const noexcept = default;
};
struct ProductionSiteId
{
    GameplayObjectId value{};
    static constexpr ProductionSiteId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProductionSiteId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProductionSiteId &) const noexcept = default;
};
struct ProductionRecipeId
{
    TypeId value{};
    static constexpr ProductionRecipeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProductionRecipeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProductionRecipeId &) const noexcept = default;
};
struct ResourceTransactionId
{
    GameplayObjectId value{};
    static constexpr ResourceTransactionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceTransactionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceTransactionId &) const noexcept = default;
};
struct ResourceSourceId
{
    GameplayObjectId value{};
    static constexpr ResourceSourceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceSourceId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceSourceId &) const noexcept = default;
};
struct ResourceSinkId
{
    GameplayObjectId value{};
    static constexpr ResourceSinkId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceSinkId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceSinkId &) const noexcept = default;
};
struct ResourceReservationId
{
    GameplayObjectId value{};
    static constexpr ResourceReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ResourceReservationId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ResourceReservationId &) const noexcept = default;
};
struct ProductionCapabilityId
{
    GameplayObjectId value{};
    static constexpr ProductionCapabilityId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProductionCapabilityId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProductionCapabilityId &) const noexcept = default;
};
struct ProductionPlanId
{
    GameplayObjectId value{};
    static constexpr ProductionPlanId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProductionPlanId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProductionPlanId &) const noexcept = default;
};
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

enum class ResourceStoragePolicy
{
    Abstract,
    Stockpile,
    Node,
    Virtual
};
enum class ResourceDecayPolicy
{
    None,
    Linear,
    External
};
enum class StockpileState
{
    Active,
    Locked,
    Disabled,
    Destroyed
};
enum class ResourceNodeState
{
    Active,
    Depleted,
    Disabled,
    Regenerating
};
enum class ProductionSiteState
{
    Active,
    Paused,
    Disabled,
    Destroyed
};
enum class ResourceReservationState
{
    Active,
    Consumed,
    Released
};
enum class ProductionPlanState
{
    Planned,
    Active,
    Satisfied,
    Paused,
    Cancelled
};
enum class ResourceChangeKind
{
    StockpileCreated,
    ResourceAdded,
    ResourceRemoved,
    ResourceTransferred,
    ResourceReserved,
    ResourceReservationReleased,
    ResourceReservationConsumed,
    StockpileStateChanged,
    NodeCreated,
    NodeDepleted,
    NodeRegenerated,
    ProductionSiteCreated,
    ProductionCapabilityCreated,
    ProductionPlanCreated,
    ProductionPlanChanged,
    ShortageDetected,
    SurplusDetected
};

struct ResourceType
{
    ResourceTypeId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    ResourceUnitId unit{};
    ResourceStoragePolicy storage_policy = ResourceStoragePolicy::Stockpile;
    ResourceDecayPolicy decay_policy = ResourceDecayPolicy::None;
    Revision revision{};
};
struct ResourceQuantity
{
    ResourceTypeId type{};
    Fixed amount = 0;
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return type.IsValid() && amount >= 0;
    }
};
struct ResourceStockpile
{
    ResourceStockpileId id{};
    GameplayObjectRef owner{};
    GameplayObjectRef location_object{};
    GameplayObjectRef area{};
    StockpileState state = StockpileState::Active;
    Revision revision{};
};
struct ResourceNode
{
    ResourceNodeId id{};
    ResourceTypeId type{};
    GameplayObjectRef area{};
    Fixed remaining_amount = 0;
    Fixed regeneration_rate_per_tick = 0;
    ResourceNodeState state = ResourceNodeState::Active;
    Revision revision{};
    Fixed maximum_amount = 0;
    GameplayTimePoint last_regenerated_at{};
};
struct ProductionSite
{
    ProductionSiteId id{};
    GameplayObjectRef site_object{};
    GameplayObjectRef area{};
    GameplayTagSet capabilities;
    ProductionSiteState state = ProductionSiteState::Active;
    Fixed efficiency_micro = 1'000'000;
    Revision revision{};
};
struct ProductionRecipe
{
    ProductionRecipeId id{};
    std::string canonical_name;
    std::vector<ResourceQuantity> inputs;
    std::vector<ResourceQuantity> outputs;
    GameplayDuration duration{};
    Revision revision{};
};
struct ResourceReservation
{
    ResourceReservationId id{};
    ResourceStockpileId stockpile{};
    std::vector<ResourceQuantity> quantities;
    GameplayObjectRef owner{};
    TypeId reason{};
    ResourceReservationState state = ResourceReservationState::Active;
    Revision revision{};
};
struct ProductionCapability
{
    ProductionCapabilityId id{};
    GameplayObjectRef site{};
    GameplayTagSet process_tags;
    Fixed capacity = 0;
    Revision revision{};
};
struct ProductionPlan
{
    ProductionPlanId id{};
    GameplayObjectRef owner{};
    ResourceTypeId desired_output{};
    Fixed target_quantity = 0;
    std::int32_t priority = 0;
    ProductionPlanState state = ProductionPlanState::Planned;
    Revision revision{};
};
struct ResourceTransaction
{
    ResourceTransactionId id{};
    ResourceStockpileId from{};
    ResourceStockpileId to{};
    std::vector<ResourceQuantity> quantities;
    TypeId reason{};
    GameplayTimePoint time{};
    GameplayContext context{};
    Revision revision{};
};
struct ResourceChange
{
    std::uint64_t sequence = 0;
    ResourceChangeKind kind = ResourceChangeKind::ResourceAdded;
    ResourceStockpileId stockpile{};
    ResourceTypeId resource{};
    ResourceReservationId reservation{};
    Fixed amount = 0;
    GameplayTimePoint time{};
    GameplayContext context{};
    Revision revision{};
    ResourceNodeId node{};
};
struct ResourceChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    std::vector<ResourceChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct ResourcesSnapshot
{
    std::vector<ResourceStockpile> stockpiles;
    std::vector<std::pair<ResourceStockpileId, ResourceQuantity>> amounts;
    std::vector<ResourceNode> nodes;
    std::vector<ProductionSite> sites;
    std::vector<ResourceReservation> reservations;
    std::vector<ProductionCapability> capabilities;
    std::vector<ProductionPlan> plans;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot stockpile_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot node_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot site_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot reservation_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot capability_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot plan_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot transaction_ids{};
    Revision revision{};
    std::vector<ResourceTransaction> transactions;

    std::uint64_t change_epoch = 1;
};
struct ResourcesDiagnostics
{
    std::uint64_t stockpiles = 0;
    std::uint64_t resource_records = 0;
    std::uint64_t nodes = 0;
    std::uint64_t production_sites = 0;
    std::uint64_t active_reservations = 0;
    std::uint64_t production_capabilities = 0;
    std::uint64_t production_plans = 0;
    std::uint64_t transactions = 0;
    std::uint64_t shortages = 0;
};

class ResourcesProductionService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<ResourcesProductionService>;

  public:
    ResourcesProductionService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.resources_production");
    }
    [[nodiscard]] foundation::Result<ResourceTypeId> RegisterResourceType(ResourceType type);
    [[nodiscard]] foundation::Result<ProductionRecipeId> RegisterProductionRecipe(ProductionRecipe recipe);
    void Freeze() noexcept
    {
        frozen_ = true;
    }
    [[nodiscard]] const ResourceType *FindResourceType(ResourceTypeId id) const noexcept;
    [[nodiscard]] const ProductionRecipe *FindProductionRecipe(ProductionRecipeId id) const noexcept;

    [[nodiscard]] foundation::Result<ResourceStockpileId> CreateStockpile(ResourceStockpile stockpile);
    [[nodiscard]] foundation::Result<ResourceNodeId> CreateNode(ResourceNode node);
    [[nodiscard]] foundation::Result<ProductionSiteId> CreateProductionSite(ProductionSite site);
    [[nodiscard]] const ResourceStockpile *FindStockpile(ResourceStockpileId id) const noexcept;
    [[nodiscard]] foundation::Result<void> SetStockpileState(ResourceStockpileId stockpile, StockpileState state,
                                                             GameplayContext context = {});
    [[nodiscard]] Fixed GetAmount(ResourceStockpileId stockpile, ResourceTypeId type) const noexcept;
    [[nodiscard]] Fixed GetReservedAmount(ResourceStockpileId stockpile, ResourceTypeId type) const noexcept;
    [[nodiscard]] Fixed GetAvailableAmount(ResourceStockpileId stockpile, ResourceTypeId type) const noexcept;
    [[nodiscard]] foundation::Result<void> Add(ResourceStockpileId stockpile, ResourceQuantity quantity,
                                               GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Remove(ResourceStockpileId stockpile, ResourceQuantity quantity,
                                                  GameplayContext context = {});
    [[nodiscard]] foundation::Result<ResourceTransactionId> Transfer(ResourceStockpileId from, ResourceStockpileId to,
                                                                     std::vector<ResourceQuantity> quantities,
                                                                     TypeId reason = {}, GameplayContext context = {});
    [[nodiscard]] bool CanReserve(ResourceStockpileId stockpile,
                                  std::span<const ResourceQuantity> quantities) const;
    [[nodiscard]] foundation::Result<ResourceReservationId> Reserve(ResourceStockpileId stockpile,
                                                                    std::vector<ResourceQuantity> quantities,
                                                                    GameplayObjectRef owner = {}, TypeId reason = {},
                                                                    GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ReleaseReservation(ResourceReservationId reservation,
                                                              GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ConsumeReservation(ResourceReservationId reservation,
                                                              GameplayContext context = {});
    [[nodiscard]] const ResourceReservation *FindReservation(ResourceReservationId reservation) const noexcept;

    [[nodiscard]] foundation::Result<void> DepleteNode(ResourceNodeId node, Fixed amount, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RegenerateNode(ResourceNodeId node, GameplayDuration elapsed,
                                                          GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RegenerateNode(ResourceNodeId node, GameplayTimePoint to,
                                                          GameplayContext context = {});

    [[nodiscard]] foundation::Result<ProductionCapabilityId> CreateProductionCapability(
        ProductionCapability capability);
    [[nodiscard]] const ProductionCapability *FindProductionCapability(ProductionCapabilityId id) const noexcept;
    [[nodiscard]] foundation::Result<ProductionPlanId> CreateProductionPlan(ProductionPlan plan);
    [[nodiscard]] foundation::Result<void> SetProductionPlanState(ProductionPlanId plan, ProductionPlanState state,
                                                                  GameplayContext context = {});
    [[nodiscard]] const ProductionPlan *FindProductionPlan(ProductionPlanId id) const noexcept;
    [[nodiscard]] std::vector<ProductionPlan> FindProductionPlans(GameplayObjectRef owner = {}) const;

    private:
        [[nodiscard]] std::vector<ResourceChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] ResourceChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] ResourceChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] ResourcesSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(ResourcesSnapshot snapshot);
    [[nodiscard]] ResourcesDiagnostics GetDiagnostics() const noexcept;

  private:
    struct AmountKey
    {
        ResourceStockpileId stockpile{};
        ResourceTypeId type{};
        [[nodiscard]] constexpr bool operator==(const AmountKey &) const noexcept = default;
    };
    struct AmountKeyHash
    {
        [[nodiscard]] std::size_t operator()(const AmountKey &k) const noexcept
        {
            const auto a = std::hash<GameplayObjectId>{}(k.stockpile.value);
            const auto b = std::hash<TypeId>{}(k.type.value);
            return a ^ (b + 0x9E3779B97F4A7C15ull + (a << 6u) + (a >> 2u));
        }
    };
    [[nodiscard]] bool Bump() noexcept
    {
        if (revision_.value == std::numeric_limits<std::uint64_t>::max())
            return false;
        ++revision_.value;
        return true;
    }
    enum class StockpileOperation
    {
        Add,
        Remove,
        Reserve,
        TransferSource,
        TransferDestination
    };
    void Record(ResourceChange change);
    [[nodiscard]] foundation::Result<void> ValidateQuantities(std::span<const ResourceQuantity> quantities) const;
    [[nodiscard]] foundation::Result<std::vector<ResourceQuantity>> CanonicalizeQuantities(
        std::span<const ResourceQuantity> quantities) const;
    [[nodiscard]] foundation::Result<void> ValidateStockpileOperation(ResourceStockpileId stockpile,
                                                                      StockpileOperation operation) const;
    [[nodiscard]] bool CanAddToStockpile(ResourceStockpileId stockpile) const noexcept;
    [[nodiscard]] bool CanRemoveFromStockpile(ResourceStockpileId stockpile) const noexcept;
    [[nodiscard]] static std::uint64_t LowPart(GameplayObjectId id) noexcept;
    void AddReservedIndex(ResourceStockpileId stockpile, ResourceTypeId type, Fixed amount);
    void RemoveReservedIndex(ResourceStockpileId stockpile, ResourceTypeId type, Fixed amount) noexcept;
    void RebuildDerivedState();

    bool frozen_ = false;
    Revision revision_{};
    std::unordered_map<ResourceTypeId, ResourceType, IdHash> types_;
    std::unordered_map<ResourceStockpileId, ResourceStockpile, IdHash> stockpiles_;
    std::unordered_map<AmountKey, Fixed, AmountKeyHash> amounts_;
    std::unordered_map<AmountKey, Fixed, AmountKeyHash> reserved_amounts_;
    std::unordered_map<ResourceNodeId, ResourceNode, IdHash> nodes_;
    std::unordered_map<ProductionSiteId, ProductionSite, IdHash> sites_;
    std::unordered_map<ResourceReservationId, ResourceReservation, IdHash> reservations_;
    std::unordered_map<ProductionCapabilityId, ProductionCapability, IdHash> capabilities_;
    std::unordered_map<ProductionPlanId, ProductionPlan, IdHash> plans_;
    std::unordered_map<ProductionRecipeId, ProductionRecipe, IdHash> recipes_;
    std::unordered_map<ResourceTransactionId, ResourceTransaction, IdHash> transactions_;
    MonotonicIdGenerator<GameplayObjectId> stockpile_ids_;
    MonotonicIdGenerator<GameplayObjectId> node_ids_;
    MonotonicIdGenerator<GameplayObjectId> site_ids_;
    MonotonicIdGenerator<GameplayObjectId> reservation_ids_{0x3110};
    MonotonicIdGenerator<GameplayObjectId> capability_ids_{0x3111};
    MonotonicIdGenerator<GameplayObjectId> plan_ids_{0x3112};
    MonotonicIdGenerator<GameplayObjectId> transaction_ids_;
    std::deque<ResourceChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    std::size_t change_retention_ = 4096;
    ResourcesDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::resources
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `ResourceStoragePolicy`: `Abstract`, `Stockpile`, `Node`, `Virtual`
- `ResourceDecayPolicy`: `None`, `Linear`, `External`
- `StockpileState`: `Active`, `Locked`, `Disabled`, `Destroyed`
- `ResourceNodeState`: `Active`, `Depleted`, `Disabled`, `Regenerating`
- `ProductionSiteState`: `Active`, `Paused`, `Disabled`, `Destroyed`
- `ResourceReservationState`: `Active`, `Consumed`, `Released`
- `ProductionPlanState`: `Planned`, `Active`, `Satisfied`, `Paused`, `Cancelled`
- `ResourceChangeKind`: `StockpileCreated`, `ResourceAdded`, `ResourceRemoved`, `ResourceTransferred`, `ResourceReserved`, `ResourceReservationReleased`, `ResourceReservationConsumed`, `StockpileStateChanged`, `NodeCreated`, `NodeDepleted`, `NodeRegenerated`, `ProductionSiteCreated`, `ProductionCapabilityCreated`, `ProductionPlanCreated`, `ProductionPlanChanged`, `ShortageDetected`, `SurplusDetected`
- `StockpileOperation`: `Add`, `Remove`, `Reserve`, `TransferSource`, `TransferDestination`

### Состав типов и операции

#### `ResourceTypeId` (строка заголовка 20)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceTypeId &) const noexcept = default;
```

#### `ResourceUnitId` (строка заголовка 34)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceUnitId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceUnitId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceUnitId &) const noexcept = default;
```

#### `ResourceStockpileId` (строка заголовка 48)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceStockpileId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceStockpileId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceStockpileId &) const noexcept = default;
```

#### `ResourceNodeId` (строка заголовка 62)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceNodeId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceNodeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceNodeId &) const noexcept = default;
```

#### `ProductionSiteId` (строка заголовка 76)

Публичные сигнатуры операций:

```cpp
static constexpr ProductionSiteId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProductionSiteId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProductionSiteId &) const noexcept = default;
```

#### `ProductionRecipeId` (строка заголовка 90)

Публичные сигнатуры операций:

```cpp
static constexpr ProductionRecipeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProductionRecipeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProductionRecipeId &) const noexcept = default;
```

#### `ResourceTransactionId` (строка заголовка 104)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceTransactionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceTransactionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceTransactionId &) const noexcept = default;
```

#### `ResourceSourceId` (строка заголовка 118)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceSourceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceSourceId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceSourceId &) const noexcept = default;
```

#### `ResourceSinkId` (строка заголовка 132)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceSinkId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceSinkId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceSinkId &) const noexcept = default;
```

#### `ResourceReservationId` (строка заголовка 146)

Публичные сигнатуры операций:

```cpp
static constexpr ResourceReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceReservationId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ResourceReservationId &) const noexcept = default;
```

#### `ProductionCapabilityId` (строка заголовка 160)

Публичные сигнатуры операций:

```cpp
static constexpr ProductionCapabilityId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProductionCapabilityId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProductionCapabilityId &) const noexcept = default;
```

#### `ProductionPlanId` (строка заголовка 174)

Публичные сигнатуры операций:

```cpp
static constexpr ProductionPlanId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProductionPlanId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProductionPlanId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 188)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 190)

#### `ResourceType` (строка заголовка 265)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `ResourceStoragePolicy storage_policy = ResourceStoragePolicy::Stockpile`
- `ResourceDecayPolicy decay_policy = ResourceDecayPolicy::None`

#### `ResourceQuantity` (строка заголовка 275)

Публичные поля и значения по умолчанию:

- `Fixed amount = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
```

#### `ResourceStockpile` (строка заголовка 284)

Публичные поля и значения по умолчанию:

- `StockpileState state = StockpileState::Active`

#### `ResourceNode` (строка заголовка 293)

Публичные поля и значения по умолчанию:

- `Fixed remaining_amount = 0`
- `Fixed regeneration_rate_per_tick = 0`
- `ResourceNodeState state = ResourceNodeState::Active`
- `Fixed maximum_amount = 0`

#### `ProductionSite` (строка заголовка 305)

Публичные поля и значения по умолчанию:

- `GameplayTagSet capabilities`
- `ProductionSiteState state = ProductionSiteState::Active`
- `Fixed efficiency_micro = 1'000'000`

#### `ProductionRecipe` (строка заголовка 315)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::vector<ResourceQuantity> inputs`
- `std::vector<ResourceQuantity> outputs`

#### `ResourceReservation` (строка заголовка 324)

Публичные поля и значения по умолчанию:

- `std::vector<ResourceQuantity> quantities`
- `ResourceReservationState state = ResourceReservationState::Active`

#### `ProductionCapability` (строка заголовка 334)

Публичные поля и значения по умолчанию:

- `GameplayTagSet process_tags`
- `Fixed capacity = 0`

#### `ProductionPlan` (строка заголовка 342)

Публичные поля и значения по умолчанию:

- `Fixed target_quantity = 0`
- `std::int32_t priority = 0`
- `ProductionPlanState state = ProductionPlanState::Planned`

#### `ResourceTransaction` (строка заголовка 352)

Публичные поля и значения по умолчанию:

- `std::vector<ResourceQuantity> quantities`

#### `ResourceChange` (строка заголовка 363)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `ResourceChangeKind kind = ResourceChangeKind::ResourceAdded`
- `Fixed amount = 0`

#### `ResourceChangeBatch` (строка заголовка 376)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `std::vector<ResourceChange> changes`

#### `ResourcesSnapshot` (строка заголовка 387)

Публичные поля и значения по умолчанию:

- `std::vector<ResourceStockpile> stockpiles`
- `std::vector<std::pair<ResourceStockpileId, ResourceQuantity>> amounts`
- `std::vector<ResourceNode> nodes`
- `std::vector<ProductionSite> sites`
- `std::vector<ResourceReservation> reservations`
- `std::vector<ProductionCapability> capabilities`
- `std::vector<ProductionPlan> plans`
- `std::vector<ResourceTransaction> transactions`
- `std::uint64_t change_epoch = 1`

#### `ResourcesDiagnostics` (строка заголовка 408)

Публичные поля и значения по умолчанию:

- `std::uint64_t stockpiles = 0`
- `std::uint64_t resource_records = 0`
- `std::uint64_t nodes = 0`
- `std::uint64_t production_sites = 0`
- `std::uint64_t active_reservations = 0`
- `std::uint64_t production_capabilities = 0`
- `std::uint64_t production_plans = 0`
- `std::uint64_t transactions = 0`
- `std::uint64_t shortages = 0`

#### `ResourcesProductionService` (строка заголовка 421)

Внутреннее состояние (не часть публичного API): `bool frozen_ = false`, `std::unordered_map<ResourceTypeId, ResourceType, IdHash> types_`, `std::unordered_map<ResourceStockpileId, ResourceStockpile, IdHash> stockpiles_`, `std::unordered_map<AmountKey, Fixed, AmountKeyHash> amounts_`, `std::unordered_map<AmountKey, Fixed, AmountKeyHash> reserved_amounts_`, `std::unordered_map<ResourceNodeId, ResourceNode, IdHash> nodes_`, `std::unordered_map<ProductionSiteId, ProductionSite, IdHash> sites_`, `std::unordered_map<ResourceReservationId, ResourceReservation, IdHash> reservations_`, `std::unordered_map<ProductionCapabilityId, ProductionCapability, IdHash> capabilities_`.

Публичные сигнатуры операций:

```cpp
ResourcesProductionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<ResourceTypeId> RegisterResourceType(ResourceType type);
[[nodiscard]] foundation::Result<ProductionRecipeId> RegisterProductionRecipe(ProductionRecipe recipe);
void Freeze() noexcept;
[[nodiscard]] const ResourceType *FindResourceType(ResourceTypeId id) const noexcept;
[[nodiscard]] const ProductionRecipe *FindProductionRecipe(ProductionRecipeId id) const noexcept;
[[nodiscard]] foundation::Result<ResourceStockpileId> CreateStockpile(ResourceStockpile stockpile);
[[nodiscard]] foundation::Result<ResourceNodeId> CreateNode(ResourceNode node);
[[nodiscard]] foundation::Result<ProductionSiteId> CreateProductionSite(ProductionSite site);
[[nodiscard]] const ResourceStockpile *FindStockpile(ResourceStockpileId id) const noexcept;
[[nodiscard]] Fixed GetAmount(ResourceStockpileId stockpile, ResourceTypeId type) const noexcept;
[[nodiscard]] Fixed GetReservedAmount(ResourceStockpileId stockpile, ResourceTypeId type) const noexcept;
[[nodiscard]] Fixed GetAvailableAmount(ResourceStockpileId stockpile, ResourceTypeId type) const noexcept;
[[nodiscard]] bool CanReserve(ResourceStockpileId stockpile, std::span<const ResourceQuantity> quantities) const;
[[nodiscard]] const ResourceReservation *FindReservation(ResourceReservationId reservation) const noexcept;
[[nodiscard]] foundation::Result<ProductionCapabilityId> CreateProductionCapability( ProductionCapability capability);
[[nodiscard]] const ProductionCapability *FindProductionCapability(ProductionCapabilityId id) const noexcept;
[[nodiscard]] foundation::Result<ProductionPlanId> CreateProductionPlan(ProductionPlan plan);
[[nodiscard]] const ProductionPlan *FindProductionPlan(ProductionPlanId id) const noexcept;
[[nodiscard]] ResourceChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] std::uint64_t OldestChangeSequence() const noexcept;
void PruneChangesBefore(std::uint64_t sequence) noexcept;
[[nodiscard]] ResourcesSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(ResourcesSnapshot snapshot);
[[nodiscard]] ResourcesDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `AmountKey` (строка заголовка 512)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const AmountKey &) const noexcept = default;
```

#### `AmountKeyHash` (строка заголовка 518)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const AmountKey &k) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `ResourcesProductionService` | `ResourcesProductionService::ResourcesProductionService` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:218` |
| `RegisterResourceType` | `ResourcesProductionService::RegisterResourceType` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:223` |
| `RegisterProductionRecipe` | `ResourcesProductionService::RegisterProductionRecipe` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:259` |
| `FindResourceType` | `ResourcesProductionService::FindResourceType` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:307` |
| `FindProductionRecipe` | `ResourcesProductionService::FindProductionRecipe` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:313` |
| `CreateStockpile` | `ResourcesProductionService::CreateStockpile` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:431` |
| `CreateNode` | `ResourcesProductionService::CreateNode` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:483` |
| `CreateProductionSite` | `ResourcesProductionService::CreateProductionSite` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:545` |
| `FindStockpile` | `ResourcesProductionService::FindStockpile` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:594` |
| `GetAmount` | `ResourcesProductionService::GetAmount` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:639` |
| `GetReservedAmount` | `ResourcesProductionService::GetReservedAmount` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:645` |
| `GetAvailableAmount` | `ResourcesProductionService::GetAvailableAmount` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:651` |
| `CanReserve` | `ResourcesProductionService::CanReserve` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:750` |
| `FindReservation` | `ResourcesProductionService::FindReservation` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:894` |
| `CreateProductionCapability` | `ResourcesProductionService::CreateProductionCapability` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1261` |
| `FindProductionCapability` | `ResourcesProductionService::FindProductionCapability` | `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1311` |

