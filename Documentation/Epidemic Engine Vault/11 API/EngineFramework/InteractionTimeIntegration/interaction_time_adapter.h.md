---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h"
---

# InteractionTimeIntegration: interaction_time_adapter.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h`.

**Модуль:** [[06 EngineFramework/Модуль InteractionTimeIntegration|InteractionTimeIntegration]].
**Размер исходного файла:** 3278 байт.
**Подключение:** `#include <Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h>`.

## Объявления типов (лексический индекс)
`InteractionTimeReconciliationKind`, `InteractionTimeReconciliationRecord`, `InteractionTimeAdapter`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/GameFramework/Interaction/interaction.h"
#include "Epidemic/GameFramework/Time/gameplay_time.h"

#include <cstdint>
#include <vector>

namespace epidemic::gameplay::interaction_time_integration
{
enum class InteractionTimeReconciliationKind
{
    OrphanScheduleCancellation,
};

struct InteractionTimeReconciliationRecord
{
    InteractionTimeReconciliationKind kind = InteractionTimeReconciliationKind::OrphanScheduleCancellation;
    interaction::InteractionExecutionId execution{};
    ScheduleId schedule{};
};

class InteractionTimeAdapter
{
  public:
    InteractionTimeAdapter(interaction::InteractionService& interactions, time::GameplayTimeService& time)
        : interactions_(interactions), time_(time)
    {
    }

    // Bootstrap contract. Register the Time action and bind this adapter to the shared scheduled-trigger dispatcher
    // before freezing Time/dispatcher registries. Repeated calls on the same adapter are idempotent.
    [[nodiscard]] foundation::Result<void> RegisterContracts();
    [[nodiscard]] foundation::Result<void> RegisterWithDispatcher(integration::ScheduledTriggerDispatcher& dispatcher);

    // Scheduler bindings are a derived projection of active Interaction sessions. Synchronize is safe to call after
    // restore and after an Interaction journal gap; correctness never depends on retaining cursor history.
    [[nodiscard]] foundation::Result<std::uint64_t> Synchronize(ClockId clock);

    // Public primarily for focused tests/alternative dispatcher composition. Normal production delivery comes through
    // RegisterWithDispatcher(). The shared dispatcher owns destructive Time::CollectDue().
    [[nodiscard]] foundation::Result<integration::ScheduledTriggerDisposition> HandleTrigger(
        const time::ScheduledTrigger& trigger,
        GameplayContext context = {});

    [[nodiscard]] ActionTypeId CompleteAction() const noexcept { return complete_action_; }
    [[nodiscard]] ChangeCursor Cursor() const noexcept { return cursor_; }
    [[nodiscard]] const std::vector<InteractionTimeReconciliationRecord>& PendingReconciliations() const noexcept
    {
        return reconciliations_;
    }

  private:
    [[nodiscard]] static GameplayObjectRef SessionRef(interaction::InteractionExecutionId id) noexcept
    {
        return {interaction::InteractionService::Domain(), id.value};
    }

    [[nodiscard]] foundation::Result<std::uint64_t> ReconcileSchedules(ClockId clock);
    [[nodiscard]] foundation::Result<void> CancelOrRecord(
        interaction::InteractionExecutionId execution,
        ScheduleId schedule);
    [[nodiscard]] std::uint64_t LatestRetainedSequence() const;
    [[nodiscard]] bool HasPendingCompletion(interaction::InteractionExecutionId execution) const;

    interaction::InteractionService& interactions_;
    time::GameplayTimeService& time_;
    integration::ScheduledTriggerDispatcher* dispatcher_ = nullptr;
    ActionTypeId complete_action_{};
    ChangeCursor cursor_{};
    std::vector<InteractionTimeReconciliationRecord> reconciliations_;
};
} // namespace epidemic::gameplay::interaction_time_integration
```
