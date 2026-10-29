#include "Epidemic/Game/Editor/editor_workspace.h"
#include "../../example/demo_game.h"
#include <cstdio>
#include <limits>
#include <cmath>

using namespace epidemic;
namespace ed = game::editor;
namespace ex = ed::example;
namespace comp = gameplay::composition;
void InspectorCases();
void PrefabCases();
namespace {
void Check(bool value) { if (!value) throw std::runtime_error("workspace invariant"); }
void Spatial() {
    ex::Schema schema;
    ed::EditorWorkspace workspace{schema.View(), [&] { return ex::Options(schema); }};
    const auto scene=runtime::AssetId::FromString("example.scene"), prefab=runtime::AssetId::FromString("example.prefab");
    const runtime::SceneNodeId child{42}, parent{43};
    const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY "/spatial.epidemic"};
    std::filesystem::create_directories(file.parent_path());
    ex::Take(workspace.New(file,ex::Definition(schema)));
    const auto current=[&] { return std::get<game::composition::SceneCompositionDefinition>(workspace.Project()->Snapshot()->Find(scene)->data).scene.nodes.front().local_transform; };
    const auto apply=[&](ed::UiTransformField field,std::string_view text) {
        auto command=ex::Take(ed::EditorWorkspace::TransformCommand(scene,child,current(),field,text));
        ex::Take(workspace.Execute({std::move(command)},workspace.Project()->Revision()));
    };
    apply(ed::UiTransformField::PositionX,"-2"); apply(ed::UiTransformField::PositionY,"4"); apply(ed::UiTransformField::ScaleY,"2");
    apply(ed::UiTransformField::ScaleX,"-1"); Check(current().scale.x==-1); ex::Take(workspace.Undo());
    for (const auto text:{"nan","inf","-inf","1e100","1e-100","bad"}) Check(!ed::EditorWorkspace::TransformCommand(scene,child,current(),ed::UiTransformField::PositionX,text));
    Check(!ed::EditorWorkspace::TransformCommand(scene,child,current(),ed::UiTransformField::ScaleX,"0"));
    auto tilted=current(); tilted.rotation={0.70710678f,0,0,0.70710678f};
    Check(!ed::EditorWorkspace::TransformCommand(scene,child,tilted,ed::UiTransformField::RotationZDegrees,"90"));
    const auto turn=ex::Take(ed::EditorWorkspace::TransformCommand(scene,parent,runtime::Transform{},ed::UiTransformField::RotationZDegrees,"450"));
    runtime::SceneNodeSnapshot group; group.id=parent; group.local_transform=std::get<ed::SetSceneTransform>(turn).transform;
    group.local_transform.position={10,20,0}; group.local_transform.scale={2,3,1};
    comp::PrefabOverrides overrides; overrides.components.push_back({ex::Id("enemy"),schema.health,{}, {schema.points,std::int64_t{77}}});
    ex::Take(workspace.Execute({ed::AddSceneNode{scene,group},ed::SetSceneParent{scene,child,parent},ed::PlacePrefab{scene,{child,prefab,overrides}}},workspace.Project()->Revision()));
    ex::Take(workspace.Save()); const auto saved=workspace.Project()->Snapshot();
    apply(ed::UiTransformField::PositionX,"-3"); ex::Take(workspace.Undo());
    const auto revision=workspace.Project()->Revision();
    const auto reject=[&](std::vector<ed::Command> commands) {
        Check(!workspace.Execute(std::move(commands),revision));
        Check(workspace.Project()->Snapshot()==saved && workspace.Project()->Revision()==revision && !workspace.Project()->IsDirty() && workspace.Project()->CanRedo());
    };
    auto invalid=current(); invalid.scale.x=0;
    reject({ed::RenameDocument{scene,"Rejected"},ed::SetSceneTransform{scene,child,invalid}});
    reject({ed::SetSceneParent{scene,parent,child}}); // Cycle.
    reject({ed::SetSceneParent{scene,child,runtime::SceneNodeId{999}}});
    invalid=current(); invalid.position.x=std::numeric_limits<float>::infinity(); reject({ed::SetSceneTransform{scene,child,invalid}});
    invalid.position.x=std::numeric_limits<float>::quiet_NaN(); reject({ed::SetSceneTransform{scene,child,invalid}});
    ex::Take(workspace.Redo()); Check(current().position.x==-3); ex::Take(workspace.Undo());
    ex::Take(workspace.Open(file)); Check(current().position==runtime::Vec3{-2,4,0});
    const auto &restored=std::get<game::composition::SceneCompositionDefinition>(workspace.Project()->Snapshot()->Find(scene)->data);
    Check(restored.scene.nodes.front().id==child && restored.scene.nodes.front().parent==parent && std::get<std::int64_t>(restored.placements.front().overrides.components.front().property.value)==77);
    ex::Take(workspace.Play()); const auto *runtime_scene=workspace.Session()->Scene();
    if (!runtime_scene) throw std::runtime_error("missing runtime Scene");
    const auto world=runtime_scene->scene.transforms->GetWorldTransform(runtime_scene->nodes.at(child));
    if (!world) throw std::runtime_error("missing world transform");
    Check(std::abs(world->position.x+2)<0.0001f && std::abs(world->position.y-16)<0.0001f && world->scale==runtime::Vec3{2,6,1});
    apply(ed::UiTransformField::PositionX,"-5");
    Check(runtime_scene->scene.transforms->GetWorldTransform(runtime_scene->nodes.at(child))==world);
    const auto authored=workspace.Project()->Snapshot(); ex::Take(workspace.Stop()); Check(workspace.Project()->Snapshot()==authored);
    ex::Take(workspace.Undo()); Check(!workspace.Project()->IsDirty());
}
void Run() {
    ex::Schema schema;
    ed::EditorWorkspace workspace{schema.View(), [&] { return ex::Options(schema); }};
    const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY "/ui.epidemic"};
    std::filesystem::create_directories(file.parent_path());
    ex::Take(workspace.New(file, ex::Definition(schema)));
    ex::Take(workspace.Save());
    const auto prefab = runtime::AssetId::FromString("example.prefab");
    const auto scene = runtime::AssetId::FromString("example.scene");
    Check(workspace.Select({scene, ed::UiElement::SceneNode, 42}));
    const auto definition = workspace.PrefabDefinitionSelection();
    if (!definition) throw std::runtime_error("missing placement definition");
    const auto definition_selection = *definition;
    Check(definition_selection.document == prefab && definition_selection.element == ex::Id("enemy").Raw());
    Check(workspace.Select(definition_selection));
    Check(!workspace.PrefabDefinitionSelection());
    const ed::UiSelection health{prefab, ed::UiElement::PrefabComponent, ex::Id("enemy").Raw(), schema.health, {}};
    Check(workspace.Select(health));
    Check(!workspace.Select({prefab, ed::UiElement::PrefabComponent, ex::Id("missing").Raw(), schema.health, {}}));
    Check(!workspace.Select({prefab, ed::UiElement::PrefabNode, ex::Id("enemy").Raw(), schema.health, {}}));
    Check(workspace.Selection() == health);
    ex::Take(workspace.Execute({ed::DeleteComponent{prefab, ex::Id("enemy"), schema.health, {}}}, workspace.Project()->Revision()));
    Check(workspace.Selection() == definition_selection); // Deletion stays at the owning Entity, not the first Scene.
    Check(!workspace.Select(health));
    ex::Take(workspace.Undo()); Check(workspace.Select(health));
    ex::Take(workspace.Redo()); Check(workspace.Selection() == definition_selection);
    ex::Take(workspace.Undo());
    const auto child = ex::Id("ui.child");
    auto child_node = ex::Enemy(schema, child); child_node.behaviors.clear();
    ex::Take(workspace.Execute({ed::AddEntity{prefab, ex::Id("enemy"), std::move(child_node)}}, workspace.Project()->Revision()));
    Check(workspace.Select({prefab, ed::UiElement::PrefabNode, child.Raw()}));
    ex::Take(workspace.Execute({ed::DeleteEntity{prefab, child}}, workspace.Project()->Revision()));
    Check(workspace.Selection() == ed::UiSelection{prefab});
    ex::Take(workspace.Undo()); Check(workspace.Select({prefab, ed::UiElement::PrefabNode, child.Raw()}));
    ex::Take(workspace.Undo()); // Restore the initial saved state before the existing command/runtime scenarios.
    Check(workspace.Selection() == ed::UiSelection{prefab});
    Check(workspace.Select({scene, ed::UiElement::SceneNode, 42}));
    ex::Take(workspace.Execute({ed::DeleteSceneNode{scene, runtime::SceneNodeId{42}}}, workspace.Project()->Revision()));
    Check(workspace.Selection() == ed::UiSelection{scene} && !workspace.PrefabDefinitionSelection());
    ex::Take(workspace.Undo()); Check(workspace.Select({scene, ed::UiElement::SceneNode, 42}));
    Check(workspace.PrefabDefinitionSelection() == definition);
    ex::Take(workspace.Redo()); Check(workspace.Selection() == ed::UiSelection{scene});
    ex::Take(workspace.Undo());
    Check(workspace.Select({prefab, ed::UiElement::PrefabNode, ex::Id("enemy").Raw()}));
    const auto revision = workspace.Project()->Revision();
    auto edit = ed::SetProperty{prefab, ex::Id("enemy"), schema.health, {}, {schema.points, std::int64_t{90}, {}}};
    ex::Take(workspace.Execute({edit}, revision));
    const auto edited = workspace.Project()->Snapshot();
    Check(workspace.Project()->IsDirty());
    Check(!workspace.Execute({edit}, revision)); // Stale views cannot overwrite a newer command.
    Check(workspace.Project()->Snapshot() == edited);
    auto invalid = edit; invalid.property.value = std::int64_t{-1};
    Check(!workspace.Execute({ed::RenameDocument{prefab, "Rejected"}, invalid}, workspace.Project()->Revision()));
    Check(workspace.Project()->Snapshot() == edited);
    ex::Take(workspace.Undo()); Check(!workspace.Project()->IsDirty());
    ex::Take(workspace.Redo()); Check(workspace.Project()->Snapshot() == edited);
    Check(workspace.Selection().element == ex::Id("enemy").Raw());
    Check(!workspace.Open(file)); // Dirty replacement requires an explicit decision by the host.
    ex::Take(workspace.Save());
    Check(!workspace.Open(file.parent_path() / "not-present.epidemic"));
    Check(workspace.Project()->Snapshot() == edited);
    ex::Take(workspace.Play());
    (void)ex::Take(workspace.Send(runtime::SceneNodeId{42}, {ex::Id("hit"), 1}));
    ex::Take(workspace.Tick({}));
    const auto activation = workspace.Activations().at(0);
    Check(!activation->IsFinished());
    ex::Take(workspace.Stop());
    const auto cancelled = activation->Report();
    if (!cancelled) throw std::runtime_error("missing cancelled activation report");
    Check(activation->IsFinished() && cancelled->status == runtime::graph::ExecutionStatus::Cancelled);
    ex::Take(workspace.Stop());
    Check(workspace.Project()->Snapshot() == edited && !workspace.Project()->IsDirty());
    ex::Take(workspace.Open(file));
    Check(workspace.Selection().kind == ed::UiElement::Document && !workspace.Session());
    ex::Take(workspace.Play());
    (void)ex::Take(workspace.Send(runtime::SceneNodeId{42}, {ex::Id("hit"), 1}));
    ex::Take(workspace.Tick({})); ex::Take(workspace.Tick(runtime::GameDuration{1}));
    const auto completed = workspace.Activations().at(0)->Report();
    if (!completed) throw std::runtime_error("missing completed activation report");
    Check(completed->status == runtime::graph::ExecutionStatus::Completed);
    ex::Take(workspace.Stop());
    Check(schema.entities.AllEntities().empty());
    ex::Take(workspace.Play());
    (void)ex::Take(workspace.Send(runtime::SceneNodeId{42}, {ex::Id("hit"), std::numeric_limits<std::int64_t>::min()}));
    ex::Take(workspace.Tick({})); // Native failure is observable independently of Tick success.
    const auto failed = workspace.Activations().at(0)->Report();
    if (!failed || !failed->diagnostic) throw std::runtime_error("missing failed activation diagnostic");
    Check(failed->status == runtime::graph::ExecutionStatus::Failed);
    Check(failed->diagnostic->error.code == "example.damage_range");
    ex::Take(workspace.Stop());
    Check(!workspace.Project()->IsDirty());
    using K = comp::PropertyKind;
    for (const auto value : {"nan", "inf", "-inf", "", "1junk", "1,5"}) Check(!ed::EditorWorkspace::ParseValue(K::Real, value));
    Check(!ed::EditorWorkspace::ParseValue(K::Real, std::string_view{}));
    for (const auto value : {"-9223372036854775808", "9223372036854775807"}) Check(ed::EditorWorkspace::ParseValue(K::Integer, value).HasValue());
    Check(!ed::EditorWorkspace::ParseValue(K::Integer, "9223372036854775808"));
    Check(!ed::EditorWorkspace::ParseValue(K::Boolean, "1"));
    Check(std::get<std::string>(ex::Take(ed::EditorWorkspace::ParseValue(K::String, "  text  "))) == "  text  ");
    for (const comp::PropertyValue value : {comp::PropertyValue{0.1}, comp::PropertyValue{std::int64_t{-45}}, comp::PropertyValue{true}}) {
        const auto kind = static_cast<K>(value.index());
        Check(ex::Take(ed::EditorWorkspace::ParseValue(kind, ed::EditorWorkspace::FormatValue(value))) == value);
    }
}
}
void FolderWorkspace() {
    ex::Schema schema;
    const auto root=std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"workspace-folder";
    const auto clone=std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"workspace-folder-clone";
    std::error_code ec; std::filesystem::remove_all(root,ec); std::filesystem::remove_all(clone,ec);
    const auto scene=runtime::AssetId::FromString("example.scene");
    {
        ed::EditorWorkspace workspace{schema.View(),[&]{return ex::Options(schema);}};
        ex::Take(workspace.NewFolder(root,"Folder UI",ex::Definition(schema)));
        Check(workspace.Folder() && !workspace.IsDirty());
        ex::Take(workspace.Execute({ed::RenameDocument{scene,"Folder scene"}},workspace.Project()->Revision()));
        Check(workspace.IsDirty() && !workspace.Open(root/"Project.epidemicproj"));
        ex::Take(workspace.Undo()); Check(!workspace.IsDirty());
        ex::Take(workspace.Redo()); Check(workspace.IsDirty());
        ex::Take(workspace.Save()); Check(!workspace.IsDirty());
        ex::Take(workspace.CloneFolder(clone));
        ex::Take(workspace.Open(clone/"Project.epidemicproj"));
        Check(workspace.Folder() && workspace.Project()->Snapshot()->Find(scene)->name=="Folder scene");
        ex::Take(workspace.Play()); ex::Take(workspace.Stop());
        Check(!workspace.IsDirty());
    }
    ed::EditorWorkspace reopened{schema.View(),[&]{return ex::Options(schema);}};
    ex::Take(reopened.Open(root/"Project.epidemicproj"));
    Check(reopened.Project()->Snapshot()->Find(scene)->name=="Folder scene");
}
int main() {
    try { FolderWorkspace(); Spatial(); Run(); InspectorCases(); PrefabCases(); std::puts("PASS: workspace, schema Inspector, Prefab parameters/overrides, persistence and runtime isolation"); }
    catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
