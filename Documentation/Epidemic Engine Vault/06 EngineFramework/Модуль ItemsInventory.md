---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "items_inventory"]
source_path: "EngineFramework/GameplayWorldStateOwners/ItemsInventory"
---

# EngineFramework / ItemsInventory

Модуль: **ItemsInventory**. Предметы и инвентари.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/ItemsInventory/`.
**CMake targets:** `EpidemicGameFrameworkItemsInventory`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ItemsInventory/items_inventory.h|items_inventory.h]]: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`; объявленные типы: `name`, `IdHash`, `RegisteredPayload`, `ItemStackPolicy`, `ItemDurabilityPolicy`, `ItemChargePolicy`, `ItemLocationKind`, `ContainerState`, `ReservationState`, `ItemChangeKind`, `ItemProperty`, `ItemDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ItemsInventory|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility and ownership

Owns item definitions, item instances, containers, reservations, transfer preparation state, bindings, identity generators, revisions and the item change journal.

The service remains the single authoritative owner of the state listed below. Derived indexes and journals are not independent semantic owners.

#### Authoritative state and indexes

Admission inventory: 82 public callables, 92 mutation obligations, 1 lifecycle candidates, 23 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Mutations that span several containers must complete all fallible staging before the no-fail authoritative commit point. Query ordering and index-derived views are required to agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B03 coverage projection. Revision, ID-generator and journal publication is part of the mutation contract. Allocation/publication failure must not expose a partially advanced generator, revision, primary record, derived index or journal entry.

B03 allocation evidence uses source-private module seams rather than the shared process-global allocator override. These seams are test-only implementation details and do not expand the public API.

#### Lifecycle and identity

Lifecycle candidates and stale identity candidates from the admission inventory were reviewed. Invalid, removed, duplicate and stale IDs are required to be rejected or treated as the documented no-op without publishing false state. Terminal records are not silently resurrected by retry.

#### Persistence

Snapshot/restore is reviewed as a candidate-state operation: validate first, build off-state, preserve all live state on failure, then publish the candidate. Successful restore preserves the persistent ID-generator/revision/journal boundary represented by the snapshot contract.

#### External boundaries

Transfer and reservation protocols are represented by owned durable records. No external-boundary candidates were emitted by the admission scanner.

External callback/provider failure is converted to the module error/result contract. Retry must not duplicate an already committed semantic side effect, and failed compensation that requires reconciliation stays represented by owned state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B03. These owners are treated as externally serialized/owner-thread services for local correctness evidence. Later Goal 6 performs the engine-wide concurrency qualification.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkItemsInventoryTests` from `EngineFramework/DevelopmentInfrastructure/Tests/items_inventory_tests.cpp`.

Источник контрактов: `Documentation/EngineFramework/ItemsInventory.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ItemStackPolicy` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `NonStackable`, `StackByDefinition`, `StackByEquivalentState`, `CustomRegistered`.
- `ItemDurabilityPolicy` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `None`, `InstanceValue`.
- `ItemChargePolicy` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `None`, `InstanceValue`.
- `ItemLocationKind` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `None`, `Container`, `World`, `Equipped`, `Reserved`, `InTransit`, `Destroyed`.
- `ContainerState` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `Active`, `Locked`, `Disabled`, `Destroyed`.
- `ReservationState` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `Active`, `Consumed`, `Released`.
- `ItemChangeKind` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h`): `ItemCreated`, `ItemDestroyed`, `StackSplit`, `StacksMerged`, `TransferCommitted`, `ContainerCreated`, `ContainerStateChanged`, `ContainerRemoved`, `ReservationCreated`, `ReservationReleased`, `ReservationConsumed`, `DurabilityChanged`, `ChargesChanged`.

### Операции и хранилища состояния

#### `ItemsInventoryService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:317`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<ItemDefinitionId, ItemDefinition, IdHash> definitions_`; `std::unordered_map<ItemPropertyTypeId, ItemPropertySchema, IdHash> property_schemas_`; `std::unordered_map<ContainerPolicyId, const IContainerPolicy*, IdHash> container_policies_`; `std::unordered_map<ItemInstanceId, ItemInstance, IdHash> items_`; `std::unordered_map<ContainerId, ContainerRecord, IdHash> containers_`; `std::unordered_map<ItemReservationId, ItemReservation, IdHash> reservations_`; `std::unordered_map<ContainerId, std::vector<ItemInstanceId>, IdHash> container_items_`.

#### `IContainerPolicy`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:300`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IContainerPolicy() = default;
[[nodiscard]] virtual ContainerPolicyId Id() const noexcept = 0;
[[nodiscard]] virtual bool AllowsInsert(const ContainerPolicyContext& context) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:63`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `ItemProperty`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:155`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `RegisteredPayload payload`.

#### `ItemLocation`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:178`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ItemLocationKind kind = ItemLocationKind::None`; `RegisteredPayload payload`.

#### `ItemDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:161`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string canonical_name`; `GameplayTagSet tags`; `Fixed base_weight = 0`; `Fixed base_volume = 0`; `ItemStackPolicy stack_policy = ItemStackPolicy::NonStackable`; `ItemDurabilityPolicy durability_policy = ItemDurabilityPolicy::None`; `ItemChargePolicy charge_policy = ItemChargePolicy::None`; `Fixed default_durability = 0`.

#### `ItemInstance`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:187`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed quantity = 1`; `Fixed durability = 0`; `Fixed charges = 0`; `std::vector<ItemProperty> properties`.

#### `ItemCreateRequest`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:198`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ItemInstance item`.

#### `ItemTransferPlan`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:223`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed quantity = 0`.

#### `ItemReservation`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:235`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed quantity = 0`; `ReservationState state = ReservationState::Active`.

#### `ItemChange`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:245`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t sequence = 0`; `ItemChangeKind kind = ItemChangeKind::ItemCreated`; `Fixed quantity = 0`.

#### `ItemsChangeBatch`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ItemsInventory/include/Epidemic/GameFramework/ItemsInventory/items_inventory.h:259`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool snapshot_required = false`; `std::uint64_t oldest_available_sequence = 0`; `std::uint64_t latest_sequence = 0`; `std::vector<ItemChange> changes`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp` (1602 строк): `Error::Create`, `ItemsInventoryService::ItemsInventoryService`, `ItemsInventoryService::PrepareRevision`, `ItemsInventoryService::RegisterDefinition`, `ItemDefinitionId::FromString`, `ItemsInventoryService::RegisterPropertySchema`, `ItemsInventoryService::RegisterContainerPolicy`, `ItemsInventoryService::FindDefinition`, `ItemsInventoryService::CreateContainer`, `ItemsInventoryService::SetContainerState`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:116`
- `gameplay.items.frozen`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:162`
- `gameplay.items.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:144`
- `gameplay.items.duplicate_definition`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:131`
- `gameplay.items.definition_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:134`
- `gameplay.items.unsupported_stack_policy`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:139`
- `gameplay.items.invalid_property_schema`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:155`
- `gameplay.items.invalid_container_policy`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:164`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:1286`
- `gameplay.items.invalid_container`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:181`
- `gameplay.items.container_policy_missing`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:184`
- `gameplay.items.duplicate_container`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:214`
- `gameplay.items.parent_missing`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:197`
- `gameplay.items.container_cycle`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:200`
- `gameplay.items.container_depth`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:203`
- `gameplay.items.storage_failed`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:1087`
- `gameplay.items.invalid_container_state`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:233`
- `gameplay.items.container_missing`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:253`
- `gameplay.items.container_not_empty`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:260`
- `gameplay.items.definition_missing`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:329`
- `gameplay.items.invalid_durability`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:347`
- `gameplay.items.invalid_charges`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:349`
- `gameplay.items.invalid_quantity`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:340`
- `gameplay.items.nonstackable_quantity`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:343`
- `gameplay.items.invalid_item_state`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:345`
- `gameplay.items.duplicate_world_identity`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:874`
- `gameplay.items.container_reject`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:548`
- `gameplay.items.capacity_overflow`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:363`
- `gameplay.items.container_capacity`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:879`
- `gameplay.items.duplicate_item`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:813`
- `gameplay.items.item_missing`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:1157`
- `gameplay.items.item_reserved`: `EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:602`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:116`)

```cpp
114     if (!next)
  115         return foundation::Result<Revision>::Failure(
  116             Error("gameplay.revision_exhausted", "items revision counter is exhausted"));
  117     return foundation::Result<Revision>::Success(*next);
  118 }
```

#### `gameplay.items.frozen` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:162`)

```cpp
160 foundation::Result<void> ItemsInventoryService::RegisterContainerPolicy(const IContainerPolicy& policy)
  161 {
  162     if (frozen_) return foundation::Result<void>::Failure(Error("gameplay.items.frozen", "item definitions are frozen"));
  163     if (!policy.Id().IsValid() || container_policies_.contains(policy.Id()))
  164         return foundation::Result<void>::Failure(Error("gameplay.items.invalid_container_policy", "invalid or duplicate container policy"));
```

#### `gameplay.items.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:144`)

```cpp
142         d.default_charges > d.max_charges)
  143         return foundation::Result<ItemDefinitionId>::Failure(
  144             Error("gameplay.items.invalid_definition", "negative item definition values"));
  145     d.revision = Revision{1};
  146     const auto id = d.id;
```

#### `gameplay.items.duplicate_definition` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:131`)

```cpp
129     if (definitions_.contains(d.id))
  130         return foundation::Result<ItemDefinitionId>::Failure(
  131             Error("gameplay.items.duplicate_definition", "duplicate item definition"));
  132     const auto expected = ItemDefinitionId::FromString(d.canonical_name);
  133     if (!d.canonical_name.empty() && d.id.IsValid() && d.id != expected)
```

#### `gameplay.items.definition_id_mismatch` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:134`)

```cpp
132     const auto expected = ItemDefinitionId::FromString(d.canonical_name);
  133     if (!d.canonical_name.empty() && d.id.IsValid() && d.id != expected)
  134         return foundation::Result<ItemDefinitionId>::Failure(Error("gameplay.items.definition_id_mismatch", "item definition id does not match canonical name"));
  135     if (!IsValid(d.stack_policy) || !IsValid(d.durability_policy) || !IsValid(d.charge_policy))
  136         return foundation::Result<ItemDefinitionId>::Failure(
```

#### `gameplay.items.unsupported_stack_policy` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:139`)

```cpp
137             Error("gameplay.items.invalid_definition", "item definition contains an invalid policy enum"));
  138     if (d.stack_policy == ItemStackPolicy::CustomRegistered)
  139         return foundation::Result<ItemDefinitionId>::Failure(Error("gameplay.items.unsupported_stack_policy", "custom stack policy requires an explicit comparator contract"));
  140     if (d.base_weight < 0 || d.base_volume < 0 || d.max_durability < 0 || d.max_charges < 0 ||
  141         d.default_durability < 0 || d.default_charges < 0 || d.default_durability …
```

#### `gameplay.items.invalid_property_schema` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:155`)

```cpp
153     if (!schema.type.IsValid() || !schema.payload_type.IsValid() || schema.schema_version == 0 ||
  154         schema.max_payload_bytes == 0 || property_schemas_.contains(schema.type))
  155         return foundation::Result<void>::Failure(Error("gameplay.items.invalid_property_schema", "invalid or duplicate item property schema"));
  156     property_schemas_.emplace(schema.type, std::move(schema));
  157     return foundation::Result<void>::Success();
```

#### `gameplay.items.invalid_container_policy` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:164`)

```cpp
162     if (frozen_) return foundation::Result<void>::Failure(Error("gameplay.items.frozen", "item definitions are frozen"));
  163     if (!policy.Id().IsValid() || container_policies_.contains(policy.Id()))
  164         return foundation::Result<void>::Failure(Error("gameplay.items.invalid_container_policy", "invalid or duplicate container policy"));
  165     container_policies_.emplace(policy.Id(), &policy);
  166     return foundation::Result<void>::Success();
```

#### `gameplay.registry_not_frozen` (`EngineFramework/GameplayWorldStateOwners/ItemsInventory/src/items_inventory.cpp:1286`)

```cpp
1284             foundation::Error::Create("gameplay.change_journal.epoch_exhausted", "change journal epoch is exhausted"));
 1285     }
 1286     if (!frozen_) return foundation::Result<void>::Failure(Error("gameplay.registry_not_frozen", "item registry must be frozen before restore"));
 1287     if (s.revision.value == 0 && (!s.items.empty() || !s.containers.empty() || !s.reservations.empty()))
 1288         return foundation::Result<void>::Failure(Error("gameplay.items.restore_invalid", "non-empty snapshot requires a non-zero revision"));
```
