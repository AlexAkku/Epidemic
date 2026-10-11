---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "resources_production"]
source_path: "EngineFramework/GameplayWorldStateOwners/ResourcesProduction"
---

# EngineFramework / ResourcesProduction

Модуль: **ResourcesProduction**. Геймплейное производство.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/`.
**CMake targets:** `EpidemicGameFrameworkResourcesProduction`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ResourcesProduction/resources_production.h|resources_production.h]]: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`; объявленные типы: `ResourceTypeId`, `ResourceUnitId`, `ResourceStockpileId`, `ResourceNodeId`, `ProductionSiteId`, `ProductionRecipeId`, `ResourceTransactionId`, `ResourceSourceId`, `ResourceSinkId`, `ResourceReservationId`, `ProductionCapabilityId`, `ProductionPlanId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ResourcesProduction|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns resource definitions and recipes, stockpiles, nodes, production sites, reservations, transfers, capabilities/plans, quantity/index state, identity generators, revisions and the resource journal.

The service remains the single authoritative owner of the state listed below. Derived indexes and journals are not independent semantic owners.

#### Authoritative state and indexes

Admission inventory: 88 public callables, 84 mutation obligations, 1 lifecycle candidates, 21 stale-identity candidates and 3 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Mutations that span several containers must complete all fallible staging before the no-fail authoritative commit point. Query ordering and index-derived views are required to agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B03 coverage projection. Revision, ID-generator and journal publication is part of the mutation contract. Allocation/publication failure must not expose a partially advanced generator, revision, primary record, derived index or journal entry.

B03 allocation evidence uses source-private module seams rather than the shared process-global allocator override. These seams are test-only implementation details and do not expand the public API.

#### Lifecycle and identity

Lifecycle candidates and stale identity candidates from the admission inventory were reviewed. Invalid, removed, duplicate and stale IDs are required to be rejected or treated as the documented no-op without publishing false state. Terminal records are not silently resurrected by retry.

#### Persistence

Snapshot/restore is reviewed as a candidate-state operation: validate first, build off-state, preserve all live state on failure, then publish the candidate. Successful restore preserves the persistent ID-generator/revision/journal boundary represented by the snapshot contract.

#### External boundaries

Process integration remains behind resource/production ports. Three admission external-boundary candidates were reviewed. Accepted semantic state is not published before local primary/index/journal publication is proven.

External callback/provider failure is converted to the module error/result contract. Retry must not duplicate an already committed semantic side effect, and failed compensation that requires reconciliation stays represented by owned state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B03. These owners are treated as externally serialized/owner-thread services for local correctness evidence. Later Goal 6 performs the engine-wide concurrency qualification.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkResourcesProductionTests` from `EngineFramework/DevelopmentInfrastructure/Tests/resources_production_tests.cpp`.

Additional registered target: `EpidemicGameFrameworkResourcesProductionSeparationTests` from `EngineFramework/DevelopmentInfrastructure/Tests/resources_production_separation_tests.cpp`.

`G4-RESPROD-001` is closed by staging all relevant ID generators until revision/publication preflight succeeds; `TestRevisionExhaustionKeepsAllGenerators` covers stockpile, node, site, reservation, transaction, capability and plan IDs.

`G4-RESPROD-002` is closed by staged primary/index/journal publication with private fault points; `TestPublicationFaultAtomicity` checks primary insert, reserved-index insert, transaction publication and journal append.

Источник контрактов: `Documentation/EngineFramework/ResourcesProduction.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ResourceStoragePolicy` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Abstract`, `Stockpile`, `Node`, `Virtual`.
- `ResourceDecayPolicy` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `None`, `Linear`, `External`.
- `StockpileState` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Active`, `Locked`, `Disabled`, `Destroyed`.
- `ResourceNodeState` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Active`, `Depleted`, `Disabled`, `Regenerating`.
- `ProductionSiteState` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Active`, `Paused`, `Disabled`, `Destroyed`.
- `ResourceReservationState` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Active`, `Consumed`, `Released`.
- `ProductionPlanState` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Planned`, `Active`, `Satisfied`, `Paused`, `Cancelled`.
- `ResourceChangeKind` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `StockpileCreated`, `ResourceAdded`, `ResourceRemoved`, `ResourceTransferred`, `ResourceReserved`, `ResourceReservationReleased`, `ResourceReservationConsumed`, `StockpileStateChanged`, `NodeCreated`, `NodeDepleted`, `NodeRegenerated`, `ProductionSiteCreated`, `ProductionCapabilityCreated`, `ProductionPlanCreated` и другие значения.
- `StockpileOperation` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h`): `Add`, `Remove`, `Reserve`, `TransferSource`, `TransferDestination`.

### Операции и хранилища состояния

#### `ResourcesProductionService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:421`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<ResourceTypeId, ResourceType, IdHash> types_`; `std::unordered_map<ResourceStockpileId, ResourceStockpile, IdHash> stockpiles_`; `std::unordered_map<AmountKey, Fixed, AmountKeyHash> amounts_`; `std::unordered_map<AmountKey, Fixed, AmountKeyHash> reserved_amounts_`; `std::unordered_map<ResourceNodeId, ResourceNode, IdHash> nodes_`; `std::unordered_map<ProductionSiteId, ProductionSite, IdHash> sites_`; `std::unordered_map<ResourceReservationId, ResourceReservation, IdHash> reservations_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:188`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `ResourceStockpileId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ResourceStockpileId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ResourceNodeId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:62`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ResourceNodeId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ProductionSiteId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:76`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ProductionSiteId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ResourceTransactionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:104`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ResourceTransactionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ResourceSourceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:118`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ResourceSourceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ResourceSinkId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:132`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ResourceSinkId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ResourceReservationId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/include/Epidemic/GameFramework/ResourcesProduction/resources_production.h:146`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ResourceReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp` (1754 строк): `Error::Create`, `ResourcesProductionService::ResourcesProductionService`, `ResourcesProductionService::RegisterResourceType`, `ResourceTypeId::FromString`, `ResourcesProductionService::RegisterProductionRecipe`, `ProductionRecipeId::FromString`, `ResourcesProductionService::FindResourceType`, `ResourcesProductionService::FindProductionRecipe`, `ResourcesProductionService::ValidateQuantities`, `ResourcesProductionService::CanonicalizeQuantities`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.resources.publication_failed`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1600`
- `gameplay.resources.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:263`
- `gameplay.resources.invalid_type`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:248`
- `gameplay.resources.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1378`
- `gameplay.resources.invalid_recipe`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:296`
- `gameplay.resources.invalid_quantity`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:338`
- `gameplay.resources.quantity_overflow`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:354`
- `gameplay.resources.stockpile_missing`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:605`
- `gameplay.resources.stockpile_unavailable`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:379`
- `gameplay.resources.invalid_stockpile`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:468`
- `gameplay.resources.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1330`
- `gameplay.resources.invalid_node`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:531`
- `gameplay.resources.invalid_site`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:580`
- `gameplay.resources.stockpile_terminal`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:610`
- `gameplay.resources.active_reservations`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:618`
- `gameplay.resources.amount_overflow`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1022`
- `gameplay.resources.shortage`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1017`
- `gameplay.resources.duplicate_reservation`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:855`
- `gameplay.resources.reservation_missing`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:942`
- `gameplay.resources.reservation_invalid`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:950`
- `gameplay.resources.invalid_transfer`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:996`
- `gameplay.resources.duplicate_transaction`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1074`
- `gameplay.resources.node_missing`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1210`
- `gameplay.resources.node_disabled`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1212`
- `gameplay.resources.regeneration_interval_conflict`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1215`
- `gameplay.time_overflow`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1221`
- `gameplay.resources.invalid_capability`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1266`
- `gameplay.resources.duplicate_capability`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1297`
- `gameplay.resources.invalid_plan`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1323`
- `gameplay.resources.duplicate_plan`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1354`
- `gameplay.resources.plan_missing`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1374`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1538`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.resources.publication_failed` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1600`)

```cpp
1598     }
 1599     if (internal_test::ConsumeAllocationFault(internal_test::AllocationFaultPoint::RestoreDerived))
 1600         return foundation::Result<void>::Failure(Error("gameplay.resources.publication_failed", "resource restore derived staging failed"));
 1601     for (auto &v : s.reservations)
 1602     {
```

#### `gameplay.resources.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:263`)

```cpp
261     if (frozen_)
  262         return foundation::Result<ProductionRecipeId>::Failure(
  263             Error("gameplay.resources.registry_frozen", "resource registry is frozen"));
  264     if (recipe.canonical_name.empty())
  265         return foundation::Result<ProductionRecipeId>::Failure(
```

#### `gameplay.resources.invalid_type` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:248`)

```cpp
246         if (!types_.emplace(id, std::move(type)).second)
  247             return foundation::Result<ResourceTypeId>::Failure(
  248                 Error("gameplay.resources.invalid_type", "duplicate resource type"));
  249     }
  250     catch (...)
```

#### `gameplay.resources.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:1378`)

```cpp
1376         return foundation::Result<void>::Success();
 1377     if (revision_.value == std::numeric_limits<std::uint64_t>::max())
 1378         return foundation::Result<void>::Failure(Error("gameplay.resources.revision_exhausted", "resources revision exhausted"));
 1379     const Revision next_revision{revision_.value + 1};
 1380     const std::array<ResourceChange, 1> additions{{
```

#### `gameplay.resources.invalid_recipe` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:296`)

```cpp
294         if (!recipes_.emplace(id, std::move(recipe)).second)
  295             return foundation::Result<ProductionRecipeId>::Failure(
  296                 Error("gameplay.resources.invalid_recipe", "duplicate production recipe"));
  297     }
  298     catch (...)
```

#### `gameplay.resources.invalid_quantity` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:338`)

```cpp
336         {
  337             return foundation::Result<std::vector<ResourceQuantity>>::Failure(
  338                 Error("gameplay.resources.invalid_quantity", "invalid resource quantity"));
  339         }
  340         out.push_back(q);
```

#### `gameplay.resources.quantity_overflow` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:354`)

```cpp
352             {
  353                 return foundation::Result<std::vector<ResourceQuantity>>::Failure(
  354                     Error("gameplay.resources.quantity_overflow", "resource quantity overflow"));
  355             }
  356             canonical.back().amount = sum;
```

#### `gameplay.resources.stockpile_missing` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:605`)

```cpp
603     auto it = stockpiles_.find(id);
  604     if (it == stockpiles_.end() || !IsValidStockpileState(state))
  605         return foundation::Result<void>::Failure(Error("gameplay.resources.stockpile_missing", "stockpile missing"));
  606     if (it->second.state == state)
  607         return foundation::Result<void>::Success();
```

#### `gameplay.resources.stockpile_unavailable` (`EngineFramework/GameplayWorldStateOwners/ResourcesProduction/src/resources_production.cpp:379`)

```cpp
377         (void)operation;
  378         return foundation::Result<void>::Failure(
  379             Error("gameplay.resources.stockpile_unavailable", "stockpile is not operational"));
  380     }
  381     return foundation::Result<void>::Success();
```
