---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "effects"]
source_path: "EngineFramework/GameplayWorldStateOwners/Effects"
---

# EngineFramework / Effects

Модуль: **Effects**. Эффекты игровых действий.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Effects/`.
**CMake targets:** `EpidemicGameFrameworkEffects`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Effects/effects.h|effects.h]]: `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`; объявленные типы: `EffectTypeId`, `EffectDefinitionId`, `EffectExecutionId`, `DeferredEffectId`, `EffectTypeIdHash`, `EffectDefinitionIdHash`, `DeferredEffectIdHash`, `RegisteredEffectPayload`, `EffectTargetSelector`, `EffectStepDefinition`, `EffectExecutionPolicy`, `EffectDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Effects|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns effect handler/definition registries, immediate effect execution orchestration, deferred effect records/schedule bindings, execution/deferred ID spaces, diagnostics and effect journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`.

#### Dependency list

Direct production links are `EpidemicGameFrameworkFoundation` and `EpidemicFoundation`. No peer GameplayWorldStateOwner or Runtime module dependency is added by this delta.

#### External ports/callbacks/providers/backends

IEffectHandler and IEffectTargetStateProvider are non-owning ports. Handler Prepare runs before acceptance; handler Commit is treated as an accepted local/external side effect and post-commit framework bookkeeping must not require unbounded allocation.

#### Authoritative state

Handler and definition registries; deferred record map; reverse schedule index; execution/deferred ID generators; diagnostics counters; bounded change journal.

#### Derived/cache/index state

deferred_by_schedule_ is a reverse index over deferred_. It is preflighted/updated with deferred mutations and rebuilt from validated snapshot state.

#### ID spaces, generations, revisions and cursors

Execution and deferred IDs use distinct MonotonicIdGenerator scopes. ChangeCursor epoch/sequence is checked and journal append failure forces snapshot resynchronization rather than false change publication.

#### State machines

Handler/definition registry is mutable until Freeze. Execute is rejected before Freeze. Deferred requests progress through defer, schedule bind/clear, peek/take/acknowledge or cancellation. Restore is rejected before Freeze.

Источник контрактов: `Documentation/EngineFramework/Effects.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `EffectTargetSelector` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `AllTargets`, `FirstTarget`.
- `EffectExecutionPolicy` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `BestEffort`, `RequireAllPrepared`.
- `EffectPrepareDisposition` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `Accepted`, `Rejected`, `NoOp`, `Unavailable`, `InvalidTarget`, `Unsupported`.
- `EffectCommitDisposition` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `Applied`, `NoOp`.
- `EffectOperationDisposition` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `Applied`, `Rejected`, `NoOp`, `Unavailable`, `InvalidTarget`, `Unsupported`, `BudgetExceeded`, `Failed`.
- `EffectBatchDisposition` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `Succeeded`, `PartiallyApplied`, `Rejected`, `Failed`.
- `EffectChangeKind` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `ExecutionStarted`, `Applied`, `Rejected`, `Failed`, `ExecutionCompleted`, `BudgetExceeded`, `DeferredCreated`, `DeferredCancelled`, `DeferredExecuted`.
- `DeferredEffectPersistence` (`EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h`): `Session`, `Persistent`.

### Операции и хранилища состояния

#### `EffectService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:331`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
EffectService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<EffectDefinitionId> RegisterDefinition(EffectDefinition definition);
void SetTargetStateProvider(const IEffectTargetStateProvider* provider) noexcept;
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const EffectDefinition* FindDefinition(EffectDefinitionId id) const noexcept;
[[nodiscard]] foundation::Result<DeferredEffectId> Defer( EffectRequest request, ClockId clock, GameplayTimePoint due, DeferredEffectPersistence persistence = DeferredEffectPersistence::Session);
[[nodiscard]] foundation::Result<void> BindDeferredSchedule(DeferredEffectId id, ScheduleId schedule);
[[nodiscard]] const DeferredEffectRecord* FindDeferred(DeferredEffectId id) const noexcept;
[[nodiscard]] std::optional<DeferredEffectRecord> FindDeferredCopy(DeferredEffectId id) const noexcept;
[[nodiscard]] std::vector<DeferredEffectRecord> AllDeferred() const;
[[nodiscard]] std::vector<DeferredEffectRecord> UnscheduledDeferred() const;
[[nodiscard]] foundation::Result<void> ClearDeferredSchedule(DeferredEffectId id);
[[nodiscard]] EffectChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] std::uint64_t OldestChangeSequence() const noexcept;
void PruneChangesBefore(std::uint64_t sequence);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::size_t kChangeJournalCapacity = 4096`; `std::unordered_map<EffectTypeId, HandlerEntry, EffectTypeIdHash> handlers_`; `std::unordered_map<EffectDefinitionId, EffectDefinition, EffectDefinitionIdHash> definitions_`; `const IEffectTargetStateProvider* target_state_provider_ = nullptr`; `bool frozen_ = false`; `MonotonicIdGenerator<GameplayObjectId> execution_ids_`; `MonotonicIdGenerator<GameplayObjectId> deferred_ids_`; `std::unordered_map<DeferredEffectId, DeferredEffectRecord, DeferredEffectIdHash> deferred_`.

#### `IEffectHandler`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:189`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEffectHandler() = default;
[[nodiscard]] virtual EffectTypeId Type() const noexcept = 0;
[[nodiscard]] virtual EffectHandlerCapabilities Capabilities() const noexcept = 0;
[[nodiscard]] virtual foundation::Result<EffectPrepareResult> Prepare(const EffectOperation& operation) const = 0;
```

#### `IEffectTargetStateProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:210`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEffectTargetStateProvider() = default;
[[nodiscard]] virtual EffectTargetState Resolve(GameplayObjectRef target) const = 0;
```

#### `EffectTypeId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:22`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `EffectDefinitionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `RegisteredEffectPayload`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:71`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] bool Empty() const noexcept;
template <typename T> [[nodiscard]] static RegisteredEffectPayload FromTrivial(TypeId type_id, const T& value);
template <typename T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected_type) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> bytes`.

#### `EffectTypeIdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(EffectTypeId id) const noexcept;
```

#### `EffectDefinitionIdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:62`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(EffectDefinitionId id) const noexcept;
```

#### `DeferredEffectIdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:66`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const DeferredEffectId& id) const noexcept;
```

#### `EffectStepDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Effects/include/Epidemic/GameFramework/Effects/effects.h:108`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `EffectTargetSelector selector = EffectTargetSelector::AllTargets`; `std::int64_t magnitude_micro = 0`; `RegisteredEffectPayload payload`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Effects/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp` (1086 строк): `Error::Create`, `EffectService::EffectService`, `GameplayObjectId::FromString`, `EffectService::RegisterHandler`, `EffectTypeId::FromString`, `EffectService::RegisterDefinition`, `EffectDefinitionId::FromString`, `EffectService::FindDefinition`, `EffectService::ValidatePayload`, `EffectService::ExpandRequest`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.effects.executions`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:103`
- `framework.effects.deferred`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:104`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:148`
- `gameplay.effect_handler_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:121`
- `gameplay.effect_handler_type_mismatch`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:126`
- `gameplay.already_registered`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:165`
- `gameplay.effect_payload_schema_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:134`
- `gameplay.effect_storage_failed`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1068`
- `gameplay.effect_definition_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:152`
- `gameplay.effect_definition_id_mismatch`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:161`
- `gameplay.effect_type_unknown`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:172`
- `gameplay.effect_payload_unexpected`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:202`
- `gameplay.effect_payload_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:208`
- `gameplay.effect_payload_rejected`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:216`
- `gameplay.effect_payload_validator_failed`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:225`
- `gameplay.effect_request_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:238`
- `gameplay.effect_targets_empty`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:242`
- `gameplay.effect_target_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:250`
- `gameplay.effect_prepare_storage_failed`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:373`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1029`
- `gameplay.effect_definition_unknown`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:452`
- `gameplay.effect_execution_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:458`
- `gameplay.effect_execution_storage_failed`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:504`
- `gameplay.deferred_effect_invalid`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:700`
- `gameplay.deferred_effect_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:706`
- `gameplay.deferred_effect_id_conflict`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:723`
- `gameplay.deferred_effect_unknown`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:870`
- `gameplay.deferred_effect_schedule_rebind`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:740`
- `gameplay.deferred_effect_schedule_conflict`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:750`
- `gameplay.deferred_effect_schedule_unknown`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:849`
- `gameplay.deferred_effect_schedule_inconsistent`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:854`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1025`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/effects_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.effects.executions` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:103`)

```cpp
101 
  102 EffectService::EffectService()
  103     : execution_ids_(GameplayObjectId::FromString("framework.effects.executions").High()),
  104       deferred_ids_(GameplayObjectId::FromString("framework.effects.deferred").High())
  105 {
```

#### `framework.effects.deferred` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:104`)

```cpp
102 EffectService::EffectService()
  103     : execution_ids_(GameplayObjectId::FromString("framework.effects.executions").High()),
  104       deferred_ids_(GameplayObjectId::FromString("framework.effects.deferred").High())
  105 {
  106 }
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:148`)

```cpp
146     if (frozen_)
  147     {
  148         return foundation::Result<EffectDefinitionId>::Failure(Error("gameplay.registry_frozen", "effect definition registry is frozen"));
  149     }
  150     if (definition.canonical_name.empty() || definition.steps.empty())
```

#### `gameplay.effect_handler_invalid` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:121`)

```cpp
119     if (canonical_name.empty() || !handler)
  120     {
  121         return foundation::Result<EffectTypeId>::Failure(Error("gameplay.effect_handler_invalid", "effect handler name and instance are required"));
  122     }
  123     const auto id = EffectTypeId::FromString(canonical_name);
```

#### `gameplay.effect_handler_type_mismatch` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:126`)

```cpp
124     if (handler->Type() != id)
  125     {
  126         return foundation::Result<EffectTypeId>::Failure(Error("gameplay.effect_handler_type_mismatch", "effect handler Type() does not match canonical name"));
  127     }
  128     if (handlers_.contains(id))
```

#### `gameplay.already_registered` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:165`)

```cpp
163     if (definitions_.contains(definition.id))
  164     {
  165         return foundation::Result<EffectDefinitionId>::Failure(Error("gameplay.already_registered", "effect definition is already registered"));
  166     }
  167     for (const auto& step : definition.steps)
```

#### `gameplay.effect_payload_schema_invalid` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:134`)

```cpp
132     if (payload_type.IsValid() && max_payload_bytes == 0)
  133     {
  134         return foundation::Result<EffectTypeId>::Failure(Error("gameplay.effect_payload_schema_invalid", "typed effect payload requires non-zero max bytes"));
  135     }
  136     HandlerEntry entry{std::string(canonical_name), std::move(handler), payload_type, max_payload_bytes, std::move(validator)};
```

#### `gameplay.effect_storage_failed` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:1068`)

```cpp
1066     if (test_seam::Consume(test_seam::FaultPoint::RestoreBeforeCommit))
 1067         return foundation::Result<void>::Failure(
 1068             Error("gameplay.effect_storage_failed", "failed to stage effects snapshot"));
 1069 
 1070     deferred_ = std::move(rebuilt);
```

#### `gameplay.effect_definition_invalid` (`EngineFramework/GameplayWorldStateOwners/Effects/src/effects.cpp:152`)

```cpp
150     if (definition.canonical_name.empty() || definition.steps.empty())
  151     {
  152         return foundation::Result<EffectDefinitionId>::Failure(Error("gameplay.effect_definition_invalid", "effect definition requires name and at least one step"));
  153     }
  154     const auto expected = EffectDefinitionId::FromString(definition.canonical_name);
```
