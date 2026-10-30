#include "Epidemic/GameFramework/Integration/core_adapters.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>

using namespace epidemic;
using namespace epidemic::gameplay;
using namespace epidemic::gameplay::integration;
using namespace epidemic::gameplay::savegame;

namespace epidemic::gameplay::savegame::detail {
void SetSaveGameFaultPointForTesting(std::string_view point) noexcept;
void ClearSaveGameFaultPointForTesting() noexcept;
} // namespace epidemic::gameplay::savegame::detail

namespace epidemic::gameplay::integration::detail {
void SetSaveGenerationFaultPointForTesting(std::string_view point) noexcept;
void ClearSaveGenerationFaultPointForTesting() noexcept;
} // namespace epidemic::gameplay::integration::detail

namespace {
static_assert(!std::is_copy_constructible_v<WholeEngineRestoreCoordinator>);
static_assert(!std::is_copy_assignable_v<WholeEngineRestoreCoordinator>);
static_assert(std::has_virtual_destructor_v<IRuntimeGenerationRestoreStage>);
static_assert(std::has_virtual_destructor_v<IRuntimeGenerationRestorePort>);

bool Check(bool condition, const char *assertion) {
    if (!condition)
        std::cerr << "G5.4.A " << assertion << '\n';
    return condition;
}

class PlainLease final : public ISaveBarrierLease {};

class PlainBarrier final : public ISaveBarrier {
  public:
    foundation::Result<std::unique_ptr<ISaveBarrierLease>> AcquireCaptureLease() override {
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(
            std::make_unique<PlainLease>());
    }

    foundation::Result<std::unique_ptr<ISaveBarrierLease>> AcquireRestoreLease() override {
        return AcquireCaptureLease();
    }

};

class Quiescence final : public ISaveBarrierLease {
  public:
    Quiescence(bool &gate, int &releases) : gate_(gate), releases_(releases) {
        gate_ = true;
    }

    ~Quiescence() override {
        gate_ = false;
        ++releases_;
    }

  private:
    bool &gate_;
    int &releases_;
};

class SectionStage final : public IRestoreStage {
  public:
    explicit SectionStage(int candidate, int &disposals) : value(candidate), disposals_(disposals) {
    }

    ~SectionStage() override {
        ++disposals_;
    }

    int value;

  private:
    int &disposals_;
};

class Participant final : public ISaveParticipant {
  public:
    Participant(int &live, int &disposals) : live_(live), disposals_(disposals) {
    }

    bool fail_validate = false, fail_stage = false;

    SaveParticipantId Id() const noexcept override {
        return SaveParticipantId::FromString("restore.coordination.owner");
    }

    SaveSchemaVersion SchemaVersion() const noexcept override {
        return 1;
    }

    std::vector<SaveParticipantId> Dependencies() const override {
        return {};

    }

    foundation::Result<SaveSection> CaptureSnapshot(const SaveContext &) const override {
        SaveSection section{};

        section.participant = Id();
        section.schema_version = 1;
        section.payload = {std::byte(live_)};

        section.payload_hash = SaveGameOrchestrator::HashBytes(section.payload);
        return foundation::Result<SaveSection>::Success(std::move(section));
    }

    foundation::Result<void> ValidateSnapshot(const SaveSection &section,
                                              const RestoreContext &) const override {
        if (fail_validate || section.participant != Id() || section.payload.size() != 1 ||
            section.payload_hash != SaveGameOrchestrator::HashBytes(section.payload))
            return foundation::Result<void>::Failure(
                foundation::Error::Create("test.restore.validate", "validation failed"));
        return foundation::Result<void>::Success();
    }

    foundation::Result<std::unique_ptr<IRestoreStage>>
    StageRestore(const SaveSection &section, const RestoreContext &) override {
        if (fail_stage)
            return foundation::Result<std::unique_ptr<IRestoreStage>>::Failure(
                foundation::Error::Create("test.restore.stage", "staging failed"));
        return foundation::Result<std::unique_ptr<IRestoreStage>>::Success(
            std::make_unique<SectionStage>(std::to_integer<int>(section.payload.front()),
                                           disposals_));
    }

    void CommitRestore(IRestoreStage &stage) noexcept override {
        live_ = static_cast<SectionStage &>(stage).value;
    }

  private:
    int &live_;
    int &disposals_;
};

class PreparedRuntime final : public IRuntimeGenerationRestoreStage {
  public:
    PreparedRuntime(int &live, int &commits, int &disposals)
        : live_(live), commits_(commits), disposals_(disposals) {
    }

    ~PreparedRuntime() override {
        ++disposals_;
    }

    void Commit() noexcept override {
        live_ = 42;
        ++commits_;
    }

  private:
    int &live_;
    int &commits_;
    int &disposals_;
};

class RuntimePort final : public IRuntimeGenerationRestorePort {
  public:
    RuntimePort(int &live, int &commits, int &disposals)
        : live_(live), commits_(commits), disposals_(disposals) {
    }

    bool fail_prepare = false;

    foundation::Result<std::unique_ptr<IRuntimeGenerationRestoreStage>>
    Prepare(const SaveGenerationPackage &package) override {
        if (fail_prepare || !package.generation.IsValid())
            return foundation::Result<std::unique_ptr<IRuntimeGenerationRestoreStage>>::Failure(
                foundation::Error::Create("test.restore.runtime", "Runtime prepare failed"));
        return foundation::Result<std::unique_ptr<IRuntimeGenerationRestoreStage>>::Success(
            std::make_unique<PreparedRuntime>(live_, commits_, disposals_));
    }

  private:
    int &live_;
    int &commits_;
    int &disposals_;
};

class Store final : public ISaveGenerationStore {
  public:
    std::optional<std::vector<std::byte>> current;
    std::function<void()> load_probe;

    foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() override {
        if (load_probe)
            load_probe();
        return foundation::Result<std::optional<std::vector<std::byte>>>::Success(current);
    }

    foundation::Result<void> CommitCurrent(std::span<const std::byte> bytes) override {
        current = std::vector<std::byte>(bytes.begin(), bytes.end());
        return foundation::Result<void>::Success();
    }

};

struct Fixture {
    int framework_value = 7, runtime_value = 0, commits = 0, runtime_disposals = 0,
        participant_disposals = 0, releases = 0;
    bool quiesced = false;
    Participant participant{framework_value, participant_disposals};

    SaveGameOrchestrator framework;
    Store store;
    RuntimePort runtime{runtime_value, commits, runtime_disposals};

    SaveGenerationBarrier barrier{Callbacks()};

    SaveGenerationBarrierCallbacks Callbacks() {
        SaveGenerationBarrierCallbacks callbacks{};

        callbacks.acquire_capture_quiescence = [&]() {
            return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(
                std::make_unique<Quiescence>(quiesced, releases));
        };

        callbacks.acquire_restore_quiescence = callbacks.acquire_capture_quiescence;
        callbacks.capture_preflight = []() { return foundation::Result<void>::Success(); };

        return callbacks;
    }

    bool Seed() {
        PlainBarrier capture_barrier;
        if (!framework.SetBarrier(capture_barrier) || !framework.RegisterParticipant(participant) ||
            !framework.FreezeRegistry())
            return false;
        auto image = framework.Capture({});
        if (!image)
            return false;
        auto wire = EncodeSaveGameImage(image.Value());
        if (!wire)
            return false;
        SaveGenerationPackage package{SaveGenerationId::FromRaw(3),
                                      std::move(wire.Value()),
                                      {std::byte{1}},
                                      {std::byte{2}}};

        auto bytes = EncodeSaveGenerationPackage(package);
        if (!bytes)
            return false;
        store.current = std::move(bytes.Value());
        framework_value = 0;
        return bool(framework.SetBarrier(barrier));
    }

};

WholeRestoreCallbacks SuccessfulCallbacks() {
    auto success = []() { return foundation::Result<void>::Success(); };

    return {success, success, success};

}

bool PrecommitFailurePreservesBothPlanesAndReleasesPreparedState() {
    for (int point = 0; point != 5; ++point) {
        Fixture fixture;
        if (!fixture.Seed())
            return false;
        fixture.participant.fail_validate = point == 0;
        fixture.participant.fail_stage = point == 1;
        fixture.runtime.fail_prepare = point == 2;
        WholeEngineRestoreCoordinator restore(fixture.framework, fixture.runtime, fixture.barrier,
                                              fixture.store, SuccessfulCallbacks());
        if (!Check(!restore.CanResume() && !restore.RestoredGeneration().IsValid(),
                   "fresh restore starts with simulation disabled"))
            return false;
        if (point == 3)
            integration::detail::SetSaveGenerationFaultPointForTesting("lease.restore_nested");
        if (point == 4)
            savegame::detail::SetSaveGameFaultPointForTesting("restore.pre_commit");
        auto failed = restore.LoadAndRestore({});
        integration::detail::ClearSaveGenerationFaultPointForTesting();
        savegame::detail::ClearSaveGameFaultPointForTesting();
        if (!Check(!failed && fixture.framework_value == 0 && fixture.runtime_value == 0 &&
                       fixture.commits == 0,
                   "validation, staging and commit preparation failure preserve both live planes"))
            return false;
        if (!Check(!restore.CanResume() && restore.Phase() == WholeRestorePhase::AwaitLoad &&
                       !fixture.quiesced && fixture.releases == 1,
                   "precommit failure releases the complete restore lease and never resumes"))
            return false;
        if (!Check(fixture.runtime_disposals == (point == 2 ? 0 : 1),
                   "unpublished Runtime prepared state is released on precommit failure"))
            return false;
        fixture.participant.fail_validate = fixture.participant.fail_stage =
            fixture.runtime.fail_prepare = false;
        auto retry = restore.LoadAndRestore({});
        if (!Check(bool(retry) && fixture.framework_value == 7 && fixture.runtime_value == 42 &&
                       fixture.commits == 1 && restore.CanResume(),
                   "precommit retry publishes both staged planes exactly once"))
            return false;
    }

    return true;
}

bool PostcommitFailureRetainsSemanticsAndRetriesOnlyUnfinishedPhases() {
    for (std::size_t fault = 0; fault != 4; ++fault) {
        Fixture fixture;
        if (!fixture.Seed())
            return false;
        std::array<int, 3> calls{};

        bool fail_once = true, external_accepted = false;
        int external_effects = 0, ticks = 0;
        std::unique_ptr<WholeEngineRestoreCoordinator> restore;
        auto callback = [&](std::size_t step) {
            ++calls[step];
            if (!Check(!restore->CanResume() && fixture.quiesced,
                       "simulation remains guarded throughout reconciliation"))
                return foundation::Result<void>::Failure(
                    foundation::Error::Create("test.guard", "resume guard failed"));
            if (step == 1 && !external_accepted) {
                external_accepted = true;
                ++external_effects;
            }

            if (fail_once && (step == fault || (fault == 3 && step == 1))) {
                fail_once = false;
                if (fault == 3)
                    throw std::runtime_error("external provider threw after acceptance");
                return foundation::Result<void>::Failure(
                    foundation::Error::Create("test.reconcile", "postcommit failure"));
            }

            return foundation::Result<void>::Success();
        };

        WholeRestoreCallbacks callbacks{[&]() { return callback(0); },
                                        [&]() { return callback(1); },
                                        [&]() { return callback(2); }};

        restore =
            std::make_unique<WholeEngineRestoreCoordinator>(fixture.framework, fixture.runtime,
                                                            fixture.barrier, fixture.store,
                                                            std::move(callbacks));
        auto failed = restore->LoadAndRestore({});
        if (restore->CanResume())
            ++ticks;
        if (!Check(!failed && fixture.framework_value == 7 && fixture.runtime_value == 42 &&
                       fixture.commits == 1,
                   "postcommit provider failure retains restored semantic state in both planes"))
            return false;
        if (!Check(restore->Phase() == WholeRestorePhase::ReconciliationRequired &&
                       fixture.quiesced && ticks == 0,
                   "postcommit failure owns reconciliation and blocks simulation resume"))
            return false;
        if (!Check(!restore->LoadAndRestore({}),
                   "reconciliation-required cycle cannot repeat semantic restore"))
            return false;
        if (!Check(bool(restore->RetryReconciliation()) && restore->CanResume() &&
                       !fixture.quiesced && fixture.releases == 1,
                   "successful retry releases quiescence only after all restore phases complete"))
            return false;
        const auto failed_step = fault == 3 ? 1 : fault;
        for (std::size_t step = 0; step != calls.size(); ++step)
            if (!Check(calls[step] == (step == failed_step ? 2 : 1),
                       "retry does not replay already successful reconciliation phases"))
                return false;
        if (!Check(external_effects == 1 && fixture.commits == 1 && !restore->RetryReconciliation(),
                   "accepted external effect and semantic commit are not duplicated by retry"))
            return false;
        std::thread worker([&]() {
            if (restore->LoadAndRestore({}))
                ++ticks;
        });
        worker.join();
        if (!Check(ticks == 0, "restore rejects wrong-thread calls without live mutation"))
            return false;
    }

    return true;
}

bool DestroyingStoppedCompositionReleasesRestoreOwnership() {
    Fixture fixture;
    if (!fixture.Seed())
        return false;
    {
        auto callbacks = SuccessfulCallbacks();
        callbacks.materialize_runtime = []() {
            return foundation::Result<void>::Failure(
                foundation::Error::Create("test.shutdown", "stopped composition"));
        };

        WholeEngineRestoreCoordinator restore(fixture.framework, fixture.runtime, fixture.barrier,
                                              fixture.store, std::move(callbacks));
        if (!Check(!restore.LoadAndRestore({}) && fixture.quiesced && !restore.CanResume(),
                   "stopped composition retains reconciliation lease until destruction"))
            return false;
    }

    return Check(!fixture.quiesced && fixture.releases == 1 && fixture.runtime_disposals == 1 &&
                     fixture.commits == 1,
                 "coordinator destruction releases owned restore resources without replaying "
                 "semantic publication");
}

bool StoreCallbackCannotReenterRestoreBeforeQuiescence() {
    Fixture fixture;
    if (!fixture.Seed())
        return false;
    WholeEngineRestoreCoordinator restore(fixture.framework, fixture.runtime, fixture.barrier,
                                          fixture.store, SuccessfulCallbacks());
    int loads = 0;
    bool rejected = false;
    fixture.store.load_probe = [&]() {
        ++loads;
        rejected = !restore.LoadAndRestore({}) && !restore.RetryReconciliation();
    };

    const auto restored = restore.LoadAndRestore({});
    return Check(
        bool(restored) && rejected && loads == 1 && fixture.commits == 1 && restore.CanResume(),
        "store callback reentrancy is rejected before LOAD without duplicate semantic publication");
}

} // namespace

int main() {
    return PrecommitFailurePreservesBothPlanesAndReleasesPreparedState() &&
                   PostcommitFailureRetainsSemanticsAndRetriesOnlyUnfinishedPhases() &&
                   DestroyingStoppedCompositionReleasesRestoreOwnership() &&
                   StoreCallbackCannotReenterRestoreBeforeQuiescence()
               ? 0
               : 1;
}
