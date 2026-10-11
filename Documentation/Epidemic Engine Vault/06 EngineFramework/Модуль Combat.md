---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "combat"]
source_path: "EngineFramework/GameplayWorldStateOwners/Combat"
---

# EngineFramework / Combat

Модуль: **Combat**. Боевые операции и damage.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Combat/`.
**CMake targets:** `EpidemicGameFrameworkCombat`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Combat/combat.h|combat.h]]: `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`; объявленные типы: `DamageTypeId`, `DamageProfileId`, `CombatResourceTypeId`, `CombatModifierTypeId`, `CombatResolutionId`, `CombatResourceReservationId`, `IdHash`, `CombatResourceDepletionEffect`, `CombatResourceDefinition`, `DamageProfile`, `CombatLifeState`, `CombatEngagementState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Combat|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns semantic combatants, resources, life-state transitions, damage resolution plans, resource reservations and combat change journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`. B06 does not change this public header or its signatures.

#### Dependency list

Direct production links are GameFramework Foundation, SupportRandom and EngineBase Foundation. No peer state-owner dependency is added.

#### External ports/callbacks/providers/backends

Combat modifier/random inputs are borrowed ports. The service does not retain external authoritative combat state.

#### Authoritative state

Combatant records and resource states, reservation records, ID generators, per-record revisions, resolution state and bounded change journal.

#### Derived/cache/index state

Damage planning and modifier ordering are derived from definitions, combatant state and deterministic random input; diagnostics are non-authoritative.

#### ID spaces, generations, revisions and cursors

Resolution/reservation IDs use service-owned monotonic generators. Record revision and journal cursor exhaustion are checked before state-changing publication.

#### State machines

Combatant life transitions and reservation/resolve lifecycles are explicit. Failed resolution does not consume reservation or mutate resources.

Источник контрактов: `Documentation/EngineFramework/Combat.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `CombatResourceDepletionEffect` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `None`, `Downed`, `Dead`.
- `CombatLifeState` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `Alive`, `Downed`, `Dead`, `Disabled`.
- `CombatEngagementState` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `OutOfCombat`, `Engaged`, `Disengaging`.
- `CombatModifierPhase` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `Base`, `Attacker`, `Target`, `Mitigation`, `Amplification`, `Final`.
- `CombatModifierOperation` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `Add`, `Multiply`, `Override`, `ClampMin`, `ClampMax`.
- `CombatOutcome` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `None`, `Missed`, `Evaded`, `Blocked`, `Critical`, `Killed`, `Downed`.
- `CombatChangeKind` (`EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h`): `CombatantRegistered`, `CombatantRemoved`, `EngagementChanged`, `DamageResolved`, `ResourceChanged`, `LifeStateChanged`.

### Операции и хранилища состояния

#### `CombatService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h:249`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
CombatService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<CombatResourceTypeId> RegisterResource(CombatResourceDefinition definition);
[[nodiscard]] foundation::Result<DamageTypeId> RegisterDamageType(std::string canonical_name);
[[nodiscard]] foundation::Result<DamageProfileId> RegisterDamageProfile(DamageProfile profile);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
void SetModifierProvider(const ICombatModifierProvider* provider) noexcept;
[[nodiscard]] const CombatantRecord* FindCombatant(GameplayObjectRef subject) const noexcept;
[[nodiscard]] foundation::Result<CombatResourceState> GetResource(GameplayObjectRef subject, CombatResourceTypeId type) const;
void CommitResourceReservation(CombatResourceReservationId reservation) noexcept;
[[nodiscard]] const CombatResourceReservation* FindResourceReservation(CombatResourceReservationId reservation) const noexcept;
[[nodiscard]] foundation::Result<void> SetPreparePolicy(CombatPreparePolicy policy);
[[nodiscard]] CombatPreparePolicy GetPreparePolicy() const noexcept;
[[nodiscard]] foundation::Result<CombatPlan> PrepareDamage(DamageRequest request);
[[nodiscard]] foundation::Result<CombatResult> CommitDamage(const CombatPlan& plan, GameplayTimePoint now);
[[nodiscard]] foundation::Result<void> CancelDamagePlan(CombatResolutionId id);
[[nodiscard]] std::size_t ExpirePreparedPlans(GameplayTimePoint now) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<CombatResourceTypeId, CombatResourceDefinition, IdHash> resources_`; `std::unordered_map<DamageTypeId, std::string, IdHash> damage_types_`; `std::unordered_map<DamageProfileId, DamageProfile, IdHash> damage_profiles_`; `std::unordered_map<GameplayObjectRef, CombatantRecord> combatants_`; `const ICombatModifierProvider* modifier_provider_ = nullptr`; `std::uint64_t modifier_provider_epoch_ = 1`; `MonotonicIdGenerator<GameplayObjectId> resolution_ids_`; `MonotonicIdGenerator<GameplayObjectId> resource_reservation_ids_`.

#### `ICombatModifierProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h:148`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ICombatModifierProvider() = default;
[[nodiscard]] virtual std::vector<CombatModifier> Collect(const DamageRequest& request) const = 0;
[[nodiscard]] virtual Revision RevisionFor(const DamageRequest& request) const noexcept = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h:56`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <typename T> [[nodiscard]] std::size_t operator()(const T& id) const noexcept;
```

#### `CombatResourceDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h:69`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string canonical_name`; `std::int64_t minimum_micro = 0`; `std::int64_t default_maximum_micro = 100'000'000`; `CombatResourceDepletionEffect depletion_effect = CombatResourceDepletionEffect::None`.

#### `CombatResourceState`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h:99`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t current_micro = 0`; `std::int64_t maximum_micro = 0`.

#### `CombatResourceReservation`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Combat/include/Epidemic/GameFramework/Combat/combat.h:106`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t amount_micro = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Combat/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp` (967 строк): `Error::Create`, `CombatService::CombatService`, `GameplayObjectId::FromString`, `CombatService::SetModifierProvider`, `CombatService::RegisterResource`, `CombatResourceTypeId::FromString`, `CombatService::RegisterDamageType`, `DamageTypeId::FromString`, `CombatService::RegisterDamageProfile`, `DamageProfileId::FromString`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.combat.resolutions`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:141`
- `framework.combat.resource_reservations`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:142`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:590`
- `gameplay.combat.resource_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:167`
- `gameplay.combat.damage_type_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:179`
- `gameplay.combat.profile_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:199`
- `gameplay.combat.combatant_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:300`
- `gameplay.combat.resource_unknown`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:307`
- `gameplay.combat.combatant_missing`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:738`
- `gameplay.combat.resource_missing`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:755`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:759`
- `gameplay.combat.resource_reservation_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:422`
- `gameplay.combat.resource_unavailable`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:444`
- `gameplay.combat.resource_reservation_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:455`
- `gameplay.combat.resource_reservation_conflict`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:463`
- `gameplay.combat.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:710`
- `gameplay.combat.life_transition_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:573`
- `gameplay.combat.prepare_policy_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:593`
- `gameplay.combat.too_many_prepared`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:635`
- `gameplay.combat.damage_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:640`
- `gameplay.combat.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:650`
- `combat.hit`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:660`
- `combat.evade`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:661`
- `combat.block`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:662`
- `combat.critical`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:663`
- `gameplay.combat.modifier_provider_exception`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:691`
- `gameplay.combat.plan_conflict`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:705`
- `gameplay.combat.plan_unknown`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:811`
- `gameplay.combat.plan_expired`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:731`
- `gameplay.stale_revision`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:747`
- `gameplay.combat.plan_invalid`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:807`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:866`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/combat_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.combat.resolutions` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:141`)

```cpp
139 } // namespace
  140 CombatService::CombatService()
  141     : resolution_ids_(GameplayObjectId::FromString("framework.combat.resolutions").High()),
  142       resource_reservation_ids_(GameplayObjectId::FromString("framework.combat.resource_reservations").High())
  143 {
```

#### `framework.combat.resource_reservations` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:142`)

```cpp
140 CombatService::CombatService()
  141     : resolution_ids_(GameplayObjectId::FromString("framework.combat.resolutions").High()),
  142       resource_reservation_ids_(GameplayObjectId::FromString("framework.combat.resource_reservations").High())
  143 {
  144 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:590`)

```cpp
588 {
  589     if (frozen_)
  590         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "combat registry frozen"));
  591     if (policy.lifetime.ticks <= 0)
  592         return foundation::Result<void>::Failure(
```

#### `gameplay.combat.resource_invalid` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:167`)

```cpp
165     if (d.id != e || d.minimum_micro > d.default_maximum_micro || resources_.contains(d.id))
  166         return foundation::Result<CombatResourceTypeId>::Failure(
  167             Error("gameplay.combat.resource_invalid", "invalid or duplicate resource"));
  168     const auto id = d.id;
  169     resources_.emplace(id, std::move(d));
```

#### `gameplay.combat.damage_type_invalid` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:179`)

```cpp
177     if (name.empty() || damage_types_.contains(id))
  178         return foundation::Result<DamageTypeId>::Failure(
  179             Error("gameplay.combat.damage_type_invalid", "invalid or duplicate damage type"));
  180     damage_types_[id] = std::move(name);
  181     return foundation::Result<DamageTypeId>::Success(id);
```

#### `gameplay.combat.profile_invalid` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:199`)

```cpp
197         p.critical_multiplier_micro < 0 || p.flat_mitigation_micro < 0)
  198         return foundation::Result<DamageProfileId>::Failure(
  199             Error("gameplay.combat.profile_invalid", "invalid or duplicate damage profile"));
  200     const auto id = p.id;
  201     damage_profiles_.emplace(id, std::move(p));
```

#### `gameplay.combat.combatant_invalid` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:300`)

```cpp
298     if (!subject.IsValid() || combatants_.contains(subject))
  299         return foundation::Result<void>::Failure(
  300             Error("gameplay.combat.combatant_invalid", "invalid or duplicate combatant"));
  301     std::unordered_set<CombatResourceTypeId, IdHash> seen;
  302     for (auto &state : states)
```

#### `gameplay.combat.resource_unknown` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:307`)

```cpp
305         if (def == resources_.end() || !seen.insert(state.type).second)
  306             return foundation::Result<void>::Failure(
  307                 Error("gameplay.combat.resource_unknown", "unknown or duplicate combat resource"));
  308         if (state.maximum_micro == 0)
  309             state.maximum_micro = def->second.default_maximum_micro;
```

#### `gameplay.combat.combatant_missing` (`EngineFramework/GameplayWorldStateOwners/Combat/src/combat.cpp:738`)

```cpp
736         prepared_plans_.erase(prepared);
  737         return foundation::Result<CombatResult>::Failure(
  738             Error("gameplay.combat.combatant_missing", "target combatant missing"));
  739     }
  740     const bool provider_stale = plan.expected_provider_epoch != modifier_provider_epoch_ ||
```
