---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "interaction_time_integration"]
source_path: "EngineFramework/IntegrationLayer/InteractionTimeIntegration"
---

# EngineFramework / InteractionTimeIntegration

Модуль: **InteractionTimeIntegration**. Связь взаимодействий и времени.

**Реализация:** `EngineFramework/IntegrationLayer/InteractionTimeIntegration/`.
**CMake targets:** `EpidemicGameFrameworkInteractionTimeIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/InteractionTimeIntegration`. Admission: 8 public API, 16 mutation obligations, 0 lifecycle, 4 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/InteractionTimeIntegration/interaction_time_adapter.h|interaction_time_adapter.h]]: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h`; объявленные типы: `InteractionTimeReconciliationKind`, `InteractionTimeReconciliationRecord`, `InteractionTimeAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/InteractionTimeIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Interaction -> scheduled time binding.
- Cancel interaction removes/invalidates schedule по контракту.
- Trigger after interaction terminal state.
- Duplicate schedule delivery.
- Reconciliation after restore.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/interaction_time_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkInteractionTimeIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/InteractionTimeIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `InteractionTimeReconciliationKind` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h`): `OrphanScheduleCancellation`.

### Операции и хранилища состояния

#### `InteractionTimeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h:24`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
InteractionTimeAdapter(interaction::InteractionService& interactions, time::GameplayTimeService& time) : interactions_(interactions), time_(time);
[[nodiscard]] foundation::Result<void> RegisterWithDispatcher(integration::ScheduledTriggerDispatcher& dispatcher);
correctness never depends on retaining cursor history. [[nodiscard]] foundation::Result<std::uint64_t> Synchronize(ClockId clock);
// Public primarily for focused tests/alternative dispatcher composition. Normal production delivery comes through // RegisterWithDispatcher(). The shared dispatcher owns destructive Time::CollectDue(). [[nodiscard]] foundation::Result<integration::ScheduledTriggerDisposition> HandleTrigger( const time::ScheduledTrigger& trigger, GameplayContext context =;
[[nodiscard]] ActionTypeId CompleteAction() const noexcept;
[[nodiscard]] ChangeCursor Cursor() const noexcept;
[[nodiscard]] const std::vector<InteractionTimeReconciliationRecord>& PendingReconciliations() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `interaction::InteractionService& interactions_`; `time::GameplayTimeService& time_`; `integration::ScheduledTriggerDispatcher* dispatcher_ = nullptr`; `std::vector<InteractionTimeReconciliationRecord> reconciliations_`.

#### `InteractionTimeReconciliationRecord`

Объявлен в `EngineFramework/IntegrationLayer/InteractionTimeIntegration/include/Epidemic/GameFramework/InteractionTimeIntegration/interaction_time_adapter.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `InteractionTimeReconciliationKind kind = InteractionTimeReconciliationKind::OrphanScheduleCancellation`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkInteraction`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp` (354 строк): `Error::Create`, `InteractionService::Domain`, `InteractionTimeAdapter::RegisterContracts`, `InteractionTimeAdapter::RegisterWithDispatcher`, `ScheduledTriggerHandlerId::FromString`, `InteractionTimeAdapter::CancelOrRecord`, `InteractionTimeAdapter::LatestRetainedSequence`, `InteractionTimeAdapter::HasPendingCompletion`, `InteractionTimeAdapter::ReconcileSchedules`, `InteractionTimeAdapter::Synchronize`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.interaction.complete`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:30`
- `gameplay.interaction_time.unregistered`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:45`
- `gameplay.interaction_time.dispatcher_rebind`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:54`
- `framework.integration.interaction_time.complete`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:59`
- `gameplay.interaction_time.invalid_reconcile`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:132`
- `gameplay.interaction_time.clock_mismatch`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:191`
- `gameplay.interaction_time.dispatcher_unbound`: `EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:284`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.interaction.complete` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:30`)

```cpp
28     }
   29 
   30     auto registered = time_.RegisterAction("framework.interaction.complete", interaction::InteractionService::Domain());
   31     if (!registered)
   32     {
```

#### `gameplay.interaction_time.unregistered` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:45`)

```cpp
43     {
   44         return foundation::Result<void>::Failure(
   45             E("gameplay.interaction_time.unregistered", "interaction completion action must be registered before dispatcher binding"));
   46     }
   47     if (dispatcher_ == &dispatcher)
```

#### `gameplay.interaction_time.dispatcher_rebind` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:54`)

```cpp
52     {
   53         return foundation::Result<void>::Failure(
   54             E("gameplay.interaction_time.dispatcher_rebind", "interaction time adapter is already bound to another dispatcher"));
   55     }
   56
```

#### `framework.integration.interaction_time.complete` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:59`)

```cpp
57     auto result = dispatcher.RegisterActionHandler(
   58         complete_action_,
   59         integration::ScheduledTriggerHandlerId::FromString("framework.integration.interaction_time.complete"),
   60         [this](const time::ScheduledTrigger& trigger, const GameplayContext& context) {
   61             return HandleTrigger(trigger, context);
```

#### `gameplay.interaction_time.invalid_reconcile` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:132`)

```cpp
130     {
  131         return foundation::Result<std::uint64_t>::Failure(
  132             E("gameplay.interaction_time.invalid_reconcile", "interaction time reconciliation requires a valid clock and registered action"));
  133     }
  134
```

#### `gameplay.interaction_time.clock_mismatch` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:191`)

```cpp
189             {
  190                 return foundation::Result<std::uint64_t>::Failure(
  191                     E("gameplay.interaction_time.clock_mismatch", "bound interaction completion schedule belongs to another clock"));
  192             }
  193             const auto cancelled = CancelOrRecord(execution, entry.id);
```

#### `gameplay.interaction_time.dispatcher_unbound` (`EngineFramework/IntegrationLayer/InteractionTimeIntegration/src/interaction_time_adapter.cpp:284`)

```cpp
282     {
  283         return foundation::Result<std::uint64_t>::Failure(
  284             E("gameplay.interaction_time.dispatcher_unbound", "interaction time adapter must be registered with the shared trigger dispatcher"));
  285     }
  286
```
