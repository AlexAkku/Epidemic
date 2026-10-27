#pragma once
#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/play_session.h"
#include <stdexcept>
namespace epidemic::game::editor::example {
using namespace epidemic::runtime;
namespace ed = epidemic::game::editor;
namespace comp = epidemic::gameplay::composition;
namespace gc = epidemic::game::composition;
namespace rb = epidemic::gameplay::runtime_bridge;
inline foundation::StringId Id(std::string_view value) { return foundation::StringId::FromString(value); }
template<class T> T Take(foundation::Result<T> result) { if (!result) throw std::runtime_error(result.GetError().code); return std::move(result).Value(); }
struct Health { std::int64_t points = 100; };
struct Movement { double speed = 1; bool enabled = true; std::string label = "Enemy"; };
inline bool ValidHealth(const std::int64_t &value) { return value >= 0 && value <= 10000; }
class Backend final : public rb::IRuntimeBridgeBackend {
  public:
    foundation::Result<rb::RuntimeMaterializationResult> Materialize(rb::RuntimePersistentObjectHandle) override { return foundation::Result<rb::RuntimeMaterializationResult>::Success({rb::RuntimeObjectHandle{next_++}, true}); }
    foundation::Result<void> Dematerialize(rb::RuntimeObjectHandle) override { return foundation::Result<void>::Success(); }
    foundation::Result<void> Destroy(rb::RuntimeObjectHandle) override { return foundation::Result<void>::Success(); }
    foundation::Result<void> ApplyImpulse(rb::RuntimePhysicsBodyHandle, rb::RuntimeVector3) override { return foundation::Result<void>::Success(); }
    foundation::Result<void> ProjectEnvironment(rb::RuntimeRegionHandle, const rb::RuntimeEnvironmentValues &, gameplay::Revision) override { return foundation::Result<void>::Success(); }
    std::vector<rb::RuntimeContactObservation> ConsumeContacts() override { return {}; }
  private:
    std::uint64_t next_ = 1;
};
struct Schema {
    gameplay::entities::EntityService entities;
    comp::ComponentRegistry components;
    gameplay::entities::EntityArchetypeId enemy;
    gameplay::TypeId health;
    comp::PropertyId points;
    gameplay::TypeId movement;
    comp::PropertyId speed, enabled, label;
    Schema() {
        gameplay::entities::EntityArchetypeDefinition type; type.canonical_name = "editor.example.enemy";
        enemy = Take(entities.RegisterArchetype(type)); entities.Freeze();
        health = Take(components.Register<Health>("editor.example.health"));
        points = Take(components.RegisterProperty(health, "points", &Health::points, &ValidHealth));
        movement = Take(components.Register<Movement>("editor.example.movement"));
        speed = Take(components.RegisterProperty(movement, "speed", &Movement::speed));
        enabled = Take(components.RegisterProperty(movement, "enabled", &Movement::enabled));
        label = Take(components.RegisterProperty(movement, "label", &Movement::label));
        components.Freeze();
    }
    ed::ProjectSchema View() { return {entities, components, {{Id("event.type"), graph::Operation::EventEntry}, {Id("native.type"), graph::Operation::NativeCall}, {Id("wait.type"), graph::Operation::Wait}}}; }
};
inline graph::PortDefinition Port(std::string_view name, graph::PortKind kind, graph::PortDirection direction) {
    return {Id(name), std::string(name), kind, direction, kind == graph::PortKind::Control ? graph::TypeId{} : graph::kI64Type};
}

inline ed::PlayOptions Options(Schema &schema) {
    ed::PlayOptions options; options.backend = std::make_unique<Backend>();
    options.bindings = [&](ed::PlayContext &context, AssetId) {
        gc::DomainOperation damage = [store = &context.components, health = schema.health, points = schema.points](const gc::OperationContext &operation, std::int64_t amount) {
            using Result = foundation::Result<std::int64_t>;
            const auto entity = gameplay::entities::EntityService::FromGameplayObjectRef(operation.scope.Object());
            const auto component = store->Find(entity, health);
            if (!component) return Result::Failure(foundation::Error::Create("example.health_missing", "Health required"));
            const auto current = store->Read<std::int64_t>(*component, points); if (!current) return Result::Failure(current.GetError());
            if (amount < 0 || amount > current.Value()) return Result::Failure(foundation::Error::Create("example.damage_range", "Damage must be between zero and current Health"));
            const auto changed = store->WriteProperty(*component, {points, current.Value() - amount}); if (!changed) return Result::Failure(changed.GetError());
            return Result::Success(amount);
        };
        return std::vector<gc::NodeBinding>{{Id("event.type"), graph::Operation::EventEntry, {}}, {Id("native.type"), graph::Operation::NativeCall, std::move(damage)}, {Id("wait.type"), graph::Operation::Wait, {}}};
    };

    return options;
}
inline graph::GraphDefinition Graph() {
    using K = graph::PortKind; using D = graph::PortDirection;
    const auto data = [](std::string_view name, D direction) { return Port(name, K::Data, direction); };
    const auto control = [](std::string_view name, D direction) { return Port(name, K::Control, direction); };
    const std::vector<graph::NodeDefinition> nodes{
        {Id("event"), Id("event.type"), "Event", {Port("event.in", K::Event, D::Input), data("payload", D::Output), control("event.next", D::Output)}},
        {Id("before"), Id("native.type"), "Damage", {data("before.arg", D::Input), data("before.result", D::Output), control("before.in", D::Input), control("before.next", D::Output)}},
        {Id("wait"), Id("wait.type"), "Wait", {data("wait.value", D::Input), data("wait.ticks", D::Input), data("wait.result", D::Output), control("wait.in", D::Input), control("wait.next", D::Output)}},
        {Id("after"), Id("native.type"), "Damage again", {data("after.arg", D::Input), data("after.result", D::Output), control("after.in", D::Input), control("after.next", D::Output)}}};

    graph::GraphDefinition graph{AssetId::FromString("example.graph"), "Enemy behavior", nodes, {}};
    const auto edge = [&](std::string_view from, std::string_view to) {
        graph.connections.push_back({Id(std::string(from) + "/" + std::string(to)), Id(from), Id(to)});
    };
    edge("payload", "before.arg"); edge("payload", "wait.ticks"); edge("before.result", "wait.value"); edge("wait.result", "after.arg");
    edge("event.next", "before.in"); edge("before.next", "wait.in"); edge("wait.next", "after.in");

    return graph;
}
inline comp::PrefabNode Enemy(const Schema &schema, comp::PrefabNodeId id = Id("enemy")) {
    return {id, schema.enemy,
        {{schema.health, {}, {{schema.points, std::int64_t{100}, {}}}, {}},
         {schema.movement, {}, {{schema.speed, 1.0, {}}, {schema.enabled, true, {}}, {schema.label, std::string{"Enemy"}, {}}}, {}}},
        {{Id("hit.behavior"), Id("example.graph"), {{Id("hit"), Id("event.in")}}, {}}}, {}};
}
inline ProjectDefinition Definition(const Schema &schema) {
    auto graph = Graph();
    comp::PrefabDefinition prefab{Id("example.prefab"), {}, Enemy(schema)};
    gc::SceneCompositionDefinition scene; scene.asset_id = AssetId::FromString("example.scene");
    SceneNodeSnapshot node; node.id = SceneNodeId{42}; scene.scene.nodes.push_back(node);
    scene.placements.push_back({node.id, AssetId{prefab.id}, {}});
    return {Id("example.project"), scene.asset_id, {{scene.asset_id, "Scene", 0, scene},
        {AssetId{prefab.id}, "Enemy", 0, prefab}, {graph.asset_id, "Enemy behavior", 0, graph}}};
}
} // namespace epidemic::game::editor::example
