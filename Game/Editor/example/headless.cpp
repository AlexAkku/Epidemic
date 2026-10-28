#include "demo_game.h"
#include "Epidemic/Game/Editor/play_session.h"
#include <cstdio>
#include <stdexcept>

namespace {
using namespace epidemic;
using namespace epidemic::runtime;
namespace ed = game::editor;
namespace comp = gameplay::composition;
namespace gc = game::composition;
namespace rb = gameplay::runtime_bridge;
foundation::StringId Id(std::string_view value) { return foundation::StringId::FromString(value); }
template<class T> T Take(foundation::Result<T> result) { if (!result) throw std::runtime_error(result.GetError().code); return std::move(result).Value(); }
void Take(const foundation::Result<void> &result) { if (!result) throw std::runtime_error(result.GetError().code); }
void Check(bool value) { if (!value) throw std::runtime_error("headless editor invariant"); }
using ed::example::Schema; using ed::example::Port; using ed::example::Options;
void Write(const std::filesystem::path &file, Schema &schema) {
    const auto scene_id = AssetId::FromString("example.scene"), graph_id = AssetId::FromString("example.graph"), prefab_id = AssetId::FromString("example.prefab");
    gc::SceneCompositionDefinition scene; scene.asset_id = scene_id;
    graph::GraphDefinition graph{graph_id, "Enemy behavior", {}, {}};
    auto initial = Take(ed::EditorProject::Create(file, {Id("example.project"), scene_id, {{scene_id, "Scene", 0, scene}, {graph_id, "Graph", 0, graph}}}, schema.View()));
    Take(initial->Save()); initial.reset();
    auto project = Take(ed::EditorProject::Open(file, schema.View()));
    ed::EditRequest edit{project->Revision(), {}};
    comp::PrefabDefinition prefab; prefab.id = prefab_id.value;
    edit.commands = {ed::CreateDocument{{prefab_id, "Enemy", 0, prefab}},
        ed::AddEntity{prefab_id, {}, {Id("enemy"), schema.enemy, {}, {}, {}}},
        ed::SetComponent{prefab_id, Id("enemy"), {schema.health, {}, {{schema.points, std::int64_t{100}, {}}}, {}}},
        ed::SetBehavior{prefab_id, Id("enemy"), {}, {}, {Id("hit.behavior"), graph_id.value, {{Id("hit"), Id("event.in")}}, {}}}};
    const auto graph_template = ed::example::Graph();
    for (const auto &node : graph_template.nodes) edit.commands.push_back(ed::AddGraphNode{graph_id, node});
    for (const auto &edge : graph_template.connections) edit.commands.push_back(ed::ConnectPorts{graph_id, edge});
    SceneNodeSnapshot node; node.id = SceneNodeId{42}; edit.commands.push_back(ed::AddSceneNode{scene_id, node});
    SceneNodeSnapshot parent; parent.id=SceneNodeId{43}; parent.local_transform={{10,20,0},{0,0,0.70710678f,0.70710678f},{2,3,1}};
    edit.commands.push_back(ed::AddSceneNode{scene_id,parent});
    edit.commands.push_back(ed::SetSceneParent{scene_id,node.id,parent.id});
    edit.commands.push_back(ed::SetSceneTransform{scene_id,node.id,{{-2,4,0},{},{1,2,1}}});
    edit.commands.push_back(ed::PlacePrefab{scene_id, {node.id, prefab_id, {}}});
    (void)Take(project->Execute(edit)); const auto edited = project->Snapshot();
    Take(project->Undo()); Check(project->Snapshot()->definition.documents.size() == 2 && !project->IsDirty());
    Take(project->Redo()); Check(project->Snapshot() == edited);
    Take(project->Save()); Check(!project->IsDirty());
    std::puts("PASS: open, commands, references, undo/redo, atomic save");
}
void ReadPlay(const std::filesystem::path &file, Schema &schema) {
    auto project = Take(ed::EditorProject::Open(file, schema.View())); Check(!project->IsDirty());
    auto options = Options(schema);
    auto session = Take(project->Play(std::move(options)));
    const auto *scene=session->Scene(); if (!scene) throw std::runtime_error("runtime Scene missing");
    const auto local=scene->scene.transforms->GetLocalTransform(scene->nodes.at(SceneNodeId{42}));
    const auto world=scene->scene.transforms->GetWorldTransform(scene->nodes.at(SceneNodeId{42}));
    if (!local || !world) throw std::runtime_error("runtime transform missing");
    Check(local->position==Vec3{-2,4,0} && local->scale==Vec3{1,2,1});
    Check(std::abs(world->position.x+2)<0.0001f && std::abs(world->position.y-16)<0.0001f && world->scale==Vec3{2,6,1});
    const auto entity = session->Instance(SceneNodeId{42})->Objects()[0].entity;
    const auto health = session->Context()->components.Find(entity, schema.health);
    if (!health) throw std::runtime_error("runtime Health missing");
    (void)Take(session->Send(SceneNodeId{42}, {Id("hit"), 1})); Take(session->Tick());
    Check(Take(session->Context()->components.Read<std::int64_t>(*health, schema.points)) == 99);
    Take(session->Tick(GameDuration{1})); Check(Take(session->Context()->components.Read<std::int64_t>(*health, schema.points)) == 98);
    Take(session->Stop()); Check(!project->IsDirty() && schema.entities.AllEntities().empty());
    std::puts("PASS: new process load, Entity/Health/GraphBehavior, Wait/resume, Stop, isolated authoring");
}
void WriteOverrides(const std::filesystem::path &file,Schema &schema) {
    const auto scene=AssetId::FromString("example.scene"),prefab=AssetId::FromString("example.prefab");
    const auto parameter=Id("example.health.parameter"),node=Id("enemy");
    auto project=Take(ed::EditorProject::Create(file,ed::example::Definition(schema),schema.View()));
    SceneNodeSnapshot second; second.id=SceneNodeId{43}; second.local_transform.position={5,0,0};
    (void)Take(project->Execute({project->Revision(),{
        ed::SetPrefabParameter{prefab,{parameter,std::int64_t{100}}},
        ed::SetProperty{prefab,node,schema.health,{}, {schema.points,std::int64_t{100},parameter}},
        ed::AddSceneNode{scene,second},ed::PlacePrefab{scene,{second.id,prefab,{}}},
        ed::SetParameterOverride{scene,SceneNodeId{42},prefab,{parameter,std::int64_t{120}}},
        ed::SetComponentOverride{scene,SceneNodeId{43},prefab,{node,schema.health,{}, {schema.points,std::int64_t{75}}}},
        ed::SetComponentOverride{scene,SceneNodeId{42},prefab,{node,schema.movement,{}, {schema.speed,2.5}}},
        ed::SetComponentOverride{scene,SceneNodeId{43},prefab,{node,schema.movement,{}, {schema.speed,4.}}},
        ed::RenameDocument{prefab,"Shared Enemy"}}}));
    const auto snapshot=project->Snapshot(); Take(project->Undo()); Take(project->Redo()); Check(project->Snapshot()==snapshot);
    Take(project->Save()); std::puts("PASS: Prefab parameter/binding, two independent overrides, rename, Undo/Redo, save");
}
void ReadOverrides(const std::filesystem::path &file,Schema &schema) {
    auto project=Take(ed::EditorProject::Open(file,schema.View())); Check(!project->IsDirty());
    const auto prefab=AssetId::FromString("example.prefab"); const auto *doc=project->Snapshot()->Find(prefab);
    Check(doc && doc->name=="Shared Enemy");
    const auto &definition=std::get<comp::PrefabDefinition>(doc->data);
    Check(definition.parameters.size()==1 && definition.parameters.front().id==Id("example.health.parameter"));
    Check(definition.parameters.front().value==comp::PropertyValue{std::int64_t{100}} && definition.root.components.front().properties.front().parameter==Id("example.health.parameter"));
    auto session=Take(project->Play(Options(schema)));
    Check(session->Instance(SceneNodeId{42})->Definition()==session->Instance(SceneNodeId{43})->Definition());
    const auto read=[&](std::uint64_t placement,gameplay::TypeId type,comp::PropertyId property) {
        return Take(session->ReadComponentProperty(SceneNodeId{placement},Id("enemy"),type,{},property));
    };
    Check(read(42,schema.health,schema.points)==comp::PropertyValue{std::int64_t{120}} && read(43,schema.health,schema.points)==comp::PropertyValue{std::int64_t{75}});
    Check(read(42,schema.movement,schema.speed)==comp::PropertyValue{2.5} && read(43,schema.movement,schema.speed)==comp::PropertyValue{4.});
    (void)Take(session->Send(SceneNodeId{42},{Id("hit"),1})); Take(session->Tick()); Take(session->Tick(GameDuration{1}));
    Check(read(42,schema.health,schema.points)==comp::PropertyValue{std::int64_t{118}} && read(43,schema.health,schema.points)==comp::PropertyValue{std::int64_t{75}});
    Take(session->Stop()); Check(!project->IsDirty());
    std::puts("PASS: fresh-process parameters/overrides, shared definition, independent runtime state, event/Wait, Stop");
}
}
int main(int argc, char **argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: EpidemicEditorHeadless --write|--read-play|--write-overrides|--read-overrides <project path>");
        Schema schema; const std::filesystem::path file{argv[2]};
        if (std::string_view{argv[1]} == "--write") { std::filesystem::create_directories(file.parent_path()); Write(file, schema); }
        else if (std::string_view{argv[1]} == "--read-play") ReadPlay(file, schema);
        else if (std::string_view{argv[1]} == "--write-overrides") { std::filesystem::create_directories(file.parent_path()); WriteOverrides(file,schema); }
        else if (std::string_view{argv[1]} == "--read-overrides") ReadOverrides(file,schema);
        else throw std::runtime_error("unknown command");
    } catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
