---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "entities"]
source_path: "EngineFramework/GameplayWorldStateOwners/Entities"
---

# EngineFramework / Entities

Модуль: **Entities**. Gameplay entities и lifecycles.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Entities/`.
**CMake targets:** `EpidemicGameFrameworkEntities`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Entities/entities.h|entities.h]]: `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`; объявленные типы: `EntityId`, `EntityArchetypeId`, `EntityPartId`, `EntityPartRef`, `EntityIdHash`, `EntityArchetypeIdHash`, `EntityPartIdHash`, `EntityHandle`, `EntityPersistencePolicy`, `EntityMaterializationPolicy`, `EntityLifecycleState`, `EntityMaterializationState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Entities|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility

Owns gameplay entity archetype registration, entity records, lifecycle, part references, materialization state and entity change journal. It is the only owner of entity slot/generation bookkeeping in this module.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`.

#### Dependency list

Direct production links are `EpidemicGameFrameworkFoundation` and `EpidemicFoundation`. No peer GameplayWorldStateOwner or Runtime module dependency is added by this delta.

#### External ports/callbacks/providers/backends

No external backend. Public inputs are gameplay value types and GameplayTagRegistry query support; no callback is retained.

#### Authoritative state

Archetype registry; slot vector; free-slot list; EntityId to slot index; pending-destruction IDs/reasons/contexts; entity ID generator; revision; bounded change journal.

#### Derived/cache/index state

id_to_slot_ and free_slots_ derive from slots_; pending destruction maps describe pending lifecycle work and are rebuilt/validated on restore.

#### ID spaces, generations, revisions and cursors

EntityId uses the service-owned MonotonicIdGenerator. EntityHandle carries slot plus generation and stale handles do not resolve after slot reuse. Revision and ChangeCursor epoch/sequence are monotonic and checked at boundaries.

#### State machines

Registry is mutable until Freeze. Entity lifecycle covers Creating, Alive, Dormant, PendingDestroy, Destroyed and Removed; materialization has a separate state machine. Destroy requests are staged, terminal destruction is committed, and physical removal is explicit.

Источник контрактов: `Documentation/EngineFramework/Entities.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `EntityPersistencePolicy` (`EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`): `Transient`.
- `EntityMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`): `AbstractAllowed`, `PreferMaterialized`, `RequireMaterialized`.
- `EntityLifecycleState` (`EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`): `Creating`, `Alive`, `Dormant`, `PendingDestroy`, `Destroyed`, `Removed`.
- `EntityMaterializationState` (`EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`): `Abstract`, `Materializing`, `Materialized`, `Dematerializing`.
- `EntityDestroyReason` (`EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`): `Destroyed`, `Removed`, `Consumed`, `Expired`, `Converted`, `OwnerRequest`.
- `EntityChangeKind` (`EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h`): `Created`, `Activated`, `Deactivated`, `DestroyRequested`, `Destroyed`, `Removed`, `Converted`, `MaterializationChanged`, `TagAdded`, `TagRemoved`.

### Операции и хранилища состояния

#### `EntityService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:193`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<EntityArchetypeId, EntityArchetypeDefinition, EntityArchetypeIdHash> archetypes_`; `std::vector<Slot> slots_`; `std::vector<std::uint32_t> free_slots_`; `std::unordered_map<EntityId, std::uint32_t, EntityIdHash> id_to_slot_`; `std::vector<EntityId> pending_destroy_`; `std::unordered_map<EntityId, EntityDestroyReason, EntityIdHash> pending_destroy_reasons_`; `std::unordered_map<EntityId, GameplayContext, EntityIdHash> pending_destroy_contexts_`; `MonotonicIdGenerator<GameplayObjectId> ids_`.

#### `EntityId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr EntityId FromRaw(std::uint64_t high, std::uint64_t low) noexcept;
[[nodiscard]] constexpr std::uint64_t High() const noexcept;
[[nodiscard]] constexpr std::uint64_t Low() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `EntityArchetypeId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:29`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `EntityPartId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:40`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `EntityPartRef`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:51`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `EntityHandle`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:65`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t slot = 0`; `std::uint32_t generation = 0`.

#### `EntityIdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:61`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const EntityId& id) const noexcept;
```

#### `EntityArchetypeIdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:62`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(EntityArchetypeId id) const noexcept;
```

#### `EntityPartIdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:63`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(EntityPartId id) const noexcept;
```

#### `hash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:304`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] size_t operator()(const epidemic::gameplay::entities::EntityId& id) const noexcept;
```

#### `hash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:305`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] size_t operator()(epidemic::gameplay::entities::EntityArchetypeId id) const noexcept;
```

#### `hash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Entities/include/Epidemic/GameFramework/Entities/entities.h:306`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] size_t operator()(epidemic::gameplay::entities::EntityPartId id) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Entities/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp` (780 строк): `GameplayObjectId::FromString`, `Error::Create`, `EntityService::EntityService`, `EntityService::ValidateArchetypeParts`, `EntityPartId::FromString`, `EntityService::RegisterArchetype`, `EntityArchetypeId::FromString`, `EntityService::FindArchetype`, `EntityService::FindPartDefinition`, `EntityService::MaterializationPolicyOf`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.entities.instances`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:35`
- `gameplay.entity_part_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:94`
- `gameplay.entity_part_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:100`
- `gameplay.entity_part_duplicate`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:104`
- `gameplay.entity_part_parent_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:111`
- `gameplay.entity_part_cycle`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:119`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:130`
- `gameplay.entity_archetype_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:132`
- `gameplay.entity_archetype_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:135`
- `gameplay.already_registered`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:136`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:207`
- `gameplay.entity_capacity_exhausted`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:254`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:693`
- `gameplay.entity_persistence_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:264`
- `gameplay.entity_archetype_unknown`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:532`
- `gameplay.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:583`
- `gameplay.entity_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:280`
- `gameplay.entity_duplicate`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:281`
- `gameplay.entity_handle_generation_exhausted`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:703`
- `gameplay.entity_storage_failed`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:756`
- `gameplay.entity_unknown`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:580`
- `gameplay.entity_invalid_state`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:581`
- `gameplay.entity_destroy_pending`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:419`
- `gameplay.entity_destroy_reason_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:423`
- `gameplay.entity_materialization_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:548`
- `gameplay.tag_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:564`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:692`
- `gameplay.entity_snapshot_invalid`: `EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:727`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/entities_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.entities.instances` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:35`)

```cpp
33 [[nodiscard]] constexpr std::uint64_t EntityInstanceIdScope() noexcept
   34 {
   35     return GameplayObjectId::FromString("framework.entities.instances").High();
   36 }
   37
```

#### `gameplay.entity_part_invalid` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:94`)

```cpp
92         if (part.canonical_name.empty())
   93         {
   94             return foundation::Result<void>::Failure(Error("gameplay.entity_part_invalid", "entity part canonical name must not be empty"));
   95         }
   96         const auto expected = EntityPartId::FromString(part.canonical_name);
```

#### `gameplay.entity_part_id_mismatch` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:100`)

```cpp
98         if (part.id != expected)
   99         {
  100             return foundation::Result<void>::Failure(Error("gameplay.entity_part_id_mismatch", "entity part id does not match canonical name"));
  101         }
  102         if (!by_id.emplace(part.id, index).second)
```

#### `gameplay.entity_part_duplicate` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:104`)

```cpp
102         if (!by_id.emplace(part.id, index).second)
  103         {
  104             return foundation::Result<void>::Failure(Error("gameplay.entity_part_duplicate", "entity archetype contains duplicate part ids"));
  105         }
  106     }
```

#### `gameplay.entity_part_parent_invalid` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:111`)

```cpp
109         if (part.parent.IsValid() && (part.parent == part.id || !by_id.contains(part.parent)))
  110         {
  111             return foundation::Result<void>::Failure(Error("gameplay.entity_part_parent_invalid", "entity part parent must reference another part in the same archetype"));
  112         }
  113         std::unordered_set<EntityPartId, EntityPartIdHash> seen;
```

#### `gameplay.entity_part_cycle` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:119`)

```cpp
117             if (!seen.insert(current).second)
  118             {
  119                 return foundation::Result<void>::Failure(Error("gameplay.entity_part_cycle", "entity part hierarchy contains a cycle"));
  120             }
  121             current = definition.parts[by_id.at(current)].parent;
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:130`)

```cpp
128 foundation::Result<EntityArchetypeId> EntityService::RegisterArchetype(EntityArchetypeDefinition definition)
  129 {
  130     if (frozen_) return foundation::Result<EntityArchetypeId>::Failure(Error("gameplay.registry_frozen", "entity archetype registry is frozen"));
  131     if (definition.canonical_name.empty() || !IsValid(definition.persistence) || !IsValid(definition.materialization))
  132         return foundation::Result<EntityArchetypeId>::Failure(Error("gameplay.entity_archetype_invalid", "entity archetype definition contains inv…
```

#### `gameplay.entity_archetype_invalid` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:132`)

```cpp
130     if (frozen_) return foundation::Result<EntityArchetypeId>::Failure(Error("gameplay.registry_frozen", "entity archetype registry is frozen"));
  131     if (definition.canonical_name.empty() || !IsValid(definition.persistence) || !IsValid(definition.materialization))
  132         return foundation::Result<EntityArchetypeId>::Failure(Error("gameplay.entity_archetype_invalid", "entity archetype definition contains invalid fields"));
  133     const auto expected = EntityArchetypeId::FromString(definition.canonical_name);
  134     if (!de…
```

#### `gameplay.entity_archetype_id_mismatch` (`EngineFramework/GameplayWorldStateOwners/Entities/src/entities.cpp:135`)

```cpp
133     const auto expected = EntityArchetypeId::FromString(definition.canonical_name);
  134     if (!definition.id.IsValid()) definition.id = expected;
  135     if (definition.id != expected) return foundation::Result<EntityArchetypeId>::Failure(Error("gameplay.entity_archetype_id_mismatch", "entity archetype id does not match canonical name"));
  136     if (archetypes_.contains(definition.id)) return foundation::Result<EntityArchetypeId>::Failure(Error("gameplay.already_registered", "entity archetype is already registered"));
  137     if …
```
