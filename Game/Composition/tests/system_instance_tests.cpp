#include "Epidemic/GameFramework/Integration/system_instance.h"
#include "test_support.h"

using namespace composition_tests;
namespace comp = epidemic::gameplay::composition;
void Systems() {
  Fixture f;
  comp::GraphBehaviorService behaviors{f.host};
  bool scene_alive = true;
  const auto scene = fw::GameplayObjectId::FromString("scene.test");
  auto first = Take(comp::SystemInstance::Create(comp::GraphScopeKind::Scene,
                                                 scene, Id("wave"),
                                                 [&] { return scene_alive; }));
  auto second = Take(comp::SystemInstance::Create(
      comp::GraphScopeKind::World,
      fw::GameplayObjectId::FromString("world.test"), Id("wave")));
  std::vector<comp::GraphScopeKind> calls;
  const auto bindings =
      Bindings([&](const gc::OperationContext &context, std::int64_t value) {
        Check(!context.scope.Object().IsValid() &&
                  !context.scope.Handle().IsValid(),
              "system is not an artificial Entity");
        calls.push_back(context.scope.Kind());
        return foundation::Result<std::int64_t>::Success(value);
      });
  const auto graph_asset = Take(f.host.RegisterGraph(Definition(), bindings));
  comp::GraphBehaviorDefinition definition{Id("wave.behavior"),
                                           graph_asset,
                                           {{Id("wave.start"), Id("event.in")}},
                                           {}};
  const auto a = Take(behaviors.Attach(first->Scope(), definition));
  const auto b = Take(behaviors.Attach(second->Scope(), definition));
  auto active_a = Take(behaviors.Send(a, {Id("wave.start"), 1}));
  auto active_b = Take(behaviors.Send(b, {Id("wave.start"), 1}));
  f.Tick();
  Check(calls.size() == 2 && f.entities.AllEntities().empty(),
        "two standalone systems run without Entity allocation");
  scene_alive = false;
  f.Advance();
  f.Tick();
  Check(Status(active_a) == graph::ExecutionStatus::Cancelled &&
            Status(active_b) == graph::ExecutionStatus::Completed &&
            calls.size() == 3,
        "scene lifecycle cancels only its own system");
  auto pending = Take(behaviors.Send(b, {Id("wave.start"), 1}));
  second.reset();
  f.Tick();
  Check(Status(pending) == graph::ExecutionStatus::Cancelled &&
            calls.size() == 3,
        "system destruction cancels before native effects");
  auto replacement = Take(comp::SystemInstance::Create(
      comp::GraphScopeKind::Scene, scene, Id("same.name")));
  auto sibling = Take(comp::SystemInstance::Create(comp::GraphScopeKind::Scene,
                                                   scene, Id("same.name")));
  (void)Take(behaviors.Attach(replacement->Scope(), definition));
  (void)Take(behaviors.Attach(sibling->Scope(), definition));
  Check(Take(behaviors.SendTo(*replacement->Scope(), {Id("wave.start"), 0})) ==
            1,
        "distinct system instances never share event target state");
  replacement->Stop();
  f.Tick();
  Check(calls.size() == 3 && sibling->Scope()->IsAlive(),
        "independent lifetime despite shared Scene and definition");
  ErrorIs(comp::SystemInstance::Create(comp::GraphScopeKind::Entity, scene,
                                       Id("invalid")),
          "composition.invalid_system");
}
