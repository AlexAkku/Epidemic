---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h"
---

# ItemsInventory: items_inventory.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`.

**Модуль:** [[06 EngineFramework/Модуль ItemsInventory|ItemsInventory]].
**Размер исходного файла:** 20415 байт.
**Подключение:** `#include <Epidemic/GameFramework/ItemsInventory/items_inventory.h>`.

## Объявления типов (лексический индекс)
`name`, `IdHash`, `RegisteredPayload`, `ItemStackPolicy`, `ItemDurabilityPolicy`, `ItemChargePolicy`, `ItemLocationKind`, `ContainerState`, `ReservationState`, `ItemChangeKind`, `ItemProperty`, `ItemDefinition`, `ItemLocation`, `ItemInstance`, `ItemCreateRequest`, `ContainerRecord`, `ContainerUsage`, `ItemTransferPlan`, `ItemReservation`, `ItemChange`, `ItemsChangeBatch`, `ItemsSnapshot`, `ItemPropertySchema`, `ContainerPolicyContext`, `IContainerPolicy`, `ItemsDiagnostics`, `ItemsInventoryService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::items
{
using Fixed = std::int64_t;

#define EPIDEMIC_ITEM_TYPE_ID(name)                                                                                    \
    struct name                                                                                                        \
    {                                                                                                                  \
        TypeId value{};                                                                                                \
        static constexpr name FromString(std::string_view s) noexcept                                                  \
        {                                                                                                              \
            return {TypeId::FromString(s)};                                                                            \
        }                                                                                                              \
        [[nodiscard]] constexpr bool IsValid() const noexcept                                                          \
        {                                                                                                              \
            return value.IsValid();                                                                                    \
        }                                                                                                              \
        [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;                                \
        [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;                               \
    }
#define EPIDEMIC_ITEM_OBJECT_ID(name)                                                                                  \
    struct name                                                                                                        \
    {                                                                                                                  \
        GameplayObjectId value{};                                                                                      \
        static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept                                       \
        {                                                                                                              \
            return {GameplayObjectId::FromRaw(h, l)};                                                                  \
        }                                                                                                              \
        [[nodiscard]] constexpr bool IsValid() const noexcept                                                          \
        {                                                                                                              \
            return value.IsValid();                                                                                    \
        }                                                                                                              \
        [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;                                \
        [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;                               \
    }
EPIDEMIC_ITEM_TYPE_ID(ItemDefinitionId);
EPIDEMIC_ITEM_TYPE_ID(ItemPropertyTypeId);
EPIDEMIC_ITEM_TYPE_ID(ItemLocationTypeId);
EPIDEMIC_ITEM_TYPE_ID(ContainerPolicyId);
EPIDEMIC_ITEM_OBJECT_ID(ItemInstanceId);
EPIDEMIC_ITEM_OBJECT_ID(ContainerId);
EPIDEMIC_ITEM_OBJECT_ID(ItemTransferId);
EPIDEMIC_ITEM_OBJECT_ID(ItemReservationId);
#undef EPIDEMIC_ITEM_TYPE_ID
#undef EPIDEMIC_ITEM_OBJECT_ID

struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

struct RegisteredPayload
{
    TypeId type{};
    std::uint32_t schema_version = 1;
    std::vector<std::byte> bytes;
    [[nodiscard]] bool operator==(const RegisteredPayload &) const = default;
    template <class T> [[nodiscard]] static RegisteredPayload FromTrivial(TypeId type_id, const T &value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        RegisteredPayload out;
        out.type = type_id;
        out.schema_version = 1;
        out.bytes.resize(sizeof(T));
        std::memcpy(out.bytes.data(), &value, sizeof(T));
        return out;
    }
    template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected) const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (type != expected || schema_version != 1 || bytes.size() != sizeof(T))
            return std::nullopt;
        T value{};
        std::memcpy(&value, bytes.data(), sizeof(T));
        return value;
    }
};

enum class ItemStackPolicy
{
    NonStackable,
    StackByDefinition,
    StackByEquivalentState,
    CustomRegistered
};
enum class ItemDurabilityPolicy
{
    None,
    InstanceValue
};
enum class ItemChargePolicy
{
    None,
    InstanceValue
};
enum class ItemLocationKind
{
    None,
    Container,
    World,
    Equipped,
    Reserved,
    InTransit,
    Destroyed
};
enum class ContainerState
{
    Active,
    Locked,
    Disabled,
    Destroyed
};
enum class ReservationState
{
    Active,
    Consumed,
    Released
};
enum class ItemChangeKind
{
    ItemCreated,
    ItemDestroyed,
    StackSplit,
    StacksMerged,
    TransferCommitted,
    ContainerCreated,
    ContainerStateChanged,
    ContainerRemoved,
    ReservationCreated,
    ReservationReleased,
    ReservationConsumed,
    DurabilityChanged,
    ChargesChanged
};

struct ItemProperty
{
    ItemPropertyTypeId type{};
    RegisteredPayload payload;
    [[nodiscard]] bool operator==(const ItemProperty &) const = default;
};
struct ItemDefinition
{
    ItemDefinitionId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    Fixed base_weight = 0;
    Fixed base_volume = 0;
    ItemStackPolicy stack_policy = ItemStackPolicy::NonStackable;
    ItemDurabilityPolicy durability_policy = ItemDurabilityPolicy::None;
    ItemChargePolicy charge_policy = ItemChargePolicy::None;
    Fixed default_durability = 0;
    Fixed max_durability = 0;
    Fixed default_charges = 0;
    Fixed max_charges = 0;
    RegisteredPayload payload;
    Revision revision{};
};
struct ItemLocation
{
    ItemLocationKind kind = ItemLocationKind::None;
    ContainerId container{};
    GameplayObjectRef world_object{};
    GameplayObjectRef area{};
    RegisteredPayload payload;
    [[nodiscard]] bool operator==(const ItemLocation &) const = default;
};
struct ItemInstance
{
    ItemInstanceId id{};
    ItemDefinitionId definition{};
    Fixed quantity = 1;
    ItemLocation location{};
    Fixed durability = 0;
    Fixed charges = 0;
    std::vector<ItemProperty> properties;
    Revision revision{};
};
struct ItemCreateRequest
{
    ItemInstance item;
    std::optional<Fixed> durability_override{};
    std::optional<Fixed> charges_override{};
};
struct ContainerRecord
{
    ContainerId id{};
    GameplayObjectRef owner_object{};
    Fixed max_weight = 0;
    Fixed max_volume = 0;
    std::uint32_t max_slots = 0;
    std::uint32_t max_nesting_depth = 8;
    ContainerPolicyId policy{};
    ContainerId parent{};
    ContainerState state = ContainerState::Active;
    Revision revision{};
};
struct ContainerUsage
{
    Fixed weight = 0;
    Fixed volume = 0;
    std::uint32_t slots = 0;
};
struct ItemTransferPlan
{
    ItemTransferId id{};
    ItemInstanceId item{};
    ItemLocation source{};
    ItemLocation target{};
    Fixed quantity = 0;
    Revision item_revision{};
    Revision source_revision{};
    Revision target_revision{};
    GameplayContext context{};
};
struct ItemReservation
{
    ItemReservationId id{};
    ItemInstanceId item{};
    Fixed quantity = 0;
    GameplayObjectRef owner{};
    TypeId reason{};
    ReservationState state = ReservationState::Active;
    Revision revision{};
};
struct ItemChange
{
    std::uint64_t sequence = 0;
    ItemChangeKind kind = ItemChangeKind::ItemCreated;
    ItemInstanceId item{};
    ContainerId container{};
    Fixed quantity = 0;
    GameplayContext context{};
    Revision revision{};
    ItemLocation source{};
    ItemLocation target{};
    ItemInstanceId related_item{};
    ItemReservationId reservation{};
};
struct ItemsChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    std::vector<ItemChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct ItemsSnapshot
{
    std::vector<ItemInstance> items;
    std::vector<ContainerRecord> containers;
    std::vector<ItemReservation> reservations;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot item_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot container_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot transfer_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot reservation_ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};

struct ItemPropertySchema
{
    ItemPropertyTypeId type{};
    TypeId payload_type{};
    std::uint32_t schema_version = 1;
    std::size_t max_payload_bytes = 0;
};

struct ContainerPolicyContext
{
    ContainerId container{};
    ItemInstanceId moving_item{};
    ItemDefinitionId definition{};
    Fixed quantity = 0;
};

class IContainerPolicy
{
  public:
    virtual ~IContainerPolicy() = default;
    [[nodiscard]] virtual ContainerPolicyId Id() const noexcept = 0;
    [[nodiscard]] virtual bool AllowsInsert(const ContainerPolicyContext& context) const = 0;
};

struct ItemsDiagnostics
{
    std::uint64_t items = 0;
    std::uint64_t containers = 0;
    std::uint64_t active_reservations = 0;
    std::uint64_t transfers = 0;
    std::uint64_t rejected_transfers = 0;
};

class ItemsInventoryService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<ItemsInventoryService>;

  public:
    ItemsInventoryService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.items_inventory");
    }
    [[nodiscard]] foundation::Result<ItemDefinitionId> RegisterDefinition(ItemDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterPropertySchema(ItemPropertySchema schema);
    [[nodiscard]] foundation::Result<void> RegisterContainerPolicy(const IContainerPolicy& policy);
    void Freeze() noexcept
    {
        frozen_ = true;
    }
    [[nodiscard]] const ItemDefinition *FindDefinition(ItemDefinitionId id) const noexcept;

    [[nodiscard]] foundation::Result<ContainerId> CreateContainer(ContainerRecord container);
    [[nodiscard]] foundation::Result<void> SetContainerState(ContainerId container, ContainerState state,
                                                            GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveContainer(ContainerId container, GameplayContext context = {});
    [[nodiscard]] foundation::Result<ItemInstanceId> CreateItem(ItemInstance item, GameplayContext context = {});
    [[nodiscard]] foundation::Result<ItemInstanceId> CreateItem(ItemCreateRequest request, GameplayContext context = {});
    [[nodiscard]] bool CanCreateItem(ItemDefinitionId definition, Fixed quantity, const ItemLocation& location) const;
    [[nodiscard]] foundation::Result<void> DestroyItem(ItemInstanceId item, GameplayContext context = {});
    [[nodiscard]] const ItemInstance *FindItem(ItemInstanceId id) const noexcept;
    [[nodiscard]] const ContainerRecord *FindContainer(ContainerId id) const noexcept;
    [[nodiscard]] std::optional<ItemInstance> FindItemCopy(ItemInstanceId id) const noexcept;
    [[nodiscard]] std::optional<ContainerRecord> FindContainerCopy(ContainerId id) const noexcept;

    [[nodiscard]] foundation::Result<ItemInstanceId> SplitStack(ItemInstanceId item, Fixed quantity,
                                                                GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> MergeStacks(ItemInstanceId target, ItemInstanceId source,
                                                       GameplayContext context = {});

    [[nodiscard]] foundation::Result<ItemTransferPlan> PrepareTransfer(ItemInstanceId item, ItemLocation target,
                                                                       Fixed quantity, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CommitTransfer(const ItemTransferPlan &plan);
    // Commits a full-instance relocation against an active reservation. This is the
    // owner-side primitive used by cross-owner sagas such as coordinated trade: the
    // reservation remains authoritative until the location mutation succeeds.
    [[nodiscard]] foundation::Result<void> CommitReservedTransfer(ItemReservationId reservation, ItemLocation target,
                                                                  GameplayContext context = {});
    [[nodiscard]] bool CanTransfer(ItemInstanceId item, ItemLocation target, Fixed quantity) const noexcept;

    [[nodiscard]] foundation::Result<ItemReservationId> ReserveItem(ItemInstanceId item, Fixed quantity,
                                                                    GameplayObjectRef owner, TypeId reason = {},
                                                                    GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ReleaseReservation(ItemReservationId reservation,
                                                              GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ConsumeReservation(ItemReservationId reservation,
                                                              GameplayContext context = {});
    // Atomically releases an existing set of reservations and creates one new
    // reservation. No reservation state is changed if any precondition fails.
    // This is the cross-owner primitive used by Equipment for conflict swaps.
    [[nodiscard]] foundation::Result<ItemReservationId> ExchangeReservations(
        std::span<const ItemReservationId> release_reservations,
        ItemInstanceId reserve_item,
        Fixed reserve_quantity,
        GameplayObjectRef reserve_owner,
        TypeId reserve_reason = {},
        GameplayContext context = {});
    [[nodiscard]] const ItemReservation *FindReservation(ItemReservationId reservation) const noexcept;
    [[nodiscard]] std::optional<ItemReservation> FindReservationCopy(ItemReservationId reservation) const noexcept;
    [[nodiscard]] std::vector<ItemReservation> FindReservations(ItemInstanceId item) const;
    [[nodiscard]] Fixed ReservedQuantity(ItemInstanceId item) const noexcept;

    [[nodiscard]] foundation::Result<void> AdjustDurability(ItemInstanceId item, Fixed delta,
                                                            GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> AdjustCharges(ItemInstanceId item, Fixed delta,
                                                         GameplayContext context = {});

    [[nodiscard]] ContainerUsage GetContainerUsage(ContainerId container) const noexcept;
    [[nodiscard]] std::vector<ItemInstance> FindItemsInContainer(ContainerId container) const;
    [[nodiscard]] std::vector<ItemInstance> FindItemsByDefinition(ItemDefinitionId definition) const;
    private:
        [[nodiscard]] std::vector<ItemChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] ItemsChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] ItemsChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    void PruneChangesBefore(std::uint64_t sequence);

    [[nodiscard]] ItemsSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(ItemsSnapshot snapshot);
    [[nodiscard]] ItemsDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept
    {
        return revision_;
    }

  private:
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    void Record(ItemChange change) noexcept;
    void RebuildIndexes() noexcept;
    [[nodiscard]] bool EquivalentForStack(const ItemInstance &a, const ItemInstance &b) const noexcept;
    [[nodiscard]] bool ValidateContainerTarget(ItemInstanceId moving, ContainerId target,
                                               Fixed quantity) const noexcept;
    [[nodiscard]] std::uint32_t ContainerDepth(ContainerId id) const noexcept;
    [[nodiscard]] Revision LocationRevision(const ItemLocation &location) const noexcept;
    [[nodiscard]] bool ValidateLocation(const ItemLocation& location) const noexcept;
    [[nodiscard]] bool IsWorldObjectBound(GameplayObjectRef world_object, ItemInstanceId except = {}) const noexcept;
    [[nodiscard]] bool ValidateProperties(std::vector<ItemProperty>& properties) const;
    [[nodiscard]] bool PolicyAllows(ContainerId container, ItemInstanceId moving, ItemDefinitionId definition, Fixed quantity) const;

    bool frozen_ = false;
    Revision revision_{};
    std::unordered_map<ItemDefinitionId, ItemDefinition, IdHash> definitions_;
    std::unordered_map<ItemPropertyTypeId, ItemPropertySchema, IdHash> property_schemas_;
    std::unordered_map<ContainerPolicyId, const IContainerPolicy*, IdHash> container_policies_;
    std::unordered_map<ItemInstanceId, ItemInstance, IdHash> items_;
    std::unordered_map<ContainerId, ContainerRecord, IdHash> containers_;
    std::unordered_map<ItemReservationId, ItemReservation, IdHash> reservations_;
    std::unordered_map<ContainerId, std::vector<ItemInstanceId>, IdHash> container_items_;
    std::unordered_map<ItemDefinitionId, std::vector<ItemInstanceId>, IdHash> definition_items_;
    std::unordered_map<ItemInstanceId, Fixed, IdHash> active_reserved_quantities_;
    bool indexes_valid_ = true;
    MonotonicIdGenerator<GameplayObjectId> item_ids_{0x3400};
    MonotonicIdGenerator<GameplayObjectId> container_ids_{0x3401};
    MonotonicIdGenerator<GameplayObjectId> transfer_ids_{0x3402};
    MonotonicIdGenerator<GameplayObjectId> reservation_ids_{0x3403};
    std::deque<ItemChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    static constexpr std::size_t kChangeJournalCapacity = 8192;
    ItemsDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::items
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `ItemStackPolicy`: `NonStackable`, `StackByDefinition`, `StackByEquivalentState`, `CustomRegistered`
- `ItemDurabilityPolicy`: `None`, `InstanceValue`
- `ItemChargePolicy`: `None`, `InstanceValue`
- `ItemLocationKind`: `None`, `Container`, `World`, `Equipped`, `Reserved`, `InTransit`, `Destroyed`
- `ContainerState`: `Active`, `Locked`, `Disabled`, `Destroyed`
- `ReservationState`: `Active`, `Consumed`, `Released`
- `ItemChangeKind`: `ItemCreated`, `ItemDestroyed`, `StackSplit`, `StacksMerged`, `TransferCommitted`, `ContainerCreated`, `ContainerStateChanged`, `ContainerRemoved`, `ReservationCreated`, `ReservationReleased`, `ReservationConsumed`, `DurabilityChanged`, `ChargesChanged`

### Состав типов и операции

#### `name` (строка заголовка 23)

Публичные сигнатуры операций:

```cpp
\ static constexpr name FromString(std::string_view s) noexcept \;
\ [[nodiscard]] constexpr bool IsValid() const noexcept \;
\ [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;
\ [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;
```

#### `name` (строка заголовка 38)

Публичные сигнатуры операций:

```cpp
\ static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept \;
\ [[nodiscard]] constexpr bool IsValid() const noexcept \;
\ [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;
\ [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;
```

#### `IdHash` (строка заголовка 63)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 65)

#### `RegisteredPayload` (строка заголовка 71)

Публичные поля и значения по умолчанию:

- `std::uint32_t schema_version = 1`
- `std::vector<std::byte> bytes`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const RegisteredPayload &) const = default;
template <class T> [[nodiscard]] static RegisteredPayload FromTrivial(TypeId type_id, const T &value);
template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected) const;
```

#### `T` (строка заголовка 77)

#### `T` (строка заголовка 87)

#### `ItemProperty` (строка заголовка 155)

Публичные поля и значения по умолчанию:

- `RegisteredPayload payload`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const ItemProperty &) const = default;
```

#### `ItemDefinition` (строка заголовка 161)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `Fixed base_weight = 0`
- `Fixed base_volume = 0`
- `ItemStackPolicy stack_policy = ItemStackPolicy::NonStackable`
- `ItemDurabilityPolicy durability_policy = ItemDurabilityPolicy::None`
- `ItemChargePolicy charge_policy = ItemChargePolicy::None`
- `Fixed default_durability = 0`
- `Fixed max_durability = 0`
- `Fixed default_charges = 0`
- `Fixed max_charges = 0`
- `RegisteredPayload payload`

#### `ItemLocation` (строка заголовка 178)

Публичные поля и значения по умолчанию:

- `ItemLocationKind kind = ItemLocationKind::None`
- `RegisteredPayload payload`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const ItemLocation &) const = default;
```

#### `ItemInstance` (строка заголовка 187)

Публичные поля и значения по умолчанию:

- `Fixed quantity = 1`
- `Fixed durability = 0`
- `Fixed charges = 0`
- `std::vector<ItemProperty> properties`

#### `ItemCreateRequest` (строка заголовка 198)

Публичные поля и значения по умолчанию:

- `ItemInstance item`

#### `ContainerRecord` (строка заголовка 204)

Публичные поля и значения по умолчанию:

- `Fixed max_weight = 0`
- `Fixed max_volume = 0`
- `std::uint32_t max_slots = 0`
- `std::uint32_t max_nesting_depth = 8`
- `ContainerState state = ContainerState::Active`

#### `ContainerUsage` (строка заголовка 217)

Публичные поля и значения по умолчанию:

- `Fixed weight = 0`
- `Fixed volume = 0`
- `std::uint32_t slots = 0`

#### `ItemTransferPlan` (строка заголовка 223)

Публичные поля и значения по умолчанию:

- `Fixed quantity = 0`

#### `ItemReservation` (строка заголовка 235)

Публичные поля и значения по умолчанию:

- `Fixed quantity = 0`
- `ReservationState state = ReservationState::Active`

#### `ItemChange` (строка заголовка 245)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `ItemChangeKind kind = ItemChangeKind::ItemCreated`
- `Fixed quantity = 0`

#### `ItemsChangeBatch` (строка заголовка 259)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `std::vector<ItemChange> changes`

#### `ItemsSnapshot` (строка заголовка 270)

Публичные поля и значения по умолчанию:

- `std::vector<ItemInstance> items`
- `std::vector<ContainerRecord> containers`
- `std::vector<ItemReservation> reservations`
- `std::uint64_t change_epoch = 1`

#### `ItemPropertySchema` (строка заголовка 284)

Публичные поля и значения по умолчанию:

- `std::uint32_t schema_version = 1`
- `std::size_t max_payload_bytes = 0`

#### `ContainerPolicyContext` (строка заголовка 292)

Публичные поля и значения по умолчанию:

- `Fixed quantity = 0`

#### `IContainerPolicy` (строка заголовка 300)

Публичные сигнатуры операций:

```cpp
virtual ~IContainerPolicy() = default;
[[nodiscard]] virtual ContainerPolicyId Id() const noexcept = 0;
[[nodiscard]] virtual bool AllowsInsert(const ContainerPolicyContext& context) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ItemsDiagnostics` (строка заголовка 308)

Публичные поля и значения по умолчанию:

- `std::uint64_t items = 0`
- `std::uint64_t containers = 0`
- `std::uint64_t active_reservations = 0`
- `std::uint64_t transfers = 0`
- `std::uint64_t rejected_transfers = 0`

#### `ItemsInventoryService` (строка заголовка 317)

Внутреннее состояние (не часть публичного API): `bool frozen_ = false`, `std::unordered_map<ItemDefinitionId, ItemDefinition, IdHash> definitions_`, `std::unordered_map<ItemPropertyTypeId, ItemPropertySchema, IdHash> property_schemas_`, `std::unordered_map<ContainerPolicyId, const IContainerPolicy*, IdHash> container_policies_`, `std::unordered_map<ItemInstanceId, ItemInstance, IdHash> items_`, `std::unordered_map<ContainerId, ContainerRecord, IdHash> containers_`, `std::unordered_map<ItemReservationId, ItemReservation, IdHash> reservations_`, `std::unordered_map<ContainerId, std::vector<ItemInstanceId>, IdHash> container_items_`, `std::unordered_map<ItemDefinitionId, std::vector<ItemInstanceId>, IdHash> definition_items_`.

Публичные сигнатуры операций:

```cpp
ItemsInventoryService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<ItemDefinitionId> RegisterDefinition(ItemDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterPropertySchema(ItemPropertySchema schema);
[[nodiscard]] foundation::Result<void> RegisterContainerPolicy(const IContainerPolicy& policy);
void Freeze() noexcept;
[[nodiscard]] const ItemDefinition *FindDefinition(ItemDefinitionId id) const noexcept;
[[nodiscard]] foundation::Result<ContainerId> CreateContainer(ContainerRecord container);
[[nodiscard]] bool CanCreateItem(ItemDefinitionId definition, Fixed quantity, const ItemLocation& location) const;
[[nodiscard]] const ItemInstance *FindItem(ItemInstanceId id) const noexcept;
[[nodiscard]] const ContainerRecord *FindContainer(ContainerId id) const noexcept;
[[nodiscard]] std::optional<ItemInstance> FindItemCopy(ItemInstanceId id) const noexcept;
[[nodiscard]] std::optional<ContainerRecord> FindContainerCopy(ContainerId id) const noexcept;
[[nodiscard]] foundation::Result<void> CommitTransfer(const ItemTransferPlan &plan);
[[nodiscard]] bool CanTransfer(ItemInstanceId item, ItemLocation target, Fixed quantity) const noexcept;
[[nodiscard]] const ItemReservation *FindReservation(ItemReservationId reservation) const noexcept;
[[nodiscard]] std::optional<ItemReservation> FindReservationCopy(ItemReservationId reservation) const noexcept;
[[nodiscard]] std::vector<ItemReservation> FindReservations(ItemInstanceId item) const;
[[nodiscard]] Fixed ReservedQuantity(ItemInstanceId item) const noexcept;
[[nodiscard]] ContainerUsage GetContainerUsage(ContainerId container) const noexcept;
[[nodiscard]] std::vector<ItemInstance> FindItemsInContainer(ContainerId container) const;
[[nodiscard]] std::vector<ItemInstance> FindItemsByDefinition(ItemDefinitionId definition) const;
[[nodiscard]] ItemsChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesBefore(std::uint64_t sequence);
[[nodiscard]] ItemsSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(ItemsSnapshot snapshot);
[[nodiscard]] ItemsDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `ItemsInventoryService` | `ItemsInventoryService::ItemsInventoryService` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:109` |
| `RegisterDefinition` | `ItemsInventoryService::RegisterDefinition` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:119` |
| `RegisterPropertySchema` | `ItemsInventoryService::RegisterPropertySchema` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:150` |
| `RegisterContainerPolicy` | `ItemsInventoryService::RegisterContainerPolicy` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:160` |
| `FindDefinition` | `ItemsInventoryService::FindDefinition` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:169` |
| `CreateContainer` | `ItemsInventoryService::CreateContainer` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:174` |
| `CanCreateItem` | `ItemsInventoryService::CanCreateItem` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:298` |
| `FindItem` | `ItemsInventoryService::FindItem` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:440` |
| `FindContainer` | `ItemsInventoryService::FindContainer` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:445` |
| `FindItemCopy` | `ItemsInventoryService::FindItemCopy` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:450` |
| `FindContainerCopy` | `ItemsInventoryService::FindContainerCopy` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:455` |
| `GetContainerUsage` | `ItemsInventoryService::GetContainerUsage` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:640` |
| `CanTransfer` | `ItemsInventoryService::CanTransfer` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:730` |
| `CommitTransfer` | `ItemsInventoryService::CommitTransfer` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:782` |
| `ReservedQuantity` | `ItemsInventoryService::ReservedQuantity` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:906` |
| `FindReservation` | `ItemsInventoryService::FindReservation` | `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:1108` |

