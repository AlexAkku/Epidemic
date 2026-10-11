---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "state_integration"]
source_path: "EngineFramework/IntegrationLayer/StateIntegration"
---

# EngineFramework / StateIntegration

Модуль: **StateIntegration**. Согласование gameplay state.

**Реализация:** `EngineFramework/IntegrationLayer/StateIntegration/`.
**CMake targets:** `EpidemicGameFrameworkStateIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/StateIntegration`. Admission: 52 public API, 72 mutation obligations, 0 lifecycle, 13 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/StateIntegration/state_adapters.h|state_adapters.h]]: `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h`; объявленные типы: `GetEntityQuery`, `FindEntitiesQuery`, `GetMaterialSlotsQuery`, `GetConditionsQuery`, `StateQueryAdapter`, `ConditionFactValue`, `StateFactsCheckpoint`, `StateFactsAdapter`, `EntityCreateEffectPayload`, `EntityConvertEffectPayload`, `EntityTagEffectPayload`, `MaterialStimulusEffectPayload`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/StateIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Entity/material/condition queries.
- State -> Facts projection.
- Entity create/convert/tag effects.
- Material stimulus/exposure effects.
- Condition apply/remove effect delivery.
- Entity target state provider.
- Lifecycle adapter.
- State <-> Time processing.
- Deferred effect reconciliation.
- Combined checkpoint and persistence.
- Duplicate event/effect protection.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/state_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkStateIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/StateIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ConditionEffectDeliveryState` (`EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h`): `Pending`, `Applied`, `RejectedTerminal`, `ReconciliationRequired`.
- `ConditionEffectReconciliationResolution` (`EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h`): `ConfirmedApplied`, `ConfirmedNotApplied`, `RejectedTerminal`.

### Операции и хранилища состояния

#### `StateTimeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:405`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
StateTimeAdapter( conditions::ConditionService& conditions, effects::EffectService& effects, time::GameplayTimeService& time) : conditions_(conditions), effects_(effects), time_(time);
[[nodiscard]] foundation::Result<void> RegisterContracts();
[[nodiscard]] StateTimeCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(StateTimeCheckpoint checkpoint);
[[nodiscard]] std::span<const DeferredEffectReconciliationRecord> DeferredReconciliations() const noexcept;
[[nodiscard]] foundation::Result<void> ResolveDeferredReconciliation(effects::DeferredEffectId deferred);
[[nodiscard]] ActionTypeId ExpireAction() const noexcept;
[[nodiscard]] ActionTypeId PeriodicAction() const noexcept;
[[nodiscard]] ActionTypeId DeferredEffectAction() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `conditions::ConditionService& conditions_`; `effects::EffectService& effects_`; `time::GameplayTimeService& time_`; `integration::ScheduledTriggerDispatcher* dispatcher_ = nullptr`; `static constexpr std::size_t kDeferredReconciliationCapacity = 4096`; `std::vector<DeferredEffectReconciliationRecord> deferred_reconciliations_`.

#### `StateFactsAdapter`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:97`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
StateFactsAdapter( entities::EntityService& entities, materials::MaterialService& materials, conditions::ConditionService& conditions, effects::EffectService& effects, facts::GameplayFactsService& facts) : entities_(entities), materials_(materials), conditions_(conditions), effects_(effects), facts_(facts);
[[nodiscard]] foundation::Result<void> RegisterContracts();
[[nodiscard]] StateFactsCheckpoint CaptureCheckpoint() const noexcept;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(StateFactsCheckpoint checkpoint);
[[nodiscard]] EventTypeId EntityChangedEvent() const noexcept;
[[nodiscard]] EventTypeId MaterialChangedEvent() const noexcept;
[[nodiscard]] EventTypeId ConditionChangedEvent() const noexcept;
[[nodiscard]] EventTypeId EffectChangedEvent() const noexcept;
[[nodiscard]] FactTypeId ActiveConditionFact() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `entities::EntityService& entities_`; `materials::MaterialService& materials_`; `conditions::ConditionService& conditions_`; `effects::EffectService& effects_`; `facts::GameplayFactsService& facts_`.

#### `ConditionEffectsAdapter`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:342`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
ConditionEffectsAdapter(conditions::ConditionService& conditions, effects::EffectService& effects) : conditions_(conditions), effects_(effects);
[[nodiscard]] foundation::Result<void> RegisterRoute(ActionTypeId action, effects::EffectDefinitionId definition);
[[nodiscard]] ConditionEffectsCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(ConditionEffectsCheckpoint checkpoint);
[[nodiscard]] std::span<const ConditionEffectDeliveryRecord> Deliveries() const noexcept;
[[nodiscard]] foundation::Result<void> ResolveReconciliation( const ConditionEffectDeliveryKey& key, ConditionEffectReconciliationResolution resolution);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `conditions::ConditionService& conditions_`; `effects::EffectService& effects_`; `std::unordered_map<ActionTypeId, effects::EffectDefinitionId> routes_`; `static constexpr std::size_t kDeliveryCapacity = 4096`; `std::vector<ConditionEffectDeliveryRecord> deliveries_`.

#### `StateLifecycleAdapter`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:268`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
StateLifecycleAdapter( entities::EntityService& entities, materials::MaterialService& materials, conditions::ConditionService& conditions, effects::EffectService& effects) : entities_(entities), materials_(materials), conditions_(conditions), effects_(effects);
[[nodiscard]] StateLifecycleCheckpoint CaptureCheckpoint() const noexcept;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(StateLifecycleCheckpoint checkpoint);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `entities::EntityService& entities_`; `materials::MaterialService& materials_`; `conditions::ConditionService& conditions_`; `effects::EffectService& effects_`.

#### `StateIntegrationPersistence`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:490`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
StateIntegrationPersistence( StateFactsAdapter& facts, StateLifecycleAdapter& lifecycle, ConditionEffectsAdapter& condition_effects, StateTimeAdapter& time) noexcept : facts_(facts), lifecycle_(lifecycle), condition_effects_(condition_effects), time_(time);
[[nodiscard]] StateIntegrationCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(StateIntegrationCheckpoint checkpoint);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `StateFactsAdapter& facts_`; `StateLifecycleAdapter& lifecycle_`; `ConditionEffectsAdapter& condition_effects_`; `StateTimeAdapter& time_`.

#### `StateQueryAdapter`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:52`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
StateQueryAdapter( entities::EntityService& entities, materials::MaterialService& materials, conditions::ConditionService& conditions, const GameplayTagRegistry& tags, queries::GameplayQueryService& queries) : entities_(entities), materials_(materials), conditions_(conditions), tags_(tags), queries_(queries);
[[nodiscard]] foundation::Result<void> RegisterProviders();
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `entities::EntityService& entities_`; `materials::MaterialService& materials_`; `conditions::ConditionService& conditions_`; `const GameplayTagRegistry& tags_`; `queries::GameplayQueryService& queries_`.

#### `MaterialResponseEffectRouter`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:193`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<void> Register(materials::MaterialResponseTypeId response, Builder builder);
[[nodiscard]] foundation::Result<std::vector<effects::EffectOperation>> Build( const materials::MaterialResponse& response, const effects::EffectOperation& parent) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<std::uint64_t, Builder> builders_`.

#### `EntityTargetStateProvider`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:209`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit EntityTargetStateProvider(const entities::EntityService& entities) : entities_(entities);
[[nodiscard]] effects::EffectTargetState Resolve(GameplayObjectRef target) const override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `const entities::EntityService& entities_`.

#### `GetEntityQuery`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:21`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `FindEntitiesQuery`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:28`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `GetMaterialSlotsQuery`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:38`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `GetConditionsQuery`

Объявлен в `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/StateIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp` (276 строк): `FactsAccess::Clone`, `LifecycleAccess::Clone`, `DeliveryAccess::Clone`, `TimeAccess::Clone`, `FactsAccess::Commit`, `LifecycleAccess::Commit`, `DeliveryAccess::Commit`, `TimeAccess::Commit`, `SaveParticipantId::FromString`
- `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp` (2301 строк): `Error::Create`, `TypeId::FromString`, `EffectTypeId::FromString`, `EntityService::FromGameplayObjectRef`, `StateQueryAdapter::RegisterProviders`, `StateFactsAdapter::RegisterContracts`, `EntityService::Domain`, `MaterialService::Domain`, `ConditionService::Domain`, `EffectService::Domain`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.conditions`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `framework.effects`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `framework.entities`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `framework.materials`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `integration.state`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `framework.facts`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `framework.time`: `EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`
- `framework.payload.entity.create.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:14`
- `framework.payload.entity.convert.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:15`
- `framework.payload.entity.tag.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:16`
- `framework.payload.material.stimulus.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:17`
- `framework.payload.material.exposure.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:18`
- `framework.payload.condition.apply.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:19`
- `framework.payload.condition.remove_type.v1`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:20`
- `gameplay.state_integration_journal_gap`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:47`
- `gameplay.effect_payload_decode`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:111`
- `framework.effect.entity.create`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1038`
- `framework.effect.entity.destroy`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1043`
- `framework.effect.entity.convert`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1047`
- `framework.effect.entity.add_tag`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1052`
- `framework.effect.entity.remove_tag`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1057`
- `framework.effect.material.stimulus`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1062`
- `framework.effect.material.add_exposure`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1068`
- `framework.effect.material.remove_exposure`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1073`
- `framework.effect.condition.apply`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1079`
- `framework.effect.condition.remove_type`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:1090`
- `framework.query.entities.get`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:500`
- `framework.query.entities.find`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:516`
- `framework.query.materials.slots`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:557`
- `framework.query.conditions.subject`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:573`
- `framework.entities.changed`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:588`
- `framework.materials.changed`: `EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:596`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.conditions` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `framework.effects` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `framework.entities` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `framework.materials` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `integration.state` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `framework.facts` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `framework.time` (`EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp:274`)

```cpp
272 std::unique_ptr<ISaveParticipant> CreateStateIntegrationPersistenceSaveParticipant(epidemic::gameplay::state_integration::StateIntegrationPersistence& owner, SaveRestoreWorkspace& workspace)
  273 {
  274     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::state_integration::StateIntegrationPersistence>>(owner, workspace, "integration.state", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.entities"), SaveParticipantId::FromString("framework.materials"), SaveParticipantId::FromString("framewor…
```

#### `framework.payload.entity.create.v1` (`EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:14`)

```cpp
12 namespace
   13 {
   14 constexpr std::string_view kEntityCreatePayloadName = "framework.payload.entity.create.v1";
   15 constexpr std::string_view kEntityConvertPayloadName = "framework.payload.entity.convert.v1";
   16 constexpr std::string_view kEntityTagPayloadName = "framework.payload.entity.tag.v1";
```

#### `framework.payload.entity.convert.v1` (`EngineFramework/IntegrationLayer/StateIntegration/src/state_adapters.cpp:15`)

```cpp
13 {
   14 constexpr std::string_view kEntityCreatePayloadName = "framework.payload.entity.create.v1";
   15 constexpr std::string_view kEntityConvertPayloadName = "framework.payload.entity.convert.v1";
   16 constexpr std::string_view kEntityTagPayloadName = "framework.payload.entity.tag.v1";
   17 constexpr std::string_view kMaterialStimulusPayloadName = "framework.payload.material.stimulus.v1";
```
