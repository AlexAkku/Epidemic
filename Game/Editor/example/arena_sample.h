#pragma once
#include "demo_game.h"
#include <bit>

namespace epidemic::game::editor::example {
// Ordinary persisted documents, also installed by the UI's sample action.
// No sample-specific execution loop: all behavior uses native registered ops.
inline ProjectDefinition Arena(const Schema &schema) {
    const auto bullet = AssetId::FromString("arena.projectile"),
               enemy = AssetId::FromString("arena.enemy"),
               turret = AssetId::FromString("arena.turret"),
               spawner = AssetId::FromString("arena.spawner");
    const auto component =
        [](std::string_view type,
           std::initializer_list<std::pair<std::string_view, comp::PropertyValue>> values) {
            comp::PrefabComponent c{gameplay::TypeId::FromString(type), {}, {}, {}};
            for (const auto &[name, value] : values)
                c.properties.push_back({Id(name), value, {}});
            return c;
        };
    const auto prefab = [&](AssetId id, std::vector<comp::PrefabComponent> components) {
        return comp::PrefabDefinition{id.value,
                                      {},
                                      {Id(std::to_string(id.Raw()) + ".root"),
                                       schema.enemy,
                                       std::move(components),
                                       {},
                                       {}}};
    };
    auto enemy_definition =
        prefab(enemy,
               {component("editor.example.health",
                          {{"points", std::int64_t{30}}, {"maximum", std::int64_t{30}}}),
                component("editor.example.movement",
                          {{"speed", .8}, {"enabled", true}, {"label", std::string{"Enemy"}}}),
                component("gameplay.motion2d", {{"x", -1.}, {"y", 0.}}),
                component("gameplay.collider2d", {{"shape", std::int64_t{1}},
                                                  {"radius", .3},
                                                  {"layer", std::int64_t{2}},
                                                  {"mask", std::int64_t{5}}})});
    auto bullet_definition =
        prefab(bullet, {component("gameplay.projectile", {{"speed", 5.},
                                                          {"damage", std::int64_t{10}},
                                                          {"lifetime", std::int64_t{120}}}),
                        component("gameplay.collider2d", {{"shape", std::int64_t{1}},
                                                          {"radius", .12},
                                                          {"layer", std::int64_t{4}},
                                                          {"mask", std::int64_t{2}},
                                                          {"trigger", true}})});
    auto turret_definition =
        prefab(turret,
               {component("editor.example.health", {{"points", std::int64_t{100}}}),
                component("editor.example.movement", {{"speed", 2.},
                                                      {"enabled", true},
                                                      {"label", std::string{"Player/Turret"}}}),
                component("gameplay.motion2d", {{"player", true}}),
                component("gameplay.collider2d",
                          {{"layer", std::int64_t{1}}, {"mask", std::int64_t{2}}}),
                component("gameplay.turret",
                          {{"range", 6.},
                           {"cooldown", std::int64_t{15}},
                           {"projectile", std::bit_cast<std::int64_t>(bullet.Raw())}}),
                component("gameplay.state", {}), component("gameplay.inputbindings", {})});
    auto spawner_definition =
        prefab(spawner, {component("gameplay.wavespawner",
                                   {{"prefab", std::bit_cast<std::int64_t>(enemy.Raw())},
                                    {"count", std::int64_t{3}},
                                    {"interval", std::int64_t{30}},
                                    {"waves", std::int64_t{2}},
                                    {"enabled", true}}),
                         component("gameplay.state", {})});
    using K = graph::PortKind;
    using D = graph::PortDirection;
    graph::GraphDefinition graph{AssetId::FromString("arena.graph"), "Arena Wave Graph", {}, {}};
    const auto node =
        [&](const char *id, const char *type,
            std::initializer_list<std::tuple<const char *, K, D, std::int64_t>> ports) {
            graph::NodeDefinition n{Id(id), Id(type), id, {}};
            for (const auto &[name, kind, direction, value] : ports) {
                auto p = Port(std::string(id) + "." + name, kind, direction);
                p.name = name;
                if (kind == K::Data && direction == D::Input)
                    p.default_value = value;
                n.ports.push_back(p);
            }
            graph.nodes.push_back(std::move(n));
        };
    node("Start", "event.type",
         {{"event", K::Event, D::Input, 0},
          {"payload", K::Data, D::Output, 0},
          {"next", K::Control, D::Output, 0}});
    node("Branch", "branch.type",
         {{"condition", K::Data, D::Input, 1},
          {"in", K::Control, D::Input, 0},
          {"true", K::Control, D::Output, 0},
          {"false", K::Control, D::Output, 0}});
    node("Repeat", "repeat.type",
         {{"count", K::Data, D::Input, 3},
          {"iteration", K::Data, D::Output, 0},
          {"in", K::Control, D::Input, 0},
          {"body", K::Control, D::Output, 0},
          {"done", K::Control, D::Output, 0}});
    node("Spawn", "gameplay.entity.spawn.v1",
         {{"argument", K::Data, D::Input, std::bit_cast<std::int64_t>(enemy.Raw())},
          {"result", K::Data, D::Output, 0},
          {"in", K::Control, D::Input, 0},
          {"next", K::Control, D::Output, 0}});
    node("Wait", "wait.type",
         {{"value", K::Data, D::Input, 0},
          {"ticks", K::Data, D::Input, 10},
          {"result", K::Data, D::Output, 0},
          {"in", K::Control, D::Input, 0},
          {"next", K::Control, D::Output, 0}});
    const auto edge = [&](const char *a, const char *b) {
        graph.connections.push_back({Id(std::string(a) + "/" + b), Id(a), Id(b)});
    };
    graph::GraphReference target{graph::ReferenceKind::SceneEntity,
                                 AssetId::FromString("arena.scene"),
                                 Id("unused"),
                                 enemy,
                                 enemy_definition.root.id,
                                 {},
                                 {}};
    target.placement = foundation::StringId{2};
    graph.nodes.push_back(
        {Id("Enemy Reference"),
         graph::kReferenceNodeType,
         "Enemy Reference",
         {{Id("enemy.reference"), "reference", K::Data, D::Output, graph::kEntityReferenceType}},
         target});
    graph.nodes.push_back(
        {Id("Enemy Valid"),
         graph::ReferenceCheckType(graph::kEntityReferenceType),
         "Is Enemy Valid",
         {{Id("enemy.valid.input"), "reference", K::Data, D::Input, graph::kEntityReferenceType},
          {Id("enemy.valid.output"), "valid", K::Data, D::Output, graph::kI64Type}}});
    edge("enemy.reference", "enemy.valid.input");
    edge("enemy.valid.output", "Branch.condition");
    edge("Start.next", "Branch.in");
    edge("Branch.true", "Repeat.in");
    edge("Repeat.body", "Spawn.in");
    edge("Spawn.next", "Wait.in");
    turret_definition.root.behaviors.push_back(
        {Id("arena.start"), graph.asset_id.value, {{Id("Start.event"), Id("Start.event")}}, {}});
    gc::SceneCompositionDefinition scene;
    scene.asset_id = AssetId::FromString("arena.scene");
    for (auto [id, prefab_id, x] : {std::tuple{1ULL, turret, 0.f}, std::tuple{2ULL, enemy, 2.f},
                                    std::tuple{3ULL, spawner, 4.f}}) {
        SceneNodeSnapshot n;
        n.id = SceneNodeId{id};
        n.local_transform.position.x = x;
        scene.scene.nodes.push_back(n);
        scene.placements.push_back({n.id, prefab_id, {}});
        scene.node_names[id] = id == 1 ? "Player / Turret" : id == 2 ? "Enemy" : "Wave spawn point";
    }
    return {Id("FII_Arena"),
            scene.asset_id,
            {{scene.asset_id, "FII Arena", 0, scene},
             {turret, "Arena Player Turret", 0, turret_definition},
             {enemy, "Arena Enemy", 0, enemy_definition},
             {bullet, "Arena Projectile", 0, bullet_definition},
             {spawner, "Arena Spawner", 0, spawner_definition},
             {graph.asset_id, "Arena Wave Graph", 0, graph}}};
}
} // namespace epidemic::game::editor::example
