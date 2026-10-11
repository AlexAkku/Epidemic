---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "extended_gameplay_integration"]
source_path: "EngineFramework/IntegrationLayer/ExtendedGameplayIntegration"
---

# EngineFramework / ExtendedGameplayIntegration

Модуль: **ExtendedGameplayIntegration**. Расширенная композиция gameplay.

**Реализация:** `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/`.
**CMake targets:** `EpidemicGameFrameworkExtendedGameplayIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration`. Admission: 42 public API, 68 mutation obligations, 0 lifecycle, 17 stale-identity, 1 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h|extended_gameplay_adapters.h]]: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h`; объявленные типы: `EquipmentItemsAdapter`, `ProcessItemInputPayload`, `ProcessItemOutputPayload`, `ProcessItemReservationPayload`, `PreparedProcessItemOutputPayload`, `ItemProcessInputProvider`, `ItemProcessOutputHandler`, `DialogueShareKnowledgePayload`, `DialogueAssertKnowledgePayload`, `DialogueKnowledgeConsequenceHandler`, `TradeExecutionId`, `TradeExecutionIdHash`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ExtendedGameplayIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Equipment <-> Items reservation/ownership.
- Processes <-> Items input/output.
- Dialogue -> Knowledge consequence.
- Economy/Items/Ownership coordinated trade.
- Second prepare failure rollback.
- Money leg committed, goods leg failed reconciliation.
- Goods leg committed, ownership leg failed reconciliation.
- Cancel path.
- Restore trade execution checkpoint.
- Повторный retry не дублирует transfer.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

G4-EXTINT-001 and G4-EXTINT-002 are closed. Trade validation no longer consumes the execution generator, and the durable reconciliation record is published before the first Items/Economy reservation can be accepted.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/extended_gameplay_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkExtendedGameplayIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/ExtendedGameplayIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `TradeGoodsLegState` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h`): `NotReserved`, `Reserved`, `Released`, `ItemTransferred`, `OwnershipTransferred`.
- `TradeMoneyLegState` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h`): `NotPrepared`, `Prepared`, `Reserved`, `Committed`, `Cancelled`, `ReconciliationRequired`.
- `CoordinatedTradeState` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h`): `Prepared`, `Committing`, `ReconciliationRequired`, `Completed`, `Cancelled`.

### Операции и хранилища состояния

#### `TradeCoordinator`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:168`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
TradeCoordinator(economy::EconomyService& economy, items::ItemsInventoryService& items, ownership::OwnershipService& ownership) : economy_(economy), items_(items), ownership_(ownership);
[[nodiscard]] foundation::Result<TradeExecutionId> Prepare(CoordinatedTradePlan plan);
[[nodiscard]] foundation::Result<economy::TradeTransactionId> Continue(TradeExecutionId execution);
[[nodiscard]] foundation::Result<economy::TradeTransactionId> Execute(CoordinatedTradePlan plan);
[[nodiscard]] foundation::Result<void> Cancel(TradeExecutionId execution);
[[nodiscard]] const CoordinatedTradeExecution* FindExecution(TradeExecutionId id) const noexcept;
[[nodiscard]] TradeCoordinatorSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(TradeCoordinatorSnapshot snapshot);
void PruneCompleted(std::size_t keep_recent);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `economy::EconomyService& economy_`; `items::ItemsInventoryService& items_`; `ownership::OwnershipService& ownership_`; `std::unordered_map<TradeExecutionId, CoordinatedTradeExecution, TradeExecutionIdHash> executions_`.

#### `ItemProcessOutputHandler`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:69`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ItemProcessOutputHandler(items::ItemsInventoryService& items) : items_(items);
[[nodiscard]] static constexpr processes::ProcessOutputTypeId OutputType() noexcept;
[[nodiscard]] static constexpr TypeId OutputPayloadType() noexcept;
[[nodiscard]] static constexpr TypeId PreparedPayloadType() noexcept;
[[nodiscard]] bool Supports(processes::ProcessOutputTypeId type) const noexcept override;
[[nodiscard]] foundation::Result<processes::PreparedProcessOutput> Prepare(const processes::ProcessOutputDefinition&, const processes::ProcessInstance&, GameplayContext) override;
[[nodiscard]] foundation::Result<void> Commit(const processes::PreparedProcessOutput&, const processes::ProcessInstance&, GameplayContext) override;
[[nodiscard]] foundation::Result<void> Cancel(const processes::PreparedProcessOutput&, const processes::ProcessInstance&, GameplayContext) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `items::ItemsInventoryService& items_`.

#### `ItemProcessInputProvider`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:55`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ItemProcessInputProvider(items::ItemsInventoryService& items) : items_(items);
[[nodiscard]] static constexpr TypeId InputPayloadType() noexcept;
[[nodiscard]] static constexpr TypeId ReservationPayloadType() noexcept;
[[nodiscard]] foundation::Result<void> Validate(const processes::ProcessInputDefinition&, const processes::StartProcessRequest&, processes::ProcessInstanceId) override;
[[nodiscard]] foundation::Result<processes::ReservedProcessInput> Reserve(const processes::ProcessInputDefinition&, const processes::StartProcessRequest&, processes::ProcessInstanceId) override;
[[nodiscard]] foundation::Result<void> Consume(const processes::ReservedProcessInput&, GameplayContext) override;
[[nodiscard]] foundation::Result<void> Release(const processes::ReservedProcessInput&, GameplayContext) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `items::ItemsInventoryService& items_`.

#### `TradeCoordinatorSnapshot`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:162`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<CoordinatedTradeExecution> executions`.

#### `EquipmentItemsAdapter`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:22`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit EquipmentItemsAdapter(items::ItemsInventoryService& items) : items_(items);
[[nodiscard]] std::optional<equipment::EquipmentItemDescriptor> Describe(equipment::EquipmentItemId item) const override;
[[nodiscard]] foundation::Result<void> ReserveForEquipment(equipment::EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> ReleaseFromEquipment(equipment::EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> ReconcileEquipmentReservation( equipment::EquipmentItemId item, GameplayObjectRef subject, equipment::EquipmentBindingId binding, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> ExchangeEquipmentReservations( std::span<const equipment::EquipmentItemId> release_items, std::optional<equipment::EquipmentItemId> reserve_item, GameplayObjectRef subject, GameplayContext context) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `items::ItemsInventoryService& items_`.

#### `DialogueKnowledgeConsequenceHandler`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:102`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit DialogueKnowledgeConsequenceHandler(knowledge::KnowledgeService& knowledge) : knowledge_(knowledge);
[[nodiscard]] static constexpr dialogue::DialogueConsequenceTypeId ShareType() noexcept;
[[nodiscard]] static constexpr dialogue::DialogueConsequenceTypeId AssertType() noexcept;
[[nodiscard]] dialogue::DialogueConsequenceState Execute(const dialogue::DialogueConsequenceDefinition&, const dialogue::DialogueConsequenceExecution&, const dialogue::ConversationSession&) const override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `knowledge::KnowledgeService& knowledge_`.

#### `TradeExecutionId`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:113`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr TradeExecutionId FromRaw(std::uint64_t high, std::uint64_t low) noexcept;
```

#### `TradeExecutionIdHash`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:121`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(TradeExecutionId id) const noexcept;
```

#### `PreparedProcessItemOutputPayload`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:44`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `processes::Fixed amount = 0`.

#### `DialogueShareKnowledgePayload`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:84`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t speaker_index = 0`; `std::uint32_t listener_index = 1`; `knowledge::KnowledgeShareMode mode = knowledge::KnowledgeShareMode::Tell`.

#### `DialogueAssertKnowledgePayload`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:91`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t speaker_index = 0`; `std::uint32_t listener_index = 1`; `knowledge::KnowledgeAssertionValue assertion = knowledge::KnowledgeAssertionValue::Affirmed`; `knowledge::KnowledgeEpistemicState epistemic_state = knowledge::KnowledgeEpistemicState::Suspected`; `knowledge::KnowledgeConfidence confidence = knowledge::KnowledgeConfidence::Medium`.

#### `CoordinatedTradePlan`

Объявлен в `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h:133`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `economy::TradePlan money`; `std::vector<TradeGoodsLine> goods`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkItemsInventory`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp` (168 строк): `SaveParticipantId::FromString`
- `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp` (915 строк): `Error::Create`, `GameplayObjectId::FromRaw`, `ItemProcessInputProvider::InputPayloadType`, `ItemProcessOutputHandler::OutputPayloadType`, `TypeId::FromRaw`, `RegisteredPayload::FromVersioned`, `ItemProcessInputProvider::ReservationPayloadType`, `ItemProcessOutputHandler::PreparedPayloadType`, `ItemInstanceId::FromRaw`, `EquipmentItemsAdapter::Describe`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.dialogue`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:150`
- `framework.economy`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`
- `framework.equipment`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:158`
- `framework.items`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`
- `integration.trade`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`
- `framework.ownership`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`
- `gameplay.integration.equipment_item_invalid`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:275`
- `equipment.binding`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:276`
- `gameplay.integration.equipment_reservation_ambiguous`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:289`
- `gameplay.integration.equipment_reservation_missing`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:294`
- `gameplay.integration.equipment_item_missing`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:249`
- `gameplay.integration.equipment_reservation_mismatch`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:258`
- `gameplay.integration.equipment_exchange_invalid`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:269`
- `gameplay.integration.item_payload_invalid`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:374`
- `process.input`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:319`
- `gameplay.integration.item_output_id_conflict`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:381`
- `dialogue.assertion`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:432`
- `gameplay.trade.parties_invalid`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:454`
- `gameplay.trade.party_mismatch`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:471`
- `gameplay.trade.goods_invalid`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:474`
- `gameplay.trade.item_missing`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:477`
- `gameplay.trade.item_unavailable`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:483`
- `gameplay.trade.owner_mismatch`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:487`
- `gameplay.trade.allocation_failed`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:525`
- `gameplay.trade.id_exhausted`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:506`
- `gameplay.trade.execution_duplicate`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:519`
- `trade.goods`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:847`
- `gameplay.trade.execution_missing`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:712`
- `gameplay.trade.reconciliation_required`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:753`
- `gameplay.trade.already_completed`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:716`
- `gameplay.trade.restore_invalid`: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:900`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.dialogue` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:150`)

```cpp
148 std::unique_ptr<ISaveParticipant> CreateDialogueServiceSaveParticipant(epidemic::gameplay::dialogue::DialogueService& owner, SaveRestoreWorkspace& workspace)
  149 {
  150     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::dialogue::DialogueService>>(owner, workspace, "framework.dialogue", std::vector<SaveParticipantId>{});
  151 }
  152 std::unique_ptr<ISaveParticipant> CreateEconomyServiceSaveParticipant(epidemic::gameplay::economy::EconomyService& owner, SaveRestoreWorkspace& workspace)
```

#### `framework.economy` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`)

```cpp
164 std::unique_ptr<ISaveParticipant> CreateTradeCoordinatorSaveParticipant(epidemic::gameplay::integration::TradeCoordinator& owner, SaveRestoreWorkspace& workspace)
  165 {
  166     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::TradeCoordinator>>(owner, workspace, "integration.trade", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.economy"), SaveParticipantId::FromString("framework.items"), SaveParticipantId::FromString("framework.ownership")});
  167 }
  168 }
```

#### `framework.equipment` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:158`)

```cpp
156 std::unique_ptr<ISaveParticipant> CreateEquipmentServiceSaveParticipant(epidemic::gameplay::equipment::EquipmentService& owner, SaveRestoreWorkspace& workspace)
  157 {
  158     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::equipment::EquipmentService>>(owner, workspace, "framework.equipment", std::vector<SaveParticipantId>{});
  159 }
  160 std::unique_ptr<ISaveParticipant> CreateItemsInventoryServiceSaveParticipant(epidemic::gameplay::items::ItemsInventoryService& owner, SaveRestoreWorkspace& workspace)
```

#### `framework.items` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`)

```cpp
164 std::unique_ptr<ISaveParticipant> CreateTradeCoordinatorSaveParticipant(epidemic::gameplay::integration::TradeCoordinator& owner, SaveRestoreWorkspace& workspace)
  165 {
  166     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::TradeCoordinator>>(owner, workspace, "integration.trade", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.economy"), SaveParticipantId::FromString("framework.items"), SaveParticipantId::FromString("framework.ownership")});
  167 }
  168 }
```

#### `integration.trade` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`)

```cpp
164 std::unique_ptr<ISaveParticipant> CreateTradeCoordinatorSaveParticipant(epidemic::gameplay::integration::TradeCoordinator& owner, SaveRestoreWorkspace& workspace)
  165 {
  166     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::TradeCoordinator>>(owner, workspace, "integration.trade", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.economy"), SaveParticipantId::FromString("framework.items"), SaveParticipantId::FromString("framework.ownership")});
  167 }
  168 }
```

#### `framework.ownership` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/save_participants.cpp:166`)

```cpp
164 std::unique_ptr<ISaveParticipant> CreateTradeCoordinatorSaveParticipant(epidemic::gameplay::integration::TradeCoordinator& owner, SaveRestoreWorkspace& workspace)
  165 {
  166     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::integration::TradeCoordinator>>(owner, workspace, "integration.trade", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.economy"), SaveParticipantId::FromString("framework.items"), SaveParticipantId::FromString("framework.ownership")});
  167 }
  168 }
```

#### `gameplay.integration.equipment_item_invalid` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:275`)

```cpp
273     if (!replacement_item || replacement_item->quantity != 1)
  274         return foundation::Result<void>::Failure(
  275             Error("gameplay.integration.equipment_item_invalid", "equipment requires one concrete replacement item instance"));
  276     const auto reason = TypeId::FromString("equipment.binding");
  277     std::vector<items::ItemReservationId> releases;
```

#### `equipment.binding` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:276`)

```cpp
274         return foundation::Result<void>::Failure(
  275             Error("gameplay.integration.equipment_item_invalid", "equipment requires one concrete replacement item instance"));
  276     const auto reason = TypeId::FromString("equipment.binding");
  277     std::vector<items::ItemReservationId> releases;
  278     releases.reserve(release_items.size());
```

#### `gameplay.integration.equipment_reservation_ambiguous` (`EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:289`)

```cpp
287             if (matching.has_value())
  288                 return foundation::Result<void>::Failure(
  289                     Error("gameplay.integration.equipment_reservation_ambiguous", "multiple equipment reservations match one binding"));
  290             matching = reservation.id;
  291         }
```
