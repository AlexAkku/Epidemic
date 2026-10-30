#include "user_components.h"

using namespace composition_tests;

void Components() {
    Fixture f;
    ComponentsFixture types;
    comp::ComponentStore components{f.entities, types.registry};

    const auto first = f.Entity(), second = f.Entity();
    const auto hp = Take(components.Attach(first, types.health));
    const std::array properties{comp::PropertyAssignment{types.speed, 2.5},
                                comp::PropertyAssignment{types.label, std::string("fast")}};

    const auto movement = Take(components.Attach(first, types.movement, properties));
    const auto other_hp = Take(components.Attach(second, types.health));
    Check(Take(components.Get<Health>(hp))->points == 100 &&
              Take(components.Read<double>(movement, types.speed)) == 2.5,
          "user components without engine changes");
    Check(components.WriteProperty(hp, {types.points, std::int64_t{30}}).HasValue() &&
              Take(components.Read<std::int64_t>(other_hp, types.points)) == 100,
          "independent component state");
    ErrorIs(components.Get<Movement>(hp), "component.type_mismatch");
    ErrorIs(components.Read<double>(hp, types.points), "component.property_type");
    ErrorIs(components.WriteProperty(hp, {types.points, 1.0}), "component.property_type");
    ErrorIs(components.WriteProperty(hp, {types.points, std::int64_t{-1}}),
            "component.value_rejected");
    Check(Take(components.Read<std::int64_t>(hp, types.points)) == 30,
          "rejected property did not mutate state");
    ErrorIs(components.Attach(first, types.health), "component.duplicate");
    ErrorIs(types.registry.Register<Health>("game.late"), "component.registry_frozen");
    const auto *info = types.registry.Find(types.movement);
    Check(info && info->properties.size() == 2 &&
              info->properties[0].kind == comp::PropertyKind::Real,
          "typed property metadata");
    const auto part = fw::entities::EntityPartId::FromString("game.body");
    const auto part_hp = Take(components.Attach(first, types.health, {}, part));
    Check(part_hp.part == part, "components extend existing Parts");
    Check(f.entities.Deactivate(second).HasValue(), "deactivate existing Entity");
    components.Synchronize();
    Check(Take(components.Read<std::int64_t>(other_hp, types.points)) == 100,
          "Dormant preserves component state");
    Check(components.Find(second, types.health) == other_hp,
          "Dormant component remains discoverable");
    Check(components.WriteProperty(other_hp, {types.points, std::int64_t{99}}).HasValue(),
          "Dormant state remains mutable under EntityService rules");
    Check(f.entities.Activate(second).HasValue(), "reactivate existing Entity");
    auto old_scope = Take(components.ScopeOf(hp));
    Check(components.Remove(hp).HasValue() && !old_scope->IsAlive(),
          "removing component invalidates its lifetime scope");
    const auto replaced = Take(components.Attach(first, types.health));
    ErrorIs(components.Read<std::int64_t>(hp, types.points), "component.stale_handle");
    Check(replaced.generation != hp.generation,
          "stale component generation cannot alias a replacement");
    Check(f.entities.RequestDestroy(first, fw::entities::EntityDestroyReason::Destroyed).HasValue(),
          "destroy Entity");
    components.Synchronize();
    ErrorIs(components.Get<Health>(replaced), "component.stale_handle");
    Check(Take(components.Get<Health>(other_hp))->points == 99, "unrelated Entity preserved");
    comp::ComponentRegistry invalid_registry;
    const auto null_type = Take(
        invalid_registry.Register<Health>("game.null", [] { return std::shared_ptr<Health>{}; }));
    invalid_registry.Freeze();
    comp::ComponentStore invalid{f.entities, invalid_registry};

    ErrorIs(invalid.Attach(second, null_type), "component.invalid_factory");
    const std::array bad{comp::PropertyAssignment{types.points, std::int64_t{-2}}};

    ErrorIs(components.Attach(second, types.health, bad, part), "component.value_rejected");
    Check(components.Attach(second, types.health, {}, part).HasValue(),
          "failed initialization did not publish a partial component");
}
