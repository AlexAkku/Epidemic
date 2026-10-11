---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "traversal_navigation_construction_integration"]
source_path: "EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration"
---

# EngineFramework / TraversalNavigationConstructionIntegration

Модуль: **TraversalNavigationConstructionIntegration**. Связь перемещения, навигации и строительства.

**Реализация:** `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/`.
**CMake targets:** `EpidemicGameFrameworkTraversalNavigationConstructionIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration`. Admission: 8 public API, 0 mutation obligations, 0 lifecycle, 0 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h|traversal_navigation_construction_adapters.h]]: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/include/Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h`; объявленные типы: `TraversalNavigationCapabilityProvider`, `TraversalNavigationAdapter`, `ConstructionNavigationOperation`, `ConstructionNavigationLayerPayload`, `ConstructionNavigationAdapter`, `ConstructionTraversalAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/TraversalNavigationConstructionIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Public contract review

- TraversalNavigationCapabilityProvider maps traversal capabilities into NavigationSemantics without owning either state.
- TraversalNavigationAdapter::CanUseLink propagates stale/invalid subject and capability decisions correctly.
- ConstructionNavigationLayerPayload encoding/decoding validates durable payloads and stable layer identity.
- ConstructionNavigationAdapter::QueueNavigationOperation creates durable placement outputs without duplicate semantic operations.
- ProcessPendingOutputs performs idempotent add/update/remove, acknowledges only after Navigation commit, and supports retry after partial external progress.
- Duplicate delivery is idempotent by stable layer ID; already-applied state is acknowledged rather than duplicated.
- ConstructionTraversalAdapter::CancelTraversalSessions handles the documented partial-progress/retry contract without claiming batch atomicity that the API does not provide.
- Corrupt payloads, stale world references and restore/replay paths leave both sides reconcilable.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

The durable placement-output protocol was reviewed specifically for stable layer IDs, idempotent add/update/remove and acknowledgement only after successful Navigation mutation.

#### Persistence and reconciliation

No independent adapter-owned snapshot/checkpoint surface is required by this freeze unit. Durable state is either carried by owner-issued tokens/outputs or explicitly delegated to the authoritative owners; whole-engine persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/traversal_navigation_construction_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkTraversalNavigationConstructionIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/TraversalNavigationConstructionIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ConstructionNavigationOperation` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/include/Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h`): `Add`, `Update`, `Remove`.

### Операции и хранилища состояния

#### `TraversalNavigationCapabilityProvider`

Объявлен в `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/include/Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit TraversalNavigationCapabilityProvider(const traversal::TraversalService &traversal) : traversal_(traversal);
[[nodiscard]] bool HasCapability(GameplayObjectRef subject, TypeId capability, navigation_semantics::Fixed min_parameter_micro) const noexcept override;
```

#### `ConstructionNavigationLayerPayload`

Объявлен в `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/include/Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h:50`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ConstructionNavigationOperation operation = ConstructionNavigationOperation::Add`; `navigation_semantics::NavigationDecisionKind decision = navigation_semantics::NavigationDecisionKind::AddCost`; `navigation_semantics::NavigationLayerLifetime lifetime = navigation_semantics::NavigationLayerLifetime::Persistent`; `std::int32_t priority = 0`; `navigation_semantics::Fixed additive_cost_micro = 0`; `navigation_semantics::Fixed multiplier_micro = 1'000'000`; `navigation_semantics::Fixed required_parameter_micro = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkTraversal`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp` (106 строк)
- `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp` (431 строк): `Error::Create`, `PlacementOutputTypeId::FromString`, `GameplayDomainId::FromString`, `TraversalNavigationCapabilityProvider::HasCapability`, `TraversalNavigationAdapter::CanUseLink`, `NavigationReasonId::FromString`, `NavigationLayerId::FromRaw`, `ConstructionNavigationAdapter::DecodePayload`, `TypeId::FromRaw`, `ConstructionNavigationAdapter::BuildLayer`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.construction`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp:96`
- `framework.navigation_semantics`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp:100`
- `framework.traversal`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp:104`
- `construction.output.navigation_layer`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:15`
- `framework.construction.placed_object`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:16`
- `navigation.traversal_mode_rejected`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:177`
- `gameplay.integration.construction_navigation_payload_invalid`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:293`
- `gameplay.integration.construction_navigation_operation_invalid`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:322`
- `gameplay.integration.construction_navigation_identity_invalid`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:351`
- `gameplay.integration.construction_navigation_add_conflict`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:383`
- `gameplay.integration.construction_navigation_update_missing`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:391`
- `gameplay.integration.construction_navigation_update_identity_conflict`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:395`
- `traversal.route_invalidated`: `EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:419`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.construction` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp:96`)

```cpp
94 std::unique_ptr<ISaveParticipant> CreateConstructionServiceSaveParticipant(epidemic::gameplay::construction::ConstructionService& owner, SaveRestoreWorkspace& workspace)
   95 {
   96     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::construction::ConstructionService>>(owner, workspace, "framework.construction", std::vector<SaveParticipantId>{});
   97 }
   98 std::unique_ptr<ISaveParticipant> CreateNavigationSemanticsServiceSaveParticipant(epidemic::gameplay::navigation_semantics::NavigationSemanticsService& owner,…
```

#### `framework.navigation_semantics` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp:100`)

```cpp
98 std::unique_ptr<ISaveParticipant> CreateNavigationSemanticsServiceSaveParticipant(epidemic::gameplay::navigation_semantics::NavigationSemanticsService& owner, SaveRestoreWorkspace& workspace)
   99 {
  100     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::navigation_semantics::NavigationSemanticsService>>(owner, workspace, "framework.navigation_semantics", std::vector<SaveParticipantId>{});
  101 }
  102 std::unique_ptr<ISaveParticipant> CreateTraversalServiceSaveParticipant(epidemic::gameplay::traversal::TraversalS…
```

#### `framework.traversal` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/save_participants.cpp:104`)

```cpp
102 std::unique_ptr<ISaveParticipant> CreateTraversalServiceSaveParticipant(epidemic::gameplay::traversal::TraversalService& owner, SaveRestoreWorkspace& workspace)
  103 {
  104     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::traversal::TraversalService>>(owner, workspace, "framework.traversal", std::vector<SaveParticipantId>{});
  105 }
  106 }
```

#### `construction.output.navigation_layer` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:15`)

```cpp
13 }
   14 
   15 constexpr auto NavigationOutputType = construction::PlacementOutputTypeId::FromString("construction.output.navigation_layer");
   16 constexpr auto ConstructionPlacedObjectDomain = GameplayDomainId::FromString("framework.construction.placed_object");
   17 constexpr std::uint16_t NavigationPayloadVersion = 1;
```

#### `framework.construction.placed_object` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:16`)

```cpp
14 
   15 constexpr auto NavigationOutputType = construction::PlacementOutputTypeId::FromString("construction.output.navigation_layer");
   16 constexpr auto ConstructionPlacedObjectDomain = GameplayDomainId::FromString("framework.construction.placed_object");
   17 constexpr std::uint16_t NavigationPayloadVersion = 1;
   18 constexpr std::size_t NavigationPayloadSize = 70;
```

#### `navigation.traversal_mode_rejected` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:177`)

```cpp
175         nav_result.decision = navigation_semantics::NavigationDecisionKind::Deny;
  176         nav_result.availability = navigation_semantics::NavigationAvailability::TraversalUnsupported;
  177         nav_result.reasons.push_back({navigation_semantics::NavigationReasonId::FromString("navigation.traversal_mode_rejected"),
  178                                       {}, {}, link, navigation_semantics::NavigationDecisionKind::Deny});
  179     }
```

#### `gameplay.integration.construction_navigation_payload_invalid` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:293`)

```cpp
291     if (!stable_id.IsValid() || payload.operation == ConstructionNavigationOperation::Remove)
  292         return foundation::Result<navigation_semantics::NavigationSemanticLayer>::Failure(
  293             Error("gameplay.integration.construction_navigation_payload_invalid",
  294                   "construction navigation layer cannot be built from this output"));
  295
```

#### `gameplay.integration.construction_navigation_operation_invalid` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:322`)

```cpp
320         (payload.operation != ConstructionNavigationOperation::Remove && !area.IsValid()))
  321         return foundation::Result<construction::PlacementOutputId>::Failure(
  322             Error("gameplay.integration.construction_navigation_operation_invalid",
  323                   "construction navigation operation is missing stable identity or target area"));
  324
```

#### `gameplay.integration.construction_navigation_identity_invalid` (`EngineFramework/IntegrationLayer/TraversalNavigationConstructionIntegration/src/traversal_navigation_construction_adapters.cpp:351`)

```cpp
349         if (!stable_id.IsValid())
  350             return foundation::Result<std::size_t>::Failure(
  351                 Error("gameplay.integration.construction_navigation_identity_invalid",
  352                       "construction navigation output has no stable layer identity"));
  353
```
