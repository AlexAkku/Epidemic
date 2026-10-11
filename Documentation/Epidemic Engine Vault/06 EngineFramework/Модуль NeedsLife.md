---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "needs_life"]
source_path: "EngineFramework/GameplayWorldStateOwners/NeedsLife"
---

# EngineFramework / NeedsLife

Модуль: **NeedsLife**. Нужды и жизненные потребности.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/NeedsLife/`.
**CMake targets:** `EpidemicGameFrameworkNeedsLife`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/NeedsLife/needs_life.h|needs_life.h]]: `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h`; объявленные типы: `NeedTypeId`, `NeedProfileId`, `LifeRoutineId`, `LifePressureId`, `NeedDecayRuleId`, `LifeSimulationProfileId`, `IdHash`, `NeedStateKey`, `NeedStateKeyHash`, `NeedThreshold`, `NeedMaterializationPolicy`, `LifePressureState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/NeedsLife|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Scope

Freeze unit: `EngineFramework/GameplayWorldStateOwners/NeedsLife`. Admission: 65 public callables, 72 mutation obligations, 1 lifecycle candidates, 17 stale-identity candidates, 0 external-boundary candidates.

#### Public contract review

- Need definitions, profiles and committed need state.
- Satisfy/decay numeric boundaries and saturating arithmetic.
- Life pressure create/resolve/expire/prune lifecycle.
- Large simulation intervals and terminal pressure pruning.
- Revision/generator/journal exhaustion and no-false-publish semantics.
- Snapshot/restore validates first and preserves live state on failure.

#### Failure atomicity and boundary review

All revision-bearing mutations were reviewed for revision/generator exhaustion before authoritative mutation. Multi-container mutations were reviewed for rollback or staged publication. Snapshot restore builds/validates candidate state before live-state replacement. Allocation-failure evidence for this block no longer depends on the process-global Framework allocator helper; owned tests use module-local failure seams where allocation failure is evidence.

#### Persistence and deterministic reads

The module snapshot surface is treated as local in-memory persistence evidence for Goal 4. Non-empty roundtrip, invalid restore, generator/revision continuity, journal epoch/cursor behavior and failed-restore pre-state preservation are covered by the module test executable. Whole-engine ordered restore and replay remain Goal 5.

#### Test evidence

Primary regression source: `EngineFramework/DevelopmentInfrastructure/Tests/needs_life_tests.cpp`.
Registered target: `EpidemicGameFrameworkNeedsLifeTests`.
Local Linux verification in the worker environment used direct C++23 compilation with `-Wall -Wextra -Wpedantic -Werror` and executed the module test binary. The repository top-level CMake configure is Windows-only and therefore cannot be used in this Linux worker environment.

#### Goal 4 result

Block-local review result: LOCAL_READY candidate for serial integration. This document does not claim whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN`.

Источник контрактов: `Documentation/EngineFramework/NeedsLife.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `NeedThreshold` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h`): `Satisfied`, `Low`, `Medium`, `High`, `Critical`.
- `NeedMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h`): `AbstractCapable`, `MaterializedOnly`, `DisabledWhenAbstract`.
- `LifePressureState` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h`): `Active`, `Resolved`, `Expired`.
- `NeedsLifeChangeKind` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h`): `NeedProfileCreated`, `NeedProfileChanged`, `NeedChanged`, `NeedThresholdCrossed`, `NeedSatisfied`, `NeedPressureAdded`, `LifePressureCreated`, `LifePressureChanged`, `LifePressureResolved`, `LifePressureExpired`, `RoutineChanged`, `LifeSimulationStepCompleted`.

### Операции и хранилища состояния

#### `NeedsLifeService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:268`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterDecayRule(NeedDecayRule rule);
[[nodiscard]] foundation::Result<void> RegisterSimulationProfile(LifeSimulationProfile profile);
[[nodiscard]] foundation::Result<void> RegisterNeedDefinition(NeedDefinition definition);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] NeedState EvaluateNeed(GameplayObjectRef subject, NeedTypeId need, GameplayTimePoint now) const;
[[nodiscard]] foundation::Result<void> SatisfyNeed(SatisfyNeedRequest request);
[[nodiscard]] foundation::Result<void> AddNeedPressure(AddNeedPressureRequest request);
[[nodiscard]] std::vector<LifeRoutineOccurrence> FindRoutineEntriesInInterval(GameplayObjectRef subject, GameplayTimePoint from, GameplayTimePoint to) const;
[[nodiscard]] const NeedProfile *GetNeedProfile(GameplayObjectRef subject) const noexcept;
[[nodiscard]] const NeedState *GetCommittedNeedState(GameplayObjectRef subject, NeedTypeId need) const noexcept;
[[nodiscard]] std::int64_t GetNeedUrgency(GameplayObjectRef subject, NeedTypeId need, GameplayTimePoint now) const;
[[nodiscard]] std::vector<NeedState> FindCriticalNeeds(GameplayTimePoint now) const;
[[nodiscard]] std::vector<NeedState> FindCriticalNeeds(GameplayObjectRef subject, GameplayTimePoint now) const;
[[nodiscard]] std::vector<LifePressure> FindLifePressures(GameplayObjectRef subject) const;
[[nodiscard]] const LifeRoutine *GetRoutine(GameplayObjectRef subject) const noexcept;
[[nodiscard]] NeedsLifeChangeBatch ReadChangesSince(ChangeCursor cursor) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool definitions_frozen_ = false`; `std::unordered_map<NeedDecayRuleId, NeedDecayRule, IdHash> decay_rules_`; `std::unordered_map<LifeSimulationProfileId, LifeSimulationProfile, IdHash> simulation_profiles_`; `std::unordered_map<NeedTypeId, NeedDefinition, IdHash> definitions_`; `std::unordered_map<NeedProfileId, NeedProfile, IdHash> profiles_`; `std::unordered_map<GameplayObjectRef, NeedProfileId> profile_by_subject_`; `std::unordered_map<NeedStateKey, NeedState, NeedStateKeyHash> states_`; `std::unordered_map<LifePressureId, LifePressure, IdHash> pressures_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:68`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `NeedProfileId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:25`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NeedProfileId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `LifeRoutineId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:34`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr LifeRoutineId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `LifePressureId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:43`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr LifePressureId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NeedStateKeyHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:81`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const NeedStateKey &key) const noexcept;
```

#### `LifeRoutineOccurrence`

Объявлен в `EngineFramework/GameplayWorldStateOwners/NeedsLife/include/Epidemic/GameFramework/NeedsLife/needs_life.h:213`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t entry_index = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/NeedsLife/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp` (1012 строк): `Error::Create`, `NeedsLifeService::SaturatingAdd64`, `NeedsLifeService::SaturatingMultiply64`, `NeedsLifeService::ScaleMicro`, `NeedsLifeService::AdvanceRevision`, `NeedsLifeService::RegisterDecayRule`, `NeedsLifeService::RegisterSimulationProfile`, `NeedsLifeService::ValidateDefinition`, `NeedsLifeService::RegisterNeedDefinition`, `NeedsLifeService::FreezeDefinitions`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.needs.definitions_frozen`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:162`
- `gameplay.needs.invalid_decay_rule`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:112`
- `gameplay.needs.duplicate_decay_rule`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:114`
- `gameplay.needs.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:589`
- `gameplay.needs.invalid_simulation_profile`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:131`
- `gameplay.needs.duplicate_simulation_profile`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:134`
- `gameplay.needs.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:147`
- `gameplay.needs.unknown_decay_rule`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:149`
- `gameplay.needs.invalid_thresholds`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:155`
- `gameplay.needs.duplicate_definition`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:167`
- `gameplay.needs.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:195`
- `gameplay.needs.unknown_simulation_profile`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:557`
- `gameplay.needs.unknown_need`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:203`
- `gameplay.needs.duplicate_need`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:205`
- `gameplay.needs.profile_exists`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:241`
- `gameplay.needs.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:470`
- `gameplay.needs.duplicate_profile`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:223`
- `gameplay.needs.state_exists`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:237`
- `gameplay.needs.profile_missing`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:539`
- `gameplay.needs.invariant`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:542`
- `gameplay.needs.state_missing`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:578`
- `gameplay.needs.time_regression`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:382`
- `gameplay.needs.invalid_satisfy`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:349`
- `gameplay.needs.invalid_pressure_delta`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:377`
- `gameplay.needs.invalid_pressure`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:406`
- `gameplay.needs.duplicate_pressure`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:411`
- `gameplay.needs.pressure_missing`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:424`
- `gameplay.needs.invalid_routine`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:463`
- `gameplay.needs.routine_exists`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:468`
- `gameplay.needs.invalid_simulation`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:536`
- `gameplay.needs.interval_conflict`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:582`
- `gameplay.needs.materialization_required`: `EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:549`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.needs.definitions_frozen` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:162`)

```cpp
160 {
  161     if (definitions_frozen_)
  162         return foundation::Result<void>::Failure(Error("gameplay.needs.definitions_frozen", "definitions are frozen"));
  163     if (auto valid = ValidateDefinition(d); !valid)
  164         return valid;
```

#### `gameplay.needs.invalid_decay_rule` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:112`)

```cpp
110         return foundation::Result<void>::Failure(Error("gameplay.needs.definitions_frozen", "definitions are frozen"));
  111     if (!rule.id.IsValid() || rule.rate_multiplier_micro < 0)
  112         return foundation::Result<void>::Failure(Error("gameplay.needs.invalid_decay_rule", "invalid decay rule"));
  113     if (decay_rules_.contains(rule.id))
  114         return foundation::Result<void>::Failure(Error("gameplay.needs.duplicate_decay_rule", "duplicate decay rule"));
```

#### `gameplay.needs.duplicate_decay_rule` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:114`)

```cpp
112         return foundation::Result<void>::Failure(Error("gameplay.needs.invalid_decay_rule", "invalid decay rule"));
  113     if (decay_rules_.contains(rule.id))
  114         return foundation::Result<void>::Failure(Error("gameplay.needs.duplicate_decay_rule", "duplicate decay rule"));
  115     const auto next_revision = CheckedNext(revision_);
  116     if (!next_revision)
```

#### `gameplay.needs.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:589`)

```cpp
587 
  588     if (!AdvanceRevision())
  589         return foundation::Result<void>::Failure(Error("gameplay.needs.revision_exhausted", "revision exhausted"));
  590     auto event_context = r.context;
  591     event_context.time = r.to;
```

#### `gameplay.needs.invalid_simulation_profile` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:131`)

```cpp
129         profile.materialized_rate_multiplier_micro < 0)
  130         return foundation::Result<void>::Failure(
  131             Error("gameplay.needs.invalid_simulation_profile", "invalid simulation profile"));
  132     if (simulation_profiles_.contains(profile.id))
  133         return foundation::Result<void>::Failure(
```

#### `gameplay.needs.duplicate_simulation_profile` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:134`)

```cpp
132     if (simulation_profiles_.contains(profile.id))
  133         return foundation::Result<void>::Failure(
  134             Error("gameplay.needs.duplicate_simulation_profile", "duplicate simulation profile"));
  135     const auto next_revision = CheckedNext(revision_);
  136     if (!next_revision)
```

#### `gameplay.needs.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:147`)

```cpp
145 {
  146     if (!d.id.IsValid() || d.min_value > d.max_value || d.decay_per_tick < 0)
  147         return foundation::Result<void>::Failure(Error("gameplay.needs.invalid_definition", "invalid need definition"));
  148     if (d.decay_rule.IsValid() && !decay_rules_.contains(d.decay_rule))
  149         return foundation::Result<void>::Failure(Error("gameplay.needs.unknown_decay_rule", "unknown decay rule"));
```

#### `gameplay.needs.unknown_decay_rule` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:149`)

```cpp
147         return foundation::Result<void>::Failure(Error("gameplay.needs.invalid_definition", "invalid need definition"));
  148     if (d.decay_rule.IsValid() && !decay_rules_.contains(d.decay_rule))
  149         return foundation::Result<void>::Failure(Error("gameplay.needs.unknown_decay_rule", "unknown decay rule"));
  150     if (!ValidMicro(d.satisfied_threshold_micro) || !ValidMicro(d.low_threshold_micro) ||
  151         !ValidMicro(d.medium_threshold_micro) || !ValidMicro(d.high_threshold_micro) ||
```

#### `gameplay.needs.invalid_thresholds` (`EngineFramework/GameplayWorldStateOwners/NeedsLife/src/needs_life.cpp:155`)

```cpp
153         d.medium_threshold_micro < d.high_threshold_micro)
  154         return foundation::Result<void>::Failure(
  155             Error("gameplay.needs.invalid_thresholds", "invalid need threshold policy"));
  156     return foundation::Result<void>::Success();
  157 }
```
