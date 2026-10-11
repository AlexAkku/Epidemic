---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "interaction"]
source_path: "EngineFramework/GameplayWorldStateOwners/Interaction"
---

# EngineFramework / Interaction

Модуль: **Interaction**. Игровые взаимодействия.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Interaction/`.
**CMake targets:** `EpidemicGameFrameworkInteraction`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Interaction/interaction.h|interaction.h]]: `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`; объявленные типы: `InteractionTypeId`, `InteractionProviderId`, `InteractionExecutionId`, `InteractionPoint`, `InteractionAvailability`, `InteractionExecutionMode`, `InteractionMaterializationPolicy`, `InteractionSessionState`, `InteractionPersistence`, `InteractionDefinition`, `InteractionContext`, `InteractionCandidate`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Interaction|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility

Owns interaction definitions, provider/executor registrations, active timed interaction sessions, schedule bindings, execution IDs, revision/journal state and interaction diagnostics.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`.

#### Dependency list

Direct production links are `EpidemicGameFrameworkFoundation` and `EpidemicFoundation`. No peer GameplayWorldStateOwner or Runtime module dependency is added by this delta.

#### External ports/callbacks/providers/backends

IInteractionProvider, IInteractionExecutor and IInteractionStateProvider are non-owning ports. Candidate collection and executor validation/commit failures are contained by the service contract; volatile materialization is rechecked at commit.

#### Authoritative state

Definition registry; provider/executor maps; active session map; schedule-to-session reverse index; ID generator; revision; bounded journal; non-owning state provider.

#### Derived/cache/index state

session_by_schedule_ derives from session records. Actor active-session queries derive from sessions_ and cannot become a second source of truth.

#### ID spaces, generations, revisions and cursors

InteractionExecutionId is service-generated. Prepared plans carry actor/target revisions used for stale checks. Revision and journal cursor are monotonic and restored with epoch continuity.

#### State machines

Definitions/providers/executors register before Freeze. Interaction flows candidate -> prepare -> commit, with immediate or active timed session outcome, then complete/cancel/timeout. Restore is rejected before Freeze.

Источник контрактов: `Documentation/EngineFramework/Interaction.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `InteractionAvailability` (`EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`): `Available`, `Unavailable`, `Hidden`, `Deferred`.
- `InteractionExecutionMode` (`EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`): `Instant`, `Timed`, `Continuous`, `Channelled`.
- `InteractionMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`): `AbstractAllowed`, `RequiresMaterializedActor`, `RequiresMaterializedTarget`, `RequiresBothMaterialized`.
- `InteractionSessionState` (`EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`): `Preparing`, `Active`, `Completed`, `Cancelled`, `Failed`.
- `InteractionPersistence` (`EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`): `Transient`, `PersistentSession`.
- `InteractionChangeKind` (`EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h`): `Started`, `Completed`, `Cancelled`, `Failed`.

### Операции и хранилища состояния

#### `InteractionService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:229`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
InteractionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterDefinition(InteractionDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterExecutor(InteractionTypeId type, IInteractionExecutor &executor);
void SetStateProvider(const IInteractionStateProvider *provider) noexcept;
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const InteractionDefinition *FindDefinition(InteractionTypeId type) const noexcept;
[[nodiscard]] std::vector<InteractionCandidate> GetAvailableInteractions(const InteractionContext &context) const;
[[nodiscard]] foundation::Result<InteractionPlan> Prepare(const InteractionContext &context, InteractionCandidate candidate) const;
[[nodiscard]] foundation::Result<InteractionResult> Commit(const InteractionPlan &plan);
[[nodiscard]] foundation::Result<void> BindCompletionSchedule(InteractionExecutionId id, ScheduleId schedule);
[[nodiscard]] const InteractionSession *FindSession(InteractionExecutionId id) const noexcept;
[[nodiscard]] std::optional<InteractionSession> FindSessionCopy(InteractionExecutionId id) const noexcept;
[[nodiscard]] std::vector<InteractionSession> FindActive(GameplayObjectRef actor) const;
[[nodiscard]] InteractionSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(InteractionSnapshot snapshot);
[[nodiscard]] InteractionChangeBatch ReadChangesSince(ChangeCursor cursor) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<InteractionTypeId, InteractionDefinition, TypeHash> definitions_`; `std::unordered_map<InteractionProviderId, const IInteractionProvider *, ProviderHash> providers_`; `std::unordered_map<InteractionTypeId, IInteractionExecutor *, TypeHash> executors_`; `std::unordered_map<InteractionExecutionId, InteractionSession, ExecutionHash> sessions_`; `std::unordered_map<ScheduleId, InteractionExecutionId> session_by_schedule_`; `MonotonicIdGenerator<GameplayObjectId> ids_`; `bool frozen_ = false`; `mutable std::uint64_t candidate_requests_ = 0, candidates_ = 0`.

#### `IInteractionProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:205`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IInteractionProvider() = default;
[[nodiscard]] virtual InteractionProviderId Id() const noexcept = 0;
[[nodiscard]] virtual std::vector<InteractionCandidate> Collect(const InteractionContext &context) const = 0;
```

#### `IInteractionStateProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:221`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IInteractionStateProvider() = default;
[[nodiscard]] virtual bool IsMaterialized(GameplayObjectRef object) const = 0;
[[nodiscard]] virtual Revision RevisionOf(GameplayObjectRef object) const = 0;
```

#### `IInteractionExecutor`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:212`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IInteractionExecutor() = default;
[[nodiscard]] virtual foundation::Result<void> Validate(const InteractionPlan &plan) const = 0;
```

#### `InteractionPoint`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:60`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`.

#### `InteractionDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:100`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string canonical_name`; `GameplayTagSet tags`; `InteractionExecutionMode mode = InteractionExecutionMode::Instant`; `InteractionMaterializationPolicy materialization = InteractionMaterializationPolicy::RequiresBothMaterialized`; `InteractionPersistence persistence = InteractionPersistence::Transient`; `std::vector<std::byte> payload`.

#### `InteractionContext`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:112`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `GameplayTagSet context_tags`.

#### `InteractionCandidate`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:122`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `InteractionPriority priority = 0`; `InteractionAvailability availability = InteractionAvailability::Available`; `std::vector<std::byte> payload`.

#### `InteractionSession`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Interaction/include/Epidemic/GameFramework/Interaction/interaction.h:133`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `InteractionSessionState state = InteractionSessionState::Preparing`; `std::vector<std::byte> payload`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Interaction/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp` (687 строк): `Error::Create`, `InteractionService::InteractionService`, `GameplayObjectId::FromString`, `InteractionService::PrepareRevision`, `InteractionService::RegisterDefinition`, `InteractionService::RegisterProvider`, `InteractionService::RegisterExecutor`, `InteractionService::FindDefinition`, `InteractionService::GetAvailableInteractions`, `InteractionService::MaterializationAllowed`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.interaction.execution.seed`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:67`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:76`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:104`
- `gameplay.interaction.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:87`
- `gameplay.interaction.invalid_provider`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:97`
- `gameplay.interaction.invalid_executor`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:107`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:581`
- `gameplay.interaction.unavailable`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:214`
- `gameplay.interaction.not_materialized`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:441`
- `gameplay.interaction.context_mismatch`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:220`
- `gameplay.interaction.state_provider_exception`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:453`
- `gameplay.stale_revision`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:448`
- `gameplay.interaction.executor_exception`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:469`
- `gameplay.interaction.definition_missing`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:409`
- `gameplay.interaction.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:298`
- `gameplay.time_overflow`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:346`
- `gameplay.interaction.storage_failed`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:608`
- `gameplay.interaction.session_missing`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:491`
- `gameplay.interaction.schedule_rebind`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:376`
- `gameplay.interaction.schedule_conflict`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:381`
- `gameplay.interaction.failed`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:437`
- `gameplay.interaction.executor_validation_failed`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:464`
- `gameplay.interaction.executor_commit_failed`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:472`
- `gameplay.interaction.external_time_required`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:509`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:577`
- `gameplay.interaction.restore_invalid_generator`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:604`
- `gameplay.interaction.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:594`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.interaction.execution.seed` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:67`)

```cpp
65 } // namespace
   66 InteractionService::InteractionService()
   67     : ids_(GameplayObjectId::FromString("framework.interaction.execution.seed").High())
   68 {
   69 }
```

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:76`)

```cpp
74     if (!next)
   75         return foundation::Result<Revision>::Failure(
   76             E("gameplay.revision_exhausted", "interaction revision counter is exhausted"));
   77     return foundation::Result<Revision>::Success(*next);
   78 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:104`)

```cpp
102 {
  103     if (frozen_)
  104         return foundation::Result<void>::Failure(E("gameplay.registry_frozen", "interaction registry is frozen"));
  105     if (!t.IsValid() || !definitions_.contains(t) || executors_.contains(t))
  106         return foundation::Result<void>::Failure(
```

#### `gameplay.interaction.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:87`)

```cpp
85         definitions_.contains(d.type))
   86         return foundation::Result<void>::Failure(
   87             E("gameplay.interaction.invalid_definition", "invalid or duplicate interaction definition"));
   88     definitions_.emplace(d.type, std::move(d));
   89     return foundation::Result<void>::Success();
```

#### `gameplay.interaction.invalid_provider` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:97`)

```cpp
95     if (!p.Id().IsValid() || providers_.contains(p.Id()))
   96         return foundation::Result<void>::Failure(
   97             E("gameplay.interaction.invalid_provider", "invalid or duplicate interaction provider"));
   98     providers_.emplace(p.Id(), &p);
   99     return foundation::Result<void>::Success();
```

#### `gameplay.interaction.invalid_executor` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:107`)

```cpp
105     if (!t.IsValid() || !definitions_.contains(t) || executors_.contains(t))
  106         return foundation::Result<void>::Failure(
  107             E("gameplay.interaction.invalid_executor", "invalid or duplicate interaction executor"));
  108     executors_.emplace(t, &e);
  109     return foundation::Result<void>::Success();
```

#### `gameplay.registry_not_frozen` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:581`)

```cpp
579     if (!frozen_)
  580         return foundation::Result<void>::Failure(
  581             E("gameplay.registry_not_frozen", "interaction registry must be frozen before restore"));
  582     if (!MonotonicIdGenerator<GameplayObjectId>::IsValidSnapshot(s.ids) || s.ids.scope != ids_.Scope().Raw())
  583         return foundation::Result<void>::Failure(
```

#### `gameplay.interaction.unavailable` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:214`)

```cpp
212     if (!IsValid(candidate.availability) || !d || candidate.availability != InteractionAvailability::Available)
  213         return foundation::Result<InteractionPlan>::Failure(
  214             E("gameplay.interaction.unavailable", "interaction is unavailable"));
  215     if (!MaterializationAllowed(*d, c))
  216         return foundation::Result<InteractionPlan>::Failure(
```

#### `gameplay.interaction.not_materialized` (`EngineFramework/GameplayWorldStateOwners/Interaction/src/interaction.cpp:441`)

```cpp
439 
  440     if (!MaterializationAllowed(*definition, context))
  441         return fail_terminal(TypeId::FromString("gameplay.interaction.not_materialized"));
  442     if (state_provider_)
  443     {
```
