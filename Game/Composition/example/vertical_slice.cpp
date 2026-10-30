#include "Epidemic/Game/Composition/graph_host.h"
#include "Epidemic/GameFramework/Integration/prefab.h"
#include "Epidemic/GameFramework/Integration/system_instance.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
using namespace epidemic;
using namespace epidemic::runtime;
namespace fw = epidemic::gameplay;
namespace comp = fw::composition;
namespace gc = epidemic::game::composition;
namespace rb = fw::runtime_bridge;
foundation::StringId Id(std::string_view name) {
  return foundation::StringId::FromString(name);
}
void Require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
template <typename T> T Take(foundation::Result<T> &&result) {
  if (!result)
    throw std::runtime_error(result.GetError().code);
  return std::move(result).Value();
}
// This headless representation backend uses the normal RuntimeBridge projection
// protocol. Entity identity/lifecycle remains entirely in EntityService.
class HeadlessBackend final : public rb::IRuntimeBridgeBackend {
public:
  foundation::Result<rb::RuntimeMaterializationResult>
  Materialize(rb::RuntimePersistentObjectHandle) override {
    return foundation::Result<rb::RuntimeMaterializationResult>::Success(
        {rb::RuntimeObjectHandle{next_++}, true});
  }
  foundation::Result<void> Dematerialize(rb::RuntimeObjectHandle) override {
    return foundation::Result<void>::Success();
  }
  foundation::Result<void> Destroy(rb::RuntimeObjectHandle) override {
    return foundation::Result<void>::Success();
  }
  foundation::Result<void> ApplyImpulse(rb::RuntimePhysicsBodyHandle,
                                        rb::RuntimeVector3) override {
    return foundation::Result<void>::Success();
  }
  foundation::Result<void>
  ProjectEnvironment(rb::RuntimeRegionHandle,
                     const rb::RuntimeEnvironmentValues &,
                     fw::Revision) override {
    return foundation::Result<void>::Success();
  }
  std::vector<rb::RuntimeContactObservation> ConsumeContacts() override {
    return {};
  }

private:
  std::uint64_t next_ = 1;
};
class Clock final : public simulation::ISimulationClock {
public:
  explicit Clock(std::shared_ptr<IGameClock> clock)
      : clock_(std::move(clock)) {}
  simulation::SimulationTime Now() const override { return clock_->Now(); }

private:
  std::shared_ptr<IGameClock> clock_;
};
struct Health {
  std::int64_t points = 100;
};
struct Movement {
  double speed = 1.0;
};
bool HealthValid(const std::int64_t &value) {
  return value >= 0 && value <= 10000;
}
bool SpeedValid(const double &value) {
  return std::isfinite(value) && value >= 0;
}
graph::PortDefinition Port(std::string_view name, graph::PortKind kind,
                           graph::PortDirection direction) {
  return {Id(name), std::string(name), kind, direction,
          kind == graph::PortKind::Control ? graph::TypeId{} : graph::kI64Type};
}
auto DIn(std::string_view name) {
  return Port(name, graph::PortKind::Data, graph::PortDirection::Input);
}
auto DOut(std::string_view name) {
  return Port(name, graph::PortKind::Data, graph::PortDirection::Output);
}
auto CIn(std::string_view name) {
  return Port(name, graph::PortKind::Control, graph::PortDirection::Input);
}
auto COut(std::string_view name) {
  return Port(name, graph::PortKind::Control, graph::PortDirection::Output);
}
struct Authoring {
  graph::GraphDefinition definition;
  explicit Authoring(std::string_view asset)
      : definition{AssetId::FromString(asset), std::string(asset), {}, {}} {}
  void Node(std::string_view name, std::string_view type,
            std::initializer_list<graph::PortDefinition> ports) {
    definition.nodes.push_back({Id(name), Id(type), std::string(name), ports});
  }
  void Edge(std::string_view from, std::string_view to) {
    definition.connections.push_back(
        {Id(std::string(from) + "/" + std::string(to)), Id(from), Id(to)});
  }
  void Entry() {
    Node("event", "event.type",
         {Port("event.in", graph::PortKind::Event, graph::PortDirection::Input),
          DOut("payload"), COut("event.next")});
  }
  void Delay() { Node("delay", "input.type", {DOut("delay.value")}); }
  void Wait() {
    Node("wait", "wait.type",
         {DIn("wait.value"), DIn("wait.ticks"), DOut("wait.result"),
          CIn("wait.in"), COut("wait.next")});
    Edge("delay.value", "wait.ticks");
  }
};
graph::GraphDefinition EnemyGraph() {
  Authoring a{"example.enemy.graph"};
  a.Entry();
  a.Delay();
  a.Wait();
  a.Node("damage.before", "native.type",
         {DIn("before.arg"), DOut("before.result"), CIn("before.in"),
          COut("before.next")});
  a.Node("damage.after", "native.type",
         {DIn("after.arg"), DOut("after.result"), CIn("after.in"),
          COut("after.next")});
  a.Edge("payload", "before.arg");
  a.Edge("before.result", "wait.value");
  a.Edge("wait.result", "after.arg");
  a.Edge("event.next", "before.in");
  a.Edge("before.next", "wait.in");
  a.Edge("wait.next", "after.in");
  return std::move(a.definition);
}
graph::GraphDefinition WaveGraph() {
  Authoring a{"example.wave.graph"};
  a.Entry();
  a.Delay();
  a.Wait();
  a.Node("branch", "branch.type",
         {DIn("condition"), CIn("branch.in"), COut("branch.true"),
          COut("branch.false")});
  a.Node("repeat", "repeat.type",
         {DIn("count"), DOut("index"), CIn("repeat.in"), COut("body"),
          COut("done")});
  a.Node("spawn", "native.type",
         {DIn("spawn.arg"), DOut("spawn.result"), CIn("spawn.in"),
          COut("spawn.next")});
  a.Edge("payload", "condition");
  a.Edge("payload", "count");
  a.Edge("index", "spawn.arg");
  a.Edge("spawn.result", "wait.value");
  a.Edge("event.next", "branch.in");
  a.Edge("branch.true", "repeat.in");
  a.Edge("body", "spawn.in");
  a.Edge("spawn.next", "wait.in");
  return std::move(a.definition);
}
std::vector<gc::NodeBinding> Bindings(gc::DomainOperation invoke) {
  return {{Id("event.type"), graph::Operation::EventEntry, {}},
          {Id("input.type"), graph::Operation::Input, {}},
          {Id("native.type"), graph::Operation::NativeCall, std::move(invoke)},
          {Id("wait.type"), graph::Operation::Wait, {}},
          {Id("branch.type"), graph::Operation::Branch, {}},
          {Id("repeat.type"), graph::Operation::Repeat, {}}};
}
graph::ExecutionStatus
Status(const std::shared_ptr<comp::IGraphActivation> &activation) {
  const auto native =
      std::dynamic_pointer_cast<gc::GraphActivation>(activation);
  if (!native)
    throw std::runtime_error("unexpected activation type");
  const auto &report = native->Report();
  if (!report)
    throw std::runtime_error("missing activation report");
  return report->status;
}
void Run() {
  fw::entities::EntityService entities;
  fw::entities::EntityArchetypeDefinition archetype;
  archetype.canonical_name = "example.enemy";
  const auto enemy_type = Take(entities.RegisterArchetype(archetype));
  entities.Freeze();
  comp::ComponentRegistry registry;
  const auto health = Take(registry.Register<Health>("example.health"));
  const auto movement = Take(registry.Register<Movement>("example.movement"));
  const auto points = Take(registry.RegisterProperty(
      health, "points", &Health::points, &HealthValid));
  const auto speed = Take(registry.RegisterProperty(
      movement, "speed", &Movement::speed, &SpeedValid));
  registry.Freeze();
  comp::ComponentStore components{entities, registry};
  HeadlessBackend backend;
  rb::RuntimeBridgeService bridge{backend};
  auto time = Take(CreateTimeServices());
  simulation::SimulationDependencies dependencies;
  dependencies.clock = std::make_shared<Clock>(time.clock);
  auto simulation =
      Take(simulation::CreateSimulationServices({}, dependencies));
  simulation.scheduler->SetBudget({16, 512});
  GraphScheduler scheduler{time.clock, simulation, SimulationZoneId{1}};
  gc::GraphExecutionHost host{scheduler, bridge};
  comp::GraphBehaviorService behaviors{host};
  comp::PrefabComposer prefabs{entities, registry, components, behaviors};
  std::size_t damage_steps = 0;
  const auto damage =
      Bindings([&](const gc::OperationContext &context, std::int64_t amount) {
        if (amount < 0)
          return foundation::Result<std::int64_t>::Failure(
              foundation::Error::Create("example.invalid_damage",
                                        "negative damage"));
        const auto entity = fw::entities::EntityService::FromGameplayObjectRef(
            context.scope.Object());
        const auto component = components.Find(entity, health);
        if (!component)
          return foundation::Result<std::int64_t>::Failure(
              foundation::Error::Create("example.health_missing",
                                        "Health required"));
        Require(context.bridge.GetBinding(context.scope.Object()).has_value(),
                "RuntimeBridge binding missing");
        const auto current =
            Take(components.Read<std::int64_t>(*component, points));
        const auto written =
            components.WriteProperty(*component, {points, current - amount});
        if (!written)
          return foundation::Result<std::int64_t>::Failure(written.GetError());
        ++damage_steps;
        return foundation::Result<std::int64_t>::Success(amount);
      });
  const auto enemy_asset = Take(host.RegisterGraph(EnemyGraph(), damage));
  comp::GraphBehaviorDefinition enemy_behavior{
      Id("enemy.behavior"),
      enemy_asset,
      {{Id("enemy.hit"), Id("event.in")}},
      {{Id("delay.value"), 1}}};
  comp::PrefabDefinition enemy{Id("example.enemy.prefab"),
                               {{Id("health.parameter"), std::int64_t{100}}},
                               {}};
  enemy.root = {
      Id("enemy.root"),
      enemy_type,
      {{health, {}, {{points, std::int64_t{0}, Id("health.parameter")}}, {}},
       {movement, {}, {{speed, 1.0, {}}}, {}}},
      {enemy_behavior},
      {}};
  const auto prefab = Take(prefabs.Register(enemy));
  std::vector<std::shared_ptr<const comp::PrefabInstance>> spawned;
  const auto spawn = Bindings([&](const gc::OperationContext &context,
                                  std::int64_t index) {
    Require(context.scope.Kind() == comp::GraphScopeKind::World &&
                !context.scope.Object().IsValid(),
            "WaveDirector must not be an Entity");
    comp::PrefabOverrides overrides;
    overrides.parameters.push_back(
        {Id("health.parameter"), std::int64_t{100 + index * 50}});
    overrides.components.push_back({Id("enemy.root"),
                                    movement,
                                    {},
                                    {speed, 1.0 + static_cast<double>(index)}});
    auto instance = Take(prefabs.Spawn(prefab, overrides));
    const auto &root = instance->Objects().front();
    Require(bridge
                .Enqueue(rb::MaterializeRequest{
                    root.scope->Object(),
                    rb::RuntimePersistentObjectHandle{
                        static_cast<std::uint64_t>(index + 1)}})
                .HasValue(),
            "materialize enemy");
    (void)bridge.Process();
    // Dispatch during a native callback is accepted by the Game adapter and
    // submitted at the next Prepare; Runtime's reentrancy guard is preserved.
    (void)Take(
        behaviors.Send(root.behaviors.front().handle, {Id("enemy.hit"), 10}));
    spawned.push_back(std::move(instance));
    return foundation::Result<std::int64_t>::Success(index);
  });
  const auto wave_asset = Take(host.RegisterGraph(WaveGraph(), spawn));
  auto director = Take(comp::SystemInstance::Create(
      comp::GraphScopeKind::World,
      fw::GameplayObjectId::FromString("example.world"), Id("wave.director")));
  const auto wave = Take(
      behaviors.Attach(director->Scope(), {Id("wave.behavior"),
                                           wave_asset,
                                           {{Id("wave.start"), Id("event.in")}},
                                           {{Id("delay.value"), 1}}}));
  const auto activation = Take(behaviors.Send(wave, {Id("wave.start"), 3}));
  const auto tick = [&] {
    Require(host.Prepare().HasValue(), "prepare");
    Require(Take(simulation.runtime->Tick()).failures.empty(),
            "simulation tick");
  };
  for (int step = 0; step < 5; ++step) {
    tick();
    Require(time.runtime->Skip(GameDuration{1}).HasValue(), "advance time");
  }
  Require(spawned.size() == 3 && damage_steps == 6 &&
              Status(activation) == graph::ExecutionStatus::Completed,
          "wave and per-enemy continuations");
  for (std::size_t index = 0; index < spawned.size(); ++index) {
    const auto &root = spawned[index]->Objects().front();
    const auto hp = components.Find(root.entity, health),
               move = components.Find(root.entity, movement);
    if (!hp || !move)
      throw std::runtime_error("missing spawned component");
    Require(spawned[index]->Definition() == spawned[0]->Definition(),
            "shared definition snapshot");
    Require(Take(components.Read<std::int64_t>(*hp, points)) ==
                80 + static_cast<std::int64_t>(index) * 50,
            "independent Health");
    Require(Take(components.Read<double>(*move, speed)) ==
                1.0 + static_cast<double>(index),
            "instance Movement override");
  }
  const auto &root = spawned[0]->Objects().front();
  auto waiting =
      Take(behaviors.Send(root.behaviors.front().handle, {Id("enemy.hit"), 5}));
  tick();
  Require(Status(waiting) == graph::ExecutionStatus::Waiting &&
              damage_steps == 7,
          "enemy pauses before destruction");
  const auto binding = bridge.GetBinding(root.scope->Object());
  Require(prefabs.Destroy(*spawned[0]).HasValue(),
          "destroy prefab through Entity lifecycle");
  tick();
  Require(Status(waiting) == graph::ExecutionStatus::Cancelled &&
              damage_steps == 7,
          "destruction cancels suffix");
  if (binding) {
    Require(bridge
                .Enqueue(rb::DestroyRuntimeRequest{root.scope->Object(),
                                                   binding->generation})
                .HasValue(),
            "destroy representation");
    (void)bridge.Process();
  }
  (void)Take(entities.CommitPendingDestruction());
  components.Synchronize();
  behaviors.Synchronize();
  Require(entities.FindByLifecycle(fw::entities::EntityLifecycleState::Alive)
                  .size() == 2,
          "remaining enemies survive");
  std::puts("PASS: WaveDirector spawned 3 Entity enemies with Health + "
            "Movement + GraphBehavior; shared Prefab definition, isolated "
            "overrides; Wait/resume and Entity cancellation verified.");
}
} // namespace
int main() {
  try {
    Run();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
  return 0;
}
