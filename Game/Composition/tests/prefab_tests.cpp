#include "Epidemic/GameFramework/Integration/prefab.h"
#include "user_components.h"

using namespace composition_tests;
namespace {
class FailSecondBinding final : public comp::IGraphBehaviorHost {
public:
  struct Program final : comp::IGraphProgram {};
  foundation::Result<std::shared_ptr<const comp::IGraphProgram>>
  Bind(const std::shared_ptr<comp::GraphScope> &,
       const comp::GraphBehaviorDefinition &) override {
    using Result =
        foundation::Result<std::shared_ptr<const comp::IGraphProgram>>;
    if (++calls_ == 2)
      return Result::Failure(foundation::Error::Create(
          "fixture.bind_failure", "injected binding failure"));
    return Result::Success(std::make_shared<Program>());
  }
  foundation::Result<std::shared_ptr<comp::IGraphActivation>>
  Dispatch(const std::shared_ptr<const comp::IGraphProgram> &,
           const comp::GameEvent &) override {
    return foundation::Result<std::shared_ptr<comp::IGraphActivation>>::Failure(
        foundation::Error::Create("fixture.unused", "no event expected"));
  }

private:
  int calls_ = 0;
};
} // namespace
void Prefabs() {
  Fixture f;
  ComponentsFixture types;
  comp::ComponentStore components{f.entities, types.registry};
  comp::GraphBehaviorService behaviors{f.host};
  comp::PrefabComposer prefabs{f.entities, types.registry, components,
                               behaviors};
  const auto bindings =
      Bindings([](const gc::OperationContext &, std::int64_t value) {
        return foundation::Result<std::int64_t>::Success(value);
      });
  auto timed = Definition();
  timed.asset_id = AssetId::FromString("game.prefab.timed");
  timed.nodes.push_back({Id("delay"),
                         Id("input.type"),
                         "Delay",
                         {{Id("delay.value"), "Delay", graph::PortKind::Data,
                           graph::PortDirection::Output, graph::kI64Type}}});
  for (auto &edge : timed.connections)
    if (edge.input_port == Id("wait.ticks"))
      edge.output_port = Id("delay.value");
  std::vector<gc::NodeBinding> timed_bindings{bindings.begin(), bindings.end()};
  timed_bindings.push_back({Id("input.type"), graph::Operation::Input, {}});
  const auto asset = Take(f.host.RegisterGraph(timed, timed_bindings));
  comp::GraphBehaviorDefinition behavior{Id("enemy.behavior"),
                                         asset,
                                         {{Id("game.hit"), Id("event.in")}},
                                         {{Id("delay.value"), 1}}};
  comp::PrefabDefinition definition{
      Id("prefab.enemy"), {{Id("max.health"), std::int64_t{100}}}, {}};
  definition.root.id = Id("root");
  definition.root.archetype = f.archetype;
  definition.root.components = {
      {types.health,
       {},
       {{types.points, std::int64_t{0}, Id("max.health")}},
       {behavior}},
      {types.movement, {}, {{types.speed, 2.0, {}}}, {}}};
  definition.root.children.push_back(
      {Id("child"), f.archetype, {{types.health, {}, {}, {}}}, {}, {}});
  const auto prefab = Take(prefabs.Register(definition));
  definition.root.components
      .clear(); // Snapshot is independent from authoring storage.
  const auto first = Take(prefabs.Spawn(prefab));
  comp::PrefabOverrides overrides;
  overrides.parameters.push_back({Id("max.health"), std::int64_t{250}});
  overrides.components.push_back(
      {Id("root"), types.movement, {}, {types.speed, 4.0}});
  overrides.components.push_back(
      {Id("child"), types.health, {}, {types.points, std::int64_t{75}}});
  overrides.behaviors.push_back(
      {Id("root"), types.health, {}, behavior.id, {Id("delay.value"), 2}});
  const auto second = Take(prefabs.Spawn(prefab, overrides));
  Check(first->Overrides().parameters.empty() &&
            std::get<std::int64_t>(
                second->Overrides().parameters.front().value) == 250,
        "instance retains its own authoring overrides separately from live "
        "state");
  Check(first->Definition() == second->Definition() &&
            first->Objects().size() == 2,
        "shared immutable definition, nested objects");
  const auto *a = first->Find(Id("root"));
  const auto *b = second->Find(Id("root"));
  const auto *child = second->Find(Id("child"));
  if (!a || !b || !child)
    throw std::runtime_error("missing prefab node");
  Check(child->parent == Id("root") && a->entity != b->entity &&
            a->handle != b->handle,
        "independent existing Entity identities");
  const auto health_a = components.Find(a->entity, types.health),
             health_b = components.Find(b->entity, types.health);
  const auto movement_a = components.Find(a->entity, types.movement),
             movement_b = components.Find(b->entity, types.movement);
  if (!health_a || !health_b || !movement_a || !movement_b)
    throw std::runtime_error("missing prefab components");
  Check(Take(components.Read<std::int64_t>(*health_a, types.points)) == 100 &&
            Take(components.Read<std::int64_t>(*health_b, types.points)) == 250,
        "typed prefab parameters");
  Check(Take(components.Read<double>(*movement_a, types.speed)) == 2.0 &&
            Take(components.Read<double>(*movement_b, types.speed)) == 4.0,
        "instance property override");
  Check(Take(components.Get<Health>(*health_a)) !=
            Take(components.Get<Health>(*health_b)),
        "component objects never alias across instances");
  Check(components.WriteProperty(*health_a, {types.points, std::int64_t{40}})
                .HasValue() &&
            Take(components.Read<std::int64_t>(*health_b, types.points)) == 250,
        "runtime changes are per-instance");
  const auto activation =
      Take(behaviors.Send(a->behaviors.front().handle, {Id("game.hit"), 1}));
  const auto independent =
      Take(behaviors.Send(b->behaviors.front().handle, {Id("game.hit"), 1}));
  f.Tick();
  Check(Status(activation) == graph::ExecutionStatus::Waiting,
        "prefab component GraphBehavior waits");
  Check(prefabs.Destroy(*first).HasValue(),
        "request normal nested Entity destruction");
  f.Tick();
  Check(Status(activation) == graph::ExecutionStatus::Cancelled &&
            b->scope->IsAlive(),
        "destroying one prefab cancels only its own graph");
  f.Advance();
  f.Tick();
  Check(Status(independent) == graph::ExecutionStatus::Waiting,
        "behavior input override preserves independent wait duration");
  f.Advance();
  f.Tick();
  Check(Status(independent) == graph::ExecutionStatus::Completed,
        "overridden behavior resumes at its own deadline");
  const auto before = f.entities.AllEntities().size();
  auto invalid = overrides;
  invalid.parameters[0].value = 1.0;
  ErrorIs(prefabs.Spawn(prefab, invalid), "prefab.parameter_type");
  invalid = overrides;
  invalid.components[0].property.value = -1.0;
  ErrorIs(prefabs.Spawn(prefab, invalid), "component.value_rejected");
  invalid = overrides;
  invalid.components[0].node = Id("unknown.node");
  ErrorIs(prefabs.Spawn(prefab, invalid), "prefab.unknown_override");
  invalid = overrides;
  invalid.components.push_back(invalid.components.front());
  ErrorIs(prefabs.Spawn(prefab, invalid), "prefab.duplicate_override");
  Check(f.entities.AllEntities().size() == before,
        "preflight failures create no Entities");
  auto duplicate = *first->Definition();
  duplicate.id = Id("prefab.duplicate.nodes");
  duplicate.root.children[0].id = duplicate.root.id;
  ErrorIs(prefabs.Register(duplicate), "prefab.invalid_node");
  auto unknown = *first->Definition();
  unknown.id = Id("prefab.unknown.graph");
  unknown.root.components[0].behaviors[0].graph = Id("unknown.graph");
  ErrorIs(prefabs.Register(unknown), "behavior.unknown_graph");
  comp::ComponentRegistry failing_registry;
  const auto failing_type = Take(failing_registry.Register<Health>(
      "game.failing", []() -> std::shared_ptr<Health> {
        throw std::runtime_error("factory");
      }));
  failing_registry.Freeze();
  comp::ComponentStore failing_components{f.entities, failing_registry};
  comp::PrefabComposer failing_prefabs{f.entities, failing_registry,
                                       failing_components, behaviors};
  comp::PrefabDefinition failing{
      Id("prefab.failing"),
      {},
      {Id("failing.root"), f.archetype, {{failing_type, {}, {}, {}}}, {}, {}}};
  const auto bad_prefab = Take(failing_prefabs.Register(failing));
  ErrorIs(failing_prefabs.Spawn(bad_prefab), "component.factory_exception");
  Check(f.entities.AllEntities().size() == before,
        "all factories prepare before publishing any Entity");
  FailSecondBinding failing_host;
  comp::GraphBehaviorService failing_behaviors{failing_host};
  comp::ComponentStore rollback_components{f.entities, types.registry};
  comp::PrefabComposer rollback_prefabs{f.entities, types.registry,
                                        rollback_components, failing_behaviors};
  auto rollback_definition = *first->Definition();
  rollback_definition.id = Id("prefab.rollback");
  rollback_definition.root.children[0].behaviors.push_back(behavior);
  const auto rollback_id = Take(rollback_prefabs.Register(rollback_definition));
  const auto alive_before =
      f.entities.FindByLifecycle(fw::entities::EntityLifecycleState::Alive)
          .size();
  const auto failed_spawn = rollback_prefabs.Spawn(rollback_id);
  ErrorIs(failed_spawn, "fixture.bind_failure");
  Check(failed_spawn.GetError().context.find("rollback=") == std::string::npos,
        "cleanup must not silently fail");
  Check(f.entities.FindByLifecycle(fw::entities::EntityLifecycleState::Alive)
                .size() == alive_before,
        "late bind failure rolls back every created Entity and component");
  const auto unrelated_pending = f.entities.Find(a->entity);
  Check(unrelated_pending &&
            unrelated_pending->lifecycle ==
                fw::entities::EntityLifecycleState::PendingDestroy,
        "rollback never commits unrelated pending destruction");
  behaviors.Synchronize();
  components.Synchronize();
}
