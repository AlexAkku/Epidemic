---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "traversal"]
source_path: "EngineFramework/GameplayWorldStateOwners/Traversal"
---

# EngineFramework / Traversal

Модуль: **Traversal**. Прохождение и перемещение персонажей.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Traversal/`.
**CMake targets:** `EpidemicGameFrameworkTraversal`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Traversal/traversal.h|traversal.h]]: `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`; объявленные типы: `TraversalModeId`, `TraversalProfileId`, `TraversalSessionId`, `TraversalRouteId`, `TraversalCapabilityId`, `TraversalCapabilityGrantId`, `TraversalCarrierRoleId`, `TraversalReasonId`, `IdHash`, `RefHash`, `WorldPosition`, `TraversalMaterializationPolicy`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Traversal|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns traversal modes/profiles, subject traversal state, capability grants, routes, sessions, carrier bindings, generators, revision and change journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`. B06 does not change the public header.

#### Dependency list

Direct production links are GameFramework Foundation and EngineBase Foundation only.

#### External ports/callbacks/providers/backends

Materialization capability is queried through a borrowed port. Runtime/navigation ownership remains outside this module.

#### Authoritative state

Modes/profiles, subject states, capability grants, routes, live sessions, carrier bindings, ID generators, revision and bounded journal.

#### Derived/cache/index state

Carrier passenger queries and mode/capability availability are derived from authoritative maps plus immutable definitions.

#### ID spaces, generations, revisions and cursors

Session, route and capability-grant IDs use monotonic generators. Every state-changing family now checks revision exhaustion before authoritative mutation (`G4-TRAV-001`).

#### State machines

Traversal session covers active/suspended and terminal completion/cancel/failure. Carrier board/disembark and route/capability lifetime are explicit semantic lifecycles.

Источник контрактов: `Documentation/EngineFramework/Traversal.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `TraversalMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`): `AbstractCapable`, `RequiresMaterialized`, `RequiresRuntimeProjection`.
- `TraversalRouteLifetime` (`EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`): `Persistent`, `Session`, `Transient`.
- `TraversalSessionState` (`EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`): `Active`, `Suspended`, `Completed`, `Cancelled`, `Failed`.
- `TraversalChangeKind` (`EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`): `ProfileAssigned`, `ModeChanged`, `RequestRejected`, `SessionStarted`, `SessionCompleted`, `SessionCancelled`, `CarrierBoarded`, `CarrierDisembarked`, `CapabilityGranted`, `CapabilityRevoked`, `SessionSuspended`, `SessionResumed`, `SessionFailed`, `RouteRegistered` и другие значения.
- `TraversalResultKind` (`EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h`): `Accepted`, `Rejected`, `NoOp`.

### Операции и хранилища состояния

#### `TraversalService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h:370`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
TraversalService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterMode(TraversalModeDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterProfile(TraversalProfile profile);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const TraversalModeDefinition *FindMode(TraversalModeId id) const noexcept;
[[nodiscard]] const TraversalProfile *FindProfile(TraversalProfileId id) const noexcept;
void SetMaterializationProvider(const ITraversalMaterializationProvider *provider) noexcept;
[[nodiscard]] bool HasCapability(GameplayObjectRef subject, TraversalCapabilityId capability, Fixed min_parameter_micro = 0) const noexcept;
[[nodiscard]] Fixed EffectiveCapabilityParameter(GameplayObjectRef subject, TraversalCapabilityId capability) const noexcept;
[[nodiscard]] foundation::Result<TraversalResult> ChangeMode(ChangeTraversalModeRequest request);
[[nodiscard]] TraversalResult CanUseMode(GameplayObjectRef subject, TraversalModeId mode) const;
[[nodiscard]] foundation::Result<TraversalRouteId> RegisterRoute(TraversalRoute route);
[[nodiscard]] const TraversalRoute *FindRoute(TraversalRouteId id) const noexcept;
[[nodiscard]] const TraversalState *FindState(GameplayObjectRef subject) const noexcept;
[[nodiscard]] const TraversalSession *FindSession(TraversalSessionId id) const noexcept;
[[nodiscard]] std::vector<TraversalState> FindSubjectsUsingMode(TraversalModeId mode) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<TraversalModeId, TraversalModeDefinition, IdHash> modes_`; `std::unordered_map<TraversalProfileId, TraversalProfile, IdHash> profiles_`; `std::unordered_map<GameplayObjectRef, TraversalState, RefHash> states_`; `std::unordered_map<TraversalSessionId, TraversalSession, IdHash> sessions_`; `std::unordered_map<TraversalRouteId, TraversalRoute, IdHash> routes_`; `std::unordered_map<TraversalCapabilityGrantId, TraversalCapabilityGrant, IdHash> capability_grants_`; `std::unordered_map<GameplayObjectRef, TraversalCarrierBinding, RefHash> carrier_by_passenger_`; `MonotonicIdGenerator<GameplayObjectId> session_ids_`.

#### `ITraversalMaterializationProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h:242`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ITraversalMaterializationProvider() = default;
[[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef subject) const noexcept = 0;
[[nodiscard]] virtual bool HasRuntimeProjection(GameplayObjectRef subject) const noexcept = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h:129`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `TraversalSessionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr TraversalSessionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `TraversalCapabilityValue`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Traversal/include/Epidemic/GameFramework/Traversal/traversal.h:210`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
constexpr TraversalCapabilityValue() noexcept = default;
constexpr TraversalCapabilityValue(TraversalCapabilityId capability, Fixed parameter = 1'000'000) noexcept : id(capability), parameter_micro(parameter);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed parameter_micro = 1'000'000`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Traversal/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp` (1109 строк): `GameplayObjectId::FromString`, `Error::Create`, `TraversalService::TraversalService`, `TraversalService::RegisterMode`, `TraversalModeId::FromString`, `TraversalService::RegisterProfile`, `TraversalProfileId::FromString`, `TraversalService::FindMode`, `TraversalService::FindProfile`, `TraversalService::GrantCapability`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.traversal.session`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:18`
- `framework.traversal.route`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:23`
- `framework.traversal.capability_grant`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:28`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:153`
- `gameplay.traversal.invalid_mode`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:145`
- `gameplay.traversal.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:201`
- `gameplay.traversal.unknown_mode`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:167`
- `gameplay.traversal.capability_invalid`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:223`
- `gameplay.traversal.capability_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:232`
- `gameplay.already_registered`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:500`
- `gameplay.traversal.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:774`
- `gameplay.traversal.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:1057`
- `gameplay.traversal.invalid_profile_assignment`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:346`
- `gameplay.traversal.session_active`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:603`
- `gameplay.traversal.carrier_binding_active`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:393`
- `gameplay.traversal.route_in_use`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:534`
- `traversal.no_state`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:433`
- `traversal.unknown_mode`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:437`
- `traversal.mode_not_allowed`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:439`
- `traversal.capability_missing`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:442`
- `traversal.materialization_required`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:445`
- `traversal.runtime_projection_required`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:448`
- `gameplay.traversal.state_missing`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:763`
- `gameplay.traversal.invalid_route`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:488`
- `gameplay.traversal.route_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:497`
- `gameplay.traversal.mode_rejected`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:731`
- `gameplay.traversal.route_mismatch`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:610`
- `gameplay.traversal.session_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:617`
- `gameplay.traversal.session_state`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:682`
- `gameplay.traversal.invalid_carrier_binding`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:727`
- `gameplay.traversal.binding_missing`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:759`
- `gameplay.traversal.profile_missing`: `EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:766`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/traversal_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.traversal.session` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:18`)

```cpp
16 [[nodiscard]] constexpr std::uint64_t TraversalSessionIdScope() noexcept
   17 {
   18     return GameplayObjectId::FromString("framework.traversal.session").High();
   19 }
   20
```

#### `framework.traversal.route` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:23`)

```cpp
21 [[nodiscard]] constexpr std::uint64_t TraversalRouteIdScope() noexcept
   22 {
   23     return GameplayObjectId::FromString("framework.traversal.route").High();
   24 }
   25
```

#### `framework.traversal.capability_grant` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:28`)

```cpp
26 [[nodiscard]] constexpr std::uint64_t TraversalCapabilityGrantIdScope() noexcept
   27 {
   28     return GameplayObjectId::FromString("framework.traversal.capability_grant").High();
   29 }
   30
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:153`)

```cpp
151 {
  152     if (frozen_)
  153         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "traversal registry frozen"));
  154     if (profile.canonical_name.empty() || !profile.default_mode.IsValid())
  155         return foundation::Result<void>::Failure(
```

#### `gameplay.traversal.invalid_mode` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:145`)

```cpp
143         definition.acceleration_modifier_micro < 0 || definition.required_capability_parameter_micro < 0)
  144         return foundation::Result<void>::Failure(
  145             Error("gameplay.traversal.invalid_mode", "invalid or duplicate traversal mode"));
  146     modes_.emplace(definition.id, std::move(definition));
  147     return foundation::Result<void>::Success();
```

#### `gameplay.traversal.invalid_profile` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:201`)

```cpp
199         ProfileCapabilityParameter(profile, default_mode.required_capability) < default_mode.required_capability_parameter_micro)
  200         return foundation::Result<void>::Failure(
  201             Error("gameplay.traversal.invalid_profile", "default mode must be supported by an intrinsic capability"));
  202     profiles_.emplace(profile.id, std::move(profile));
  203     return foundation::Result<void>::Success();
```

#### `gameplay.traversal.unknown_mode` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:167`)

```cpp
165     for (const auto mode : profile.allowed_modes)
  166         if (!modes_.contains(mode))
  167             return foundation::Result<void>::Failure(Error("gameplay.traversal.unknown_mode", "allowed mode is not registered"));
  168     std::sort(profile.allowed_modes.begin(), profile.allowed_modes.end());
  169     profile.allowed_modes.erase(std::unique(profile.allowed_modes.begin(), profile.allowed_modes.end()), profile.allowed_modes.end());
```

#### `gameplay.traversal.capability_invalid` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:223`)

```cpp
221     if (!grant.subject.IsValid() || !grant.capability.IsValid() || grant.parameter_micro < 0 || !states_.contains(grant.subject))
  222         return foundation::Result<TraversalCapabilityGrantId>::Failure(
  223             Error("gameplay.traversal.capability_invalid", "capability grant is invalid"));
  224 
  225     auto staged_ids = capability_grant_ids_;
```

#### `gameplay.traversal.capability_id_exhausted` (`EngineFramework/GameplayWorldStateOwners/Traversal/src/traversal.cpp:232`)

```cpp
230     if (!grant.id.IsValid())
  231         return foundation::Result<TraversalCapabilityGrantId>::Failure(
  232             Error("gameplay.traversal.capability_id_exhausted", "capability grant id is invalid or exhausted"));
  233     if (capability_grants_.contains(grant.id))
  234         return foundation::Result<TraversalCapabilityGrantId>::Failure(
```
