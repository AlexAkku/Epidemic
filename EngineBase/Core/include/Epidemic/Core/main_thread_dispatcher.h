#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
#include <shadow_probe.h>
#endif

namespace epidemic::core {
// This file defines the main-thread task queue used by Application.
// The dispatcher lets background code request work back on the owning thread without knowing
// anything about the frame loop implementation.

class IMainThreadDispatcher {
  public:
    virtual ~IMainThreadDispatcher() = default;

    // Queues a task for later execution on the owning main thread.
    virtual void Post(std::function<void()> task, std::string debug_name = {}) = 0;

    // Executes all queued tasks on the main thread and returns the number processed.
    [[nodiscard]] virtual std::size_t Drain() = 0;

    // Returns whether the current thread is the dispatcher's owning main thread.
    [[nodiscard]] virtual bool IsMainThread() const noexcept = 0;
};

class MainThreadDispatcher final : public IMainThreadDispatcher {
  public:
    // Captures the constructing thread as the owning main thread.
    MainThreadDispatcher();

    // Queues one task for later main-thread execution.
    void Post(std::function<void()> task, std::string debug_name = {}) override;

    // Executes queued tasks in FIFO order on the owning main thread.
    [[nodiscard]] std::size_t Drain() override;

    // Returns whether the calling thread matches the owning thread.
    [[nodiscard]] bool IsMainThread() const noexcept override;

  private:
    struct PendingTask {
        std::function<void()> task;
        std::string debug_name;
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
        auditor::shadow::Context auditor_context;
        auditor::shadow::Identity auditor_correlation;
#endif
    };

    std::thread::id owner_thread_id_;
    std::mutex mutex_;
    std::queue<PendingTask> tasks_;
};

} // namespace epidemic::core
