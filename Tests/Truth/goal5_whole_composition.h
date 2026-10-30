#pragma once
#include "Epidemic/Core/application.h"
#include "Epidemic/EngineBase/engine_base_support.h"
#include "Epidemic/Runtime/Support/runtime_support.h"
#include "engine_runtime_bridge_backend.h"
#include "goal5_digest.h"
#include "goal5_framework_composition.h"
#include "truth_test.h"
#include <chrono>
#include <functional>
#include <memory>

namespace goal5_test_utility {
namespace rt = epidemic::runtime;
namespace fw = epidemic::gameplay;
namespace ig = fw::integration;
namespace rb = fw::runtime_bridge;
namespace fd = epidemic::foundation;

// The outer generation store is the only durable publication point. This
// composition-provided backend initializes a private Runtime service from an
// already decoded package; it is not a second authoritative save backend.
class PreparedSnapshotBackend final : public rt::IPersistenceBackend {
public:
  explicit PreparedSnapshotBackend(rt::PersistenceSnapshot snapshot)
      : snapshot_(std::move(snapshot)) {}
  fd::Result<rt::PersistenceSnapshot> Load() override {
    return fd::Result<rt::PersistenceSnapshot>::Success(snapshot_);
  }
  fd::Result<void> CommitSnapshot(const rt::PersistenceSnapshot &,
                                  rt::PersistenceDurability) override {
    return fd::Result<void>::Failure(
        fd::Error::Create("whole.unexpected_commit",
                          "outer generation owns durable publication"));
  }

private:
  rt::PersistenceSnapshot snapshot_;
};

struct RuntimeHost {
  epidemic::core::Application application{{"Goal5WholeEngine", std::nullopt}};
  rt::EngineRuntimeServices services;
  explicit RuntimeHost(const rt::TimeCheckpoint *clock = nullptr,
                       const rt::PersistenceSnapshot *persistence = nullptr) {
    auto logger = epidemic::enginebase::RegisterEngineBase(
        application, {.runtime_name = "Goal5WholeEngine",
                      .log_module = "Goal5WholeEngine",
                      .worker_count = 1});
    TRUTH_REQUIRE(logger);
    rt::EngineRuntimeOptions options;
    options.time.game_ticks_per_real_second = 60;
    if (clock) {
      options.time.game_ticks_per_real_second =
          clock->game_ticks_per_real_second;
      options.time.calendar = clock->calendar;
      options.time.phase_boundaries = clock->phase_boundaries;
      options.time.initial_time_scale = clock->time_scale;
    }
    if (persistence)
      options.persistence.backend =
          std::make_shared<PreparedSnapshotBackend>(*persistence);
    auto runtime = rt::RegisterDefaultEngineRuntime(application, options);
    TRUTH_REQUIRE(runtime);
    services = std::move(runtime.Value());
    if (clock)
      TRUTH_REQUIRE(services.time->runtime->RestoreCheckpoint(*clock));
    TRUTH_REQUIRE(application.Bootstrap() == 0);
    TRUTH_REQUIRE(application.Initialize() == 0);
    TRUTH_REQUIRE(services.world->regions->RegisterRegion(
        {rt::RegionId{1}, fd::StringId::FromString("whole.region")}));
    TRUTH_REQUIRE(services.world->chunks->RegisterChunk(
        {rt::ChunkId{1}, rt::RegionId{1}, 0, 0, 0}));
    auto chunk = services.world->chunks->GetChunkSnapshot(rt::ChunkId{1});
    TRUTH_REQUIRE(chunk);
    TRUTH_REQUIRE(services.world->chunks->SetChunkState(
        {rt::ChunkId{1}, chunk.Value().revision, rt::ChunkState::Loading}));
    chunk = services.world->chunks->GetChunkSnapshot(rt::ChunkId{1});
    TRUTH_REQUIRE(services.world->chunks->SetChunkState(
        {rt::ChunkId{1}, chunk.Value().revision, rt::ChunkState::Resident}));
    services.world->regions->Freeze();
    services.world->chunks->Freeze();
  }
  ~RuntimeHost() {
    if (services.coordinator)
      (void)services.coordinator->Shutdown();
    (void)application.Shutdown();
  }
};

class PreparedRuntimeHost final : public ig::IRuntimeGenerationRestoreStage {
public:
  PreparedRuntimeHost(std::unique_ptr<RuntimeHost> &live,
                      std::unique_ptr<RuntimeHost> prepared)
      : live_(live), prepared_(std::move(prepared)) {}
  void Commit() noexcept override { live_.swap(prepared_); }

private:
  std::unique_ptr<RuntimeHost> &live_;
  std::unique_ptr<RuntimeHost> prepared_;
};

class RuntimeRestorePort final : public ig::IRuntimeGenerationRestorePort {
public:
  bool fail_prepare = false;
  std::weak_ptr<rt::IGameClock> prepared_clock;
  std::uint32_t prepared_count = 0;
  explicit RuntimeRestorePort(std::unique_ptr<RuntimeHost> &live)
      : live_(live) {}
  fd::Result<std::unique_ptr<ig::IRuntimeGenerationRestoreStage>>
  Prepare(const ig::SaveGenerationPackage &package) override {
    if (fail_prepare)
      throw std::bad_alloc();
    auto clock = rt::DecodeRuntimeTimeCheckpointPackage(package.runtime_time);
    if (!clock)
      return fd::Result<std::unique_ptr<ig::IRuntimeGenerationRestoreStage>>::
          Failure(clock.GetError());
    auto persistence =
        rt::DecodeRuntimePersistencePackage(package.runtime_persistence);
    if (!persistence)
      return fd::Result<std::unique_ptr<ig::IRuntimeGenerationRestoreStage>>::
          Failure(persistence.GetError());
    auto prepared =
        std::make_unique<RuntimeHost>(&clock.Value(), &persistence.Value());
    prepared_clock = prepared->services.time->clock;
    ++prepared_count;
    return fd::Result<std::unique_ptr<ig::IRuntimeGenerationRestoreStage>>::
        Success(
            std::make_unique<PreparedRuntimeHost>(live_, std::move(prepared)));
  }

private:
  std::unique_ptr<RuntimeHost> &live_;
};

class WholeQuiescence final : public fw::savegame::ISaveBarrierLease {
public:
  explicit WholeQuiescence(bool &paused) : paused_(paused) {
    TRUTH_REQUIRE(!paused_);
    paused_ = true;
  }
  ~WholeQuiescence() override { paused_ = false; }

private:
  bool &paused_;
};

inline std::vector<std::byte> EncodeEntityIdentity(fw::entities::EntityId id) {
  std::vector<std::byte> result;
  for (auto word : {id.High(), id.Low()})
    for (unsigned shift = 0; shift != 64; shift += 8)
      result.push_back(std::byte((word >> shift) & 255));
  return result;
}
inline fw::entities::EntityId
DecodeEntityIdentity(const rt::PersistentObjectRecord &record) {
  TRUTH_REQUIRE(record.payload.schema_id ==
                fd::StringId::FromString("whole.entity.identity"));
  TRUTH_REQUIRE(record.payload.schema_version == 1 &&
                record.payload.bytes.size() == 16);
  std::uint64_t high = 0, low = 0;
  for (unsigned index = 0; index != 8; ++index) {
    high |= std::uint64_t(
                std::to_integer<unsigned char>(record.payload.bytes[index]))
            << (8 * index);
    low |= std::uint64_t(
               std::to_integer<unsigned char>(record.payload.bytes[8 + index]))
           << (8 * index);
  }
  return fw::entities::EntityId::FromRaw(high, low);
}

struct WholeComposition {
  FrameworkCompositionView &framework;
  std::unique_ptr<RuntimeHost> &runtime;
  bool quiesced = false;
  bool restore_mode = false;
  bool action_ready = false;
  std::uint64_t synchronized_runtime_revision = 0;
  int action_commits = 0;
  std::string_view fault_once;
  std::unique_ptr<rb::EngineRuntimeBridgeBackend> backend;
  std::unique_ptr<rb::RuntimeBridgeService> bridge;
  ig::SaveGenerationBarrier barrier;
  ig::FileSaveGenerationStore store;
  ig::CallbackRuntimeSaveGenerationPort capture_port;
  RuntimeRestorePort restore_port;
  ig::SaveGenerationCoordinator saver;
  ig::WholeEngineRestoreCoordinator restorer;

  WholeComposition(FrameworkCompositionView &view,
                   std::unique_ptr<RuntimeHost> &host,
                   const std::filesystem::path &path)
      : framework(view), runtime(host), barrier(BarrierCallbacks()),
        store(path),
        capture_port(
            [&](auto) {
              return rt::EncodeRuntimeTimeCheckpointPackage(
                  runtime->services.time->runtime->CaptureCheckpoint());
            },
            [&](auto) {
              auto state = rt::CaptureRuntimePersistenceSnapshot(
                  *runtime->services.persistence->query);
              if (!state)
                return fd::Result<std::vector<std::byte>>::Failure(
                    state.GetError());
              return rt::EncodeRuntimePersistencePackage(state.Value());
            }),
        restore_port(runtime),
        saver(framework.orchestrator, capture_port, barrier, store),
        restorer(framework.orchestrator, restore_port, barrier, store,
                 RestoreCallbacks()) {
    TRUTH_REQUIRE(framework.orchestrator.SetBarrier(barrier));
  }

  ig::SaveGenerationBarrierCallbacks BarrierCallbacks() {
    ig::SaveGenerationBarrierCallbacks callbacks;
    callbacks.acquire_capture_quiescence = [&]() {
      return fd::Result<std::unique_ptr<fw::savegame::ISaveBarrierLease>>::
          Success(std::make_unique<WholeQuiescence>(quiesced));
    };
    callbacks.acquire_restore_quiescence = callbacks.acquire_capture_quiescence;
    callbacks.capture_preflight = [&]() {
      if (bridge && (bridge->GetDiagnostics().projection_backlog != 0 ||
                     bridge->GetDiagnostics().reconciliation_requests != 0))
        return fd::Result<void>::Failure(fd::Error::Create(
            "whole.pending_projection",
            "complete materialization/recovery before capture"));
      return fd::Result<void>::Success();
    };
    return callbacks;
  }
  ig::WholeRestoreCallbacks RestoreCallbacks() {
    return {[&]() {
              Fault("rebuild");
              RebuildRuntimeBridge();
              return fd::Result<void>::Success();
            },
            [&]() {
              SynchronizeAndReconcile();
              Fault("reconcile.accepted");
              return fd::Result<void>::Success();
            },
            [&]() {
              MaterializePersistentObjects();
              return fd::Result<void>::Success();
            }};
  }
  void Fault(std::string_view point) {
    if (fault_once == point) {
      fault_once = {};
      throw std::runtime_error(std::string(point));
    }
  }
  fd::Result<ig::SaveGenerationId> Restore() {
    restore_mode = true;
    auto result = restorer.LoadAndRestore({});
    if (!result)
      std::cerr << "whole restore: " << result.GetError().code << ": "
                << result.GetError().message << '\n';
    return result;
  }
  bool CanTick() const noexcept {
    return !quiesced && (!restore_mode || restorer.CanResume());
  }
  fd::Result<void> Advance(std::chrono::microseconds delta) {
    if (!CanTick())
      return fd::Result<void>::Failure(fd::Error::Create(
          "whole.resume_guard", "restore is not ready for simulation"));
    const auto advanced = runtime->services.time->runtime->Advance(delta);
    if (!advanced)
      return fd::Result<void>::Failure(advanced.GetError());
    return fd::Result<void>::Success();
  }
  void RebuildRuntimeBridge() {
    bridge.reset();
    backend.reset();
    backend = std::make_unique<rb::EngineRuntimeBridgeBackend>(
        *runtime->services.world, *runtime->services.physics,
        runtime->services.environment->runtime, *runtime->services.navigation);
    bridge = std::make_unique<rb::RuntimeBridgeService>(*backend);
  }
  void SynchronizeAndReconcile() {
    ig::RuntimeTimeAdapter clock(*runtime->services.time->clock, framework.time,
                                 framework.clock);
    const auto source = runtime->services.time->clock->GetSnapshot();
    if (framework.time.NeedsSynchronizationRebind(framework.clock) ||
        synchronized_runtime_revision != source.revision) {
      TRUTH_REQUIRE(clock.Synchronize());
      synchronized_runtime_revision = source.revision;
    } else {
      const auto current = framework.time.GetClock(framework.clock);
      TRUTH_REQUIRE(current &&
                    current->now == fw::GameplayTimePoint{source.now.ticks});
    }
    TRUTH_REQUIRE(framework.state_facts.PublishPendingChanges());
    TRUTH_REQUIRE(framework.facts.Dispatch());
    TRUTH_REQUIRE(framework.state_lifecycle.ProcessEntityChanges());
    TRUTH_REQUIRE(framework.state_delivery.ProcessPending());
    if (framework.abilities.NeedsResourceReconciliation())
      TRUTH_REQUIRE(framework.abilities.ReconcileRestoredReservations());
    TRUTH_REQUIRE(framework.condition_progression.ProcessChanges());
    TRUTH_REQUIRE(framework.state_time.SynchronizeConditionSchedules());
    TRUTH_REQUIRE(framework.state_time.SynchronizeDeferredEffects());
    framework.world_facts.ResetCursorsToLatest();
    VerifyObservationBindings();
  }
  void VerifyObservationBindings() {
    const auto state = framework.state.CaptureCheckpoint();
    const auto bound = [](fw::ChangeCursor cursor, fw::ChangeCursor latest) {
      return cursor.IsValid() ? cursor.epoch == latest.epoch &&
                                    cursor.sequence <= latest.sequence
                              : cursor.sequence == 0;
    };
    const std::pair<fw::ChangeCursor, fw::ChangeCursor> cursors[] = {
        {state.facts.entity_cursor, framework.entities.LatestChangeCursor()},
        {state.facts.material_cursor, framework.materials.LatestChangeCursor()},
        {state.facts.condition_cursor,
         framework.conditions.LatestChangeCursor()},
        {state.facts.effect_cursor, framework.effects.LatestChangeCursor()},
        {state.lifecycle.cursor, framework.entities.LatestChangeCursor()},
        {state.condition_effects.cursor,
         framework.conditions.LatestChangeCursor()},
        {state.time.condition_cursor,
         framework.conditions.LatestChangeCursor()},
        {state.time.effect_cursor, framework.effects.LatestChangeCursor()},
        {framework.death_reward.CaptureCheckpoint().cursor,
         framework.combat.LatestChangeCursor()},
        {framework.social_legal.CaptureCheckpoint().crime_cursor,
         framework.crime.LatestChangeCursor()},
        {framework.condition_progression.CaptureCheckpoint().cursor,
         framework.conditions.LatestChangeCursor()}};
    for (const auto &[cursor, latest] : cursors)
      TRUTH_REQUIRE(bound(cursor, latest));
    const auto world = framework.world_facts.CaptureCheckpoint();
    TRUTH_REQUIRE(world.world_epoch ==
                  framework.world.LatestChangeCursor().epoch);
    TRUTH_REQUIRE(world.environment_epoch ==
                  framework.environment.LatestChangeCursor().epoch);
    TRUTH_REQUIRE(world.interaction_epoch ==
                  framework.interaction.LatestChangeCursor().epoch);
  }
  void MaterializePersistentObjects() {
    TRUTH_REQUIRE(backend && bridge);
    auto snapshot = rt::CaptureRuntimePersistenceSnapshot(
        *runtime->services.persistence->query);
    TRUTH_REQUIRE(snapshot);
    for (const auto &record : snapshot.Value().objects) {
      const auto entity = DecodeEntityIdentity(record);
      TRUTH_REQUIRE(framework.entities.Exists(entity));
      const auto object =
          fw::entities::EntityService::ToGameplayObjectRef(entity);
      if (bridge->GetBinding(object))
        continue;
      // A prior attempt may have published the World record or accepted a
      // bridge request. Recover that work rather than create/enqueue twice.
      if (bridge->GetDiagnostics().projection_backlog != 0) {
        const auto retried = bridge->Process();
        TRUTH_REQUIRE(retried.failed == 0);
        if (bridge->GetBinding(object))
          continue;
      }
      for (const auto &pending : bridge->ReconciliationRequests())
        TRUTH_REQUIRE(bridge->RetryReconciliation(pending.sequence));
      if (bridge->GetDiagnostics().projection_backlog != 0) {
        TRUTH_REQUIRE(bridge->Process().failed == 0);
        if (bridge->GetBinding(object))
          continue;
      }
      rt::WorldObjectRecord projection;
      projection.runtime_id = rt::RuntimeObjectId{record.persistent_id.value};
      projection.persistent_id = record.persistent_id;
      projection.asset_id = record.asset_id;
      projection.persistence_tier = record.tier;
      projection.residency = rt::ResidencyState::Resident;
      projection.placement = rt::WorldSurfacePlacement{
          record.location.region_id, record.location.chunk_id, {}};
      if (!runtime->services.world->query->FindObject(projection.runtime_id))
        TRUTH_REQUIRE(runtime->services.world->writer->Apply(
            rt::CreateObjectCommand{projection}));
      Fault("materialize.world_accepted");
      const auto persistent =
          backend->ImportPersistentObjectId(record.persistent_id);
      TRUTH_REQUIRE(persistent.IsValid());
      TRUTH_REQUIRE(bridge->Enqueue(rb::MaterializeRequest{
          object, persistent, fw::Revision{record.revision}}));
      Fault("materialize.queued");
      const auto processed = bridge->Process();
      TRUTH_REQUIRE(processed.failed == 0 && processed.materialized == 1);
      const auto binding = bridge->GetBinding(object);
      TRUTH_REQUIRE(binding);
      const auto reverse_binding = bridge->GetBinding(binding->runtime_object);
      TRUTH_REQUIRE(reverse_binding &&
                    reverse_binding->gameplay_object == object);
      Fault("materialize.accepted");
    }
  }
  void Seed() {
    TRUTH_REQUIRE(!quiesced);
    for (int index = 0; index != 3; ++index) {
      fw::entities::CreateEntityRequest create;
      create.archetype =
          fw::entities::EntityArchetypeId::FromString("whole.actor");
      auto entity = framework.entities.Create(std::move(create));
      TRUTH_REQUIRE(entity);
      if (index == 0) {
        const auto subject =
            fw::entities::EntityService::ToGameplayObjectRef(entity.Value().id);
        rt::PersistentObjectRecord record;
        record.persistent_id = rt::PersistentObjectId{entity.Value().id.Low()};
        record.asset_id = rt::AssetId::FromString("whole.actor.asset");
        record.kind = rt::PersistentObjectKind::Object;
        record.tier = rt::PersistenceTier::Protected;
        record.location.region_id = rt::RegionId{1};
        record.location.chunk_id = rt::ChunkId{1};
        record.payload = {fd::StringId::FromString("whole.entity.identity"), 1,
                          EncodeEntityIdentity(entity.Value().id)};
        auto transaction =
            runtime->services.persistence->store->OpenTransaction();
        TRUTH_REQUIRE(transaction && transaction->UpsertObject(record) &&
                      transaction->Commit());
        TRUTH_REQUIRE(framework.time.Schedule(framework.clock,
                                              fw::GameplayTimePoint{1}, subject,
                                              framework.action));
      }
      if (index == 1)
        TRUTH_REQUIRE(framework.progression.EnsureProfile(
            fw::entities::EntityService::ToGameplayObjectRef(
                entity.Value().id)));
    }
    TRUTH_REQUIRE(Advance(std::chrono::microseconds{1'333'333}));
    RebuildRuntimeBridge();
    SynchronizeAndReconcile();
    MaterializePersistentObjects();
    TRUTH_REQUIRE(framework.scheduled.CollectDue(framework.clock, {}));
    TRUTH_REQUIRE(framework.scheduled.DispatchPending().pending == 1);
    TRUTH_REQUIRE(framework.entities.CaptureSnapshot().records.size() == 3);
    TRUTH_REQUIRE(framework.entities.CurrentRevision().value > 0);
  }
  void SeedCrossOwnerReward() {
    const fw::GameplayObjectRef enemy{
        fw::combat::CombatService::Domain(),
        fw::GameplayObjectId::FromString("whole.reward.enemy")};
    TRUTH_REQUIRE(framework.combat.RegisterCombatant(enemy, {}));
    fw::GameplayContext context;
    context.instigator = enemy;
    TRUTH_REQUIRE(framework.combat.TransitionLifeState(
        enemy, fw::combat::CombatLifeState::Dead, context));
    auto generated = framework.death_reward.ProcessChanges();
    TRUTH_REQUIRE(generated && generated.Value().size() == 1);
    TRUTH_REQUIRE(framework.loot.FindGenerated(generated.Value().front()) ||
                  framework.loot.FindPending(generated.Value().front()));
  }
  std::vector<std::byte> DurablePackage() {
    auto lease =
        barrier.AcquireGenerationLease(ig::SaveGenerationId::FromRaw(1));
    TRUTH_REQUIRE(lease);
    auto image = framework.orchestrator.Capture({});
    if (!image)
      throw std::runtime_error("whole Framework capture: " +
                               image.GetError().code);
    auto wire = fw::savegame::EncodeSaveGameImage(image.Value());
    TRUTH_REQUIRE(wire);
    auto runtime_state = capture_port.Capture(ig::SaveGenerationId::FromRaw(1));
    TRUTH_REQUIRE(runtime_state);
    auto package = ig::EncodeSaveGenerationPackage(
        {ig::SaveGenerationId::FromRaw(1), std::move(wire.Value()),
         std::move(runtime_state.Value().time_package),
         std::move(runtime_state.Value().persistence_package)});
    TRUTH_REQUIRE(package);
    return std::move(package).Value();
  }
  SemanticContributions SemanticState() {
    auto lease =
        barrier.AcquireGenerationLease(ig::SaveGenerationId::FromRaw(1));
    TRUTH_REQUIRE(lease);
    SemanticContributions digest;
    AddFrameworkSemanticContributions(framework, digest);
    auto clock = rt::EncodeRuntimeTimeCheckpointPackage(
        runtime->services.time->runtime->CaptureCheckpoint());
    auto snapshot = rt::CaptureRuntimePersistenceSnapshot(
        *runtime->services.persistence->query);
    TRUTH_REQUIRE(clock && snapshot);
    auto persistence = rt::EncodeRuntimePersistencePackage(snapshot.Value());
    TRUTH_REQUIRE(persistence);
    digest.AddBytes("runtime.time", std::move(clock.Value()));
    digest.AddBytes("runtime.persistence", std::move(persistence.Value()));
    TRUTH_REQUIRE(digest.Count() == kFrameworkSemanticOwnerCount + 2);
    return digest;
  }
  std::vector<std::byte> SemanticPackage() {
    return SemanticState().CanonicalBytes();
  }
  void Continue() {
    TRUTH_REQUIRE(CanTick());
    action_ready = true;
    const auto delivered = framework.scheduled.DispatchPending();
    TRUTH_REQUIRE(delivered.pending == 0 && action_commits == 1);
    TRUTH_REQUIRE(framework.scheduled.DispatchPending().pending == 0 &&
                  action_commits == 1);
    TRUTH_REQUIRE(Advance(std::chrono::microseconds{777'777}));
    SynchronizeAndReconcile();
    TRUTH_REQUIRE(framework.progression.CaptureSnapshot().profiles.size() == 2);
    const auto before = bridge->GetDiagnostics().materializations;
    MaterializePersistentObjects();
    TRUTH_REQUIRE(bridge->GetDiagnostics().materializations == before);
  }
};

template <typename Bootstrap, typename Use>
decltype(auto) WithWholeComposition(const std::filesystem::path &path,
                                    Bootstrap &&bootstrap, Use &&use) {
  auto runtime = std::make_unique<RuntimeHost>();
  WholeComposition *live = nullptr;
  class RewardHandler final : public fw::loot::IRewardHandler {
  public:
    fw::loot::RewardTypeId Type() const noexcept override {
      return fw::loot::RewardTypeId::FromString("whole.reward");
    }
    fd::Result<fw::loot::RewardDeliveryStage>
    Prepare(const fw::loot::RewardOperation &operation) override {
      return fd::Result<fw::loot::RewardDeliveryStage>::Success(
          {operation, fw::loot::RewardDeliveryDisposition::Delivered});
    }
    void Commit(fw::loot::RewardDeliveryStage &) noexcept override {}
    void Cancel(fw::loot::RewardDeliveryStage &) noexcept override {}
  } reward_handler;
  return WithFrameworkComposition(
      [&](FrameworkCompositionView &view) {
        TRUTH_REQUIRE(
            view.loot.RegisterRewardHandler("whole.reward", &reward_handler));
        fw::loot::RewardDefinition reward;
        reward.canonical_name = "whole.reward.definition";
        reward.type = reward_handler.Type();
        reward.min_quantity_micro = reward.max_quantity_micro = 1;
        const auto reward_id = view.loot.RegisterRewardDefinition(reward);
        TRUTH_REQUIRE(reward_id);
        fw::loot::LootTableDefinition table;
        table.canonical_name = "whole.reward.table";
        table.policy = fw::loot::LootRollPolicy::GuaranteedAll;
        table.entries.push_back(
            {fw::loot::LootEntryId::FromString("whole.reward.entry"),
             1,
             1'000'000,
             reward_id.Value(),
             {},
             {}});
        const auto table_id = view.loot.RegisterLootTable(table);
        TRUTH_REQUIRE(table_id);
        TRUTH_REQUIRE(view.death_reward.SetTable(
            {fw::combat::CombatService::Domain(),
             fw::GameplayObjectId::FromString("whole.reward.enemy")},
            table_id.Value()));
        fw::entities::EntityArchetypeDefinition definition;
        definition.canonical_name = "whole.actor";
        TRUTH_REQUIRE(view.entities.RegisterArchetype(std::move(definition)));
        const auto action = view.time.RegisterAction(
            "whole.progression", fw::entities::EntityService::Domain());
        TRUTH_REQUIRE(action);
        view.action = action.Value();
        TRUTH_REQUIRE(view.scheduled.RegisterActionHandler(
            view.action,
            ig::ScheduledTriggerHandlerId::FromString("whole.progression"),
            [&](const fw::time::ScheduledTrigger &trigger, const auto &) {
              TRUTH_REQUIRE(live);
              if (!live->action_ready)
                return fd::Result<ig::ScheduledTriggerDisposition>::Success(
                    ig::ScheduledTriggerDisposition::Retry);
              TRUTH_REQUIRE(
                  live->framework.progression.EnsureProfile(trigger.owner));
              ++live->action_commits;
              return fd::Result<ig::ScheduledTriggerDisposition>::Success(
                  ig::ScheduledTriggerDisposition::Ack);
            }));
        bootstrap(view);
      },
      [&](FrameworkCompositionView &view) -> decltype(auto) {
        WholeComposition composition(view, runtime, path);
        live = &composition;
        struct ClearLive {
          WholeComposition *&slot;
          ~ClearLive() { slot = nullptr; }
        } clear{live};
        return use(composition);
      });
}
template <typename Use>
decltype(auto) WithWholeComposition(const std::filesystem::path &path,
                                    Use &&use) {
  return WithWholeComposition(
      path, [](FrameworkCompositionView &) {}, std::forward<Use>(use));
}
} // namespace goal5_test_utility
