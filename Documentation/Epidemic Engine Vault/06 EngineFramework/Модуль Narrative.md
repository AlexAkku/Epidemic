---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "narrative"]
source_path: "EngineFramework/GameplayWorldStateOwners/Narrative"
---

# EngineFramework / Narrative

Модуль: **Narrative**. Повествовательные структуры и состояния.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Narrative/`.
**CMake targets:** `EpidemicGameFrameworkNarrative`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Narrative/narrative.h|narrative.h]]: `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`; объявленные типы: `NarrativeThreadId`, `NarrativeArcId`, `NarrativeBeatId`, `NarrativeObjectiveId`, `NarrativeConditionId`, `NarrativeConsequenceId`, `NarrativeConsequenceExecutionId`, `JournalEntryId`, `ClueId`, `RumorId`, `StoryletId`, `NarrativeGateId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/narrative|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `NarrativeRuntimeState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Hidden`, `Available`, `Discovered`, `Active`, `Suspended`, `Completed`, `Failed`, `Expired`, `Locked`.
- `NarrativeObjectiveRuntimeState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Hidden`, `Available`, `Active`, `Completed`, `Failed`, `Cancelled`, `OptionalMissed`.
- `ConditionEvaluationState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Satisfied`, `Unsatisfied`, `Unknown`, `Unavailable`, `Partial`.
- `ConsequenceExecutionState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Pending`, `Applied`, `Deferred`, `FailedRetryable`, `FailedPermanent`, `Unsupported`, `AlreadyApplied`.
- `JournalVisibilityState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Hidden`, `Discovered`, `Updated`, `Resolved`, `Archived`, `Suppressed`.
- `RumorState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Active`, `Spread`, `Discredited`, `Expired`, `Confirmed`, `Suppressed`.
- `NarrativeChoiceState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Open`, `Resolved`, `Cancelled`, `Expired`.
- `NarrativeEventExecutionState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Pending`, `Processing`, `Completed`, `FailedRetryable`.
- `NarrativeEventExecutionPhase` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Beats`, `Objectives`, `Storylets`, `Completed`.
- `NarrativeGateState` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `Closed`, `Open`, `Locked`, `Disabled`.
- `NarrativeChangeKind` (`EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`): `ThreadStarted`, `ThreadCompleted`, `ThreadFailed`, `ThreadSuspended`, `ThreadResumed`, `ThreadExpired`, `ObjectiveActivated`, `ObjectiveCompleted`, `ObjectiveFailed`, `ObjectiveCancelled`, `ObjectiveOptionalMissed`, `ObjectiveProgressChanged`, `JournalEntryAdded`, `ClueDiscovered` и другие значения.

### Операции и хранилища состояния

#### `NarrativeService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:931`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterThreadDefinition(NarrativeThreadDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterArcDefinition(NarrativeArcDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterBeatDefinition(NarrativeBeatDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterObjectiveDefinition(NarrativeObjectiveDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterConditionDefinition(NarrativeConditionDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterConsequenceDefinition(NarrativeConsequenceDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterStoryletDefinition(StoryletDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterConditionResolver(NarrativeConditionTypeId type, const INarrativeConditionResolver &resolver);
[[nodiscard]] foundation::Result<void> RegisterConsequenceHandler(NarrativeConsequenceTypeId type, const INarrativeConsequenceHandler &handler);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] NarrativeConditionResult EvaluateCondition(NarrativeConditionId condition, NarrativeEvaluationContext context) const;
[[nodiscard]] std::vector<NarrativeConsequenceExecutionId> ExecutePendingConsequences( NarrativeExecutionContext context, std::size_t budget = 256);
[[nodiscard]] const NarrativeThreadState *GetThreadState(NarrativeThreadId thread) const noexcept;
[[nodiscard]] const NarrativeObjectiveState *GetObjectiveState(NarrativeObjectiveId objective) const noexcept;
[[nodiscard]] const JournalEntry *GetJournalEntry(JournalEntryId id) const noexcept;
[[nodiscard]] const ClueRecord *GetClue(ClueId id) const noexcept;
[[nodiscard]] const RumorRecord *GetRumor(RumorId id) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<NarrativeThreadId, NarrativeThreadDefinition, IdHash> thread_defs_`; `std::unordered_map<NarrativeArcId, NarrativeArcDefinition, IdHash> arc_defs_`; `std::unordered_map<NarrativeBeatId, NarrativeBeatDefinition, IdHash> beat_defs_`; `std::unordered_map<NarrativeObjectiveId, NarrativeObjectiveDefinition, IdHash> objective_defs_`; `std::unordered_map<NarrativeConditionId, NarrativeConditionDefinition, IdHash> condition_defs_`; `std::unordered_map<NarrativeConsequenceId, NarrativeConsequenceDefinition, IdHash> consequence_defs_`; `std::unordered_map<StoryletId, StoryletDefinition, IdHash> storylet_defs_`.

#### `INarrativeConditionResolver`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:908`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INarrativeConditionResolver() = default;
[[nodiscard]] virtual NarrativeConditionResult Evaluate(const NarrativeConditionDefinition &definition, const NarrativeEvaluationContext &context) const = 0;
```

#### `INarrativeConsequenceHandler`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:915`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~INarrativeConsequenceHandler() = default;
[[nodiscard]] virtual NarrativeConsequenceResult Execute(const NarrativeConsequenceDefinition &definition, const NarrativeConsequenceExecution &execution, const NarrativeExecutionContext &context) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:396`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `StoryletRuntimeState`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:840`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t activations = 0`.

#### `NarrativeThreadId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeThreadId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NarrativeArcId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:38`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeArcId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NarrativeBeatId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:56`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeBeatId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NarrativeObjectiveId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:74`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeObjectiveId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NarrativeConditionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:92`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeConditionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NarrativeConsequenceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:110`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeConsequenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `NarrativeConsequenceExecutionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h:128`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr NarrativeConsequenceExecutionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Narrative/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp` (2744 строк): `Error::Create`, `NarrativeService::PrepareRevision`, `NarrativeService::CanAdvanceRevisionBy`, `NarrativeService::RegisterThreadDefinition`, `NarrativeService::RegisterArcDefinition`, `NarrativeService::RegisterBeatDefinition`, `NarrativeService::RegisterObjectiveDefinition`, `NarrativeService::RegisterConditionDefinition`, `NarrativeService::RegisterConsequenceDefinition`, `NarrativeService::RegisterStoryletDefinition`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.narrative.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:1780`
- `gameplay.narrative.frozen`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:236`
- `gameplay.narrative.invalid_thread`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:136`
- `gameplay.narrative.duplicate_thread`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:138`
- `gameplay.narrative.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:2670`
- `gameplay.narrative.invalid_arc`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:153`
- `gameplay.narrative.duplicate_arc`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:155`
- `gameplay.narrative.invalid_beat`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:170`
- `gameplay.narrative.duplicate_beat`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:172`
- `gameplay.narrative.invalid_objective`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:187`
- `gameplay.narrative.duplicate_objective`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:189`
- `gameplay.narrative.invalid_condition`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:204`
- `gameplay.narrative.duplicate_condition`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:206`
- `gameplay.narrative.invalid_consequence`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:221`
- `gameplay.narrative.duplicate_consequence`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:223`
- `gameplay.narrative.invalid_storylet`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:238`
- `gameplay.narrative.duplicate_storylet`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:240`
- `gameplay.narrative.invalid_resolver`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:266`
- `gameplay.narrative.duplicate_resolver`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:269`
- `gameplay.narrative.invalid_handler`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:286`
- `gameplay.narrative.duplicate_handler`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:289`
- `gameplay.narrative.invalid_builtin_consequence`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:319`
- `gameplay.narrative.ambiguous_condition`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:328`
- `gameplay.narrative.unknown_condition`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:431`
- `gameplay.narrative.unknown_beat`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:350`
- `gameplay.narrative.thread_mismatch`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:362`
- `gameplay.narrative.unknown_objective`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:359`
- `gameplay.narrative.unknown_thread`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:404`
- `gameplay.narrative.unknown_consequence`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:437`
- `gameplay.narrative.condition_cycle`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:442`
- `gameplay.narrative.thread_missing`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:658`
- `gameplay.narrative.thread_expired`: `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:644`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.narrative.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:1780`)

```cpp
1778     if (!CanAdvanceRevisionBy(required + 1))
 1779         return foundation::Result<void>::Failure(
 1780             Error("gameplay.narrative.revision_exhausted", "narrative revision counter is exhausted"));
 1781     auto planned = PlanConsequences(it->second.thread, {}, {}, found->consequences, c.correlation, c.time);
 1782     if (!planned)
```

#### `gameplay.narrative.frozen` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:236`)

```cpp
234 {
  235     if (frozen_)
  236         return foundation::Result<void>::Failure(Error("gameplay.narrative.frozen", "narrative definitions are frozen"));
  237     if (!d.id.IsValid())
  238         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_storylet", "invalid storylet definition"));
```

#### `gameplay.narrative.invalid_thread` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:136`)

```cpp
134         return foundation::Result<void>::Failure(Error("gameplay.narrative.frozen", "narrative definitions are frozen"));
  135     if (!d.id.IsValid() || !IsValid(d.initial_state))
  136         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_thread", "invalid thread definition"));
  137     if (thread_defs_.contains(d.id))
  138         return foundation::Result<void>::Failure(Error("gameplay.narrative.duplicate_thread", "duplicate thread definition"));
```

#### `gameplay.narrative.duplicate_thread` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:138`)

```cpp
136         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_thread", "invalid thread definition"));
  137     if (thread_defs_.contains(d.id))
  138         return foundation::Result<void>::Failure(Error("gameplay.narrative.duplicate_thread", "duplicate thread definition"));
  139     auto next_revision = PrepareRevision();
  140     if (!next_revision)
```

#### `gameplay.narrative.allocation_failed` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:2670`)

```cpp
2668     {
 2669         return foundation::Result<void>::Failure(
 2670             Error("gameplay.narrative.allocation_failed", "failed to stage narrative snapshot restore"));
 2671     }
 2672 }
```

#### `gameplay.narrative.invalid_arc` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:153`)

```cpp
151         return foundation::Result<void>::Failure(Error("gameplay.narrative.frozen", "narrative definitions are frozen"));
  152     if (!d.id.IsValid())
  153         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_arc", "invalid arc definition"));
  154     if (arc_defs_.contains(d.id))
  155         return foundation::Result<void>::Failure(Error("gameplay.narrative.duplicate_arc", "duplicate arc definition"));
```

#### `gameplay.narrative.duplicate_arc` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:155`)

```cpp
153         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_arc", "invalid arc definition"));
  154     if (arc_defs_.contains(d.id))
  155         return foundation::Result<void>::Failure(Error("gameplay.narrative.duplicate_arc", "duplicate arc definition"));
  156     auto next_revision = PrepareRevision();
  157     if (!next_revision)
```

#### `gameplay.narrative.invalid_beat` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:170`)

```cpp
168         return foundation::Result<void>::Failure(Error("gameplay.narrative.frozen", "narrative definitions are frozen"));
  169     if (!d.id.IsValid() || !d.thread.IsValid())
  170         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_beat", "invalid beat definition"));
  171     if (beat_defs_.contains(d.id))
  172         return foundation::Result<void>::Failure(Error("gameplay.narrative.duplicate_beat", "duplicate beat definition"));
```

#### `gameplay.narrative.duplicate_beat` (`EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:172`)

```cpp
170         return foundation::Result<void>::Failure(Error("gameplay.narrative.invalid_beat", "invalid beat definition"));
  171     if (beat_defs_.contains(d.id))
  172         return foundation::Result<void>::Failure(Error("gameplay.narrative.duplicate_beat", "duplicate beat definition"));
  173     auto next_revision = PrepareRevision();
  174     if (!next_revision)
```
