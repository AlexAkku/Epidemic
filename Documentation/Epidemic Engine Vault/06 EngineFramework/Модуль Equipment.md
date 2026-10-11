---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "equipment"]
source_path: "EngineFramework/GameplayWorldStateOwners/Equipment"
---

# EngineFramework / Equipment

Модуль: **Equipment**. Экипировка и equip rules.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Equipment/`.
**CMake targets:** `EpidemicGameFrameworkEquipment`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Equipment/equipment.h|equipment.h]]: `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h`; объявленные типы: `name`, `IdHash`, `EquipmentBindingState`, `EquipmentChangeKind`, `EquipmentReconciliationState`, `EquipmentGrantDescriptor`, `EquipmentSlotDefinition`, `EquipmentProfileDefinition`, `EquipmentGrantSchema`, `EquipmentProfile`, `EquipmentItemDescriptor`, `EquipmentBinding`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Equipment|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns equipment profiles, slot definitions, entity loadouts, item bindings, equipment revision/journal state and reconciliation metadata.

The service remains the single authoritative owner of the state listed below. Derived indexes and journals are not independent semantic owners.

#### Authoritative state and indexes

Admission inventory: 72 public callables, 72 mutation obligations, 1 lifecycle candidates, 18 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Mutations that span several containers must complete all fallible staging before the no-fail authoritative commit point. Query ordering and index-derived views are required to agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B03 coverage projection. Revision, ID-generator and journal publication is part of the mutation contract. Allocation/publication failure must not expose a partially advanced generator, revision, primary record, derived index or journal entry.

B03 allocation evidence uses source-private module seams rather than the shared process-global allocator override. These seams are test-only implementation details and do not expand the public API.

#### Lifecycle and identity

Lifecycle candidates and stale identity candidates from the admission inventory were reviewed. Invalid, removed, duplicate and stale IDs are required to be rejected or treated as the documented no-op without publishing false state. Terminal records are not silently resurrected by retry.

#### Persistence

Snapshot/restore is reviewed as a candidate-state operation: validate first, build off-state, preserve all live state on failure, then publish the candidate. Successful restore preserves the persistent ID-generator/revision/journal boundary represented by the snapshot contract.

#### External boundaries

Item-provider reservation/reconciliation is kept behind the Equipment provider contract. Admission emitted no separate external-boundary candidate rows.

External callback/provider failure is converted to the module error/result contract. Retry must not duplicate an already committed semantic side effect, and failed compensation that requires reconciliation stays represented by owned state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B03. These owners are treated as externally serialized/owner-thread services for local correctness evidence. Later Goal 6 performs the engine-wide concurrency qualification.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkEquipmentTests` from `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp`.

Источник контрактов: `Documentation/EngineFramework/Equipment.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `EquipmentBindingState` (`EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h`): `Active`, `Disabled`, `Broken`.
- `EquipmentChangeKind` (`EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h`): `ProfileCreated`, `SlotAdded`, `BindingCreated`, `BindingRemoved`, `BindingStateChanged`, `LoadoutSaved`.
- `EquipmentReconciliationState` (`EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h`): `Ready`, `NeedsReconciliation`.

### Операции и хранилища состояния

#### `EquipmentService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:244`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<EquipmentProfileDefinitionId, EquipmentProfileDefinition, IdHash> profile_definitions_`; `std::unordered_map<EquipmentGrantTypeId, EquipmentGrantSchema, IdHash> grant_schemas_`; `std::unordered_map<EquipmentProfileId, EquipmentProfile, IdHash> profiles_`; `std::unordered_map<GameplayObjectRef, EquipmentProfileId> profile_by_subject_`; `std::unordered_map<EquipmentBindingId, EquipmentBinding, IdHash> bindings_`; `std::unordered_map<EquipmentLoadoutId, EquipmentLoadout, IdHash> loadouts_`; `std::deque<EquipmentChange> changes_`.

#### `IEquipmentItemProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:207`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEquipmentItemProvider() = default;
[[nodiscard]] virtual std::optional<EquipmentItemDescriptor> Describe(EquipmentItemId item) const = 0;
[[nodiscard]] virtual foundation::Result<void> ReserveForEquipment(EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) = 0;
[[nodiscard]] virtual foundation::Result<void> ReleaseFromEquipment(EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) = 0;
// Restore reconciliation must be idempotent and must not create a second reservation. // Providers that persist reservation state should verify that the restored binding is backed by // exactly the reservation expected for this subject/item pair. [[nodiscard]] virtual foundation::Result<void> ReconcileEquipmentReservation( EquipmentItemId, GameplayObjectRef, EquipmentBindingId, GameplayContext);
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `name`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:33`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
\ static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept \;
```

#### `EquipmentGrantDescriptor`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:87`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> payload`.

#### `EquipmentSlotDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:93`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `GameplayTagSet accepted_tags`; `GameplayTagSet blocked_tags`.

#### `EquipmentProfileDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:102`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string canonical_name`; `std::vector<EquipmentSlotDefinition> slots`.

#### `EquipmentGrantSchema`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:108`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t schema_version = 1`; `std::size_t max_payload_bytes = 0`.

#### `EquipmentProfile`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:115`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<EquipmentSlotDefinition> slots`.

#### `EquipmentItemDescriptor`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Equipment/include/Epidemic/GameFramework/Equipment/equipment.h:123`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `GameplayTagSet tags`; `bool available = false`; `std::vector<EquipmentGrantDescriptor> grants`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Equipment/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp` (948 строк): `Error::Create`, `EquipmentService::EquipmentService`, `EquipmentService::PrepareRevision`, `EquipmentService::EnsureMutationReady`, `EquipmentService::ReconcileRestoredBindings`, `EquipmentService::RegisterProfileDefinition`, `EquipmentProfileDefinitionId::FromString`, `EquipmentService::RegisterGrantSchema`, `EquipmentService::MutableProfile`, `EquipmentService::CreateProfile`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:58`
- `gameplay.equipment.reconciliation_required`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:66`
- `gameplay.equipment.item_provider_missing`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:641`
- `gameplay.equipment.provider_exception`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:872`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:127`
- `gameplay.equipment.invalid_profile_definition`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:120`
- `gameplay.equipment.invalid_grant_schema`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:130`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:145`
- `gameplay.equipment.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:156`
- `gameplay.equipment.invalid_slot`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:231`
- `gameplay.equipment.duplicate_slot`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:235`
- `gameplay.equipment.storage_failed`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:699`
- `gameplay.equipment.profile_definition_missing`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:210`
- `gameplay.equipment.profile_missing`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:224`
- `gameplay.equipment.cannot_equip`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:383`
- `gameplay.equipment.stale_item`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:438`
- `gameplay.equipment.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:457`
- `gameplay.equipment.not_ready`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:420`
- `gameplay.equipment.stale_plan`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:472`
- `gameplay.equipment.item_already_bound`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:445`
- `gameplay.equipment.invalid_grants`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:449`
- `gameplay.equipment.binding_missing`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:588`
- `gameplay.equipment.invalid_binding_state`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:591`
- `gameplay.equipment.invalid_loadout`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:685`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:766`
- `gameplay.equipment.publication_failed`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:840`
- `gameplay.equipment.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:888`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/equipment_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:58`)

```cpp
56     if (!next)
   57         return foundation::Result<Revision>::Failure(
   58             Error("gameplay.revision_exhausted", "equipment revision counter is exhausted"));
   59     return foundation::Result<Revision>::Success(*next);
   60 }
```

#### `gameplay.equipment.reconciliation_required` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:66`)

```cpp
64     if (reconciliation_state_ == EquipmentReconciliationState::NeedsReconciliation)
   65         return foundation::Result<void>::Failure(
   66             Error("gameplay.equipment.reconciliation_required", "restored equipment bindings must be reconciled before mutation"));
   67     return foundation::Result<void>::Success();
   68 }
```

#### `gameplay.equipment.item_provider_missing` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:641`)

```cpp
639         return foundation::Result<void>::Failure(Error("gameplay.equipment.invalid_loadout", "loadout subject has no equipment profile"));
  640     if (!item_provider_ && !loadout.desired.empty())
  641         return foundation::Result<void>::Failure(Error("gameplay.equipment.item_provider_missing", "item provider is required to validate loadout items"));
  642 
  643     std::sort(loadout.desired.begin(), loadout.desired.end(), [](const auto& a, const auto& b) {
```

#### `gameplay.equipment.provider_exception` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:872`)

```cpp
870             {
  871                 return foundation::Result<void>::Failure(
  872                     Error("gameplay.equipment.provider_exception", "item provider threw while validating restored loadout"));
  873             }
  874             if (!descriptor || !SlotAccepts(*slot, *descriptor))
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:127`)

```cpp
125 foundation::Result<void> EquipmentService::RegisterGrantSchema(EquipmentGrantSchema schema)
  126 {
  127     if (frozen_) return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "equipment registry is frozen"));
  128     if (!schema.type.IsValid() || !schema.payload_type.IsValid() || schema.schema_version == 0 || schema.max_payload_bytes == 0 ||
  129         grant_schemas_.contains(schema.type))
```

#### `gameplay.equipment.invalid_profile_definition` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:120`)

```cpp
118         return foundation::Result<EquipmentProfileDefinitionId>::Failure(Error("gameplay.equipment.invalid_profile_definition", "profile definition contains invalid slot"));
  119     if (std::adjacent_find(d.slots.begin(), d.slots.end(), [](const auto& a, const auto& b){ return a.id == b.id; }) != d.slots.end())
  120         return foundation::Result<EquipmentProfileDefinitionId>::Failure(Error("gameplay.equipment.invalid_profile_definition", "profile definition contains duplicate slot"));
  121     const auto id = d.id;
  122     profile…
```

#### `gameplay.equipment.invalid_grant_schema` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:130`)

```cpp
128     if (!schema.type.IsValid() || !schema.payload_type.IsValid() || schema.schema_version == 0 || schema.max_payload_bytes == 0 ||
  129         grant_schemas_.contains(schema.type))
  130         return foundation::Result<void>::Failure(Error("gameplay.equipment.invalid_grant_schema", "invalid or duplicate equipment grant schema"));
  131     grant_schemas_.emplace(schema.type, std::move(schema));
  132     return foundation::Result<void>::Success();
```

#### `gameplay.registry_not_frozen` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:145`)

```cpp
143     if (!frozen_)
  144         return foundation::Result<EquipmentProfileId>::Failure(
  145             Error("gameplay.registry_not_frozen", "equipment registry must be frozen before runtime mutation"));
  146     if (!p.subject.IsValid() || profile_by_subject_.contains(p.subject))
  147         return foundation::Result<EquipmentProfileId>::Failure(
```

#### `gameplay.equipment.invalid_profile` (`EngineFramework/GameplayWorldStateOwners/Equipment/src/equipment.cpp:156`)

```cpp
154     if (!p.id.IsValid() || profiles_.contains(p.id))
  155         return foundation::Result<EquipmentProfileId>::Failure(
  156             Error("gameplay.equipment.invalid_profile", "invalid or duplicate equipment profile id"));
  157 
  158     for (auto &slot : p.slots)
```
