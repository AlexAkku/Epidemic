---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "integration"]
source_path: "EngineFramework/IntegrationLayer/Integration"
---

# EngineFramework / Integration

Модуль: **Integration**. Core gameplay adapters.

**Реализация:** `EngineFramework/IntegrationLayer/Integration/`.
**CMake targets:** `EpidemicGameFrameworkCoreIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/Integration`. Admission: 34 public API, 68 mutation obligations, 1 lifecycle, 14 stale-identity, 2 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Integration/core_adapters.h|core_adapters.h]]: `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h`; объявленные типы: `FileSavePackageStore`, `FindFactsQuery`, `FindHistoryQuery`, `FactsQueryAdapter`, `RuntimeTimeAdapter`, `ScheduledTriggerDisposition`, `ScheduledTriggerActionDeliveryMode`, `ScheduledTriggerDispatcherPolicy`, `ScheduledTriggerDeliveryRecord`, `ScheduledTriggerDispatcherSnapshot`, `ScheduledTriggerPumpReport`, `ScheduledTriggerDispatcherSaveParticipant`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Integration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- FactsQueryAdapter query mapping.
- RuntimeTimeAdapter time projection.
- ScheduledTriggerDispatcher registration/freeze.
- Trigger handler failure.
- Delivery modes.
- Duplicate trigger prevention.
- Dispatcher checkpoint/save participant.
- TimeFactsAdapter exactly-once event projection.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

B08-INT-001 was found during the block audit: completion bookkeeping is now capacity-staged before a ScheduledTrigger handler can accept work, so allocation failure cannot occur after the external callback has produced a side effect.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/core_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkCoreIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/Integration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ScheduledTriggerDisposition` (`EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h`): `Ack`, `Retry`, `DiscardTerminal`.
- `ScheduledTriggerActionDeliveryMode` (`EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h`): `RequiresActionHandler`, `ObserverOnly`.
- `WholeRestorePhase` (`EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h`): `AwaitLoad`, `Precommit`, `CommittedSemantic`, `RebuildTransient`, `ReconcileExternal`, `MaterializeRuntime`, `ReconciliationRequired`, `ReadyToResume`.

### Операции и хранилища состояния

#### `WholeEngineRestoreCoordinator`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:478`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
~WholeEngineRestoreCoordinator();
WholeEngineRestoreCoordinator(const WholeEngineRestoreCoordinator&) = delete;
[[nodiscard]] foundation::Result<SaveGenerationId> LoadAndRestore(const savegame::RestoreContext& context);
[[nodiscard]] foundation::Result<void> RetryReconciliation();
[[nodiscard]] bool CanResume() const noexcept;
[[nodiscard]] WholeRestorePhase Phase() const noexcept;
[[nodiscard]] SaveGenerationId RestoredGeneration() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `savegame::SaveGameOrchestrator& framework_`; `IRuntimeGenerationRestorePort& runtime_`; `SaveGenerationBarrier& barrier_`; `ISaveGenerationStore& store_`; `WholeRestoreCallbacks callbacks_`; `SaveGenerationLimits limits_`; `savegame::SaveWireLimits framework_limits_`; `std::unique_ptr<savegame::ISaveBarrierLease> lease_`.

#### `FileSavePackageStore`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:24`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : path_(foundation::Path(std::move(path)).Native()), limits_(limits);
[[nodiscard]] foundation::Result<std::vector<std::byte>> Load() override;
[[nodiscard]] foundation::Result<void> Commit(std::span<const std::byte> package) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::filesystem::path path_`; `savegame::SaveWireLimits limits_`.

#### `ISaveGenerationStore`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:354`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISaveGenerationStore() = default;
[[nodiscard]] virtual foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() = 0;
[[nodiscard]] virtual foundation::Result<void> CommitCurrent(std::span<const std::byte> package) = 0;
```

#### `FileSaveGenerationStore`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:362`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : path_(foundation::Path(std::move(path)).Native()), limits_(limits);
[[nodiscard]] foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() override;
[[nodiscard]] foundation::Result<void> CommitCurrent(std::span<const std::byte> package) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::filesystem::path path_`; `SaveGenerationLimits limits_`.

#### `RuntimeTimeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:79`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
RuntimeTimeAdapter(runtime::IGameClock& runtime_clock, time::GameplayTimeService& gameplay_time, ClockId clock) : runtime_clock_(runtime_clock), gameplay_time_(gameplay_time), clock_(clock);
[[nodiscard]] foundation::Result<void> Synchronize();
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `runtime::IGameClock& runtime_clock_`; `time::GameplayTimeService& gameplay_time_`.

#### `IRuntimeSaveGenerationPort`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:311`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimeSaveGenerationPort() = default;
[[nodiscard]] virtual foundation::Result<RuntimeSaveGenerationCapture> Capture( SaveGenerationId generation) = 0;
```

#### `CallbackRuntimeSaveGenerationPort`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:322`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
CallbackRuntimeSaveGenerationPort(PlaneCapture time_capture, PlaneCapture persistence_capture) : time_capture_(std::move(time_capture)), persistence_capture_(std::move(persistence_capture));
[[nodiscard]] foundation::Result<RuntimeSaveGenerationCapture> Capture( SaveGenerationId generation) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `PlaneCapture time_capture_`; `PlaneCapture persistence_capture_`.

#### `IRuntimeGenerationRestoreStage`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:439`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimeGenerationRestoreStage() = default;
virtual void Commit() noexcept = 0;
```

#### `IRuntimeGenerationRestorePort`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:446`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimeGenerationRestorePort() = default;
[[nodiscard]] virtual foundation::Result<std::unique_ptr<IRuntimeGenerationRestoreStage>> Prepare( const SaveGenerationPackage& package) = 0;
```

#### `SaveGenerationCoordinator`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:410`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<SaveGenerationId> CaptureAndCommit( const savegame::SaveContext& context);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `savegame::SaveGameOrchestrator& framework_`; `IRuntimeSaveGenerationPort& runtime_`; `SaveGenerationBarrier& barrier_`; `ISaveGenerationStore& store_`; `SaveGenerationLimits limits_`; `savegame::SaveWireLimits framework_limits_`.

#### `RuntimeSaveGenerationCapture`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:303`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> time_package`; `std::vector<std::byte> persistence_package`.

#### `ScheduledTriggerDispatcher`

Объявлен в `EngineFramework/IntegrationLayer/Integration/include/Epidemic/GameFramework/Integration/core_adapters.h:147`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : time_(time_service), policy_(policy);
[[nodiscard]] foundation::Result<void> DeclareRequiresActionHandler(ActionTypeId action);
[[nodiscard]] foundation::Result<void> DeclareObserverOnlyAction(ActionTypeId action);
[[nodiscard]] foundation::Result<void> RegisterActionHandler(ActionTypeId action, ScheduledTriggerHandlerId id, Handler handler);
foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] ScheduledTriggerPumpReport DispatchPending( std::uint64_t max_attempts = 0);
[[nodiscard]] ScheduledTriggerDispatcherSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(ScheduledTriggerDispatcherSnapshot snapshot);
[[nodiscard]] std::size_t PendingCount() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `time::GameplayTimeService& time_`; `std::vector<RegisteredHandler> observers_`; `std::unordered_map<ActionTypeId, ScheduledTriggerActionDeliveryMode> action_manifest_`; `std::unordered_map<ActionTypeId, RegisteredHandler> action_handlers_`; `std::vector<ScheduledTriggerDeliveryRecord> pending_`; `bool frozen_ = false`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/Integration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameplayQueries`, `EpidemicFoundation`, `EpidemicPlatform`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/Integration/src/save_package_store.cpp` (26 строк): `FileSavePackageStore::Load`, `FileSavePackageStore::Commit`
- `EngineFramework/IntegrationLayer/Integration/src/save_participants.cpp` (71 строк)
- `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp` (1261 строк): `OperationId::FromRaw`, `Id::FromRaw`, `GameplayDomainId::FromRaw`, `Error::Create`, `ScheduledTriggerHandlerId::FromRaw`, `ClockId::FromRaw`, `ActionTypeId::FromRaw`, `FactsQueryAdapter::RegisterProviders`, `RuntimeTimeAdapter::Synchronize`, `ScheduledTriggerDispatcher::RegisterObserver`
- `EngineFramework/IntegrationLayer/Integration/src/save_restore.cpp` (147 строк): `Error::Create`, `WholeEngineRestoreCoordinator::WholeEngineRestoreCoordinator`, `WholeEngineRestoreCoordinator::OnOwnerThread`, `WholeEngineRestoreCoordinator::LoadAndRestore`, `WholeEngineRestoreCoordinator::ContinueReconciliation`, `WholeEngineRestoreCoordinator::RetryReconciliation`
- `EngineFramework/IntegrationLayer/Integration/src/save_generation.cpp` (779 строк): `Error::Create`, `SaveGameOrchestrator::HashBytes`, `SaveGenerationId::FromRaw`, `CallbackRuntimeSaveGenerationPort::Capture`, `FileSaveGenerationStore::LoadCurrent`, `FileSaveGenerationStore::CommitCurrent`, `SaveGenerationBarrier::SaveGenerationBarrier`, `SaveGenerationBarrier::AcquireGenerationLease`, `SaveGenerationBarrier::AcquireCaptureLease`, `SaveGenerationBarrier::AcquireRestoreLease`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.facts`: `EngineFramework/IntegrationLayer/Integration/src/save_participants.cpp:65`
- `framework.time`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1249`
- `integration.trigger_save_decode`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1130`
- `framework.query.facts.find`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:371`
- `framework.query.history.find`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:419`
- `integration.trigger_dispatcher_frozen`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:568`
- `integration.trigger_handler_invalid`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:573`
- `integration.trigger_handler_duplicate`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:578`
- `integration.trigger_action_manifest_invalid`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:527`
- `integration.trigger_action_manifest_conflict`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:607`
- `integration.trigger_action_handler_missing`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:602`
- `integration.trigger_action_manifest_missing`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:617`
- `integration.trigger_dispatcher_not_frozen`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:934`
- `integration.trigger_collection_invalid`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:684`
- `integration.trigger_pending_capacity`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:697`
- `integration.trigger_pending_allocation`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:707`
- `integration.trigger_handler_failed`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:824`
- `integration.trigger_snapshot_capacity`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:939`
- `integration.trigger_snapshot_manifest_mismatch`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1001`
- `integration.trigger_snapshot_invalid`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:963`
- `integration.trigger_snapshot_duplicate`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:969`
- `integration.trigger_snapshot_handler_invalid`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1013`
- `integration.scheduled_trigger_dispatcher`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1039`
- `integration.trigger_save_not_frozen`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1048`
- `integration.trigger_save_too_large`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1063`
- `integration.trigger_save_allocation`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1162`
- `integration.trigger_save_section_invalid`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1125`
- `framework.schedule.due`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1184`
- `integration.time_facts_unregistered`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1228`
- `framework.integration.time_facts`: `EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1209`
- `gameplay.restore.invalid_state`: `EngineFramework/IntegrationLayer/Integration/src/save_restore.cpp:50`
- `gameplay.restore.unconfigured`: `EngineFramework/IntegrationLayer/Integration/src/save_restore.cpp:53`

### Конкретные тестовые якоря

- `EngineRuntime/Tests/Integration/integration_tests.cpp`: `TestDefaultCompositionSmoke`, `TestProductionUsesStandardStreamingAdapter`, `TestStreamingUsesResourcesAndPersistence`, `TestAnimationPoseReachesRenderer`, `TestAudioResourceLeaseFollowsVoiceLifetime`, `TestEventPublishFailureRetainsSourceEvents`, `TestCoordinatorShutdownRetriesAdapterCleanup`, `TestPhaseFailureDoesNotStopLaterPhases`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.facts` (`EngineFramework/IntegrationLayer/Integration/src/save_participants.cpp:65`)

```cpp
63 std::unique_ptr<ISaveParticipant> CreateGameplayFactsServiceSaveParticipant(epidemic::gameplay::facts::GameplayFactsService& owner, SaveRestoreWorkspace& workspace)
   64 {
   65     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::facts::GameplayFactsService>>(owner, workspace, "framework.facts", std::vector<SaveParticipantId>{});
   66 }
   67 std::unique_ptr<ISaveParticipant> CreateGameplayTimeServiceSaveParticipant(epidemic::gameplay::time::GameplayTimeService& owner, SaveRestoreWorkspace& workspace)
```

#### `framework.time` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1249`)

```cpp
1247         trigger.owner,
 1248         trigger,
 1249         ProducerId::FromString("framework.time"));
 1250     if (!published)
 1251     {
```

#### `integration.trigger_save_decode` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:1130`)

```cpp
1128     foundation::Result<ScheduledTriggerDispatcherSnapshot> decoded =
 1129         foundation::Result<ScheduledTriggerDispatcherSnapshot>::Failure(
 1130             foundation::Error::Create("integration.trigger_save_decode", "scheduled trigger dispatcher snapshot decode failed"));
 1131     try
 1132     {
```

#### `framework.query.facts.find` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:371`)

```cpp
369 
  370     const auto facts_result = queries_.RegisterSnapshotProvider<FindFactsQuery, facts::FactsSnapshot>(
  371         "framework.query.facts.find",
  372         capabilities,
  373         [this](const FindFactsQuery& query, const queries::QueryContext&) {
```

#### `framework.query.history.find` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:419`)

```cpp
417 
  418     const auto history_result = queries_.RegisterSnapshotProvider<FindHistoryQuery, facts::FactsSnapshot>(
  419         "framework.query.history.find",
  420         capabilities,
  421         [this](const FindHistoryQuery& query, const queries::QueryContext&) {
```

#### `integration.trigger_dispatcher_frozen` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:568`)

```cpp
566     {
  567         return foundation::Result<void>::Failure(
  568             foundation::Error::Create("integration.trigger_dispatcher_frozen", "scheduled trigger dispatcher registry is frozen"));
  569     }
  570     if (!action.IsValid() || !id.IsValid() || !handler)
```

#### `integration.trigger_handler_invalid` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:573`)

```cpp
571     {
  572         return foundation::Result<void>::Failure(
  573             foundation::Error::Create("integration.trigger_handler_invalid", "scheduled trigger action/id/callback must be valid"));
  574     }
  575     if (HandlerExists(id) || action_handlers_.contains(action))
```

#### `integration.trigger_handler_duplicate` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:578`)

```cpp
576     {
  577         return foundation::Result<void>::Failure(
  578             foundation::Error::Create("integration.trigger_handler_duplicate", "scheduled trigger action or handler id is already registered"));
  579     }
  580
```

#### `integration.trigger_action_manifest_invalid` (`EngineFramework/IntegrationLayer/Integration/src/core_adapters.cpp:527`)

```cpp
525     {
  526         return foundation::Result<void>::Failure(
  527             foundation::Error::Create("integration.trigger_action_manifest_invalid", "scheduled trigger action manifest mode is invalid"));
  528     }
  529
```
