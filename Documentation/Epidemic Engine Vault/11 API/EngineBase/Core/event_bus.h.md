---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Core/include/Epidemic/Core/event_bus.h"
---

# Core: event_bus.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Core/include/Epidemic/Core/event_bus.h`.

**Модуль:** [[04 EngineBase/Модуль Core|Core]].
**Размер исходного файла:** 5409 байт.
**Подключение:** `#include <Epidemic/Core/event_bus.h>`.

## Объявления типов (лексический индекс)
`EventDispatchMode`, `IEventBus`, `EventBus`, `Subscription`, `QueuedEvent`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <any>
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
#include <shadow_probe.h>
#endif
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace epidemic::core::events
{
// This file defines the minimal in-process event bus used by EngineBase.
// The bus supports immediate dispatch and queued dispatch so systems can choose whether
// handlers run synchronously at publish time or later during the frame loop drain phase.

enum class EventDispatchMode
{
    Sync,
    Queued,
};

class IEventBus
{
  public:
    using HandlerToken = std::uint64_t;
    using AnyEventHandler = std::function<void(const std::any &)>;

    virtual ~IEventBus() = default;

    // Registers a synchronous handler for EventT.
    template <typename EventT, typename HandlerT> HandlerToken SubscribeSync(HandlerT &&handler)
    {
        auto typed_handler = std::function<void(const EventT &)>(std::forward<HandlerT>(handler));
        if (!typed_handler)
        {
            throw std::invalid_argument("Event handler must be valid");
        }

        return SubscribeImpl(typeid(EventT), EventDispatchMode::Sync,
                             [typed_handler = std::move(typed_handler)](const std::any &payload) {
                                 typed_handler(std::any_cast<const EventT &>(payload));
                             });
    }

    // Registers a queued handler for EventT.
    template <typename EventT, typename HandlerT> HandlerToken SubscribeQueued(HandlerT &&handler)
    {
        auto typed_handler = std::function<void(const EventT &)>(std::forward<HandlerT>(handler));
        if (!typed_handler)
        {
            throw std::invalid_argument("Event handler must be valid");
        }

        return SubscribeImpl(typeid(EventT), EventDispatchMode::Queued,
                             [typed_handler = std::move(typed_handler)](const std::any &payload) {
                                 typed_handler(std::any_cast<const EventT &>(payload));
                             });
    }

    // Immediately dispatches one event to synchronous subscribers of EventT.
    template <typename EventT> void PublishSync(const EventT &event)
    {
        PublishSyncImpl(typeid(EventT), std::any(event));
    }

    // Queues one event for later delivery to queued subscribers of EventT.
    template <typename EventT> void Enqueue(const EventT &event)
    {
        EnqueueImpl(typeid(EventT), std::any(event));
    }

    // Drains queued events and returns the number of dispatched items.
    [[nodiscard]] virtual std::size_t DrainQueued() = 0;

    // Removes a previously registered handler token.
    [[nodiscard]] virtual bool Unsubscribe(HandlerToken token) = 0;

  private:
    // Registers a type-erased handler for the specified event type and dispatch mode.
    virtual HandlerToken SubscribeImpl(std::type_index event_type, EventDispatchMode mode, AnyEventHandler handler) = 0;

    // Immediately dispatches a type-erased event to synchronous subscribers.
    virtual void PublishSyncImpl(std::type_index event_type, std::any event) = 0;

    // Enqueues a type-erased event for later dispatch.
    virtual void EnqueueImpl(std::type_index event_type, std::any event) = 0;
};

class EventBus final : public IEventBus
{
  public:
    // Drains queued events in FIFO order and returns the number processed.
    [[nodiscard]] std::size_t DrainQueued() override;

    // Removes a handler token from either the synchronous or queued subscription tables.
    [[nodiscard]] bool Unsubscribe(HandlerToken token) override;

  private:
    struct Subscription
    {
        HandlerToken token{};
        AnyEventHandler handler;
    };

    struct QueuedEvent
    {
        std::type_index type{typeid(void)};
        std::any payload;
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
        std::uint64_t auditor_correlation{}; // Build-only queue delivery identity.
        auditor::shadow::Context auditor_context;
#endif
    };

    // Registers a type-erased handler in the correct subscription map.
    HandlerToken SubscribeImpl(std::type_index event_type, EventDispatchMode mode, AnyEventHandler handler) override;

    // Immediately dispatches an event to synchronous subscribers.
    void PublishSyncImpl(std::type_index event_type, std::any event) override;

    // Stores an event in the queued FIFO.
    void EnqueueImpl(std::type_index event_type, std::any event) override;

    // Copies synchronous subscribers for an event type and invokes them outside the lock.
    void DispatchSync(std::type_index event_type, const std::any &event);

    // Copies queued subscribers for an event type and invokes them outside the lock.
    void DispatchQueued(std::type_index event_type, const std::any &event);

    std::mutex drain_mutex_;
    std::mutex mutex_;
    HandlerToken next_token_{1};
    std::unordered_map<std::type_index, std::vector<Subscription>> sync_subscriptions_;
    std::unordered_map<std::type_index, std::vector<Subscription>> queued_subscriptions_;
    std::queue<QueuedEvent> queued_events_;
};
} // namespace epidemic::core::events
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `EventDispatchMode`: `Sync`, `Queued`

### Состав типов и операции

#### `IEventBus` (строка заголовка 29)

Публичные сигнатуры операций:

```cpp
virtual ~IEventBus() = default;
// Registers a synchronous handler for EventT. template <typename EventT, typename HandlerT> HandlerToken SubscribeSync(HandlerT &&handler);
// Registers a queued handler for EventT. template <typename EventT, typename HandlerT> HandlerToken SubscribeQueued(HandlerT &&handler);
// Immediately dispatches one event to synchronous subscribers of EventT. template <typename EventT> void PublishSync(const EventT &event);
// Queues one event for later delivery to queued subscribers of EventT. template <typename EventT> void Enqueue(const EventT &event);
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `EventBus` (строка заголовка 96)

#### `Subscription` (строка заголовка 106)

Публичные поля и значения по умолчанию:

- `AnyEventHandler handler`

#### `QueuedEvent` (строка заголовка 112)

Публичные поля и значения по умолчанию:

- `std::any payload`

Публичные сигнатуры операций:

```cpp
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES std::uint64_t auditor_correlation;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

