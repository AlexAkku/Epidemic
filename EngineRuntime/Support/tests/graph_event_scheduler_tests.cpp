#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Support/graph_scheduler.h"
#include "Epidemic/Runtime/Support/graph_serialization.h"

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
template <typename T>
void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
  Check(!value && value.GetError().HasCode(code),
        "unexpected event scheduler error");
}
class ClockAdapter final : public simulation::ISimulationClock {
public:
  explicit ClockAdapter(std::shared_ptr<IGameClock> clock)
      : clock_(std::move(clock)) {}
  simulation::SimulationTime Now() const override { return clock_->Now(); }

private:
  std::shared_ptr<IGameClock> clock_;
};
simulation::SimulationServices
Simulation(const std::shared_ptr<IGameClock> &clock) {
  simulation::SimulationDependencies dependencies;
  dependencies.clock = std::make_shared<ClockAdapter>(clock);
  auto services =
      simulation::CreateSimulationServices({}, dependencies).Value();
  services.scheduler->SetBudget({8, 256});
  return services;
}
using Effect = std::pair<int, std::int64_t>;
struct Effects {
  std::vector<Effect> values;
  std::function<void()> callback;
};
class Invocation final : public graph::INativeInvocation {
public:
  Invocation(std::shared_ptr<Effects> effects, int marker)
      : effects_(std::move(effects)), marker_(marker) {}
  foundation::Result<std::int64_t> Invoke(std::int64_t value) override {
    effects_->values.emplace_back(marker_, value);
    if (marker_ == 1 && effects_->callback)
      effects_->callback();
    return foundation::Result<std::int64_t>::Success(value);
  }

private:
  std::shared_ptr<Effects> effects_;
  int marker_;
};
class Factory final : public graph::INativeCallableFactory {
public:
  Factory(std::shared_ptr<Effects> effects, int marker)
      : effects_(std::move(effects)), marker_(marker) {}
  foundation::Result<std::unique_ptr<graph::INativeInvocation>>
  CreateInvocation() const override {
    return foundation::Result<std::unique_ptr<graph::INativeInvocation>>::
        Success(std::make_unique<Invocation>(effects_, marker_));
  }

private:
  std::shared_ptr<Effects> effects_;
  int marker_;
};
graph::CompiledGraphPtr Program(const std::shared_ptr<Effects> &effects) {
  using namespace graph;
  const auto data = [](std::string_view name, PortDirection direction) {
    return PortDefinition{Id(name), std::string(name), PortKind::Data,
                          direction, kI64Type};
  };
  const auto control = [](std::string_view name, PortDirection direction) {
    return PortDefinition{
        Id(name), std::string(name), PortKind::Control, direction, {}};
  };
  constexpr auto in = PortDirection::Input;
  constexpr auto out = PortDirection::Output;
  GraphDefinition definition{
      AssetId::FromString("wave.control"), "Wave", {}, {}};
  definition.nodes = {
      {Id("event"),
       Id("event.type"),
       "Event",
       {{Id("event.in"), "Event", PortKind::Event, in, kI64Type},
        data("payload", out),
        control("entry", out)}},
      {Id("branch"),
       Id("branch.type"),
       "Branch",
       {data("condition", in), control("branch.in", in), control("true", out),
        control("false", out)}},
      {Id("repeat"),
       Id("repeat.type"),
       "Repeat",
       {data("count", in), data("index", out), control("repeat.in", in),
        control("body", out), control("done", out)}},
      {Id("sequence"),
       Id("sequence.type"),
       "Sequence",
       {control("sequence.in", in), control("sequence.first", out),
        control("sequence.second", out)}},
      {Id("delay"), Id("input.type"), "Delay", {data("delay.out", out)}},
      {Id("spawn"),
       Id("spawn.type"),
       "Spawn",
       {data("spawn.arg", in), data("spawn.result", out),
        control("spawn.in", in), control("spawn.next", out)}},
      {Id("wait"),
       Id("wait.type"),
       "Wait",
       {data("wait.value", in), data("wait.ticks", in),
        data("wait.result", out), control("wait.in", in),
        control("wait.next", out)}},
      {Id("after"),
       Id("after.type"),
       "After",
       {data("after.arg", in), data("after.result", out),
        control("after.in", in), control("after.next", out)}},
      {Id("finish"),
       Id("finish.type"),
       "Finish",
       {data("finish.arg", in), data("finish.result", out),
        control("finish.in", in), control("finish.next", out)}},
      {Id("output"),
       Id("output.type"),
       "Output",
       {data("result", in), control("output.in", in)}}};
  const auto edge = [&](std::string_view from, std::string_view to) {
    definition.connections.push_back(
        {Id(std::string(from) + "/" + std::string(to)), Id(from), Id(to)});
  };
  edge("payload", "condition");
  edge("payload", "count");
  edge("payload", "finish.arg");
  edge("index", "spawn.arg");
  edge("spawn.result", "wait.value");
  edge("delay.out", "wait.ticks");
  edge("wait.result", "after.arg");
  edge("finish.result", "result");
  edge("entry", "branch.in");
  edge("true", "repeat.in");
  edge("false", "finish.in");
  edge("body", "sequence.in");
  edge("sequence.first", "spawn.in");
  edge("sequence.second", "after.in");
  edge("spawn.next", "wait.in");
  edge("done", "finish.in");
  edge("finish.next", "output.in");
  const std::array bindings{
      NodeBinding{Id("event.type"), Operation::EventEntry, {}},
      NodeBinding{Id("branch.type"), Operation::Branch, {}},
      NodeBinding{Id("repeat.type"), Operation::Repeat, {}},
      NodeBinding{Id("input.type"), Operation::Input, {}},
      NodeBinding{Id("sequence.type"), Operation::Sequence, {}},
      NodeBinding{Id("wait.type"), Operation::Wait, {}},
      NodeBinding{Id("output.type"), Operation::Output, {}},
      NodeBinding{Id("spawn.type"), Operation::NativeCall,
                  std::make_shared<Factory>(effects, 1)},
      NodeBinding{Id("after.type"), Operation::NativeCall,
                  std::make_shared<Factory>(effects, 2)},
      NodeBinding{Id("finish.type"), Operation::NativeCall,
                  std::make_shared<Factory>(effects, 9)}};
  std::vector<TypeId> types{kI64Type};
  for (const auto &binding : bindings)
    types.push_back(binding.node_type);
  GraphDefinitionSerializer serializer{types};
  auto serialization = CreateSerializationServices().Value();
  const auto saved =
      SerializeToDocument(serializer, definition, *serialization.archives)
          .Value();
  auto reader = serialization.archives->CreateReader(saved).Value();
  GraphDefinition loaded;
  Check(DeserializeObject(serializer, *reader, loaded).HasValue() &&
            loaded == definition,
        "wave round-trip preserves stable ports and edges");
  return CompileGraph(loaded, bindings).Value();
}
struct Fixture {
  TimeServices time = CreateTimeServices().Value();
  simulation::SimulationServices simulation = Simulation(time.clock);
  std::shared_ptr<Effects> effects = std::make_shared<Effects>();
  graph::CompiledGraphPtr program = Program(effects);
  GraphScheduler graphs{time.clock, simulation, SimulationZoneId{1}};
  std::unique_ptr<graph::GraphInstance> Instance(std::int64_t count,
                                                 GameDuration delay) {
    auto instance =
        graph::GraphInstance::CreateForEvent(program, Id("event.in"), count)
            .Value();
    Check(instance->SetInput(Id("delay.out"), delay.ticks).HasValue(),
          "configure wait delay");
    return instance;
  }
  void Tick() {
    Check(graphs.Prepare().HasValue(), "prepare event continuations");
    Check(simulation.runtime->Tick().Value().failures.empty(),
          "host tick succeeds");
  }
  void Advance(std::int64_t ticks = 1) {
    Check(time.runtime->Skip(GameDuration{ticks}).HasValue(),
          "advance game time");
  }
};
bool Status(const GraphTask &task, graph::ExecutionStatus status) {
  return task.LastReport() && task.LastReport()->status == status;
}
void WaveAndZeroWait() {
  Fixture f;
  const auto task = f.graphs.Submit(f.Instance(3, GameDuration{1})).Value();
  f.Tick();
  Check(f.effects->values == std::vector<Effect>{{1, 0}} &&
            Status(*task, graph::ExecutionStatus::Waiting),
        "first spawn before wait");
  ErrorIs(task->ReadValue(Id("wait.result")), "graph.vm.value_unavailable");
  f.Tick();
  Check(f.effects->values.size() == 1, "game time must reach due point");
  for (int i = 0; i < 3; ++i) {
    f.Advance();
    f.Tick();
  }
  Check(f.effects->values ==
            std::vector<Effect>{
                {1, 0}, {2, 0}, {1, 1}, {2, 1}, {1, 2}, {2, 2}, {9, 3}},
        "ordered wave effects across waits");
  Check(Status(*task, graph::ExecutionStatus::Completed) &&
            task->ReadValue(Id("result")).Value() == 3,
        "wave result");
  Fixture zero;
  auto immediate =
      zero.graphs.Submit(zero.Instance(2, GameDuration{0})).Value();
  zero.Tick();
  Check(zero.effects->values.size() == 1, "zero wait yields host tick");
  zero.Tick();
  Check(zero.effects->values.size() == 3,
        "zero wait resumes exactly once per tick");
  zero.Tick();
  Check(Status(*immediate, graph::ExecutionStatus::Completed) &&
            zero.effects->values.size() == 5,
        "zero wait completes without clock advance");
  Fixture bypass;
  auto false_branch =
      bypass.graphs.Submit(bypass.Instance(0, GameDuration{-1})).Value();
  bypass.Tick();
  Check(Status(*false_branch, graph::ExecutionStatus::Completed) &&
            bypass.effects->values == std::vector<Effect>{{9, 0}},
        "unchosen invalid wait is not evaluated");
}
void IndependentAndBudgeted() {
  Fixture f;
  auto cancelled = f.graphs.Submit(f.Instance(2, GameDuration{1})).Value();
  auto survivor = f.graphs.Submit(f.Instance(1, GameDuration{1})).Value();
  f.Tick();
  Check(f.effects->values == std::vector<Effect>{{1, 0}, {1, 0}},
        "independent event prefixes");
  Check(f.graphs.Cancel(*cancelled).HasValue(), "cancel waiting event");
  f.Advance();
  f.Tick();
  Check(Status(*cancelled, graph::ExecutionStatus::Cancelled) &&
            Status(*survivor, graph::ExecutionStatus::Completed) &&
            f.effects->values ==
                std::vector<Effect>{{1, 0}, {1, 0}, {2, 0}, {9, 1}},
        "cancel leaves other activation intact");
  Check(f.simulation.scheduled_tasks->QueryDue(GameTimePoint{1000}).empty(),
        "no orphan wait timer");
  Fixture bounded;
  auto task =
      bounded.graphs.Submit(bounded.Instance(2, GameDuration{0}), {{1, 1}, {}})
          .Value();
  for (int tick = 0;
       tick < 100 && !Status(*task, graph::ExecutionStatus::Completed);
       ++tick) {
    bounded.Tick();
    const auto &report = task->LastReport();
    if (!report)
      throw std::runtime_error("missing budget report");
    Check(report->instructions <= 1 && report->native_calls <= 1,
          "per-tick budgets");
  }
  Check(Status(*task, graph::ExecutionStatus::Completed) &&
            bounded.effects->values ==
                std::vector<Effect>{{1, 0}, {2, 0}, {1, 1}, {2, 1}, {9, 2}},
        "budget yields preserve loop and wait continuations");
}
void OwnerLifetime() {
  Fixture f;
  GraphActivationOwner dead;
  auto never =
      f.graphs.SubmitOwned(f.Instance(3, GameDuration{1}), dead).Value();
  dead.Invalidate();
  dead.Invalidate();
  f.Tick();
  Check(!dead.IsAlive() && Status(*never, graph::ExecutionStatus::Cancelled) &&
            f.effects->values.empty(),
        "owner invalidated before first execution");
  ErrorIs(f.graphs.SubmitOwned(f.Instance(1, GameDuration{1}), dead),
          "graph.scheduler.owner_expired");
  auto owner = std::make_unique<GraphActivationOwner>();
  auto waiting =
      f.graphs.SubmitOwned(f.Instance(3, GameDuration{1}), *owner).Value();
  auto other = f.graphs.Submit(f.Instance(1, GameDuration{1})).Value();
  f.Tick();
  Check(f.graphs.Prepare().HasValue(), "arm both wait timers");
  owner.reset();
  f.Tick();
  Check(
      Status(*waiting, graph::ExecutionStatus::Cancelled) &&
          f.simulation.scheduled_tasks->QueryDue(GameTimePoint{1000}).size() ==
              1,
      "owner destruction removes only owned timer");
  f.Advance();
  f.Tick();
  Check(Status(*other, graph::ExecutionStatus::Completed) &&
            f.effects->values ==
                std::vector<Effect>{{1, 0}, {1, 0}, {2, 0}, {9, 1}},
        "unowned activation survives");
  Fixture inside;
  auto callback_owner = std::make_unique<GraphActivationOwner>();
  auto task =
      inside.graphs
          .SubmitOwned(inside.Instance(3, GameDuration{1}), *callback_owner)
          .Value();
  inside.effects->callback = [&] { callback_owner.reset(); };
  inside.Tick();
  inside.Tick();
  inside.Advance(100);
  inside.Tick();
  Check(Status(*task, graph::ExecutionStatus::Cancelled) &&
            inside.effects->values == std::vector<Effect>{{1, 0}} &&
            inside.simulation.scheduled_tasks->QueryDue(GameTimePoint{1000})
                .empty(),
        "owner destroyed inside native action stops suffix without reentrancy");
  Fixture siblings;
  GraphActivationOwner shared_owner;
  auto first =
      siblings.graphs
          .SubmitOwned(siblings.Instance(2, GameDuration{1}), shared_owner)
          .Value();
  auto second =
      siblings.graphs
          .SubmitOwned(siblings.Instance(2, GameDuration{1}), shared_owner)
          .Value();
  siblings.Tick();
  siblings.effects->callback = [&] { shared_owner.Invalidate(); };
  siblings.Advance();
  siblings.Tick();
  siblings.Tick();
  Check(Status(*first, graph::ExecutionStatus::Cancelled) &&
            Status(*second, graph::ExecutionStatus::Cancelled) &&
            siblings.effects->values ==
                std::vector<Effect>{{1, 0}, {1, 0}, {2, 0}, {1, 1}},
        "owner cancellation reaches a waiting sibling already dequeued in the "
        "same host tick");
  for (int i = 0; i < 12; ++i) {
    GraphActivationOwner temporary;
    auto short_task =
        inside.graphs
            .SubmitOwned(inside.Instance(1, GameDuration{1}), temporary)
            .Value();
    temporary.Invalidate();
    inside.Tick();
    Check(Status(*short_task, graph::ExecutionStatus::Cancelled),
          "short-lived owned activation retires");
  }
}
void DirectWaitAndFailure() {
  Fixture f;
  auto instance = f.Instance(1, GameDuration{1});
  Check(instance->Run().status == graph::ExecutionStatus::Waiting,
        "controlled VM wait");
  const auto stalled = instance->Run();
  Check(stalled.status == graph::ExecutionStatus::Waiting &&
            stalled.instructions == 0 && f.effects->values.size() == 1,
        "Run cannot skip wait");
  Check(instance->ResumeWait().HasValue() &&
            instance->ReadValue(Id("wait.result")).Value() == 0,
        "host acknowledgment publishes wait result");
  Check(instance->Run().status == graph::ExecutionStatus::Completed &&
            f.effects->values == std::vector<Effect>{{1, 0}, {2, 0}, {9, 1}},
        "direct resume preserves suffix");
  auto negative = f.Instance(1, GameDuration{-1});
  const auto failed = negative->Run();
  Check(failed.status == graph::ExecutionStatus::Failed && failed.diagnostic &&
            failed.diagnostic->node == Id("wait") &&
            failed.diagnostic->error.HasCode("graph.vm.negative_wait"),
        "negative controlled wait attributed to node");
  Fixture overflow;
  overflow.Advance(1);
  auto task =
      overflow.graphs
          .Submit(overflow.Instance(
              1, GameDuration{std::numeric_limits<std::int64_t>::max()}))
          .Value();
  overflow.Tick();
  const auto &report = task->LastReport();
  if (!report || !report->diagnostic)
    throw std::runtime_error("missing overflow diagnostic");
  Check(report->status == graph::ExecutionStatus::Failed &&
            report->diagnostic->error.HasCode("graph.scheduler.time_overflow"),
        "controlled wait clock overflow");
}
} // namespace
int main() {
  try {
    WaveAndZeroWait();
    IndependentAndBudgeted();
    OwnerLifetime();
    DirectWaitAndFailure();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
  return 0;
}
