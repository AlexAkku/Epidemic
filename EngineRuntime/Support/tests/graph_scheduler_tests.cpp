#include "Epidemic/Runtime/Support/graph_scheduler.h"

#include <array>
#include <cstdio>
#include <functional>
#include <limits>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime;

namespace {
graph::ElementId Id(std::string_view name) {
    return graph::ElementId::FromString(name);
}

void Check(bool value, const char *message) {
    if (!value) {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        throw std::runtime_error(message);
    }

}

template <typename T> void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
    Check(!value && value.GetError().HasCode(code), "unexpected graph scheduler error");
}

class ClockAdapter final : public simulation::ISimulationClock {
  public:
    explicit ClockAdapter(std::shared_ptr<IGameClock> clock) : clock_(std::move(clock)) {
    }

    simulation::SimulationTime Now() const override {
        return clock_->Now();
    }

  private:
    std::shared_ptr<IGameClock> clock_;
};

simulation::SimulationServices Simulation(std::shared_ptr<IGameClock> clock) {
    simulation::SimulationDependencies dependencies;
    dependencies.clock = std::make_shared<ClockAdapter>(std::move(clock));
    auto services = simulation::CreateSimulationServices({}, dependencies).Value();
    services.scheduler->SetBudget({8, 128});
    return services;
}

struct Effects {
    std::vector<std::int64_t> values;
    std::function<void()> callback;
};

class Invocation final : public graph::INativeInvocation {
  public:
    explicit Invocation(std::shared_ptr<Effects> effects) : effects_(std::move(effects)) {
    }

    foundation::Result<std::int64_t> Invoke(std::int64_t value) override {
        effects_->values.push_back(value);
        if (effects_->callback)
            effects_->callback();
        return foundation::Result<std::int64_t>::Success(value + 1);
    }

  private:
    std::shared_ptr<Effects> effects_;
};

class Factory final : public graph::INativeCallableFactory {
  public:
    explicit Factory(std::shared_ptr<Effects> effects) : effects_(std::move(effects)) {
    }

    foundation::Result<std::unique_ptr<graph::INativeInvocation>>
    CreateInvocation() const override {
        return foundation::Result<std::unique_ptr<graph::INativeInvocation>>::Success(
            std::make_unique<Invocation>(effects_));
    }

  private:
    std::shared_ptr<Effects> effects_;
};

graph::CompiledGraphPtr Program(const std::shared_ptr<Effects> &effects) {
    using namespace graph;
    const auto port = [](std::string_view name, PortDirection direction) {
        return PortDefinition{Id(name), std::string(name), PortKind::Data, direction, kI64Type};

    };

    GraphDefinition definition{AssetId::FromString("scheduler.example"), "Scheduler", {}, {}};

    definition.nodes =
        {{Id("value"), Id("input"), "Value", {port("value.out", PortDirection::Output)}},
         {Id("delay"), Id("input"), "Delay", {port("delay.out", PortDirection::Output)}},
         {Id("before"),
          Id("native"),
          "Before",
          {port("before.in", PortDirection::Input), port("before.out", PortDirection::Output)}},
         {Id("wait"),
          Id("wait.type"),
          "Wait",
          {port("wait.in", PortDirection::Input), port("wait.ticks", PortDirection::Input),
           port("wait.out", PortDirection::Output)}},
         {Id("after"),
          Id("native"),
          "After",
          {port("after.in", PortDirection::Input), port("after.out", PortDirection::Output)}},
         {Id("result"), Id("output"), "Result", {port("result.in", PortDirection::Input)}}};

    definition.connections = {{Id("e1"), Id("value.out"), Id("before.in")},
                              {Id("e2"), Id("before.out"), Id("wait.in")},
                              {Id("e3"), Id("delay.out"), Id("wait.ticks")},
                              {Id("e4"), Id("wait.out"), Id("after.in")},
                              {Id("e5"), Id("after.out"), Id("result.in")}};

    const std::array bindings{NodeBinding{Id("input"), Operation::Input, {}},
                              NodeBinding{Id("native"), Operation::NativeCall,
                                          std::make_shared<Factory>(effects)},
                              NodeBinding{Id("wait.type"), Operation::Wait, {}},
                              NodeBinding{Id("output"), Operation::Output, {}}};

    return CompileGraph(definition, bindings).Value();
}

std::unique_ptr<graph::GraphInstance> Instance(const graph::CompiledGraphPtr &program,
                                               std::int64_t value = 10, std::int64_t delay = 3) {
    auto instance = graph::GraphInstance::Create(program).Value();
    Check(instance->SetInput(Id("value.out"), value).HasValue() &&
              instance->SetInput(Id("delay.out"), delay).HasValue(),
          "inputs");
    return instance;
}

struct Fixture {
    TimeServices time = CreateTimeServices().Value();
    simulation::SimulationServices simulation = Simulation(time.clock);
    std::shared_ptr<Effects> effects = std::make_shared<Effects>();
    graph::CompiledGraphPtr program = Program(effects);
    GraphScheduler graphs{time.clock, simulation, SimulationZoneId{1}};

    simulation::SimulationTickResult Tick() {
        Check(graphs.Prepare().HasValue(), "prepare schedules");
        auto tick = simulation.runtime->Tick().Value();
        Check(tick.failures.empty(), "simulation tick has no failures");
        return tick;
    }

    void Advance(std::int64_t ticks) {
        Check(time.runtime->Skip(GameDuration{ticks}).HasValue(), "advance authoritative time");
    }

};

void WaitsAndCancellation() {
    Fixture f;
    auto first = f.graphs.Submit(Instance(f.program)).Value();
    auto second = f.graphs.Submit(Instance(f.program, 20)).Value();
    Check(f.Tick().processed_jobs == 2 && f.effects->values == std::vector<std::int64_t>{10, 20},
          "two ordered prefixes");
    Check(first->LastReport() && first->LastReport()->status == graph::ExecutionStatus::Waiting &&
              first->NextWake() == GameTimePoint{3},
          "waiting continuation has deadline");
    Check(f.Tick().processed_jobs == 0 && f.graphs.Prepare().Value() == 0,
          "waiting jobs are suspended, duplicate Prepare is inert");
    Check(f.time.runtime->Pause().HasValue(), "pause");
    Check(f.time.runtime->Advance(std::chrono::seconds{20}).HasValue() &&
              f.Tick().processed_jobs == 0,
          "pause does not expire waits");
    Check(f.time.runtime->Resume().HasValue() &&
              f.time.runtime->SetTimeScale(TimeScale{2, 1}).HasValue(),
          "resume scaled time");
    Check(f.time.runtime->Advance(std::chrono::seconds{1}).HasValue() &&
              f.Tick().processed_jobs == 0,
          "scaled game time still before deadline");
    f.Advance(1);
    Check(f.Tick().processed_jobs == 2 && first->ReadValue(Id("result.in")).Value() == 12 &&
              second->ReadValue(Id("result.in")).Value() == 22 &&
              f.effects->values == std::vector<std::int64_t>{10, 20, 11, 21},
          "same program, independent continuations, sequential effects");
    Check(f.Tick().processed_jobs == 0, "completed effects never replay");
    auto cancelled = f.graphs.Submit(Instance(f.program, 30)).Value();
    f.Tick();
    f.Tick();
    Check(f.graphs.Cancel(*cancelled).HasValue() && !cancelled->NextWake(), "cancel armed Wait");
    f.Advance(100);
    f.Tick();
    Check(cancelled->LastReport() &&
              cancelled->LastReport()->status == graph::ExecutionStatus::Cancelled &&
              f.effects->values.back() == 30 &&
              f.simulation.scheduled_tasks->QueryDue(GameTimePoint{1000}).empty(),
          "cancel removes timer and preserves only accepted prefix");
    auto ready = f.graphs.Submit(Instance(f.program)).Value();
    Check(f.graphs.Cancel(*ready).HasValue() && f.Tick().processed_jobs == 0,
          "cancel before first execution");
    GraphScheduler foreign{f.time.clock, f.simulation, SimulationZoneId{1}};

    ErrorIs(foreign.Cancel(*first), "graph.scheduler.foreign_task");
}

class FlakyTasks final : public simulation::IScheduledSimulationTasks {
  public:
    explicit FlakyTasks(std::shared_ptr<simulation::IScheduledSimulationTasks> tasks)
        : tasks_(std::move(tasks)) {
    }

    bool fail_next = false;

    foundation::Result<simulation::ScheduledSimulationTaskId>
    Schedule(simulation::ScheduledSimulationTask task) override {
        if (fail_next) {
            fail_next = false;
            return foundation::Result<simulation::ScheduledSimulationTaskId>::Failure(
                foundation::Error::Create("test.timer_failure", "retry timer"));
        }

        return tasks_->Schedule(task);
    }

    foundation::Result<void> Cancel(simulation::ScheduledSimulationTaskId id) override {
        return tasks_->Cancel(id);
    }

    std::vector<simulation::ScheduledSimulationTask>
    QueryDue(simulation::SimulationTime now) const override {
        return tasks_->QueryDue(now);
    }

    simulation::ScheduledTaskExecutionResult
    ExecuteDueWithinBudget(simulation::SimulationTime now,
                           const simulation::SimulationBudget &budget) override {
        return tasks_->ExecuteDueWithinBudget(now, budget);
    }

  private:
    std::shared_ptr<simulation::IScheduledSimulationTasks> tasks_;
};

void RetryAndFixedStep() {
    Fixture f;
    auto proxy = std::make_shared<FlakyTasks>(f.simulation.scheduled_tasks);
    auto services = f.simulation;
    services.scheduled_tasks = proxy;
    GraphScheduler graphs{f.time.clock, services, SimulationZoneId{1}};

    auto task = graphs.Submit(Instance(f.program)).Value();
    Check(graphs.Prepare().Value() == 1 && f.simulation.runtime->Tick().Value().failures.empty(),
          "initial prefix");
    proxy->fail_next = true;
    ErrorIs(graphs.Prepare(), "test.timer_failure");
    Check(graphs.Prepare().Value() == 1 && graphs.Prepare().Value() == 0 &&
              f.effects->values.size() == 1,
          "retry arm without replay");
    f.Advance(3);
    Check(f.simulation.runtime->Tick().Value().failures.empty() &&
              task->ReadValue(Id("result.in")).Value() == 12 && f.effects->values.size() == 2,
          "retried continuation completes once");

    Fixture fixed;
    auto stepped =
        fixed.graphs.Submit(Instance(fixed.program, 10, 15), {{3, 8}, GameDuration{10}}).Value();
    fixed.Tick();
    Check(stepped->NextWake() == GameTimePoint{10} && fixed.effects->values.size() == 1,
          "instruction yield retains fixed grid");
    fixed.Advance(25);
    fixed.Tick();
    Check(stepped->NextWake() == GameTimePoint{30} && fixed.effects->values.size() == 1,
          "Wait rounds to next grid from logical step, not late host time");
    fixed.Advance(4);
    Check(fixed.Tick().processed_jobs == 0, "fixed Wait not early");
    fixed.Advance(1);
    fixed.Tick();
    Check(stepped->ReadValue(Id("result.in")).Value() == 12, "fixed-step continuation");

    Fixture catchup;
    auto bounded =
        catchup.graphs.Submit(Instance(catchup.program), {{1, 1}, GameDuration{10}}).Value();
    catchup.Tick();
    catchup.Advance(25);
    catchup.Tick();
    Check(bounded->NextWake() == GameTimePoint{20}, "overdue phase retained");
    catchup.Tick();
    Check(bounded->NextWake() == GameTimePoint{30} && catchup.effects->values.size() == 1,
          "catch up one bounded step per host Tick");
    Check(catchup.Tick().processed_jobs == 0, "future fixed step suspended");
}

void FailuresAndBudgets() {
    Fixture f;
    f.simulation.scheduler->SetBudget({1, 1});
    auto task = f.graphs.Submit(Instance(f.program, 1, 0)).Value();
    const auto tick = f.Tick();
    Check(tick.processed_jobs == 1 && task->LastReport() && task->LastReport()->instructions == 1,
          "existing Simulation work budget caps VM instructions");
    Check(f.graphs.Cancel(*task).HasValue(), "cancel budget yield");
    GraphScheduler limited{f.time.clock, f.simulation, SimulationZoneId{1}, 1};

    auto active = limited.Submit(Instance(f.program)).Value();
    ErrorIs(limited.Submit(Instance(f.program)), "graph.scheduler.capacity");
    Check(limited.Cancel(*active).HasValue() && limited.Submit(Instance(f.program)).HasValue(),
          "terminal capacity released");
    ErrorIs(f.graphs.Submit(Instance(f.program), {{0, 1}, {}}), "graph.scheduler.invalid_options");
    ErrorIs(f.graphs.Submit(Instance(f.program), {{1, 1}, GameDuration{-1}}),
            "graph.scheduler.invalid_options");
    auto started = Instance(f.program);
    (void)started->Run({0, 0});
    ErrorIs(f.graphs.Submit(std::move(started)), "graph.scheduler.invalid_options");

    Fixture unarmed;
    auto missed = unarmed.graphs.Submit(Instance(unarmed.program)).Value();
    Check(unarmed.simulation.runtime->Tick().Value().failures.empty() && missed->LastReport() &&
              missed->LastReport()->diagnostic &&
              missed->LastReport()->diagnostic->error.HasCode("graph.scheduler.prepare_required") &&
              unarmed.effects->values.empty(),
          "missing Prepare fails before any graph effect");
    Fixture native_failure;
    native_failure.effects->callback = [] { throw std::runtime_error("native rejected"); };

    auto failed = native_failure.graphs.Submit(Instance(native_failure.program)).Value();
    native_failure.Tick();
    Check(failed->LastReport() && failed->LastReport()->diagnostic &&
              failed->LastReport()->native_calls == 1 &&
              failed->LastReport()->diagnostic->error.HasCode("graph.vm.native_exception") &&
              native_failure.simulation.scheduler->GetJobState(failed->Handle()).Value() ==
                  simulation::SimulationJobState::Failed &&
              native_failure.Tick().processed_jobs == 0,
          "native failure terminal and diagnosed without replay");

    Fixture overflow;
    auto excessive =
        overflow.graphs
            .Submit(Instance(overflow.program, 1, std::numeric_limits<std::int64_t>::max()))
            .Value();
    overflow.Advance(1);
    Check(overflow.graphs.Prepare().HasValue(), "overflow prepare");
    Check(overflow.simulation.runtime->Tick().Value().failures.empty() && excessive->LastReport() &&
              excessive->LastReport()->diagnostic &&
              excessive->LastReport()->diagnostic->node == Id("wait") &&
              excessive->LastReport()->diagnostic->error.HasCode("graph.scheduler.time_overflow") &&
              excessive->LastReport()->instructions == 4 &&
              overflow.simulation.scheduler->GetJobState(excessive->Handle()).Value() ==
                  simulation::SimulationJobState::Failed,
          "deadline overflow diagnosed at Wait and spent work accounted");
    Fixture rewind;
    const auto checkpoint = rewind.time.runtime->CaptureCheckpoint();
    auto waiting = rewind.graphs.Submit(Instance(rewind.program)).Value();
    rewind.Tick();
    rewind.Advance(1);
    rewind.Tick();
    Check(rewind.time.runtime->RestoreCheckpoint(checkpoint).HasValue(), "restore clock fixture");
    ErrorIs(rewind.graphs.Prepare(), "graph.scheduler.clock_rewound");
    Check(rewind.graphs.Cancel(*waiting).HasValue(), "cancel after time rebind required");

    Fixture self;
    auto self_task = self.graphs.Submit(Instance(self.program)).Value();
    self.effects->callback = [&] {
        ErrorIs(self.graphs.Prepare(), "graph.scheduler.reentrant");
        Check(self.graphs.Cancel(*self_task).HasValue(), "native self cancellation");
    };

    self.Tick();
    self.effects->callback = {};

    Check(self_task->LastReport() &&
              self_task->LastReport()->status == graph::ExecutionStatus::Cancelled &&
              self.effects->values.size() == 1,
          "self cancellation commits through Simulation without suffix");
}

} // namespace

int main() {
    try {
        WaitsAndCancellation();
        RetryAndFixedStep();
        FailuresAndBudgets();
        return 0;
    } catch (...) {
        std::fputs("Graph scheduler tests failed\n", stderr);
        return 1;
    }

}
