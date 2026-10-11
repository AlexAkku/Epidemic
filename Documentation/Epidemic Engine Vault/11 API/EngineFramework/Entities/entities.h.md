---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h"
---

# Entities: entities.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`.

**Модуль:** [[06 EngineFramework/Модуль Entities|Entities]].
**Размер исходного файла:** 15182 байт.
**Подключение:** `#include <Epidemic/GameFramework/Entities/entities.h>`.

## Объявления типов (лексический индекс)
`EntityId`, `EntityArchetypeId`, `EntityPartId`, `EntityPartRef`, `EntityIdHash`, `EntityArchetypeIdHash`, `EntityPartIdHash`, `EntityHandle`, `EntityPersistencePolicy`, `EntityMaterializationPolicy`, `EntityLifecycleState`, `EntityMaterializationState`, `EntityDestroyReason`, `EntityPartDefinition`, `EntityArchetypeDefinition`, `EntityRecord`, `PendingEntityDestruction`, `CreateEntityRequest`, `CreateEntityResult`, `EntityConversionPolicy`, `EntityChangeKind`, `EntityChange`, `EntityChangeBatch`, `EntitySnapshot`, `EntitiesDiagnostics`, `EntityService`, `Slot`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::entities
{
struct EntityId
{
    GameplayObjectId value{};
    [[nodiscard]] static constexpr EntityId FromRaw(std::uint64_t high, std::uint64_t low) noexcept { return EntityId{GameplayObjectId::FromRaw(high, low)}; }
    [[nodiscard]] static constexpr EntityId FromString(std::string_view value) noexcept { return EntityId{GameplayObjectId::FromString(value)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t High() const noexcept { return value.High(); }
    [[nodiscard]] constexpr std::uint64_t Low() const noexcept { return value.Low(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EntityId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EntityId&) const noexcept = default;
};

struct EntityArchetypeId
{
    TypeId value{};
    [[nodiscard]] static constexpr EntityArchetypeId FromString(std::string_view value) noexcept { return EntityArchetypeId{TypeId::FromString(value)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EntityArchetypeId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EntityArchetypeId&) const noexcept = default;
};

struct EntityPartId
{
    TypeId value{};
    [[nodiscard]] static constexpr EntityPartId FromString(std::string_view value) noexcept { return EntityPartId{TypeId::FromString(value)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value.Raw(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EntityPartId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EntityPartId&) const noexcept = default;
};

struct EntityPartRef
{
    EntityId entity{};
    EntityPartId part{};
    [[nodiscard]] constexpr bool IsValid() const noexcept { return entity.IsValid() && part.IsValid(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EntityPartRef&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const EntityPartRef&) const noexcept = default;
};

struct EntityIdHash { [[nodiscard]] std::size_t operator()(const EntityId& id) const noexcept { return std::hash<GameplayObjectId>{}(id.value); } };
struct EntityArchetypeIdHash { [[nodiscard]] std::size_t operator()(EntityArchetypeId id) const noexcept { return std::hash<TypeId>{}(id.value); } };
struct EntityPartIdHash { [[nodiscard]] std::size_t operator()(EntityPartId id) const noexcept { return std::hash<TypeId>{}(id.value); } };

struct EntityHandle
{
    std::uint32_t slot = 0;
    std::uint32_t generation = 0;
    [[nodiscard]] constexpr bool IsValid() const noexcept { return generation != 0; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const EntityHandle&) const noexcept = default;
};

enum class EntityPersistencePolicy
{
    Transient, // runtime-only entity; omitted from SaveGame snapshots
    Session,   // authoritative for the current session, omitted from SaveGame snapshots
    Persistent // restored by SaveGame
};
enum class EntityMaterializationPolicy { AbstractAllowed, PreferMaterialized, RequireMaterialized };
enum class EntityLifecycleState { Creating, Alive, Dormant, PendingDestroy, Destroyed, Removed };
enum class EntityMaterializationState { Abstract, Materializing, Materialized, Dematerializing };
enum class EntityDestroyReason { Destroyed, Removed, Consumed, Expired, Converted, OwnerRequest };

struct EntityPartDefinition
{
    EntityPartId id{};
    std::string canonical_name;
    EntityPartId parent{};
    GameplayTagSet tags;
};

struct EntityArchetypeDefinition
{
    EntityArchetypeId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    std::vector<EntityPartDefinition> parts;
    EntityPersistencePolicy persistence = EntityPersistencePolicy::Persistent;
    EntityMaterializationPolicy materialization = EntityMaterializationPolicy::AbstractAllowed;
};

struct EntityRecord
{
    EntityId id{};
    EntityArchetypeId archetype{};
    EntityLifecycleState lifecycle = EntityLifecycleState::Creating;
    EntityMaterializationState materialization = EntityMaterializationState::Abstract;
    GameplayTagSet instance_tags;
    Revision revision{};
    EntityPersistencePolicy persistence = EntityPersistencePolicy::Persistent;
};

struct PendingEntityDestruction
{
    EntityId entity{};
    EntityDestroyReason reason = EntityDestroyReason::Destroyed;
    GameplayContext context{};
};

struct CreateEntityRequest
{
    EntityArchetypeId archetype{};
    std::optional<EntityId> requested_id{};
    GameplayTagSet additional_tags;
    std::optional<EntityPersistencePolicy> persistence{};
    GameplayContext context{};
};

struct CreateEntityResult { EntityId id{}; EntityHandle handle{}; };
struct EntityConversionPolicy { bool preserve_instance_tags = true; bool preserve_persistence = true; };

enum class EntityChangeKind
{
    Created,
    Activated,
    Deactivated,
    DestroyRequested,
    Destroyed,
    Removed,
    Converted,
    MaterializationChanged,
    TagAdded,
    TagRemoved,
};

struct EntityChange
{
    std::uint64_t sequence = 0;
    EntityChangeKind kind = EntityChangeKind::Created;
    EntityId entity{};
    EntityArchetypeId previous_archetype{};
    EntityArchetypeId archetype{};
    EntityLifecycleState previous_lifecycle = EntityLifecycleState::Creating;
    EntityLifecycleState lifecycle = EntityLifecycleState::Alive;
    EntityMaterializationState previous_materialization = EntityMaterializationState::Abstract;
    EntityMaterializationState materialization = EntityMaterializationState::Abstract;
    TagId tag{};
    EntityDestroyReason destroy_reason = EntityDestroyReason::Destroyed;
    GameplayContext context{};
    Revision revision{};
};

struct EntityChangeBatch
{
    std::vector<EntityChange> changes;
    bool snapshot_required = false;
    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct EntitySnapshot
{
    std::vector<EntityRecord> records;
    std::vector<PendingEntityDestruction> pending_destruction;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot id_generator{};
    Revision revision{};
    std::uint64_t change_epoch = 1;
};

struct EntitiesDiagnostics
{
    std::uint64_t entity_count = 0;
    std::uint64_t persistent_entities = 0;
    std::uint64_t abstract_entities = 0;
    std::uint64_t materialized_entities = 0;
    std::uint64_t pending_destruction = 0;
    std::uint64_t creates = 0;
    std::uint64_t destroys = 0;
    std::uint64_t invalid_handle_lookups = 0;
};

class EntityService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<EntityService>;

  public:
    EntityService();

    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept { return GameplayDomainId::FromString("framework.entities"); }
    [[nodiscard]] static constexpr GameplayObjectRef ToGameplayObjectRef(EntityId id) noexcept { return GameplayObjectRef{Domain(), id.value}; }
    [[nodiscard]] static constexpr EntityId FromGameplayObjectRef(GameplayObjectRef ref) noexcept { return ref.domain == Domain() ? EntityId{ref.id} : EntityId{}; }
    [[nodiscard]] static constexpr GameplayObjectPartRef ToGameplayObjectPartRef(EntityPartRef ref) noexcept
    {
        return ref.IsValid() ? GameplayObjectPartRef{ToGameplayObjectRef(ref.entity), ref.part.value} : GameplayObjectPartRef{};
    }
    [[nodiscard]] static constexpr EntityPartRef FromGameplayObjectPartRef(GameplayObjectPartRef ref) noexcept
    {
        const auto entity = FromGameplayObjectRef(ref.object);
        return entity.IsValid() && ref.part.IsValid() ? EntityPartRef{entity, EntityPartId{ref.part}} : EntityPartRef{};
    }

    [[nodiscard]] foundation::Result<EntityArchetypeId> RegisterArchetype(EntityArchetypeDefinition definition);
    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }

    [[nodiscard]] const EntityArchetypeDefinition* FindArchetype(EntityArchetypeId id) const noexcept;
    [[nodiscard]] const EntityPartDefinition* FindPartDefinition(EntityArchetypeId archetype, EntityPartId part) const noexcept;

    // Entity materialization policy is an orchestration requirement declared by the archetype.
    // Creating a RequireMaterialized entity is still a valid semantic operation that starts in
    // Abstract state; runtime materialization is coordinated by RuntimeBridge/integration code.
    [[nodiscard]] std::optional<EntityMaterializationPolicy> MaterializationPolicyOf(EntityId id) const noexcept;
    [[nodiscard]] bool RequiresMaterialization(EntityId id) const noexcept;
    [[nodiscard]] std::optional<EntityPartRef> GetPart(EntityId entity, EntityPartId part) const;
    [[nodiscard]] std::vector<EntityPartRef> GetParts(EntityId entity) const;
    [[nodiscard]] std::vector<EntityPartRef> GetPartAncestors(EntityPartRef part) const;

    [[nodiscard]] foundation::Result<CreateEntityResult> Create(CreateEntityRequest request);
    [[nodiscard]] foundation::Result<void> Activate(EntityId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Deactivate(EntityId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RequestDestroy(EntityId id, EntityDestroyReason reason, GameplayContext context = {});
    [[nodiscard]] foundation::Result<std::vector<EntityRecord>> CommitPendingDestruction();
    [[nodiscard]] foundation::Result<void> Remove(EntityId id, GameplayContext context = {});

    [[nodiscard]] foundation::Result<void> Convert(EntityId id, EntityArchetypeId new_archetype, EntityConversionPolicy policy = {}, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SetMaterializationState(EntityId id, EntityMaterializationState state, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> AddTag(EntityId id, TagId tag, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> RemoveTag(EntityId id, TagId tag, GameplayContext context = {});

    [[nodiscard]] std::optional<EntityRecord> Find(EntityId id) const;
    [[nodiscard]] std::optional<EntityRecord> Resolve(EntityHandle handle) const;
    [[nodiscard]] EntityHandle HandleOf(EntityId id) const noexcept;
    [[nodiscard]] bool Exists(EntityId id) const noexcept { return id_to_slot_.contains(id); }

    [[nodiscard]] std::vector<EntityRecord> AllEntities() const;
    [[nodiscard]] std::vector<EntityRecord> FindByArchetype(EntityArchetypeId archetype) const;
    [[nodiscard]] std::vector<EntityRecord> FindByTag(TagId tag, const GameplayTagRegistry& registry) const;
    [[nodiscard]] std::vector<EntityRecord> FindByLifecycle(EntityLifecycleState lifecycle) const;
    [[nodiscard]] std::vector<EntityRecord> FindByMaterialization(EntityMaterializationState state) const;

    private:

        [[nodiscard]] std::vector<EntityChange> ChangesSinceSequence(std::uint64_t sequence) const;

    public:
    [[nodiscard]] EntityChangeBatch ReadChangesSince(ChangeCursor cursor) const;
    [[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept { return {journal_epoch_, last_change_sequence_}; }
    void PruneChangesBefore(std::uint64_t sequence);

    [[nodiscard]] EntitySnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(EntitySnapshot snapshot);
    [[nodiscard]] EntitiesDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }

  private:
    struct Slot { EntityRecord record{}; std::uint32_t generation = 1; bool occupied = false; };

    [[nodiscard]] foundation::Result<std::uint32_t> AllocateSlot();
    [[nodiscard]] EntityRecord* FindMutable(EntityId id) noexcept;
    [[nodiscard]] const EntityRecord* FindInternal(EntityId id) const noexcept;
    [[nodiscard]] Slot* ResolveSlot(EntityHandle handle) noexcept;
    [[nodiscard]] const Slot* ResolveSlot(EntityHandle handle) const noexcept;
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    [[nodiscard]] bool CanRecordChanges(std::size_t count) const noexcept;
    void RecordChange(EntityChange change) noexcept;
    [[nodiscard]] foundation::Result<void> ValidateArchetypeParts(EntityArchetypeDefinition& definition) const;

    std::unordered_map<EntityArchetypeId, EntityArchetypeDefinition, EntityArchetypeIdHash> archetypes_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_slots_;
    std::unordered_map<EntityId, std::uint32_t, EntityIdHash> id_to_slot_;
    std::vector<EntityId> pending_destroy_;
    std::unordered_map<EntityId, EntityDestroyReason, EntityIdHash> pending_destroy_reasons_;
    std::unordered_map<EntityId, GameplayContext, EntityIdHash> pending_destroy_contexts_;

    MonotonicIdGenerator<GameplayObjectId> ids_;
    Revision revision_{};
    bool frozen_ = false;

    std::vector<EntityChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t last_change_sequence_ = 0;
    std::uint64_t journal_epoch_ = 1;

    std::uint64_t creates_ = 0;
    std::uint64_t destroys_ = 0;
    mutable std::uint64_t invalid_handle_lookups_ = 0;
};
} // namespace epidemic::gameplay::entities

namespace std
{
template <> struct hash<epidemic::gameplay::entities::EntityId> { [[nodiscard]] size_t operator()(const epidemic::gameplay::entities::EntityId& id) const noexcept { return epidemic::gameplay::entities::EntityIdHash{}(id); } };
template <> struct hash<epidemic::gameplay::entities::EntityArchetypeId> { [[nodiscard]] size_t operator()(epidemic::gameplay::entities::EntityArchetypeId id) const noexcept { return epidemic::gameplay::entities::EntityArchetypeIdHash{}(id); } };
template <> struct hash<epidemic::gameplay::entities::EntityPartId> { [[nodiscard]] size_t operator()(epidemic::gameplay::entities::EntityPartId id) const noexcept { return epidemic::gameplay::entities::EntityPartIdHash{}(id); } };
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `EntityPersistencePolicy`: `Transient`
- `EntityMaterializationPolicy`: `AbstractAllowed`, `PreferMaterialized`, `RequireMaterialized`
- `EntityLifecycleState`: `Creating`, `Alive`, `Dormant`, `PendingDestroy`, `Destroyed`, `Removed`
- `EntityMaterializationState`: `Abstract`, `Materializing`, `Materialized`, `Dematerializing`
- `EntityDestroyReason`: `Destroyed`, `Removed`, `Consumed`, `Expired`, `Converted`, `OwnerRequest`
- `EntityChangeKind`: `Created`, `Activated`, `Deactivated`, `DestroyRequested`, `Destroyed`, `Removed`, `Converted`, `MaterializationChanged`, `TagAdded`, `TagRemoved`

### Состав типов и операции

#### `EntityId` (строка заголовка 16)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr EntityId FromRaw(std::uint64_t high, std::uint64_t low) noexcept;
[[nodiscard]] static constexpr EntityId FromString(std::string_view value) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t High() const noexcept;
[[nodiscard]] constexpr std::uint64_t Low() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const EntityId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EntityId&) const noexcept = default;
```

#### `EntityArchetypeId` (строка заголовка 29)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr EntityArchetypeId FromString(std::string_view value) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const EntityArchetypeId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EntityArchetypeId&) const noexcept = default;
```

#### `EntityPartId` (строка заголовка 40)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr EntityPartId FromString(std::string_view value) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const EntityPartId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EntityPartId&) const noexcept = default;
```

#### `EntityPartRef` (строка заголовка 51)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const EntityPartRef&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const EntityPartRef&) const noexcept = default;
```

#### `EntityIdHash` (строка заголовка 61)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const EntityId& id) const noexcept;
```

#### `EntityArchetypeIdHash` (строка заголовка 62)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(EntityArchetypeId id) const noexcept;
```

#### `EntityPartIdHash` (строка заголовка 63)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(EntityPartId id) const noexcept;
```

#### `EntityHandle` (строка заголовка 65)

Публичные поля и значения по умолчанию:

- `std::uint32_t slot = 0`
- `std::uint32_t generation = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const EntityHandle&) const noexcept = default;
```

#### `EntityPartDefinition` (строка заголовка 85)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`

#### `EntityArchetypeDefinition` (строка заголовка 93)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `std::vector<EntityPartDefinition> parts`
- `EntityPersistencePolicy persistence = EntityPersistencePolicy::Persistent`
- `EntityMaterializationPolicy materialization = EntityMaterializationPolicy::AbstractAllowed`

#### `EntityRecord` (строка заголовка 103)

Публичные поля и значения по умолчанию:

- `EntityLifecycleState lifecycle = EntityLifecycleState::Creating`
- `EntityMaterializationState materialization = EntityMaterializationState::Abstract`
- `GameplayTagSet instance_tags`
- `EntityPersistencePolicy persistence = EntityPersistencePolicy::Persistent`

#### `PendingEntityDestruction` (строка заголовка 114)

Публичные поля и значения по умолчанию:

- `EntityDestroyReason reason = EntityDestroyReason::Destroyed`

#### `CreateEntityRequest` (строка заголовка 121)

Публичные поля и значения по умолчанию:

- `GameplayTagSet additional_tags`

#### `EntityConversionPolicy` (строка заголовка 131)

Публичные поля и значения по умолчанию:

- `bool preserve_instance_tags = true`
- `bool preserve_persistence = true`

#### `EntityChange` (строка заголовка 147)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `EntityChangeKind kind = EntityChangeKind::Created`
- `EntityLifecycleState previous_lifecycle = EntityLifecycleState::Creating`
- `EntityLifecycleState lifecycle = EntityLifecycleState::Alive`
- `EntityMaterializationState previous_materialization = EntityMaterializationState::Abstract`
- `EntityMaterializationState materialization = EntityMaterializationState::Abstract`
- `EntityDestroyReason destroy_reason = EntityDestroyReason::Destroyed`

#### `EntityChangeBatch` (строка заголовка 164)

Публичные поля и значения по умолчанию:

- `std::vector<EntityChange> changes`
- `bool snapshot_required = false`

#### `EntitySnapshot` (строка заголовка 172)

Публичные поля и значения по умолчанию:

- `std::vector<EntityRecord> records`
- `std::vector<PendingEntityDestruction> pending_destruction`
- `std::uint64_t change_epoch = 1`

#### `EntitiesDiagnostics` (строка заголовка 181)

Публичные поля и значения по умолчанию:

- `std::uint64_t entity_count = 0`
- `std::uint64_t persistent_entities = 0`
- `std::uint64_t abstract_entities = 0`
- `std::uint64_t materialized_entities = 0`
- `std::uint64_t pending_destruction = 0`
- `std::uint64_t creates = 0`
- `std::uint64_t destroys = 0`
- `std::uint64_t invalid_handle_lookups = 0`

#### `EntityService` (строка заголовка 193)

Внутреннее состояние (не часть публичного API): `std::unordered_map<EntityArchetypeId, EntityArchetypeDefinition, EntityArchetypeIdHash> archety`, `std::vector<Slot> slots_`, `std::vector<std::uint32_t> free_slots_`, `std::unordered_map<EntityId, std::uint32_t, EntityIdHash> id_to_slot_`, `std::vector<EntityId> pending_destroy_`, `std::unordered_map<EntityId, EntityDestroyReason, EntityIdHash> pending_destroy_reasons_`, `std::unordered_map<EntityId, GameplayContext, EntityIdHash> pending_destroy_contexts_`, `MonotonicIdGenerator<GameplayObjectId> ids_`, `bool frozen_ = false`.

Публичные сигнатуры операций:

```cpp
EntityService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] static constexpr GameplayObjectRef ToGameplayObjectRef(EntityId id) noexcept;
[[nodiscard]] static constexpr EntityId FromGameplayObjectRef(GameplayObjectRef ref) noexcept;
[[nodiscard]] static constexpr GameplayObjectPartRef ToGameplayObjectPartRef(EntityPartRef ref) noexcept;
[[nodiscard]] static constexpr EntityPartRef FromGameplayObjectPartRef(GameplayObjectPartRef ref) noexcept;
[[nodiscard]] foundation::Result<EntityArchetypeId> RegisterArchetype(EntityArchetypeDefinition definition);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const EntityArchetypeDefinition* FindArchetype(EntityArchetypeId id) const noexcept;
[[nodiscard]] const EntityPartDefinition* FindPartDefinition(EntityArchetypeId archetype, EntityPartId part) const noexcept;
runtime materialization is coordinated by RuntimeBridge/integration code. [[nodiscard]] std::optional<EntityMaterializationPolicy> MaterializationPolicyOf(EntityId id) const noexcept;
[[nodiscard]] bool RequiresMaterialization(EntityId id) const noexcept;
[[nodiscard]] std::optional<EntityPartRef> GetPart(EntityId entity, EntityPartId part) const;
[[nodiscard]] std::vector<EntityPartRef> GetParts(EntityId entity) const;
[[nodiscard]] std::vector<EntityPartRef> GetPartAncestors(EntityPartRef part) const;
[[nodiscard]] foundation::Result<CreateEntityResult> Create(CreateEntityRequest request);
[[nodiscard]] foundation::Result<std::vector<EntityRecord>> CommitPendingDestruction();
[[nodiscard]] std::optional<EntityRecord> Find(EntityId id) const;
[[nodiscard]] std::optional<EntityRecord> Resolve(EntityHandle handle) const;
[[nodiscard]] EntityHandle HandleOf(EntityId id) const noexcept;
[[nodiscard]] bool Exists(EntityId id) const noexcept;
[[nodiscard]] std::vector<EntityRecord> AllEntities() const;
[[nodiscard]] std::vector<EntityRecord> FindByArchetype(EntityArchetypeId archetype) const;
[[nodiscard]] std::vector<EntityRecord> FindByTag(TagId tag, const GameplayTagRegistry& registry) const;
[[nodiscard]] std::vector<EntityRecord> FindByLifecycle(EntityLifecycleState lifecycle) const;
[[nodiscard]] std::vector<EntityRecord> FindByMaterialization(EntityMaterializationState state) const;
[[nodiscard]] EntityChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesBefore(std::uint64_t sequence);
[[nodiscard]] EntitySnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(EntitySnapshot snapshot);
[[nodiscard]] EntitiesDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `Slot` (строка заголовка 267)

Публичные поля и значения по умолчанию:

- `std::uint32_t generation = 1`
- `bool occupied = false`

#### `hash` (строка заголовка 304)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(const epidemic::gameplay::entities::EntityId& id) const noexcept;
```

#### `hash` (строка заголовка 305)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::gameplay::entities::EntityArchetypeId id) const noexcept;
```

#### `hash` (строка заголовка 306)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::gameplay::entities::EntityPartId id) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `EntityService` | `EntityService::EntityService` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:80` |
| `RegisterArchetype` | `EntityService::RegisterArchetype` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:128` |
| `FindArchetype` | `EntityService::FindArchetype` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:143` |
| `FindPartDefinition` | `EntityService::FindPartDefinition` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:149` |
| `MaterializationPolicyOf` | `EntityService::MaterializationPolicyOf` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:157` |
| `RequiresMaterialization` | `EntityService::RequiresMaterialization` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:165` |
| `GetPart` | `EntityService::GetPart` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:171` |
| `GetParts` | `EntityService::GetParts` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:178` |
| `GetPartAncestors` | `EntityService::GetPartAncestors` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:190` |
| `Create` | `EntityService::Create` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:259` |
| `Find` | `EntityService::Find` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:342` |
| `Resolve` | `EntityService::Resolve` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:362` |
| `HandleOf` | `EntityService::HandleOf` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:373` |
| `CommitPendingDestruction` | `EntityService::CommitPendingDestruction` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:446` |
| `AllEntities` | `EntityService::AllEntities` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:591` |
| `FindByArchetype` | `EntityService::FindByArchetype` | `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:598` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterArchetype`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:21`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:23`
- `Create`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:26`
- `Find`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:30`
- `Resolve`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:72`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:81`
- `RequiresMaterialization`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:85`
- `GetPart`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:86`
- `GetPartAncestors`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:87`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:100`
- `CommitPendingDestruction`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:113`
- `CurrentRevision`: `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp:120`

