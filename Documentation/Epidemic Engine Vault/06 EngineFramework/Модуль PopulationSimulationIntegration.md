---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "population_simulation_integration"]
source_path: "EngineFramework/IntegrationLayer/PopulationSimulationIntegration"
---

# EngineFramework / PopulationSimulationIntegration

Модуль: **PopulationSimulationIntegration**. Связь популяции с симуляцией.

**Реализация:** `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/`.
**CMake targets:** `EpidemicGameFrameworkPopulationSimulationIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration`. Admission: 14 public API, 28 mutation obligations, 0 lifecycle, 7 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/PopulationSimulationIntegration/population_simulation_adapters.h|population_simulation_adapters.h]]: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h`; объявленные типы: `PopulationEncounterPlanState`, `PopulationBackedEncounterSlot`, `PopulationBackedEncounterPlan`, `PopulationEncounterAdapterSnapshot`, `PopulationBackedSpawnResult`, `PopulationEncounterAdapter`, `RolesNeedsAdapter`, `PopulationLifecycleReconciliation`, `PopulationLifecycleAdapterSnapshot`, `PopulationLifecycleAdapter`, `CityMorningResult`, `CityLifeAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/PopulationSimulationIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

- Population backed encounter planning.
- Spawn success/failure.
- Encounter termination updates population once.
- Roles -> Needs mapping.
- Population lifecycle reconciliation.
- City life scheduled update.
- Snapshot restore plans/checkpoints.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/population_simulation_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkPopulationSimulationIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/PopulationSimulationIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PopulationEncounterPlanState` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h`): `AwaitingEntityBindings`, `RollbackPending`, `Completed`, `Failed`.

### Операции и хранилища состояния

#### `PopulationEncounterAdapter`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:56`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<PopulationBackedSpawnResult> SpawnFromPopulation( population::PopulationService& population_service, encounters::EncountersService& encounter_service, population::PopulationGroupId group, encounters::SpawnRequest request, std::size_t max_units);
[[nodiscard]] const PopulationBackedEncounterPlan* FindPlan(encounters::SpawnRequestId request) const noexcept;
[[nodiscard]] PopulationEncounterAdapterSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot( PopulationEncounterAdapterSnapshot snapshot, const population::PopulationService& population_service, const encounters::EncountersService& encounter_service);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<PopulationBackedEncounterPlan> plans_`.

#### `PopulationLifecycleAdapter`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:129`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<void> MarkResidentDead( population::PopulationService& population_service, roles_jobs::RolesJobsService& roles_service, needs_life::NeedsLifeService& needs_service, population::PopulationUnitId unit, GameplayContext context);
[[nodiscard]] PopulationLifecycleAdapterSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(PopulationLifecycleAdapterSnapshot snapshot);
void PruneCompleted();
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<PopulationLifecycleReconciliation> reconciliations_`.

#### `RolesNeedsAdapter`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:94`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<void> CreateWorkPressureForDuty( const roles_jobs::Duty& duty, needs_life::NeedsLifeService& needs_service, std::int64_t urgency, GameplayContext context) const;
[[nodiscard]] foundation::Result<void> SatisfyWorkNeedFromDuty( const roles_jobs::Duty& duty, needs_life::NeedsLifeService& needs_service, needs_life::NeedTypeId work_need, std::int64_t amount, GameplayContext context) const;
```

#### `PopulationLifecycleReconciliation`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:111`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] bool Complete() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool roles_cleaned = false`; `bool needs_cleaned = false`.

#### `CityLifeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:158`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] CityMorningResult RunMorningStep( population::PopulationService& population_service, roles_jobs::RolesJobsService& roles_service, needs_life::NeedsLifeService& needs_service, GameplayObjectRef area, GameplayTimePoint now, std::size_t materialization_limit, GameplayContext context) const;
```

#### `PopulationBackedEncounterPlan`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:33`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `PopulationEncounterPlanState state = PopulationEncounterPlanState::AwaitingEntityBindings`; `std::vector<PopulationBackedEncounterSlot> slots`.

#### `PopulationEncounterAdapterSnapshot`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<PopulationBackedEncounterPlan> plans`.

#### `PopulationBackedSpawnResult`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:50`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `encounters::SpawnResult spawn`; `std::vector<population::PopulationUnitId> allocated_units`.

#### `PopulationLifecycleAdapterSnapshot`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:124`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<PopulationLifecycleReconciliation> reconciliations`.

#### `CityMorningResult`

Объявлен в `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h:151`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<roles_jobs::DutyId> activated_duties`; `std::vector<population::PopulationUnit> materialization_candidates`; `std::vector<needs_life::NeedState> critical_needs`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkPopulation`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp` (686 строк): `Error::Create`, `TypeId::FromString`, `PopulationEncounterAdapter::FindMutablePlan`, `PopulationEncounterAdapter::FindPlan`, `PopulationEncounterAdapter::SpawnFromPopulation`, `PopulationService::ResidentRef`, `PopulationEncounterAdapter::BindSpawnedEntity`, `PopulationEncounterAdapter::CancelPendingSpawn`, `PopulationEncounterAdapter::CaptureSnapshot`, `PopulationEncounterAdapter::RestoreSnapshot`
- `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/save_participants.cpp` (211 строк): `SaveParticipantId::FromString`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `population.allocation.encounter`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:21`
- `life.pressure.work`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:26`
- `gameplay.population_sim.invalid_population_spawn`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:104`
- `gameplay.population_sim.spawn_request_conflict`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:113`
- `gameplay.population_sim.reconciliation_required`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:382`
- `gameplay.population_sim.empty_population_spawn`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:144`
- `gameplay.population_sim.population_budget_exceeded`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:147`
- `gameplay.population_sim.population_slot_unavailable`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:173`
- `gameplay.population_sim.encounter_spawn_failed`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:251`
- `gameplay.population_sim.spawn_plan_mismatch`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:267`
- `gameplay.population_sim.population_plan_missing`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:290`
- `gameplay.population_sim.population_slot_missing`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:295`
- `gameplay.population_sim.population_slot_conflict`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:298`
- `gameplay.population_sim.spawn_record_mismatch`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:305`
- `gameplay.population_sim.spawn_entity_conflict`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:315`
- `gameplay.population_sim.allocation_mismatch`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:322`
- `gameplay.population_sim.allocation_commit_conflict`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:332`
- `gameplay.population_sim.population_plan_reconciliation_required`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:359`
- `gameplay.population_sim.restore_invalid_plan`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:426`
- `gameplay.population_sim.restore_invalid_slot`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:482`
- `gameplay.population_sim.restore_binding_conflict`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:458`
- `gameplay.population_sim.invalid_duty`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:530`
- `gameplay.population_sim.work_pressure_conflict`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:506`
- `roles_jobs.duty`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:535`
- `gameplay.population_sim.unit_missing`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:558`
- `gameplay.population_sim.death_not_committed`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:571`
- `gameplay.population_sim.restore_invalid_reconciliation`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:649`
- `framework.encounters`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/save_participants.cpp:205`
- `framework.needs_life`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/save_participants.cpp:209`
- `framework.population`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/save_participants.cpp:209`
- `framework.roles_jobs`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/save_participants.cpp:209`
- `integration.population_encounter`: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/save_participants.cpp:205`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `population.allocation.encounter` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:21`)

```cpp
19 [[nodiscard]] constexpr TypeId EncounterAllocationPurpose() noexcept
   20 {
   21     return TypeId::FromString("population.allocation.encounter");
   22 }
   23
```

#### `life.pressure.work` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:26`)

```cpp
24 [[nodiscard]] constexpr TypeId WorkPressureType() noexcept
   25 {
   26     return TypeId::FromString("life.pressure.work");
   27 }
   28
```

#### `gameplay.population_sim.invalid_population_spawn` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:104`)

```cpp
102     if (!group.IsValid() || !pop.GetGroup(group) || !request.encounter.IsValid() || !request.area.IsValid())
  103         return foundation::Result<PopulationBackedSpawnResult>::Failure(
  104             Error("gameplay.population_sim.invalid_population_spawn", "invalid population-backed encounter request"));
  105 
  106     if (request.id.IsValid())
```

#### `gameplay.population_sim.spawn_request_conflict` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:113`)

```cpp
111                 existing->seed != request.seed || existing->state == PopulationEncounterPlanState::Failed)
  112                 return foundation::Result<PopulationBackedSpawnResult>::Failure(
  113                     Error("gameplay.population_sim.spawn_request_conflict", "spawn request id already belongs to another plan"));
  114             if (existing->state == PopulationEncounterPlanState::RollbackPending)
  115                 return foundation::Result<PopulationBackedSpawnResult>::Failure(
```

#### `gameplay.population_sim.reconciliation_required` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:382`)

```cpp
380     if (!rollback_ok)
  381         return foundation::Result<void>::Failure(
  382             foundation::Error::Create("gameplay.population_sim.reconciliation_required",
  383                                       "population-backed encounter rollback is still incomplete",
  384                                       std::to_string(request.value.High()) + ":" +
```

#### `gameplay.population_sim.empty_population_spawn` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:144`)

```cpp
142     if (preview.Value().empty())
  143         return foundation::Result<PopulationBackedSpawnResult>::Failure(
  144             Error("gameplay.population_sim.empty_population_spawn", "population-backed encounter resolved to no spawn slots"));
  145     if (preview.Value().size() > max_units)
  146         return foundation::Result<PopulationBackedSpawnResult>::Failure(
```

#### `gameplay.population_sim.population_budget_exceeded` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:147`)

```cpp
145     if (preview.Value().size() > max_units)
  146         return foundation::Result<PopulationBackedSpawnResult>::Failure(
  147             Error("gameplay.population_sim.population_budget_exceeded", "encounter requires more population units than allowed"));
  148 
  149     auto candidates = pop.FindUnitsByGroup(group);
```

#### `gameplay.population_sim.population_slot_unavailable` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:173`)

```cpp
171         if (!match)
  172             return foundation::Result<PopulationBackedSpawnResult>::Failure(
  173                 Error("gameplay.population_sim.population_slot_unavailable",
  174                       "no unallocated population unit matches an encounter spawn archetype"));
  175         used.insert(match->id);
```

#### `gameplay.population_sim.encounter_spawn_failed` (`EngineFramework/IntegrationLayer/PopulationSimulationIntegration/src/population_simulation_adapters.cpp:251`)

```cpp
249     if (!population_spawn_created)
  250         return fail_with_recovery(
  251             Error("gameplay.population_sim.encounter_spawn_failed", "population-backed encounter spawn failed"));
  252     spawn.state = encounters::SpawnResultState::Succeeded;
  253
```
