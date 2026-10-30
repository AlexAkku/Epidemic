#pragma once

#include "Epidemic/Runtime/Graph/graph_vm.h"
#include "Epidemic/Runtime/Simulation/simulation_runtime.h"
#include "Epidemic/Runtime/Time/time_runtime.h"

namespace epidemic::runtime {
struct GraphScheduleOptions {
    graph::ExecutionBudget budget{};

    // Zero: resume at the next host Tick. Positive: fixed game-tick grid,
    // anchored at submission; overdue steps catch up one per host Tick.
    GameDuration fixed_step{};

};

class GraphScheduler;
class GraphTask;

// A transient lifetime scope, not an entity identity or event registry. Invalidate
// on the native-effect thread; Prepare removes the host's queued work/timers.
class GraphActivationOwner final {
  public:
    GraphActivationOwner() = default;
    ~GraphActivationOwner();
    GraphActivationOwner(const GraphActivationOwner &) = delete;
    GraphActivationOwner &operator=(const GraphActivationOwner &) = delete;
    void Invalidate() noexcept;

    [[nodiscard]] bool IsAlive() const noexcept {
        return state_->alive;
    }

  private:
    struct State {
        bool alive = true;
        std::vector<std::weak_ptr<GraphTask>> tasks;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();
    friend class GraphScheduler;
};

class GraphTask final : public simulation::ISimulationJob {
  public:
    GraphTask(const GraphTask &) = delete;
    GraphTask &operator=(const GraphTask &) = delete;
    GraphTask(GraphTask &&) = delete;
    GraphTask &operator=(GraphTask &&) = delete;

    [[nodiscard]] simulation::SimulationJobHandle Handle() const noexcept {
        return handle_;
    }

    [[nodiscard]] const std::optional<graph::ExecutionReport> &LastReport() const noexcept {
        return report_;
    }

    [[nodiscard]] std::optional<GameTimePoint> NextWake() const noexcept {
        return terminal_ ? std::nullopt : std::optional{due_};

    }

    [[nodiscard]] foundation::Result<std::int64_t> ReadValue(graph::ElementId port) const;

  private:
    void RequestOwnerCancellation() noexcept;
    [[nodiscard]] foundation::Result<simulation::SimulationStepResult>
    ExecuteStep(const RuntimeBudget &budget) override;
    [[nodiscard]] foundation::Result<void> Cancel() override;
    GraphTask(std::unique_ptr<graph::GraphInstance> instance, std::shared_ptr<IGameClock> clock,
              std::shared_ptr<const int> owner, const GraphScheduleOptions &options,
              GameTimePoint now);
    [[nodiscard]] foundation::Result<simulation::SimulationStepResult>
    Fail(const foundation::Error &error, std::size_t instructions = 0,
         std::size_t native_calls = 0);
    std::unique_ptr<graph::GraphInstance> instance_;
    std::shared_ptr<IGameClock> clock_;
    std::shared_ptr<const int> owner_;
    GraphScheduleOptions options_{};

    simulation::SimulationJobHandle handle_{};

    GameTimePoint due_{};

    std::optional<GameTimePoint> observed_;
    std::optional<graph::ExecutionReport> report_;
    bool needs_arm_ = true;
    bool running_ = false;
    bool terminal_ = false;
    bool owner_cancel_ = false;
    friend class GraphScheduler;
    friend class GraphActivationOwner;
};

// Composition only: never calls Simulation::Tick or advances a clock. The host
// calls Prepare before its existing Simulation Tick, on the native-effect thread.
// Simulation must use the same Time clock/tick domain (Runtime Support already
// supplies that adapter). All calls and observations are externally synchronized.
class GraphScheduler final {
  public:
    GraphScheduler(std::shared_ptr<IGameClock> clock, simulation::SimulationServices simulation,
                   SimulationZoneId zone, std::size_t max_active = 1024);
    GraphScheduler(const GraphScheduler &) = delete;
    GraphScheduler &operator=(const GraphScheduler &) = delete;
    [[nodiscard]] foundation::Result<std::shared_ptr<GraphTask>>
    Submit(std::unique_ptr<graph::GraphInstance> instance,
           const GraphScheduleOptions &options = {});
    [[nodiscard]] foundation::Result<std::shared_ptr<GraphTask>>
    SubmitOwned(std::unique_ptr<graph::GraphInstance> instance, GraphActivationOwner &owner,
                const GraphScheduleOptions &options = {});
    // Scheduling failure retains the pending request. Retry does not replay VM
    // instructions or previously accepted schedules. Does not run graph code.
    [[nodiscard]] foundation::Result<std::size_t> Prepare();
    [[nodiscard]] foundation::Result<void> Cancel(GraphTask &task);
    [[nodiscard]] foundation::Result<GameTimePoint> Now() const;

  private:
    std::shared_ptr<IGameClock> clock_;
    simulation::SimulationServices simulation_;
    SimulationZoneId zone_{};

    std::shared_ptr<const int> owner_ = std::make_shared<const int>(0);
    std::vector<std::shared_ptr<GraphTask>> tasks_;
    std::optional<GameTimePoint> observed_;
    std::size_t max_active_;
};

} // namespace epidemic::runtime
