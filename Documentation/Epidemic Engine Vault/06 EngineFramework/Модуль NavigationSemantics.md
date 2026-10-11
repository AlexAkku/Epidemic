---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "navigation_semantics"]
source_path: "EngineFramework/GameplayWorldStateOwners/NavigationSemantics"
---

# EngineFramework / NavigationSemantics

Модуль: **NavigationSemantics**. Игровые правила навигации.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/`.
**CMake targets:** `EpidemicGameFrameworkNavigationSemantics`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/NavigationSemantics/navigation_semantics.h|navigation_semantics.h]]: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h`; объявленные типы: `NavigationDomainId`, `NavigationRuleId`, `NavigationLayerId`, `NavigationLinkId`, `NavigationLayerTypeId`, `NavigationLinkTypeId`, `TraversalModeSemanticId`, `NavigationReasonId`, `IdHash`, `RefHash`, `NavigationDecisionKind`, `NavigationAvailability`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/NavigationSemantics|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns semantic navigation profiles, layers and links/rules, their indexes, layer identity generator, revisions and the bounded navigation-semantics change journal.

The service remains the single authoritative owner of that state. Derived indexes, journals and diagnostic/read models are not independent semantic owners.

#### Authoritative state and indexes

Admission/generated B05 inventory: 69 public callables, 60 mutation obligations, 1 lifecycle candidates, 14 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Multi-container mutations must complete fallible staging before authoritative publication. Deterministic query/order semantics must agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B05 coverage projection. Revision, identity-generator and journal publication is part of the mutation contract. Allocation/publication failure may not expose partially advanced primary state, index state, generator, revision or journal.

B05 allocation evidence uses source-private module seams rather than the shared process-global allocator override. The seams are implementation/test details and do not expand the public API.

#### Lifecycle and identity

All emitted lifecycle and stale-identity candidates were reviewed. Invalid, removed, duplicate and stale identities are rejected or handled as the documented no-op without false publication. Retry does not silently resurrect terminal state.

#### Persistence

Snapshot/restore is reviewed as candidate-state publication: validate first, build off-state, preserve the current service on failure, then atomically publish the candidate. Successful restore preserves generator/revision/cursor continuity required by the module snapshot contract.

#### External boundaries

Capability lookup/evaluation is a semantic provider boundary. Failure is not authoritative navigation state and may not leave a partial layer/link/index publication.

#### Threading contract

No additional internal synchronization guarantee is introduced by B05. These gameplay owners are treated as externally serialized/owner-thread services for Goal 4 local correctness. Engine-wide concurrency qualification remains Goal 6.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkNavigationSemanticsTests` from `EngineFramework/DevelopmentInfrastructure/Tests/navigation_semantics_tests.cpp`.

Goal 4 migrates restore failure evidence to narrow source-private fault points for profiles, layers, links, journal and indexes. Existing deterministic rule, invalid-reference, revision/cursor and snapshot regressions remain green.

Источник контрактов: `Documentation/EngineFramework/NavigationSemantics.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `NavigationDecisionKind` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h`): `Allow`, `Deny`, `AddCost`, `RequireCapability`, `RequireFact`, `Avoid`, `Prefer`.
- `NavigationAvailability` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h`): `PhysicallyImpossible`, `TraversalUnsupported`, `Forbidden`, `Unsafe`, `TemporarilyBlocked`, `Available`.
- `NavigationLayerLifetime` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h`): `Transient`, `Session`, `Persistent`, `Timed`.
- `LinkState` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h`): `Open`, `Closed`, `Locked`, `Blocked`, `Destroyed`, `Disabled`, `Conditional`.
- `NavigationChangeKind` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h`): `ProfileChanged`, `LayerAdded`, `LayerRemoved`, `LayerChanged`, `LinkChanged`, `RuleRegistered`.

### Операции и хранилища состояния

#### `NavigationSemanticsService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h:336`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
NavigationSemanticsService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterDomain(NavigationDomainDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterRule(NavigationRuleDefinition rule);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
void SetCapabilityProvider(const INavigationCapabilityProvider *provider) noexcept;
void SetFactProvider(const INavigationFactProvider *provider) noexcept;
[[nodiscard]] NavigationPermissionResult CanEnterArea(GameplayObjectRef subject, GameplayObjectRef area, TraversalModeSemanticId traversal_mode, const GameplayContext &context) const;
[[nodiscard]] NavigationPermissionResult CanUseLink(GameplayObjectRef subject, NavigationLinkId link, TraversalModeSemanticId traversal_mode, const GameplayContext &context) const;
[[nodiscard]] NavigationPermissionResult EvaluatePath(NavigationPermissionQuery query) const;
[[nodiscard]] std::vector<NavigationSemanticLayer> FindLayersInArea(GameplayObjectRef area) const;
[[nodiscard]] std::vector<NavigationSemanticLink> FindLinksBetween(GameplayObjectRef from, GameplayObjectRef to) const;
[[nodiscard]] const NavigationSemanticProfile *FindProfile(GameplayObjectRef subject) const noexcept;
[[nodiscard]] const NavigationSemanticLink *FindLink(NavigationLinkId id) const noexcept;
[[nodiscard]] const NavigationSemanticLayer *FindLayer(NavigationLayerId id) const noexcept;
[[nodiscard]] NavigationSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(NavigationSnapshot snapshot);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<NavigationDomainId, NavigationDomainDefinition, IdHash> domains_`; `std::vector<NavigationRuleDefinition> rules_`; `std::unordered_map<GameplayObjectRef, NavigationSemanticProfile, RefHash> profiles_`; `std::unordered_map<NavigationLayerId, NavigationSemanticLayer, IdHash> layers_`; `std::unordered_map<NavigationLinkId, NavigationSemanticLink, IdHash> links_`; `std::unordered_map<GameplayObjectRef, std::vector<NavigationLayerId>, RefHash> layer_ids_by_area_`; `std::unordered_map<LinkPairKey, std::vector<NavigationLinkId>, LinkPairHash> link_ids_by_pair_`; `MonotonicIdGenerator<GameplayObjectId> layer_ids_`.

#### `INavigationCapabilityProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h:270`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavigationCapabilityProvider() = default;
[[nodiscard]] virtual bool HasCapability(GameplayObjectRef subject, TypeId capability, Fixed min_parameter_micro) const noexcept = 0;
```

#### `INavigationFactProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h:276`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INavigationFactProvider() = default;
[[nodiscard]] virtual bool HasFact(GameplayObjectRef subject, TypeId fact, const GameplayContext &context) const noexcept = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h:135`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `NavigationLayerId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/include/Epidemic/GameFramework/NavigationSemantics/navigation_semantics.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NavigationLayerId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp` (794 строк): `Error::Create`, `NavigationSemanticsService::NavigationSemanticsService`, `TypeId::FromString`, `NavigationSemanticsService::PrepareRevision`, `NavigationSemanticsService::CanAdvanceRevisionBy`, `NavigationSemanticsService::RegisterDomain`, `NavigationDomainId::FromString`, `NavigationSemanticsService::RegisterRule`, `NavigationSemanticsService::SetProfile`, `NavigationSemanticsService::ValidateAndAdvanceLayerId`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.navigation.layer`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:705`
- `gameplay.navigation.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:133`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:162`
- `gameplay.navigation.invalid_domain`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:154`
- `gameplay.navigation.invalid_rule`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:169`
- `gameplay.navigation.duplicate_rule`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:171`
- `gameplay.navigation.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:184`
- `gameplay.navigation.invalid_layer`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:289`
- `gameplay.navigation.layer_missing`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:301`
- `gameplay.navigation.layer_stale`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:288`
- `gameplay.navigation.invalid_link`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:351`
- `gameplay.navigation.link_missing`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:350`
- `navigation.link_missing`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:432`
- `navigation.path_invalid`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:410`
- `navigation.link_forbidden`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:461`
- `navigation.link_blocked`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:468`
- `navigation.link_destroyed`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:473`
- `navigation.no_profile`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:502`
- `navigation.layer_rule`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:545`
- `navigation.rule`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:576`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:684`
- `gameplay.navigation.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:740`
- `gameplay.navigation.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:734`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.navigation.layer` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:705`)

```cpp
703         return foundation::Result<void>::Failure(Error("gameplay.navigation.allocation_failed", "layer restore staging failed"));
  704     std::uint64_t max_own_layer_low = 0;
  705     const auto expected_scope = TypeId::FromString("framework.navigation.layer").Raw();
  706     for (const auto& layer : snapshot.layers)
  707     {
```

#### `gameplay.navigation.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:133`)

```cpp
131 {
  132     const auto next=CheckedNext(revision_);
  133     if(!next)return foundation::Result<Revision>::Failure(Error("gameplay.navigation.revision_exhausted","navigation revision is exhausted"));
  134     return foundation::Result<Revision>::Success(*next);
  135 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:162`)

```cpp
160 {
  161     if (frozen_)
  162         return foundation::Result<void>::Failure(Error("gameplay.registry_frozen", "navigation registry frozen"));
  163     if (!rule.id.IsValid() || !domains_.contains(rule.domain) || !EnumInRange(rule.decision, NavigationDecisionKind::Prefer) || rule.multiplier_micro < 0 ||
  164         rule.required_parameter_micro < 0)
```

#### `gameplay.navigation.invalid_domain` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:154`)

```cpp
152     if (definition.id != expected || domains_.contains(definition.id))
  153         return foundation::Result<void>::Failure(
  154             Error("gameplay.navigation.invalid_domain", "invalid or duplicate navigation domain"));
  155     domains_.emplace(definition.id, std::move(definition));
  156     return foundation::Result<void>::Success();
```

#### `gameplay.navigation.invalid_rule` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:169`)

```cpp
167         !rule.requirement.IsValid())
  168         return foundation::Result<void>::Failure(
  169             Error("gameplay.navigation.invalid_rule", "requirement rule must declare a requirement id"));
  170     if (std::any_of(rules_.begin(), rules_.end(), [&](const auto& current) { return current.id == rule.id; }))
  171         return foundation::Result<void>::Failure(Error("gameplay.navigation.duplicate_rule", "duplicate navigation rule"));
```

#### `gameplay.navigation.duplicate_rule` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:171`)

```cpp
169             Error("gameplay.navigation.invalid_rule", "requirement rule must declare a requirement id"));
  170     if (std::any_of(rules_.begin(), rules_.end(), [&](const auto& current) { return current.id == rule.id; }))
  171         return foundation::Result<void>::Failure(Error("gameplay.navigation.duplicate_rule", "duplicate navigation rule"));
  172     rules_.push_back(std::move(rule));
  173     std::sort(rules_.begin(), rules_.end(), [](const auto& a, const auto& b) {
```

#### `gameplay.navigation.invalid_profile` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:184`)

```cpp
182 {
  183     if (!profile.subject.IsValid() || !domains_.contains(profile.domain))
  184         return foundation::Result<void>::Failure(Error("gameplay.navigation.invalid_profile", "invalid navigation profile"));
  185     auto next=PrepareRevision();if(!next)return foundation::Result<void>::Failure(next.GetError());
  186     profile.revision=next.Value();
```

#### `gameplay.navigation.invalid_layer` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:289`)

```cpp
287     const auto it=layers_.find(id);if(it==layers_.end())return foundation::Result<void>::Failure(Error("gameplay.navigation.layer_missing","navigation layer missing"));
  288     if(it->second.revision!=expected_revision)return foundation::Result<void>::Failure(Error("gameplay.navigation.layer_stale","navigation layer revision changed"));
  289     replacement.id=it->second.id;replacement.source=it->second.source;replacement.lifetime=it->second.lifetime;if(!IsValidLayer(replacement))return foundation::Result<void>::Failure(Error("gameplay.n…
```

#### `gameplay.navigation.layer_missing` (`EngineFramework/GameplayWorldStateOwners/NavigationSemantics/src/navigation_semantics.cpp:301`)

```cpp
299 foundation::Result<void> NavigationSemanticsService::RemoveLayer(NavigationLayerId id, GameplayContext context)
  300 {
  301     const auto it=layers_.find(id);if(it==layers_.end())return foundation::Result<void>::Failure(Error("gameplay.navigation.layer_missing","navigation layer missing"));
  302     auto next=PrepareRevision();if(!next)return foundation::Result<void>::Failure(next.GetError());
  303     UnindexLayer(it->second);layers_.erase(it);revision_=next.Value();if(dynamic_updates_!=std::numeric_limits<std::uint64_t>::max())++dyna…
```
