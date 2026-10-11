---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h"
---

# Narrative: narrative.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Narrative/include/Epidemic/GameFramework/Narrative/narrative.h`.

**Модуль:** [[06 EngineFramework/Модуль Narrative|Narrative]].
**Размер исходного файла:** 43795 байт.
**Подключение:** `#include <Epidemic/GameFramework/Narrative/narrative.h>`.

## Объявления типов (лексический индекс)
`NarrativeThreadId`, `NarrativeArcId`, `NarrativeBeatId`, `NarrativeObjectiveId`, `NarrativeConditionId`, `NarrativeConsequenceId`, `NarrativeConsequenceExecutionId`, `JournalEntryId`, `ClueId`, `RumorId`, `StoryletId`, `NarrativeGateId`, `NarrativeFlagId`, `NarrativeVariableId`, `NarrativeSceneId`, `NarrativeChoiceId`, `NarrativeChoiceOptionId`, `NarrativeThreadTypeId`, `NarrativeEventTypeId`, `NarrativeConditionTypeId`, `NarrativeConsequenceTypeId`, `JournalEntryTypeId`, `IdHash`, `NarrativeRuntimeState`, `NarrativeObjectiveRuntimeState`, `ConditionEvaluationState`, `ConsequenceExecutionState`, `JournalVisibilityState`, `RumorState`, `NarrativeChoiceState`, `NarrativeEventExecutionState`, `NarrativeEventExecutionPhase`, `NarrativeGateState`, `NarrativeChangeKind`, `NarrativeReason`, `NarrativeEvent`, `NarrativeEventKey`, `NarrativeConsequenceKey`, `NarrativeEventKeyHash`, `NarrativeConsequenceKeyHash`, `NarrativeEvaluationContext`, `NarrativeExecutionContext`, `NarrativeConditionResult`, `NarrativeConsequenceResult`, `NarrativeConditionDefinition`, `NarrativeConsequenceDefinition`, `JournalTemplate`, `RumorTemplate`, `NarrativeBeatDefinition`, `NarrativeObjectiveDefinition`, `NarrativeThreadDefinition`, `NarrativeArcDefinition`, `StoryletDefinition`, `NarrativeChoiceOption`, `NarrativeThreadState`, `NarrativeObjectiveState`, `NarrativeConsequenceExecution`, `JournalEntry`, `ClueRecord`, `RumorRecord`, `NarrativeFlag`, `NarrativeVariable`, `NarrativeChoice`, `NarrativeEventExecution`, `StoryletRuntimeState`, `NarrativeChange`, `NarrativeSnapshot`, `NarrativeChangeBatch`, `NarrativeDiagnostics`, `INarrativeConditionResolver`, `INarrativeConsequenceHandler`, `NarrativeProcessBudget`, `NarrativeService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <deque>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace epidemic::gameplay::narrative
{
struct NarrativeThreadId
{
    GameplayObjectId value{};
    static constexpr NarrativeThreadId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeThreadId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeThreadId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeThreadId &) const noexcept = default;
};
struct NarrativeArcId
{
    GameplayObjectId value{};
    static constexpr NarrativeArcId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeArcId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeArcId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeArcId &) const noexcept = default;
};
struct NarrativeBeatId
{
    GameplayObjectId value{};
    static constexpr NarrativeBeatId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeBeatId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeBeatId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeBeatId &) const noexcept = default;
};
struct NarrativeObjectiveId
{
    GameplayObjectId value{};
    static constexpr NarrativeObjectiveId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeObjectiveId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeObjectiveId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeObjectiveId &) const noexcept = default;
};
struct NarrativeConditionId
{
    GameplayObjectId value{};
    static constexpr NarrativeConditionId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeConditionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeConditionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeConditionId &) const noexcept = default;
};
struct NarrativeConsequenceId
{
    GameplayObjectId value{};
    static constexpr NarrativeConsequenceId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeConsequenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeConsequenceId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceId &) const noexcept = default;
};
struct NarrativeConsequenceExecutionId
{
    GameplayObjectId value{};
    static constexpr NarrativeConsequenceExecutionId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeConsequenceExecutionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeConsequenceExecutionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceExecutionId &) const noexcept = default;
};
struct JournalEntryId
{
    GameplayObjectId value{};
    static constexpr JournalEntryId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr JournalEntryId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const JournalEntryId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JournalEntryId &) const noexcept = default;
};
struct ClueId
{
    GameplayObjectId value{};
    static constexpr ClueId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr ClueId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ClueId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ClueId &) const noexcept = default;
};
struct RumorId
{
    GameplayObjectId value{};
    static constexpr RumorId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr RumorId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const RumorId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const RumorId &) const noexcept = default;
};
struct StoryletId
{
    GameplayObjectId value{};
    static constexpr StoryletId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr StoryletId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const StoryletId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const StoryletId &) const noexcept = default;
};
struct NarrativeGateId
{
    GameplayObjectId value{};
    static constexpr NarrativeGateId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeGateId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeGateId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeGateId &) const noexcept = default;
};
struct NarrativeFlagId
{
    GameplayObjectId value{};
    static constexpr NarrativeFlagId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeFlagId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeFlagId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeFlagId &) const noexcept = default;
};
struct NarrativeVariableId
{
    GameplayObjectId value{};
    static constexpr NarrativeVariableId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeVariableId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeVariableId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeVariableId &) const noexcept = default;
};
struct NarrativeSceneId
{
    GameplayObjectId value{};
    static constexpr NarrativeSceneId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeSceneId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeSceneId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeSceneId &) const noexcept = default;
};
struct NarrativeChoiceId
{
    GameplayObjectId value{};
    static constexpr NarrativeChoiceId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeChoiceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeChoiceId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeChoiceId &) const noexcept = default;
};
struct NarrativeChoiceOptionId
{
    GameplayObjectId value{};
    static constexpr NarrativeChoiceOptionId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    static constexpr NarrativeChoiceOptionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeChoiceOptionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeChoiceOptionId &) const noexcept = default;
};
struct NarrativeThreadTypeId
{
    TypeId value{};
    static constexpr NarrativeThreadTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeThreadTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeThreadTypeId &) const noexcept = default;
};
struct NarrativeEventTypeId
{
    TypeId value{};
    static constexpr NarrativeEventTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeEventTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeEventTypeId &) const noexcept = default;
};
struct NarrativeConditionTypeId
{
    TypeId value{};
    static constexpr NarrativeConditionTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeConditionTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeConditionTypeId &) const noexcept = default;
};
struct NarrativeConsequenceTypeId
{
    TypeId value{};
    static constexpr NarrativeConsequenceTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeConsequenceTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceTypeId &) const noexcept = default;
};
struct JournalEntryTypeId
{
    TypeId value{};
    static constexpr JournalEntryTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const JournalEntryTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const JournalEntryTypeId &) const noexcept = default;
};
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

enum class NarrativeRuntimeState
{
    Hidden,
    Available,
    Discovered,
    Active,
    Suspended,
    Completed,
    Failed,
    Expired,
    Locked
};
enum class NarrativeObjectiveRuntimeState
{
    Hidden,
    Available,
    Active,
    Completed,
    Failed,
    Cancelled,
    OptionalMissed
};
enum class ConditionEvaluationState
{
    Satisfied,
    Unsatisfied,
    Unknown,
    Unavailable,
    Partial
};
enum class ConsequenceExecutionState
{
    Pending,
    Applied,
    Deferred,
    FailedRetryable,
    FailedPermanent,
    Unsupported,
    AlreadyApplied
};
enum class JournalVisibilityState
{
    Hidden,
    Discovered,
    Updated,
    Resolved,
    Archived,
    Suppressed
};
enum class RumorState
{
    Active,
    Spread,
    Discredited,
    Expired,
    Confirmed,
    Suppressed
};
enum class NarrativeChoiceState
{
    Open,
    Resolved,
    Cancelled,
    Expired
};
enum class NarrativeEventExecutionState
{
    Pending,
    Processing,
    Completed,
    FailedRetryable
};
enum class NarrativeEventExecutionPhase
{
    Beats,
    Objectives,
    Storylets,
    Completed
};
enum class NarrativeGateState
{
    Closed,
    Open,
    Locked,
    Disabled
};
enum class NarrativeChangeKind
{
    ThreadStarted,
    ThreadCompleted,
    ThreadFailed,
    ThreadSuspended,
    ThreadResumed,
    ThreadExpired,
    ObjectiveActivated,
    ObjectiveCompleted,
    ObjectiveFailed,
    ObjectiveCancelled,
    ObjectiveOptionalMissed,
    ObjectiveProgressChanged,
    JournalEntryAdded,
    ClueDiscovered,
    RumorCreated,
    RumorExpired,
    ChoiceCreated,
    ChoiceResolved,
    ChoiceCancelled,
    ChoiceExpired,
    StoryletActivated,
    ConsequencePlanned,
    ConsequenceApplied,
    ConsequenceFailed,
    EventProcessed,
    EventDeduplicated,
    FlagChanged,
    VariableChanged
};

using NarrativeValue = std::variant<bool, std::int64_t, GameplayObjectRef, GameplayTimePoint, TypeId>;
struct NarrativeReason
{
    TypeId type{};
    std::vector<std::byte> payload;
};
struct NarrativeEvent
{
    NarrativeEventTypeId type{};
    GameplayObjectRef subject{};
    GameplayObjectRef instigator{};
    GameplayObjectRef target{};
    GameplayObjectRef area{};
    GameplayTimePoint time{};
    GameplayTagSet tags{};
    std::vector<std::byte> payload;
    CorrelationId correlation{};
};
struct NarrativeEventKey
{
    NarrativeEventTypeId type{};
    GameplayObjectRef subject{};
    GameplayObjectRef instigator{};
    GameplayObjectRef target{};
    GameplayObjectRef area{};
    GameplayTimePoint time{};
    CorrelationId correlation{};
    [[nodiscard]] constexpr bool operator==(const NarrativeEventKey &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeEventKey &) const noexcept = default;
};
struct NarrativeConsequenceKey
{
    NarrativeThreadId thread{};
    NarrativeBeatId beat{};
    NarrativeObjectiveId objective{};
    NarrativeConsequenceId consequence{};
    CorrelationId correlation{};
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return consequence.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const NarrativeConsequenceKey &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceKey &) const noexcept = default;
};
struct NarrativeEventKeyHash
{
    [[nodiscard]] std::size_t operator()(const NarrativeEventKey &key) const noexcept
    {
        auto h = std::hash<TypeId>{}(key.type.value);
        Combine(h, std::hash<GameplayObjectRef>{}(key.subject));
        Combine(h, std::hash<GameplayObjectRef>{}(key.instigator));
        Combine(h, std::hash<GameplayObjectRef>{}(key.target));
        Combine(h, std::hash<GameplayObjectRef>{}(key.area));
        Combine(h, std::hash<std::int64_t>{}(key.time.ticks));
        Combine(h, std::hash<CorrelationId>{}(key.correlation));
        return h;
    }

  private:
    static void Combine(std::size_t &h, std::size_t v) noexcept
    {
        h ^= v + 0x9E3779B97F4A7C15ull + (h << 6u) + (h >> 2u);
    }
};
struct NarrativeConsequenceKeyHash
{
    [[nodiscard]] std::size_t operator()(const NarrativeConsequenceKey &key) const noexcept
    {
        auto h = std::hash<GameplayObjectId>{}(key.thread.value);
        Combine(h, std::hash<GameplayObjectId>{}(key.beat.value));
        Combine(h, std::hash<GameplayObjectId>{}(key.objective.value));
        Combine(h, std::hash<GameplayObjectId>{}(key.consequence.value));
        Combine(h, std::hash<CorrelationId>{}(key.correlation));
        return h;
    }

  private:
    static void Combine(std::size_t &h, std::size_t v) noexcept
    {
        h ^= v + 0x9E3779B97F4A7C15ull + (h << 6u) + (h >> 2u);
    }
};
struct NarrativeEvaluationContext
{
    NarrativeEvent event{};
    GameplayObjectRef viewer{};
    GameplayTimePoint now{};
};
struct NarrativeExecutionContext
{
    GameplayObjectRef default_owner{};
    GameplayTimePoint now{};
    GameplayContext gameplay{};
};
struct NarrativeConditionResult
{
    NarrativeConditionId condition{};
    ConditionEvaluationState state = ConditionEvaluationState::Unavailable;
    std::int64_t score = 0;
    std::vector<NarrativeReason> reasons;
    Revision dependencies_revision{};
};
struct NarrativeConsequenceResult
{
    ConsequenceExecutionState state = ConsequenceExecutionState::Unsupported;
    Revision revision{};
};

struct NarrativeConditionDefinition
{
    NarrativeConditionId id{};
    NarrativeConditionTypeId type{};
    GameplayTagSet tags{};
    std::vector<NarrativeConditionId> all_of;
    std::vector<NarrativeConditionId> any_of;
    std::vector<std::byte> payload;
    Revision revision{};
};
struct NarrativeConsequenceDefinition
{
    NarrativeConsequenceId id{};
    NarrativeConsequenceTypeId type{};
    GameplayTagSet tags{};
    std::int32_t priority = 0;
    bool require_success_before_advance = false;
    std::vector<std::byte> payload;
    struct JournalTemplate
    {
        JournalEntryTypeId type{};
        JournalVisibilityState visibility = JournalVisibilityState::Discovered;
        GameplayTagSet tags{};
        std::vector<std::byte> payload;
    };
    struct RumorTemplate
    {
        TypeId topic{};
        std::int64_t confidence = 0;
        GameplayDuration lifetime{};
        std::vector<std::byte> payload;
    };
    std::optional<JournalTemplate> journal_template;
    std::optional<RumorTemplate> rumor_template;
    Revision revision{};
};
struct NarrativeBeatDefinition
{
    NarrativeBeatId id{};
    NarrativeThreadId thread{};
    GameplayTagSet tags{};
    std::vector<NarrativeConditionId> activation_conditions;
    std::vector<NarrativeConditionId> completion_conditions;
    std::vector<NarrativeConsequenceId> consequences;
    std::int32_t order = 0;
    Revision revision{};
};
struct NarrativeObjectiveDefinition
{
    NarrativeObjectiveId id{};
    NarrativeThreadId thread{};
    GameplayTagSet tags{};
    std::vector<NarrativeConditionId> start_conditions;
    std::vector<NarrativeConditionId> completion_conditions;
    std::vector<NarrativeConditionId> failure_conditions;
    bool player_visible = false;
    Revision revision{};
};
struct NarrativeThreadDefinition
{
    NarrativeThreadId id{};
    GameplayTagSet tags{};
    NarrativeThreadTypeId type{};
    std::vector<NarrativeBeatId> beats;
    std::vector<NarrativeObjectiveId> root_objectives;
    NarrativeRuntimeState initial_state = NarrativeRuntimeState::Hidden;
    std::int32_t priority = 0;
    Revision revision{};
};
struct NarrativeArcDefinition
{
    NarrativeArcId id{};
    GameplayTagSet tags{};
    std::vector<NarrativeThreadId> threads;
    Revision revision{};
};
struct StoryletDefinition
{
    StoryletId id{};
    GameplayTagSet tags{};
    std::vector<NarrativeConditionId> availability_conditions;
    std::vector<NarrativeConsequenceId> consequences;
    std::int32_t priority = 0;
    std::uint32_t max_activations = 1;
    GameplayDuration cooldown{};
    Revision revision{};
};
struct NarrativeChoiceOption
{
    NarrativeChoiceOptionId id{};
    GameplayTagSet tags{};
    std::vector<NarrativeConditionId> availability_conditions;
    std::vector<NarrativeConsequenceId> consequences;
    std::vector<std::byte> payload;
};

struct NarrativeThreadState
{
    NarrativeThreadId thread{};
    NarrativeRuntimeState state = NarrativeRuntimeState::Hidden;
    GameplayObjectRef owner_subject{};
    GameplayObjectRef scope{};
    GameplayTimePoint started_at{};
    GameplayTimePoint updated_at{};
    Revision revision{};
};
struct NarrativeObjectiveState
{
    NarrativeObjectiveId objective{};
    NarrativeObjectiveRuntimeState state = NarrativeObjectiveRuntimeState::Hidden;
    GameplayTimePoint started_at{};
    GameplayTimePoint completed_at{};
    std::int64_t progress = 0;
    Revision revision{};
};
struct NarrativeConsequenceExecution
{
    NarrativeConsequenceExecutionId id{};
    NarrativeThreadId thread{};
    NarrativeBeatId beat{};
    NarrativeObjectiveId objective{};
    NarrativeConsequenceId consequence{};
    ConsequenceExecutionState state = ConsequenceExecutionState::Pending;
    GameplayTimePoint planned_at{};
    GameplayTimePoint applied_at{};
    CorrelationId correlation{};
    std::uint32_t attempts = 0;
    NarrativeConsequenceKey idempotency_key{};
    Revision revision{};
};
struct JournalEntry
{
    JournalEntryId id{};
    GameplayObjectRef owner{};
    NarrativeThreadId thread{};
    NarrativeObjectiveId objective{};
    JournalEntryTypeId type{};
    JournalVisibilityState visibility = JournalVisibilityState::Hidden;
    GameplayTimePoint discovered_at{};
    GameplayTagSet tags{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct ClueRecord
{
    ClueId id{};
    GameplayObjectRef owner{};
    TypeId type{};
    TypeId topic{};
    GameplayObjectRef area{};
    std::int64_t confidence = 0;
    GameplayTimePoint discovered_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct RumorRecord
{
    RumorId id{};
    GameplayObjectRef owner_or_scope{};
    TypeId topic{};
    RumorState state = RumorState::Active;
    std::int64_t confidence = 0;
    GameplayTimePoint created_at{};
    GameplayTimePoint expires_at{};
    std::vector<std::byte> payload;
    Revision revision{};
};
struct NarrativeFlag
{
    NarrativeFlagId id{};
    GameplayObjectRef scope{};
    bool value = false;
    GameplayTimePoint changed_at{};
    Revision revision{};
};
struct NarrativeVariable
{
    NarrativeVariableId id{};
    GameplayObjectRef scope{};
    NarrativeValue value{};
    Revision revision{};
};
struct NarrativeChoice
{
    NarrativeChoiceId id{};
    NarrativeThreadId thread{};
    GameplayObjectRef actor{};
    std::vector<NarrativeChoiceOption> options;
    NarrativeChoiceState state = NarrativeChoiceState::Open;
    std::optional<NarrativeChoiceOptionId> selected_option;
    GameplayTimePoint created_at{};
    GameplayTimePoint closed_at{};
    GameplayTimePoint expires_at{};
    Revision revision{};
};
struct NarrativeEventExecution
{
    NarrativeEventKey key{};
    NarrativeEvent event{};
    NarrativeEventExecutionState state = NarrativeEventExecutionState::Pending;
    NarrativeEventExecutionPhase phase = NarrativeEventExecutionPhase::Beats;
    bool work_plan_initialized = false;
    std::vector<NarrativeBeatId> beat_plan;
    std::vector<NarrativeObjectiveId> objective_plan;
    std::vector<StoryletId> storylet_plan;
    std::size_t next_beat = 0;
    std::size_t next_objective = 0;
    std::size_t next_storylet = 0;
    Revision revision{};
};
struct StoryletRuntimeState
{
    StoryletId id{};
    std::uint32_t activations = 0;
    GameplayTimePoint last_activated_at{};
    Revision revision{};
};
struct NarrativeChange
{
    std::uint64_t sequence = 0;
    NarrativeChangeKind kind = NarrativeChangeKind::EventProcessed;
    NarrativeThreadId thread{};
    NarrativeObjectiveId objective{};
    NarrativeBeatId beat{};
    NarrativeConsequenceExecutionId consequence_execution{};
    JournalEntryId journal{};
    ClueId clue{};
    RumorId rumor{};
    NarrativeChoiceId choice{};
    GameplayObjectRef subject{};
    GameplayContext context{};
    Revision revision{};
};

struct NarrativeSnapshot
{
    std::vector<NarrativeThreadState> thread_states;
    std::vector<NarrativeObjectiveState> objective_states;
    std::vector<NarrativeConsequenceExecution> consequences;
    std::vector<JournalEntry> journal_entries;
    std::vector<ClueRecord> clues;
    std::vector<RumorRecord> rumors;
    std::vector<NarrativeFlag> flags;
    std::vector<NarrativeVariable> variables;
    std::vector<NarrativeChoice> choices;
    std::vector<StoryletRuntimeState> storylet_runtime;
    std::vector<NarrativeEventExecution> event_executions;
    std::vector<NarrativeBeatId> activated_beats;
    std::vector<NarrativeEventKey> processed_event_keys;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot journal_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot clue_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot rumor_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot choice_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot consequence_ids{};
    Revision revision{};
    bool frozen = false;

    std::uint64_t change_epoch = 1;
};
struct NarrativeChangeBatch
{
    std::vector<NarrativeChange> changes;
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct NarrativeDiagnostics
{
    std::uint64_t threads = 0, active_threads = 0, completed_threads = 0, objectives = 0, active_objectives = 0,
                  completed_objectives = 0, journal_entries = 0, clues = 0, rumors = 0, storylets = 0,
                  events_processed = 0, deduplicated_events = 0, condition_evaluations = 0, consequences_planned = 0,
                  consequences_applied = 0, consequences_failed = 0, storylets_evaluated = 0, storylets_activated = 0,
                  budget_exhaustions = 0, idempotency_hits = 0;
};

class INarrativeConditionResolver
{
  public:
    virtual ~INarrativeConditionResolver() = default;
    [[nodiscard]] virtual NarrativeConditionResult Evaluate(const NarrativeConditionDefinition &definition,
                                                            const NarrativeEvaluationContext &context) const = 0;
};
class INarrativeConsequenceHandler
{
  public:
    virtual ~INarrativeConsequenceHandler() = default;
    [[nodiscard]] virtual NarrativeConsequenceResult Execute(const NarrativeConsequenceDefinition &definition,
                                                             const NarrativeConsequenceExecution &execution,
                                                             const NarrativeExecutionContext &context) const = 0;
};
struct NarrativeProcessBudget
{
    std::size_t max_condition_evaluations = 1024;
    std::size_t max_storylets_evaluated = 256;
    std::size_t max_storylets_activated = 16;
    std::size_t max_consequences_planned = 256;
};

class NarrativeService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<NarrativeService>;

  public:
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.narrative");
    }
    [[nodiscard]] foundation::Result<void> RegisterThreadDefinition(NarrativeThreadDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterArcDefinition(NarrativeArcDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterBeatDefinition(NarrativeBeatDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterObjectiveDefinition(NarrativeObjectiveDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterConditionDefinition(NarrativeConditionDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterConsequenceDefinition(NarrativeConsequenceDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterStoryletDefinition(StoryletDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterConditionResolver(NarrativeConditionTypeId type,
                                                                     const INarrativeConditionResolver &resolver);
    [[nodiscard]] foundation::Result<void> RegisterConsequenceHandler(NarrativeConsequenceTypeId type,
                                                                      const INarrativeConsequenceHandler &handler);
    [[nodiscard]] foundation::Result<void> FreezeDefinitions();
    [[nodiscard]] foundation::Result<void> StartThread(NarrativeThreadId thread, GameplayObjectRef owner,
                                                       GameplayObjectRef scope, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SuspendThread(NarrativeThreadId thread, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ResumeThread(NarrativeThreadId thread, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ExpireThread(NarrativeThreadId thread, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompleteThread(NarrativeThreadId thread, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> FailThread(NarrativeThreadId thread, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ActivateObjective(NarrativeObjectiveId objective,
                                                             GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompleteObjective(NarrativeObjectiveId objective,
                                                             GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> FailObjective(NarrativeObjectiveId objective, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CancelObjective(NarrativeObjectiveId objective,
                                                           GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> MarkObjectiveOptionalMissed(NarrativeObjectiveId objective,
                                                                       GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> UpdateObjectiveProgress(NarrativeObjectiveId objective,
                                                                   std::int64_t progress, GameplayContext context = {});
    [[nodiscard]] NarrativeConditionResult EvaluateCondition(NarrativeConditionId condition,
                                                             NarrativeEvaluationContext context) const;
    [[nodiscard]] foundation::Result<void> ProcessNarrativeEvent(NarrativeEvent event,
                                                                 NarrativeProcessBudget budget = {});
    [[nodiscard]] foundation::Result<JournalEntryId> AddJournalEntry(JournalEntry entry, GameplayContext context = {});
    [[nodiscard]] foundation::Result<ClueId> DiscoverClue(ClueRecord clue, GameplayContext context = {});
    [[nodiscard]] foundation::Result<RumorId> CreateRumor(RumorRecord rumor, GameplayContext context = {});
    [[nodiscard]] std::vector<RumorId> ExpireRumors(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] foundation::Result<NarrativeChoiceId> CreateChoice(NarrativeChoice choice,
                                                                     GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ResolveChoice(NarrativeChoiceId choice, NarrativeChoiceOptionId option,
                                                         GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CancelChoice(NarrativeChoiceId choice, GameplayContext context = {});
    [[nodiscard]] std::vector<NarrativeChoiceId> ExpireChoices(GameplayTimePoint now,
                                                               GameplayContext context = {});
    [[nodiscard]] std::vector<NarrativeConsequenceExecutionId> ExecutePendingConsequences(
        NarrativeExecutionContext context, std::size_t budget = 256);
    [[nodiscard]] foundation::Result<void> SetFlag(NarrativeFlag flag, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> SetVariable(NarrativeVariable variable, GameplayContext context = {});
    [[nodiscard]] const NarrativeThreadState *GetThreadState(NarrativeThreadId thread) const noexcept;
    [[nodiscard]] const NarrativeObjectiveState *GetObjectiveState(NarrativeObjectiveId objective) const noexcept;
    [[nodiscard]] const JournalEntry *GetJournalEntry(JournalEntryId id) const noexcept;
    [[nodiscard]] const ClueRecord *GetClue(ClueId id) const noexcept;
    [[nodiscard]] const RumorRecord *GetRumor(RumorId id) const noexcept;
    [[nodiscard]] const NarrativeChoice *GetChoice(NarrativeChoiceId id) const noexcept;
    [[nodiscard]] std::vector<JournalEntry> GetJournal(GameplayObjectRef owner) const;
    [[nodiscard]] std::vector<ClueRecord> FindClues(GameplayObjectRef owner, TypeId topic = {}) const;
    [[nodiscard]] std::vector<RumorRecord> FindRumors(GameplayObjectRef scope, TypeId topic = {}) const;
    [[nodiscard]] std::vector<NarrativeConsequenceExecution> FindConsequences(ConsequenceExecutionState state) const;
    private:
        [[nodiscard]] std::vector<NarrativeChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] NarrativeChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] NarrativeChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] std::uint64_t CompactTerminalExecutions(GameplayTimePoint before);
    [[nodiscard]] NarrativeSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(NarrativeSnapshot snapshot);
    [[nodiscard]] NarrativeDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept
    {
        return revision_;
    }

  private:
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const noexcept;
    [[nodiscard]] bool CanAdvanceRevisionBy(std::size_t count) const noexcept;
    void CommitRevision(Revision revision) noexcept
    {
        revision_ = revision;
    }
    void Record(NarrativeChange change) noexcept;
    [[nodiscard]] bool AllSatisfied(const std::vector<NarrativeConditionId> &conditions,
                                    const NarrativeEvaluationContext &context) const;
    [[nodiscard]] bool AnySatisfied(const std::vector<NarrativeConditionId> &conditions,
                                    const NarrativeEvaluationContext &context) const;
    [[nodiscard]] bool ValidateConditionGraphAcyclic() const;
    [[nodiscard]] bool HasConditionCycle(NarrativeConditionId id,
                                         std::unordered_map<NarrativeConditionId, std::uint8_t, IdHash> &marks) const;
    [[nodiscard]] std::size_t CountUnplannedConsequences(NarrativeThreadId thread, NarrativeBeatId beat,
                                                         NarrativeObjectiveId objective,
                                                         const std::vector<NarrativeConsequenceId> &consequences,
                                                         CorrelationId correlation) const;
    [[nodiscard]] foundation::Result<void> PlanConsequences(NarrativeThreadId thread, NarrativeBeatId beat,
                                                            NarrativeObjectiveId objective,
                                                            const std::vector<NarrativeConsequenceId> &consequences,
                                                            CorrelationId correlation, GameplayTimePoint now);
    [[nodiscard]] NarrativeEventKey MakeEventKey(const NarrativeEvent &event) const noexcept;
    [[nodiscard]] NarrativeConsequenceKey MakeConsequenceKey(NarrativeThreadId thread, NarrativeBeatId beat,
                                                             NarrativeObjectiveId objective,
                                                             NarrativeConsequenceId consequence,
                                                             CorrelationId correlation) const noexcept;
    [[nodiscard]] bool IsBuiltinAddJournal(NarrativeConsequenceTypeId type) const noexcept;
    [[nodiscard]] bool IsBuiltinCreateRumor(NarrativeConsequenceTypeId type) const noexcept;
    void IndexRumorExpiry(const RumorRecord &rumor);
    [[nodiscard]] NarrativeThreadState &EnsureThreadState(NarrativeThreadId thread);
    [[nodiscard]] NarrativeObjectiveState &EnsureObjectiveState(NarrativeObjectiveId objective);
    Revision revision_{};
    bool frozen_ = false;
    MonotonicIdGenerator<GameplayObjectId> journal_ids_{0x3300}, clue_ids_{0x3301}, rumor_ids_{0x3302},
        choice_ids_{0x3303}, consequence_ids_{0x3304};
    std::unordered_map<NarrativeThreadId, NarrativeThreadDefinition, IdHash> thread_defs_;
    std::unordered_map<NarrativeArcId, NarrativeArcDefinition, IdHash> arc_defs_;
    std::unordered_map<NarrativeBeatId, NarrativeBeatDefinition, IdHash> beat_defs_;
    std::unordered_map<NarrativeObjectiveId, NarrativeObjectiveDefinition, IdHash> objective_defs_;
    std::unordered_map<NarrativeConditionId, NarrativeConditionDefinition, IdHash> condition_defs_;
    std::unordered_map<NarrativeConsequenceId, NarrativeConsequenceDefinition, IdHash> consequence_defs_;
    std::unordered_map<StoryletId, StoryletDefinition, IdHash> storylet_defs_;
    std::unordered_map<NarrativeConditionTypeId, const INarrativeConditionResolver *, IdHash> condition_resolvers_;
    std::unordered_map<NarrativeConsequenceTypeId, const INarrativeConsequenceHandler *, IdHash> consequence_handlers_;
    std::unordered_map<NarrativeThreadId, NarrativeThreadState, IdHash> threads_;
    std::unordered_map<NarrativeObjectiveId, NarrativeObjectiveState, IdHash> objectives_;
    std::unordered_map<NarrativeConsequenceExecutionId, NarrativeConsequenceExecution, IdHash> consequences_;
    std::unordered_map<JournalEntryId, JournalEntry, IdHash> journal_;
    std::unordered_map<ClueId, ClueRecord, IdHash> clues_;
    std::unordered_map<RumorId, RumorRecord, IdHash> rumors_;
    std::map<GameplayTimePoint, std::vector<RumorId>> rumor_expiry_index_;
    std::unordered_map<NarrativeFlagId, NarrativeFlag, IdHash> flags_;
    std::unordered_map<NarrativeVariableId, NarrativeVariable, IdHash> variables_;
    std::unordered_map<NarrativeChoiceId, NarrativeChoice, IdHash> choices_;
    std::unordered_map<StoryletId, StoryletRuntimeState, IdHash> storylet_runtime_;
    std::unordered_map<NarrativeConsequenceKey, NarrativeConsequenceExecutionId, NarrativeConsequenceKeyHash>
        consequence_idempotency_;
    std::unordered_map<NarrativeEventKey, NarrativeEventExecution, NarrativeEventKeyHash> event_executions_;
    std::unordered_set<NarrativeEventKey, NarrativeEventKeyHash> processed_event_keys_;
    std::vector<NarrativeBeatId> activated_beats_;
    std::deque<NarrativeChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    mutable NarrativeDiagnostics diagnostics_{};
    mutable std::size_t evaluation_counter_ = 0;
    mutable std::size_t evaluation_limit_ = static_cast<std::size_t>(-1);
    mutable std::size_t evaluation_depth_ = 0;
    mutable bool evaluation_budget_exhausted_ = false;
    static constexpr std::size_t kMaxConditionDepth = 256;
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    static constexpr std::size_t kCompletedEventRetention = 4096;
};
} // namespace epidemic::gameplay::narrative
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `NarrativeRuntimeState`: `Hidden`, `Available`, `Discovered`, `Active`, `Suspended`, `Completed`, `Failed`, `Expired`, `Locked`
- `NarrativeObjectiveRuntimeState`: `Hidden`, `Available`, `Active`, `Completed`, `Failed`, `Cancelled`, `OptionalMissed`
- `ConditionEvaluationState`: `Satisfied`, `Unsatisfied`, `Unknown`, `Unavailable`, `Partial`
- `ConsequenceExecutionState`: `Pending`, `Applied`, `Deferred`, `FailedRetryable`, `FailedPermanent`, `Unsupported`, `AlreadyApplied`
- `JournalVisibilityState`: `Hidden`, `Discovered`, `Updated`, `Resolved`, `Archived`, `Suppressed`
- `RumorState`: `Active`, `Spread`, `Discredited`, `Expired`, `Confirmed`, `Suppressed`
- `NarrativeChoiceState`: `Open`, `Resolved`, `Cancelled`, `Expired`
- `NarrativeEventExecutionState`: `Pending`, `Processing`, `Completed`, `FailedRetryable`
- `NarrativeEventExecutionPhase`: `Beats`, `Objectives`, `Storylets`, `Completed`
- `NarrativeGateState`: `Closed`, `Open`, `Locked`, `Disabled`
- `NarrativeChangeKind`: `ThreadStarted`, `ThreadCompleted`, `ThreadFailed`, `ThreadSuspended`, `ThreadResumed`, `ThreadExpired`, `ObjectiveActivated`, `ObjectiveCompleted`, `ObjectiveFailed`, `ObjectiveCancelled`, `ObjectiveOptionalMissed`, `ObjectiveProgressChanged`, `JournalEntryAdded`, `ClueDiscovered`, `RumorCreated`, `RumorExpired`, `ChoiceCreated`, `ChoiceResolved`, `ChoiceCancelled`, `ChoiceExpired`, `StoryletActivated`, `ConsequencePlanned`, `ConsequenceApplied`, `ConsequenceFailed`

### Состав типов и операции

#### `NarrativeThreadId` (строка заголовка 20)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeThreadId FromString(std::string_view s) noexcept;
static constexpr NarrativeThreadId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeThreadId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeThreadId &) const noexcept = default;
```

#### `NarrativeArcId` (строка заголовка 38)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeArcId FromString(std::string_view s) noexcept;
static constexpr NarrativeArcId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeArcId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeArcId &) const noexcept = default;
```

#### `NarrativeBeatId` (строка заголовка 56)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeBeatId FromString(std::string_view s) noexcept;
static constexpr NarrativeBeatId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeBeatId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeBeatId &) const noexcept = default;
```

#### `NarrativeObjectiveId` (строка заголовка 74)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeObjectiveId FromString(std::string_view s) noexcept;
static constexpr NarrativeObjectiveId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeObjectiveId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeObjectiveId &) const noexcept = default;
```

#### `NarrativeConditionId` (строка заголовка 92)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeConditionId FromString(std::string_view s) noexcept;
static constexpr NarrativeConditionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeConditionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeConditionId &) const noexcept = default;
```

#### `NarrativeConsequenceId` (строка заголовка 110)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeConsequenceId FromString(std::string_view s) noexcept;
static constexpr NarrativeConsequenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeConsequenceId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceId &) const noexcept = default;
```

#### `NarrativeConsequenceExecutionId` (строка заголовка 128)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeConsequenceExecutionId FromString(std::string_view s) noexcept;
static constexpr NarrativeConsequenceExecutionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeConsequenceExecutionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceExecutionId &) const noexcept = default;
```

#### `JournalEntryId` (строка заголовка 146)

Публичные сигнатуры операций:

```cpp
static constexpr JournalEntryId FromString(std::string_view s) noexcept;
static constexpr JournalEntryId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const JournalEntryId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const JournalEntryId &) const noexcept = default;
```

#### `ClueId` (строка заголовка 164)

Публичные сигнатуры операций:

```cpp
static constexpr ClueId FromString(std::string_view s) noexcept;
static constexpr ClueId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ClueId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ClueId &) const noexcept = default;
```

#### `RumorId` (строка заголовка 182)

Публичные сигнатуры операций:

```cpp
static constexpr RumorId FromString(std::string_view s) noexcept;
static constexpr RumorId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const RumorId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const RumorId &) const noexcept = default;
```

#### `StoryletId` (строка заголовка 200)

Публичные сигнатуры операций:

```cpp
static constexpr StoryletId FromString(std::string_view s) noexcept;
static constexpr StoryletId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const StoryletId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const StoryletId &) const noexcept = default;
```

#### `NarrativeGateId` (строка заголовка 218)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeGateId FromString(std::string_view s) noexcept;
static constexpr NarrativeGateId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeGateId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeGateId &) const noexcept = default;
```

#### `NarrativeFlagId` (строка заголовка 236)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeFlagId FromString(std::string_view s) noexcept;
static constexpr NarrativeFlagId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeFlagId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeFlagId &) const noexcept = default;
```

#### `NarrativeVariableId` (строка заголовка 254)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeVariableId FromString(std::string_view s) noexcept;
static constexpr NarrativeVariableId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeVariableId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeVariableId &) const noexcept = default;
```

#### `NarrativeSceneId` (строка заголовка 272)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeSceneId FromString(std::string_view s) noexcept;
static constexpr NarrativeSceneId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeSceneId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeSceneId &) const noexcept = default;
```

#### `NarrativeChoiceId` (строка заголовка 290)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeChoiceId FromString(std::string_view s) noexcept;
static constexpr NarrativeChoiceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeChoiceId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeChoiceId &) const noexcept = default;
```

#### `NarrativeChoiceOptionId` (строка заголовка 308)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeChoiceOptionId FromString(std::string_view s) noexcept;
static constexpr NarrativeChoiceOptionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeChoiceOptionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeChoiceOptionId &) const noexcept = default;
```

#### `NarrativeThreadTypeId` (строка заголовка 326)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeThreadTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeThreadTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeThreadTypeId &) const noexcept = default;
```

#### `NarrativeEventTypeId` (строка заголовка 340)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeEventTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeEventTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeEventTypeId &) const noexcept = default;
```

#### `NarrativeConditionTypeId` (строка заголовка 354)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeConditionTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeConditionTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeConditionTypeId &) const noexcept = default;
```

#### `NarrativeConsequenceTypeId` (строка заголовка 368)

Публичные сигнатуры операций:

```cpp
static constexpr NarrativeConsequenceTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeConsequenceTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceTypeId &) const noexcept = default;
```

#### `JournalEntryTypeId` (строка заголовка 382)

Публичные сигнатуры операций:

```cpp
static constexpr JournalEntryTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const JournalEntryTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const JournalEntryTypeId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 396)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 398)

#### `NarrativeReason` (строка заголовка 523)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`

#### `NarrativeEvent` (строка заголовка 528)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`

#### `NarrativeEventKey` (строка заголовка 540)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const NarrativeEventKey &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeEventKey &) const noexcept = default;
```

#### `NarrativeConsequenceKey` (строка заголовка 552)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const NarrativeConsequenceKey &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const NarrativeConsequenceKey &) const noexcept = default;
```

#### `NarrativeEventKeyHash` (строка заголовка 566)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const NarrativeEventKey &key) const noexcept;
```

#### `NarrativeConsequenceKeyHash` (строка заголовка 586)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const NarrativeConsequenceKey &key) const noexcept;
```

#### `NarrativeConditionResult` (строка заголовка 616)

Публичные поля и значения по умолчанию:

- `ConditionEvaluationState state = ConditionEvaluationState::Unavailable`
- `std::int64_t score = 0`
- `std::vector<NarrativeReason> reasons`

#### `NarrativeConsequenceResult` (строка заголовка 624)

Публичные поля и значения по умолчанию:

- `ConsequenceExecutionState state = ConsequenceExecutionState::Unsupported`

#### `NarrativeConditionDefinition` (строка заголовка 630)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeConditionId> all_of`
- `std::vector<NarrativeConditionId> any_of`
- `std::vector<std::byte> payload`

#### `NarrativeConsequenceDefinition` (строка заголовка 640)

Публичные поля и значения по умолчанию:

- `std::int32_t priority = 0`
- `bool require_success_before_advance = false`
- `std::vector<std::byte> payload`
- `std::optional<JournalTemplate> journal_template`
- `std::optional<RumorTemplate> rumor_template`

#### `JournalTemplate` (строка заголовка 648)

Публичные поля и значения по умолчанию:

- `JournalVisibilityState visibility = JournalVisibilityState::Discovered`
- `std::vector<std::byte> payload`

#### `RumorTemplate` (строка заголовка 655)

Публичные поля и значения по умолчанию:

- `std::int64_t confidence = 0`
- `std::vector<std::byte> payload`

#### `NarrativeBeatDefinition` (строка заголовка 666)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeConditionId> activation_conditions`
- `std::vector<NarrativeConditionId> completion_conditions`
- `std::vector<NarrativeConsequenceId> consequences`
- `std::int32_t order = 0`

#### `NarrativeObjectiveDefinition` (строка заголовка 677)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeConditionId> start_conditions`
- `std::vector<NarrativeConditionId> completion_conditions`
- `std::vector<NarrativeConditionId> failure_conditions`
- `bool player_visible = false`

#### `NarrativeThreadDefinition` (строка заголовка 688)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeBeatId> beats`
- `std::vector<NarrativeObjectiveId> root_objectives`
- `NarrativeRuntimeState initial_state = NarrativeRuntimeState::Hidden`
- `std::int32_t priority = 0`

#### `NarrativeArcDefinition` (строка заголовка 699)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeThreadId> threads`

#### `StoryletDefinition` (строка заголовка 706)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeConditionId> availability_conditions`
- `std::vector<NarrativeConsequenceId> consequences`
- `std::int32_t priority = 0`
- `std::uint32_t max_activations = 1`

#### `NarrativeChoiceOption` (строка заголовка 717)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeConditionId> availability_conditions`
- `std::vector<NarrativeConsequenceId> consequences`
- `std::vector<std::byte> payload`

#### `NarrativeThreadState` (строка заголовка 726)

Публичные поля и значения по умолчанию:

- `NarrativeRuntimeState state = NarrativeRuntimeState::Hidden`

#### `NarrativeObjectiveState` (строка заголовка 736)

Публичные поля и значения по умолчанию:

- `NarrativeObjectiveRuntimeState state = NarrativeObjectiveRuntimeState::Hidden`
- `std::int64_t progress = 0`

#### `NarrativeConsequenceExecution` (строка заголовка 745)

Публичные поля и значения по умолчанию:

- `ConsequenceExecutionState state = ConsequenceExecutionState::Pending`
- `std::uint32_t attempts = 0`

#### `JournalEntry` (строка заголовка 760)

Публичные поля и значения по умолчанию:

- `JournalVisibilityState visibility = JournalVisibilityState::Hidden`
- `std::vector<std::byte> payload`

#### `ClueRecord` (строка заголовка 773)

Публичные поля и значения по умолчанию:

- `std::int64_t confidence = 0`
- `std::vector<std::byte> payload`

#### `RumorRecord` (строка заголовка 785)

Публичные поля и значения по умолчанию:

- `RumorState state = RumorState::Active`
- `std::int64_t confidence = 0`
- `std::vector<std::byte> payload`

#### `NarrativeFlag` (строка заголовка 797)

Публичные поля и значения по умолчанию:

- `bool value = false`

#### `NarrativeChoice` (строка заголовка 812)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeChoiceOption> options`
- `NarrativeChoiceState state = NarrativeChoiceState::Open`
- `std::optional<NarrativeChoiceOptionId> selected_option`

#### `NarrativeEventExecution` (строка заголовка 825)

Публичные поля и значения по умолчанию:

- `NarrativeEventExecutionState state = NarrativeEventExecutionState::Pending`
- `NarrativeEventExecutionPhase phase = NarrativeEventExecutionPhase::Beats`
- `bool work_plan_initialized = false`
- `std::vector<NarrativeBeatId> beat_plan`
- `std::vector<NarrativeObjectiveId> objective_plan`
- `std::vector<StoryletId> storylet_plan`
- `std::size_t next_beat = 0`
- `std::size_t next_objective = 0`
- `std::size_t next_storylet = 0`

#### `StoryletRuntimeState` (строка заголовка 840)

Публичные поля и значения по умолчанию:

- `std::uint32_t activations = 0`

#### `NarrativeChange` (строка заголовка 847)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `NarrativeChangeKind kind = NarrativeChangeKind::EventProcessed`

#### `NarrativeSnapshot` (строка заголовка 864)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeThreadState> thread_states`
- `std::vector<NarrativeObjectiveState> objective_states`
- `std::vector<NarrativeConsequenceExecution> consequences`
- `std::vector<JournalEntry> journal_entries`
- `std::vector<ClueRecord> clues`
- `std::vector<RumorRecord> rumors`
- `std::vector<NarrativeFlag> flags`
- `std::vector<NarrativeVariable> variables`
- `std::vector<NarrativeChoice> choices`
- `std::vector<StoryletRuntimeState> storylet_runtime`
- `std::vector<NarrativeEventExecution> event_executions`
- `std::vector<NarrativeBeatId> activated_beats`
- `std::vector<NarrativeEventKey> processed_event_keys`
- `bool frozen = false`
- `std::uint64_t change_epoch = 1`

#### `NarrativeChangeBatch` (строка заголовка 889)

Публичные поля и значения по умолчанию:

- `std::vector<NarrativeChange> changes`
- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`

#### `INarrativeConditionResolver` (строка заголовка 908)

Публичные сигнатуры операций:

```cpp
virtual ~INarrativeConditionResolver() = default;
[[nodiscard]] virtual NarrativeConditionResult Evaluate(const NarrativeConditionDefinition &definition, const NarrativeEvaluationContext &context) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `INarrativeConsequenceHandler` (строка заголовка 915)

Публичные сигнатуры операций:

```cpp
virtual ~INarrativeConsequenceHandler() = default;
[[nodiscard]] virtual NarrativeConsequenceResult Execute(const NarrativeConsequenceDefinition &definition, const NarrativeConsequenceExecution &execution, const NarrativeExecutionContext &context) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `NarrativeProcessBudget` (строка заголовка 923)

Публичные поля и значения по умолчанию:

- `std::size_t max_condition_evaluations = 1024`
- `std::size_t max_storylets_evaluated = 256`
- `std::size_t max_storylets_activated = 16`
- `std::size_t max_consequences_planned = 256`

#### `NarrativeService` (строка заголовка 931)

Внутреннее состояние (не часть публичного API): `bool frozen_ = false`, `std::unordered_map<NarrativeThreadId, NarrativeThreadDefinition, IdHash> thread_defs_`, `std::unordered_map<NarrativeArcId, NarrativeArcDefinition, IdHash> arc_defs_`, `std::unordered_map<NarrativeBeatId, NarrativeBeatDefinition, IdHash> beat_defs_`, `std::unordered_map<NarrativeObjectiveId, NarrativeObjectiveDefinition, IdHash> objective_defs_`, `std::unordered_map<NarrativeConditionId, NarrativeConditionDefinition, IdHash> condition_defs_`, `std::unordered_map<NarrativeConsequenceId, NarrativeConsequenceDefinition, IdHash> consequence_`, `std::unordered_map<StoryletId, StoryletDefinition, IdHash> storylet_defs_`, `std::unordered_map<NarrativeConditionTypeId, const INarrativeConditionResolver *, IdHash> condi`.

Публичные сигнатуры операций:

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
[[nodiscard]] const NarrativeChoice *GetChoice(NarrativeChoiceId id) const noexcept;
[[nodiscard]] std::vector<JournalEntry> GetJournal(GameplayObjectRef owner) const;
[[nodiscard]] std::vector<NarrativeConsequenceExecution> FindConsequences(ConsequenceExecutionState state) const;
[[nodiscard]] NarrativeChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] std::uint64_t CompactTerminalExecutions(GameplayTimePoint before);
[[nodiscard]] NarrativeSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(NarrativeSnapshot snapshot);
[[nodiscard]] NarrativeDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
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
| `RegisterThreadDefinition` | `NarrativeService::RegisterThreadDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:131` |
| `RegisterArcDefinition` | `NarrativeService::RegisterArcDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:148` |
| `RegisterBeatDefinition` | `NarrativeService::RegisterBeatDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:165` |
| `RegisterObjectiveDefinition` | `NarrativeService::RegisterObjectiveDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:182` |
| `RegisterConditionDefinition` | `NarrativeService::RegisterConditionDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:199` |
| `RegisterConsequenceDefinition` | `NarrativeService::RegisterConsequenceDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:216` |
| `RegisterStoryletDefinition` | `NarrativeService::RegisterStoryletDefinition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:233` |
| `RegisterConditionResolver` | `NarrativeService::RegisterConditionResolver` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:261` |
| `RegisterConsequenceHandler` | `NarrativeService::RegisterConsequenceHandler` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:281` |
| `FreezeDefinitions` | `NarrativeService::FreezeDefinitions` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:301` |
| `EvaluateCondition` | `NarrativeService::EvaluateCondition` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:840` |
| `ExecutePendingConsequences` | `NarrativeService::ExecutePendingConsequences` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:1881` |
| `GetThreadState` | `NarrativeService::GetThreadState` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:2087` |
| `GetObjectiveState` | `NarrativeService::GetObjectiveState` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:2092` |
| `GetJournalEntry` | `NarrativeService::GetJournalEntry` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:2097` |
| `GetClue` | `NarrativeService::GetClue` | `EngineFramework/GameplayWorldStateOwners/Narrative/src/narrative.cpp:2102` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/narrativeintegration_save_participant_tests.cpp:16`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/narrativeintegration_save_participant_tests.cpp:17`
- `RegisterConditionResolver`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:69`
- `RegisterConsequenceHandler`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:70`
- `RegisterConditionDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:83`
- `RegisterConsequenceDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:89`
- `RegisterBeatDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:106`
- `RegisterObjectiveDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:114`
- `RegisterThreadDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:121`
- `FreezeDefinitions`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:122`
- `GetJournal`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:137`
- `GetThreadState`: `EngineFramework/DevelopmentInfrastructure/Tests/narrative_tests.cpp:148`

