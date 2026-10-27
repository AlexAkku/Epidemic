#include "Epidemic/Game/Editor/editor_commands.h"
#include "../../Composition/tests/user_components.h"

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
    Take(project->Execute({0, {ed::DeleteBehavior{prefab, Id("enemy"), {}, {}, Id("behavior")}, ed::DeleteDocument{graph_id}}}));
    Take(project->Undo()); Check(project->Snapshot()->Find(graph_id) != nullptr, "undo restores referenced graph and behavior together");
}
int main() { try { Documents(); Commands(); } catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; } return 0; }
