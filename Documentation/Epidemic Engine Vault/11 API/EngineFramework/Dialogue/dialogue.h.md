---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h"
---

# Dialogue: dialogue.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`.

**Модуль:** [[06 EngineFramework/Модуль Dialogue|Dialogue]].
**Размер исходного файла:** 17409 байт.
**Подключение:** `#include <Epidemic/GameFramework/Dialogue/dialogue.h>`.

## Объявления типов (лексический индекс)
`name`, `IdHash`, `ConversationState`, `DialogueConditionState`, `DialogueConsequenceState`, `DialogueOptionRepeatPolicy`, `DialogueChangeKind`, `DialogueTextRef`, `DialogueConditionDefinition`, `DialogueConsequenceDefinition`, `DialogueOptionDefinition`, `DialogueNodeDefinition`, `ConversationDefinition`, `ConversationParticipant`, `ConversationContext`, `ConversationSession`, `DialogueConditionContext`, `DialogueConditionResult`, `DialogueConsequenceExecution`, `DialogueChange`, `DialogueChangeBatch`, `DialogueSnapshot`, `DialogueDiagnostics`, `IDialogueConditionResolver`, `IDialogueConsequenceHandler`, `DialogueService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once
#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::dialogue
{
#define DLG_TYPE(name)                                                                                                 \
    struct name                                                                                                        \
    {                                                                                                                  \
        TypeId value{};                                                                                                \
        static constexpr name FromString(std::string_view s) noexcept                                                  \
        {                                                                                                              \
            return {TypeId::FromString(s)};                                                                            \
        }                                                                                                              \
        [[nodiscard]] constexpr bool IsValid() const noexcept                                                          \
        {                                                                                                              \
            return value.IsValid();                                                                                    \
        }                                                                                                              \
        [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;                                \
        [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;                               \
    }
#define DLG_OBJ(name)                                                                                                  \
    struct name                                                                                                        \
    {                                                                                                                  \
        GameplayObjectId value{};                                                                                      \
        static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept                                       \
        {                                                                                                              \
            return {GameplayObjectId::FromRaw(h, l)};                                                                  \
        }                                                                                                              \
        [[nodiscard]] constexpr bool IsValid() const noexcept                                                          \
        {                                                                                                              \
            return value.IsValid();                                                                                    \
        }                                                                                                              \
        [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;                                \
        [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;                               \
    }
DLG_TYPE(ConversationDefinitionId);
DLG_TYPE(DialogueNodeId);
DLG_TYPE(DialogueOptionId);
DLG_TYPE(DialogueTopicId);
DLG_TYPE(DialogueConditionTypeId);
DLG_TYPE(DialogueConsequenceTypeId);
DLG_TYPE(DialogueTextKey);
DLG_OBJ(ConversationSessionId);
DLG_OBJ(DialogueConsequenceExecutionId);
#undef DLG_TYPE
#undef DLG_OBJ
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

enum class ConversationState
{
    Active,
    WaitingForChoice,
    WaitingForExternalAction,
    Completed,
    Interrupted,
    Failed
};
enum class DialogueConditionState
{
    Satisfied,
    Unsatisfied,
    Unknown,
    Unavailable
};
enum class DialogueConsequenceState
{
    Pending,
    Applied,
    Deferred,
    Failed,
    ReconciliationRequired
};
enum class DialogueOptionRepeatPolicy
{
    Repeatable,
    OncePerConversation
};
enum class DialogueChangeKind
{
    ConversationStarted,
    NodeEntered,
    OptionSelected,
    ConversationCompleted,
    ConversationInterrupted,
    ConsequencePlanned,
    ConsequenceApplied,
    ConsequenceDeferred,
    ConsequenceFailed,
    ConsequenceResumed,
    ConsequenceReconciliationRequired
};

struct DialogueTextRef
{
    DialogueTextKey key{};
    std::vector<TypeId> parameter_keys;
};
struct DialogueConditionDefinition
{
    TypeId id{};
    DialogueConditionTypeId type{};
    std::vector<std::byte> payload;
};
struct DialogueConsequenceDefinition
{
    TypeId id{};
    DialogueConsequenceTypeId type{};
    std::vector<std::byte> payload;
    std::int32_t priority = 0;
};
struct DialogueOptionDefinition
{
    DialogueOptionId id{};
    DialogueTextRef text;
    std::vector<TypeId> conditions;
    std::vector<TypeId> consequences;
    DialogueNodeId next_node{};
    DialogueOptionRepeatPolicy repeat_policy = DialogueOptionRepeatPolicy::Repeatable;
    GameplayTagSet tags;
};
struct DialogueNodeDefinition
{
    DialogueNodeId id{};
    DialogueTextRef utterance;
    TypeId speaker_role{};
    std::vector<TypeId> conditions;
    std::vector<DialogueOptionDefinition> options;
    std::vector<TypeId> consequences;
    DialogueNodeId automatic_next{};
};
struct ConversationDefinition
{
    ConversationDefinitionId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    std::vector<TypeId> participant_roles;
    DialogueNodeId entry_node{};
    std::vector<DialogueNodeDefinition> nodes;
    Revision revision{};
};
struct ConversationParticipant
{
    TypeId role{};
    GameplayObjectRef object{};
    [[nodiscard]] bool operator==(const ConversationParticipant &) const noexcept = default;
};
struct ConversationContext
{
    GameplayObjectRef area{};
    GameplayTagSet tags;
    GameplayContext gameplay{};
};
struct ConversationSession
{
    ConversationSessionId id{};
    ConversationDefinitionId definition{};
    std::vector<GameplayObjectRef> participants;
    std::vector<ConversationParticipant> participant_bindings;
    GameplayObjectRef current_speaker{};
    DialogueNodeId current_node{};
    ConversationState state = ConversationState::Active;
    ConversationContext context{};
    GameplayTimePoint started_at{};
    std::vector<TypeId> resolved_once_per_conversation_options;
    Revision revision{};
};
struct DialogueConditionContext
{
    ConversationSessionId session{};
    GameplayObjectRef actor{};
    GameplayObjectRef listener{};
    ConversationContext context{};
};
struct DialogueConditionResult
{
    DialogueConditionState state = DialogueConditionState::Unknown;
};
struct DialogueConsequenceExecution
{
    DialogueConsequenceExecutionId id{};
    ConversationSessionId session{};
    TypeId consequence{};
    DialogueConsequenceState state = DialogueConsequenceState::Pending;
    DialogueNodeId source_node{};
    DialogueOptionId source_option{};
    GameplayContext context{};
    Revision revision{};
};
struct DialogueChange
{
    std::uint64_t sequence = 0;
    DialogueChangeKind kind = DialogueChangeKind::ConversationStarted;
    ConversationSessionId session{};
    DialogueNodeId node{};
    DialogueOptionId option{};
    DialogueConsequenceExecutionId consequence_execution{};
    TypeId consequence{};
    DialogueConsequenceState consequence_state = DialogueConsequenceState::Pending;
    TypeId reason{};
    GameplayContext context{};
    Revision revision{};
};
struct DialogueChangeBatch
{
    std::vector<DialogueChange> changes;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    bool snapshot_required = false;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct DialogueSnapshot
{
    std::vector<ConversationSession> sessions;
    std::vector<DialogueConsequenceExecution> consequences;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot session_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot consequence_ids{};
    Revision revision{};

    std::uint64_t change_epoch = 1;
};
struct DialogueDiagnostics
{
    std::uint64_t definitions = 0;
    std::uint64_t active_sessions = 0;
    std::uint64_t options_selected = 0;
    std::uint64_t interruptions = 0;
    std::uint64_t consequences = 0;
};

class IDialogueConditionResolver
{
  public:
    virtual ~IDialogueConditionResolver() = default;
    [[nodiscard]] virtual DialogueConditionResult Evaluate(const DialogueConditionDefinition &,
                                                           const DialogueConditionContext &) const = 0;
};
class IDialogueConsequenceHandler
{
  public:
    virtual ~IDialogueConsequenceHandler() = default;
    [[nodiscard]] virtual DialogueConsequenceState Execute(const DialogueConsequenceDefinition &,
                                                           const DialogueConsequenceExecution &,
                                                           const ConversationSession &) const = 0;
};

class DialogueService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<DialogueService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.dialogue");
    }
    [[nodiscard]] foundation::Result<void> RegisterCondition(DialogueConditionDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterConsequence(DialogueConsequenceDefinition definition);
    [[nodiscard]] foundation::Result<ConversationDefinitionId> RegisterConversation(ConversationDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterConditionResolver(DialogueConditionTypeId type,
                                                                     const IDialogueConditionResolver &resolver);
    [[nodiscard]] foundation::Result<void> RegisterConsequenceHandler(DialogueConsequenceTypeId type,
                                                                      const IDialogueConsequenceHandler &handler);
    [[nodiscard]] foundation::Result<void> FreezeDefinitions();
    [[nodiscard]] foundation::Result<ConversationSessionId> StartConversation(
        ConversationDefinitionId definition, std::vector<GameplayObjectRef> participants,
        ConversationContext context = {});
    [[nodiscard]] foundation::Result<ConversationSessionId> StartConversation(
        ConversationDefinitionId definition, std::vector<ConversationParticipant> participants,
        ConversationContext context = {});
    [[nodiscard]] foundation::Result<void> SelectOption(ConversationSessionId session, DialogueOptionId option,
                                                        GameplayObjectRef actor, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Advance(ConversationSessionId session, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Interrupt(ConversationSessionId session, TypeId reason,
                                                     GameplayContext context = {});
    [[nodiscard]] std::vector<DialogueOptionDefinition> GetAvailableOptions(ConversationSessionId session,
                                                                            GameplayObjectRef actor) const;
    [[nodiscard]] std::optional<ConversationSession> GetSession(ConversationSessionId id) const noexcept;
    [[nodiscard]] const ConversationDefinition *GetDefinition(ConversationDefinitionId id) const noexcept;
    [[nodiscard]] std::vector<DialogueConsequenceExecutionId> ExecutePendingConsequences(std::size_t budget = 128);
    [[nodiscard]] foundation::Result<void> ResumeConsequence(DialogueConsequenceExecutionId execution,
                                                             GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ResolveConsequenceReconciliation(
        DialogueConsequenceExecutionId execution, bool confirmed_applied, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> FailConsequence(DialogueConsequenceExecutionId execution, TypeId reason,
                                                           GameplayContext context = {});
    private:
        [[nodiscard]] DialogueChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] DialogueChangeBatch ReadChangesSince(ChangeCursor cursor) const
    {
        auto batch = ReadChangesSinceSequence(cursor.sequence);
        batch.oldest_available_cursor = {journal_epoch_, batch.oldest_available_sequence};
        batch.latest_cursor = {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                                    : next_change_sequence_ - 1};
        if ((!cursor.IsValid() && cursor.sequence != 0) || (cursor.IsValid() && cursor.epoch != journal_epoch_))
        {
            batch.changes.clear();
            batch.snapshot_required = true;
        }
        return batch;
    }
    [[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept
    {
        return {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                          : next_change_sequence_ - 1};
    }
    private:
        [[nodiscard]] std::vector<DialogueChange> ChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] DialogueSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(DialogueSnapshot snapshot);
    [[nodiscard]] DialogueDiagnostics GetDiagnostics() const noexcept;

  private:
    [[nodiscard]] std::optional<Revision> NextRevision() const noexcept;
    void Record(DialogueChange c) noexcept;
    [[nodiscard]] const DialogueNodeDefinition *FindNode(const ConversationDefinition &d,
                                                         DialogueNodeId id) const noexcept;
    [[nodiscard]] bool ConditionsPass(const std::vector<TypeId> &ids, const DialogueConditionContext &ctx) const;
    [[nodiscard]] foundation::Result<void> EnterNode(ConversationSession &session, DialogueNodeId node,
                                                     GameplayContext context,
                                                     std::optional<std::vector<DialogueConsequenceExecution>> prepared_consequences = std::nullopt);
    [[nodiscard]] foundation::Result<std::vector<DialogueConsequenceExecution>> PrepareConsequences(
        ConversationSessionId session, const std::vector<TypeId> &ids, DialogueNodeId source_node,
        DialogueOptionId source_option, GameplayContext context,
        MonotonicIdGenerator<GameplayObjectId> &staged_generator);
    [[nodiscard]] foundation::Result<void> CommitPreparedConsequences(
        std::vector<DialogueConsequenceExecution> prepared, Revision revision);
    [[nodiscard]] GameplayObjectRef ResolveRole(const ConversationSession &session, TypeId role) const noexcept;
    [[nodiscard]] DialogueConditionContext MakeConditionContext(const ConversationSession &session,
                                                                GameplayObjectRef actor) const noexcept;
    void MaybeCleanupTerminalSession(ConversationSessionId id);
    void RefreshExternalWaitState(ConversationSessionId id, GameplayContext context);
    bool frozen_ = false;
    Revision revision_{};
    std::unordered_map<ConversationDefinitionId, ConversationDefinition, IdHash> definitions_;
    std::unordered_map<TypeId, DialogueConditionDefinition> conditions_;
    std::unordered_map<TypeId, DialogueConsequenceDefinition> consequence_defs_;
    std::unordered_map<DialogueConditionTypeId, const IDialogueConditionResolver *, IdHash> condition_resolvers_;
    std::unordered_map<DialogueConsequenceTypeId, const IDialogueConsequenceHandler *, IdHash> consequence_handlers_;
    std::unordered_map<ConversationSessionId, ConversationSession, IdHash> sessions_;
    std::unordered_map<DialogueConsequenceExecutionId, DialogueConsequenceExecution, IdHash> consequences_;
    MonotonicIdGenerator<GameplayObjectId> session_ids_{0x3600}, consequence_ids_{0x3601};
    std::deque<DialogueChange> changes_;
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    DialogueDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::dialogue
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `ConversationState`: `Active`, `WaitingForChoice`, `WaitingForExternalAction`, `Completed`, `Interrupted`, `Failed`
- `DialogueConditionState`: `Satisfied`, `Unsatisfied`, `Unknown`, `Unavailable`
- `DialogueConsequenceState`: `Pending`, `Applied`, `Deferred`, `Failed`, `ReconciliationRequired`
- `DialogueOptionRepeatPolicy`: `Repeatable`, `OncePerConversation`
- `DialogueChangeKind`: `ConversationStarted`, `NodeEntered`, `OptionSelected`, `ConversationCompleted`, `ConversationInterrupted`, `ConsequencePlanned`, `ConsequenceApplied`, `ConsequenceDeferred`, `ConsequenceFailed`, `ConsequenceResumed`, `ConsequenceReconciliationRequired`

### Состав типов и операции

#### `name` (строка заголовка 15)

Публичные сигнатуры операций:

```cpp
\ static constexpr name FromString(std::string_view s) noexcept \;
\ [[nodiscard]] constexpr bool IsValid() const noexcept \;
\ [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;
\ [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;
```

#### `name` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
\ static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept \;
\ [[nodiscard]] constexpr bool IsValid() const noexcept \;
\ [[nodiscard]] constexpr bool operator==(const name &) const noexcept = default;
\ [[nodiscard]] constexpr auto operator<=>(const name &) const noexcept = default;
```

#### `IdHash` (строка заголовка 55)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 57)

#### `DialogueTextRef` (строка заголовка 107)

Публичные поля и значения по умолчанию:

- `std::vector<TypeId> parameter_keys`

#### `DialogueConditionDefinition` (строка заголовка 112)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`

#### `DialogueConsequenceDefinition` (строка заголовка 118)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`
- `std::int32_t priority = 0`

#### `DialogueOptionDefinition` (строка заголовка 125)

Публичные поля и значения по умолчанию:

- `DialogueTextRef text`
- `std::vector<TypeId> conditions`
- `std::vector<TypeId> consequences`
- `DialogueOptionRepeatPolicy repeat_policy = DialogueOptionRepeatPolicy::Repeatable`
- `GameplayTagSet tags`

#### `DialogueNodeDefinition` (строка заголовка 135)

Публичные поля и значения по умолчанию:

- `DialogueTextRef utterance`
- `std::vector<TypeId> conditions`
- `std::vector<DialogueOptionDefinition> options`
- `std::vector<TypeId> consequences`

#### `ConversationDefinition` (строка заголовка 145)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `std::vector<TypeId> participant_roles`
- `std::vector<DialogueNodeDefinition> nodes`

#### `ConversationParticipant` (строка заголовка 155)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const ConversationParticipant &) const noexcept = default;
```

#### `ConversationContext` (строка заголовка 161)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`

#### `ConversationSession` (строка заголовка 167)

Публичные поля и значения по умолчанию:

- `std::vector<GameplayObjectRef> participants`
- `std::vector<ConversationParticipant> participant_bindings`
- `ConversationState state = ConversationState::Active`
- `std::vector<TypeId> resolved_once_per_conversation_options`

#### `DialogueConditionResult` (строка заголовка 188)

Публичные поля и значения по умолчанию:

- `DialogueConditionState state = DialogueConditionState::Unknown`

#### `DialogueConsequenceExecution` (строка заголовка 192)

Публичные поля и значения по умолчанию:

- `DialogueConsequenceState state = DialogueConsequenceState::Pending`

#### `DialogueChange` (строка заголовка 203)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `DialogueChangeKind kind = DialogueChangeKind::ConversationStarted`
- `DialogueConsequenceState consequence_state = DialogueConsequenceState::Pending`

#### `DialogueChangeBatch` (строка заголовка 217)

Публичные поля и значения по умолчанию:

- `std::vector<DialogueChange> changes`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `bool snapshot_required = false`

#### `DialogueSnapshot` (строка заголовка 227)

Публичные поля и значения по умолчанию:

- `std::vector<ConversationSession> sessions`
- `std::vector<DialogueConsequenceExecution> consequences`
- `std::uint64_t change_epoch = 1`

#### `DialogueDiagnostics` (строка заголовка 237)

Публичные поля и значения по умолчанию:

- `std::uint64_t definitions = 0`
- `std::uint64_t active_sessions = 0`
- `std::uint64_t options_selected = 0`
- `std::uint64_t interruptions = 0`
- `std::uint64_t consequences = 0`

#### `IDialogueConditionResolver` (строка заголовка 246)

Публичные сигнатуры операций:

```cpp
virtual ~IDialogueConditionResolver() = default;
[[nodiscard]] virtual DialogueConditionResult Evaluate(const DialogueConditionDefinition &, const DialogueConditionContext &) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IDialogueConsequenceHandler` (строка заголовка 253)

Публичные сигнатуры операций:

```cpp
virtual ~IDialogueConsequenceHandler() = default;
[[nodiscard]] virtual DialogueConsequenceState Execute(const DialogueConsequenceDefinition &, const DialogueConsequenceExecution &, const ConversationSession &) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `DialogueService` (строка заголовка 262)

Внутреннее состояние (не часть публичного API): `bool frozen_ = false`, `std::unordered_map<ConversationDefinitionId, ConversationDefinition, IdHash> definitions_`, `std::unordered_map<TypeId, DialogueConditionDefinition> conditions_`, `std::unordered_map<TypeId, DialogueConsequenceDefinition> consequence_defs_`, `std::unordered_map<DialogueConditionTypeId, const IDialogueConditionResolver *, IdHash> conditi`, `std::unordered_map<DialogueConsequenceTypeId, const IDialogueConsequenceHandler *, IdHash> cons`, `std::unordered_map<ConversationSessionId, ConversationSession, IdHash> sessions_`, `std::unordered_map<DialogueConsequenceExecutionId, DialogueConsequenceExecution, IdHash> conseq`, `std::deque<DialogueChange> changes_`.

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterCondition` | `DialogueService::RegisterCondition` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:74` |
| `RegisterConsequence` | `DialogueService::RegisterConsequence` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:82` |
| `RegisterConversation` | `DialogueService::RegisterConversation` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:90` |
| `RegisterConditionResolver` | `DialogueService::RegisterConditionResolver` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:136` |
| `RegisterConsequenceHandler` | `DialogueService::RegisterConsequenceHandler` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:145` |
| `FreezeDefinitions` | `DialogueService::FreezeDefinitions` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:154` |
| `GetDefinition` | `DialogueService::GetDefinition` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:221` |
| `GetSession` | `DialogueService::GetSession` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:226` |
| `GetAvailableOptions` | `DialogueService::GetAvailableOptions` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:547` |
| `ExecutePendingConsequences` | `DialogueService::ExecutePendingConsequences` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:731` |
| `CaptureSnapshot` | `DialogueService::CaptureSnapshot` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:922` |
| `RestoreSnapshot` | `DialogueService::RestoreSnapshot` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:947` |
| `GetDiagnostics` | `DialogueService::GetDiagnostics` | `EngineFramework/GameplayWorldStateOwners/Dialogue/src/dialogue.cpp:1078` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterConditionResolver`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:97`
- `RegisterConsequenceHandler`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:98`
- `RegisterCondition`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:101`
- `RegisterConsequence`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:102`
- `RegisterConversation`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:121`
- `FreezeDefinitions`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:131`
- `GetAvailableOptions`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:135`
- `GetSession`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:137`
- `ExecutePendingConsequences`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:138`
- `GetDiagnostics`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:142`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:143`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/dialogue_tests.cpp:152`

