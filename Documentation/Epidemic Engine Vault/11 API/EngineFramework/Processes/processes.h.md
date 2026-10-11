---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h"
---

# Processes: processes.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h`.

**Модуль:** [[06 EngineFramework/Модуль Processes|Processes]].
**Размер исходного файла:** 25056 байт.
**Подключение:** `#include <Epidemic/GameFramework/Processes/processes.h>`.

## Объявления типов (лексический индекс)
`ProcessDefinitionId`, `ProcessRecipeId`, `ProcessKindId`, `ProcessStepId`, `ProcessInputId`, `ProcessOutputId`, `ProcessInputTypeId`, `ProcessOutputTypeId`, `ProcessStationId`, `ProcessInstanceId`, `ProcessReservationId`, `ProcessQualityId`, `IdHash`, `RegisteredPayload`, `ProcessTimingPolicy`, `ProcessPersistencePolicy`, `InputConsumptionPolicy`, `OutputDeliveryPolicy`, `ProcessInstanceState`, `StationState`, `ProcessChangeKind`, `ProcessStepDefinition`, `ProcessInputDefinition`, `ProcessOutputDefinition`, `ProcessDefinition`, `ProcessRecipe`, `ProcessStation`, `ProcessQualityResult`, `ProcessInputCommitState`, `ProcessOutputCommitState`, `ReservedProcessInput`, `PreparedProcessOutput`, `ProcessInstance`, `StartProcessRequest`, `ProcessChange`, `ProcessCompletionFailure`, `ProcessBatchCompletionReport`, `ProcessChangeBatch`, `ProcessesSnapshot`, `ProcessesDiagnostics`, `IProcessInputProvider`, `IProcessOutputHandler`, `IProcessQualityProvider`, `ProcessesService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::processes
{
using Fixed = std::int64_t;

struct ProcessDefinitionId
{
    TypeId value{};
    static constexpr ProcessDefinitionId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessDefinitionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessDefinitionId &) const noexcept = default;
};
struct ProcessRecipeId
{
    TypeId value{};
    static constexpr ProcessRecipeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessRecipeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessRecipeId &) const noexcept = default;
};
struct ProcessKindId
{
    TypeId value{};
    static constexpr ProcessKindId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessKindId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessKindId &) const noexcept = default;
};
struct ProcessStepId
{
    TypeId value{};
    static constexpr ProcessStepId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessStepId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessStepId &) const noexcept = default;
};
struct ProcessInputId
{
    TypeId value{};
    static constexpr ProcessInputId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessInputId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessInputId &) const noexcept = default;
};
struct ProcessOutputId
{
    TypeId value{};
    static constexpr ProcessOutputId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessOutputId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessOutputId &) const noexcept = default;
};
struct ProcessInputTypeId
{
    TypeId value{};
    static constexpr ProcessInputTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessInputTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessInputTypeId &) const noexcept = default;
};
struct ProcessOutputTypeId
{
    TypeId value{};
    static constexpr ProcessOutputTypeId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessOutputTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessOutputTypeId &) const noexcept = default;
};
struct ProcessStationId
{
    GameplayObjectId value{};
    static constexpr ProcessStationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessStationId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessStationId &) const noexcept = default;
};
struct ProcessInstanceId
{
    GameplayObjectId value{};
    static constexpr ProcessInstanceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessInstanceId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessInstanceId &) const noexcept = default;
};
struct ProcessReservationId
{
    GameplayObjectId value{};
    static constexpr ProcessReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessReservationId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessReservationId &) const noexcept = default;
};
struct ProcessQualityId
{
    TypeId value{};
    static constexpr ProcessQualityId FromString(std::string_view s) noexcept
    {
        return {TypeId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const ProcessQualityId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ProcessQualityId &) const noexcept = default;
};

struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};

struct RegisteredPayload
{
    TypeId type{};
    std::uint32_t schema_version = 0;
    bool portable = false;
    std::vector<std::byte> bytes;
    template <class T> [[nodiscard]] static RegisteredPayload FromTrivial(TypeId type_id, const T &value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        RegisteredPayload payload;
        payload.type = type_id;
        payload.schema_version = 0;
        payload.portable = false;
        payload.bytes.resize(sizeof(T));
        std::memcpy(payload.bytes.data(), &value, sizeof(T));
        return payload;
    }
    [[nodiscard]] static RegisteredPayload FromVersioned(TypeId type_id, std::uint32_t version, std::vector<std::byte> encoded)
    {
        RegisteredPayload payload;
        payload.type = type_id;
        payload.schema_version = version;
        payload.portable = true;
        payload.bytes = std::move(encoded);
        return payload;
    }
    [[nodiscard]] bool IsPortable() const noexcept { return !type.IsValid() || portable; }
    template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected) const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (type != expected || bytes.size() != sizeof(T))
            return std::nullopt;
        T value{};
        std::memcpy(&value, bytes.data(), sizeof(T));
        return value;
    }
};

enum class ProcessTimingPolicy
{
    Instant,
    Timed,
    ExternalCompletion
};
enum class ProcessPersistencePolicy
{
    Transient,
    Session,
    Persistent
};
enum class InputConsumptionPolicy
{
    ConsumeOnStart,
    ConsumeOnCompletion,
    ReserveThenConsume,
    ToolNotConsumed,
    Catalyst,
    ConditionRequired
};
enum class OutputDeliveryPolicy
{
    Immediate,
    OnCompletion
};
enum class ProcessInstanceState
{
    Prepared,
    Reserved,
    Running,
    Paused,
    Completed,
    Failed,
    Cancelled,
    Expired,
    ReconciliationRequired
};
enum class StationState
{
    Active,
    Disabled,
    Destroyed,
    Occupied,
    Unavailable
};
enum class ProcessChangeKind
{
    Prepared,
    Started,
    Paused,
    Resumed,
    Completed,
    Failed,
    Cancelled,
    InputReserved,
    InputConsumed,
    OutputProduced,
    QualityResolved,
    ReconciliationRequired,
    StationRegistered,
    Pruned
};

struct ProcessStepDefinition
{
    ProcessStepId id{};
    TypeId step_type{};
    GameplayDuration duration{};
    std::vector<TypeId> requirement_ids;
    RegisteredPayload payload;
};
struct ProcessInputDefinition
{
    ProcessInputId id{};
    ProcessInputTypeId type{};
    Fixed amount = 0;
    InputConsumptionPolicy consumption = InputConsumptionPolicy::ReserveThenConsume;
    GameplayTagSet required_tags;
    RegisteredPayload payload;
};
struct ProcessOutputDefinition
{
    ProcessOutputId id{};
    ProcessOutputTypeId type{};
    Fixed amount = 0;
    OutputDeliveryPolicy delivery = OutputDeliveryPolicy::OnCompletion;
    RegisteredPayload payload;
};
struct ProcessDefinition
{
    ProcessDefinitionId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    ProcessKindId kind{};
    std::vector<ProcessStepDefinition> steps;
    ProcessTimingPolicy timing = ProcessTimingPolicy::Instant;
    ProcessPersistencePolicy persistence = ProcessPersistencePolicy::Transient;
    RegisteredPayload payload;
    Revision revision{};
};
struct ProcessRecipe
{
    ProcessRecipeId id{};
    std::string canonical_name;
    ProcessDefinitionId process{};
    std::vector<ProcessInputDefinition> inputs;
    std::vector<ProcessOutputDefinition> outputs;
    std::vector<TypeId> requirements;
    GameplayTagSet station_capabilities;
    RegisteredPayload payload;
    Revision revision{};
};
struct ProcessStation
{
    ProcessStationId id{};
    GameplayObjectRef station_object{};
    GameplayTagSet capabilities;
    StationState state = StationState::Active;
    Fixed efficiency_micro = 1'000'000;
    Revision revision{};
};
struct ProcessQualityResult
{
    ProcessQualityId quality{};
    Fixed quality_score_micro = 1'000'000;
    std::vector<TypeId> reasons;
};
enum class ProcessInputCommitState
{
    Reserved,
    Consumed,
    Released
};
enum class ProcessOutputCommitState
{
    Prepared,
    Committed,
    Cancelled
};
struct ReservedProcessInput
{
    // Framework-owned reservation identity. Providers must keep their own stable identity in
    // provider_token; Processes canonicalizes id after Reserve().
    ProcessReservationId id{};
    ProcessInputId input{};
    ProcessInputTypeId type{};
    Fixed amount = 0;
    InputConsumptionPolicy consumption = InputConsumptionPolicy::ReserveThenConsume;
    ProcessInputCommitState state = ProcessInputCommitState::Reserved;
    RegisteredPayload provider_token;
};
struct PreparedProcessOutput
{
    ProcessOutputId output{};
    ProcessOutputTypeId type{};
    OutputDeliveryPolicy delivery = OutputDeliveryPolicy::OnCompletion;
    ProcessOutputCommitState state = ProcessOutputCommitState::Prepared;
    RegisteredPayload provider_token;
};
struct ProcessInstance
{
    ProcessInstanceId id{};
    ProcessRecipeId recipe{};
    GameplayObjectRef actor{};
    GameplayObjectRef station{};
    GameplayObjectRef target{};
    GameplayObjectRef simulation_area{};
    ProcessInstanceState state = ProcessInstanceState::Prepared;
    GameplayTimePoint started_at{};
    GameplayTimePoint due_at{};
    GameplayTimePoint last_updated_at{};
    std::vector<ReservedProcessInput> reserved_inputs;
    std::vector<PreparedProcessOutput> prepared_outputs;
    GameplayDuration total_duration{};
    GameplayDuration paused_remaining{};
    ProcessQualityResult quality{};
    RegisteredPayload runtime_payload;
    Revision revision{};
};
struct StartProcessRequest
{
    ProcessRecipeId recipe{};
    GameplayObjectRef actor{};
    GameplayObjectRef station{};
    GameplayObjectRef target{};
    GameplayTimePoint now{};
    std::uint64_t seed = 0;
    GameplayContext context{};
    GameplayObjectRef simulation_area{};
};
struct ProcessChange
{
    std::uint64_t sequence = 0;
    ProcessChangeKind kind = ProcessChangeKind::Prepared;
    ProcessInstanceId process{};
    ProcessRecipeId recipe{};
    GameplayObjectRef actor{};
    GameplayTimePoint time{};
    GameplayContext context{};
    Revision revision{};
    ProcessStationId station{};
};
struct ProcessCompletionFailure
{
    ProcessInstanceId process{};
    foundation::Error error{};
};
struct ProcessBatchCompletionReport
{
    std::vector<ProcessInstanceId> completed;
    std::vector<ProcessCompletionFailure> failures;
    [[nodiscard]] bool AllCompleted() const noexcept { return failures.empty(); }
};
struct ProcessChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::uint64_t latest_sequence = 0;
    std::vector<ProcessChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};

struct ProcessesSnapshot
{
    std::vector<ProcessStation> stations;
    std::vector<ProcessInstance> instances;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot station_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot instance_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot reservation_ids{};
    std::uint64_t next_change_sequence = 1;
    Revision revision{};

    std::uint64_t change_epoch = 1;
};
struct ProcessesDiagnostics
{
    std::uint64_t stations = 0;
    std::uint64_t active_processes = 0;
    std::uint64_t completed_processes = 0;
    std::uint64_t failed_processes = 0;
    std::uint64_t reservations = 0;
    std::uint64_t outputs = 0;
};

class IProcessInputProvider
{
  public:
    virtual ~IProcessInputProvider() = default;
    [[nodiscard]] virtual foundation::Result<void> Validate(const ProcessInputDefinition &input,
                                                            const StartProcessRequest &request,
                                                            ProcessInstanceId instance) = 0;
    [[nodiscard]] virtual foundation::Result<ReservedProcessInput> Reserve(const ProcessInputDefinition &input,
                                                                           const StartProcessRequest &request,
                                                                           ProcessInstanceId instance) = 0;
    // Consume and Release must be idempotent for the stable provider token. A successful call may be
    // repeated after restore/reconciliation without applying the mutation twice.
    [[nodiscard]] virtual foundation::Result<void> Consume(const ReservedProcessInput &reservation,
                                                           GameplayContext context) = 0;
    [[nodiscard]] virtual foundation::Result<void> Release(const ReservedProcessInput &reservation,
                                                           GameplayContext context) = 0;
};
class IProcessOutputHandler
{
  public:
    virtual ~IProcessOutputHandler() = default;
    [[nodiscard]] virtual bool Supports(ProcessOutputTypeId type) const noexcept = 0;
    // Prepare is the only stage allowed to fail before delivery. The returned token must be stable
    // and Commit must be idempotent for it so a partially completed process can resume safely.
    [[nodiscard]] virtual foundation::Result<PreparedProcessOutput> Prepare(const ProcessOutputDefinition &output,
                                                                           const ProcessInstance &instance,
                                                                           GameplayContext context) = 0;
    [[nodiscard]] virtual foundation::Result<void> Commit(const PreparedProcessOutput &output,
                                                          const ProcessInstance &instance,
                                                          GameplayContext context) = 0;
    [[nodiscard]] virtual foundation::Result<void> Cancel(const PreparedProcessOutput &output,
                                                          const ProcessInstance &instance,
                                                          GameplayContext context) = 0;
};
class IProcessQualityProvider
{
  public:
    virtual ~IProcessQualityProvider() = default;
    [[nodiscard]] virtual ProcessQualityResult Resolve(const ProcessRecipe &recipe,
                                                       const StartProcessRequest &request) const = 0;
};

class ProcessesService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<ProcessesService>;

  public:
    ProcessesService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.processes");
    }
    [[nodiscard]] foundation::Result<ProcessDefinitionId> RegisterDefinition(ProcessDefinition definition);
    [[nodiscard]] foundation::Result<ProcessRecipeId> RegisterRecipe(ProcessRecipe recipe);
    [[nodiscard]] foundation::Result<ProcessStationId> RegisterStation(ProcessStation station);
    void Freeze() noexcept
    {
        frozen_ = true;
    }
    void SetInputProvider(IProcessInputProvider *provider) noexcept
    {
        input_provider_ = provider;
    }
    void AddOutputHandler(IProcessOutputHandler *handler)
    {
        output_handlers_.push_back(handler);
    }
    void SetQualityProvider(const IProcessQualityProvider *provider) noexcept
    {
        quality_provider_ = provider;
    }

    [[nodiscard]] const ProcessDefinition *FindDefinition(ProcessDefinitionId id) const noexcept;
    [[nodiscard]] const ProcessRecipe *FindRecipe(ProcessRecipeId id) const noexcept;
    [[nodiscard]] const ProcessStation *FindStationByObject(GameplayObjectRef station) const noexcept;
    [[nodiscard]] const ProcessInstance *FindInstance(ProcessInstanceId id) const noexcept;
    [[nodiscard]] Fixed EvaluateProgress(ProcessInstanceId id, GameplayTimePoint now) const noexcept;

    [[nodiscard]] foundation::Result<ProcessInstanceId> StartProcess(StartProcessRequest request);
    [[nodiscard]] foundation::Result<void> Pause(ProcessInstanceId id, GameplayTimePoint now,
                                                 GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Resume(ProcessInstanceId id, GameplayTimePoint now,
                                                  GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Cancel(ProcessInstanceId id, GameplayTimePoint now,
                                                  GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> Complete(ProcessInstanceId id, GameplayTimePoint now,
                                                    GameplayContext context = {});
    [[nodiscard]] foundation::Result<ProcessBatchCompletionReport> CompleteDue(GameplayTimePoint now);
    [[nodiscard]] std::vector<ProcessInstance> FindDueProcessesForSimulation(GameplayObjectRef simulation_area,
                                                                              GameplayTimePoint from,
                                                                              GameplayTimePoint to) const;
    [[nodiscard]] foundation::Result<ProcessBatchCompletionReport> CompletePreparedDueForSimulation(
        GameplayObjectRef simulation_area, std::span<const ProcessInstanceId> process_ids, GameplayTimePoint now,
        GameplayContext context = {});

    [[nodiscard]] std::vector<ProcessInstance> FindProcessesByActor(GameplayObjectRef actor) const;
    [[nodiscard]] std::vector<ProcessInstance> FindProcessesByStation(GameplayObjectRef station) const;
    private:
        [[nodiscard]] std::vector<ProcessChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] ProcessChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] ProcessChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] foundation::Result<void> PruneTerminalProcesses(std::size_t keep_recent = 0);
    [[nodiscard]] ProcessesSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(ProcessesSnapshot snapshot);
    [[nodiscard]] ProcessesDiagnostics GetDiagnostics() const noexcept;

  private:
    [[nodiscard]] bool CanAdvanceRevision(std::size_t count = 1) const noexcept;
    [[nodiscard]] bool CanRecordChanges(std::size_t count = 1) const noexcept;
    void Bump() noexcept { ++revision_.value; }
    void Record(ProcessChange change);
    [[nodiscard]] foundation::Result<void> ReserveInputs(ProcessInstance &instance, const ProcessRecipe &recipe,
                                                         const StartProcessRequest &request);
    [[nodiscard]] foundation::Result<void> ConsumeInputs(ProcessInstance &instance, InputConsumptionPolicy phase,
                                                         GameplayContext context);
    [[nodiscard]] foundation::Result<void> ReleaseInputs(ProcessInstance &instance, GameplayContext context);
    [[nodiscard]] foundation::Result<void> PrepareOutputs(ProcessInstance &instance, const ProcessRecipe &recipe,
                                                          OutputDeliveryPolicy phase, GameplayContext context);
    [[nodiscard]] foundation::Result<void> CommitOutputs(ProcessInstance &instance, OutputDeliveryPolicy phase,
                                                         GameplayContext context);
    [[nodiscard]] foundation::Result<void> CancelPreparedOutputs(ProcessInstance &instance, GameplayContext context);
    [[nodiscard]] foundation::Result<void> ValidateProviderInput(const ProcessInputDefinition &input,
                                                                 const StartProcessRequest &request,
                                                                 ProcessInstanceId instance) const;
    [[nodiscard]] GameplayDuration DurationFor(const ProcessRecipe &recipe, const ProcessDefinition &definition,
                                               const ProcessStation *station) const noexcept;

    bool frozen_ = false;
    Revision revision_{};
    std::unordered_map<ProcessDefinitionId, ProcessDefinition, IdHash> definitions_;
    std::unordered_map<ProcessRecipeId, ProcessRecipe, IdHash> recipes_;
    std::unordered_map<ProcessStationId, ProcessStation, IdHash> stations_;
    std::unordered_map<GameplayObjectRef, ProcessStationId> station_by_object_;
    std::unordered_map<ProcessInstanceId, ProcessInstance, IdHash> instances_;
    MonotonicIdGenerator<GameplayObjectId> station_ids_;
    MonotonicIdGenerator<GameplayObjectId> instance_ids_;
    MonotonicIdGenerator<GameplayObjectId> reservation_ids_;
    IProcessInputProvider *input_provider_ = nullptr;
    std::vector<IProcessOutputHandler *> output_handlers_;
    const IProcessQualityProvider *quality_provider_ = nullptr;
    std::deque<ProcessChange> changes_;
    std::size_t change_journal_capacity_ = 4096;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    ProcessesDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::processes
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `ProcessTimingPolicy`: `Instant`, `Timed`, `ExternalCompletion`
- `ProcessPersistencePolicy`: `Transient`, `Session`, `Persistent`
- `InputConsumptionPolicy`: `ConsumeOnStart`, `ConsumeOnCompletion`, `ReserveThenConsume`, `ToolNotConsumed`, `Catalyst`, `ConditionRequired`
- `OutputDeliveryPolicy`: `Immediate`, `OnCompletion`
- `ProcessInstanceState`: `Prepared`, `Reserved`, `Running`, `Paused`, `Completed`, `Failed`, `Cancelled`, `Expired`, `ReconciliationRequired`
- `StationState`: `Active`, `Disabled`, `Destroyed`, `Occupied`, `Unavailable`
- `ProcessChangeKind`: `Prepared`, `Started`, `Paused`, `Resumed`, `Completed`, `Failed`, `Cancelled`, `InputReserved`, `InputConsumed`, `OutputProduced`, `QualityResolved`, `ReconciliationRequired`, `StationRegistered`, `Pruned`
- `ProcessInputCommitState`: `Reserved`, `Consumed`, `Released`
- `ProcessOutputCommitState`: `Prepared`, `Committed`, `Cancelled`

### Состав типов и операции

#### `ProcessDefinitionId` (строка заголовка 23)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessDefinitionId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessDefinitionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessDefinitionId &) const noexcept = default;
```

#### `ProcessRecipeId` (строка заголовка 37)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessRecipeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessRecipeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessRecipeId &) const noexcept = default;
```

#### `ProcessKindId` (строка заголовка 51)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessKindId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessKindId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessKindId &) const noexcept = default;
```

#### `ProcessStepId` (строка заголовка 65)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessStepId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessStepId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessStepId &) const noexcept = default;
```

#### `ProcessInputId` (строка заголовка 79)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessInputId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessInputId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessInputId &) const noexcept = default;
```

#### `ProcessOutputId` (строка заголовка 93)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessOutputId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessOutputId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessOutputId &) const noexcept = default;
```

#### `ProcessInputTypeId` (строка заголовка 107)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessInputTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessInputTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessInputTypeId &) const noexcept = default;
```

#### `ProcessOutputTypeId` (строка заголовка 121)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessOutputTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessOutputTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessOutputTypeId &) const noexcept = default;
```

#### `ProcessStationId` (строка заголовка 135)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessStationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessStationId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessStationId &) const noexcept = default;
```

#### `ProcessInstanceId` (строка заголовка 149)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessInstanceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessInstanceId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessInstanceId &) const noexcept = default;
```

#### `ProcessReservationId` (строка заголовка 163)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessReservationId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessReservationId &) const noexcept = default;
```

#### `ProcessQualityId` (строка заголовка 177)

Публичные сигнатуры операций:

```cpp
static constexpr ProcessQualityId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ProcessQualityId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ProcessQualityId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 192)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 194)

#### `RegisteredPayload` (строка заголовка 200)

Публичные поля и значения по умолчанию:

- `std::uint32_t schema_version = 0`
- `bool portable = false`
- `std::vector<std::byte> bytes`

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] static RegisteredPayload FromTrivial(TypeId type_id, const T &value);
[[nodiscard]] static RegisteredPayload FromVersioned(TypeId type_id, std::uint32_t version, std::vector<std::byte> encoded);
[[nodiscard]] bool IsPortable() const noexcept;
template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId expected) const;
```

#### `T` (строка заголовка 206)

#### `T` (строка заголовка 227)

#### `ProcessStepDefinition` (строка заголовка 302)

Публичные поля и значения по умолчанию:

- `std::vector<TypeId> requirement_ids`
- `RegisteredPayload payload`

#### `ProcessInputDefinition` (строка заголовка 310)

Публичные поля и значения по умолчанию:

- `Fixed amount = 0`
- `InputConsumptionPolicy consumption = InputConsumptionPolicy::ReserveThenConsume`
- `GameplayTagSet required_tags`
- `RegisteredPayload payload`

#### `ProcessOutputDefinition` (строка заголовка 319)

Публичные поля и значения по умолчанию:

- `Fixed amount = 0`
- `OutputDeliveryPolicy delivery = OutputDeliveryPolicy::OnCompletion`
- `RegisteredPayload payload`

#### `ProcessDefinition` (строка заголовка 327)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `std::vector<ProcessStepDefinition> steps`
- `ProcessTimingPolicy timing = ProcessTimingPolicy::Instant`
- `ProcessPersistencePolicy persistence = ProcessPersistencePolicy::Transient`
- `RegisteredPayload payload`

#### `ProcessRecipe` (строка заголовка 339)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::vector<ProcessInputDefinition> inputs`
- `std::vector<ProcessOutputDefinition> outputs`
- `std::vector<TypeId> requirements`
- `GameplayTagSet station_capabilities`
- `RegisteredPayload payload`

#### `ProcessStation` (строка заголовка 351)

Публичные поля и значения по умолчанию:

- `GameplayTagSet capabilities`
- `StationState state = StationState::Active`
- `Fixed efficiency_micro = 1'000'000`

#### `ProcessQualityResult` (строка заголовка 360)

Публичные поля и значения по умолчанию:

- `Fixed quality_score_micro = 1'000'000`
- `std::vector<TypeId> reasons`

#### `ReservedProcessInput` (строка заголовка 378)

Публичные поля и значения по умолчанию:

- `Fixed amount = 0`
- `InputConsumptionPolicy consumption = InputConsumptionPolicy::ReserveThenConsume`
- `ProcessInputCommitState state = ProcessInputCommitState::Reserved`
- `RegisteredPayload provider_token`

Публичные сигнатуры операций:

```cpp
Processes canonicalizes id after Reserve(). ProcessReservationId id;
```

#### `PreparedProcessOutput` (строка заголовка 390)

Публичные поля и значения по умолчанию:

- `OutputDeliveryPolicy delivery = OutputDeliveryPolicy::OnCompletion`
- `ProcessOutputCommitState state = ProcessOutputCommitState::Prepared`
- `RegisteredPayload provider_token`

#### `ProcessInstance` (строка заголовка 398)

Публичные поля и значения по умолчанию:

- `ProcessInstanceState state = ProcessInstanceState::Prepared`
- `std::vector<ReservedProcessInput> reserved_inputs`
- `std::vector<PreparedProcessOutput> prepared_outputs`
- `RegisteredPayload runtime_payload`

#### `StartProcessRequest` (строка заголовка 418)

Публичные поля и значения по умолчанию:

- `std::uint64_t seed = 0`

#### `ProcessChange` (строка заголовка 429)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `ProcessChangeKind kind = ProcessChangeKind::Prepared`

#### `ProcessBatchCompletionReport` (строка заголовка 446)

Публичные поля и значения по умолчанию:

- `std::vector<ProcessInstanceId> completed`
- `std::vector<ProcessCompletionFailure> failures`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool AllCompleted() const noexcept;
```

#### `ProcessChangeBatch` (строка заголовка 452)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::uint64_t latest_sequence = 0`
- `std::vector<ProcessChange> changes`

#### `ProcessesSnapshot` (строка заголовка 463)

Публичные поля и значения по умолчанию:

- `std::vector<ProcessStation> stations`
- `std::vector<ProcessInstance> instances`
- `std::uint64_t next_change_sequence = 1`
- `std::uint64_t change_epoch = 1`

#### `ProcessesDiagnostics` (строка заголовка 475)

Публичные поля и значения по умолчанию:

- `std::uint64_t stations = 0`
- `std::uint64_t active_processes = 0`
- `std::uint64_t completed_processes = 0`
- `std::uint64_t failed_processes = 0`
- `std::uint64_t reservations = 0`
- `std::uint64_t outputs = 0`

#### `IProcessInputProvider` (строка заголовка 485)

Публичные сигнатуры операций:

```cpp
virtual ~IProcessInputProvider() = default;
[[nodiscard]] virtual foundation::Result<void> Validate(const ProcessInputDefinition &input, const StartProcessRequest &request, ProcessInstanceId instance) = 0;
[[nodiscard]] virtual foundation::Result<ReservedProcessInput> Reserve(const ProcessInputDefinition &input, const StartProcessRequest &request, ProcessInstanceId instance) = 0;
[[nodiscard]] virtual foundation::Result<void> Release(const ReservedProcessInput &reservation, GameplayContext context) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IProcessOutputHandler` (строка заголовка 502)

Публичные сигнатуры операций:

```cpp
virtual ~IProcessOutputHandler() = default;
[[nodiscard]] virtual bool Supports(ProcessOutputTypeId type) const noexcept = 0;
[[nodiscard]] virtual foundation::Result<void> Commit(const PreparedProcessOutput &output, const ProcessInstance &instance, GameplayContext context) = 0;
[[nodiscard]] virtual foundation::Result<void> Cancel(const PreparedProcessOutput &output, const ProcessInstance &instance, GameplayContext context) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IProcessQualityProvider` (строка заголовка 519)

Публичные сигнатуры операций:

```cpp
virtual ~IProcessQualityProvider() = default;
[[nodiscard]] virtual ProcessQualityResult Resolve(const ProcessRecipe &recipe, const StartProcessRequest &request) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ProcessesService` (строка заголовка 527)

Внутреннее состояние (не часть публичного API): `bool frozen_ = false`, `std::unordered_map<ProcessDefinitionId, ProcessDefinition, IdHash> definitions_`, `std::unordered_map<ProcessRecipeId, ProcessRecipe, IdHash> recipes_`, `std::unordered_map<ProcessStationId, ProcessStation, IdHash> stations_`, `std::unordered_map<GameplayObjectRef, ProcessStationId> station_by_object_`, `std::unordered_map<ProcessInstanceId, ProcessInstance, IdHash> instances_`, `MonotonicIdGenerator<GameplayObjectId> station_ids_`, `MonotonicIdGenerator<GameplayObjectId> instance_ids_`, `MonotonicIdGenerator<GameplayObjectId> reservation_ids_`.

Публичные сигнатуры операций:

```cpp
ProcessesService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<ProcessDefinitionId> RegisterDefinition(ProcessDefinition definition);
[[nodiscard]] foundation::Result<ProcessRecipeId> RegisterRecipe(ProcessRecipe recipe);
[[nodiscard]] foundation::Result<ProcessStationId> RegisterStation(ProcessStation station);
void Freeze() noexcept;
void SetInputProvider(IProcessInputProvider *provider) noexcept;
void AddOutputHandler(IProcessOutputHandler *handler);
void SetQualityProvider(const IProcessQualityProvider *provider) noexcept;
[[nodiscard]] const ProcessDefinition *FindDefinition(ProcessDefinitionId id) const noexcept;
[[nodiscard]] const ProcessRecipe *FindRecipe(ProcessRecipeId id) const noexcept;
[[nodiscard]] const ProcessStation *FindStationByObject(GameplayObjectRef station) const noexcept;
[[nodiscard]] const ProcessInstance *FindInstance(ProcessInstanceId id) const noexcept;
[[nodiscard]] Fixed EvaluateProgress(ProcessInstanceId id, GameplayTimePoint now) const noexcept;
[[nodiscard]] foundation::Result<ProcessInstanceId> StartProcess(StartProcessRequest request);
[[nodiscard]] foundation::Result<ProcessBatchCompletionReport> CompleteDue(GameplayTimePoint now);
[[nodiscard]] std::vector<ProcessInstance> FindDueProcessesForSimulation(GameplayObjectRef simulation_area, GameplayTimePoint from, GameplayTimePoint to) const;
[[nodiscard]] std::vector<ProcessInstance> FindProcessesByActor(GameplayObjectRef actor) const;
[[nodiscard]] std::vector<ProcessInstance> FindProcessesByStation(GameplayObjectRef station) const;
[[nodiscard]] ProcessChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] foundation::Result<void> PruneTerminalProcesses(std::size_t keep_recent = 0);
[[nodiscard]] ProcessesSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(ProcessesSnapshot snapshot);
[[nodiscard]] ProcessesDiagnostics GetDiagnostics() const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `ProcessesService` | `ProcessesService::ProcessesService` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:132` |
| `RegisterDefinition` | `ProcessesService::RegisterDefinition` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:145` |
| `RegisterRecipe` | `ProcessesService::RegisterRecipe` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:180` |
| `RegisterStation` | `ProcessesService::RegisterStation` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:217` |
| `FindDefinition` | `ProcessesService::FindDefinition` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:236` |
| `FindRecipe` | `ProcessesService::FindRecipe` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:241` |
| `FindStationByObject` | `ProcessesService::FindStationByObject` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:246` |
| `FindInstance` | `ProcessesService::FindInstance` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:254` |
| `EvaluateProgress` | `ProcessesService::EvaluateProgress` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:260` |
| `StartProcess` | `ProcessesService::StartProcess` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:620` |
| `Cancel` | `ProcessesService::Cancel` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:653` |
| `CompleteDue` | `ProcessesService::CompleteDue` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:721` |
| `FindDueProcessesForSimulation` | `ProcessesService::FindDueProcessesForSimulation` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:726` |
| `FindProcessesByActor` | `ProcessesService::FindProcessesByActor` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:754` |
| `FindProcessesByStation` | `ProcessesService::FindProcessesByStation` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:766` |
| `PruneTerminalProcesses` | `ProcessesService::PruneTerminalProcesses` | `EngineFramework/GameplayWorldStateOwners/Processes/src/processes.cpp:812` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:188`
- `RegisterRecipe`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:214`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:219`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:239`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:241`
- `SetInputProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:244`
- `StartProcess`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:246`
- `FindProcessesByActor`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:247`
- `FindInstance`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:263`
- `Cancel`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:308`
- `AddOutputHandler`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:322`
- `EvaluateProgress`: `EngineFramework/DevelopmentInfrastructure/Tests/processes_tests.cpp:425`

