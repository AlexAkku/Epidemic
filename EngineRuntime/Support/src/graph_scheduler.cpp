#include "Epidemic/Runtime/Support/graph_scheduler.h"

#include <algorithm>
#include <limits>

namespace epidemic::runtime {
namespace {
foundation::Error Error(std::string_view code) {
    return foundation::Error::Create(code, "graph scheduling rejected");
}

foundation::Result<GameTimePoint> ReadClock(const std::shared_ptr<IGameClock> &clock) {
    if (!clock)
        return foundation::Result<GameTimePoint>::Failure(Error("graph.scheduler.missing_clock"));
    try {
        const auto now = clock->Now();
        if (now.ticks < 0)
            return foundation::Result<GameTimePoint>::Failure(
                Error("graph.scheduler.invalid_time"));
        return foundation::Result<GameTimePoint>::Success(now);
    } catch (...) {
        return foundation::Result<GameTimePoint>::Failure(Error("graph.scheduler.clock_exception"));
    }

}

simulation::SimulationJobState State(graph::ExecutionStatus status) {
    switch (status) {
    case graph::ExecutionStatus::Completed:
        return simulation::SimulationJobState::Completed;
    case graph::ExecutionStatus::Cancelled:
        return simulation::SimulationJobState::Cancelled;
    case graph::ExecutionStatus::Failed:
        return simulation::SimulationJobState::Failed;
    default:
        return simulation::SimulationJobState::PartiallyComplete;
    }

}

struct RunningGuard {
    bool &running;

    ~RunningGuard() {
        running = false;
    }

};

std::optional<GameTimePoint> NextDue(GameTimePoint base, GameDuration delay, GameDuration step) {
    if (step.ticks == 0)
        return CheckedAdd(base, delay);
    const auto whole = delay.ticks / step.ticks;
    const auto rounded = whole + (delay.ticks % step.ticks != 0 ? 1 : 0);
    const auto count = std::max<std::int64_t>(1, rounded);
    if (count > std::numeric_limits<std::int64_t>::max() / step.ticks)
        return std::nullopt;
    return CheckedAdd(base, GameDuration{count * step.ticks});
}

} // namespace

GraphActivationOwner::~GraphActivationOwner() {
    Invalidate();
}

void GraphActivationOwner::Invalidate() noexcept {
    if (!state_->alive)
        return;
    state_->alive = false;
    for (const auto &entry : state_->tasks)
        if (const auto task = entry.lock())
            task->RequestOwnerCancellation();
    state_->tasks.clear();
}

void GraphTask::RequestOwnerCancellation() noexcept {
    if (!terminal_) {
        owner_cancel_ = true;
        instance_->Cancel();
    }

}

GraphTask::GraphTask(std::unique_ptr<graph::GraphInstance> instance,
                     std::shared_ptr<IGameClock> clock, std::shared_ptr<const int> owner,
                     const GraphScheduleOptions &options, GameTimePoint now)
    : instance_(std::move(instance)), clock_(std::move(clock)), owner_(std::move(owner)),
      options_(options), due_(now) {
}

foundation::Result<std::int64_t> GraphTask::ReadValue(graph::ElementId port) const {
    return instance_->ReadValue(port);
}

foundation::Result<simulation::SimulationStepResult> GraphTask::Fail(const foundation::Error &error,
                                                                     std::size_t instructions,
                                                                     std::size_t native_calls) {
    graph::ExecutionDiagnostic diagnostic;
    if (report_ && report_->diagnostic)
        diagnostic = *report_->diagnostic;
    else {
        diagnostic.asset = instance_->Program().Asset();
        diagnostic.instruction = instance_->InstructionPointer();
        const auto code = instance_->Program().Instructions();
        if (diagnostic.instruction < code.size())
            diagnostic.node = code[diagnostic.instruction].node;
    }

    diagnostic.error = error;
    report_ = graph::ExecutionReport{graph::ExecutionStatus::Failed, instructions, native_calls,
                                     std::move(diagnostic)};

    terminal_ = true;
    needs_arm_ = false;
    return foundation::Result<simulation::SimulationStepResult>::Success(
        {simulation::SimulationJobState::Failed, static_cast<std::uint32_t>(instructions), {}});
}

foundation::Result<simulation::SimulationStepResult>
GraphTask::ExecuteStep(const RuntimeBudget &budget) {
    using Result = foundation::Result<simulation::SimulationStepResult>;
    if (running_)
        return Result::Failure(Error("graph.scheduler.reentrant"));
    if (terminal_) {
        if (!report_)
            return Result::Failure(Error("graph.scheduler.invalid_state"));
        return Result::Success({State(report_->status), 0, {}});
    }

    if (needs_arm_)
        return Fail(Error("graph.scheduler.prepare_required"));
    running_ = true;
    RunningGuard guard{running_};

    // A sibling callback may invalidate our owner after this host Tick already
    // dequeued us. Observe cancellation before acknowledging an earlier wait.
    if (owner_cancel_) {
        report_ = instance_->Run({0, 0});
        terminal_ = true;
        needs_arm_ = false;
        return Result::Success({State(report_->status), 0, {}});
    }

    const auto now = ReadClock(clock_);
    if (!now)
        return Fail(now.GetError());
    if (observed_ && now.Value() < *observed_)
        return Fail(Error("graph.scheduler.clock_rewound"));
    observed_ = now.Value();
    if (now.Value() < due_)
        return Fail(Error("graph.scheduler.clock_mismatch"));
    needs_arm_ = false;
    if (report_ && report_->status == graph::ExecutionStatus::Waiting) {
        const auto resumed = instance_->ResumeWait();
        if (!resumed)
            return Fail(resumed.GetError());
    }

    auto granted = options_.budget;
    const auto limit =
        budget.max_items == 0 ? std::numeric_limits<std::uint32_t>::max() : budget.max_items;
    granted.instructions = std::min<std::size_t>(granted.instructions, limit);
    report_ = instance_->Run(granted);
    const auto state = State(report_->status);
    terminal_ = state != simulation::SimulationJobState::PartiallyComplete;
    if (!terminal_) {
        const auto delay = report_->wait.value_or(GameDuration{});
        const auto due = NextDue(options_.fixed_step.ticks == 0 ? now.Value() : due_, delay,
                                 options_.fixed_step);
        if (!due)
            return Fail(Error("graph.scheduler.time_overflow"), report_->instructions,
                        report_->native_calls);
        due_ = *due;
        needs_arm_ = true;
    }

    return Result::Success({state, static_cast<std::uint32_t>(report_->instructions), {}});
}

foundation::Result<void> GraphTask::Cancel() {
    if (!terminal_) {
        instance_->Cancel();
        if (!running_) {
            report_ = instance_->Run({0, 0});
            terminal_ = true;
            needs_arm_ = false;
        }

    }

    return foundation::Result<void>::Success();
}

GraphScheduler::GraphScheduler(std::shared_ptr<IGameClock> clock,
                               simulation::SimulationServices simulation, SimulationZoneId zone,
                               std::size_t max_active)
    : clock_(std::move(clock)), simulation_(std::move(simulation)), zone_(zone),
      max_active_(max_active) {
}

foundation::Result<GameTimePoint> GraphScheduler::Now() const {
    return ReadClock(clock_);
}

foundation::Result<std::shared_ptr<GraphTask>>
GraphScheduler::Submit(std::unique_ptr<graph::GraphInstance> instance,
                       const GraphScheduleOptions &options) {
    using Result = foundation::Result<std::shared_ptr<GraphTask>>;
    if (std::any_of(tasks_.begin(), tasks_.end(), [](const auto &task) { return task->running_; }))
        return Result::Failure(Error("graph.scheduler.reentrant"));
    if (!instance || instance->HasStarted() || !simulation_.scheduler ||
        !simulation_.scheduled_tasks || !zone_.IsValid() || options.fixed_step.ticks < 0 ||
        options.budget.instructions == 0 || options.budget.native_calls == 0)
        return Result::Failure(Error("graph.scheduler.invalid_options"));
    const auto now = Now();
    if (!now)
        return Result::Failure(now.GetError());
    if (observed_ && now.Value() < *observed_)
        return Result::Failure(Error("graph.scheduler.clock_rewound"));
    observed_ = now.Value();
    std::erase_if(tasks_, [](const auto &task) { return task->terminal_ && !task->owner_cancel_; });
    if (tasks_.size() >= max_active_)
        return Result::Failure(Error("graph.scheduler.capacity"));
    tasks_.reserve(tasks_.size() + 1); // Before publishing a Simulation job.
    auto task = std::shared_ptr<GraphTask>(
        new GraphTask(std::move(instance), clock_, owner_, options, now.Value()));
    simulation::SimulationJobDesc metadata;
    metadata.zone = zone_;
    metadata.lane = simulation::SimulationLane::MainThread;
    const auto submitted = simulation_.scheduler->SubmitJob(task, metadata);
    if (!submitted)
        return Result::Failure(submitted.GetError());
    task->handle_ = submitted.Value();
    tasks_.push_back(task);
    return Result::Success(std::move(task));
}

foundation::Result<std::shared_ptr<GraphTask>>
GraphScheduler::SubmitOwned(std::unique_ptr<graph::GraphInstance> instance,
                            GraphActivationOwner &owner, const GraphScheduleOptions &options) {
    using Result = foundation::Result<std::shared_ptr<GraphTask>>;
    const auto state = owner.state_;
    if (!state->alive)
        return Result::Failure(Error("graph.scheduler.owner_expired"));
    std::erase_if(state->tasks, [](const auto &entry) {
        const auto task = entry.lock();
        return !task || task->terminal_;
    });
    state->tasks.reserve(state->tasks.size() + 1); // Before publishing a job.
    auto submitted = Submit(std::move(instance), options);
    if (!submitted)
        return submitted;
    state->tasks.emplace_back(submitted.Value());
    if (!state->alive)
        submitted.Value()->RequestOwnerCancellation();
    return submitted;
}

foundation::Result<std::size_t> GraphScheduler::Prepare() {
    using Result = foundation::Result<std::size_t>;
    if (std::any_of(tasks_.begin(), tasks_.end(), [](const auto &task) { return task->running_; }))
        return Result::Failure(Error("graph.scheduler.reentrant"));
    const auto now = Now();
    if (!now)
        return Result::Failure(now.GetError());
    if (observed_ && now.Value() < *observed_)
        return Result::Failure(Error("graph.scheduler.clock_rewound"));
    observed_ = now.Value();
    std::size_t armed = 0;
    for (const auto &task : tasks_) {
        if (task->owner_cancel_) {
            const auto state = simulation_.scheduler->GetJobState(task->handle_);
            if (!state) {
                if (!task->terminal_) {
                    (void)task->Fail(state.GetError());
                    return Result::Failure(state.GetError());
                }

            } else if (state.Value() == simulation::SimulationJobState::Completed ||
                       state.Value() == simulation::SimulationJobState::Failed ||
                       state.Value() == simulation::SimulationJobState::Cancelled) {
                if (!task->terminal_)
                    (void)task->Fail(Error("graph.scheduler.host_terminal"));
            } else {
                const auto cancelled = simulation_.scheduler->CancelJob(task->handle_);
                if (!cancelled)
                    return Result::Failure(cancelled.GetError());
            }

            task->owner_cancel_ = false;
        }

        if (task->terminal_)
            continue;
        const auto state = simulation_.scheduler->GetJobState(task->handle_);
        if (!state) {
            (void)task->Fail(state.GetError());
            return Result::Failure(state.GetError());
        }

        if (state.Value() == simulation::SimulationJobState::Failed ||
            state.Value() == simulation::SimulationJobState::Cancelled ||
            state.Value() == simulation::SimulationJobState::Completed) {
            const auto error = Error("graph.scheduler.host_terminal");
            (void)task->Fail(error);
            return Result::Failure(error);
        }

        if (!task->needs_arm_)
            continue;
        const auto scheduled = simulation_.scheduled_tasks->Schedule(
            {{}, task->handle_, task->due_, simulation::SimulationLane::MainThread});
        if (!scheduled)
            return Result::Failure(scheduled.GetError());
        task->needs_arm_ = false;
        ++armed;
    }

    std::erase_if(tasks_, [](const auto &task) { return task->terminal_ && !task->owner_cancel_; });
    return Result::Success(armed);
}

foundation::Result<void> GraphScheduler::Cancel(GraphTask &task) {
    if (task.owner_ != owner_)
        return foundation::Result<void>::Failure(Error("graph.scheduler.foreign_task"));
    // A native may cancel its own task. Let the active ExecuteStep publish the
    // terminal transition; changing Simulation state inside that callback is invalid.
    if (task.running_ || task.terminal_)
        return task.Cancel();
    return simulation_.scheduler->CancelJob(task.handle_);
}

} // namespace epidemic::runtime
