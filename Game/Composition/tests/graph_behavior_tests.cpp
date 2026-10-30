#include "user_components.h"

using namespace composition_tests;

void GraphBehaviors() {
    Fixture f;
    ComponentsFixture types;
    comp::ComponentStore components{f.entities, types.registry};

    comp::GraphBehaviorService behaviors{f.host};

    const auto first = f.Entity(), second = f.Entity();
    const auto hp = Take(components.Attach(first, types.health));
    const auto other = Take(components.Attach(second, types.health));
    auto definition = Definition();
    definition.nodes.push_back({Id("delay"),
                                Id("input.type"),
                                "Delay",
                                {{Id("delay.value"), "Delay", graph::PortKind::Data,
                                  graph::PortDirection::Output, graph::kI64Type}}});
    for (auto &edge : definition.connections)
        if (edge.input_port == Id("wait.ticks"))
            edge.output_port = Id("delay.value");
    const auto actions = Bindings([&](const gc::OperationContext &context, std::int64_t value) {
        const auto entity =
            fw::entities::EntityService::FromGameplayObjectRef(context.scope.Object());
        const auto health = components.Find(entity, types.health);
        if (!health)
            return foundation::Result<std::int64_t>::Failure(
                foundation::Error::Create("game.health_missing", "health required"));
        const auto current = Take(components.Read<std::int64_t>(*health, types.points));
        const auto changed = components.WriteProperty(*health, {types.points, current - value});
        if (!changed)
            return foundation::Result<std::int64_t>::Failure(changed.GetError());
        return foundation::Result<std::int64_t>::Success(value);
    });
    std::vector<gc::NodeBinding> bindings{actions.begin(), actions.end()};

    bindings.push_back({Id("input.type"), graph::Operation::Input, {}});
    const auto graph_asset = Take(f.host.RegisterGraph(definition, bindings));
    comp::GraphBehaviorDefinition behavior{Id("enemy.behavior"),
                                           graph_asset,
                                           {{Id("game.damage"), Id("event.in")}},
                                           {{Id("delay.value"), 1}}};

    const auto first_behavior =
        Take(behaviors.Attach(Take(gc::GraphScope::ForEntity(f.entities, first)), behavior));
    const auto second_behavior = Take(behaviors.Attach(Take(components.ScopeOf(other)), behavior));
    behavior.events[0].port =
        Id("mutated.authoring.port"); // An attached behavior owns its definition snapshot.
    definition.nodes.clear(); // Registered GraphDefinition is also isolated from authoring edits.
    auto activation = Take(behaviors.Send(first_behavior, {Id("game.damage"), 10}));
    f.Tick();
    Check(Take(components.Read<std::int64_t>(hp, types.points)) == 90 &&
              Take(components.Read<std::int64_t>(other, types.points)) == 100,
          "event targets only its object's graph");
    f.Advance();
    f.Tick();
    Check(Status(activation) == graph::ExecutionStatus::Completed &&
              Take(components.Read<std::int64_t>(hp, types.points)) == 80,
          "configured Wait resumes the domain operation");
    const auto paused = Take(behaviors.Send(first_behavior, {Id("game.damage"), 0}));
    f.Tick();
    Check(f.entities.Deactivate(first).HasValue(), "deactivate graph owner");
    behaviors.Synchronize();
    components.Synchronize();
    f.Tick();
    Check(Status(paused) == graph::ExecutionStatus::Cancelled &&
              Take(components.Read<std::int64_t>(hp, types.points)) == 80,
          "Dormant cancels execution without dropping components or behavior configuration");
    ErrorIs(behaviors.Send(first_behavior, {Id("game.damage"), 1}), "behavior.owner_inactive");
    Check(f.entities.Activate(first).HasValue(), "activate graph owner");
    const auto reactivated = Take(behaviors.Send(first_behavior, {Id("game.damage"), 0}));
    f.Tick();
    f.Advance();
    f.Tick();
    Check(Status(reactivated) == graph::ExecutionStatus::Completed,
          "same behavior handle receives a fresh event after reactivation");
    const auto waiting = Take(behaviors.Send(second_behavior, {Id("game.damage"), 1}));
    f.Tick();
    Check(!waiting->IsFinished(), "component graph waits");
    Check(components.Remove(other).HasValue(), "remove behavior's component");
    f.Tick();
    Check(Status(waiting) == graph::ExecutionStatus::Cancelled,
          "component removal cancels active graph");
    ErrorIs(behaviors.Send(second_behavior, {Id("game.damage"), 1}), "behavior.stale_handle");
    ErrorIs(behaviors.Send(first_behavior, {Id("game.unknown"), 1}), "behavior.event_unbound");
    const auto pending = Take(behaviors.Send(first_behavior, {Id("game.damage"), 1}));
    Check(behaviors.Detach(first_behavior).HasValue(), "detach behavior");
    f.Tick();
    Check(Status(pending) == graph::ExecutionStatus::Cancelled &&
              Take(components.Read<std::int64_t>(hp, types.points)) == 80,
          "detach cancels before first effect without destroying Entity");
    ErrorIs(behaviors.Attach(Take(gc::GraphScope::ForEntity(f.entities, first)), behavior),
            "behavior.invalid_routes");
    behavior.events[0].port = Id("event.in");
    behavior.inputs[0].port = Id("payload");
    ErrorIs(behaviors.Attach(Take(gc::GraphScope::ForEntity(f.entities, first)), behavior),
            "behavior.invalid_inputs");
    behavior.inputs[0].port = Id("delay.value");
    const auto routed =
        Take(behaviors.Attach(Take(gc::GraphScope::ForEntity(f.entities, first)), behavior));
    auto target = Take(gc::GraphScope::ForEntity(f.entities, first));
    Check(Take(behaviors.SendTo(*target, {Id("game.damage"), 1})) == 1,
          "explicit gameplay event fanout uses target context");
    Check(behaviors.Detach(routed).HasValue(), "discard queued final fixture event");
    f.Tick();
    behavior.inputs.clear();
    ErrorIs(behaviors.Attach(target, behavior), "behavior.missing_input");
}
