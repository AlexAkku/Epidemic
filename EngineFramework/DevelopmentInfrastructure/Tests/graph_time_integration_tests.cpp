#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/Runtime/Support/graph_scheduler.h"
#include "Epidemic/Runtime/Support/schema_graph_callable.h"

#include <array>
#include <cstdio>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime;

namespace {
void Check(bool value, const char *message) {
    if (!value) {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        throw std::runtime_error(message);
    }

}

foundation::StringId Id(std::string_view name) {
    return foundation::StringId::FromString(name);
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

struct Owner {
    std::shared_ptr<std::size_t> calls;
};

foundation::Result<std::int64_t> Native(Owner &owner, const std::int64_t &value) {
    ++*owner.calls;
    return foundation::Result<std::int64_t>::Success(value + 1);
}

graph::CompiledGraphPtr Program(std::shared_ptr<std::size_t> calls) {
    auto registry = std::make_shared<schema::NativeSchemaRegistry>();
    const auto owner = registry->RegisterType<Owner>("graph.time.native").Value();
    (void)registry->RegisterType<std::int64_t>(graph::kI64TypeName).Value();
    const auto operation = registry->RegisterOperation(owner, "increment", &Native).Value();
    auto native =
        CreateSchemaGraphCallable(registry, {owner, operation}, Owner{std::move(calls)}).Value();
    using namespace graph;
    const auto port = [](std::string_view name, PortDirection direction) {
        return PortDefinition{Id(name), std::string(name), PortKind::Data, direction, kI64Type};

    };

    GraphDefinition definition{AssetId::FromString("framework.graph.time"), "Timed", {}, {}};

    definition.nodes =
        {{Id("value"), Id("input"), "Value", {port("value.out", PortDirection::Output)}},
         {Id("delay"), Id("input"), "Delay", {port("delay.out", PortDirection::Output)}},
         {Id("wait"),
          Id("wait.type"),
          "Wait",
          {port("wait.in", PortDirection::Input), port("wait.ticks", PortDirection::Input),
           port("wait.out", PortDirection::Output)}},
         {Id("native"),
          owner,
          "Native",
          {port("native.in", PortDirection::Input), port("native.out", PortDirection::Output)}},
         {Id("result"), Id("output"), "Result", {port("result.in", PortDirection::Input)}}};

    definition.connections = {{Id("e1"), Id("value.out"), Id("wait.in")},
                              {Id("e2"), Id("delay.out"), Id("wait.ticks")},
                              {Id("e3"), Id("wait.out"), Id("native.in")},
                              {Id("e4"), Id("native.out"), Id("result.in")}};

    const std::array bindings{NodeBinding{Id("input"), Operation::Input, {}},
                              NodeBinding{Id("wait.type"), Operation::Wait, {}},
                              NodeBinding{owner, Operation::NativeCall, native},
                              NodeBinding{Id("output"), Operation::Output, {}}};

    return CompileGraph(definition, bindings).Value();
}

std::unique_ptr<graph::GraphInstance> Instance(const graph::CompiledGraphPtr &program) {
    auto instance = graph::GraphInstance::Create(program).Value();
    Check(instance->SetInput(Id("value.out"), 7).HasValue() &&
              instance->SetInput(Id("delay.out"), 15).HasValue(),
          "inputs");
    return instance;
}

void Run() {
    TimeOptions options;
    options.game_ticks_per_real_second = 1000;
    auto time = CreateTimeServices(options).Value();
    simulation::SimulationDependencies dependencies;
    dependencies.clock = std::make_shared<ClockAdapter>(time.clock);
    auto simulation = simulation::CreateSimulationServices({}, dependencies).Value();
    simulation.scheduler->SetBudget({4, 128});
    GraphScheduler graphs{time.clock, simulation, SimulationZoneId{1}};

    gameplay::time::GameplayTimeService gameplay_time;
    const auto clock = gameplay_time.RegisterClock("gameplay.graph").Value();
    gameplay_time.Freeze();
    gameplay::integration::RuntimeTimeAdapter adapter{*time.clock, gameplay_time, clock};

    auto calls = std::make_shared<std::size_t>(0);
    const auto program = Program(calls);
    const gameplay::GameplayDuration fixed_step{10};

    Check(!adapter.Synchronize(), "zero Runtime revision cannot synchronize Framework");
    Check(time.runtime->Pause().HasValue() && time.runtime->Resume().HasValue(),
          "publish initial Runtime revision");
    Check(adapter.Synchronize().HasValue(), "initial Framework synchronization");
    auto task = graphs.Submit(Instance(program), {{}, GameDuration{fixed_step.ticks}}).Value();
    Check(graphs.Prepare().Value() == 1 && simulation.runtime->Tick().Value().failures.empty() &&
              task->NextWake() == GameTimePoint{20} && *calls == 0,
          "Framework fixed-step Wait at game-tick grid");
    Check(time.runtime->SetTimeScale(TimeScale{2, 1}).HasValue(), "scale Runtime authority");
    Check(time.runtime->Advance(std::chrono::milliseconds{5}).HasValue(), "advance Runtime");
    Check(adapter.Synchronize().HasValue() && graphs.Prepare().HasValue() &&
              simulation.runtime->Tick().Value().processed_jobs == 0,
          "prepare scaled Runtime time");
    const auto scaled_clock = gameplay_time.GetClock(clock);
    Check(scaled_clock && scaled_clock->now.ticks == 10,
          "Framework observes the same scaled Runtime clock");
    Check(time.runtime->Pause().HasValue() &&
              time.runtime->Advance(std::chrono::milliseconds{100}).HasValue(),
          "paused host frame");
    Check(adapter.Synchronize().HasValue() && graphs.Prepare().HasValue() &&
              simulation.runtime->Tick().Value().processed_jobs == 0 && *calls == 0,
          "paused Wait stays suspended");
    Check(time.runtime->Resume().HasValue() &&
              time.runtime->Advance(std::chrono::milliseconds{5}).HasValue(),
          "resume Runtime");
    Check(adapter.Synchronize().HasValue() && graphs.Prepare().HasValue() &&
              simulation.runtime->Tick().Value().failures.empty() &&
              task->ReadValue(Id("result.in")).Value() == 8 && *calls == 1,
          "Runtime -> Framework adapter -> Simulation -> Schema callable");
    const auto completed_clock = gameplay_time.GetClock(clock);
    Check(completed_clock && completed_clock->now.ticks == 20, "Framework time at completion");
    const auto stale = adapter.Synchronize();
    Check(!stale && stale.GetError().HasCode("gameplay.clock_sync_stale"),
          "unchanged Runtime revision stays rejected");
    Check(graphs.Prepare().HasValue() && simulation.runtime->Tick().Value().processed_jobs == 0 &&
              *calls == 1,
          "no repeated side effects");

    gameplay::integration::RuntimeTimeAdapter unknown{*time.clock, gameplay_time, {}};

    Check(!unknown.Synchronize(), "invalid Gameplay clock rejected");
}

} // namespace

int main() {
    try {
        Run();
        return 0;
    } catch (...) {
        std::fputs("Graph Time integration tests failed\n", stderr);
        return 1;
    }

}
