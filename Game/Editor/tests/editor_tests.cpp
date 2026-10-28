#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/play_session.h"
#include "../../Composition/tests/user_components.h"
#include <fstream>
#include <array>

using namespace composition_tests;
namespace ed = epidemic::game::editor;
ed::ProjectDefinition Project(const Fixture &f, const ComponentsFixture &types) {
    auto graph = Definition();
    comp::PrefabDefinition prefab{Id("editor.prefab"), {}, {Id("enemy"), f.archetype,
        {{types.health, {}, {{types.points, std::int64_t{100}, {}}}, {}}},
        {{Id("behavior"), graph.asset_id.value, {{Id("hit"), Id("event.in")}}, {}}}, {}}};
    gc::SceneCompositionDefinition scene; scene.asset_id = AssetId::FromString("editor.scene");
    SceneNodeSnapshot node; node.id = SceneNodeId{42}; scene.scene.nodes.push_back(node);
    scene.placements.push_back({node.id, AssetId{prefab.id}, {}});
    return {Id("editor.project"), scene.asset_id,
        {{scene.asset_id, "Scene", 0, scene}, {AssetId{prefab.id}, "Enemy", 0, prefab}, {graph.asset_id, "Graph", 0, graph}}};
}
ed::ProjectSchema Schema(Fixture &f, const ComponentsFixture &types) {
    return {f.entities, types.registry, {{Id("event.type"), graph::Operation::EventEntry},
        {Id("call.type"), graph::Operation::NativeCall}, {Id("wait.type"), graph::Operation::Wait}}};
}
void Documents() {
    Fixture f; ComponentsFixture types;
    auto definition = Project(f, types);
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/project.epidemic", definition, Schema(f, types)));
    const auto snapshot = project->Snapshot();
    Check(snapshot->definition.documents.size() == 3 && snapshot->assets->IsSealed(), "existing Assets catalog backs document identity");
    Check(project->IsDirty() && project->IsDirty(definition.documents[0].id), "new documents are unsaved");
    const auto manifest = Take(snapshot->assets->BuildDependencyManifest(definition.entry_scene));
    (void)manifest;
    definition.documents.clear();
    Check(snapshot->definition.documents.size() == 3 && f.entities.AllEntities().empty(), "detached authoring snapshot never creates gameplay Entities");
    auto invalid = snapshot->definition;
    std::get<comp::PrefabDefinition>(invalid.documents[1].data).root.behaviors[0].graph = Id("missing");
    ErrorIs(ed::ValidateProject(invalid, Schema(f, types)), "editor.graph_reference");
    invalid = snapshot->definition;
    std::get<gc::SceneCompositionDefinition>(invalid.documents[0].data).scene.nodes[0].parent = SceneNodeId{999};
    ErrorIs(ed::ValidateProject(invalid, Schema(f, types)), "composition.scene_parent");
    auto repeated=snapshot->definition;
    auto &scene=std::get<gc::SceneCompositionDefinition>(repeated.documents[0].data);
    for (const auto id:{43u,44u}) {
        SceneNodeSnapshot node; node.id=SceneNodeId{id}; scene.scene.nodes.push_back(node);
        scene.placements.push_back({node.id,repeated.documents[1].id,{}});
    }
    auto &prefab=std::get<comp::PrefabDefinition>(repeated.documents[1].data);
    auto behavior=prefab.root.behaviors.front(); behavior.id=Id("another.behavior"); prefab.root.behaviors.push_back(behavior);
    const auto catalog=Take(ed::ValidateProject(repeated,Schema(f,types)));
    const auto scene_asset=catalog->FindById(scene.asset_id), prefab_asset=catalog->FindById(AssetId{prefab.id});
    if (!scene_asset || !prefab_asset) throw std::runtime_error("repeated asset references lost catalog identity");
    Check(scene_asset->dependencies.size()==1 && prefab_asset->dependencies.size()==1,
        "repeated Prefab placements and behaviors share a single asset dependency");
}
void Commands() {
    Fixture f; ComponentsFixture types;
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/commands.epidemic", Project(f, types), Schema(f, types)));
    const auto before = project->Snapshot();
    const auto prefab = AssetId::FromString("editor.prefab");
    const auto graph_id = Definition().asset_id;
    const auto revision = project->Revision();
    ErrorIs(project->Execute({revision, {ed::RenameDocument{prefab, "changed"}, ed::DeleteDocument{graph_id}}}), "editor.graph_reference");
    Check(project->Snapshot() == before && !project->CanUndo() && project->Revision() == revision, "failed composite has no effects or history");
    Take(project->Execute({revision, {ed::SetProperty{prefab, Id("enemy"), types.health, {}, {types.points, std::int64_t{50}, {}}},
        ed::RenameDocument{prefab, "modified"}}}));
    const auto after = project->Snapshot();
    Check(std::get<std::int64_t>(std::get<comp::PrefabDefinition>(after->Find(prefab)->data).root.components[0].properties[0].value) == 50,
        "typed command changes component property");
    Take(project->Undo());
    Check(project->Snapshot() == before && project->Revision() > revision, "undo restores exact identity snapshot with monotonic automation revision");
    Take(project->Redo()); Check(project->Snapshot() == after, "redo restores composite state");
    ErrorIs(project->Execute({revision, {ed::RenameDocument{prefab, "stale"}}}), "editor.stale_revision");
    auto node = graph::NodeDefinition{Id("new.node"), Id("call.type"), "unused", {}};
    Take(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}));
    Take(project->Undo());
    ErrorIs(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}), "editor.retired_id");
    Check(project->CanRedo(), "rejected ID reuse preserves redo");
    Take(project->Redo());
    Take(project->Execute({0, {ed::DeleteGraphNode{graph_id, node.id}}}));
    ErrorIs(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}), "editor.retired_id");
    // These probes document current container-scoped identities: reuse is currently accepted.
    const auto parameter=comp::PrefabParameter{Id("probe.parameter"),std::int64_t{42}};
    Take(project->Execute({0,{ed::SetPrefabParameter{prefab,parameter}}})); Take(project->Undo());
    Check(static_cast<bool>(project->Execute({0,{ed::SetPrefabParameter{prefab,parameter}}})), "parameter ID reuse after Undo is currently accepted");
    auto behavior=std::get<comp::PrefabDefinition>(project->Snapshot()->Find(prefab)->data).root.behaviors.front();
    behavior.id=Id("probe.behavior");
    Take(project->Execute({0,{ed::SetBehavior{prefab,Id("enemy"),{},{},behavior}}}));
    Take(project->Execute({0,{ed::DeleteBehavior{prefab,Id("enemy"),{},{},behavior.id}}}));
    Check(static_cast<bool>(project->Execute({0,{ed::SetBehavior{prefab,Id("enemy"),{},{},behavior}}})), "behavior ID reuse after delete is currently accepted");
    Take(project->Execute({0,{ed::DeleteBehavior{prefab,Id("enemy"),{},{},behavior.id}}}));
    Take(project->Execute({0, {ed::DeleteBehavior{prefab, Id("enemy"), {}, {}, Id("behavior")}, ed::DeleteDocument{graph_id}}}));
    Take(project->Undo()); Check(project->Snapshot()->Find(graph_id) != nullptr, "undo restores referenced graph and behavior together");
}
void Persistence() {
    Fixture f; ComponentsFixture types;
    const auto directory = std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}; std::filesystem::create_directories(directory);
    const auto file = directory / "unit.epidemic";
    auto definition = Project(f, types);
    auto &authored_prefab = std::get<comp::PrefabDefinition>(definition.documents[1].data);
    authored_prefab.parameters.push_back({Id("start"), std::int64_t{100}});
    authored_prefab.root.components[0].properties[0].parameter = Id("start");
    authored_prefab.root.components.push_back({types.movement, {}, {{types.speed, 2.5, {}}, {types.label, std::string{"враг"}, {}}}, {}});
    std::get<std::string>(authored_prefab.root.components[1].properties[1].value).append(70000, 'x');
    for (std::size_t i = 0; i < 511; ++i) authored_prefab.root.behaviors[0].events.push_back({Id("event." + std::to_string(i)), Id("event.in")});
    authored_prefab.root.children.push_back({Id("nested"), f.archetype, {}, {}, {}});
    auto &authored_scene = std::get<gc::SceneCompositionDefinition>(definition.documents[0].data);
    authored_scene.scene.nodes[0].local_transform.position = {1.25f, -2, 3};
    authored_scene.scene.nodes[0].local_bounds = Aabb{{-1, -1, -1}, {1, 1, 1}};
    SceneNodeSnapshot child; child.id = SceneNodeId{43}; child.parent = SceneNodeId{42}; authored_scene.scene.nodes.push_back(child);
    authored_scene.placements[0].overrides.parameters.push_back({Id("start"), std::int64_t{85}});
    auto project = Take(ed::EditorProject::Create(file, definition, Schema(f, types)));
    const auto graph_id = Definition().asset_id;
    const auto prefab = AssetId::FromString("editor.prefab");
    const graph::NodeDefinition node{Id("retired.saved"), Id("call.type"), "Temporary", {}};
    Take(project->Execute({0, {ed::AddGraphNode{graph_id, node}}})); Take(project->Undo());
    Take(project->Save()); Check(!project->IsDirty() && !project->IsDirty(prefab), "save point recorded after publication");
    Take(project->Execute({0, {ed::RenameDocument{prefab, "new name"}}}));
    Check(project->IsDirty(prefab) && !project->IsDirty(graph_id), "per-document dirty state");
    Take(project->Undo()); Check(!project->IsDirty(), "undo to saved state is clean"); Take(project->Redo());
    auto restored = Take(ed::EditorProject::Open(file, Schema(f, types)));
    Check(!restored->IsDirty() && !restored->CanUndo() && restored->Snapshot()->Find(prefab)->name == "Enemy", "disk contains committed save point");
    Check(std::get<graph::GraphDefinition>(restored->Snapshot()->Find(graph_id)->data) == Definition(), "Graph uses existing serializer without changing IDs");
    const auto &restored_prefab = std::get<comp::PrefabDefinition>(restored->Snapshot()->Find(prefab)->data);
    Check(restored_prefab.root.children[0].id == Id("nested") && restored_prefab.root.components[0].properties[0].parameter == Id("start") &&
        std::get<std::string>(restored_prefab.root.components[1].properties[1].value) == std::get<std::string>(authored_prefab.root.components[1].properties[1].value) &&
        restored_prefab.root.behaviors[0].events.size() == 512, "nested prefab parameters, large typed properties and supported event routes survive reload");
    const auto &restored_scene = std::get<gc::SceneCompositionDefinition>(restored->Snapshot()->Find(definition.entry_scene)->data);
    Check(restored_scene.scene.nodes[1].parent == SceneNodeId{42} && restored_scene.scene.nodes[0].local_bounds.has_value() &&
        restored_scene.scene.nodes[0].local_transform.position == Vec3{1.25f, -2, 3} &&
        std::get<std::int64_t>(restored_scene.placements[0].overrides.parameters[0].value) == 85, "Scene references transforms bounds and instance overrides survive reload");
    ErrorIs(restored->Execute({0, {ed::AddGraphNode{graph_id, node}}}), "editor.retired_id");
    auto failed = Take(ed::EditorProject::Create(directory / "absent" / "child.epidemic", Project(f, types), Schema(f, types)));
    Check(!failed->Save() && failed->IsDirty(), "failed disk write does not mark clean");
    { std::ofstream broken(file, std::ios::binary); broken << "invalid"; }
    Check(!ed::EditorProject::Open(file, Schema(f, types)), "corrupt project rejected");
    Check(restored->Snapshot()->Find(prefab)->name == "Enemy", "failed open does not affect existing session");
}
ed::PlayOptions Options(const ComponentsFixture &types) {
    ed::PlayOptions options;
    options.backend = std::make_unique<Backend>();
    options.bindings = [&](ed::PlayContext &context, AssetId) {
        auto bindings = Bindings([store = &context.components, health = types.health, points = types.points](const gc::OperationContext &operation, std::int64_t amount) {
            const auto entity = fw::entities::EntityService::FromGameplayObjectRef(operation.scope.Object());
            const auto component = store->Find(entity, health);
            if (!component) return foundation::Result<std::int64_t>::Failure(foundation::Error::Create("test.no_health", "missing Health"));
            const auto current = store->Read<std::int64_t>(*component, points);
            if (!current) return foundation::Result<std::int64_t>::Failure(current.GetError());
            const auto changed = store->WriteProperty(*component, {points, current.Value() - amount});
            if (!changed) return foundation::Result<std::int64_t>::Failure(changed.GetError());
            return foundation::Result<std::int64_t>::Success(amount);
        });
        return std::vector<gc::NodeBinding>{bindings.begin(), bindings.end()};
    };
    return options;
}
std::int64_t HealthOf(ed::PlaySession &session, const ComponentsFixture &types) {
    const auto *instance = session.Instance(SceneNodeId{42}); Check(instance != nullptr, "placed prefab exists in Play");
    const auto component = session.Context()->components.Find(instance->Objects()[0].entity, types.health);
    if (!component) throw std::runtime_error("runtime Health missing");
    return Take(session.Context()->components.Read<std::int64_t>(*component, types.points));
}
void Play() {
    Fixture f; ComponentsFixture types;
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/play.epidemic", Project(f, types), Schema(f, types)));
    const auto source = project->Snapshot(); auto session = Take(project->Play(Options(types)));
    Check(session->Scene()->nodes.contains(SceneNodeId{42}) && HealthOf(*session, types) == 100, "Scene and Prefab use existing runtime services");
    Check(Take(session->Send(SceneNodeId{42}, {Id("hit"), 1})) == 1, "gameplay event enters graph");
    Take(session->Tick()); Check(HealthOf(*session, types) == 99, "side effect before Wait");
    Take(session->Tick(GameDuration{1})); Check(HealthOf(*session, types) == 98, "resume after Wait");
    Check(project->Snapshot() == source && f.entities.AllEntities().empty(), "Play isolates authoring data and Entities");
    auto second = Take(project->Play(Options(types))); Check(HealthOf(*second, types) == 100, "independent Play sessions");
    const auto *instance = session->Instance(SceneNodeId{42});
    auto activation = Take(session->Context()->behaviors.Send(instance->Objects()[0].behaviors[0].handle, {Id("hit"), 1}));
    Take(session->Tick()); Check(Status(activation) == graph::ExecutionStatus::Waiting, "activation awaiting Stop");
    Take(session->Stop()); Check(Status(activation) == graph::ExecutionStatus::Cancelled, "Stop cancels pending runtime activations");
    Check(!session->IsPlaying() && session->Context() == nullptr && !session->Tick(), "stopped session releases mutable runtime");
    Take(session->Stop());
    auto restarted = Take(project->Play(Options(types))); Check(HealthOf(*restarted, types) == 100, "Play after Stop starts from authoring state");
    Take(project->Execute({0, {ed::SetProperty{AssetId::FromString("editor.prefab"), Id("enemy"), types.health, {}, {types.points, std::int64_t{75}, {}}}}}));
    Check(HealthOf(*restarted, types) == 100, "authoring edits during Play cannot mutate running instance");
    const auto destroyed = restarted->Instance(SceneNodeId{42})->Objects()[0].entity;
    Take(restarted->Context()->entities.RequestDestroy(destroyed, fw::entities::EntityDestroyReason::Destroyed));
    Take(restarted->Stop()); Check(!restarted->IsPlaying(), "Stop accepts Entities already pending destruction");
    auto draft=Definition(); draft.asset_id=AssetId::FromString("editor.unused.graph");
    Take(project->Execute({0,{ed::CreateDocument{{draft.asset_id,"Unused graph",0,draft}}}}));
    auto unused_failure=Options(types); auto normal=unused_failure.bindings;
    unused_failure.bindings=[normal,draft](ed::PlayContext &ctx,AssetId id) {
        if(id==draft.asset_id) throw std::runtime_error("unused factory failure");
        return normal(ctx,id);
    };
    ErrorIs(project->Play(std::move(unused_failure)),"editor.play_exception");
    auto unused_mismatch=Options(types); normal=unused_mismatch.bindings;
    unused_mismatch.bindings=[normal,draft](ed::PlayContext &ctx,AssetId id) {
        auto result=normal(ctx,id);
        if(id==draft.asset_id && !result.empty()) result[0].operation=graph::Operation::Wait;
        return result;
    };
    ErrorIs(project->Play(std::move(unused_mismatch)),"editor.play_binding");
    auto throwing = Options(types); throwing.bindings = [](ed::PlayContext &, AssetId) -> std::vector<gc::NodeBinding> { throw std::runtime_error("binding failed"); };
    ErrorIs(project->Play(std::move(throwing)), "editor.play_exception");
    Check(project->Snapshot()->Find(AssetId::FromString("editor.prefab"))->name == "Enemy", "failed Play does not mutate authoring");
}
void PlacementDiagnostic() {
    Fixture f; ComponentsFixture types; auto definition=Project(f,types);
    auto &scene=std::get<gc::SceneCompositionDefinition>(definition.documents[0].data);
    scene.scene.nodes[0].local_transform.position={4,0,0};
    SceneNodeSnapshot parent; parent.id=SceneNodeId{99}; parent.local_transform.position={10,0,0}; scene.scene.nodes.push_back(parent);
    SceneNodeSnapshot child; child.id=SceneNodeId{43}; child.parent=parent.id; child.local_transform.position={2,0,0}; scene.scene.nodes.push_back(child);
    SceneNodeSnapshot third; third.id=SceneNodeId{44}; third.local_transform.position={-3,0,0}; scene.scene.nodes.push_back(third);
    for(auto id:{43u,44u})scene.placements.push_back({SceneNodeId{id},definition.documents[1].id,{}});
    auto project=Take(ed::EditorProject::Create("Temp/editor-tests/placement-diagnostic.epidemic",definition,Schema(f,types)));
    auto play=Take(project->Play(Options(types))); const auto *runtime=play->Scene();
    for(const auto &[id,x]:std::array<std::pair<unsigned,float>,3>{{{42u,4.0f},{43u,12.0f},{44u,-3.0f}}}) {
        const auto node=runtime->nodes.at(SceneNodeId{id}); const auto world=runtime->scene.transforms->GetWorldTransform(node);
        Check(world && world->position.x==x && play->Instance(SceneNodeId{id}),"scene world transform and placement instance exist");
    }
    const auto a=play->Instance(SceneNodeId{42})->Objects()[0].entity;
    const auto b=play->Instance(SceneNodeId{43})->Objects()[0].entity;
    const auto c=play->Instance(SceneNodeId{44})->Objects()[0].entity;
    Check(a!=b && a!=c && b!=c,"placement instances own independent entities; spatial Entity binding is not implemented");
}
#include "stabilization_cases.h"
int main(int argc, char **argv) {
    try {
        if (argc == 1) { Documents(); Commands(); Persistence(); Play(); PlacementDiagnostic(); }
        else if (argc == 2 && std::string_view{argv[1]} == "failures") StabilizationFailures();
        else if (argc == 2 && std::string_view{argv[1]} == "authoring") StabilizationAuthoring();
        else if (argc == 2 && std::string_view{argv[1]} == "stop") StabilizationStop();
        else if (argc == 2 && std::string_view{argv[1]} == "dependencies") StabilizationDependencies();
        else throw std::runtime_error("unknown editor test case");
    } catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
    return 0;
}
