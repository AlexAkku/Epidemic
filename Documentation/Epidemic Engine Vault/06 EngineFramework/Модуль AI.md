---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "a_i"]
source_path: "EngineFramework/GameplayWorldStateOwners/AI"
---

# EngineFramework / AI

Модуль: **AI**. Gameplay AI contracts.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/AI/`.
**CMake targets:** `EpidemicGameFrameworkAI`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/AI/ai.h|ai.h]]: `EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`; объявленные типы: `AIProfileId`, `AIGoalId`, `AIIntentTypeId`, `AIIntentId`, `AIConsiderationId`, `AIConsiderationEvaluatorId`, `AIInputKeyId`, `AIAccessPolicyId`, `BlackboardKeyId`, `AIReplanReasonId`, `IdHash`, `RefHash`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/AI|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns AI profiles/goals/considerations, agents, blackboards, intents, think scheduling indexes, generators, revisions and the bounded AI change journal.

The service remains the single authoritative owner of that state. Derived indexes, journals and diagnostic/read models are not independent semantic owners.

#### Authoritative state and indexes

Admission/generated B05 inventory: 90 public callables, 104 mutation obligations, 4 lifecycle candidates, 26 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Multi-container mutations must complete fallible staging before authoritative publication. Deterministic query/order semantics must agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B05 coverage projection. Revision, identity-generator and journal publication is part of the mutation contract. Allocation/publication failure may not expose partially advanced primary state, index state, generator, revision or journal.

B05 allocation evidence uses source-private module seams rather than the shared process-global allocator override. The seams are implementation/test details and do not expand the public API.

#### Lifecycle and identity

All emitted lifecycle and stale-identity candidates were reviewed. Invalid, removed, duplicate and stale identities are rejected or handled as the documented no-op without false publication. Retry does not silently resurrect terminal state.

#### Persistence

Snapshot/restore is reviewed as candidate-state publication: validate first, build off-state, preserve the current service on failure, then atomically publish the candidate. Successful restore preserves generator/revision/cursor continuity required by the module snapshot contract.

#### External boundaries

AI evaluators and target/input providers are decision inputs only. Throws/failures are isolated before authoritative agent/intent/schedule publication, with deterministic tie breaking retained.

#### Threading contract

No additional internal synchronization guarantee is introduced by B05. These gameplay owners are treated as externally serialized/owner-thread services for Goal 4 local correctness. Engine-wide concurrency qualification remains Goal 6.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkAITests` from `EngineFramework/DevelopmentInfrastructure/Tests/ai_tests.cpp`.

Goal 4 adds source-private publication fault points around profile, agent, scheduling and think-decision/index staging. Existing evaluator failure, scheduling, deterministic choice and snapshot tests remain green.

Источник контрактов: `Documentation/EngineFramework/AI.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AIAgentActivity` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Idle`, `Thinking`, `ExecutingIntent`, `Suspended`, `Disabled`.
- `AIMaterializationPolicy` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `AbstractCapable`, `RequiresMaterialized`, `RequiresRuntimeProjection`.
- `AIIntentStatus` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Issued`, `Accepted`, `Running`, `Succeeded`, `Failed`, `Cancelled`, `TimedOut`.
- `AITargetPolicy` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Targetless`, `Optional`, `Required`.
- `AIConsiderationScope` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Context`, `Target`.
- `AITargetSource` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Perceived`, `Known`, `Public`, `Group`, `LastKnown`, `CurrentPerception`, `Knowledge`.
- `AIInputValueKind` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Fixed`, `Boolean`, `Type`, `Object`, `Payload`.
- `BlackboardPersistencePolicy` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `Session`, `Persistent`.
- `AIThinkDeferReason` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `None`, `NotDue`, `Suspended`, `Disabled`, `MaterializationUnavailable`, `ActiveIntent`, `BudgetExceeded`.
- `AIChangeKind` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `AgentRegistered`, `AgentUnregistered`, `AgentEnabled`, `AgentDisabled`, `AgentSuspended`, `AgentResumed`, `ThinkScheduled`, `ThinkStarted`, `ThinkCompleted`, `GoalSelected`, `GoalCompleted`, `GoalFailed`, `IntentIssued`, `IntentAccepted` и другие значения.
- `AIAccessFlag` (`EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h`): `SelfState`, `PerceivedState`, `KnownState`, `PublicFacts`, `GroupKnowledge`, `OmniscientDebug`.

### Операции и хранилища состояния

#### `AIService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h:427`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
AIService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] static constexpr AIConsiderationEvaluatorId InputEvaluatorId() noexcept;
[[nodiscard]] foundation::Result<void> RegisterAccessPolicy(AIAccessPolicy policy);
[[nodiscard]] foundation::Result<void> RegisterConsiderationEvaluator(AIConsiderationEvaluatorId id, const IAIConsiderationEvaluator &evaluator);
[[nodiscard]] foundation::Result<void> RegisterGoal(AIGoalDefinition goal);
[[nodiscard]] foundation::Result<void> RegisterProfile(AIProfile profile);
[[nodiscard]] foundation::Result<void> RegisterBlackboardKey(BlackboardKeyDefinition definition);
[[nodiscard]] foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] const AIAccessPolicy *FindAccessPolicy(AIAccessPolicyId id) const noexcept;
[[nodiscard]] const AIGoalDefinition *FindGoal(AIGoalId id) const noexcept;
[[nodiscard]] const AIProfile *FindProfile(AIProfileId id) const noexcept;
[[nodiscard]] const BlackboardKeyDefinition *FindBlackboardKey(BlackboardKeyId id) const noexcept;
[[nodiscard]] const AIAgentState *FindAgent(GameplayObjectRef subject) const noexcept;
[[nodiscard]] const AIBlackboardEntry *FindBlackboard(GameplayObjectRef subject, BlackboardKeyId key) const noexcept;
void SetBudget(AIBudget budget) noexcept;
void SetChangeJournalCapacity(std::size_t capacity) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<AIAccessPolicyId, AIAccessPolicy, IdHash> policies_`; `std::unordered_map<AIConsiderationEvaluatorId, const IAIConsiderationEvaluator *, IdHash> evaluators_`; `std::unordered_map<AIGoalId, AIGoalDefinition, IdHash> goals_`; `std::unordered_map<AIProfileId, AIProfile, IdHash> profiles_`; `std::unordered_map<BlackboardKeyId, BlackboardKeyDefinition, IdHash> blackboard_keys_`; `std::unordered_map<GameplayObjectRef, AIAgentState, RefHash> agents_`; `std::unordered_map<AIIntentId, GameplayObjectRef, IdHash> intent_to_agent_`.

#### `IAIConsiderationEvaluator`

Объявлен в `EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h:418`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAIConsiderationEvaluator() = default;
[[nodiscard]] virtual foundation::Result<Fixed> Evaluate(const AIConsideration &definition, const AIContextSnapshot &context, const AITargetCandidate *target) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h:107`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `AIIntentId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h:43`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr AIIntentId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `AIInputKeyId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/AI/include/Epidemic/GameFramework/AI/ai.h:74`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr AIInputKeyId FromType(TypeId value) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/AI/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp` (1355 строк): `Error::Create`, `AIService::AIService`, `AIAccessPolicyId::FromString`, `AIService::RegisterAccessPolicy`, `AIService::RegisterConsiderationEvaluator`, `AIService::RegisterGoal`, `AIService::RegisterProfile`, `AIService::RegisterBlackboardKey`, `AIService::Freeze`, `AIService::FindAccessPolicy`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.ai.input_key_invalid`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:53`
- `gameplay.ai.input_type_unsupported`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:74`
- `gameplay.ai.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:189`
- `gameplay.ai.invalid_policy`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:126`
- `gameplay.ai.invalid_evaluator`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:138`
- `gameplay.ai.invalid_goal`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:148`
- `gameplay.ai.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:161`
- `gameplay.ai.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:1122`
- `gameplay.ai.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:902`
- `gameplay.ai.invalid_blackboard_key`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:193`
- `gameplay.ai.invalid_consideration`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:211`
- `gameplay.ai.duplicate_consideration`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:214`
- `gameplay.ai.invalid_weight`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:219`
- `gameplay.ai.invalid_input_key`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:222`
- `gameplay.ai.invalid_target_policy`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:225`
- `gameplay.ai.unknown_policy`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:233`
- `gameplay.ai.debug_access_forbidden`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:238`
- `gameplay.ai.invalid_profile_timing`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:241`
- `gameplay.ai.unknown_goal`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:247`
- `gameplay.ai.duplicate_goal`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:250`
- `gameplay.ai.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:283`
- `gameplay.ai.invalid_agent`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:333`
- `gameplay.ai.schedule_collision`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:412`
- `gameplay.ai.agent_missing`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:1114`
- `gameplay.ai.intent_active`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:484`
- `gameplay.ai.agent_not_disabled`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:454`
- `gameplay.ai.agent_disabled`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:489`
- `gameplay.ai.agent_not_suspended`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:507`
- `gameplay.ai.invalid_replan_reason`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:553`
- `gameplay.ai.evaluator_missing`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:600`
- `gameplay.ai.evaluator_range`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:609`
- `gameplay.ai.evaluator_exception`: `EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:622`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/ai_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.ai.input_key_invalid` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:53`)

```cpp
51         if (!definition.input_key.IsValid())
   52             return foundation::Result<Fixed>::Failure(
   53                 Error("gameplay.ai.input_key_invalid", "input evaluator requires a valid input key"));
   54         const AIInputValue *input = nullptr;
   55         if (definition.scope == AIConsiderationScope::Target)
```

#### `gameplay.ai.input_type_unsupported` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:74`)

```cpp
72         default:
   73             return foundation::Result<Fixed>::Failure(Error(
   74                 "gameplay.ai.input_type_unsupported", "builtin input evaluator supports fixed and boolean values"));
   75         }
   76     }
```

#### `gameplay.ai.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:189`)

```cpp
187 {
  188     if (frozen_)
  189         return foundation::Result<void>::Failure(Error("gameplay.ai.registry_frozen", "ai registry frozen"));
  190     if (!definition.key.IsValid() || definition.canonical_name.empty() || !definition.value_type.IsValid() ||
  191         definition.max_payload_bytes == 0 || blackboard_keys_.contains(definition.key))
```

#### `gameplay.ai.invalid_policy` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:126`)

```cpp
124     if (!policy.id.IsValid() || policy.canonical_name.empty() || policies_.contains(policy.id))
  125         return foundation::Result<void>::Failure(
  126             Error("gameplay.ai.invalid_policy", "invalid or duplicate access policy"));
  127     policies_.emplace(policy.id, std::move(policy));
  128     return foundation::Result<void>::Success();
```

#### `gameplay.ai.invalid_evaluator` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:138`)

```cpp
136     if (!id.IsValid() || evaluators_.contains(id))
  137         return foundation::Result<void>::Failure(
  138             Error("gameplay.ai.invalid_evaluator", "invalid or duplicate consideration evaluator"));
  139     evaluators_.emplace(id, &evaluator);
  140     return foundation::Result<void>::Success();
```

#### `gameplay.ai.invalid_goal` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:148`)

```cpp
146         return foundation::Result<void>::Failure(Error("gameplay.ai.registry_frozen", "ai registry frozen"));
  147     if (!goal.id.IsValid() || goal.canonical_name.empty() || !goal.intent_type.IsValid() || goals_.contains(goal.id))
  148         return foundation::Result<void>::Failure(Error("gameplay.ai.invalid_goal", "invalid or duplicate goal"));
  149     std::sort(goal.considerations.begin(), goal.considerations.end(),
  150               [](const auto &a, const auto &b) { return a.id < b.id; });
```

#### `gameplay.ai.invalid_profile` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:161`)

```cpp
159     if (!profile.id.IsValid() || profile.canonical_name.empty() || profiles_.contains(profile.id))
  160         return foundation::Result<void>::Failure(
  161             Error("gameplay.ai.invalid_profile", "invalid or duplicate ai profile"));
  162     if (!CanBump())
  163         return foundation::Result<void>::Failure(Error("gameplay.ai.revision_exhausted", "ai revision exhausted"));
```

#### `gameplay.ai.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:1122`)

```cpp
1120     if (!CanBump() || !CanRecord())
 1121         return foundation::Result<void>::Failure(
 1122             Error("gameplay.ai.revision_exhausted", "ai revision or journal exhausted"));
 1123     agent->blackboard.erase(it);
 1124     (void)Bump();
```

#### `gameplay.ai.allocation_failed` (`EngineFramework/GameplayWorldStateOwners/AI/src/ai.cpp:902`)

```cpp
900     {
  901         return foundation::Result<AIThinkResult>::Failure(
  902             Error("gameplay.ai.allocation_failed", "failed to publish ai decision state"));
  903     }
  904
```
