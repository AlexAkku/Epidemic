---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "process_resource_simulation_integration"]
source_path: "EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration"
---

# EngineFramework / ProcessResourceSimulationIntegration

Модуль: **ProcessResourceSimulationIntegration**. Связь процессов и ресурсов с симуляцией.

**Реализация:** `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/`.
**CMake targets:** `EpidemicGameFrameworkProcessResourceSimulationIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration`. Admission: 22 public API, 32 mutation obligations, 0 lifecycle, 8 stale-identity, 1 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h|process_resource_simulation_adapters.h]]: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/include/Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h`; объявленные типы: `ProcessResourcePayload`, `ProcessResourceReservationPayload`, `PreparedProcessResourceOutputPayload`, `ResourceProcessInputProvider`, `ResourceProcessOutputHandler`, `ProcessesSimulationLayer`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/ProcessResourceSimulationIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Processes <-> resource input reservation.
- Output prepare/commit.
- Provider failure.
- Resource quantity rollback.
- Simulation layer proposal/application.
- Duplicate simulation execution.
- Restore checkpoint/reconciliation.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

B08-PRSINT-001 was found during the block audit: provider-token storage is staged before Resources accepts a reservation, eliminating the post-accept allocation gap.

#### Persistence and reconciliation

No independent adapter-owned snapshot/checkpoint surface is required by this freeze unit. Durable state is either carried by owner-issued tokens/outputs or explicitly delegated to the authoritative owners; whole-engine persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/process_resource_simulation_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkProcessResourceSimulationIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/ProcessResourceSimulationIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Операции и хранилища состояния

#### `ResourceProcessInputProvider`

Объявлен в `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/include/Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h:36`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ResourceProcessInputProvider(resources::ResourcesProductionService& resources) : resources_(resources);
[[nodiscard]] static constexpr TypeId PayloadType() noexcept;
[[nodiscard]] static constexpr TypeId ReservationPayloadType() noexcept;
[[nodiscard]] foundation::Result<void> Validate(const processes::ProcessInputDefinition& input, const processes::StartProcessRequest& request, processes::ProcessInstanceId instance) override;
[[nodiscard]] foundation::Result<processes::ReservedProcessInput> Reserve(const processes::ProcessInputDefinition& input, const processes::StartProcessRequest& request, processes::ProcessInstanceId instance) override;
[[nodiscard]] foundation::Result<void> Consume(const processes::ReservedProcessInput& reservation, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> Release(const processes::ReservedProcessInput& reservation, GameplayContext context) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `resources::ResourcesProductionService& resources_`.

#### `ResourceProcessOutputHandler`

Объявлен в `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/include/Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h:50`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ResourceProcessOutputHandler(resources::ResourcesProductionService& resources) : resources_(resources);
[[nodiscard]] static constexpr processes::ProcessOutputTypeId OutputType() noexcept;
[[nodiscard]] bool Supports(processes::ProcessOutputTypeId type) const noexcept override;
[[nodiscard]] foundation::Result<processes::PreparedProcessOutput> Prepare(const processes::ProcessOutputDefinition& output, const processes::ProcessInstance& instance, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> Commit(const processes::PreparedProcessOutput& output, const processes::ProcessInstance& instance, GameplayContext context) override;
[[nodiscard]] foundation::Result<void> Cancel(const processes::PreparedProcessOutput& output, const processes::ProcessInstance& instance, GameplayContext context) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `resources::ResourcesProductionService& resources_`.

#### `ProcessesSimulationLayer`

Объявлен в `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/include/Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h:66`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit ProcessesSimulationLayer(processes::ProcessesService& processes) : processes_(processes);
[[nodiscard]] static constexpr simulation::SimulationLayerId StaticLayer() noexcept;
[[nodiscard]] simulation::SimulationLayerId Layer() const noexcept override;
[[nodiscard]] foundation::Result<simulation::SimulationLayerSummary> Prepare(const simulation::SimulationTask& task) override;
[[nodiscard]] foundation::Result<void> Commit(const simulation::SimulationTask& task, const simulation::SimulationLayerSummary& summary) override;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `processes::ProcessesService& processes_`.

#### `PreparedProcessResourceOutputPayload`

Объявлен в `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/include/Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h:21`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `processes::Fixed amount = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkProcesses`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp` (108 строк)
- `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp` (364 строк): `Error::Create`, `ResourceProcessInputProvider::PayloadType`, `GameplayObjectId::FromRaw`, `TypeId::FromRaw`, `ResourceProcessInputProvider::ReservationPayloadType`, `ResourceProcessInputProvider::Validate`, `ResourceProcessInputProvider::Reserve`, `ProcessReservationId::FromRaw`, `TypeId::FromString`, `ResourceProcessInputProvider::Consume`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.processes`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp:98`
- `framework.resources`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp:102`
- `framework.simulation`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp:106`
- `gameplay.integration.resource_payload_invalid`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:300`
- `gameplay.integration.resource_stockpile_missing`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:168`
- `gameplay.integration.resource_token_allocation`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:204`
- `framework.process.input`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:208`
- `framework.process.resource.output.prepared`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:236`
- `gameplay.integration.process_region_missing`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:337`
- `gameplay.integration.process_prepared_token_invalid`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:346`
- `gameplay.integration.process_missing`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:350`
- `gameplay.integration.process_revision_mismatch`: `EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:352`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.processes` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp:98`)

```cpp
96 std::unique_ptr<ISaveParticipant> CreateProcessesServiceSaveParticipant(epidemic::gameplay::processes::ProcessesService& owner, SaveRestoreWorkspace& workspace)
   97 {
   98     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::processes::ProcessesService>>(owner, workspace, "framework.processes", std::vector<SaveParticipantId>{});
   99 }
  100 std::unique_ptr<ISaveParticipant> CreateResourcesProductionServiceSaveParticipant(epidemic::gameplay::resources::ResourcesProductionService& owner, SaveRestoreWorkspace& worksp…
```

#### `framework.resources` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp:102`)

```cpp
100 std::unique_ptr<ISaveParticipant> CreateResourcesProductionServiceSaveParticipant(epidemic::gameplay::resources::ResourcesProductionService& owner, SaveRestoreWorkspace& workspace)
  101 {
  102     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::resources::ResourcesProductionService>>(owner, workspace, "framework.resources", std::vector<SaveParticipantId>{});
  103 }
  104 std::unique_ptr<ISaveParticipant> CreateSimulationServiceSaveParticipant(epidemic::gameplay::simulation::SimulationService& owner, SaveRestoreWor…
```

#### `framework.simulation` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/save_participants.cpp:106`)

```cpp
104 std::unique_ptr<ISaveParticipant> CreateSimulationServiceSaveParticipant(epidemic::gameplay::simulation::SimulationService& owner, SaveRestoreWorkspace& workspace)
  105 {
  106     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::simulation::SimulationService>>(owner, workspace, "framework.simulation", std::vector<SaveParticipantId>{});
  107 }
  108 }
```

#### `gameplay.integration.resource_payload_invalid` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:300`)

```cpp
298     if (!payload)
  299         return foundation::Result<void>::Failure(
  300             Error("gameplay.integration.resource_payload_invalid", "prepared process resource output token is invalid"));
  301     return resources_.Add(payload->stockpile, {payload->resource, payload->amount}, context);
  302 }
```

#### `gameplay.integration.resource_stockpile_missing` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:168`)

```cpp
166     if (!stockpile)
  167         return foundation::Result<void>::Failure(
  168             Error("gameplay.integration.resource_stockpile_missing", "process resource stockpile is missing"));
  169     if (!payload->resource.IsValid())
  170         return foundation::Result<void>::Failure(
```

#### `gameplay.integration.resource_token_allocation` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:204`)

```cpp
202     {
  203         return foundation::Result<processes::ReservedProcessInput>::Failure(
  204             Error("gameplay.integration.resource_token_allocation", "failed to stage resource reservation token"));
  205     }
  206
```

#### `framework.process.input` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:208`)

```cpp
206 
  207     auto resource_reservation = resources_.Reserve(payload->stockpile, std::move(required), request.actor,
  208                                                    TypeId::FromString("framework.process.input"), request.context);
  209     if (!resource_reservation)
  210         return foundation::Result<processes::ReservedProcessInput>::Failure(resource_reservation.GetError());
```

#### `framework.process.resource.output.prepared` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:236`)

```cpp
234 [[nodiscard]] TypeId PreparedOutputPayloadType() noexcept
  235 {
  236     return TypeId::FromString("framework.process.resource.output.prepared");
  237 }
  238
```

#### `gameplay.integration.process_region_missing` (`EngineFramework/IntegrationLayer/ProcessResourceSimulationIntegration/src/process_resource_simulation_adapters.cpp:337`)

```cpp
335         return foundation::Result<void>::Success();
  336     if (!task.area.IsValid())
  337         return foundation::Result<void>::Failure(Error("gameplay.integration.process_region_missing", "simulation task area is required to commit processes"));
  338     if (summary.prepared_operations.size() != summary.operations)
  339         return foundation::Result<void>::Failure(Error("gameplay.integration.process_prepared_token_invalid", "process layer summary has an invalid prepared operation token"));
```
