---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h"
---

# Construction: construction.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Construction/include/Epidemic/GameFramework/Construction/construction.h`.

**Модуль:** [[06 EngineFramework/Модуль Construction|Construction]].
**Размер исходного файла:** 27905 байт.
**Подключение:** `#include <Epidemic/GameFramework/Construction/construction.h>`.

## Объявления типов (лексический индекс)
`PlacementRuleId`, `PlacementPlanId`, `PlacementExecutionId`, `ConstructionRecipeId`, `ConstructionSiteId`, `PlacementSocketId`, `PlacedObjectId`, `ConstructionSocketReservationId`, `PlacementOutputId`, `ConstructionCostTypeId`, `PlacementOutputTypeId`, `PlacementReasonId`, `IdHash`, `RefHash`, `WorldPosition`, `OrientationFixed`, `WorldVolumeRef`, `PlacementTargetKind`, `PlacementAvailability`, `PlacementCommitPolicy`, `ConstructionCostPolicy`, `ConstructionSiteState`, `PlacementPlanState`, `SocketState`, `ConstructionChangeKind`, `PlacementDefinition`, `ConstructionCost`, `PlacementOutputTemplate`, `ConstructionRecipe`, `PlacementTarget`, `PlacementFootprint`, `PlacementReason`, `PlacementSemanticProjection`, `PlacementValidationResult`, `PlacementPlan`, `PlacementOutputOperation`, `PlacementOutputEnvelope`, `PlacementOutputDeadLetter`, `PlacementCommitResult`, `PlacementSocket`, `ConstructionSocketReservation`, `ConstructionSite`, `PlacedObjectRecord`, `PlacementRequest`, `ConstructionCostReservation`, `IConstructionPlacementProvider`, `IConstructionCostProvider`, `ConstructionChange`, `ConstructionChangeBatch`, `ConstructionSnapshot`, `ConstructionDiagnostics`, `ConstructionService`

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
#include <limits>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::construction
{
struct PlacementRuleId
{
    TypeId value{};
    static constexpr PlacementRuleId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementRuleId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementRuleId &) const noexcept = default;
};
struct PlacementPlanId
{
    GameplayObjectId value{};
    static constexpr PlacementPlanId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    static constexpr PlacementPlanId FromString(std::string_view s) noexcept { return {GameplayObjectId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementPlanId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementPlanId &) const noexcept = default;
};
struct PlacementExecutionId
{
    GameplayObjectId value{};
    static constexpr PlacementExecutionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    static constexpr PlacementExecutionId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementExecutionId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementExecutionId &) const noexcept = default;
};
struct ConstructionRecipeId
{
    TypeId value{};
    static constexpr ConstructionRecipeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const ConstructionRecipeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ConstructionRecipeId &) const noexcept = default;
};
struct ConstructionSiteId
{
    GameplayObjectId value{};
    static constexpr ConstructionSiteId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const ConstructionSiteId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ConstructionSiteId &) const noexcept = default;
};
struct PlacementSocketId
{
    GameplayObjectId value{};
    static constexpr PlacementSocketId FromString(std::string_view s) noexcept
    {
        return {GameplayObjectId::FromString(s)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementSocketId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementSocketId &) const noexcept = default;
};
struct PlacedObjectId
{
    GameplayObjectId value{};
    static constexpr PlacedObjectId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacedObjectId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacedObjectId &) const noexcept = default;
};
struct ConstructionSocketReservationId
{
    GameplayObjectId value{};
    static constexpr ConstructionSocketReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const ConstructionSocketReservationId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ConstructionSocketReservationId &) const noexcept = default;
};
struct PlacementOutputId
{
    GameplayObjectId value{};
    static constexpr PlacementOutputId FromRaw(std::uint64_t h, std::uint64_t l) noexcept
    {
        return {GameplayObjectId::FromRaw(h, l)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementOutputId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementOutputId &) const noexcept = default;
};
struct ConstructionCostTypeId
{
    TypeId value{};
    static constexpr ConstructionCostTypeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const ConstructionCostTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ConstructionCostTypeId &) const noexcept = default;
};
struct PlacementOutputTypeId
{
    TypeId value{};
    static constexpr PlacementOutputTypeId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementOutputTypeId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementOutputTypeId &) const noexcept = default;
};
struct PlacementReasonId
{
    TypeId value{};
    static constexpr PlacementReasonId FromString(std::string_view s) noexcept { return {TypeId::FromString(s)}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const PlacementReasonId &) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const PlacementReasonId &) const noexcept = default;
};
struct IdHash
{
    template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept
    {
        return std::hash<decltype(id.value)>{}(id.value);
    }
};
struct RefHash
{
    [[nodiscard]] std::size_t operator()(GameplayObjectRef ref) const noexcept { return std::hash<GameplayObjectRef>{}(ref); }
};
using Fixed = std::int64_t;

struct WorldPosition
{
    Fixed x_mm = 0, y_mm = 0, z_mm = 0;
    [[nodiscard]] constexpr bool operator==(const WorldPosition &) const noexcept = default;
};
struct OrientationFixed
{
    Fixed yaw_micro = 0, pitch_micro = 0, roll_micro = 0;
    [[nodiscard]] constexpr bool operator==(const OrientationFixed &) const noexcept = default;
};
struct WorldVolumeRef
{
    GameplayObjectRef area{};
    WorldPosition min{};
    WorldPosition max{};
    [[nodiscard]] constexpr bool IsValid() const noexcept { return area.IsValid() || min != max; }
};

enum class PlacementTargetKind
{
    Free,
    Surface,
    Socket,
    Attach,
    Area
};
enum class PlacementAvailability
{
    Available,
    Unavailable,
    Blocked,
    RequiresResource,
    RequiresPermission,
    RequiresCapability,
    InvalidTarget,
    NotMaterialized,
    Unsupported
};
enum class PlacementCommitPolicy
{
    Instant,
    CreateSite,
    PreviewOnly
};
enum class ConstructionCostPolicy
{
    ReserveThenCommit,
    Free
};
enum class ConstructionSiteState
{
    Planned,
    UnderConstruction,
    Paused,
    Completed,
    Cancelled,
    Failed,
    ResourcesCommitted,
    Destroyed
};
enum class PlacementPlanState
{
    Prepared,
    Committed,
    Cancelled,
    Expired
};
enum class SocketState
{
    Free,
    Occupied,
    Disabled,
    Reserved
};
enum class ConstructionChangeKind
{
    PlacementValidated,
    PlacementRejected,
    PlacementCommitted,
    PlacedObjectCreated,
    SiteCreated,
    SiteStarted,
    SiteCompleted,
    SiteCancelled,
    SocketReserved,
    SocketReleased,
    SocketOccupied,
    SitePaused,
    SiteResumed,
    SiteDestroyed,
    OutputQueued,
    OutputAcknowledged,
    OutputDeadLettered,
    PlacementPlanCancelled,
    PlacementPlanExpired,
    PlacementStateCompacted,
    SocketRegistered,
    SiteProgressed,
    SiteFailed
};

struct PlacementDefinition
{
    PlacementRuleId id{};
    std::string canonical_name;
    GameplayTagSet tags;
    PlacementTargetKind target_kind = PlacementTargetKind::Free;
    PlacementCommitPolicy commit_policy = PlacementCommitPolicy::Instant;
    bool allow_pitch = true;
    bool allow_roll = true;
    Fixed yaw_step_micro = 0;
    bool requires_permission = false;
    bool requires_capability = false;
};
struct ConstructionCost
{
    ConstructionCostTypeId type{};
    Fixed amount_micro = 0;
    std::vector<std::byte> payload;
};
struct PlacementOutputTemplate
{
    PlacementOutputTypeId type{};
    TypeId archetype{};
    bool subject_is_target_area = false;
    std::vector<std::byte> payload;
};
struct ConstructionRecipe
{
    ConstructionRecipeId id{};
    std::string canonical_name;
    TypeId result_entity_archetype{};
    GameplayTagSet required_surface_tags;
    GameplayTagSet blocked_area_tags;
    GameplayTagSet placement_tags;
    WorldPosition footprint_size_mm{};
    std::vector<ConstructionCost> costs;
    ConstructionCostPolicy cost_policy = ConstructionCostPolicy::ReserveThenCommit;
    std::vector<PlacementOutputTemplate> output_templates;
    std::vector<std::byte> placement_payload;
};
struct PlacementTarget
{
    std::optional<WorldPosition> position{};
    std::optional<GameplayObjectRef> area{};
    std::optional<GameplayObjectRef> attach_to{};
    std::optional<PlacementSocketId> socket{};
    OrientationFixed orientation{};
};
struct PlacementFootprint
{
    WorldVolumeRef volume{};
    GameplayTagSet footprint_tags;
    std::vector<std::byte> payload;
};
struct PlacementReason
{
    PlacementReasonId id{};
};
struct PlacementSemanticProjection
{
    bool materialized = true;
    bool area_allowed = true;
    bool permission_granted = true;
    bool capability_available = true;
    bool spacing_clear = true;
    GameplayTagSet surface_tags;
    GameplayTagSet area_tags;
    PlacementFootprint footprint{};
    Revision revision{};
};
struct PlacementValidationResult
{
    PlacementAvailability availability = PlacementAvailability::Unavailable;
    std::vector<PlacementReason> reasons;
    PlacementFootprint footprint{};
    Revision dependencies_revision{};
};
struct PlacementPlan
{
    PlacementPlanId id{};
    GameplayObjectRef actor{};
    ConstructionRecipeId recipe{};
    PlacementTarget target{};
    PlacementFootprint footprint{};
    std::vector<ConstructionCost> reserved_costs;
    Revision dependency_revision{};
    GameplayContext context{};
    PlacementPlanState state = PlacementPlanState::Prepared;
    std::optional<PlacementSocketId> dependency_socket{};
    std::optional<ConstructionSocketReservationId> dependency_socket_reservation{};
    Revision dependency_socket_revision{};
    PlacementRuleId placement_rule{};
    std::uint64_t placement_provider_epoch = 0;
    std::uint64_t cost_provider_epoch = 0;
    GameplayTimePoint prepared_at{};
    GameplayTimePoint expires_at{};
};
struct PlacementOutputOperation
{
    PlacementOutputTypeId type{};
    GameplayObjectRef subject{};
    TypeId archetype{};
    PlacedObjectId placed_record{};
    std::vector<std::byte> payload;
};
struct PlacementOutputEnvelope
{
    PlacementOutputId id{};
    PlacementExecutionId execution{};
    std::uint32_t ordinal = 0;
    PlacementOutputOperation operation{};
};
struct PlacementOutputDeadLetter
{
    PlacementOutputId id{};
    PlacementExecutionId execution{};
    PlacementReasonId reason{};
    GameplayContext context{};
};
struct PlacementCommitResult
{
    PlacementExecutionId execution{};
    PlacedObjectId placed_object{};
    ConstructionSiteId site{};
    std::vector<PlacementOutputOperation> outputs;
    Revision revision{};
};
struct PlacementSocket
{
    PlacementSocketId id{};
    GameplayObjectRef owner{};
    GameplayTagSet accepted_tags;
    SocketState state = SocketState::Free;
    Revision revision{};
};
struct ConstructionSocketReservation
{
    ConstructionSocketReservationId id{};
    PlacementSocketId socket{};
    GameplayObjectRef owner{};
    Revision socket_revision{};
    GameplayContext context{};
};
struct ConstructionSite
{
    ConstructionSiteId id{};
    GameplayObjectRef actor{};
    ConstructionRecipeId recipe{};
    PlacementTarget target{};
    ConstructionSiteState state = ConstructionSiteState::Planned;
    GameplayTimePoint started_at{};
    Fixed progress_micro = 0;
    Revision revision{};
    PlacementPlanId plan{};
    PlacementExecutionId completion_execution{};
    PlacedObjectId placed_object{};
    PlacementReasonId terminal_reason{};
};
struct PlacedObjectRecord
{
    PlacedObjectId id{};
    PlacementPlanId plan{};
    PlacementExecutionId execution{};
};
struct PlacementRequest
{
    GameplayObjectRef actor{};
    ConstructionRecipeId recipe{};
    PlacementRuleId placement_rule{};
    PlacementTarget target{};
    std::optional<ConstructionSocketReservationId> socket_reservation{};
    GameplayContext context{};
};
struct ConstructionCostReservation
{
    GameplayObjectId token{};
};

class IConstructionPlacementProvider
{
  public:
    virtual ~IConstructionPlacementProvider() = default;
    [[nodiscard]] virtual foundation::Result<PlacementSemanticProjection> Project(
        const PlacementRequest &request) const = 0;
    [[nodiscard]] virtual Revision CurrentRevision() const noexcept = 0;
};
class IConstructionCostProvider
{
  public:
    virtual ~IConstructionCostProvider() = default;
    [[nodiscard]] virtual foundation::Result<bool> CanAfford(GameplayObjectRef actor, const ConstructionCost &cost,
                                                             const GameplayContext &context) const = 0;
    [[nodiscard]] virtual foundation::Result<ConstructionCostReservation> Reserve(GameplayObjectRef actor,
                                                                                   const ConstructionCost &cost,
                                                                                   const GameplayContext &context) = 0;
    virtual void Commit(const ConstructionCostReservation &reservation, const GameplayContext &context) noexcept = 0;
    virtual void Release(const ConstructionCostReservation &reservation, const GameplayContext &context) noexcept = 0;
};

struct ConstructionChange
{
    std::uint64_t sequence = 0;
    ConstructionChangeKind kind = ConstructionChangeKind::PlacementValidated;
    PlacementPlanId plan{};
    PlacementExecutionId execution{};
    ConstructionSiteId site{};
    PlacementSocketId socket{};
    GameplayObjectRef actor{};
    Revision revision{};
    GameplayContext context{};
    PlacementOutputId output{};
};
struct ConstructionChangeBatch
{
    bool snapshot_required = false;
    std::uint64_t oldest_available_sequence = 0;
    std::vector<ConstructionChange> changes;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct ConstructionSnapshot
{
    std::vector<PlacementPlan> plans;
    std::vector<ConstructionSite> sites;
    std::vector<PlacementSocket> sockets;
    std::vector<ConstructionSocketReservation> socket_reservations;
    std::vector<PlacedObjectRecord> placed_objects;
    std::vector<PlacementOutputEnvelope> pending_outputs;
    std::vector<PlacementOutputDeadLetter> dead_letters;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot plan_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot site_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot placed_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot execution_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot output_ids{};
    MonotonicIdGenerator<GameplayObjectId>::Snapshot socket_reservation_ids{};
    Revision revision{};
    std::vector<ConstructionChange> journal;
    std::uint64_t next_change_sequence = 1;

    std::uint64_t change_epoch = 1;
};
struct ConstructionDiagnostics
{
    std::uint64_t recipes = 0, placement_definitions = 0, validations = 0, rejections = 0, committed = 0,
                  active_sites = 0, socket_reservations = 0, completed_sites = 0, pending_outputs = 0,
                  dead_lettered_outputs = 0;
};

class ConstructionService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<ConstructionService>;

  public:
    ConstructionService();
    [[nodiscard]] static constexpr GameplayDomainId Domain() noexcept
    {
        return GameplayDomainId::FromString("framework.construction");
    }
    [[nodiscard]] foundation::Result<void> RegisterPlacementDefinition(PlacementDefinition definition);
    [[nodiscard]] foundation::Result<void> RegisterRecipe(ConstructionRecipe recipe);
    [[nodiscard]] foundation::Result<void> RegisterSocket(PlacementSocket socket);
    void Freeze() noexcept { frozen_ = true; }
    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }
    void SetPlacementProvider(const IConstructionPlacementProvider *provider) noexcept;
    void SetCostProvider(IConstructionCostProvider *provider) noexcept;
    [[nodiscard]] const ConstructionRecipe *FindRecipe(ConstructionRecipeId id) const noexcept;
    [[nodiscard]] const PlacementDefinition *FindPlacementDefinition(PlacementRuleId id) const noexcept;
    [[nodiscard]] const PlacementSocket *FindSocket(PlacementSocketId id) const noexcept;
    [[nodiscard]] foundation::Result<PlacementValidationResult> ValidatePlacement(const PlacementRequest &request) const;
    [[nodiscard]] foundation::Result<PlacementPlan> PreparePlacementPlan(const PlacementRequest &request);
    [[nodiscard]] foundation::Result<PlacementCommitResult> CommitPlacement(PlacementPlanId plan);
    [[nodiscard]] foundation::Result<PlacementCommitResult> CommitPlacement(const PlacementPlan &plan);
    [[nodiscard]] foundation::Result<void> CancelPlacementPlan(PlacementPlanId plan, GameplayContext context = {});
    [[nodiscard]] std::size_t ExpirePlacementPlans(GameplayTimePoint now, GameplayContext context = {});
    [[nodiscard]] foundation::Result<ConstructionSiteId> StartConstructionSite(PlacementPlanId plan,
                                                                               GameplayTimePoint started_at = {},
                                                                               GameplayContext context = {});
    [[nodiscard]] foundation::Result<ConstructionSiteId> StartConstructionSite(const PlacementPlan &plan,
                                                                               GameplayTimePoint started_at = {},
                                                                               GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> PauseConstructionSite(ConstructionSiteId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ResumeConstructionSite(ConstructionSiteId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> AdvanceConstructionProgress(ConstructionSiteId id, Fixed delta_micro,
                                                                      GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompleteConstructionSite(ConstructionSiteId id,
                                                                    GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CancelConstructionSite(ConstructionSiteId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> FailConstructionSite(ConstructionSiteId id, PlacementReasonId reason,
                                                                GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> DestroyConstructionSite(ConstructionSiteId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> PruneTerminalSite(ConstructionSiteId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<ConstructionSocketReservationId> ReserveSocket(
        PlacementSocketId id, GameplayObjectRef owner, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> ReleaseSocket(ConstructionSocketReservationId reservation,
                                                         GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> OccupySocket(PlacementSocketId id,
                                                        std::optional<ConstructionSocketReservationId> reservation = {},
                                                        GameplayContext context = {});
    [[nodiscard]] const PlacementPlan *FindPlan(PlacementPlanId id) const noexcept;
    [[nodiscard]] const ConstructionSite *FindSite(ConstructionSiteId id) const noexcept;
    [[nodiscard]] std::vector<ConstructionSite> FindConstructionSites(GameplayObjectRef actor = {}) const;
    [[nodiscard]] std::vector<PlacementSocket> FindSocketsForObject(GameplayObjectRef owner) const;
    [[nodiscard]] std::vector<PlacementOutputEnvelope> PendingOutputs() const;
    // Queue a durable integration output for an already placed construction object. The output inherits
    // the original placement execution identity and is persisted in the Construction outbox until Ack.
    [[nodiscard]] foundation::Result<PlacementOutputId> EnqueuePlacedObjectOutput(
        PlacedObjectId placed_object, PlacementOutputOperation operation, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> AcknowledgeOutput(PlacementOutputId id, GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> DeadLetterOutput(PlacementOutputId id, PlacementReasonId reason,
                                                            GameplayContext context = {});
    [[nodiscard]] foundation::Result<void> CompactPlacementState(PlacementPlanId plan, GameplayContext context = {});
    [[nodiscard]] ConstructionSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(ConstructionSnapshot snapshot);
    private:
        [[nodiscard]] std::vector<ConstructionChange> ChangesSinceSequence(std::uint64_t sequence) const;
        [[nodiscard]] ConstructionChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] ConstructionChangeBatch ReadChangesSince(ChangeCursor cursor) const
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
    [[nodiscard]] ConstructionDiagnostics GetDiagnostics() const noexcept;
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }

  private:
    [[nodiscard]] std::optional<Revision> NextRevision() const noexcept;
    void Record(ConstructionChange change) noexcept;
    [[nodiscard]] PlacementSocket *FindMutableSocket(PlacementSocketId id) noexcept;
    [[nodiscard]] ConstructionSite *FindMutableSite(ConstructionSiteId id) noexcept;
    [[nodiscard]] foundation::Result<std::vector<ConstructionCostReservation>> ReserveCosts(const PlacementPlan &plan);
    void CommitCosts(const std::vector<ConstructionCostReservation> &reservations, const GameplayContext &context) noexcept;
    void ReleaseCosts(const std::vector<ConstructionCostReservation> &reservations, const GameplayContext &context) noexcept;
    [[nodiscard]] std::vector<PlacementOutputOperation> BuildOutputs(const PlacementPlan &plan,
                                                                     const ConstructionRecipe &recipe,
                                                                     PlacedObjectId placed) const;
    [[nodiscard]] foundation::Result<std::vector<PlacementOutputEnvelope>> StageOutputs(
        PlacementExecutionId execution,
        const std::vector<PlacementOutputOperation> &outputs) const;
    [[nodiscard]] foundation::Result<void> QueueOutputs(std::vector<PlacementOutputEnvelope> outputs,
                      PlacementPlanId plan, ConstructionSiteId site,
                      GameplayObjectRef actor, GameplayContext context, Revision publication_revision);
    [[nodiscard]] foundation::Result<PlacementValidationResult> ValidatePlacementInternal(
        const PlacementRequest &request, bool count_diagnostics) const;

    std::unordered_map<PlacementRuleId, PlacementDefinition, IdHash> placement_definitions_;
    std::unordered_map<ConstructionRecipeId, ConstructionRecipe, IdHash> recipes_;
    std::unordered_map<PlacementSocketId, PlacementSocket, IdHash> sockets_;
    std::unordered_map<PlacementPlanId, PlacementPlan, IdHash> plans_;
    std::unordered_map<ConstructionSiteId, ConstructionSite, IdHash> sites_;
    std::unordered_map<ConstructionSocketReservationId, ConstructionSocketReservation, IdHash> socket_reservations_by_id_;
    std::unordered_map<PlacementSocketId, ConstructionSocketReservationId, IdHash> socket_reservation_by_socket_;
    std::vector<PlacedObjectRecord> placed_objects_;
    std::deque<PlacementOutputEnvelope> outbox_;
    std::deque<PlacementOutputDeadLetter> dead_letters_;
    MonotonicIdGenerator<GameplayObjectId> plan_ids_, site_ids_, placed_ids_, execution_ids_, output_ids_, socket_reservation_ids_;
    const IConstructionPlacementProvider *placement_provider_ = nullptr;
    IConstructionCostProvider *cost_provider_ = nullptr;
    std::uint64_t placement_provider_epoch_ = 1;
    std::uint64_t cost_provider_epoch_ = 1;
    Revision revision_{};
    bool frozen_ = false;
    static constexpr std::size_t kChangeJournalCapacity = 4096;
    static constexpr std::size_t kOutboxCapacity = 8192;
    static constexpr std::size_t kDeadLetterCapacity = 1024;
    static constexpr GameplayDuration kDefaultPlanLifetime{300};
    std::deque<ConstructionChange> changes_;
    std::uint64_t next_change_sequence_ = 1;
    std::uint64_t journal_epoch_ = 1;
    mutable std::uint64_t validations_ = 0, rejections_ = 0;
    std::uint64_t committed_ = 0, socket_reservations_ = 0, completed_sites_ = 0;
};
} // namespace epidemic::gameplay::construction
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `PlacementTargetKind`: `Free`, `Surface`, `Socket`, `Attach`, `Area`
- `PlacementAvailability`: `Available`, `Unavailable`, `Blocked`, `RequiresResource`, `RequiresPermission`, `RequiresCapability`, `InvalidTarget`, `NotMaterialized`, `Unsupported`
- `PlacementCommitPolicy`: `Instant`, `CreateSite`, `PreviewOnly`
- `ConstructionCostPolicy`: `ReserveThenCommit`, `Free`
- `ConstructionSiteState`: `Planned`, `UnderConstruction`, `Paused`, `Completed`, `Cancelled`, `Failed`, `ResourcesCommitted`, `Destroyed`
- `PlacementPlanState`: `Prepared`, `Committed`, `Cancelled`, `Expired`
- `SocketState`: `Free`, `Occupied`, `Disabled`, `Reserved`
- `ConstructionChangeKind`: `PlacementValidated`, `PlacementRejected`, `PlacementCommitted`, `PlacedObjectCreated`, `SiteCreated`, `SiteStarted`, `SiteCompleted`, `SiteCancelled`, `SocketReserved`, `SocketReleased`, `SocketOccupied`, `SitePaused`, `SiteResumed`, `SiteDestroyed`, `OutputQueued`, `OutputAcknowledged`, `OutputDeadLettered`, `PlacementPlanCancelled`, `PlacementPlanExpired`, `PlacementStateCompacted`, `SocketRegistered`, `SiteProgressed`, `SiteFailed`

### Состав типов и операции

#### `PlacementRuleId` (строка заголовка 19)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementRuleId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementRuleId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementRuleId &) const noexcept = default;
```

#### `PlacementPlanId` (строка заголовка 27)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementPlanId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
static constexpr PlacementPlanId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementPlanId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementPlanId &) const noexcept = default;
```

#### `PlacementExecutionId` (строка заголовка 39)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementExecutionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
static constexpr PlacementExecutionId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementExecutionId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementExecutionId &) const noexcept = default;
```

#### `ConstructionRecipeId` (строка заголовка 54)

Публичные сигнатуры операций:

```cpp
static constexpr ConstructionRecipeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ConstructionRecipeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ConstructionRecipeId &) const noexcept = default;
```

#### `ConstructionSiteId` (строка заголовка 62)

Публичные сигнатуры операций:

```cpp
static constexpr ConstructionSiteId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ConstructionSiteId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ConstructionSiteId &) const noexcept = default;
```

#### `PlacementSocketId` (строка заголовка 73)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementSocketId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementSocketId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementSocketId &) const noexcept = default;
```

#### `PlacedObjectId` (строка заголовка 84)

Публичные сигнатуры операций:

```cpp
static constexpr PlacedObjectId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacedObjectId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacedObjectId &) const noexcept = default;
```

#### `ConstructionSocketReservationId` (строка заголовка 95)

Публичные сигнатуры операций:

```cpp
static constexpr ConstructionSocketReservationId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ConstructionSocketReservationId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ConstructionSocketReservationId &) const noexcept = default;
```

#### `PlacementOutputId` (строка заголовка 106)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementOutputId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementOutputId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementOutputId &) const noexcept = default;
```

#### `ConstructionCostTypeId` (строка заголовка 117)

Публичные сигнатуры операций:

```cpp
static constexpr ConstructionCostTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ConstructionCostTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ConstructionCostTypeId &) const noexcept = default;
```

#### `PlacementOutputTypeId` (строка заголовка 125)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementOutputTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementOutputTypeId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementOutputTypeId &) const noexcept = default;
```

#### `PlacementReasonId` (строка заголовка 133)

Публичные сигнатуры операций:

```cpp
static constexpr PlacementReasonId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const PlacementReasonId &) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const PlacementReasonId &) const noexcept = default;
```

#### `IdHash` (строка заголовка 141)

Публичные сигнатуры операций:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `T` (строка заголовка 143)

#### `RefHash` (строка заголовка 148)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef ref) const noexcept;
```

#### `WorldPosition` (строка заголовка 154)

Публичные поля и значения по умолчанию:

- `Fixed x_mm = 0, y_mm = 0, z_mm = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const WorldPosition &) const noexcept = default;
```

#### `OrientationFixed` (строка заголовка 159)

Публичные поля и значения по умолчанию:

- `Fixed yaw_micro = 0, pitch_micro = 0, roll_micro = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const OrientationFixed &) const noexcept = default;
```

#### `WorldVolumeRef` (строка заголовка 164)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
```

#### `PlacementDefinition` (строка заголовка 255)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet tags`
- `PlacementTargetKind target_kind = PlacementTargetKind::Free`
- `PlacementCommitPolicy commit_policy = PlacementCommitPolicy::Instant`
- `bool allow_pitch = true`
- `bool allow_roll = true`
- `Fixed yaw_step_micro = 0`
- `bool requires_permission = false`
- `bool requires_capability = false`

#### `ConstructionCost` (строка заголовка 268)

Публичные поля и значения по умолчанию:

- `Fixed amount_micro = 0`
- `std::vector<std::byte> payload`

#### `PlacementOutputTemplate` (строка заголовка 274)

Публичные поля и значения по умолчанию:

- `bool subject_is_target_area = false`
- `std::vector<std::byte> payload`

#### `ConstructionRecipe` (строка заголовка 281)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `GameplayTagSet required_surface_tags`
- `GameplayTagSet blocked_area_tags`
- `GameplayTagSet placement_tags`
- `std::vector<ConstructionCost> costs`
- `ConstructionCostPolicy cost_policy = ConstructionCostPolicy::ReserveThenCommit`
- `std::vector<PlacementOutputTemplate> output_templates`
- `std::vector<std::byte> placement_payload`

#### `PlacementFootprint` (строка заголовка 303)

Публичные поля и значения по умолчанию:

- `GameplayTagSet footprint_tags`
- `std::vector<std::byte> payload`

#### `PlacementSemanticProjection` (строка заголовка 313)

Публичные поля и значения по умолчанию:

- `bool materialized = true`
- `bool area_allowed = true`
- `bool permission_granted = true`
- `bool capability_available = true`
- `bool spacing_clear = true`
- `GameplayTagSet surface_tags`
- `GameplayTagSet area_tags`

#### `PlacementValidationResult` (строка заголовка 325)

Публичные поля и значения по умолчанию:

- `PlacementAvailability availability = PlacementAvailability::Unavailable`
- `std::vector<PlacementReason> reasons`

#### `PlacementPlan` (строка заголовка 332)

Публичные поля и значения по умолчанию:

- `std::vector<ConstructionCost> reserved_costs`
- `PlacementPlanState state = PlacementPlanState::Prepared`
- `std::uint64_t placement_provider_epoch = 0`
- `std::uint64_t cost_provider_epoch = 0`

#### `PlacementOutputOperation` (строка заголовка 352)

Публичные поля и значения по умолчанию:

- `std::vector<std::byte> payload`

#### `PlacementOutputEnvelope` (строка заголовка 360)

Публичные поля и значения по умолчанию:

- `std::uint32_t ordinal = 0`

#### `PlacementCommitResult` (строка заголовка 374)

Публичные поля и значения по умолчанию:

- `std::vector<PlacementOutputOperation> outputs`

#### `PlacementSocket` (строка заголовка 382)

Публичные поля и значения по умолчанию:

- `GameplayTagSet accepted_tags`
- `SocketState state = SocketState::Free`

#### `ConstructionSite` (строка заголовка 398)

Публичные поля и значения по умолчанию:

- `ConstructionSiteState state = ConstructionSiteState::Planned`
- `Fixed progress_micro = 0`

#### `IConstructionPlacementProvider` (строка заголовка 433)

Публичные сигнатуры операций:

```cpp
virtual ~IConstructionPlacementProvider() = default;
[[nodiscard]] virtual foundation::Result<PlacementSemanticProjection> Project( const PlacementRequest &request) const = 0;
[[nodiscard]] virtual Revision CurrentRevision() const noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IConstructionCostProvider` (строка заголовка 441)

Публичные сигнатуры операций:

```cpp
virtual ~IConstructionCostProvider() = default;
[[nodiscard]] virtual foundation::Result<bool> CanAfford(GameplayObjectRef actor, const ConstructionCost &cost, const GameplayContext &context) const = 0;
[[nodiscard]] virtual foundation::Result<ConstructionCostReservation> Reserve(GameplayObjectRef actor, const ConstructionCost &cost, const GameplayContext &context) = 0;
virtual void Commit(const ConstructionCostReservation &reservation, const GameplayContext &context) noexcept = 0;
virtual void Release(const ConstructionCostReservation &reservation, const GameplayContext &context) noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ConstructionChange` (строка заголовка 454)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`
- `ConstructionChangeKind kind = ConstructionChangeKind::PlacementValidated`

#### `ConstructionChangeBatch` (строка заголовка 467)

Публичные поля и значения по умолчанию:

- `bool snapshot_required = false`
- `std::uint64_t oldest_available_sequence = 0`
- `std::vector<ConstructionChange> changes`

#### `ConstructionSnapshot` (строка заголовка 476)

Публичные поля и значения по умолчанию:

- `std::vector<PlacementPlan> plans`
- `std::vector<ConstructionSite> sites`
- `std::vector<PlacementSocket> sockets`
- `std::vector<ConstructionSocketReservation> socket_reservations`
- `std::vector<PlacedObjectRecord> placed_objects`
- `std::vector<PlacementOutputEnvelope> pending_outputs`
- `std::vector<PlacementOutputDeadLetter> dead_letters`
- `std::vector<ConstructionChange> journal`
- `std::uint64_t next_change_sequence = 1`
- `std::uint64_t change_epoch = 1`

#### `ConstructionDiagnostics` (строка заголовка 497)

Публичные поля и значения по умолчанию:

- `std::uint64_t recipes = 0, placement_definitions = 0, validations = 0, rejections = 0, committed = 0, active_sites = 0, socket_reservations = 0, completed_sites = 0, pending_outputs = 0, dead_lettered_outputs = 0`

#### `ConstructionService` (строка заголовка 504)

Внутреннее состояние (не часть публичного API): `std::unordered_map<PlacementRuleId, PlacementDefinition, IdHash> placement_definitions_`, `std::unordered_map<ConstructionRecipeId, ConstructionRecipe, IdHash> recipes_`, `std::unordered_map<PlacementSocketId, PlacementSocket, IdHash> sockets_`, `std::unordered_map<PlacementPlanId, PlacementPlan, IdHash> plans_`, `std::unordered_map<ConstructionSiteId, ConstructionSite, IdHash> sites_`, `std::unordered_map<ConstructionSocketReservationId, ConstructionSocketReservation, IdHash> sock`, `std::unordered_map<PlacementSocketId, ConstructionSocketReservationId, IdHash> socket_reservati`, `std::vector<PlacedObjectRecord> placed_objects_`, `std::deque<PlacementOutputEnvelope> outbox_`.

Публичные сигнатуры операций:

```cpp
ConstructionService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterPlacementDefinition(PlacementDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterRecipe(ConstructionRecipe recipe);
[[nodiscard]] foundation::Result<void> RegisterSocket(PlacementSocket socket);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
void SetPlacementProvider(const IConstructionPlacementProvider *provider) noexcept;
void SetCostProvider(IConstructionCostProvider *provider) noexcept;
[[nodiscard]] const ConstructionRecipe *FindRecipe(ConstructionRecipeId id) const noexcept;
[[nodiscard]] const PlacementDefinition *FindPlacementDefinition(PlacementRuleId id) const noexcept;
[[nodiscard]] const PlacementSocket *FindSocket(PlacementSocketId id) const noexcept;
[[nodiscard]] foundation::Result<PlacementValidationResult> ValidatePlacement(const PlacementRequest &request) const;
[[nodiscard]] foundation::Result<PlacementPlan> PreparePlacementPlan(const PlacementRequest &request);
[[nodiscard]] foundation::Result<PlacementCommitResult> CommitPlacement(PlacementPlanId plan);
[[nodiscard]] foundation::Result<PlacementCommitResult> CommitPlacement(const PlacementPlan &plan);
[[nodiscard]] const PlacementPlan *FindPlan(PlacementPlanId id) const noexcept;
[[nodiscard]] const ConstructionSite *FindSite(ConstructionSiteId id) const noexcept;
[[nodiscard]] std::vector<PlacementSocket> FindSocketsForObject(GameplayObjectRef owner) const;
[[nodiscard]] std::vector<PlacementOutputEnvelope> PendingOutputs() const;
[[nodiscard]] ConstructionSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(ConstructionSnapshot snapshot);
[[nodiscard]] ConstructionChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] ConstructionDiagnostics GetDiagnostics() const noexcept;
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
| `ConstructionService` | `ConstructionService::ConstructionService` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:52` |
| `SetPlacementProvider` | `ConstructionService::SetPlacementProvider` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:62` |
| `SetCostProvider` | `ConstructionService::SetCostProvider` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:72` |
| `RegisterPlacementDefinition` | `ConstructionService::RegisterPlacementDefinition` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:82` |
| `RegisterRecipe` | `ConstructionService::RegisterRecipe` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:94` |
| `RegisterSocket` | `ConstructionService::RegisterSocket` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:114` |
| `FindRecipe` | `ConstructionService::FindRecipe` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:140` |
| `FindPlacementDefinition` | `ConstructionService::FindPlacementDefinition` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:146` |
| `FindSocket` | `ConstructionService::FindSocket` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:152` |
| `ValidatePlacement` | `ConstructionService::ValidatePlacement` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:158` |
| `PreparePlacementPlan` | `ConstructionService::PreparePlacementPlan` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:312` |
| `CommitPlacement` | `ConstructionService::CommitPlacement` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:523` |
| `FindPlan` | `ConstructionService::FindPlan` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:1139` |
| `FindSite` | `ConstructionService::FindSite` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:1145` |
| `FindSocketsForObject` | `ConstructionService::FindSocketsForObject` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:1176` |
| `PendingOutputs` | `ConstructionService::PendingOutputs` | `EngineFramework/GameplayWorldStateOwners/Construction/src/construction.cpp:1189` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `SetPlacementProvider`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:41`
- `RegisterPlacementDefinition`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:47`
- `RegisterRecipe`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:60`
- `RegisterSocket`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:66`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:68`
- `ValidatePlacement`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:81`
- `PreparePlacementPlan`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:84`
- `CommitPlacement`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:87`
- `PendingOutputs`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:97`
- `FindSocket`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:132`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:134`
- `RestoreSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/construction_tests.cpp:143`

