#include "Epidemic/GameFramework/Integration/core_adapters.h"

#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <vector>

using namespace epidemic;
using namespace epidemic::gameplay;
using namespace epidemic::gameplay::integration;
using namespace epidemic::gameplay::savegame;

// Private, thread-local logical seams; not part of the consumer header surface.
namespace epidemic::gameplay::integration::detail
{
void SetSaveGenerationFaultPointForTesting(std::string_view point) noexcept;
void ClearSaveGenerationFaultPointForTesting() noexcept;
}

namespace
{
class TestSaveBarrierLease final : public ISaveBarrierLease
{
};

class CountingQuiescenceLease final : public ISaveBarrierLease
{
  public:
    explicit CountingQuiescenceLease(int& releases) : releases_(releases) {}
    ~CountingQuiescenceLease() override { ++releases_; }
  private:
    int& releases_;
};

class GenerationTestStage final : public IRestoreStage
{
};

class GenerationTestParticipant final : public ISaveParticipant
{
  public:
    bool fail_capture = false;
    std::function<void()> capture_probe;
    [[nodiscard]] SaveParticipantId Id() const noexcept override
    { return SaveParticipantId::FromString("integration.generation_test"); }
    [[nodiscard]] SaveSchemaVersion SchemaVersion() const noexcept override { return 1; }
    [[nodiscard]] std::vector<SaveParticipantId> Dependencies() const override { return {}; }
    [[nodiscard]] foundation::Result<SaveSection> CaptureSnapshot(const SaveContext&) const override
    {
        if (capture_probe) capture_probe();
        if (fail_capture)
            return foundation::Result<SaveSection>::Failure(foundation::Error::Create("test.generation.capture", "injected participant capture failure"));
        SaveSection section{};
        section.participant=Id(); section.schema_version=SchemaVersion();
        section.payload={std::byte{0x31},std::byte{0x32},std::byte{0x33}};
        section.payload_hash=SaveGameOrchestrator::HashBytes(section.payload);
        return foundation::Result<SaveSection>::Success(std::move(section));
    }
    [[nodiscard]] foundation::Result<void> ValidateSnapshot(const SaveSection& section,const RestoreContext&) const override
    {
        if (section.participant!=Id() || section.schema_version!=SchemaVersion() ||
            section.payload_hash!=SaveGameOrchestrator::HashBytes(section.payload))
            return foundation::Result<void>::Failure(foundation::Error::Create("test.generation.invalid","invalid generation test participant"));
        return foundation::Result<void>::Success();
    }
    [[nodiscard]] foundation::Result<std::unique_ptr<IRestoreStage>> StageRestore(const SaveSection&,const RestoreContext&) override
    {
        return foundation::Result<std::unique_ptr<IRestoreStage>>::Success(std::make_unique<GenerationTestStage>());
    }
    void CommitRestore(IRestoreStage&) noexcept override {}
};

class MemoryGenerationStore final : public ISaveGenerationStore
{
  public:
    std::optional<std::vector<std::byte>> current;
    bool fail_commit=false;
    int loads=0;
    int commits=0;
    std::function<void(int)> before_load;

    [[nodiscard]] foundation::Result<std::optional<std::vector<std::byte>>> LoadCurrent() override
    { ++loads; if (before_load) before_load(loads); return foundation::Result<std::optional<std::vector<std::byte>>>::Success(current); }
    [[nodiscard]] foundation::Result<void> CommitCurrent(std::span<const std::byte> package) override
    {
        ++commits;
        if (fail_commit)
            return foundation::Result<void>::Failure(foundation::Error::Create("test.generation.store","injected generation store failure"));
        current=std::vector<std::byte>(package.begin(),package.end());
        return foundation::Result<void>::Success();
    }
};

bool Check(bool condition,const char* message)
{
    if (!condition) std::cerr << "G5.3.B " << message << '\n';
    return condition;
}

bool LeasePreparationFailureDoesNotStrandQuiescence()
{
    for (const auto point : {"lease.outer", "lease.nested", "lease.restore"})
    {
        int releases = 0;
        SaveGenerationBarrierCallbacks callbacks{};
        callbacks.acquire_capture_quiescence = [&]() {
            std::unique_ptr<ISaveBarrierLease> lease = std::make_unique<CountingQuiescenceLease>(releases);
            return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
        };
        callbacks.acquire_restore_quiescence = callbacks.acquire_capture_quiescence;
        callbacks.capture_preflight = []() { return foundation::Result<void>::Success(); };
        SaveGenerationBarrier barrier(std::move(callbacks));
        std::unique_ptr<ISaveBarrierLease> outer;
        if (point == std::string_view{"lease.nested"})
        {
            auto acquired = barrier.AcquireGenerationLease(SaveGenerationId::FromRaw(1));
            if (!acquired) return false;
            outer = std::move(acquired.Value());
        }
        integration::detail::SetSaveGenerationFaultPointForTesting(point);
        auto failed = point == std::string_view{"lease.restore"} ? barrier.AcquireRestoreLease()
                    : point == std::string_view{"lease.nested"} ? barrier.AcquireCaptureLease()
                    : barrier.AcquireGenerationLease(SaveGenerationId::FromRaw(1));
        integration::detail::ClearSaveGenerationFaultPointForTesting();
        if (!Check(!failed, "G5-BARRIER-001 logical lease allocation failure is controlled")) return false;
        outer.reset();
        if (!Check(!barrier.IsCaptureActive() && !barrier.ActiveGeneration().IsValid() && releases == 1,
                   "G5-BARRIER-001 allocation failure never strands quiescence or nested bookkeeping")) return false;
        auto retry = barrier.AcquireGenerationLease(SaveGenerationId::FromRaw(2));
        if (!Check(bool(retry), "G5-BARRIER-001 acquisition can retry after preparation failure")) return false;
        retry.Value().reset();
        if (!Check(releases == 2, "G5-BARRIER-001 retry releases exactly once")) return false;
    }
    return true;
}

class GateLease final : public ISaveBarrierLease
{
  public:
    explicit GateLease(bool& gate) : gate_(gate) { gate_ = true; }
    ~GateLease() override { gate_ = false; }
  private:
    bool& gate_;
};

bool SaveFailureMatrixPreservesPublishedGeneration()
{
    for (const auto fault : {"acquire", "preflight", "framework", "time", "persistence", "package.encode", "store", "exception"})
    {
        bool quiesced = false;
        bool inject = false;
        int mutation_attempts = 0;
        int prohibited_mutations = 0;
        SaveGenerationBarrierCallbacks callbacks{};
        callbacks.acquire_capture_quiescence = [&]() {
            if (inject && fault == std::string_view{"acquire"})
                return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Failure(foundation::Error::Create("test.acquire", "acquire failed"));
            std::unique_ptr<ISaveBarrierLease> lease = std::make_unique<GateLease>(quiesced);
            return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
        };
        callbacks.capture_preflight = [&]() {
            if (inject && fault == std::string_view{"preflight"})
                return foundation::Result<void>::Failure(foundation::Error::Create("test.preflight", "streaming/runtime materialization in flight"));
            return foundation::Result<void>::Success();
        };
        callbacks.acquire_restore_quiescence = callbacks.acquire_capture_quiescence;
        SaveGenerationBarrier barrier(std::move(callbacks));
        GenerationTestParticipant participant;
        participant.capture_probe = [&]() {
            // The composition mutation entry obeys the production lease's
            // injected quiescence gate, including adversarial capture reentry.
            ++mutation_attempts;
            if (!quiesced) ++prohibited_mutations;
        };
        SaveGameOrchestrator orchestrator;
        if (!orchestrator.RegisterParticipant(participant) || !orchestrator.FreezeRegistry()) return false;
        auto plane = [&](std::string_view name) {
            if (inject && fault == name)
                return foundation::Result<std::vector<std::byte>>::Failure(foundation::Error::Create("test.runtime", "runtime capture failed"));
            if (inject && fault == std::string_view{"exception"}) throw std::runtime_error("provider exception");
            return foundation::Result<std::vector<std::byte>>::Success({std::byte{1}});
        };
        CallbackRuntimeSaveGenerationPort runtime([&](SaveGenerationId) { return plane("time"); },
                                                  [&](SaveGenerationId) { return plane("persistence"); });
        MemoryGenerationStore store;
        SaveGenerationCoordinator coordinator(orchestrator, runtime, barrier, store);
        if (!coordinator.CaptureAndCommit({})) return false;
        const auto previous = store.current;
        const int commits_before = store.commits;
        inject = true;
        participant.fail_capture = fault == std::string_view{"framework"};
        store.fail_commit = fault == std::string_view{"store"};
        if (fault == std::string_view{"package.encode"}) integration::detail::SetSaveGenerationFaultPointForTesting(fault);
        auto failed = coordinator.CaptureAndCommit({});
        integration::detail::ClearSaveGenerationFaultPointForTesting();
        if (!Check(!failed && store.current == previous && !quiesced && !barrier.IsCaptureActive(),
                   "failure at every logical save phase preserves old package and releases quiescence")) return false;
        if (!Check(store.commits == commits_before + (store.fail_commit ? 1 : 0),
                   "failed preparation never reaches durable publication")) return false;
        inject = participant.fail_capture = store.fail_commit = false;
        auto retry = coordinator.CaptureAndCommit({});
        if (!Check(bool(retry) && retry.Value().Raw() == 2 && !quiesced,
                   "retry after each logical save failure uses next valid generation")) return false;
        if (!Check(mutation_attempts >= 2 && prohibited_mutations == 0,
                   "adversarial mutation attempt is excluded for complete capture lifetime")) return false;
    }
    return true;
}

bool DurablePendingGenerationRestoresOnlyUnfinishedDelivery()
{
    time::GameplayTimeService clock;
    const auto domain = GameplayDomainId::FromString("generation.pending");
    auto clock_id = clock.RegisterClock("generation.pending.clock", time::CalendarDefinition{});
    auto action_id = clock.RegisterAction("generation.pending.action", domain);
    if (!clock_id || !action_id) return false;
    clock.Freeze();
    int completed_side_effects = 0;
    int resumed_side_effects = 0;
    const auto observer = ScheduledTriggerHandlerId::FromString("generation.pending.accepted");
    const auto handler = ScheduledTriggerHandlerId::FromString("generation.pending.deferred");
    ScheduledTriggerDispatcher dispatcher(clock);
    if (!dispatcher.RegisterObserver(observer, 0, [&](const auto&, const auto&) {
            ++completed_side_effects;
            return foundation::Result<ScheduledTriggerDisposition>::Success(ScheduledTriggerDisposition::Ack);
        }) || !dispatcher.RegisterActionHandler(action_id.Value(), handler, [](const auto&, const auto&) {
            return foundation::Result<ScheduledTriggerDisposition>::Success(ScheduledTriggerDisposition::Retry);
        }) || !dispatcher.Freeze()) return false;
    GameplayObjectRef subject{domain, GameplayObjectId::FromString("subject")};
    if (!clock.Schedule(clock_id.Value(), GameplayTimePoint{1}, subject, action_id.Value()) ||
        !clock.AdvanceTo(clock_id.Value(), GameplayTimePoint{1}) ||
        !dispatcher.CollectDue(clock_id.Value(), {})) return false;
    auto delivered = dispatcher.DispatchPending();
    if (!Check(delivered.pending == 1 && completed_side_effects == 1, "accepted leg is complete while deferred leg is durable pending")) return false;
    ScheduledTriggerDispatcherSaveParticipant participant(dispatcher);
    SaveGameOrchestrator orchestrator;
    if (!orchestrator.RegisterParticipant(participant) || !orchestrator.FreezeRegistry()) return false;
    bool quiesced = false;
    SaveGenerationBarrierCallbacks callbacks{};
    callbacks.acquire_capture_quiescence = [&]() {
        std::unique_ptr<ISaveBarrierLease> lease = std::make_unique<GateLease>(quiesced);
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
    };
    callbacks.acquire_restore_quiescence = callbacks.acquire_capture_quiescence;
    callbacks.capture_preflight = []() { return foundation::Result<void>::Success(); };
    SaveGenerationBarrier barrier(std::move(callbacks));
    CallbackRuntimeSaveGenerationPort runtime([](SaveGenerationId) { return foundation::Result<std::vector<std::byte>>::Success({std::byte{1}}); },
                                              [](SaveGenerationId) { return foundation::Result<std::vector<std::byte>>::Success({std::byte{2}}); });
    MemoryGenerationStore store;
    SaveGenerationCoordinator coordinator(orchestrator, runtime, barrier, store);
    if (!coordinator.CaptureAndCommit({})) return false;
    auto package = DecodeSaveGenerationPackage(*store.current);
    if (!package) return false;
    auto image = DecodeSaveGameImage(package.Value().framework_savegame);
    if (!image) return false;
    ScheduledTriggerDispatcher restored(clock);
    if (!restored.RegisterObserver(observer, 0, [&](const auto&, const auto&) {
            ++completed_side_effects;
            return foundation::Result<ScheduledTriggerDisposition>::Success(ScheduledTriggerDisposition::Ack);
        }) || !restored.RegisterActionHandler(action_id.Value(), handler, [&](const auto&, const auto&) {
            ++resumed_side_effects;
            return foundation::Result<ScheduledTriggerDisposition>::Success(ScheduledTriggerDisposition::Ack);
        }) || !restored.Freeze()) return false;
    ScheduledTriggerDispatcherSaveParticipant restored_participant(restored);
    SaveGameOrchestrator restore;
    if (!restore.RegisterParticipant(restored_participant) || !restore.FreezeRegistry() ||
        !restore.SetBarrier(barrier) || !restore.Restore(std::move(image.Value()), {})) return false;
    auto resumed = restored.DispatchPending();
    auto repeated = restored.DispatchPending();
    return Check(resumed.pending == 0 && repeated.pending == 0 && completed_side_effects == 1 && resumed_side_effects == 1,
                 "generation save preserves accepted/deferred checkpoint and retries each side effect exactly once");
}

bool NestedLeaseLifetimeAndWrongThreadAreControlled()
{
    int releases = 0;
    SaveGenerationBarrierCallbacks callbacks{};
    callbacks.acquire_capture_quiescence = [&]() {
        std::unique_ptr<ISaveBarrierLease> lease = std::make_unique<CountingQuiescenceLease>(releases);
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
    };
    callbacks.acquire_restore_quiescence = callbacks.acquire_capture_quiescence;
    callbacks.capture_preflight = []() { return foundation::Result<void>::Success(); };
    SaveGenerationBarrier barrier(std::move(callbacks));
    auto outer = barrier.AcquireGenerationLease(SaveGenerationId::FromRaw(7));
    if (!outer) return false;
    bool wrong_thread_rejected = false;
    std::thread worker([&]() { wrong_thread_rejected = !barrier.AcquireCaptureLease(); });
    worker.join();
    if (!Check(wrong_thread_rejected && !barrier.AcquireRestoreLease() && !barrier.AcquireGenerationLease(SaveGenerationId::FromRaw(8)),
               "wrong-thread nested capture and overlapping capture/restore are rejected")) return false;
    auto nested = barrier.AcquireCaptureLease();
    if (!nested) return false;
    outer.Value().reset();
    if (!Check(barrier.IsCaptureActive() && releases == 0 && !barrier.AcquireCaptureLease(),
               "nested capture retains quiescence after outer release but cannot acquire successors")) return false;
    nested.Value().reset();
    return Check(!barrier.IsCaptureActive() && releases == 1 && bool(barrier.AcquireRestoreLease()),
                 "last nested release clears generation and permits restore");
}

void RepairPackageHash(std::vector<std::byte>& bytes)
{
    const auto hash=SaveGameOrchestrator::HashBytes(std::span<const std::byte>(bytes).first(bytes.size()-8));
    for (std::size_t index=0;index<8;++index)
        bytes[bytes.size()-8+index]=std::byte((hash>>(index*8))&0xffu);
}

bool RunSaveGenerationTests()
{
    int quiescence_acquires=0;
    int quiescence_releases=0;
    int preflights=0;
    SaveGenerationBarrierCallbacks callbacks{};
    callbacks.acquire_capture_quiescence=[&]() {
        ++quiescence_acquires;
        std::unique_ptr<ISaveBarrierLease> lease=std::make_unique<CountingQuiescenceLease>(quiescence_releases);
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
    };
    callbacks.capture_preflight=[&]() { ++preflights; return foundation::Result<void>::Success(); };
    callbacks.acquire_restore_quiescence=[]() {
        std::unique_ptr<ISaveBarrierLease> lease=std::make_unique<TestSaveBarrierLease>();
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
    };
    SaveGenerationBarrier barrier(std::move(callbacks));

    GenerationTestParticipant participant;
    SaveGameOrchestrator orchestrator;
    if (!orchestrator.RegisterParticipant(participant) || !orchestrator.FreezeRegistry())
        return Check(false,"generation participant bootstrap");

    SaveGenerationId time_generation{};
    SaveGenerationId persistence_generation{};
    int time_captures=0;
    int persistence_captures=0;
    CallbackRuntimeSaveGenerationPort runtime_port(
        [&](SaveGenerationId generation) {
            ++time_captures; time_generation=generation;
            return foundation::Result<std::vector<std::byte>>::Success({std::byte{0x54},std::byte{0x49},std::byte{0x4d}});
        },
        [&](SaveGenerationId generation) {
            ++persistence_captures; persistence_generation=generation;
            return foundation::Result<std::vector<std::byte>>::Success({std::byte{0x50},std::byte{0x45},std::byte{0x52}});
        });
    MemoryGenerationStore store;
    SaveGenerationCoordinator coordinator(orchestrator,runtime_port,barrier,store);

    SaveContext context{}; context.now=GameplayTimePoint{77}; context.content_version=1; context.engine_version=2; context.framework_version=3;
    auto first=coordinator.CaptureAndCommit(context);
    if (!Check(bool(first) && first.Value().Raw()==1,"first generation is 1") ||
        !Check(store.current.has_value(),"first generation published") ||
        !Check(quiescence_acquires==1 && quiescence_releases==1 && preflights==1,"outer lease owns one safe point and nested SaveGame capture reuses it") ||
        !Check(time_generation==first.Value() && persistence_generation==first.Value(),"Runtime planes receive same generation") ||
        !Check(time_captures==1 && persistence_captures==1,"both Runtime planes captured exactly once") ||
        !Check(!barrier.IsCaptureActive() && !barrier.ActiveGeneration().IsValid(),"barrier released after successful publish"))
        return false;

    auto decoded=DecodeSaveGenerationPackage(*store.current);
    if (!Check(bool(decoded) && decoded.Value().generation==first.Value(),"outer package generation roundtrip") ||
        !Check(decoded.Value().runtime_time==std::vector<std::byte>({std::byte{0x54},std::byte{0x49},std::byte{0x4d}}),"Runtime Time plane present") ||
        !Check(decoded.Value().runtime_persistence==std::vector<std::byte>({std::byte{0x50},std::byte{0x45},std::byte{0x52}}),"Runtime Persistence plane present"))
        return false;
    auto framework=DecodeSaveGameImage(decoded.Value().framework_savegame);
    if (!Check(bool(framework) && framework.Value().sections.size()==1 && framework.Value().sections[0].participant==participant.Id(),"Framework SaveGame plane present and valid"))
        return false;

    auto direct_capture=orchestrator.Capture(context);
    if (!Check(!direct_capture,"production generation barrier rejects Framework capture without outer generation"))
        return false;

    auto second=coordinator.CaptureAndCommit(context);
    if (!Check(bool(second) && second.Value().Raw()==2,"generation increments from current authoritative package") ||
        !Check(quiescence_acquires==2 && quiescence_releases==2 && preflights==2,"second generation uses one new safe point"))
        return false;

    const auto previous=*store.current;
    store.fail_commit=true;
    auto failed_commit=coordinator.CaptureAndCommit(context);
    store.fail_commit=false;
    if (!Check(!failed_commit && *store.current==previous,"failed outer commit preserves previous authoritative generation") ||
        !Check(!barrier.IsCaptureActive() && quiescence_acquires==3 && quiescence_releases==3,"failed publish releases outer barrier"))
        return false;

    int blocked_release=0;
    SaveGenerationBarrierCallbacks blocked_callbacks{};
    blocked_callbacks.acquire_capture_quiescence=[&]() {
        std::unique_ptr<ISaveBarrierLease> lease=std::make_unique<CountingQuiescenceLease>(blocked_release);
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
    };
    blocked_callbacks.capture_preflight=[]() {
        return foundation::Result<void>::Failure(foundation::Error::Create("test.generation.streaming_blocked","accepted transient Streaming work"));
    };
    blocked_callbacks.acquire_restore_quiescence=[]() {
        std::unique_ptr<ISaveBarrierLease> lease=std::make_unique<TestSaveBarrierLease>();
        return foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(std::move(lease));
    };
    SaveGenerationBarrier blocked_barrier(std::move(blocked_callbacks));
    SaveGenerationCoordinator blocked_coordinator(orchestrator,runtime_port,blocked_barrier,store);
    const auto before_block=*store.current;
    const int runtime_before_block=time_captures;
    auto blocked=blocked_coordinator.CaptureAndCommit(context);
    if (!Check(!blocked && *store.current==before_block,"save blocker prevents publication") ||
        !Check(blocked_release==1 && time_captures==runtime_before_block,"preflight runs under quiescence and blocks before plane capture"))
        return false;

    CallbackRuntimeSaveGenerationPort failing_runtime(
        [](SaveGenerationId) { return foundation::Result<std::vector<std::byte>>::Failure(foundation::Error::Create("test.generation.runtime","injected Runtime Time capture failure")); },
        [](SaveGenerationId) { return foundation::Result<std::vector<std::byte>>::Success({std::byte{1}}); });
    SaveGenerationCoordinator failing_runtime_coordinator(orchestrator,failing_runtime,barrier,store);
    const auto before_runtime_failure=*store.current;
    auto runtime_failed=failing_runtime_coordinator.CaptureAndCommit(context);
    if (!Check(!runtime_failed && *store.current==before_runtime_failure,"Runtime plane failure preserves previous generation") ||
        !Check(!barrier.IsCaptureActive(),"Runtime plane failure releases barrier"))
        return false;

    auto mismatch=*store.current;
    mismatch[28]^=std::byte{1};
    RepairPackageHash(mismatch);
    if (!Check(!DecodeSaveGenerationPackage(mismatch),"mismatched plane generation rejected even with repaired outer checksum"))
        return false;

    auto maximum_package=DecodeSaveGenerationPackage(*store.current);
    if (!maximum_package) return Check(false,"maximum generation fixture decode");
    maximum_package.Value().generation=SaveGenerationId::FromRaw(std::numeric_limits<std::uint64_t>::max());
    auto maximum_bytes=EncodeSaveGenerationPackage(maximum_package.Value());
    if (!maximum_bytes) return Check(false,"maximum generation fixture encode");
    store.current=std::move(maximum_bytes.Value());
    const int acquire_before_overflow=quiescence_acquires;
    auto overflow=coordinator.CaptureAndCommit(context);
    if (!Check(!overflow && quiescence_acquires==acquire_before_overflow,"generation overflow fails before safe-point acquisition"))
        return false;

    auto corrupt=*store.current; corrupt[0]^=std::byte{1}; store.current=corrupt;
    auto corrupt_lineage=coordinator.CaptureAndCommit(context);
    if (!Check(!corrupt_lineage && quiescence_acquires==acquire_before_overflow,"malformed existing lineage never resets silently"))
        return false;

    // G5-SAVE-001: publication between initial lineage read and lease acquisition
    // cannot let a second coordinator silently reuse the same generation.
    store.current = previous;
    const auto stable_lineage = store.current;
    const int next_load = store.loads + 2;
    store.before_load = [&](int load) {
        if (load == next_load)
        {
            auto successor = DecodeSaveGenerationPackage(*store.current);
            if (!successor) return;
            successor.Value().generation = SaveGenerationId::FromRaw(3);
            auto bytes = EncodeSaveGenerationPackage(successor.Value());
            if (bytes) store.current = std::move(bytes.Value());
        }
    };
    const int before_race_commit = store.commits;
    auto raced = coordinator.CaptureAndCommit(context);
    store.before_load = {};
    if (!Check(!raced && store.commits == before_race_commit && !barrier.IsCaptureActive() && store.current != stable_lineage,
               "G5-SAVE-001 stale lineage rejects capture before overwriting another published generation")) return false;
    auto race_retry = coordinator.CaptureAndCommit(context);
    if (!Check(bool(race_retry) && race_retry.Value().Raw() == 4,
               "G5-SAVE-001 retry derives next generation from latest authoritative lineage")) return false;
    return true;
}
} // namespace

int main()
{
    return LeasePreparationFailureDoesNotStrandQuiescence() && SaveFailureMatrixPreservesPublishedGeneration() &&
           DurablePendingGenerationRestoresOnlyUnfinishedDelivery() && NestedLeaseLifetimeAndWrongThreadAreControlled() &&
           RunSaveGenerationTests() ? 0 : 1;
}

