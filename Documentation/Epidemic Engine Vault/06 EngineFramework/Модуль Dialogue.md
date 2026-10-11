---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "dialogue"]
source_path: "EngineFramework/GameplayWorldStateOwners/Dialogue"
---

# EngineFramework / Dialogue

Модуль: **Dialogue**. Dialogue state и контракты разговора.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Dialogue/`.
**CMake targets:** `EpidemicGameFrameworkDialogue`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Dialogue/dialogue.h|dialogue.h]]: `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`; объявленные типы: `name`, `IdHash`, `ConversationState`, `DialogueConditionState`, `DialogueConsequenceState`, `DialogueOptionRepeatPolicy`, `DialogueChangeKind`, `DialogueTextRef`, `DialogueConditionDefinition`, `DialogueConsequenceDefinition`, `DialogueOptionDefinition`, `DialogueNodeDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/dialogue|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `ConversationState` (`EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`): `Active`, `WaitingForChoice`, `WaitingForExternalAction`, `Completed`, `Interrupted`, `Failed`.
- `DialogueConditionState` (`EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`): `Satisfied`, `Unsatisfied`, `Unknown`, `Unavailable`.
- `DialogueConsequenceState` (`EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`): `Pending`, `Applied`, `Deferred`, `Failed`, `ReconciliationRequired`.
- `DialogueOptionRepeatPolicy` (`EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`): `Repeatable`, `OncePerConversation`.
- `DialogueChangeKind` (`EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`): `ConversationStarted`, `NodeEntered`, `OptionSelected`, `ConversationCompleted`, `ConversationInterrupted`, `ConsequencePlanned`, `ConsequenceApplied`, `ConsequenceDeferred`, `ConsequenceFailed`, `ConsequenceResumed`, `ConsequenceReconciliationRequired`.

### Операции и хранилища состояния

#### `DialogueService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:262`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterCondition(DialogueConditionDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterConsequence(DialogueConsequenceDefinition definition);
[[nodiscard]] foundation::Result<ConversationDefinitionId> RegisterConversation(ConversationDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterConditionResolver(DialogueConditionTypeId type, const IDialogueConditionResolver &resolver);
[[nodiscard]] foundation::Result<void> RegisterConsequenceHandler(DialogueConsequenceTypeId type, const IDialogueConsequenceHandler &handler);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] std::vector<DialogueOptionDefinition> GetAvailableOptions(ConversationSessionId session, GameplayObjectRef actor) const;
[[nodiscard]] std::optional<ConversationSession> GetSession(ConversationSessionId id) const noexcept;
[[nodiscard]] const ConversationDefinition *GetDefinition(ConversationDefinitionId id) const noexcept;
[[nodiscard]] std::vector<DialogueConsequenceExecutionId> ExecutePendingConsequences(std::size_t budget = 128);
[[nodiscard]] DialogueChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] DialogueSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(DialogueSnapshot snapshot);
[[nodiscard]] DialogueDiagnostics GetDiagnostics() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`; `std::unordered_map<ConversationDefinitionId, ConversationDefinition, IdHash> definitions_`; `std::unordered_map<TypeId, DialogueConditionDefinition> conditions_`; `std::unordered_map<TypeId, DialogueConsequenceDefinition> consequence_defs_`; `std::unordered_map<DialogueConditionTypeId, const IDialogueConditionResolver *, IdHash> condition_resolvers_`; `std::unordered_map<DialogueConsequenceTypeId, const IDialogueConsequenceHandler *, IdHash> consequence_handlers_`; `std::unordered_map<ConversationSessionId, ConversationSession, IdHash> sessions_`; `std::unordered_map<DialogueConsequenceExecutionId, DialogueConsequenceExecution, IdHash> consequences_`.

#### `IDialogueConditionResolver`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:246`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IDialogueConditionResolver() = default;
[[nodiscard]] virtual DialogueConditionResult Evaluate(const DialogueConditionDefinition &, const DialogueConditionContext &) const = 0;
```

#### `IDialogueConsequenceHandler`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:253`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IDialogueConsequenceHandler() = default;
[[nodiscard]] virtual DialogueConsequenceState Execute(const DialogueConsequenceDefinition &, const DialogueConsequenceExecution &, const ConversationSession &) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:55`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `name`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
\ static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept \;
```

#### `DialogueTextRef`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:107`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<TypeId> parameter_keys`.

#### `DialogueConditionDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:112`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> payload`.

#### `DialogueConsequenceDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:118`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> payload`; `std::int32_t priority = 0`.

#### `DialogueOptionDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h:125`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `DialogueTextRef text`; `std::vector<TypeId> conditions`; `std::vector<TypeId> consequences`; `DialogueOptionRepeatPolicy repeat_policy = DialogueOptionRepeatPolicy::Repeatable`; `GameplayTagSet tags`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Dialogue/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp` (1105 строк): `Error::Create`, `DialogueService::RegisterCondition`, `DialogueService::RegisterConsequence`, `DialogueService::RegisterConversation`, `ConversationDefinitionId::FromString`, `DialogueService::RegisterConditionResolver`, `DialogueService::RegisterConsequenceHandler`, `DialogueService::FreezeDefinitions`, `DialogueService::FindNode`, `DialogueService::GetDefinition`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.dialogue.invalid_condition`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:78`
- `gameplay.dialogue.invalid_consequence`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:86`
- `gameplay.dialogue.frozen`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:94`
- `gameplay.dialogue.invalid_definition`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:103`
- `gameplay.dialogue.invalid_roles`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:108`
- `gameplay.dialogue.invalid_graph`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:129`
- `gameplay.dialogue.invalid_resolver`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:141`
- `gameplay.dialogue.invalid_handler`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:150`
- `gameplay.dialogue.unknown_condition`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:187`
- `gameplay.dialogue.missing_resolver`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:191`
- `gameplay.dialogue.unknown_consequence`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:287`
- `gameplay.dialogue.missing_handler`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:201`
- `gameplay.dialogue.unknown_node`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:209`
- `gameplay.dialogue.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:484`
- `gameplay.dialogue.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:687`
- `gameplay.dialogue.duplicate_consequence`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:648`
- `gameplay.dialogue.node_missing`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:594`
- `gameplay.dialogue.speaker_missing`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:365`
- `gameplay.dialogue.node_unavailable`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:597`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:879`
- `gameplay.dialogue.invalid_participant`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:475`
- `dialogue.participant.primary`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:448`
- `dialogue.participant.secondary`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:448`
- `gameplay.dialogue.not_frozen`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:456`
- `gameplay.dialogue.cannot_start`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:459`
- `gameplay.dialogue.duplicate_session`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:510`
- `gameplay.dialogue.session_not_waiting`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:574`
- `gameplay.dialogue.actor_invalid`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:577`
- `gameplay.dialogue.option_unavailable`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:587`
- `gameplay.dialogue.session_not_active`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:695`
- `gameplay.dialogue.no_automatic_transition`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:700`
- `gameplay.dialogue.session_missing`: `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:708`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.dialogue.invalid_condition` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:78`)

```cpp
76     if (frozen_ || !d.id.IsValid() || !d.type.IsValid() || conditions_.contains(d.id))
   77         return foundation::Result<void>::Failure(
   78             Error("gameplay.dialogue.invalid_condition", "invalid/duplicate dialogue condition"));
   79     conditions_.emplace(d.id, std::move(d));
   80     return foundation::Result<void>::Success();
```

#### `gameplay.dialogue.invalid_consequence` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:86`)

```cpp
84     if (frozen_ || !d.id.IsValid() || !d.type.IsValid() || consequence_defs_.contains(d.id))
   85         return foundation::Result<void>::Failure(
   86             Error("gameplay.dialogue.invalid_consequence", "invalid/duplicate dialogue consequence"));
   87     consequence_defs_.emplace(d.id, std::move(d));
   88     return foundation::Result<void>::Success();
```

#### `gameplay.dialogue.frozen` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:94`)

```cpp
92     if (frozen_)
   93         return foundation::Result<ConversationDefinitionId>::Failure(
   94             Error("gameplay.dialogue.frozen", "dialogue definitions frozen"));
   95     if (d.canonical_name.empty())
   96         return foundation::Result<ConversationDefinitionId>::Failure(
```

#### `gameplay.dialogue.invalid_definition` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:103`)

```cpp
101     if (!d.id.IsValid() || d.id != expected || !d.entry_node.IsValid() || definitions_.contains(d.id))
  102         return foundation::Result<ConversationDefinitionId>::Failure(
  103             Error("gameplay.dialogue.invalid_definition", "invalid/duplicate conversation definition"));
  104     std::unordered_set<TypeId> roles;
  105     for (auto role : d.participant_roles)
```

#### `gameplay.dialogue.invalid_roles` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:108`)

```cpp
106         if (!role.IsValid() || !roles.insert(role).second)
  107             return foundation::Result<ConversationDefinitionId>::Failure(
  108                 Error("gameplay.dialogue.invalid_roles", "invalid/duplicate participant role"));
  109     std::unordered_set<DialogueNodeId, IdHash> nodes;
  110     std::unordered_set<DialogueOptionId, IdHash> options;
```

#### `gameplay.dialogue.invalid_graph` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:129`)

```cpp
127     if (!nodes.contains(d.entry_node))
  128         return foundation::Result<ConversationDefinitionId>::Failure(
  129             Error("gameplay.dialogue.invalid_graph", "entry node missing"));
  130     d.revision = Revision{1};
  131     const auto id = d.id;
```

#### `gameplay.dialogue.invalid_resolver` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:141`)

```cpp
139     if (frozen_ || !t.IsValid() || condition_resolvers_.contains(t))
  140         return foundation::Result<void>::Failure(
  141             Error("gameplay.dialogue.invalid_resolver", "invalid/duplicate condition resolver"));
  142     condition_resolvers_[t] = &r;
  143     return foundation::Result<void>::Success();
```

#### `gameplay.dialogue.invalid_handler` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:150`)

```cpp
148     if (frozen_ || !t.IsValid() || consequence_handlers_.contains(t))
  149         return foundation::Result<void>::Failure(
  150             Error("gameplay.dialogue.invalid_handler", "invalid/duplicate consequence handler"));
  151     consequence_handlers_[t] = &h;
  152     return foundation::Result<void>::Success();
```

#### `gameplay.dialogue.unknown_condition` (`EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:187`)

```cpp
185                     auto it = conditions_.find(c);
  186                     if (it == conditions_.end())
  187                         return foundation::Result<void>::Failure(Error("gameplay.dialogue.unknown_condition",
  188                                                                        "dialogue option references unknown condition"));
  189                     if (!condition_resolvers_.contains(it->second.type))
```
