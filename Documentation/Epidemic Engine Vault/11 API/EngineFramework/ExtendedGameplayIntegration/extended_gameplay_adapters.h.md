---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h"
---

# ExtendedGameplayIntegration: extended_gameplay_adapters.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/include/Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h`.

**Модуль:** [[06 EngineFramework/Модуль ExtendedGameplayIntegration|ExtendedGameplayIntegration]].
**Размер исходного файла:** 11521 байт.
**Подключение:** `#include <Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h>`.

## Объявления типов (лексический индекс)
`EquipmentItemsAdapter`, `ProcessItemInputPayload`, `ProcessItemOutputPayload`, `ProcessItemReservationPayload`, `PreparedProcessItemOutputPayload`, `ItemProcessInputProvider`, `ItemProcessOutputHandler`, `DialogueShareKnowledgePayload`, `DialogueAssertKnowledgePayload`, `DialogueKnowledgeConsequenceHandler`, `TradeExecutionId`, `TradeExecutionIdHash`, `TradeGoodsLine`, `CoordinatedTradePlan`, `TradeGoodsLegState`, `TradeMoneyLegState`, `CoordinatedTradeState`, `CoordinatedTradeGoodsExecution`, `CoordinatedTradeExecution`, `TradeCoordinatorSnapshot`, `TradeCoordinator`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once
#include "Epidemic/GameFramework/Dialogue/dialogue.h"
#include "Epidemic/GameFramework/Economy/economy.h"
#include "Epidemic/GameFramework/Equipment/equipment.h"
#include "Epidemic/GameFramework/ItemsInventory/items_inventory.h"
#include "Epidemic/GameFramework/Knowledge/knowledge.h"
#include "Epidemic/GameFramework/Ownership/ownership.h"
#include "Epidemic/GameFramework/Processes/processes.h"

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::integration
{
[[nodiscard]] constexpr GameplayObjectRef ItemPropertyRef(items::ItemInstanceId item) noexcept
{
    return {items::ItemsInventoryService::Domain(), item.value};
}

class EquipmentItemsAdapter final : public equipment::IEquipmentItemProvider
{
public:
    explicit EquipmentItemsAdapter(items::ItemsInventoryService& items) : items_(items) {}
    [[nodiscard]] std::optional<equipment::EquipmentItemDescriptor> Describe(equipment::EquipmentItemId item) const override;
    [[nodiscard]] foundation::Result<void> ReserveForEquipment(equipment::EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) override;
    [[nodiscard]] foundation::Result<void> ReleaseFromEquipment(equipment::EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) override;
    [[nodiscard]] foundation::Result<void> ReconcileEquipmentReservation(
        equipment::EquipmentItemId item, GameplayObjectRef subject, equipment::EquipmentBindingId binding,
        GameplayContext context) override;
    [[nodiscard]] foundation::Result<void> ExchangeEquipmentReservations(
        std::span<const equipment::EquipmentItemId> release_items,
        std::optional<equipment::EquipmentItemId> reserve_item,
        GameplayObjectRef subject,
        GameplayContext context) override;
private:
    items::ItemsInventoryService& items_;
};

struct ProcessItemInputPayload { items::ItemInstanceId item{}; };
struct ProcessItemOutputPayload { items::ItemDefinitionId definition{}; items::ContainerId container{}; };
struct ProcessItemReservationPayload { items::ItemReservationId reservation{}; };
struct PreparedProcessItemOutputPayload
{
    items::ItemDefinitionId definition{};
    items::ContainerId container{};
    items::ItemInstanceId item{};
    processes::Fixed amount = 0;
};

[[nodiscard]] processes::RegisteredPayload EncodeProcessItemInputPayload(ProcessItemInputPayload payload);
[[nodiscard]] processes::RegisteredPayload EncodeProcessItemOutputPayload(ProcessItemOutputPayload payload);

class ItemProcessInputProvider final : public processes::IProcessInputProvider
{
public:
    explicit ItemProcessInputProvider(items::ItemsInventoryService& items) : items_(items) {}
    [[nodiscard]] static constexpr TypeId InputPayloadType() noexcept { return TypeId::FromString("framework.process.item.input"); }
    [[nodiscard]] static constexpr TypeId ReservationPayloadType() noexcept { return TypeId::FromString("framework.process.item.reservation"); }
    [[nodiscard]] foundation::Result<void> Validate(const processes::ProcessInputDefinition&, const processes::StartProcessRequest&, processes::ProcessInstanceId) override;
    [[nodiscard]] foundation::Result<processes::ReservedProcessInput> Reserve(const processes::ProcessInputDefinition&, const processes::StartProcessRequest&, processes::ProcessInstanceId) override;
    [[nodiscard]] foundation::Result<void> Consume(const processes::ReservedProcessInput&, GameplayContext) override;
    [[nodiscard]] foundation::Result<void> Release(const processes::ReservedProcessInput&, GameplayContext) override;
private:
    items::ItemsInventoryService& items_;
};

class ItemProcessOutputHandler final : public processes::IProcessOutputHandler
{
public:
    explicit ItemProcessOutputHandler(items::ItemsInventoryService& items) : items_(items) {}
    [[nodiscard]] static constexpr processes::ProcessOutputTypeId OutputType() noexcept { return processes::ProcessOutputTypeId::FromString("framework.output.item"); }
    [[nodiscard]] static constexpr TypeId OutputPayloadType() noexcept { return TypeId::FromString("framework.process.item.output"); }
    [[nodiscard]] static constexpr TypeId PreparedPayloadType() noexcept { return TypeId::FromString("framework.process.item.output.prepared"); }
    [[nodiscard]] bool Supports(processes::ProcessOutputTypeId type) const noexcept override { return type == OutputType(); }
    [[nodiscard]] foundation::Result<processes::PreparedProcessOutput> Prepare(const processes::ProcessOutputDefinition&, const processes::ProcessInstance&, GameplayContext) override;
    [[nodiscard]] foundation::Result<void> Commit(const processes::PreparedProcessOutput&, const processes::ProcessInstance&, GameplayContext) override;
    [[nodiscard]] foundation::Result<void> Cancel(const processes::PreparedProcessOutput&, const processes::ProcessInstance&, GameplayContext) override;
private:
    items::ItemsInventoryService& items_;
};

struct DialogueShareKnowledgePayload
{
    std::uint32_t speaker_index = 0;
    std::uint32_t listener_index = 1;
    knowledge::KnowledgeRecordId record{};
    knowledge::KnowledgeShareMode mode = knowledge::KnowledgeShareMode::Tell;
};
struct DialogueAssertKnowledgePayload
{
    std::uint32_t speaker_index = 0;
    std::uint32_t listener_index = 1;
    knowledge::BeliefTypeId belief{};
    knowledge::KnowledgeTopicId topic{};
    knowledge::KnowledgeAssertionValue assertion = knowledge::KnowledgeAssertionValue::Affirmed;
    knowledge::KnowledgeEpistemicState epistemic_state = knowledge::KnowledgeEpistemicState::Suspected;
    knowledge::KnowledgeConfidence confidence = knowledge::KnowledgeConfidence::Medium;
    GameplayObjectRef subject{};
};
class DialogueKnowledgeConsequenceHandler final : public dialogue::IDialogueConsequenceHandler
{
public:
    explicit DialogueKnowledgeConsequenceHandler(knowledge::KnowledgeService& knowledge) : knowledge_(knowledge) {}
    [[nodiscard]] static constexpr dialogue::DialogueConsequenceTypeId ShareType() noexcept { return dialogue::DialogueConsequenceTypeId::FromString("dialogue.knowledge.share"); }
    [[nodiscard]] static constexpr dialogue::DialogueConsequenceTypeId AssertType() noexcept { return dialogue::DialogueConsequenceTypeId::FromString("dialogue.knowledge.assert"); }
    [[nodiscard]] dialogue::DialogueConsequenceState Execute(const dialogue::DialogueConsequenceDefinition&, const dialogue::DialogueConsequenceExecution&, const dialogue::ConversationSession&) const override;
private:
    knowledge::KnowledgeService& knowledge_;
};

struct TradeExecutionId
{
    GameplayObjectId value{};
    static constexpr TradeExecutionId FromRaw(std::uint64_t high, std::uint64_t low) noexcept { return {GameplayObjectId::FromRaw(high, low)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const TradeExecutionId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const TradeExecutionId&) const noexcept = default;
};
struct TradeExecutionIdHash
{
    [[nodiscard]] std::size_t operator()(TradeExecutionId id) const noexcept { return std::hash<GameplayObjectId>{}(id.value); }
};

struct TradeGoodsLine
{
    items::ItemInstanceId item{};
    items::ContainerId destination{};
    GameplayObjectRef from_owner{};
    GameplayObjectRef to_owner{};
};
struct CoordinatedTradePlan
{
    economy::TradePlan money;
    std::vector<TradeGoodsLine> goods;
};
enum class TradeGoodsLegState { NotReserved, Reserved, Released, ItemTransferred, OwnershipTransferred };
enum class TradeMoneyLegState { NotPrepared, Prepared, Reserved, Committed, Cancelled, ReconciliationRequired };
enum class CoordinatedTradeState { Prepared, Committing, ReconciliationRequired, Completed, Cancelled };
struct CoordinatedTradeGoodsExecution
{
    TradeGoodsLine line;
    items::Fixed quantity = 0;
    items::ItemLocation source{};
    items::ItemReservationId reservation{};
    Revision ownership_revision{};
    ownership::PropertyDomainId ownership_domain{};
    TradeGoodsLegState state = TradeGoodsLegState::NotReserved;
};
struct CoordinatedTradeExecution
{
    TradeExecutionId id{};
    economy::TradeTransactionId money_transaction{};
    TradeMoneyLegState money_state = TradeMoneyLegState::NotPrepared;
    GameplayObjectRef buyer{};
    GameplayObjectRef seller{};
    GameplayContext context{};
    std::vector<CoordinatedTradeGoodsExecution> goods;
    CoordinatedTradeState state = CoordinatedTradeState::Prepared;
};
struct TradeCoordinatorSnapshot
{
    std::vector<CoordinatedTradeExecution> executions;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot execution_ids{};
};

class TradeCoordinator
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<TradeCoordinator>;

public:
    TradeCoordinator(economy::EconomyService& economy, items::ItemsInventoryService& items, ownership::OwnershipService& ownership)
        : economy_(economy), items_(items), ownership_(ownership) {}

    [[nodiscard]] foundation::Result<TradeExecutionId> Prepare(CoordinatedTradePlan plan);
    [[nodiscard]] foundation::Result<economy::TradeTransactionId> Continue(TradeExecutionId execution);
    [[nodiscard]] foundation::Result<economy::TradeTransactionId> Execute(CoordinatedTradePlan plan);
    [[nodiscard]] foundation::Result<void> Cancel(TradeExecutionId execution);
    [[nodiscard]] const CoordinatedTradeExecution* FindExecution(TradeExecutionId id) const noexcept;
    [[nodiscard]] TradeCoordinatorSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(TradeCoordinatorSnapshot snapshot);
    void PruneCompleted(std::size_t keep_recent);

private:
    economy::EconomyService& economy_;
    items::ItemsInventoryService& items_;
    ownership::OwnershipService& ownership_;
    MonotonicIdGenerator<GameplayObjectId> execution_ids_{0x4700};
    std::unordered_map<TradeExecutionId, CoordinatedTradeExecution, TradeExecutionIdHash> executions_;
};
}

// BEGIN GENERATED GOAL5 PARTICIPANTS
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/Dialogue/dialogue.h"
#include "Epidemic/GameFramework/Economy/economy.h"
#include "Epidemic/GameFramework/Equipment/equipment.h"
#include "Epidemic/GameFramework/ItemsInventory/items_inventory.h"
namespace epidemic::gameplay::savegame
{
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateDialogueServiceSaveParticipant(epidemic::gameplay::dialogue::DialogueService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateEconomyServiceSaveParticipant(epidemic::gameplay::economy::EconomyService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateEquipmentServiceSaveParticipant(epidemic::gameplay::equipment::EquipmentService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateItemsInventoryServiceSaveParticipant(epidemic::gameplay::items::ItemsInventoryService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateTradeCoordinatorSaveParticipant(epidemic::gameplay::integration::TradeCoordinator& owner, SaveRestoreWorkspace& workspace);
}
// END GENERATED GOAL5 PARTICIPANTS
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/GameFramework/Dialogue/dialogue.h`, `Epidemic/GameFramework/Economy/economy.h`, `Epidemic/GameFramework/Equipment/equipment.h`, `Epidemic/GameFramework/ItemsInventory/items_inventory.h`, `Epidemic/GameFramework/Knowledge/knowledge.h`, `Epidemic/GameFramework/Ownership/ownership.h`, `Epidemic/GameFramework/Processes/processes.h`, `Epidemic/GameFramework/SaveGame/save_game.h`, `Epidemic/GameFramework/Dialogue/dialogue.h`, `Epidemic/GameFramework/Economy/economy.h`, `Epidemic/GameFramework/Equipment/equipment.h`, `Epidemic/GameFramework/ItemsInventory/items_inventory.h`.

### Перечисления и допустимые категории

- `TradeGoodsLegState`: `NotReserved`, `Reserved`, `Released`, `ItemTransferred`, `OwnershipTransferred`
- `TradeMoneyLegState`: `NotPrepared`, `Prepared`, `Reserved`, `Committed`, `Cancelled`, `ReconciliationRequired`
- `CoordinatedTradeState`: `Prepared`, `Committing`, `ReconciliationRequired`, `Completed`, `Cancelled`

### Состав типов и операции

#### `EquipmentItemsAdapter` (строка заголовка 22)

Публичные сигнатуры операций:

```cpp
explicit EquipmentItemsAdapter(items::ItemsInventoryService& items) : items_(items);
[[nodiscard]] std::optional<equipment::EquipmentItemDescriptor> Describe(equipment::EquipmentItemId item) const override;
[[nodiscard]] foundation::Result<void> ReserveForEquipment(equipment::EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> ReleaseFromEquipment(equipment::EquipmentItemId item, GameplayObjectRef subject, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> ReconcileEquipmentReservation( equipment::EquipmentItemId item, GameplayObjectRef subject, equipment::EquipmentBindingId binding, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> ExchangeEquipmentReservations( std::span<const equipment::EquipmentItemId> release_items, std::optional<equipment::EquipmentItemId> reserve_item, GameplayObjectRef subject, GameplayContext context) override;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `PreparedProcessItemOutputPayload` (строка заголовка 44)

Публичные поля и значения по умолчанию:

- `processes::Fixed amount = 0`

#### `ItemProcessInputProvider` (строка заголовка 55)

Публичные сигнатуры операций:

```cpp
explicit ItemProcessInputProvider(items::ItemsInventoryService& items) : items_(items);
[[nodiscard]] static constexpr TypeId InputPayloadType() noexcept;
[[nodiscard]] static constexpr TypeId ReservationPayloadType() noexcept;
[[nodiscard]] foundation::Result<void> Validate(const processes::ProcessInputDefinition&, const processes::StartProcessRequest&, processes::ProcessInstanceId) override;
[[nodiscard]] foundation::Result<processes::ReservedProcessInput> Reserve(const processes::ProcessInputDefinition&, const processes::StartProcessRequest&, processes::ProcessInstanceId) override;
[[nodiscard]] foundation::Result<void> Consume(const processes::ReservedProcessInput&, GameplayContext) override;
[[nodiscard]] foundation::Result<void> Release(const processes::ReservedProcessInput&, GameplayContext) override;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ItemProcessOutputHandler` (строка заголовка 69)

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `DialogueShareKnowledgePayload` (строка заголовка 84)

Публичные поля и значения по умолчанию:

- `std::uint32_t speaker_index = 0`
- `std::uint32_t listener_index = 1`
- `knowledge::KnowledgeShareMode mode = knowledge::KnowledgeShareMode::Tell`

#### `DialogueAssertKnowledgePayload` (строка заголовка 91)

Публичные поля и значения по умолчанию:

- `std::uint32_t speaker_index = 0`
- `std::uint32_t listener_index = 1`
- `knowledge::KnowledgeAssertionValue assertion = knowledge::KnowledgeAssertionValue::Affirmed`
- `knowledge::KnowledgeEpistemicState epistemic_state = knowledge::KnowledgeEpistemicState::Suspected`
- `knowledge::KnowledgeConfidence confidence = knowledge::KnowledgeConfidence::Medium`

#### `DialogueKnowledgeConsequenceHandler` (строка заголовка 102)

Публичные сигнатуры операций:

```cpp
explicit DialogueKnowledgeConsequenceHandler(knowledge::KnowledgeService& knowledge) : knowledge_(knowledge);
[[nodiscard]] static constexpr dialogue::DialogueConsequenceTypeId ShareType() noexcept;
[[nodiscard]] static constexpr dialogue::DialogueConsequenceTypeId AssertType() noexcept;
[[nodiscard]] dialogue::DialogueConsequenceState Execute(const dialogue::DialogueConsequenceDefinition&, const dialogue::DialogueConsequenceExecution&, const dialogue::ConversationSession&) const override;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `TradeExecutionId` (строка заголовка 113)

Публичные сигнатуры операций:

```cpp
static constexpr TradeExecutionId FromRaw(std::uint64_t high, std::uint64_t low) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const TradeExecutionId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const TradeExecutionId&) const noexcept = default;
```

#### `TradeExecutionIdHash` (строка заголовка 121)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(TradeExecutionId id) const noexcept;
```

#### `CoordinatedTradePlan` (строка заголовка 133)

Публичные поля и значения по умолчанию:

- `economy::TradePlan money`
- `std::vector<TradeGoodsLine> goods`

#### `CoordinatedTradeGoodsExecution` (строка заголовка 141)

Публичные поля и значения по умолчанию:

- `TradeGoodsLine line`
- `items::Fixed quantity = 0`
- `TradeGoodsLegState state = TradeGoodsLegState::NotReserved`

#### `CoordinatedTradeExecution` (строка заголовка 151)

Публичные поля и значения по умолчанию:

- `TradeMoneyLegState money_state = TradeMoneyLegState::NotPrepared`
- `std::vector<CoordinatedTradeGoodsExecution> goods`
- `CoordinatedTradeState state = CoordinatedTradeState::Prepared`

#### `TradeCoordinatorSnapshot` (строка заголовка 162)

Публичные поля и значения по умолчанию:

- `std::vector<CoordinatedTradeExecution> executions`

#### `TradeCoordinator` (строка заголовка 168)

Внутреннее состояние (не часть публичного API): `economy::EconomyService& economy_`, `items::ItemsInventoryService& items_`, `ownership::OwnershipService& ownership_`, `std::unordered_map<TradeExecutionId, CoordinatedTradeExecution, TradeExecutionIdHash> execution`.

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `InputPayloadType` | `ItemProcessInputProvider::InputPayloadType` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:109` |
| `OutputPayloadType` | `ItemProcessOutputHandler::OutputPayloadType` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:118` |
| `ReservationPayloadType` | `ItemProcessInputProvider::ReservationPayloadType` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:132` |
| `PreparedPayloadType` | `ItemProcessOutputHandler::PreparedPayloadType` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:151` |
| `Describe` | `EquipmentItemsAdapter::Describe` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:196` |
| `ReserveForEquipment` | `EquipmentItemsAdapter::ReserveForEquipment` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:210` |
| `ReleaseFromEquipment` | `EquipmentItemsAdapter::ReleaseFromEquipment` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:222` |
| `ReconcileEquipmentReservation` | `EquipmentItemsAdapter::ReconcileEquipmentReservation` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:242` |
| `ExchangeEquipmentReservations` | `EquipmentItemsAdapter::ExchangeEquipmentReservations` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:261` |
| `Validate` | `ItemProcessInputProvider::Validate` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:302` |
| `Reserve` | `ItemProcessInputProvider::Reserve` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:311` |
| `Consume` | `ItemProcessInputProvider::Consume` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:329` |
| `Release` | `ItemProcessInputProvider::Release` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:337` |
| `Prepare` | `ItemProcessOutputHandler::Prepare` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:347` |
| `Commit` | `ItemProcessOutputHandler::Commit` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:368` |
| `Cancel` | `ItemProcessOutputHandler::Cancel` | `EngineFramework/IntegrationLayer/ExtendedGameplayIntegration/src/extended_gameplay_adapters.cpp:393` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/extendedgameplayintegration_save_participant_tests.cpp:31`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/extendedgameplayintegration_save_participant_tests.cpp:32`

