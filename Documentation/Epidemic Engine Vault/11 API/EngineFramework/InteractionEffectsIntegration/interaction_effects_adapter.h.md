---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h"
---

# InteractionEffectsIntegration: interaction_effects_adapter.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/include/Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h`.

**Модуль:** [[06 EngineFramework/Модуль InteractionEffectsIntegration|InteractionEffectsIntegration]].
**Размер исходного файла:** 5217 байт.
**Подключение:** `#include <Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h>`.

## Объявления типов (лексический индекс)
`InteractionEffectDeliveryState`, `InteractionEffectReconciliationOutcome`, `InteractionEffectReconciliationStatus`, `InteractionEffectReconciliationRequest`, `InteractionEffectDeliveryRecord`, `InteractionEffectsSnapshot`, `InteractionEffectExecutor`, `DeliveryKey`, `DeliveryKeyHash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/GameFramework/Effects/effects.h"
#include "Epidemic/GameFramework/Interaction/interaction.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::interaction_effects_integration
{
enum class InteractionEffectDeliveryState
{
    Pending,
    Applied,
    Rejected,
    ReconciliationRequired,
};

enum class InteractionEffectReconciliationOutcome
{
    None,
    ConfirmedApplied,
    ConfirmedNotApplied,
    ConfirmedRejected,
};

enum class InteractionEffectReconciliationStatus
{
    Resolved,
    AlreadyResolved,
};

struct InteractionEffectReconciliationRequest
{
    interaction::InteractionExecutionId execution{};
    std::uint32_t effect_index = 0;
    InteractionEffectReconciliationOutcome outcome = InteractionEffectReconciliationOutcome::None;
    effects::EffectExecutionId effect_execution{};
};

struct InteractionEffectDeliveryRecord
{
    interaction::InteractionExecutionId execution{};
    interaction::InteractionTypeId interaction_type{};
    std::uint32_t effect_index = 0;
    effects::EffectDefinitionId definition{};
    effects::EffectExecutionId effect_execution{};
    InteractionEffectDeliveryState state = InteractionEffectDeliveryState::Pending;
    InteractionEffectReconciliationOutcome reconciliation = InteractionEffectReconciliationOutcome::None;
};

struct InteractionEffectsSnapshot
{
    std::vector<InteractionEffectDeliveryRecord> deliveries;
};

// Maps one Interaction type to an ordered list of required Effects definitions.
// Mapping configuration is bootstrap-only. Freeze the mapping after both
// definition registries are populated, then register this executor with
// InteractionService before freezing that service. Delivery records are technical
// idempotency state and should participate in save/checkpoint orchestration when
// an Interaction execution can be retried across restore.
class InteractionEffectExecutor final : public interaction::IInteractionExecutor
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<InteractionEffectExecutor>;

  public:
    InteractionEffectExecutor(effects::EffectService& effects, interaction::InteractionTypeId interaction_type)
        : effects_(effects), interaction_type_(interaction_type)
    {
    }

    [[nodiscard]] foundation::Result<void> RegisterEffect(effects::EffectDefinitionId definition);
    [[nodiscard]] foundation::Result<void> Freeze(const interaction::InteractionService& interactions);
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }

    [[nodiscard]] foundation::Result<void> Validate(const interaction::InteractionPlan& plan) const override;
    [[nodiscard]] foundation::Result<void> Commit(
        const interaction::InteractionPlan& plan,
        interaction::InteractionExecutionId execution) noexcept override;

    [[nodiscard]] InteractionEffectsSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(InteractionEffectsSnapshot snapshot);
    [[nodiscard]] foundation::Result<InteractionEffectReconciliationStatus> ReconcileDelivery(
        const InteractionEffectReconciliationRequest& request);
    [[nodiscard]] const InteractionEffectDeliveryRecord* FindDelivery(
        interaction::InteractionExecutionId execution,
        std::uint32_t effect_index) const noexcept;

    // Safe only after the source Interaction execution is terminal and can no
    // longer be retried. Unresolved/Pending deliveries are deliberately retained.
    void PruneTerminalExecution(interaction::InteractionExecutionId execution) noexcept;

  private:
    struct DeliveryKey
    {
        interaction::InteractionExecutionId execution{};
        std::uint32_t effect_index = 0;

        [[nodiscard]] bool operator==(const DeliveryKey&) const noexcept = default;
    };

    struct DeliveryKeyHash
    {
        [[nodiscard]] std::size_t operator()(const DeliveryKey& key) const noexcept;
    };

    [[nodiscard]] static OperationId DeliveryOperation(
        interaction::InteractionExecutionId execution,
        std::uint32_t effect_index,
        effects::EffectDefinitionId definition) noexcept;
    [[nodiscard]] static CorrelationId DeliveryCorrelation(interaction::InteractionExecutionId execution) noexcept;
    [[nodiscard]] foundation::Result<void> ValidateMapping() const;

    effects::EffectService& effects_;
    interaction::InteractionTypeId interaction_type_{};
    std::vector<effects::EffectDefinitionId> definitions_;
    std::unordered_map<DeliveryKey, InteractionEffectDeliveryRecord, DeliveryKeyHash> deliveries_;
    bool frozen_ = false;
};
} // namespace epidemic::gameplay::interaction_effects_integration

// BEGIN GENERATED GOAL5 PARTICIPANTS
#include "Epidemic/GameFramework/SaveGame/save_game.h"
namespace epidemic::gameplay::savegame
{
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateInteractionEffectExecutorSaveParticipant(epidemic::gameplay::interaction_effects_integration::InteractionEffectExecutor& owner, SaveRestoreWorkspace& workspace);
}
// END GENERATED GOAL5 PARTICIPANTS
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/GameFramework/Effects/effects.h`, `Epidemic/GameFramework/Interaction/interaction.h`, `Epidemic/GameFramework/SaveGame/save_game.h`.

### Перечисления и допустимые категории

- `InteractionEffectDeliveryState`: `Pending`, `Applied`, `Rejected`, `ReconciliationRequired`
- `InteractionEffectReconciliationOutcome`: `None`, `ConfirmedApplied`, `ConfirmedNotApplied`, `ConfirmedRejected`
- `InteractionEffectReconciliationStatus`: `Resolved`, `AlreadyResolved`

### Состав типов и операции

#### `InteractionEffectReconciliationRequest` (строка заголовка 34)

Публичные поля и значения по умолчанию:

- `std::uint32_t effect_index = 0`
- `InteractionEffectReconciliationOutcome outcome = InteractionEffectReconciliationOutcome::None`

#### `InteractionEffectDeliveryRecord` (строка заголовка 42)

Публичные поля и значения по умолчанию:

- `std::uint32_t effect_index = 0`
- `InteractionEffectDeliveryState state = InteractionEffectDeliveryState::Pending`
- `InteractionEffectReconciliationOutcome reconciliation = InteractionEffectReconciliationOutcome::None`

#### `InteractionEffectsSnapshot` (строка заголовка 53)

Публичные поля и значения по умолчанию:

- `std::vector<InteractionEffectDeliveryRecord> deliveries`

#### `InteractionEffectExecutor` (строка заголовка 64)

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `DeliveryKey` (строка заголовка 96)

Публичные поля и значения по умолчанию:

- `std::uint32_t effect_index = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const DeliveryKey&) const noexcept = default;
```

#### `DeliveryKeyHash` (строка заголовка 104)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const DeliveryKey& key) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterEffect` | `InteractionEffectExecutor::RegisterEffect` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:46` |
| `Freeze` | `InteractionEffectExecutor::Freeze` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:89` |
| `Validate` | `InteractionEffectExecutor::Validate` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:114` |
| `Commit` | `InteractionEffectExecutor::Commit` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:135` |
| `CaptureSnapshot` | `InteractionEffectExecutor::CaptureSnapshot` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:277` |
| `RestoreSnapshot` | `InteractionEffectExecutor::RestoreSnapshot` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:295` |
| `ReconcileDelivery` | `InteractionEffectExecutor::ReconcileDelivery` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:385` |
| `FindDelivery` | `InteractionEffectExecutor::FindDelivery` | `EngineFramework/IntegrationLayer/InteractionEffectsIntegration/src/interaction_effects_adapter.cpp:499` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterEffect`: `EngineFramework/DevelopmentInfrastructure/Tests/interactioneffectsintegration_save_participant_tests.cpp:27`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/interactioneffectsintegration_save_participant_tests.cpp:28`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/interactioneffectsintegration_save_participant_tests.cpp:34`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/interactioneffectsintegration_save_participant_tests.cpp:43`

