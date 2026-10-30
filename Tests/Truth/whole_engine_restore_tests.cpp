#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
#include "goal5_scenario_self_tests.h"
#include "goal5_whole_composition.h"
#include <chrono>
#include <filesystem>
#include <fstream>

namespace epidemic::gameplay::savegame::detail {
void SetSaveGameFaultPointForTesting(std::string_view) noexcept;
void ClearSaveGameFaultPointForTesting() noexcept;
} // namespace epidemic::gameplay::savegame::detail
namespace epidemic::gameplay::runtime_bridge::detail {
void SetRuntimeBridgePreparationFaultForTesting(bool) noexcept;
}

using namespace goal5_test_utility;
namespace {
struct OwnedDirectory {
  std::filesystem::path path;
  OwnedDirectory()
      : path(std::filesystem::temp_directory_path() /
             ("epidemic_ms54_" + std::to_string(std::chrono::steady_clock::now()
                                                    .time_since_epoch()
                                                    .count()))) {
    TRUTH_REQUIRE(std::filesystem::create_directory(path));
  }
  ~OwnedDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};

void FreshBackendsRejectRetiredOpaqueCapabilities() {
  const rt::PersistentObjectId semantic_id{19};
  rb::RuntimePersistentObjectHandle retired;
  {
    rb::EngineRuntimeBridgeBackend old({}, {}, {});
    retired = old.ImportPersistentObjectId(semantic_id);
    TRUTH_REQUIRE(retired.IsValid());
  }
  rb::EngineRuntimeBridgeBackend fresh({}, {}, {});
  const auto current = fresh.ImportPersistentObjectId(semantic_id);
  TRUTH_REQUIRE(current.IsValid() && current != retired);
  auto stale = fresh.Materialize(retired);
  TRUTH_REQUIRE(!stale &&
                stale.GetError().code ==
                    "gameplay.runtime_bridge.persistent_object_unknown");
}

void RetainedConditionJournalReconcilesWithoutChangingProgression() {
  fw::progression::ProgressionService progression;
  fw::progression::AttributeDefinition attribute;
  attribute.canonical_name = "whole.reconcile.attribute";
  attribute.max_micro = 100'000'000;
  const auto attribute_id = progression.RegisterAttribute(attribute);
  TRUTH_REQUIRE(attribute_id && progression.Freeze());
  const fw::GameplayObjectRef actor{
      fw::entities::EntityService::Domain(),
      fw::GameplayObjectId::FromString("whole.reconcile.actor")};
  TRUTH_REQUIRE(progression.EnsureProfile(actor));
  fw::conditions::ConditionService conditions;
  fw::conditions::ConditionDefinition definition;
  definition.canonical_name = "whole.reconcile.condition";
  definition.persistence =
      fw::conditions::ConditionPersistencePolicy::Persistent;
  const auto condition_id = conditions.RegisterCondition(definition);
  TRUTH_REQUIRE(condition_id);
  conditions.Freeze();
  ig::ConditionProgressionAdapter adapter(conditions, progression);
  TRUTH_REQUIRE(adapter.AddMapping(
      {condition_id.Value(), attribute_id.Value(),
       fw::progression::ModifierOperation::FinalAdd,
       fw::TypeId::FromString("whole.reconcile.modifier"), 0, 1'000'000}));
  TRUTH_REQUIRE(adapter.FreezeMappings());
  fw::conditions::ApplyConditionRequest request;
  request.type = condition_id.Value();
  request.subject = actor;
  request.magnitude_micro = 2'000'000;
  TRUTH_REQUIRE(conditions.Apply(request) && adapter.ProcessChanges());
  SemanticContributions before;
  before.AddSnapshot("progression", progression.CaptureSnapshot());
  const auto checkpoint = adapter.CaptureCheckpoint();
  TRUTH_REQUIRE(conditions.RestoreSnapshot(conditions.CaptureSnapshot()) &&
                adapter.RestoreCheckpoint(checkpoint));
  TRUTH_REQUIRE(adapter.ProcessChanges() &&
                adapter.Cursor().epoch ==
                    conditions.LatestChangeCursor().epoch);
  SemanticContributions after;
  after.AddSnapshot("progression", progression.CaptureSnapshot());
  TRUTH_REQUIRE(after.CanonicalBytes() == before.CanonicalBytes());
  TRUTH_REQUIRE(adapter.ProcessChanges());
  SemanticContributions retry;
  retry.AddSnapshot("progression", progression.CaptureSnapshot());
  TRUTH_REQUIRE(retry.CanonicalBytes() == before.CanonicalBytes());
}

void NonemptyCrossOwnerRestoreUsesSavedDependencies() {
  OwnedDirectory directory;
  const auto path = directory.path / "cross-owner.package";
  std::vector<std::byte> saved;
  WithWholeComposition(path, [&](WholeComposition &engine) {
    engine.Seed();
    engine.SeedCrossOwnerReward();
    saved = engine.SemanticPackage();
    const auto captured = engine.saver.CaptureAndCommit({});
    if (!captured)
      throw std::runtime_error(
          "cross-owner capture: " + captured.GetError().code + ": " +
          captured.GetError().message);
  });
  WithWholeComposition(path, [&](WholeComposition &engine) {
    TRUTH_REQUIRE(engine.framework.loot.GetDiagnostics().bundles == 0);
    const auto before = engine.DurablePackage();
    fw::savegame::detail::SetSaveGameFaultPointForTesting("restore.pre_commit");
    const auto failed = engine.Restore();
    fw::savegame::detail::ClearSaveGameFaultPointForTesting();
    TRUTH_REQUIRE(!failed);
    TRUTH_REQUIRE(engine.DurablePackage() == before);
    TRUTH_REQUIRE(engine.Restore());
    TRUTH_REQUIRE(engine.SemanticPackage() == saved);
    TRUTH_REQUIRE(engine.framework.death_reward.ProcessChanges());
    TRUTH_REQUIRE(engine.SemanticPackage() == saved);
  });
}

void DeathRewardRejectsUnknownDeliveryStateAtomically() {
  OwnedDirectory directory;
  WithWholeComposition(
      directory.path / "delivery-state.package", [&](WholeComposition &engine) {
        engine.Seed();
        engine.SeedCrossOwnerReward();
        const auto before = engine.framework.death_reward.CaptureCheckpoint();
        auto invalid = before;
        TRUTH_REQUIRE(!invalid.deliveries.empty());
        invalid.deliveries.front().state =
            static_cast<ig::DeathRewardDeliveryState>(99);
        TRUTH_REQUIRE(
            !engine.framework.death_reward.RestoreCheckpoint(invalid));
        SemanticContributions expected, actual;
        expected.AddSnapshot("death_reward", before);
        actual.AddSnapshot("death_reward",
                           engine.framework.death_reward.CaptureCheckpoint());
        TRUTH_REQUIRE(actual.CanonicalBytes() == expected.CanonicalBytes());
      });
}

void SocialLifecycleCursorExcludesRebuiltJournalIdentity() {
  fw::social_legal_integration::SocialLegalCheckpoint before;
  before.mapping_revision = fw::Revision{23};
  before.crime_cursor = {5, 9};
  fw::social_legal_integration::RelationshipPenaltyDelivery delivery;
  delivery.crime.value = fw::GameplayObjectId::FromString("policy.crime");
  delivery.mapping =
      fw::social_legal_integration::SocialLegalMappingId::FromString(
          "policy.mapping");
  before.applied_relationship_penalties.push_back(delivery);
  const auto bytes = [](auto checkpoint) {
    SemanticContributions state;
    state.AddSnapshot("social_legal", checkpoint);
    return state.CanonicalBytes();
  };
  auto rebound = before;
  rebound.crime_cursor = {6, 0};
  TRUTH_REQUIRE(bytes(before) == bytes(rebound));
  ++rebound.mapping_revision.value;
  TRUTH_REQUIRE(bytes(before) != bytes(rebound));
  rebound = before;
  rebound.applied_relationship_penalties.clear();
  TRUTH_REQUIRE(bytes(before) != bytes(rebound));
  rebound = before;
  rebound.crime_cursor = {};
  TRUTH_REQUIRE(bytes(before) != bytes(rebound));
}

void SemanticObservationPoliciesPreserveDurableFields() {
  const auto bytes = [](auto snapshot) {
    SemanticContributions state;
    state.AddSnapshot("policy", snapshot);
    return state.CanonicalBytes();
  };
  fw::entities::EntitySnapshot before;
  auto rebound = before;
  ++rebound.change_epoch;
  TRUTH_REQUIRE(bytes(before) == bytes(rebound));
  ++rebound.id_generator.next;
  TRUTH_REQUIRE(bytes(before) != bytes(rebound));
  rebound = before;
  ++rebound.revision.value;
  TRUTH_REQUIRE(bytes(before) != bytes(rebound));
  fw::state_integration::StateIntegrationCheckpoint cursors;
  cursors.facts.entity_cursor = cursors.facts.entity_latest = {1, 3};
  auto rebuilt = cursors;
  rebuilt.facts.entity_cursor = rebuilt.facts.entity_latest = {2, 0};
  TRUTH_REQUIRE(bytes(cursors) == bytes(rebuilt));
  cursors.facts.condition_cursor = cursors.facts.condition_latest = {1, 7};
  rebuilt = cursors;
  rebuilt.facts.condition_cursor = rebuilt.facts.condition_latest = {2, 7};
  TRUTH_REQUIRE(bytes(cursors) == bytes(rebuilt));
  rebuilt.facts.condition_cursor.sequence = 6;
  TRUTH_REQUIRE(bytes(cursors) != bytes(rebuilt));
  rebuilt = cursors;
  ++rebuilt.condition_effects.route_revision;
  TRUTH_REQUIRE(bytes(cursors) != bytes(rebuilt));
  TRUTH_REQUIRE(bytes(fw::ChangeCursor{}) != bytes(fw::ChangeCursor{1, 0}));
  fw::savegame::detail::SnapshotWriter writer;
  auto saved_cursor = fw::ChangeCursor{7, 11};
  fw::savegame::detail::TransferSnapshot(writer, saved_cursor);
  fw::savegame::detail::SnapshotReader reader{writer.bytes};
  fw::ChangeCursor decoded_cursor;
  fw::savegame::detail::TransferSnapshot(reader, decoded_cursor);
  TRUTH_REQUIRE(reader.Remaining() == 0 && decoded_cursor == saved_cursor);
}

void FreshWholeEngineRestoresAllPlanesAndContinuesCausally() {
  OwnedDirectory directory;
  const auto path = directory.path / "generation.bin";
  std::vector<std::byte> before, expected_continuation;
  std::string saved_semantic_digest, continued_semantic_digest;
  SemanticContributions saved_contributions;
  std::weak_ptr<rt::IGameClock> retired_clock;
  rb::RuntimeBinding retired_binding;
  fw::entities::EntityHandle retired_entity_handle;
  fw::ChangeCursor retired_entity_cursor;
  fw::entities::EntitySnapshot saved_entities;
  rt::TimeCheckpoint saved_time;
  WithWholeComposition(path, [&](WholeComposition &engine) {
    engine.Seed();
    auto records = rt::CaptureRuntimePersistenceSnapshot(
        *engine.runtime->services.persistence->query);
    TRUTH_REQUIRE(records && records.Value().objects.size() == 1);
    const auto entity = DecodeEntityIdentity(records.Value().objects.front());
    retired_entity_handle = engine.framework.entities.HandleOf(entity);
    TRUTH_REQUIRE(retired_entity_handle.IsValid());
    const auto subject =
        fw::entities::EntityService::ToGameplayObjectRef(entity);
    TRUTH_REQUIRE(engine.bridge->GetBinding(subject));
    retired_binding = *engine.bridge->GetBinding(subject);
    saved_entities = engine.framework.entities.CaptureSnapshot();
    retired_entity_cursor = engine.framework.entities.LatestChangeCursor();
    saved_time = engine.runtime->services.time->runtime->CaptureCheckpoint();
    TRUTH_REQUIRE(saved_time.now.ticks > 0 &&
                  saved_time.tick_remainder_numerator > 0);
    retired_clock = engine.runtime->services.time->clock;
    before = engine.SemanticPackage();
    saved_contributions = engine.SemanticState();
    saved_semantic_digest = saved_contributions.Digest();
    const auto generation = engine.saver.CaptureAndCommit({});
    TRUTH_REQUIRE(generation && generation.Value().Raw() == 1);
    engine.Continue();
    expected_continuation = engine.SemanticPackage();
    continued_semantic_digest = engine.SemanticState().Digest();
    TRUTH_REQUIRE(before != expected_continuation);
  });
  TRUTH_REQUIRE(retired_clock.expired());
  WithWholeComposition(path, [&](WholeComposition &engine) {
    TRUTH_REQUIRE(engine.framework.entities.AllEntities().empty());
    TRUTH_REQUIRE(engine.runtime->services.persistence->query->GetRevision() ==
                  0);
    TRUTH_REQUIRE(!engine.restorer.CanResume());
    const auto generation = engine.Restore();
    TRUTH_REQUIRE(generation && generation.Value().Raw() == 1 &&
                  engine.restorer.CanResume());
    const auto restored_entities = engine.framework.entities.CaptureSnapshot();
    TRUTH_REQUIRE(!engine.framework.entities.Resolve(retired_entity_handle));
    TRUTH_REQUIRE(
        engine.framework.entities.ReadChangesSince(retired_entity_cursor)
            .snapshot_required);
    engine.VerifyObservationBindings();
    TRUTH_REQUIRE(restored_entities.records.size() == 3 &&
                  restored_entities.id_generator.scope ==
                      saved_entities.id_generator.scope);
    TRUTH_REQUIRE(restored_entities.id_generator.next ==
                  saved_entities.id_generator.next);
    TRUTH_REQUIRE(restored_entities.revision == saved_entities.revision);
    for (std::size_t index = 0; index != saved_entities.records.size();
         ++index) {
      TRUTH_REQUIRE(restored_entities.records[index].id ==
                    saved_entities.records[index].id);
      TRUTH_REQUIRE(restored_entities.records[index].revision ==
                    saved_entities.records[index].revision);
    }
    const auto restored_time =
        engine.runtime->services.time->runtime->CaptureCheckpoint();
    TRUTH_REQUIRE(restored_time.now == saved_time.now &&
                  restored_time.revision == saved_time.revision);
    TRUTH_REQUIRE(restored_time.tick_remainder_numerator ==
                  saved_time.tick_remainder_numerator);
    TRUTH_REQUIRE(engine.framework.scheduled.CaptureSnapshot().pending.size() ==
                  1);
    const auto binding =
        engine.bridge->GetBinding(retired_binding.gameplay_object);
    TRUTH_REQUIRE(binding &&
                  binding->runtime_object != retired_binding.runtime_object);
    TRUTH_REQUIRE(binding->generation != retired_binding.generation);
    TRUTH_REQUIRE(binding->gameplay_revision ==
                  retired_binding.gameplay_revision);
    TRUTH_REQUIRE(!engine.bridge->GetBinding(retired_binding.runtime_object));
    TRUTH_REQUIRE(
        !engine.backend->ObserveTransform(retired_binding.runtime_object));
    TRUTH_REQUIRE(!engine.bridge->AttachPhysicsBody(
        retired_binding.gameplay_object, retired_binding.generation,
        rb::RuntimePhysicsBodyHandle{91}));
    const auto reverse_binding =
        engine.bridge->GetBinding(binding->runtime_object);
    TRUTH_REQUIRE(reverse_binding && reverse_binding->gameplay_object ==
                                         retired_binding.gameplay_object);
    const auto restored_contributions = engine.SemanticState();
    TRUTH_REQUIRE(restored_contributions.ObservationEpochs().size() ==
                  saved_contributions.ObservationEpochs().size());
    for (const auto &[owner, epoch] : saved_contributions.ObservationEpochs())
      TRUTH_REQUIRE(restored_contributions.ObservationEpochs().at(owner) >
                    epoch);
    TRUTH_REQUIRE(engine.SemanticPackage() == before);
    TRUTH_REQUIRE(engine.SemanticState().Digest() == saved_semantic_digest);
    engine.Continue();
    TRUTH_REQUIRE(engine.SemanticPackage() == expected_continuation);
    TRUTH_REQUIRE(engine.SemanticState().Digest() == continued_semantic_digest);
  });
}

void WholeLoadAndPrecommitFailuresAreAtomic() {
  OwnedDirectory directory;
  const auto path = directory.path / "generation.bin";
  std::vector<std::byte> valid;
  WithWholeComposition(path, [&](WholeComposition &engine) {
    engine.Seed();
    TRUTH_REQUIRE(engine.saver.CaptureAndCommit({}));
    valid = *engine.store.LoadCurrent().Value();
  });
  for (const auto point : {"missing", "corrupt", "truncated", "future",
                           "validate", "stage", "prepare", "precommit"}) {
    if (point == std::string_view{"missing"})
      TRUTH_REQUIRE(std::filesystem::remove(path));
    else {
      auto bytes = valid;
      if (point == std::string_view{"corrupt"})
        bytes.back() ^= std::byte{1};
      if (point == std::string_view{"truncated"})
        bytes.resize(bytes.size() / 2);
      if (point == std::string_view{"future"}) {
        bytes[8] = std::byte{255};
        const auto checksum = fw::savegame::SaveGameOrchestrator::HashBytes(
            std::span<const std::byte>{bytes}.first(bytes.size() - 8));
        for (unsigned index = 0; index != 8; ++index)
          bytes[bytes.size() - 8 + index] =
              static_cast<std::byte>((checksum >> (8 * index)) & 255);
      }
      std::ofstream output(path, std::ios::binary | std::ios::trunc);
      output.write(reinterpret_cast<const char *>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
      TRUTH_REQUIRE(output);
    }
    WithWholeComposition(path, [&](WholeComposition &engine) {
      engine.Seed();
      engine.Continue();
      const auto before = engine.DurablePackage();
      const auto old_clock = engine.runtime->services.time->clock;
      const auto old_binding_count =
          engine.bridge->GetDiagnostics().active_bindings;
      struct ClearFault {
        ~ClearFault() {
          fw::savegame::detail::ClearSaveGameFaultPointForTesting();
        }
      } clear;
      if (point == std::string_view{"validate"})
        fw::savegame::detail::SetSaveGameFaultPointForTesting(
            "restore.validate");
      if (point == std::string_view{"stage"})
        fw::savegame::detail::SetSaveGameFaultPointForTesting("restore.stage");
      if (point == std::string_view{"precommit"})
        fw::savegame::detail::SetSaveGameFaultPointForTesting(
            "restore.pre_commit");
      engine.restore_port.fail_prepare = point == std::string_view{"prepare"};
      const auto failure = engine.Restore();
      TRUTH_REQUIRE(!failure);
      if (point == std::string_view{"missing"})
        TRUTH_REQUIRE(failure.GetError().code ==
                      "gameplay.restore.missing_package");
      if (point == std::string_view{"future"})
        TRUTH_REQUIRE(failure.GetError().code ==
                      "gameplay.save_generation.version");
      fw::savegame::detail::ClearSaveGameFaultPointForTesting();
      TRUTH_REQUIRE(engine.restorer.Phase() ==
                    ig::WholeRestorePhase::AwaitLoad);
      TRUTH_REQUIRE(!engine.quiesced && !engine.restorer.CanResume());
      TRUTH_REQUIRE(!engine.Advance(std::chrono::microseconds{1}));
      TRUTH_REQUIRE(engine.runtime->services.time->clock == old_clock);
      TRUTH_REQUIRE(engine.restore_port.prepared_clock.expired());
      if (point == std::string_view{"validate"} ||
          point == std::string_view{"stage"} ||
          point == std::string_view{"precommit"})
        TRUTH_REQUIRE(engine.restore_port.prepared_count == 1);
      else
        TRUTH_REQUIRE(engine.restore_port.prepared_count == 0);
      TRUTH_REQUIRE(engine.bridge->GetDiagnostics().active_bindings ==
                    old_binding_count);
      TRUTH_REQUIRE(engine.DurablePackage() == before);
    });
    // Repair only the owned test package, not the live composition.
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(valid.data()),
                 static_cast<std::streamsize>(valid.size()));
    TRUTH_REQUIRE(output);
  }
}

void WholePostcommitRecoveryRetainsSemanticsAndNoDuplicateRuntimeObject() {
  OwnedDirectory directory;
  const auto path = directory.path / "generation.bin";
  std::vector<std::byte> saved, continued;
  WithWholeComposition(path, [&](WholeComposition &engine) {
    engine.Seed();
    saved = engine.SemanticPackage();
    TRUTH_REQUIRE(engine.saver.CaptureAndCommit({}));
    engine.Continue();
    continued = engine.SemanticPackage();
  });
  for (const auto point :
       {"rebuild", "reconcile.accepted", "materialize.world_accepted",
        "materialize.queued", "materialize.accepted", "bridge.prepare"})
    WithWholeComposition(path, [&](WholeComposition &engine) {
      engine.fault_once = point;
      struct ClearBridgeFault {
        ~ClearBridgeFault() {
          rb::detail::SetRuntimeBridgePreparationFaultForTesting(false);
        }
      } clear;
      rb::detail::SetRuntimeBridgePreparationFaultForTesting(
          point == std::string_view{"bridge.prepare"});
      TRUTH_REQUIRE(!engine.Restore());
      rb::detail::SetRuntimeBridgePreparationFaultForTesting(false);
      TRUTH_REQUIRE(engine.restorer.Phase() ==
                    ig::WholeRestorePhase::ReconciliationRequired);
      TRUTH_REQUIRE(engine.restorer.RestoredGeneration().Raw() == 1);
      TRUTH_REQUIRE(engine.quiesced && !engine.restorer.CanResume());
      TRUTH_REQUIRE(!engine.Advance(std::chrono::microseconds{1}));
      TRUTH_REQUIRE(
          engine.framework.entities.CaptureSnapshot().records.size() == 3);
      TRUTH_REQUIRE(engine.runtime->services.persistence->query->GetRevision() >
                    0);
      const auto retried = engine.restorer.RetryReconciliation();
      if (!retried)
        std::cerr << "retry " << point << ": " << retried.GetError().code
                  << ": " << retried.GetError().message << '\n';
      TRUTH_REQUIRE(retried);
      TRUTH_REQUIRE(engine.restorer.CanResume() && !engine.quiesced);
      TRUTH_REQUIRE(!engine.restorer.RetryReconciliation());
      TRUTH_REQUIRE(engine.bridge->GetDiagnostics().active_bindings == 1);
      TRUTH_REQUIRE(engine.bridge->GetDiagnostics().materializations == 1);
      TRUTH_REQUIRE(engine.runtime->services.world->query
                        ->FindObjectsInRegion(rt::RegionId{1})
                        .size() == 1);
      TRUTH_REQUIRE(engine.SemanticPackage() == saved);
      engine.Continue();
      TRUTH_REQUIRE(engine.SemanticPackage() == continued);
    });
}

std::uint64_t RepresentativeFingerprint(const std::vector<std::byte> &bytes) {
  std::uint64_t hash = 14695981039346656037ull;
  for (auto byte : bytes) {
    hash ^= std::to_integer<std::uint8_t>(byte);
    hash *= 1099511628211ull;
  }
  return hash;
}
} // namespace

int main(int argc, char **argv) {
  if (argc == 3) {
    try {
      const std::string_view mode = argv[1];
      TRUTH_REQUIRE(mode == "--write" || mode == "--read");
      WithWholeComposition(argv[2], [&](WholeComposition &engine) {
        if (mode == "--write") {
          engine.Seed();
          TRUTH_REQUIRE(engine.saver.CaptureAndCommit({}));
        } else
          TRUTH_REQUIRE(engine.Restore());
        engine.Continue();
        std::cout << "WHOLE_FINGERPRINT "
                  << RepresentativeFingerprint(engine.SemanticPackage())
                  << '\n';
      });
      return 0;
    } catch (const std::exception &error) {
      std::cerr << error.what() << '\n';
      return 1;
    }
  }
  if (argc != 1)
    return 2;
  return epidemic::truth_tests::Run(
      "Goal5 whole-engine restore",
      {
          {"social lifecycle cursor excludes rebuilt journal identity",
           SocialLifecycleCursorExcludesRebuiltJournalIdentity},
          {"unknown death delivery state is rejected atomically",
           DeathRewardRejectsUnknownDeliveryStateAtomically},
          {"nonempty cross-owner restore uses saved dependencies",
           NonemptyCrossOwnerRestoreUsesSavedDependencies},
          {"retained condition journal reconciliation is idempotent",
           RetainedConditionJournalReconcilesWithoutChangingProgression},
          {"observation identity policy preserves durable fields",
           SemanticObservationPoliciesPreserveDurableFields},
          {"fresh production backend rejects retired capability",
           FreshBackendsRejectRetiredOpaqueCapabilities},
          {"fresh Base Runtime Framework restore and causal continuation",
           FreshWholeEngineRestoresAllPlanesAndContinuesCausally},
          {"whole LOAD and precommit failures leave all planes unchanged",
           WholeLoadAndPrecommitFailuresAreAtomic},
          {"whole postcommit recovery blocks resume and materializes exactly "
           "once",
           WholePostcommitRecoveryRetainsSemanticsAndNoDuplicateRuntimeObject},
          {"canonical runner rejects adversarial policies and detects "
           "divergence",
           CanonicalScenarioRunnerRejectsAdversarialInputs},
      });
}
