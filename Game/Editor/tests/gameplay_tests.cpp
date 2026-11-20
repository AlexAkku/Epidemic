#include "../example/arena_sample.h"
#include <iostream>
#include <cstdio>
#include <limits>
#include <thread>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif
namespace ex = epidemic::game::editor::example;
namespace ed = epidemic::game::editor;
namespace arena = epidemic::game::composition::arena;
using epidemic::foundation::Result;

void Check(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}

template <class T> T Present(const std::optional<T> &value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
    return *value;
}

void FixedInputReplay(ex::Schema &schema) {
    auto definition=ex::Definition(schema);
    auto &scene=std::get<epidemic::game::composition::SceneCompositionDefinition>(definition.documents[0].data);
    auto node=scene.scene.nodes.front();node.id=epidemic::runtime::SceneNodeId{43};scene.scene.nodes.push_back(node);
    auto placement=scene.placements.front();placement.node=node.id;scene.placements.push_back(placement);
    auto root=std::get<epidemic::gameplay::composition::PrefabDefinition>(definition.documents[1].data).root.id;
    auto &graph=std::get<epidemic::runtime::graph::GraphDefinition>(definition.documents[2].data);
    std::erase_if(graph.connections,[](const auto &edge){return edge.input_port==ex::Id("before.arg")||edge.input_port==ex::Id("wait.ticks");});
    for(auto &n:graph.nodes)for(auto &port:n.ports)if(port.id==ex::Id("before.arg")||port.id==ex::Id("wait.ticks"))port.default_value=7;
    auto project=ex::Take(ed::EditorProject::Create("Temp/editor-tests/replay.epidemic",definition,schema.View()));
    std::vector<std::pair<std::int64_t,std::int64_t>> baseline;
    for(int replay=0;replay<8;++replay){
        auto options=ex::Options(schema);options.start_events=replay>=4;
        auto play=ex::Take(project->Play(std::move(options)));
        if(replay<4){ex::Take(play->Send(epidemic::runtime::SceneNodeId{42},{ex::Id("hit"),7}));
            ex::Take(play->Send(epidemic::runtime::SceneNodeId{43},{ex::Id("hit"),7}));}
        std::vector<std::pair<std::int64_t,std::int64_t>> states;
        for(int tick=0;tick<20;++tick){
            if(replay%2&&tick%3==0)std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ex::Take(play->Tick({1}));
            auto a=ex::Take(play->ReadComponentProperty(epidemic::runtime::SceneNodeId{42},root,schema.health,{},schema.points));
            auto b=ex::Take(play->ReadComponentProperty(epidemic::runtime::SceneNodeId{43},root,schema.health,{},schema.points));
            states.emplace_back(std::get<std::int64_t>(a),std::get<std::int64_t>(b));
            Check(states.back().first==states.back().second,"Equal native events within budget diverged across entity ticks without breakpoints");
        }
        Check(states.front().first==93&&states.back().first==86,"Replay did not execute Damage/Wait/Damage");
        if(replay==0)baseline=states;else Check(states==baseline,"Same tick-stamped inputs changed replay when wall pacing changed");
        ex::Take(play->Stop());
    }
    std::cout<<"PASS two-entity fixed-input native replay, 8 explicit/autostart runs with different wall pacing; same-tick damage; no breakpoints\n";
}

void CollisionAndChurn(ex::Schema &schema) {
    auto definition = ex::Arena(schema);
    for (auto &doc : definition.documents)
        if (auto *prefab = std::get_if<ex::comp::PrefabDefinition>(&doc.data))
            for (auto &c : prefab->root.components)
                if (c.type == epidemic::gameplay::TypeId::FromString("gameplay.turret") ||
                    c.type == epidemic::gameplay::TypeId::FromString("gameplay.wavespawner") ||
                    c.type == epidemic::gameplay::TypeId::FromString("gameplay.motion2d")) {
                    auto enabled =
                        std::find_if(c.properties.begin(), c.properties.end(),
                                     [](const auto &p) { return p.property == ex::Id("enabled"); });
                    if (enabled == c.properties.end())
                        c.properties.push_back({ex::Id("enabled"), false, {}});
                    else
                        enabled->value = false;
                }
    auto project = ex::Take(ed::EditorProject::Create("Temp/editor-tests/arena-collision.epidemic",
                                                      definition, schema.View()));
    auto play = ex::Take(project->Play(ex::Options(schema)));
    auto *c = play->Context();
    auto player = play->Instance(epidemic::runtime::SceneNodeId{1})->Objects().front().handle,
         enemy = play->Instance(epidemic::runtime::SceneNodeId{2})->Objects().front().handle,
         spawner = play->Instance(epidemic::runtime::SceneNodeId{3})->Objects().front().handle;
    const auto set = [&](arena::Entity e, const char *name, ex::comp::PropertyValue value) {
        auto entity = Present(c->entities.Resolve(e), "live collider Entity");
        auto component =
            Present(c->components.Find(entity.id, epidemic::gameplay::TypeId::FromString(
                                                      "gameplay.collider2d")),
                    "collider component");
        ex::Take(c->components.WriteProperty(component, {ex::Id(name), value}));
    };
    const auto move = [&](arena::Entity e, float x) {
        auto w = ex::Take(c->WorldTransform(e));
        w.position.x = x;
        ex::Take(c->SetWorldTransform(e, w));
    };
    auto serial = std::uint64_t{0};
    const auto event = [&](const char *name) {
        auto trace = play->RuntimeSnapshot().trace;
        bool found = false;
        for (const auto &v : trace)
            if (v.sequence > serial && v.state == name)
                found = true;
        if (!trace.empty())
            serial = trace.back().sequence;
        return found;
    };
    for (auto [a, b] :
         {std::pair{0LL, 0LL}, std::pair{0LL, 1LL}, std::pair{1LL, 0LL}, std::pair{1LL, 1LL}}) {
        set(player, "shape", static_cast<std::int64_t>(a));
        set(enemy, "shape", static_cast<std::int64_t>(b));
        move(enemy, 0);
        ex::Take(play->Tick({1}));
        Check(event("CollisionEnter"), "AABB/Circle enter combination");
        move(enemy, 3);
        ex::Take(play->Tick({1}));
        Check(event("CollisionExit"), "AABB/Circle exit combination");
    }
    set(enemy, "mask", std::int64_t{0});
    move(enemy, 0);
    ex::Take(play->Tick({1}));
    Check(!event("CollisionEnter"), "mask reject");
    set(enemy, "mask", std::int64_t{5});
    set(enemy, "trigger", true);
    ex::Take(play->Tick({1}));
    Check(event("TriggerEnter"), "trigger enter");
    ex::Take(c->gameplay->Execute("gameplay.entity.destroy.v1", enemy, 0));
    ex::Take(play->Tick({1}));
    Check(event("TriggerExit"), "destroyed other exits safely");
    Check(!c->entities.Resolve(enemy), "stale generation removed");
    Check(!c->gameplay->Execute("gameplay.health.get.v1", enemy, 0),
          "stale gameplay reference rejected");
    auto baseline = c->entities.AllEntities().size();
    auto nodes = c->scene->scene.snapshots->CaptureSnapshot().nodes.size();
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX before{};
    before.cb = sizeof(before);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&before),
                         sizeof(before));
    DWORD handlesBefore = 0;
    GetProcessHandleCount(GetCurrentProcess(), &handlesBefore);
#endif
    for (int i = 0; i < 1000; ++i) {
        ex::Take(
            c->gameplay->Execute("gameplay.entity.spawn.v1", player,
                                 std::bit_cast<std::int64_t>(
                                     epidemic::runtime::AssetId::FromString("arena.enemy").Raw())));
        arena::Entity spawned{};
        for (const auto &[id, link] : c->scene->entities) {
            (void)id;
            if (link.entity != player && link.entity != spawner)
                spawned = link.entity;
        }
        Check(spawned.IsValid(), "spawn has actual Scene binding");
        ex::Take(c->gameplay->Execute("gameplay.entity.destroy.v1", spawned, 0));
        ex::Take(play->Tick({1}));
        Check(c->entities.AllEntities().size() == baseline,
              "spawn/despawn no retained Entity records");
    }
    Check(c->scene->scene.snapshots->CaptureSnapshot().nodes.size() == nodes,
          "no retained Scene nodes");
    Check(c->gameplay->Jobs() == 0, "no retained gameplay jobs");
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX after{};
    after.cb = sizeof(after);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&after),
                         sizeof(after));
    DWORD handlesAfter = 0;
    GetProcessHandleCount(GetCurrentProcess(), &handlesAfter);
    std::cout << "ARENA_CHURN cycles=1000 handles_delta="
              << static_cast<long long>(handlesAfter) - handlesBefore << " private_bytes_delta="
              << static_cast<long long>(after.PrivateUsage) -
                     static_cast<long long>(before.PrivateUsage)
              << '\n';
#endif
    ex::Take(play->Stop());
    std::cout << "PASS collision matrix/trigger/mask/destroy callback/stale generation/1000 "
                 "spawn-despawn\n";
}

void Simulation(ex::Schema &schema) {
    auto definition = ex::Arena(schema);
    auto project = ex::Take(
        ed::EditorProject::Create("Temp/editor-tests/arena.epidemic", definition, schema.View()));
    auto options = ex::Options(schema);
    options.start_events = true;
    auto play = ex::Take(project->Play(std::move(options)));
    auto *context = play->Context();
    auto player = play->Instance(epidemic::runtime::SceneNodeId{1})->Objects().front().handle;
    auto before = ex::Take(context->WorldTransform(player));
    ex::Take(play->SetKey(68, true));
    ex::Take(play->Tick({1}));
    auto after = ex::Take(context->WorldTransform(player));
    Check(after.position.x > before.position.x, "native input movement");
    ex::Take(play->Pause());
    auto tick = play->RuntimeSnapshot().tick;
    ex::Take(play->Pump(std::chrono::seconds(1)));
    Check(play->RuntimeSnapshot().tick == tick, "paused one clock");
    ex::Take(play->StepTick());
    Check(play->RuntimeSnapshot().tick == tick + 1, "one fixed step");
    Check(ex::Take(context->WorldTransform(player)).position.x == after.position.x,
          "Pause clears held keys");
    ex::Take(play->Resume());
    std::uint64_t serial = 0;
    int hits = 0, deaths = 0, waves = 0;
    for (int i = 0; i < 240; ++i) {
        ex::Take(play->Tick({1}));
        for (const auto &event : play->RuntimeSnapshot().trace)
            if (event.sequence > serial) {
                serial = event.sequence;
                if (event.state == "ProjectileHit")
                    ++hits;
                if (event.state == "Died")
                    ++deaths;
                if (event.state == "WaveCompleted")
                    ++waves;
            }
    }
    Check(hits > 0 && deaths > 0 && waves == 2, "projectile/collision/damage/death/waves");
    Check(!context->gameplay->Execute("gameplay.health.damage.v1", player, -1),
          "typed invalid damage");
    Check(!play->SetTimeScale(std::numeric_limits<double>::quiet_NaN()), "NaN scale");
    ex::Take(play->SetTimeScale(2));
    tick = play->RuntimeSnapshot().tick;
    ex::Take(play->Pump(std::chrono::nanoseconds(16666667)));
    Check(play->RuntimeSnapshot().tick == tick + 2, "time scale uses native clock");
    const auto inputComponent =
        Present(context->components
                    .Find(Present(context->entities.Resolve(player), "live player").id,
                          epidemic::gameplay::TypeId::FromString("gameplay.inputbindings")),
                "input bindings");
    ex::Take(
        context->components.WriteProperty(inputComponent, {ex::Id("right"), std::int64_t{76}}));
    ex::Take(play->SetKey(68, true));
    auto position = ex::Take(context->WorldTransform(player)).position.x;
    ex::Take(play->Tick({1}));
    Check(ex::Take(context->WorldTransform(player)).position.x == position,
          "old key no longer bound");
    ex::Take(play->SetKey(76, true));
    ex::Take(play->Tick({1}));
    Check(ex::Take(context->WorldTransform(player)).position.x > position,
          "custom saved binding key");
    ex::Take(play->SetKey(0, false));
    auto spawner = play->Instance(epidemic::runtime::SceneNodeId{3})->Objects().front().handle;
    ex::Take(context->gameplay->Execute("gameplay.wave.start.v1", spawner, 0));
    ex::Take(play->Tick({1}));
    ex::Take(context->gameplay->Execute("gameplay.wave.cancel.v1", spawner, 0));
    auto last = play->RuntimeSnapshot().trace.back().sequence;
    for (int i = 0; i < 40; ++i) {
        ex::Take(play->Tick({1}));
        for (const auto &e : play->RuntimeSnapshot().trace)
            Check(!(e.sequence > last && e.entity == 3 && e.state == "Spawned"),
                  "canceled wave kept spawning");
    }
    Check(!context->gameplay->Execute("gameplay.time.timer.v1", player, 0),
          "invalid timer typed reject");
    Check(!context->gameplay->Execute("gameplay.entity.spawn.v1", player, 0),
          "missing prefab typed reject");
    auto movement = Present(context->components
                                .Find(Present(context->entities.Resolve(player), "live player").id,
                                      schema.movement),
                            "movement component");
    Check(!context->components.WriteProperty(movement, {schema.speed, -1.}),
          "negative speed rejected");
    Check(!context->components.WriteProperty(movement, {schema.speed,
                                                        std::numeric_limits<double>::infinity()}),
          "Inf rejected");
    ex::Take(play->Stop());
    ex::Take(play->Stop());
    Check(play->RuntimeSnapshot().total_entities == 0, "Stop no entity jobs");
    auto again = ex::Take(project->Play(ex::Options(schema)));
    auto restored = again->Instance(epidemic::runtime::SceneNodeId{1})->Objects().front().handle;
    Check(ex::Take(again->Context()->gameplay->Execute("gameplay.health.get.v1", restored, 0)) ==
              100,
          "Arena authoring Health remains100");
    ex::Take(again->Stop());
    std::cout << "PASS Arena native Graph/Wait/input/time-scale/collision/projectile/waves/Stop\n";
}

void DeathEventOwnership(ex::Schema &schema) {
    auto definition = ex::Arena(schema);
    auto deathId = epidemic::runtime::AssetId::FromString("arena.death.regression");
    using K = epidemic::runtime::graph::PortKind;
    using D = epidemic::runtime::graph::PortDirection;
    epidemic::runtime::graph::GraphDefinition
        death{deathId,
              "Died lifecycle",
              {{ex::Id("death.event.node"),
                ex::Id("gameplay.event.Died.v1"),
                "Died",
                {ex::Port("death.event", K::Event, D::Input),
                 ex::Port("death.payload", K::Data, D::Output),
                 ex::Port("death.next", K::Control, D::Output)}},
               {ex::Id("death.read.node"),
                ex::Id("gameplay.health.get.v1"),
                "Read Health at death",
                {ex::Port("death.arg", K::Data, D::Input),
                 ex::Port("death.value", K::Data, D::Output),
                 ex::Port("death.in", K::Control, D::Input),
                 ex::Port("death.done", K::Control, D::Output)}}},
              {{ex::Id("death.edge"), ex::Id("death.next"), ex::Id("death.in")}}};
    death.nodes[1].ports[0].default_value = 0;
    for (auto &doc : definition.documents)
        if (auto *prefab = std::get_if<ex::comp::PrefabDefinition>(&doc.data)) {
            if (doc.name == "Arena Player Turret")
                prefab->root.behaviors.clear();
            if (doc.name == "Arena Enemy") {
                prefab->root.behaviors.push_back(
                    {ex::Id("death.behavior"),
                     deathId.value,
                     {{ex::Id("gameplay.event.Died.v1"), ex::Id("death.event")}},
                     {}});
                for (auto &component : prefab->root.components)
                    if (component.type == schema.health)
                        for (auto &field : component.properties)
                            if (field.property == schema.points)
                                field.value = std::int64_t{1};
            }
            if (doc.name == "Arena Spawner")
                for (auto &component : prefab->root.components)
                    if (component.type ==
                        epidemic::gameplay::TypeId::FromString("gameplay.wavespawner"))
                        for (auto &field : component.properties)
                            if (field.property == ex::Id("enabled"))
                                field.value = false;
        }
    definition.documents.push_back({deathId, "Died lifecycle", 0, death});
    auto project = ex::Take(ed::EditorProject::Create("Temp/editor-tests/arena-death.epidemic",
                                                      definition, schema.View()));
    auto options = ex::Options(schema);
    options.start_events = true;
    auto play = ex::Take(project->Play(std::move(options)));
    for (int i = 0; i < 90; ++i)
        ex::Take(play->Tick({1}));
    auto snapshot = play->RuntimeSnapshot();
    int completed = 0;
    for (const auto &activation : snapshot.activations)
        if (activation.graph == deathId.Raw()) {
            Check(activation.state == "Completed" && activation.native_calls == 1,
                  "Died Graph canceled before its native callback could run");
            ++completed;
        }
    Check(completed == 1, "Died Graph must run exactly once with live scope before destroy");
    ex::Take(play->Stop());
    auto debugOptions = ex::Options(schema);
    debugOptions.start_events = true;
    play = ex::Take(project->Play(std::move(debugOptions)));
    auto dying = play->Instance(epidemic::runtime::SceneNodeId{2})->Objects().front().handle;
    play->SetBreakpoint(deathId, ex::Id("death.read.node"), true);
    for (int i = 0; i < 90 && play->Phase() != ed::PlayPhase::Paused; ++i)
        ex::Take(play->Tick({1}));
    Check(play->Phase() == ed::PlayPhase::Paused &&
              play->Context()->entities.Resolve(dying).has_value(),
          "Died breakpoint must preserve its owner while paused");
    auto pausedTick = play->RuntimeSnapshot().tick;
    ex::Take(play->Pump(std::chrono::seconds(1)));
    Check(play->RuntimeSnapshot().tick == pausedTick, "death grace cannot advance while paused");
    play->SetBreakpoint(deathId, ex::Id("death.read.node"), false);
    ex::Take(play->Resume());
    for (int i = 0; i < 5; ++i)
        ex::Take(play->Tick({1}));
    Check(!play->Context()->entities.Resolve(dying), "continued death scope must be removed");
    ex::Take(play->Stop());
    std::cout << "PASS Died Graph executes once before generation-safe deferred destroy\n";
}

int main() {
    try {
        ex::Schema schema;
        FixedInputReplay(schema);
        auto definition = ex::Definition(schema);
        auto project = ex::Take(ed::EditorProject::Create("Temp/editor-tests/gameplay.epidemic",
                                                          definition, schema.View()));
        auto play = ex::Take(project->Play(ex::Options(schema)));
        auto *context = play->Context();
        auto entities = context->entities.AllEntities();
        Check(!entities.empty(), "runtime entities");
        auto self = context->entities.HandleOf(entities.front().id);
        auto &game = *context->gameplay;
        Check(ex::Take(game.Execute("gameplay.health.damage.v1", self, 7)) == 7, "unified damage");
        Check(ex::Take(game.Execute("gameplay.health.get.v1", self, 0)) == 93,
              "read native Health");
        Check(ex::Take(game.Execute("gameplay.health.heal.v1", self, 10000)) == 7,
              "healing clamps to maximum");
        Check(!game.Execute("gameplay.health.damage.v1", self, -1), "reject negative");
        Check(ex::Take(game.Execute("gameplay.health.damage.v1", self, 10000)) == 100,
              "overkill clamps");
        Check(ex::Take(game.Execute("gameplay.health.damage.v1", self, 1)) == 0, "death once");
        Check(!game.Execute("not.registered", self, 0), "unknown operation");
        ex::Take(play->Stop());
        Check(!play->IsPlaying() && play->RuntimeSnapshot().total_entities == 0,
              "Stop releases runtime entities");
        auto again = ex::Take(project->Play(ex::Options(schema)));
        auto *fresh = again->Context();
        auto first = fresh->entities.AllEntities().front().id;
        Check(ex::Take(fresh->gameplay->Execute("gameplay.health.get.v1",
                                                fresh->entities.HandleOf(first), 0)) == 100,
              "authoring unchanged");
        ex::Take(again->Stop());
        Simulation(schema);
        CollisionAndChurn(schema);
        DeathEventOwnership(schema);
        std::cout << "PASS gameplay native health/schema/validation/Stop authoring\n";
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
