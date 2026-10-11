---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "abilities"]
source_path: "EngineFramework/GameplayWorldStateOwners/Abilities"
---

# EngineFramework / Abilities

Модуль: **Abilities**. Способности и activation.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Abilities/`.
**CMake targets:** `EpidemicGameFrameworkAbilities`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Abilities/abilities.h|abilities.h]]: `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`; объявленные типы: `AbilityDefinitionId`, `AbilityInstanceId`, `AbilityExecutionId`, `CooldownGroupId`, `AbilityResourceTypeId`, `AbilityReservationId`, `IdHash`, `RegisteredAbilityPayload`, `AbilityTargetPolicy`, `AbilityTimingKind`, `AbilityCostPolicy`, `AbilityGrantPersistence`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Abilities|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns ability definitions/instances, activation executions, cooldowns, schedule bindings, resource reservation lifecycle, outputs and ability change journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`. Public headers/signatures are unchanged by B06.

#### Dependency list

Direct production links are GameFramework Foundation and EngineBase Foundation only.

#### External ports/callbacks/providers/backends

Requirement/materialization/resource providers are borrowed ports. Resource reserve/commit/release is coordinated locally without taking provider ownership.

#### Authoritative state

Definitions, instances, executions, schedule index, cooldowns, ID generators, reconciliation flags and bounded change journal.

#### Derived/cache/index state

Schedule-to-execution mapping is a derived consistency index over active execution schedule identity. Diagnostics are non-authoritative.

#### ID spaces, generations, revisions and cursors

Instance/execution IDs use monotonic generators. Execution revision is checked before terminal and binding updates. Journal sequence is preflighted for mutations that publish changes.

#### State machines

Activation covers casting/executing/channeling and terminal completed/cancelled/interrupted/failed states. Resource reservations have reserve, commit and release paths; restored reservations require explicit reconciliation.

Источник контрактов: `Documentation/EngineFramework/Abilities.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AbilityTargetPolicy` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `Self`, `SingleTarget`, `MultipleTargets`, `Point`, `Area`, `Direction`, `None`.
- `AbilityTimingKind` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `Instant`, `CastTime`, `Channel`, `DelayedExecution`.
- `AbilityCostPolicy` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `PayOnStart`, `ReserveThenCommit`, `PayOnExecute`, `PayPerChannelTick`.
- `AbilityGrantPersistence` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `Permanent`, `Temporary`, `SourceBound`.
- `AbilityContinuityPolicy` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `ActivationOnly`, `UntilExecute`, `ThroughoutExecution`.
- `AbilityExecutionState` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `Casting`, `Channeling`, `Executing`, `Completed`, `Cancelled`, `Interrupted`, `Failed`.
- `AbilityAvailability` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `Available`, `Unavailable`.
- `AbilityChangeKind` (`EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h`): `Granted`, `Revoked`, `Started`, `Executed`, `Cancelled`, `Interrupted`, `Failed`, `CooldownStarted`, `CooldownFinished`, `EnabledChanged`.

### Операции и хранилища состояния

#### `AbilityService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h:379`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
AbilityService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<AbilityDefinitionId> RegisterDefinition(AbilityDefinition definition);
void Freeze() noexcept;
[[nodiscard]] const AbilityDefinition *FindDefinition(AbilityDefinitionId id) const noexcept;
void SetRequirementProvider(const IAbilityRequirementProvider *provider) noexcept;
void SetResourceProvider(IAbilityResourceProvider *provider) noexcept;
void SetMaterializationProvider(const IAbilityMaterializationProvider *provider) noexcept;
[[nodiscard]] const AbilityInstance *FindInstance(AbilityInstanceId id) const noexcept;
[[nodiscard]] std::vector<AbilityInstance> GetAbilities(GameplayObjectRef owner) const;
[[nodiscard]] AbilityAvailabilityResult CanActivate(AbilityInstanceId ability, const AbilityTargetSet &targets, GameplayTimePoint now) const;
[[nodiscard]] foundation::Result<AbilityExecutionId> BeginActivation(AbilityActivationRequest request);
[[nodiscard]] foundation::Result<std::vector<AbilityOutput>> CompleteExecution(AbilityExecutionId execution, GameplayTimePoint now);
[[nodiscard]] foundation::Result<std::vector<AbilityOutput>> ChannelTick(AbilityExecutionId execution, GameplayTimePoint now, std::uint64_t occurrences = 1);
[[nodiscard]] foundation::Result<void> BindSchedule(AbilityExecutionId execution, ScheduleId schedule);
[[nodiscard]] foundation::Result<void> NotifyScheduleDue(ScheduleId schedule, GameplayTimePoint now, std::vector<AbilityOutput> &outputs);
[[nodiscard]] GameplayDuration CooldownRemaining(GameplayObjectRef owner, CooldownGroupId group, GameplayTimePoint now) const noexcept;
[[nodiscard]] bool NeedsResourceReconciliation() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<AbilityDefinitionId, AbilityDefinition, IdHash> definitions_`; `std::unordered_map<AbilityInstanceId, AbilityInstance, IdHash> instances_`; `std::unordered_map<AbilityExecutionId, AbilityExecution, IdHash> executions_`; `std::vector<AbilityCooldownState> cooldowns_`; `MonotonicIdGenerator<GameplayObjectId> instance_ids_`; `MonotonicIdGenerator<GameplayObjectId> execution_ids_`; `std::unordered_map<ScheduleId, AbilityExecutionId> schedule_to_execution_`; `bool frozen_ = false`.

#### `IAbilityResourceProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h:284`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAbilityResourceProvider() = default;
[[nodiscard]] virtual bool CanAfford(GameplayObjectRef owner, AbilityResourceTypeId type, std::int64_t amount_micro) const = 0;
[[nodiscard]] virtual foundation::Result<AbilityResourceReservation> Reserve(GameplayObjectRef owner, AbilityResourceTypeId type, std::int64_t amount_micro, GameplayContext context) = 0;
virtual void Commit(const AbilityResourceReservation &reservation, GameplayContext context) noexcept = 0;
virtual void Release(const AbilityResourceReservation &reservation, GameplayContext context) noexcept = 0;
[[nodiscard]] virtual foundation::Result<void> ReconcileReservation(const AbilityResourceReservation &reservation, GameplayObjectRef owner, GameplayContext context);
```

#### `IAbilityRequirementProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h:269`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAbilityRequirementProvider() = default;
[[nodiscard]] virtual AbilityAvailabilityResult Check(const AbilityDefinition &definition, const AbilityInstance &instance, const AbilityTargetSet &targets, GameplayTimePoint now) const = 0;
```

#### `IAbilityMaterializationProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h:278`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAbilityMaterializationProvider() = default;
[[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef subject) const noexcept = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h:87`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `RegisteredAbilityPayload`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Abilities/include/Epidemic/GameFramework/Abilities/abilities.h:98`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] static RegisteredAbilityPayload FromTrivial(TypeId t, const T &v);
template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId t) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> bytes`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Abilities/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp` (1482 строк): `Error::Create`, `AbilityService::AbilityService`, `GameplayObjectId::FromString`, `AbilityService::RegisterDefinition`, `AbilityDefinitionId::FromString`, `AbilityService::FindDefinition`, `AbilityService::CanRecord`, `AbilityService::Record`, `AbilityService::IsTerminal`, `AbilityService::Grant`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.abilities.instances`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1445`
- `framework.abilities.executions`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1446`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:78`
- `gameplay.ability.definition_invalid`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:90`
- `gameplay.ability.cost_invalid`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:94`
- `gameplay.ability.output_invalid`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:98`
- `gameplay.ability.grant_invalid`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:144`
- `gameplay.ability.journal_exhausted`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:628`
- `gameplay.ability.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:634`
- `gameplay.ability.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1474`
- `gameplay.ability.instance_missing`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:617`
- `gameplay.ability.active`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:186`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1194`
- `ability.target_invalid`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:277`
- `ability.target_self_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:284`
- `ability.target_single_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:288`
- `ability.targets_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:292`
- `ability.point_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:296`
- `ability.area_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:300`
- `ability.direction_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:304`
- `ability.target_not_allowed`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:308`
- `ability.owner_not_materialized`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:315`
- `ability.materialization_provider_missing`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:320`
- `ability.target_not_materialized`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:332`
- `ability.materialized_target_required`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:335`
- `ability.instance_unavailable`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:345`
- `ability.definition_missing`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:348`
- `ability.cooldown`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:353`
- `ability.resource_provider_missing`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:358`
- `ability.resource_cost_overflow`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:371`
- `ability.resource_missing`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:376`
- `ability.internal_allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:380`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/abilities_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.abilities.instances` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1445`)

```cpp
1443         return foundation::Result<void>::Failure(
 1444             Error("gameplay.ability.restore_invalid", "invalid next ability change sequence"));
 1445     const auto expected_instance_scope = GameplayObjectId::FromString("framework.abilities.instances").High();
 1446     const auto expected_execution_scope = GameplayObjectId::FromString("framework.abilities.executions").High();
 1447     if (!ValidGeneratorSnapshot(snapshot.instance_ids, expected_instance_scope, max_instance_low) ||
```

#### `framework.abilities.executions` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:1446`)

```cpp
1444             Error("gameplay.ability.restore_invalid", "invalid next ability change sequence"));
 1445     const auto expected_instance_scope = GameplayObjectId::FromString("framework.abilities.instances").High();
 1446     const auto expected_execution_scope = GameplayObjectId::FromString("framework.abilities.executions").High();
 1447     if (!ValidGeneratorSnapshot(snapshot.instance_ids, expected_instance_scope, max_instance_low) ||
 1448         !ValidGeneratorSnapshot(snapshot.execution_ids, expected_execution_scope, max_execution_low)…
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:78`)

```cpp
76     if (frozen_)
   77         return foundation::Result<AbilityDefinitionId>::Failure(
   78             Error("gameplay.registry_frozen", "ability registry frozen"));
   79     if (d.canonical_name.empty())
   80         return foundation::Result<AbilityDefinitionId>::Failure(
```

#### `gameplay.ability.definition_invalid` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:90`)

```cpp
88         (d.timing.kind == AbilityTimingKind::Channel && d.timing.channel_interval.ticks <= 0))
   89         return foundation::Result<AbilityDefinitionId>::Failure(
   90             Error("gameplay.ability.definition_invalid", "invalid or duplicate ability definition"));
   91     for (const auto &cost : d.costs)
   92         if (!cost.resource.IsValid() || cost.amount_micro < 0)
```

#### `gameplay.ability.cost_invalid` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:94`)

```cpp
92         if (!cost.resource.IsValid() || cost.amount_micro < 0)
   93             return foundation::Result<AbilityDefinitionId>::Failure(
   94                 Error("gameplay.ability.cost_invalid", "invalid ability cost"));
   95     for (const auto &output : d.outputs)
   96         if (!output.action.IsValid())
```

#### `gameplay.ability.output_invalid` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:98`)

```cpp
96         if (!output.action.IsValid())
   97             return foundation::Result<AbilityDefinitionId>::Failure(
   98                 Error("gameplay.ability.output_invalid", "ability output action required"));
   99     const auto id = d.id;
  100     definitions_.emplace(id, std::move(d));
```

#### `gameplay.ability.grant_invalid` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:144`)

```cpp
142     if (!owner.IsValid() || !definitions_.contains(def))
  143         return foundation::Result<AbilityInstanceId>::Failure(
  144             Error("gameplay.ability.grant_invalid", "invalid owner or definition"));
  145     if (!CanRecord())
  146         return foundation::Result<AbilityInstanceId>::Failure(
```

#### `gameplay.ability.journal_exhausted` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:628`)

```cpp
626     if (!CanRecord(starts_cooldown ? 2 : 1))
  627         return foundation::Result<AbilityExecutionId>::Failure(
  628             Error("gameplay.ability.journal_exhausted", "ability change sequence exhausted"));
  629 
  630     auto staged_ids = execution_ids_;
```

#### `gameplay.ability.id_exhausted` (`EngineFramework/GameplayWorldStateOwners/Abilities/src/abilities.cpp:634`)

```cpp
632     if (!generated.IsValid())
  633         return foundation::Result<AbilityExecutionId>::Failure(
  634             Error("gameplay.ability.id_exhausted", "ability execution id exhausted"));
  635 
  636     AbilityExecution execution;
```
