---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h"
---

# Equipment: equipment.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h`.

**Модуль:** [[06 EngineFramework/Модуль Equipment|Equipment]].
**Размер исходного файла:** 17292 байт.
**Подключение:** `#include <Epidemic/GameFramework/Equipment/equipment.h>`.

## Объявления типов (лексический индекс)
`name`, `IdHash`, `EquipmentBindingState`, `EquipmentChangeKind`, `EquipmentReconciliationState`, `EquipmentGrantDescriptor`, `EquipmentSlotDefinition`, `EquipmentProfileDefinition`, `EquipmentGrantSchema`, `EquipmentProfile`, `EquipmentItemDescriptor`, `EquipmentBinding`, `EquipmentLoadout`, `EquipPlan`, `EquipmentChange`, `EquipmentChangeBatch`, `EquipmentSnapshot`, `EquipmentDiagnostics`, `IEquipmentItemProvider`, `EquipmentService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once
#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/error.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"
#include <algorithm>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::equipment
{
#define EQ_TYPE(name)                                                                                                  \
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
#define EQ_OBJ(name)                                                                                                   \
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
EQ_TYPE(EquipmentSlotTypeId);
EQ_TYPE(EquipmentGrantTypeId);
EQ_TYPE(EquipmentProfileDefinitionId);
EQ_OBJ(EquipmentProfileId);
EQ_OBJ(EquipmentSlotId);
EQ_OBJ(EquipmentBindingId);
EQ_OBJ(EquipOperationId);
EQ_OBJ(EquipmentLoadoutId);
EQ_OBJ(EquipmentItemId);
#undef EQ_TYPE
#undef EQ_OBJ
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

enum class EquipmentBindingState
{
    Active,
    Disabled,
    Broken
};
enum class EquipmentChangeKind
{
    ProfileCreated,
    SlotAdded,
    BindingCreated,
    BindingRemoved,
    BindingStateChanged,
    LoadoutSaved
};

enum class EquipmentReconciliationState
{
    Ready,
    NeedsReconciliation
};
struct EquipmentGrantDescriptor
{
    EquipmentGrantTypeId type{};
    TypeId value_type{};
    std::vector<std::byte> payload;
};
struct EquipmentSlotDefinition
{
    EquipmentSlotId id{};
    EquipmentSlotTypeId type{};
    GameplayTagSet accepted_tags;
    GameplayTagSet blocked_tags;
    TypeId conflict_group{};
    Revision revision{};
};
struct EquipmentProfileDefinition
{
    EquipmentProfileDefinitionId id{};
    std::string canonical_name;
    std::vector<EquipmentSlotDefinition> slots;
};
struct EquipmentGrantSchema
{
    EquipmentGrantTypeId type{};
    TypeId payload_type{};
    std::uint32_t schema_version = 1;
    std::size_t max_payload_bytes = 0;
};
struct EquipmentProfile
{
    EquipmentProfileId id{};
    GameplayObjectRef subject{};
    EquipmentProfileDefinitionId definition{};
    std::vector<EquipmentSlotDefinition> slots;
    Revision revision{};
};
struct EquipmentItemDescriptor
{
    EquipmentItemId id{};
    GameplayTagSet tags;
    Revision revision{};
    bool available = false;
    std::vector<EquipmentGrantDescriptor> grants;
};
struct EquipmentBinding
{
    EquipmentBindingId id{};
    GameplayObjectRef subject{};
    EquipmentItemId item{};
    std::vector<EquipmentSlotId> slots;
    std::vector<EquipmentGrantDescriptor> grants;
    EquipmentBindingState state = EquipmentBindingState::Active;
    Revision item_revision{};
    Revision revision{};
};
struct EquipmentLoadout
{
    EquipmentLoadoutId id{};
    GameplayObjectRef subject{};
    std::vector<std::pair<EquipmentSlotId, EquipmentItemId>> desired;
    Revision revision{};
};
struct EquipPlan
{
    EquipOperationId id{};
    GameplayObjectRef subject{};
    EquipmentItemId item{};
    std::vector<EquipmentSlotId> slots;
    Revision profile_revision{};
    Revision item_revision{};
    std::vector<EquipmentBindingId> conflicting_bindings;
    GameplayContext context{};
};
struct EquipmentChange
{
    std::uint64_t sequence = 0;
    EquipmentChangeKind kind = EquipmentChangeKind::BindingCreated;
    GameplayObjectRef subject{};
    EquipmentBindingId binding{};
    EquipmentItemId item{};
    GameplayContext context{};
    Revision revision{};
    std::vector<EquipmentSlotId> slots;
    std::vector<EquipmentGrantDescriptor> grants;
    EquipmentBindingState old_state = EquipmentBindingState::Active;
    EquipmentBindingState new_state = EquipmentBindingState::Active;
};
struct EquipmentChangeBatch
{
    std::vector<EquipmentChange> changes;
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct EquipmentSnapshot
{
    std::vector<EquipmentProfile> profiles;
    std::vector<EquipmentBinding> bindings;
    std::vector<EquipmentLoadout> loadouts;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot profile_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot slot_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot binding_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot operation_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot loadout_ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};
struct EquipmentDiagnostics
{
    std::uint64_t profiles = 0;
    std::uint64_t bindings = 0;
    std::uint64_t equip_operations = 0;
    std::uint64_t rejected_operations = 0;
};

class IEquipmentItemProvider
{
  public:
    virtual ~IEquipmentItemProvider() = default;
    [[nodiscard]] virtual std::optional<EquipmentItemDescriptor> Describe(EquipmentItemId item) const = 0;
    [[nodiscard]] virtual foundation::Result<void> ReserveForEquipment(EquipmentItemId item, GameplayObjectRef subject,
                                                                       GameplayContext context) = 0;
    [[nodiscard]] virtual foundation::Result<void> ReleaseFromEquipment(EquipmentItemId item, GameplayObjectRef subject,
                                                                        GameplayContext context) = 0;
    // Restore reconciliation must be idempotent and must not create a second reservation.
    // Providers that persist reservation state should verify that the restored binding is backed by
    // exactly the reservation expected for this subject/item pair.
    [[nodiscard]] virtual foundation::Result<void> ReconcileEquipmentReservation(
        EquipmentItemId, GameplayObjectRef, EquipmentBindingId, GameplayContext)
    {
        return foundation::Result<void>::Failure(foundation::Error::Create(
            "gameplay.equipment.reconciliation_unsupported",
            "item provider does not support equipment reservation reconciliation"));
    }
    // Override when the backing item owner can atomically exchange several equipment reservations.
    // The default path safely supports zero/one released binding and refuses larger swaps before mutation.
    [[nodiscard]] virtual foundation::Result<void> ExchangeEquipmentReservations(
        std::span<const EquipmentItemId> release_items,
        std::optional<EquipmentItemId> reserve_item,
        GameplayObjectRef subject,
        GameplayContext context)
    {
        if (!release_items.empty())
            return foundation::Result<void>::Failure(foundation::Error::Create(
                "gameplay.equipment.atomic_exchange_unsupported",
                "item provider must override atomic exchange before replacing existing equipment bindings"));
        if (reserve_item.has_value())
            return ReserveForEquipment(*reserve_item, subject, context);
        return foundation::Result<void>::Success();
    }
};

class EquipmentService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<EquipmentService>;

  public:
    explicit EquipmentService(std::size_t change_capacity = 2048);
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.equipment");
    }
    void SetItemProvider(IEquipmentItemProvider *provider) noexcept
    {
        item_provider_ = provider;
    }
    [[nodiscard]] foundation::Result<EquipmentProfileDefinitionId> RegisterProfileDefinition(EquipmentProfileDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterGrantSchema(EquipmentGrantSchema schema);
    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }
    [[nodiscard]] EquipmentReconciliationState ReconciliationState() const noexcept { return reconciliation_state_; }
    [[nodiscard]] bool NeedsReconciliation() const noexcept
    {
        return reconciliation_state_ == EquipmentReconciliationState::NeedsReconciliation;
    }
    [[nodiscard]] foundation::Result<void> ReconcileRestoredBindings(GameplayContext context = {});
    [[nodiscard]] foundation::Result<EquipmentProfileId> CreateProfile(EquipmentProfile profile);
    [[nodiscard]] foundation::Result<EquipmentProfileId> CreateProfileFromDefinition(GameplayObjectRef subject, EquipmentProfileDefinitionId definition);
    [[nodiscard]] foundation::Result<EquipmentSlotId> AddSlot(EquipmentProfileId profile, EquipmentSlotDefinition slot);
    [[nodiscard]] const EquipmentProfile *FindProfile(GameplayObjectRef subject) const noexcept;
    [[nodiscard]] const EquipmentBinding *FindBinding(EquipmentBindingId id) const noexcept;
    [[nodiscard]] std::optional<EquipmentProfile> FindProfileCopy(GameplayObjectRef subject) const noexcept;
    [[nodiscard]] std::optional<EquipmentBinding> FindBindingCopy(EquipmentBindingId id) const noexcept;
    [[nodiscard]] foundation::Result<EquipPlan> PrepareEquip(GameplayObjectRef subject, EquipmentItemId item,
                                                             std::vector<EquipmentSlotId> slots,
                                                             GameplayContext context = {});
    [[nodiscard]] foundation::Result<EquipmentBindingId> CommitEquip(const EquipPlan &plan,
                                                                     std::vector<EquipmentGrantDescriptor> grants = {});
    [[nodiscard]] foundation::Result<void> Unequip(EquipmentBindingId binding, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SetBindingState(EquipmentBindingId binding, EquipmentBindingState state,
                                                           GameplayContext context = {});
    [[nodiscard]] std::vector<EquipmentBinding> FindBindings(GameplayObjectRef subject) const;
    [[nodiscard]] std::optional<EquipmentItemId> GetEquippedItem(GameplayObjectRef subject, EquipmentSlotId slot) const;
    [[nodiscard]] bool CanEquip(GameplayObjectRef subject, EquipmentItemId item,
                                const std::vector<EquipmentSlotId> &slots) const;
    [[nodiscard]] foundation::Result<EquipmentLoadoutId> SaveLoadout(EquipmentLoadout loadout);
    private:
        [[nodiscard]] EquipmentChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] EquipmentChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    private:
        [[nodiscard]] std::vector<EquipmentChange> ChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] EquipmentSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(EquipmentSnapshot snapshot);
    [[nodiscard]] EquipmentDiagnostics GetDiagnostics() const noexcept;

  private:
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    void Record(EquipmentChange change) noexcept;
    [[nodiscard]] EquipmentProfile *MutableProfile(EquipmentProfileId id) noexcept;
    [[nodiscard]] bool SlotAccepts(const EquipmentSlotDefinition &slot,
                                   const EquipmentItemDescriptor &item) const noexcept;
    [[nodiscard]] std::vector<EquipmentBindingId> ComputeConflicts(const EquipmentProfile& profile, GameplayObjectRef subject,
                                                                    const std::vector<EquipmentSlotId>& slots) const;
    [[nodiscard]] bool ValidateGrants(const std::vector<EquipmentGrantDescriptor>& grants) const;
    [[nodiscard]] foundation::Result<void> ValidateAndCanonicalizeLoadout(EquipmentLoadout& loadout) const;
    [[nodiscard]] foundation::Result<void> EnsureMutationReady() const;
    Revision revision_{};
    bool frozen_ = false;
    std::unordered_map<EquipmentProfileDefinitionId, EquipmentProfileDefinition, IdHash> profile_definitions_;
    std::unordered_map<EquipmentGrantTypeId, EquipmentGrantSchema, IdHash> grant_schemas_;
    std::unordered_map<EquipmentProfileId, EquipmentProfile, IdHash> profiles_;
    std::unordered_map<GameplayObjectRef, EquipmentProfileId> profile_by_subject_;
    std::unordered_map<EquipmentBindingId, EquipmentBinding, IdHash> bindings_;
    std::unordered_map<EquipmentLoadoutId, EquipmentLoadout, IdHash> loadouts_;
    MonotonicIdGenerator<GameplayObjectId> profile_ids_{0x3500}, slot_ids_{0x3501}, binding_ids_{0x3502},
        operation_ids_{0x3503}, loadout_ids_{0x3504};
    IEquipmentItemProvider *item_provider_ = nullptr;
    std::deque<EquipmentChange> changes_;
    std::size_t change_capacity_ = 2048;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    EquipmentReconciliationState reconciliation_state_ = EquipmentReconciliationState::Ready;
    EquipmentDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::equipment
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Foundation/error.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `EquipmentBindingState`: `Active`, `Disabled`, `Broken`
- `EquipmentChangeKind`: `ProfileCreated`, `SlotAdded`, `BindingCreated`, `BindingRemoved`, `BindingStateChanged`, `LoadoutSaved`
- `EquipmentReconciliationState`: `Ready`, `NeedsReconciliation`

### Состав типов и операции

#### `name` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
\ static constexpr name FromString(std::string_view s) noexcept \;
\ [[nodiscard]] constexpr bool IsValid() const noexcept \;
\ [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;
\ [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;
```

#### `name` (строка заголовка 33)

Публичные сигнатуры операций:

```cpp
\ static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept \;
\ [[nodiscard]] constexpr bool IsValid() const noexcept \;
\ [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;
\ [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;
```

#### `IdHash` (строка заголовка 58)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 60)

#### `EquipmentGrantDescriptor` (строка заголовка 87)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`

#### `EquipmentSlotDefinition` (строка заголовка 93)

Публичные поля и значения по умолчанию:

- `GameplayTagSet accepted_tags`
- `GameplayTagSet blocked_tags`

#### `EquipmentProfileDefinition` (строка заголовка 102)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::vector<EquipmentSlotDefinition> slots`

#### `EquipmentGrantSchema` (строка заголовка 108)

Публичные поля и значения по умолчанию:

- `std::uint32_t schema_version = 1`
- `std::size_t max_payload_bytes = 0`

#### `EquipmentProfile` (строка заголовка 115)

Публичные поля и значения по умолчанию:

- `std::vector<EquipmentSlotDefinition> slots`

#### `EquipmentItemDescriptor` (строка заголовка 123)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `bool available = false`
- `std::vector<EquipmentGrantDescriptor> grants`

#### `EquipmentBinding` (строка заголовка 131)

Публичные поля и значения по умолчанию:

- `std::vector<EquipmentSlotId> slots`
- `std::vector<EquipmentGrantDescriptor> grants`
- `EquipmentBindingState state = EquipmentBindingState::Active`

#### `EquipmentLoadout` (строка заголовка 142)

Публичные поля и значения по умолчанию:

- `std::vector<std::pair<EquipmentSlotId, EquipmentItemId>> desired`

#### `EquipPlan` (строка заголовка 149)

Публичные поля и значения по умолчанию:

- `std::vector<EquipmentSlotId> slots`
- `std::vector<EquipmentBindingId> conflicting_bindings`

#### `EquipmentChange` (строка заголовка 160)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `EquipmentChangeKind kind = EquipmentChangeKind::BindingCreated`
- `std::vector<EquipmentSlotId> slots`
- `std::vector<EquipmentGrantDescriptor> grants`
- `EquipmentBindingState old_state = EquipmentBindingState::Active`
- `EquipmentBindingState new_state = EquipmentBindingState::Active`

#### `EquipmentChangeBatch` (строка заголовка 174)

Публичные поля и значения по умолчанию:

- `std::vector<EquipmentChange> changes`
- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`

#### `EquipmentSnapshot` (строка заголовка 185)

Публичные поля и значения по умолчанию:

- `std::vector<EquipmentProfile> profiles`
- `std::vector<EquipmentBinding> bindings`
- `std::vector<EquipmentLoadout> loadouts`
- `std::uint64_t change_epoch = 1`

#### `EquipmentDiagnostics` (строка заголовка 199)

Публичные поля и значения по умолчанию:

- `std::uint64_t profiles = 0`
- `std::uint64_t bindings = 0`
- `std::uint64_t equip_operations = 0`
- `std::uint64_t rejected_operations = 0`

#### `IEquipmentItemProvider` (строка заголовка 207)

Публичные сигнатуры операций:

```cpp
virtual ~IEquipmentItemProvider() = default;
[[nodiscard]] virtual std::optional<EquipmentItemDescriptor> Describe(EquipmentItemId item) const = 0;
[[nodiscard]] virtual foundation::Result<void> ReserveForEquipment(EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) = 0;
[[nodiscard]] virtual foundation::Result<void> ReleaseFromEquipment(EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `EquipmentService` (строка заголовка 244)

Внутреннее состояние (не часть публичного API): `bool frozen_ = false`, `std::unordered_map<EquipmentProfileDefinitionId, EquipmentProfileDefinition, IdHash> profile_de`, `std::unordered_map<EquipmentGrantTypeId, EquipmentGrantSchema, IdHash> grant_schemas_`, `std::unordered_map<EquipmentProfileId, EquipmentProfile, IdHash> profiles_`, `std::unordered_map<GameplayObjectRef, EquipmentProfileId> profile_by_subject_`, `std::unordered_map<EquipmentBindingId, EquipmentBinding, IdHash> bindings_`, `std::unordered_map<EquipmentLoadoutId, EquipmentLoadout, IdHash> loadouts_`, `std::deque<EquipmentChange> changes_`, `std::size_t change_capacity_ = 2048`.

Публичные сигнатуры операций:

```cpp
explicit EquipmentService(std::size_t change_capacity = 2048);
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
void SetItemProvider(IEquipmentItemProvider *provider) noexcept;
[[nodiscard]] foundation::Result<EquipmentProfileDefinitionId> RegisterProfileDefinition(EquipmentProfileDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterGrantSchema(EquipmentGrantSchema schema);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] EquipmentReconciliationState ReconciliationState() const noexcept;
[[nodiscard]] bool NeedsReconciliation() const noexcept;
[[nodiscard]] foundation::Result<EquipmentProfileId> CreateProfile(EquipmentProfile profile);
[[nodiscard]] foundation::Result<EquipmentProfileId> CreateProfileFromDefinition(GameplayObjectRef subject, EquipmentProfileDefinitionId definition);
[[nodiscard]] foundation::Result<EquipmentSlotId> AddSlot(EquipmentProfileId profile, EquipmentSlotDefinition slot);
[[nodiscard]] const EquipmentProfile *FindProfile(GameplayObjectRef subject) const noexcept;
[[nodiscard]] const EquipmentBinding *FindBinding(EquipmentBindingId id) const noexcept;
[[nodiscard]] std::optional<EquipmentProfile> FindProfileCopy(GameplayObjectRef subject) const noexcept;
[[nodiscard]] std::optional<EquipmentBinding> FindBindingCopy(EquipmentBindingId id) const noexcept;
[[nodiscard]] std::vector<EquipmentBinding> FindBindings(GameplayObjectRef subject) const;
[[nodiscard]] std::optional<EquipmentItemId> GetEquippedItem(GameplayObjectRef subject, EquipmentSlotId slot) const;
[[nodiscard]] bool CanEquip(GameplayObjectRef subject, EquipmentItemId item, const std::vector<EquipmentSlotId> &slots) const;
[[nodiscard]] foundation::Result<EquipmentLoadoutId> SaveLoadout(EquipmentLoadout loadout);
[[nodiscard]] EquipmentChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] EquipmentSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(EquipmentSnapshot snapshot);
[[nodiscard]] EquipmentDiagnostics GetDiagnostics() const noexcept;
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
| `EquipmentService` | `EquipmentService::EquipmentService` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:51` |
| `RegisterProfileDefinition` | `EquipmentService::RegisterProfileDefinition` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:108` |
| `RegisterGrantSchema` | `EquipmentService::RegisterGrantSchema` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:125` |
| `CreateProfile` | `EquipmentService::CreateProfile` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:139` |
| `CreateProfileFromDefinition` | `EquipmentService::CreateProfileFromDefinition` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:206` |
| `AddSlot` | `EquipmentService::AddSlot` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:217` |
| `FindProfile` | `EquipmentService::FindProfile` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:266` |
| `FindBinding` | `EquipmentService::FindBinding` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:274` |
| `FindProfileCopy` | `EquipmentService::FindProfileCopy` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:279` |
| `FindBindingCopy` | `EquipmentService::FindBindingCopy` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:284` |
| `CanEquip` | `EquipmentService::CanEquip` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:339` |
| `FindBindings` | `EquipmentService::FindBindings` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:614` |
| `GetEquippedItem` | `EquipmentService::GetEquippedItem` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:626` |
| `SaveLoadout` | `EquipmentService::SaveLoadout` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:673` |
| `CaptureSnapshot` | `EquipmentService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:730` |
| `RestoreSnapshot` | `EquipmentService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:760` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetItemProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:117`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:118`
- `CreateProfile`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:122`
- `AddSlot`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:128`
- `ReadChangesSince`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:137`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:149`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:162`
- `NeedsReconciliation`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:163`
- `SaveLoadout`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:173`
- `FindProfile`: `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp:189`

