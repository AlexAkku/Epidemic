#include "Epidemic/GameFramework/Integration/core_adapters.h"

#include <exception>
#include <array>
#include <thread>

namespace epidemic::gameplay::integration
{
namespace
{
foundation::Error RestoreError(std::string_view code, std::string_view text)
{
    return foundation::Error::Create(code, text);
}
struct OperationGuard
{
    bool& active;
    explicit OperationGuard(bool& flag) noexcept : active(flag) { active = true; }
    ~OperationGuard() noexcept { active = false; }
};
}

struct WholeEngineRestoreCoordinator::ThreadIdentity
{
    std::thread::id id = std::this_thread::get_id();
};

WholeEngineRestoreCoordinator::WholeEngineRestoreCoordinator(
    savegame::SaveGameOrchestrator& framework, IRuntimeGenerationRestorePort& runtime,
    SaveGenerationBarrier& barrier, ISaveGenerationStore& store, WholeRestoreCallbacks callbacks,
    SaveGenerationLimits limits, savegame::SaveWireLimits framework_limits)
    : framework_(framework), runtime_(runtime), barrier_(barrier), store_(store),
      callbacks_(std::move(callbacks)), limits_(limits), framework_limits_(framework_limits),
      owner_thread_(std::make_unique<ThreadIdentity>())
{
}

WholeEngineRestoreCoordinator::~WholeEngineRestoreCoordinator() = default;

bool WholeEngineRestoreCoordinator::OnOwnerThread() const noexcept
{
    return owner_thread_->id == std::this_thread::get_id();
}

foundation::Result<SaveGenerationId> WholeEngineRestoreCoordinator::LoadAndRestore(
    const savegame::RestoreContext& context)
{
    if (!OnOwnerThread() || operation_active_ || phase_ != WholeRestorePhase::AwaitLoad)
        return foundation::Result<SaveGenerationId>::Failure(
            RestoreError("gameplay.restore.invalid_state", "restore requires its owner thread and a fresh cycle"));
    if (!callbacks_.rebuild_transient || !callbacks_.reconcile_external || !callbacks_.materialize_runtime)
        return foundation::Result<SaveGenerationId>::Failure(
            RestoreError("gameplay.restore.unconfigured", "all postcommit reconciliation steps must be configured"));
    OperationGuard operation(operation_active_);
    try
    {
        auto bytes = store_.LoadCurrent();
        if (!bytes) return foundation::Result<SaveGenerationId>::Failure(bytes.GetError());
        if (!bytes.Value()) return foundation::Result<SaveGenerationId>::Failure(
            RestoreError("gameplay.restore.missing_package", "no current save generation exists"));
        auto package = DecodeSaveGenerationPackage(*bytes.Value(), limits_);
        if (!package) return foundation::Result<SaveGenerationId>::Failure(package.GetError());
        auto image = savegame::DecodeSaveGameImage(package.Value().framework_savegame, framework_limits_);
        if (!image) return foundation::Result<SaveGenerationId>::Failure(image.GetError());
        auto lease = barrier_.AcquireRestoreLease();
        if (!lease) return foundation::Result<SaveGenerationId>::Failure(lease.GetError());
        phase_ = WholeRestorePhase::Precommit;
        auto runtime_stage = runtime_.Prepare(package.Value());
        if (!runtime_stage || !runtime_stage.Value())
        {
            phase_ = WholeRestorePhase::AwaitLoad;
            return foundation::Result<SaveGenerationId>::Failure(runtime_stage ?
                RestoreError("gameplay.restore.empty_stage", "Runtime restore preparation returned an empty stage") : runtime_stage.GetError());
        }
        (void)framework_.SetBarrier(barrier_);
        // Restore performs validation, migration and all participant staging
        // before its no-throw commit loop. A failure here leaves both planes live.
        auto restored = framework_.Restore(std::move(image.Value()), context);
        if (!restored)
        {
            phase_ = WholeRestorePhase::AwaitLoad;
            return foundation::Result<SaveGenerationId>::Failure(restored.GetError());
        }
        runtime_stage.Value()->Commit();
        restored_generation_ = package.Value().generation;
        phase_ = WholeRestorePhase::CommittedSemantic;
        lease_ = std::move(lease.Value());
        auto reconciled = ContinueReconciliation();
        if (!reconciled) return foundation::Result<SaveGenerationId>::Failure(reconciled.GetError());
        return foundation::Result<SaveGenerationId>::Success(restored_generation_);
    }
    catch (const std::exception& error)
    {
        phase_ = restored_generation_.IsValid() ? WholeRestorePhase::ReconciliationRequired : WholeRestorePhase::AwaitLoad;
        return foundation::Result<SaveGenerationId>::Failure(RestoreError("gameplay.restore.exception", error.what()));
    }
    catch (...)
    {
        phase_ = restored_generation_.IsValid() ? WholeRestorePhase::ReconciliationRequired : WholeRestorePhase::AwaitLoad;
        return foundation::Result<SaveGenerationId>::Failure(RestoreError("gameplay.restore.exception", "restore provider threw"));
    }
}

foundation::Result<void> WholeEngineRestoreCoordinator::ContinueReconciliation()
{
    const std::array phases{WholeRestorePhase::RebuildTransient, WholeRestorePhase::ReconcileExternal,
                            WholeRestorePhase::MaterializeRuntime};
    const std::array callbacks{&callbacks_.rebuild_transient, &callbacks_.reconcile_external,
                               &callbacks_.materialize_runtime};
    try
    {
        while (completed_steps_ < phases.size())
        {
            phase_ = phases[completed_steps_];
            auto result = (*callbacks[completed_steps_])();
            if (!result)
            {
                phase_ = WholeRestorePhase::ReconciliationRequired;
                return result;
            }
            ++completed_steps_;
        }
        lease_.reset();
        phase_ = WholeRestorePhase::ReadyToResume;
        return foundation::Result<void>::Success();
    }
    catch (const std::exception& error)
    {
        phase_ = WholeRestorePhase::ReconciliationRequired;
        return foundation::Result<void>::Failure(RestoreError("gameplay.restore.reconciliation_exception", error.what()));
    }
    catch (...)
    {
        phase_ = WholeRestorePhase::ReconciliationRequired;
        return foundation::Result<void>::Failure(RestoreError("gameplay.restore.reconciliation_exception", "reconciliation provider threw"));
    }
}

foundation::Result<void> WholeEngineRestoreCoordinator::RetryReconciliation()
{
    if (!OnOwnerThread() || operation_active_ || phase_ != WholeRestorePhase::ReconciliationRequired || !lease_ || !restored_generation_.IsValid())
        return foundation::Result<void>::Failure(
            RestoreError("gameplay.restore.invalid_retry", "retry requires a committed semantic restore on its owner thread"));
    OperationGuard operation(operation_active_);
    return ContinueReconciliation();
}
} // namespace epidemic::gameplay::integration
