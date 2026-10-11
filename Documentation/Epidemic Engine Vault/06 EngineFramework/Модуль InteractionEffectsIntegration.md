---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "interaction_effects_integration"]
source_path: "EngineFramework/IntegrationLayer/InteractionEffectsIntegration"
---

# EngineFramework / InteractionEffectsIntegration

Модуль: **InteractionEffectsIntegration**. Связь взаимодействия и эффектов.

**Реализация:** `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/`.
**CMake targets:** `EpidemicGameFrameworkInteractionEffectsIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration`. Admission: 11 public API, 24 mutation obligations, 1 lifecycle, 5 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/InteractionEffectsIntegration/interaction_effects_adapter.h|interaction_effects_adapter.h]]: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h`; объявленные типы: `InteractionEffectDeliveryState`, `InteractionEffectReconciliationOutcome`, `InteractionEffectReconciliationStatus`, `InteractionEffectReconciliationRequest`, `InteractionEffectDeliveryRecord`, `InteractionEffectsSnapshot`, `InteractionEffectExecutor`, `DeliveryKey`, `DeliveryKeyHash`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/InteractionEffectsIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Interaction completion -> Effects exactly once.
- Effect prepare failure.
- Effect commit failure.
- Reconciliation state.
- Duplicate interaction completion.
- Restore delivery records.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_effects_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkInteractionEffectsIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/InteractionEffectsIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `InteractionEffectDeliveryState` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h`): `Pending`, `Applied`, `Rejected`, `ReconciliationRequired`.
- `InteractionEffectReconciliationOutcome` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h`): `None`, `ConfirmedApplied`, `ConfirmedNotApplied`, `ConfirmedRejected`.
- `InteractionEffectReconciliationStatus` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h`): `Resolved`, `AlreadyResolved`.

### Операции и хранилища состояния

#### `InteractionEffectExecutor`

Объявлен в `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h:64`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
InteractionEffectExecutor(effects::EffectService& effects, interaction::InteractionTypeId interaction_type) : effects_(effects), interaction_type_(interaction_type);
[[nodiscard]] foundation::Result<void> RegisterEffect(effects::EffectDefinitionId definition);
[[nodiscard]] foundation::Result<void> Freeze(const interaction::InteractionService& interactions);
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] foundation::Result<void> Validate(const interaction::InteractionPlan& plan) const override;
[[nodiscard]] foundation::Result<void> Commit( const interaction::InteractionPlan& plan, interaction::InteractionExecutionId execution) noexcept override;
[[nodiscard]] InteractionEffectsSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(InteractionEffectsSnapshot snapshot);
[[nodiscard]] foundation::Result<InteractionEffectReconciliationStatus> ReconcileDelivery( const InteractionEffectReconciliationRequest& request);
[[nodiscard]] const InteractionEffectDeliveryRecord* FindDelivery( interaction::InteractionExecutionId execution, std::uint32_t effect_index) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `effects::EffectService& effects_`; `std::vector<effects::EffectDefinitionId> definitions_`; `std::unordered_map<DeliveryKey, InteractionEffectDeliveryRecord, DeliveryKeyHash> deliveries_`; `bool frozen_ = false`.

#### `InteractionEffectReconciliationRequest`

Объявлен в `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h:34`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t effect_index = 0`; `InteractionEffectReconciliationOutcome outcome = InteractionEffectReconciliationOutcome::None`.

#### `InteractionEffectDeliveryRecord`

Объявлен в `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h:42`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t effect_index = 0`; `InteractionEffectDeliveryState state = InteractionEffectDeliveryState::Pending`; `InteractionEffectReconciliationOutcome reconciliation = InteractionEffectReconciliationOutcome::None`.

#### `InteractionEffectsSnapshot`

Объявлен в `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h:53`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<InteractionEffectDeliveryRecord> deliveries`.

#### `DeliveryKey`

Объявлен в `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h:96`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t effect_index = 0`.

#### `DeliveryKeyHash`

Объявлен в `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h:104`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const DeliveryKey& key) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkInteraction`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp` (523 строк): `Error::Create`, `InteractionEffectExecutor::DeliveryOperation`, `OperationId::FromRaw`, `InteractionEffectExecutor::DeliveryCorrelation`, `CorrelationId::FromRaw`, `InteractionEffectExecutor::RegisterEffect`, `InteractionEffectExecutor::ValidateMapping`, `InteractionEffectExecutor::Freeze`, `InteractionEffectExecutor::Validate`, `InteractionEffectExecutor::Commit`
- `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/save_participants.cpp` (33 строк): `SaveParticipantId::FromString`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.registry_frozen`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:51`
- `gameplay.interaction_effects.mapping_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:72`
- `gameplay.interaction_effects.mapping_duplicate`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:61`
- `gameplay.interaction_effects.effect_definition_missing`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:80`
- `gameplay.interaction_effects.interaction_definition_missing`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:98`
- `gameplay.interaction_effects.effects_not_frozen`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:103`
- `gameplay.registry_not_frozen`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:391`
- `gameplay.interaction_effects.mapping_mismatch`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:124`
- `gameplay.interaction_effects.plan_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:130`
- `gameplay.interaction_effects.execution_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:149`
- `gameplay.interaction_effects.delivery_rejected`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:258`
- `gameplay.interaction_effects.reconciliation_required`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:264`
- `gameplay.interaction_effects.delivery_conflict`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:194`
- `gameplay.interaction_effects.effect_exception`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:235`
- `gameplay.interaction_effects.exception`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:273`
- `gameplay.interaction_effects.snapshot_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:352`
- `gameplay.interaction_effects.snapshot_duplicate_execution`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:368`
- `gameplay.interaction_effects.snapshot_duplicate`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:377`
- `gameplay.interaction_effects.reconciliation_key_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:396`
- `gameplay.interaction_effects.reconciliation_outcome_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:491`
- `gameplay.interaction_effects.reconciliation_evidence_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:409`
- `gameplay.interaction_effects.delivery_missing`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:417`
- `gameplay.interaction_effects.reconciliation_evidence_conflict`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:473`
- `gameplay.interaction_effects.reconciliation_conflict`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:439`
- `gameplay.interaction_effects.reconciliation_state_invalid`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:462`
- `integration.interaction_effects`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/save_participants.cpp:31`
- `framework.interaction`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/save_participants.cpp:31`
- `framework.effects`: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/save_participants.cpp:31`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.registry_frozen` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:51`)

```cpp
49     {
   50         return foundation::Result<void>::Failure(
   51             Error("gameplay.registry_frozen", "interaction effect mapping is frozen"));
   52     }
   53     if (!definition.IsValid())
```

#### `gameplay.interaction_effects.mapping_invalid` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:72`)

```cpp
70     {
   71         return foundation::Result<void>::Failure(
   72             Error("gameplay.interaction_effects.mapping_invalid", "mapping requires a valid interaction type and at least one effect"));
   73     }
   74     for (const auto definition : definitions_)
```

#### `gameplay.interaction_effects.mapping_duplicate` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:61`)

```cpp
59     {
   60         return foundation::Result<void>::Failure(
   61             Error("gameplay.interaction_effects.mapping_duplicate", "effect definition is already mapped to this interaction"));
   62     }
   63     definitions_.push_back(definition);
```

#### `gameplay.interaction_effects.effect_definition_missing` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:80`)

```cpp
78         {
   79             return foundation::Result<void>::Failure(
   80                 Error("gameplay.interaction_effects.effect_definition_missing", "mapped effect definition is missing or invalid"));
   81         }
   82         // EffectService validates every registered step payload against its handler
```

#### `gameplay.interaction_effects.interaction_definition_missing` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:98`)

```cpp
96     {
   97         return foundation::Result<void>::Failure(
   98             Error("gameplay.interaction_effects.interaction_definition_missing", "mapped interaction definition is missing"));
   99     }
  100     if (!effects_.IsFrozen())
```

#### `gameplay.interaction_effects.effects_not_frozen` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:103`)

```cpp
101     {
  102         return foundation::Result<void>::Failure(
  103             Error("gameplay.interaction_effects.effects_not_frozen", "effect registry must be frozen before interaction-effect mappings"));
  104     }
  105     auto valid = ValidateMapping();
```

#### `gameplay.registry_not_frozen` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:391`)

```cpp
389     {
  390         return foundation::Result<InteractionEffectReconciliationStatus>::Failure(
  391             Error("gameplay.registry_not_frozen", "interaction effect mapping must be frozen before reconciliation"));
  392     }
  393     if (!request.execution.IsValid() || request.effect_index >= definitions_.size())
```

#### `gameplay.interaction_effects.mapping_mismatch` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:124`)

```cpp
122     {
  123         return foundation::Result<void>::Failure(
  124             Error("gameplay.interaction_effects.mapping_mismatch", "interaction plan does not match this effect mapping"));
  125     }
  126     if (!plan.candidate.actor.IsValid() || !plan.candidate.target.IsValid() ||
```

#### `gameplay.interaction_effects.plan_invalid` (`EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:130`)

```cpp
128     {
  129         return foundation::Result<void>::Failure(
  130             Error("gameplay.interaction_effects.plan_invalid", "interaction actor/target context is invalid or inconsistent"));
  131     }
  132     return ValidateMapping();
```
