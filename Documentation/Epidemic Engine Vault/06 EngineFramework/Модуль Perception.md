---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "perception"]
source_path: "EngineFramework/GameplayWorldStateOwners/Perception"
---

# EngineFramework / Perception

Модуль: **Perception**. Gameplay perception.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Perception/`.
**CMake targets:** `EpidemicGameFrameworkPerception`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Perception/perception.h|perception.h]]: `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`; объявленные типы: `SenseTypeId`, `PerceiverProfileId`, `SenseEvaluatorId`, `PerceptionStimulusId`, `PerceptionObservationId`, `AwarenessStateId`, `IdHash`, `RefHash`, `WorldPosition`, `WorldDirection`, `PerceptionMaterializationPolicy`, `SenseEvaluationModel`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Perception|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility and ownership

Owns perception definitions, perceivers, stimuli, pending/active observations, awareness state, spatial evaluation policy, generators, revisions and the bounded perception change journal.

The service remains the single authoritative owner of that state. Derived indexes, journals and diagnostic/read models are not independent semantic owners.

#### Authoritative state and indexes

Admission/generated B05 inventory: 72 public callables, 56 mutation obligations, 1 lifecycle candidates, 13 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Multi-container mutations must complete fallible staging before authoritative publication. Deterministic query/order semantics must agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B05 coverage projection. Revision, identity-generator and journal publication is part of the mutation contract. Allocation/publication failure may not expose partially advanced primary state, index state, generator, revision or journal.

B05 allocation evidence uses source-private module seams rather than the shared process-global allocator override. The seams are implementation/test details and do not expand the public API.

#### Lifecycle and identity

All emitted lifecycle and stale-identity candidates were reviewed. Invalid, removed, duplicate and stale identities are rejected or handled as the documented no-op without false publication. Retry does not silently resurrect terminal state.

#### Persistence

Snapshot/restore is reviewed as candidate-state publication: validate first, build off-state, preserve the current service on failure, then atomically publish the candidate. Successful restore preserves generator/revision/cursor continuity required by the module snapshot contract.

#### External boundaries

Perceiver sample providers and custom evaluators are read/evaluation ports. Their failure is translated to Result/error semantics before observation/index/revision publication.

#### Threading contract

No additional internal synchronization guarantee is introduced by B05. These gameplay owners are treated as externally serialized/owner-thread services for Goal 4 local correctness. Engine-wide concurrency qualification remains Goal 6.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkPerceptionTests` from `EngineFramework/DevelopmentInfrastructure/Tests/perception_tests.cpp`.

`G4-PER-001` and `G4-PER-003` are covered by `TestSpatialBoundaryMath`; `G4-PER-002` is covered by `TestDecayBoundaryArithmetic`. Exact coordinate deltas are formed before floating conversion, squared range comparison uses portable wide arithmetic, linear distance no longer narrows through signed 64-bit squared distance, and awareness interval advancement uses integer arithmetic.

Источник контрактов: `Documentation/EngineFramework/Perception.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PerceptionMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `AbstractCapable`, `RequiresMaterialized`, `RequiresRuntimeProjection`.
- `SenseEvaluationModel` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `Vision`, `Hearing`, `Radial`, `Custom`.
- `PerceptionVisibilityState` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `Visible`, `PartiallyVisible`, `Hidden`, `Occluded`, `Unknown`, `Unavailable`.
- `PerceptionAudibilityState` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `HeardExactly`, `HeardDirectionOnly`, `HeardVagueNoise`, `NotHeard`, `Unavailable`.
- `PerceptionConfidence` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `None`, `Low`, `Medium`, `High`, `Certain`.
- `AwarenessLevel` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `Unaware`, `Suspicious`, `Aware`, `Lost`, `Confirmed`.
- `PerceptionChangeKind` (`EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h`): `StimulusCreated`, `StimulusExpired`, `Observed`, `Lost`, `AwarenessChanged`, `PerceiverRegistered`, `PerceiverUnregistered`, `BudgetExceeded`.

### Операции и хранилища состояния

#### `PerceptionService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:413`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
PerceptionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterSense(SenseDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterProfileDefinition(PerceiverProfileDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterEvaluator(SenseEvaluatorId id, std::shared_ptr<const ISenseEvaluator> evaluator);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const SenseDefinition *FindSense(SenseTypeId id) const noexcept;
[[nodiscard]] const PerceiverProfileDefinition *FindProfileDefinition(PerceiverProfileId id) const noexcept;
[[nodiscard]] const PerceiverRecord *FindPerceiver(GameplayObjectRef subject) const noexcept;
[[nodiscard]] foundation::Result<PerceptionStimulusId> CreateStimulus(PerceptionStimulus stimulus);
[[nodiscard]] foundation::Result<std::vector<PerceptionObservation>> ProcessStimulus( PerceptionStimulusId stimulus, const PerceptionProcessingContext &context);
[[nodiscard]] VisibilityResult EvaluateVisibility(GameplayObjectRef perceiver, GameplayObjectRef target, const PerceiverEvaluationSample &sample, WorldPosition target_position, Fixed stimulus_strength_micro = 1'000'000) const;
[[nodiscard]] AudibilityResult EvaluateAudibility(GameplayObjectRef perceiver, PerceptionStimulusId stimulus, const PerceiverEvaluationSample &sample) const;
void SetBudget(PerceptionBudget budget) noexcept;
void SetTemporalPolicy(PerceptionTemporalPolicy policy) noexcept;
void SetChangeJournalCapacity(std::size_t capacity) noexcept;
[[nodiscard]] const PerceptionStimulus *FindStimulus(PerceptionStimulusId id) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<SenseTypeId, SenseDefinition, IdHash> senses_`; `std::unordered_map<PerceiverProfileId, PerceiverProfileDefinition, IdHash> profile_definitions_`; `std::unordered_map<SenseEvaluatorId, std::shared_ptr<const ISenseEvaluator>, IdHash> evaluators_`; `std::unordered_map<GameplayObjectRef, PerceiverRecord, RefHash> perceivers_`; `std::unordered_map<PerceptionStimulusId, PerceptionStimulus, IdHash> stimuli_`; `std::unordered_map<GameplayObjectRef, std::vector<PerceptionStimulusId>, RefHash> stimuli_by_area_`; `std::unordered_map<PerceptionObservationId, PendingPerceptionObservation, IdHash> pending_observations_`.

#### `ISenseEvaluator`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:311`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISenseEvaluator() = default;
[[nodiscard]] virtual foundation::Result<SenseEvaluationResult> Evaluate(const SenseEvaluationInput &input) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:80`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `PerceptionStimulusId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:44`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PerceptionStimulusId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PerceptionObservationId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:56`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PerceptionObservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PerceptionProcessingContext`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:280`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
PerceptionProcessingContext() = default;
) : tick(t), now(n), gameplay(g), perceivers(std::move(samples));
[[nodiscard]] const PerceiverEvaluationSample *FindPerceiver(GameplayObjectRef subject) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<PerceiverEvaluationSample> perceivers`.

#### `WorldPosition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Perception/include/Epidemic/GameFramework/Perception/perception.h:93`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed x_mm = 0`; `Fixed y_mm = 0`; `Fixed z_mm = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Perception/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp` (1572 строк): `Error::Create`, `PerceptionService::PerceptionService`, `PerceptionService::CanAdvanceRevision`, `PerceptionService::CanRecordChanges`, `PerceptionService::RegisterSense`, `PerceptionService::RegisterProfileDefinition`, `PerceptionService::RegisterEvaluator`, `PerceptionService::FindSense`, `PerceptionService::FindProfileDefinition`, `PerceptionService::FindPerceiver`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.perception.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:252`
- `gameplay.perception.invalid_sense`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:209`
- `gameplay.perception.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:226`
- `gameplay.perception.unknown_sense`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:235`
- `gameplay.perception.ambiguous_profile`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:241`
- `gameplay.perception.invalid_evaluator`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:255`
- `gameplay.perception.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1399`
- `gameplay.perception.invalid_perceiver`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:284`
- `gameplay.perception.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1124`
- `gameplay.perception.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1126`
- `gameplay.perception.publication_failed`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:373`
- `gameplay.perception.perceiver_missing`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:309`
- `gameplay.perception.invalid_stimulus`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:354`
- `gameplay.perception.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:867`
- `gameplay.perception.duplicate_stimulus`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:363`
- `gameplay.perception.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1488`
- `gameplay.perception.sample_unavailable`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:646`
- `gameplay.perception.materialization_unavailable`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:632`
- `gameplay.perception.runtime_projection_unavailable`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:638`
- `gameplay.perception.evaluator_missing`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:808`
- `gameplay.perception.invalid_evaluator_result`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:672`
- `gameplay.perception.evaluator_exception`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:686`
- `gameplay.perception.invalid_processing_sample`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:793`
- `gameplay.perception.duplicate_processing_sample`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:795`
- `gameplay.perception.stimulus_missing`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:799`
- `gameplay.perception.invalid_sense_link`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:801`
- `gameplay.perception.invalid_profile_link`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:828`
- `gameplay.perception.duplicate_observation`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:943`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1395`
- `gameplay.perception.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1518`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/perception_tests.cpp`: `TestSpatialBoundaryMath`, `TestDecayBoundaryArithmetic`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.perception.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:252`)

```cpp
250     if (frozen_)
  251         return foundation::Result<void>::Failure(
  252             Error("gameplay.perception.registry_frozen", "perception registry frozen"));
  253     if (!id.IsValid() || !evaluator || evaluators_.contains(id))
  254         return foundation::Result<void>::Failure(
```

#### `gameplay.perception.invalid_sense` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:209`)

```cpp
207         (!d.evaluator.IsValid() || !evaluators_.contains(d.evaluator)))
  208         return foundation::Result<void>::Failure(
  209             Error("gameplay.perception.invalid_sense", "custom sense requires a registered evaluator"));
  210     senses_.emplace(d.id, std::move(d));
  211     return foundation::Result<void>::Success();
```

#### `gameplay.perception.invalid_profile` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:226`)

```cpp
224     if (std::adjacent_find(p.senses.begin(), p.senses.end()) != p.senses.end())
  225         return foundation::Result<void>::Failure(
  226             Error("gameplay.perception.invalid_profile", "duplicate sense in profile"));
  227 
  228     std::size_t vision_count = 0;
```

#### `gameplay.perception.unknown_sense` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:235`)

```cpp
233         if (it == senses_.end())
  234             return foundation::Result<void>::Failure(
  235                 Error("gameplay.perception.unknown_sense", "profile references unknown sense"));
  236         vision_count += it->second.evaluation_model == SenseEvaluationModel::Vision ? 1u : 0u;
  237         hearing_count += it->second.evaluation_model == SenseEvaluationModel::Hearing ? 1u : 0u;
```

#### `gameplay.perception.ambiguous_profile` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:241`)

```cpp
239     if (vision_count > 1 || hearing_count > 1)
  240         return foundation::Result<void>::Failure(
  241             Error("gameplay.perception.ambiguous_profile", "profile has multiple primary built-in senses"));
  242 
  243     profile_definitions_.emplace(p.id, std::move(p));
```

#### `gameplay.perception.invalid_evaluator` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:255`)

```cpp
253     if (!id.IsValid() || !evaluator || evaluators_.contains(id))
  254         return foundation::Result<void>::Failure(
  255             Error("gameplay.perception.invalid_evaluator", "invalid or duplicate sense evaluator"));
  256     evaluators_.emplace(id, std::move(evaluator));
  257     return foundation::Result<void>::Success();
```

#### `gameplay.perception.registry_not_frozen` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1399`)

```cpp
1397     if (!frozen_)
 1398         return foundation::Result<void>::Failure(
 1399             Error("gameplay.perception.registry_not_frozen", "perception definitions must be frozen before restore"));
 1400 
 1401     decltype(perceivers_) new_perceivers;
```

#### `gameplay.perception.invalid_perceiver` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:284`)

```cpp
282         return foundation::Result<void>::Failure(Error("gameplay.perception.registry_not_frozen", "perception definitions must be frozen"));
  283     if (!subject.IsValid() || !profile.IsValid() || !profile_definitions_.contains(profile) || perceivers_.contains(subject))
  284         return foundation::Result<void>::Failure(Error("gameplay.perception.invalid_perceiver", "invalid, duplicate or unknown perceiver"));
  285     if (!CanAdvanceRevision())
  286         return foundation::Result<void>::Failure(Error("gameplay.perception.revisio…
```

#### `gameplay.perception.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Perception/src/perception.cpp:1124`)

```cpp
1122         return foundation::Result<void>::Success();
 1123     if (!CanAdvanceRevision(updates.size()))
 1124         return foundation::Result<void>::Failure(Error("gameplay.perception.revision_exhausted", "perception revision is exhausted"));
 1125     if (!CanRecordChanges(updates.size()))
 1126         return foundation::Result<void>::Failure(Error("gameplay.perception.change_sequence_exhausted", "perception change sequence is exhausted"));
```
