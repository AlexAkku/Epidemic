---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "construction"]
source_path: "EngineFramework/GameplayWorldStateOwners/Construction"
---

# EngineFramework / Construction

Модуль: **Construction**. Gameplay construction.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Construction/`.
**CMake targets:** `EpidemicGameFrameworkConstruction`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Construction/construction.h|construction.h]]: `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`; объявленные типы: `PlacementRuleId`, `PlacementPlanId`, `PlacementExecutionId`, `ConstructionRecipeId`, `ConstructionSiteId`, `PlacementSocketId`, `PlacedObjectId`, `ConstructionSocketReservationId`, `PlacementOutputId`, `ConstructionCostTypeId`, `PlacementOutputTypeId`, `PlacementReasonId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Construction|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns construction definitions, placement validation/plans, cost/socket reservations, sites, output envelopes, socket occupancy and construction change journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`. No B06 public-surface change was required.

#### Dependency list

Direct production links are GameFramework Foundation and EngineBase Foundation. No Traversal, Navigation or World peer dependency is introduced.

#### External ports/callbacks/providers/backends

Placement/cost/output ports are borrowed. The service owns semantic reservation and output-envelope state, not external world/runtime objects.

#### Authoritative state

Rules/recipes, plans, sites, sockets/reservations, execution/output records, ID generators, global revision and bounded journal.

#### Derived/cache/index state

Validation summaries and diagnostics derive from authoritative placement/site/socket state.

#### ID spaces, generations, revisions and cursors

Plan/site/execution/output/reservation identities are service-controlled. Revision preflight is used before state-changing publication and journal cursors are bounded.

#### State machines

Placement plan and construction site lifecycles include validate/reserve/commit/cancel/expire/start/pause/resume/progress/complete/fail/destroy/compact paths. Output envelopes have queued/acknowledged/dead-letter states.

Источник контрактов: `Documentation/EngineFramework/Construction.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PlacementTargetKind` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `Free`, `Surface`, `Socket`, `Attach`, `Area`.
- `PlacementAvailability` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `Available`, `Unavailable`, `Blocked`, `RequiresResource`, `RequiresPermission`, `RequiresCapability`, `InvalidTarget`, `NotMaterialized`, `Unsupported`.
- `PlacementCommitPolicy` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `Instant`, `CreateSite`, `PreviewOnly`.
- `ConstructionCostPolicy` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `ReserveThenCommit`, `Free`.
- `ConstructionSiteState` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `Planned`, `UnderConstruction`, `Paused`, `Completed`, `Cancelled`, `Failed`, `ResourcesCommitted`, `Destroyed`.
- `PlacementPlanState` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `Prepared`, `Committed`, `Cancelled`, `Expired`.
- `SocketState` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `Free`, `Occupied`, `Disabled`, `Reserved`.
- `ConstructionChangeKind` (`EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`): `PlacementValidated`, `PlacementRejected`, `PlacementCommitted`, `PlacedObjectCreated`, `SiteCreated`, `SiteStarted`, `SiteCompleted`, `SiteCancelled`, `SocketReserved`, `SocketReleased`, `SocketOccupied`, `SitePaused`, `SiteResumed`, `SiteDestroyed` и другие значения.

### Операции и хранилища состояния

#### `ConstructionService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:504`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
ConstructionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterPlacementDefinition(PlacementDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterRecipe(ConstructionRecipe recipe);
[[nodiscard]] foundation::Result<void> RegisterSocket(PlacementSocket socket);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
void SetPlacementProvider(const IConstructionPlacementProvider *provider) noexcept;
void SetCostProvider(IConstructionCostProvider *provider) noexcept;
[[nodiscard]] const ConstructionRecipe *FindRecipe(ConstructionRecipeId id) const noexcept;
[[nodiscard]] const PlacementDefinition *FindPlacementDefinition(PlacementRuleId id) const noexcept;
[[nodiscard]] const PlacementSocket *FindSocket(PlacementSocketId id) const noexcept;
[[nodiscard]] foundation::Result<PlacementValidationResult> ValidatePlacement(const PlacementRequest &request) const;
[[nodiscard]] foundation::Result<PlacementPlan> PreparePlacementPlan(const PlacementRequest &request);
[[nodiscard]] foundation::Result<PlacementCommitResult> CommitPlacement(PlacementPlanId plan);
[[nodiscard]] foundation::Result<PlacementCommitResult> CommitPlacement(const PlacementPlan &plan);
[[nodiscard]] const PlacementPlan *FindPlan(PlacementPlanId id) const noexcept;
[[nodiscard]] const ConstructionSite *FindSite(ConstructionSiteId id) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<PlacementRuleId, PlacementDefinition, IdHash> placement_definitions_`; `std::unordered_map<ConstructionRecipeId, ConstructionRecipe, IdHash> recipes_`; `std::unordered_map<PlacementSocketId, PlacementSocket, IdHash> sockets_`; `std::unordered_map<PlacementPlanId, PlacementPlan, IdHash> plans_`; `std::unordered_map<ConstructionSiteId, ConstructionSite, IdHash> sites_`; `std::unordered_map<ConstructionSocketReservationId, ConstructionSocketReservation, IdHash> socket_reservations_by_id_`; `std::unordered_map<PlacementSocketId, ConstructionSocketReservationId, IdHash> socket_reservation_by_socket_`; `std::vector<PlacedObjectRecord> placed_objects_`.

#### `IConstructionCostProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:441`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IConstructionCostProvider() = default;
[[nodiscard]] virtual foundation::Result<bool> CanAfford(GameplayObjectRef actor, const ConstructionCost &cost, const GameplayContext &context) const = 0;
[[nodiscard]] virtual foundation::Result<ConstructionCostReservation> Reserve(GameplayObjectRef actor, const ConstructionCost &cost, const GameplayContext &context) = 0;
virtual void Commit(const ConstructionCostReservation &reservation, const GameplayContext &context) noexcept = 0;
virtual void Release(const ConstructionCostReservation &reservation, const GameplayContext &context) noexcept = 0;
```

#### `IConstructionPlacementProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:433`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IConstructionPlacementProvider() = default;
[[nodiscard]] virtual foundation::Result<PlacementSemanticProjection> Project( const PlacementRequest &request) const = 0;
[[nodiscard]] virtual Revision CurrentRevision() const noexcept = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:141`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `PlacementPlanId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:27`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PlacementPlanId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PlacementExecutionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:39`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PlacementExecutionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ConstructionSiteId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:62`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ConstructionSiteId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PlacedObjectId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:84`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PlacedObjectId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `ConstructionSocketReservationId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h:95`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr ConstructionSocketReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Construction/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp` (1597 строк): `Error::Create`, `ConstructionService::ConstructionService`, `TypeId::FromString`, `ConstructionService::SetPlacementProvider`, `ConstructionService::SetCostProvider`, `ConstructionService::RegisterPlacementDefinition`, `ConstructionService::RegisterRecipe`, `ConstructionService::RegisterSocket`, `ConstructionService::FindRecipe`, `ConstructionService::FindPlacementDefinition`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.construction.plan`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:53`
- `framework.construction.site`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:54`
- `framework.construction.placed`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:55`
- `framework.construction.execution`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:56`
- `framework.construction.output`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:57`
- `framework.construction.socket_reservation`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:58`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:117`
- `gameplay.construction.invalid_placement_definition`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:89`
- `gameplay.construction.invalid_recipe`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:101`
- `gameplay.construction.invalid_cost`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:105`
- `gameplay.construction.invalid_output`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:109`
- `gameplay.construction.invalid_socket`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:120`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:1302`
- `gameplay.construction.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:1482`
- `construction.invalid_request`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:181`
- `construction.pitch_not_allowed`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:186`
- `construction.roll_not_allowed`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:188`
- `construction.yaw_not_aligned`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:190`
- `construction.position_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:196`
- `construction.surface_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:200`
- `construction.socket_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:204`
- `construction.attach_target_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:208`
- `construction.area_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:212`
- `construction.socket_missing`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:220`
- `construction.socket_reserved`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:224`
- `construction.socket_reservation_mismatch`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:228`
- `construction.socket_blocked`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:231`
- `construction.socket_tag_mismatch`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:234`
- `construction.semantic_provider_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:253`
- `construction.target_not_materialized`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:257`
- `construction.area_not_allowed`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:259`
- `construction.permission_required`: `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:261`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.construction.plan` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:53`)

```cpp
51 
   52 ConstructionService::ConstructionService()
   53     : plan_ids_(TypeId::FromString("framework.construction.plan").Raw()),
   54       site_ids_(TypeId::FromString("framework.construction.site").Raw()),
   55       placed_ids_(TypeId::FromString("framework.construction.placed").Raw()),
```

#### `framework.construction.site` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:54`)

```cpp
52 ConstructionService::ConstructionService()
   53     : plan_ids_(TypeId::FromString("framework.construction.plan").Raw()),
   54       site_ids_(TypeId::FromString("framework.construction.site").Raw()),
   55       placed_ids_(TypeId::FromString("framework.construction.placed").Raw()),
   56       execution_ids_(TypeId::FromString("framework.construction.execution").Raw()),
```

#### `framework.construction.placed` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:55`)

```cpp
53     : plan_ids_(TypeId::FromString("framework.construction.plan").Raw()),
   54       site_ids_(TypeId::FromString("framework.construction.site").Raw()),
   55       placed_ids_(TypeId::FromString("framework.construction.placed").Raw()),
   56       execution_ids_(TypeId::FromString("framework.construction.execution").Raw()),
   57       output_ids_(TypeId::FromString("framework.construction.output").Raw()),
```

#### `framework.construction.execution` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:56`)

```cpp
54       site_ids_(TypeId::FromString("framework.construction.site").Raw()),
   55       placed_ids_(TypeId::FromString("framework.construction.placed").Raw()),
   56       execution_ids_(TypeId::FromString("framework.construction.execution").Raw()),
   57       output_ids_(TypeId::FromString("framework.construction.output").Raw()),
   58       socket_reservation_ids_(TypeId::FromString("framework.construction.socket_reservation").Raw())
```

#### `framework.construction.output` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:57`)

```cpp
55       placed_ids_(TypeId::FromString("framework.construction.placed").Raw()),
   56       execution_ids_(TypeId::FromString("framework.construction.execution").Raw()),
   57       output_ids_(TypeId::FromString("framework.construction.output").Raw()),
   58       socket_reservation_ids_(TypeId::FromString("framework.construction.socket_reservation").Raw())
   59 {
```

#### `framework.construction.socket_reservation` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:58`)

```cpp
56       execution_ids_(TypeId::FromString("framework.construction.execution").Raw()),
   57       output_ids_(TypeId::FromString("framework.construction.output").Raw()),
   58       socket_reservation_ids_(TypeId::FromString("framework.construction.socket_reservation").Raw())
   59 {
   60 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:117`)

```cpp
115 {
  116     if (frozen_)
  117         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "construction registry frozen"));
  118     if (!socket.id.IsValid() || !socket.owner.IsValid() || sockets_.contains(socket.id))
  119         return foundation::Result<void>::Failure(
```

#### `gameplay.construction.invalid_placement_definition` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:89`)

```cpp
87         placement_definitions_.contains(definition.id))
   88         return foundation::Result<void>::Failure(
   89             Error("gameplay.construction.invalid_placement_definition", "invalid placement definition"));
   90     placement_definitions_.emplace(definition.id, std::move(definition));
   91     return foundation::Result<void>::Success();
```

#### `gameplay.construction.invalid_recipe` (`EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:101`)

```cpp
99         !FootprintDimensionsValid(recipe.footprint_size_mm))
  100         return foundation::Result<void>::Failure(
  101             Error("gameplay.construction.invalid_recipe", "invalid construction recipe"));
  102     for (const auto &cost : recipe.costs)
  103         if (!cost.type.IsValid() || cost.amount_micro < 0)
```
