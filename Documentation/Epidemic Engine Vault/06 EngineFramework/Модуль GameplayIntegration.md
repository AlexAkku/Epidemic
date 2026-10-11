---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "gameplay_integration"]
source_path: "EngineFramework/IntegrationLayer/GameplayIntegration"
---

# EngineFramework / GameplayIntegration

Модуль: **GameplayIntegration**. Сопряжение gameplay owners.

**Реализация:** `EngineFramework/IntegrationLayer/GameplayIntegration/`.
**CMake targets:** `EpidemicGameFrameworkGameplayIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/GameplayIntegration`. Admission: 72 public API, 132 mutation obligations, 5 lifecycle, 31 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/GameplayIntegration/gameplay_adapters.h|gameplay_adapters.h]]: `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h`; объявленные типы: `CombatDamageEffectPayload`, `CombatDamageEffectHandler`, `ProgressionCombatRole`, `ProgressionCombatMapping`, `ProgressionCombatModifierProvider`, `CombatAbilityResourceMapping`, `CombatAbilityResourceProvider`, `ReservationToken`, `AbilityEffectDeliveryKey`, `AbilityEffectDeliveryRecord`, `AbilityEffectsCheckpoint`, `AbilityEffectsDispatcher`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/GameplayIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Combat -> Effects damage delivery.
- Progression -> Combat modifier mapping.
- Combat <-> Ability resource reservation.
- Ability -> Effects delivery.
- Conditions -> Progression.
- Ability -> Time scheduled trigger.
- Ability output coordinator.
- Loot -> Progression rewards.
- Death reward delivery.
- Prepare/commit/release failures на каждом внешнем leg.
- Checkpoint restore без duplicate rewards/effects.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/gameplay_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkGameplayIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/GameplayIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ProgressionCombatRole` (`EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h`): `Attacker`, `Target`.
- `DeathRewardDeliveryState` (`EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h`): `Generated`, `Pending`.

### Операции и хранилища состояния

#### `AbilityOutputDeliveryCoordinator`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:218`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
AbilityOutputDeliveryCoordinator(abilities::AbilityService& abilities, AbilityTimeAdapter& time_adapter, AbilityEffectsDispatcher& effects_dispatcher) noexcept : abilities_(abilities), time_adapter_(time_adapter), effects_dispatcher_(effects_dispatcher);
[[nodiscard]] foundation::Result<std::vector<effects::EffectExecutionResult>> DeliverPendingOutputs( ScheduleId trigger);
std::uint64_t PruneSafeTerminalDeliveries(abilities::AbilityExecutionId execution) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `abilities::AbilityService& abilities_`; `AbilityTimeAdapter& time_adapter_`; `AbilityEffectsDispatcher& effects_dispatcher_`.

#### `ConditionProgressionAdapter`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:156`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
ConditionProgressionAdapter(const conditions::ConditionService& conditions,progression::ProgressionService& progression) : conditions_(conditions),progression_(progression);
[[nodiscard]] foundation::Result<void> AddMapping(ConditionProgressionMapping mapping);
[[nodiscard]] foundation::Result<void> FreezeMappings();
[[nodiscard]] std::uint64_t MappingRevision() const noexcept;
[[nodiscard]] foundation::Result<void> ProcessChanges();
[[nodiscard]] foundation::Result<void> ReconcileAll();
[[nodiscard]] ChangeCursor Cursor() const noexcept;
void RestoreCursor(ChangeCursor cursor) noexcept;
[[nodiscard]] ConditionProgressionCheckpoint CaptureCheckpoint() const noexcept;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(ConditionProgressionCheckpoint checkpoint);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `const conditions::ConditionService& conditions_`; `progression::ProgressionService& progression_`; `std::vector<ConditionProgressionMapping> mappings_`; `bool mappings_frozen_ = false`; `std::uint64_t mapping_revision_ = 0`.

#### `CombatAbilityResourceProvider`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:73`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit CombatAbilityResourceProvider(combat::CombatService& combat) : combat_(combat);
[[nodiscard]] foundation::Result<void> AddMapping(CombatAbilityResourceMapping mapping);
[[nodiscard]] foundation::Result<void> FreezeMappings();
[[nodiscard]] bool MappingsFrozen() const noexcept;
[[nodiscard]] std::uint64_t MappingRevision() const noexcept;
[[nodiscard]] bool CanAfford(GameplayObjectRef owner,abilities::AbilityResourceTypeId type,std::int64_t amount_micro) const override;
[[nodiscard]] foundation::Result<abilities::AbilityResourceReservation> Reserve(GameplayObjectRef owner,abilities::AbilityResourceTypeId type,std::int64_t amount_micro,GameplayContext context) override;
void Commit(const abilities::AbilityResourceReservation& reservation,GameplayContext context) noexcept override;
void Release(const abilities::AbilityResourceReservation& reservation,GameplayContext context) noexcept override;
[[nodiscard]] foundation::Result<void> ReconcileReservation(const abilities::AbilityResourceReservation& reservation, GameplayObjectRef owner, GameplayContext context) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `combat::CombatService& combat_`; `std::unordered_map<abilities::AbilityResourceTypeId,combat::CombatResourceTypeId,abilities::IdHash> mappings_`; `bool mappings_frozen_ = false`; `std::uint64_t mapping_revision_ = 0`.

#### `AbilityTimeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:195`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
AbilityTimeAdapter(time::GameplayTimeService& time,abilities::AbilityService& abilities,ClockId clock,ActionTypeId action) : time_(time),abilities_(abilities),clock_(clock),action_(action);
[[nodiscard]] foundation::Result<ScheduleId> ScheduleExecution(abilities::AbilityExecutionId execution);
[[nodiscard]] foundation::Result<std::vector<abilities::AbilityOutput>> ProcessTrigger(const time::ScheduledTrigger& trigger);
[[nodiscard]] const AbilityTimePendingTrigger* FindPendingOutputs(ScheduleId trigger) const noexcept;
[[nodiscard]] foundation::Result<void> AcknowledgeOutputs(ScheduleId trigger);
[[nodiscard]] AbilityTimeCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(AbilityTimeCheckpoint checkpoint);
[[nodiscard]] std::size_t PendingOutputCount() const noexcept;
[[nodiscard]] bool HasPendingOutputsForExecution(abilities::AbilityExecutionId execution) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `time::GameplayTimeService& time_`; `abilities::AbilityService& abilities_`; `static constexpr std::size_t kPendingCapacity = 4096`; `std::vector<AbilityTimePendingTrigger> pending_`.

#### `DeathRewardAdapter`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:273`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
DeathRewardAdapter(const combat::CombatService& combat,loot::LootService& loot) : combat_(combat),loot_(loot);
[[nodiscard]] foundation::Result<void> SetTable(GameplayObjectRef subject,loot::LootTableId table);
[[nodiscard]] foundation::Result<void> FreezeMappings();
[[nodiscard]] std::uint64_t MappingRevision() const noexcept;
[[nodiscard]] foundation::Result<std::vector<loot::RewardExecutionId>> ProcessChanges();
[[nodiscard]] ChangeCursor Cursor() const noexcept;
void RestoreCursor(ChangeCursor cursor) noexcept;
[[nodiscard]] DeathRewardCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(DeathRewardCheckpoint checkpoint);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `const combat::CombatService& combat_`; `loot::LootService& loot_`; `std::unordered_map<GameplayObjectRef,loot::LootTableId> tables_`; `bool mappings_frozen_ = false`; `std::uint64_t mapping_revision_ = 0`; `static constexpr std::size_t kDeliveryCapacity = 4096`; `std::vector<DeathRewardDeliveryRecord> deliveries_`.

#### `CombatDamageEffectHandler`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:25`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit CombatDamageEffectHandler(combat::CombatService& combat) : combat_(combat);
[[nodiscard]] static constexpr effects::EffectTypeId StaticType() noexcept;
[[nodiscard]] static constexpr TypeId PayloadType() noexcept;
[[nodiscard]] static constexpr TypeId PlanTokenType() noexcept;
[[nodiscard]] effects::EffectTypeId Type() const noexcept override;
[[nodiscard]] effects::EffectHandlerCapabilities Capabilities() const noexcept override;
[[nodiscard]] foundation::Result<effects::EffectPrepareResult> Prepare(const effects::EffectOperation& operation) const override;
[[nodiscard]] foundation::Result<effects::EffectCommitResult> Commit(const effects::EffectOperation& operation,const effects::RegisteredEffectPayload& token) noexcept override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `combat::CombatService& combat_`.

#### `AbilityEffectsDispatcher`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:119`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit AbilityEffectsDispatcher(effects::EffectService& effects) : effects_(effects);
[[nodiscard]] foundation::Result<void> Map(ActionTypeId action,effects::EffectDefinitionId definition);
[[nodiscard]] foundation::Result<void> FreezeMappings();
[[nodiscard]] std::uint64_t MappingRevision() const noexcept;
[[nodiscard]] foundation::Result<std::vector<effects::EffectExecutionResult>> Dispatch(std::span<const abilities::AbilityOutput> outputs);
[[nodiscard]] AbilityEffectsCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(AbilityEffectsCheckpoint checkpoint);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `effects::EffectService& effects_`; `std::unordered_map<ActionTypeId,effects::EffectDefinitionId> mappings_`; `bool mappings_frozen_ = false`; `std::uint64_t mapping_revision_ = 0`; `static constexpr std::size_t kDeliveryCapacity = 4096`; `std::vector<AbilityEffectDeliveryRecord> delivered_`.

#### `ProgressionRewardHandler`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:237`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ProgressionRewardHandler(progression::ProgressionService& progression) : progression_(progression);
[[nodiscard]] static constexpr loot::RewardTypeId StaticType() noexcept;
[[nodiscard]] static constexpr TypeId PayloadType() noexcept;
[[nodiscard]] loot::RewardTypeId Type() const noexcept override;
[[nodiscard]] foundation::Result<loot::RewardDeliveryStage> Prepare(const loot::RewardOperation& operation) override;
void Commit(loot::RewardDeliveryStage& stage) noexcept override;
void Cancel(loot::RewardDeliveryStage& stage) noexcept override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `progression::ProgressionService& progression_`.

#### `ProgressionCombatModifierProvider`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:51`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ProgressionCombatModifierProvider(const progression::ProgressionService& progression) : progression_(progression);
[[nodiscard]] foundation::Result<void> AddMapping(ProgressionCombatMapping mapping);
[[nodiscard]] foundation::Result<void> FreezeMappings();
[[nodiscard]] bool MappingsFrozen() const noexcept;
[[nodiscard]] std::uint64_t MappingRevision() const noexcept;
[[nodiscard]] std::vector<combat::CombatModifier> Collect(const combat::DamageRequest& request) const override;
[[nodiscard]] Revision RevisionFor(const combat::DamageRequest& request) const noexcept override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `const progression::ProgressionService& progression_`; `std::vector<ProgressionCombatMapping> mappings_`; `bool mappings_frozen_ = false`; `std::uint64_t mapping_revision_ = 0`.

#### `AbilityEffectDeliveryKey`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:102`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t output_index = 0`.

#### `DeathRewardDeliveryKey`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:253`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t combat_sequence = 0`.

#### `CombatDamageEffectPayload`

Объявлен в `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t seed_salt=0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/GameplayIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkProgression`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp` (222 строк): `SaveParticipantId::FromString`
- `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp` (916 строк): `Error::Create`, `CombatDamageEffectHandler::Prepare`, `RegisteredEffectPayload::FromTrivial`, `CombatDamageEffectHandler::Commit`, `ProgressionCombatModifierProvider::AddMapping`, `ProgressionCombatModifierProvider::FreezeMappings`, `ProgressionCombatModifierProvider::Collect`, `ProgressionCombatModifierProvider::RevisionFor`, `CombatAbilityResourceProvider::AddMapping`, `CombatAbilityResourceProvider::FreezeMappings`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.abilities`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:212`
- `framework.combat`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:220`
- `framework.loot`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:220`
- `framework.progression`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:216`
- `integration.ability_effects`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:208`
- `framework.effects`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:208`
- `integration.ability_time`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:212`
- `framework.time`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:212`
- `integration.condition_progression`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:216`
- `framework.conditions`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:216`
- `integration.death_reward`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:220`
- `gameplay.integration.combat_payload_invalid`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:75`
- `gameplay.integration.combat_plan_invalid`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:101`
- `gameplay.integration.mapping_frozen`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:796`
- `gameplay.integration.mapping_invalid`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:798`
- `gameplay.integration.mapping_duplicate`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:800`
- `gameplay.integration.mapping_not_frozen`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:826`
- `gameplay.integration.resource_unavailable`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:237`
- `gameplay.integration.resource_reservation_invalid`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:277`
- `gameplay.integration.resource_reservation_missing`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:282`
- `gameplay.integration.ability_action_unmapped`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:330`
- `gameplay.integration.ability_output_duplicate`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:335`
- `gameplay.integration.delivery_capacity`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:851`
- `gameplay.integration.checkpoint_invalid`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:910`
- `gameplay.integration.ability_execution_missing`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:594`
- `gameplay.integration.trigger_unhandled`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:619`
- `gameplay.integration.trigger_conflict`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:628`
- `gameplay.integration.ability_output_missing`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:704`
- `gameplay.integration.reward_payload_invalid`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:764`
- `gameplay.integration.change_gap`: `EngineFramework/IntegrationLayer/GameplayIntegration/src/gameplay_adapters.cpp:831`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.abilities` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:212`)

```cpp
210 std::unique_ptr<ISaveParticipant> CreateAbilityTimeAdapterSaveParticipant(epidemic::gameplay::integration::AbilityTimeAdapter& owner, SaveRestoreWorkspace& workspace)
  211 {
  212     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityTimeAdapter>>(owner, workspace, "integration.ability_time", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.time")});
  213 }
  214 std::unique_ptr<ISaveParticipant> CreateConditionProgressio…
```

#### `framework.combat` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:220`)

```cpp
218 std::unique_ptr<ISaveParticipant> CreateDeathRewardAdapterSaveParticipant(epidemic::gameplay::integration::DeathRewardAdapter& owner, SaveRestoreWorkspace& workspace)
  219 {
  220     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::DeathRewardAdapter>>(owner, workspace, "integration.death_reward", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.combat"), SaveParticipantId::FromString("framework.loot")});
  221 }
  222 }
```

#### `framework.loot` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:220`)

```cpp
218 std::unique_ptr<ISaveParticipant> CreateDeathRewardAdapterSaveParticipant(epidemic::gameplay::integration::DeathRewardAdapter& owner, SaveRestoreWorkspace& workspace)
  219 {
  220     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::DeathRewardAdapter>>(owner, workspace, "integration.death_reward", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.combat"), SaveParticipantId::FromString("framework.loot")});
  221 }
  222 }
```

#### `framework.progression` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:216`)

```cpp
214 std::unique_ptr<ISaveParticipant> CreateConditionProgressionAdapterSaveParticipant(epidemic::gameplay::integration::ConditionProgressionAdapter& owner, SaveRestoreWorkspace& workspace)
  215 {
  216     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::ConditionProgressionAdapter>>(owner, workspace, "integration.condition_progression", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.conditions"), SaveParticipantId::FromString("framework.progression")});
  217 }
  218 std::unique_ptr…
```

#### `integration.ability_effects` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:208`)

```cpp
206 std::unique_ptr<ISaveParticipant> CreateAbilityEffectsDispatcherSaveParticipant(epidemic::gameplay::integration::AbilityEffectsDispatcher& owner, SaveRestoreWorkspace& workspace)
  207 {
  208     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityEffectsDispatcher>>(owner, workspace, "integration.ability_effects", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.effects")});
  209 }
  210 std::unique_ptr<ISaveParticipant> C…
```

#### `framework.effects` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:208`)

```cpp
206 std::unique_ptr<ISaveParticipant> CreateAbilityEffectsDispatcherSaveParticipant(epidemic::gameplay::integration::AbilityEffectsDispatcher& owner, SaveRestoreWorkspace& workspace)
  207 {
  208     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityEffectsDispatcher>>(owner, workspace, "integration.ability_effects", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.effects")});
  209 }
  210 std::unique_ptr<ISaveParticipant> C…
```

#### `integration.ability_time` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:212`)

```cpp
210 std::unique_ptr<ISaveParticipant> CreateAbilityTimeAdapterSaveParticipant(epidemic::gameplay::integration::AbilityTimeAdapter& owner, SaveRestoreWorkspace& workspace)
  211 {
  212     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityTimeAdapter>>(owner, workspace, "integration.ability_time", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.time")});
  213 }
  214 std::unique_ptr<ISaveParticipant> CreateConditionProgressio…
```

#### `framework.time` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:212`)

```cpp
210 std::unique_ptr<ISaveParticipant> CreateAbilityTimeAdapterSaveParticipant(epidemic::gameplay::integration::AbilityTimeAdapter& owner, SaveRestoreWorkspace& workspace)
  211 {
  212     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::AbilityTimeAdapter>>(owner, workspace, "integration.ability_time", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.abilities"), SaveParticipantId::FromString("framework.time")});
  213 }
  214 std::unique_ptr<ISaveParticipant> CreateConditionProgressio…
```

#### `integration.condition_progression` (`EngineFramework/IntegrationLayer/GameplayIntegration/src/save_participants.cpp:216`)

```cpp
214 std::unique_ptr<ISaveParticipant> CreateConditionProgressionAdapterSaveParticipant(epidemic::gameplay::integration::ConditionProgressionAdapter& owner, SaveRestoreWorkspace& workspace)
  215 {
  216     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::ConditionProgressionAdapter>>(owner, workspace, "integration.condition_progression", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.conditions"), SaveParticipantId::FromString("framework.progression")});
  217 }
  218 std::unique_ptr…
```
