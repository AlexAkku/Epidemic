#include "Epidemic/Game/Editor/editor_workspace.h"
#include "../../example/demo_game.h"
#include <cstdio>
#include <limits>
#include <cmath>
#include <chrono>
#include <fstream>
#include <thread>
#include <array>
#include <source_location>

using namespace epidemic;
namespace ed = game::editor;
namespace ex = ed::example;
namespace comp = gameplay::composition;
void InspectorCases();
void PrefabCases();

namespace {
void Check(bool value, const std::source_location at = std::source_location::current()) {
    if (!value)
        throw std::runtime_error(std::string("workspace invariant at ") + at.file_name() + ":" +
                                 std::to_string(at.line()));
}

void Spatial() {
    ex::Schema schema;
    ed::EditorWorkspace workspace{schema.View(), [&] { return ex::Options(schema); }};

    const auto scene = runtime::AssetId::FromString("example.scene"),
               prefab = runtime::AssetId::FromString("example.prefab");
    const runtime::SceneNodeId child{42}, parent{43};

    const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY "/spatial.epidemic"};

    std::filesystem::create_directories(file.parent_path());
    ex::Take(workspace.New(file, ex::Definition(schema)));
    const auto current = [&] {
        return std::get<game::composition::SceneCompositionDefinition>(
                   workspace.Project()->Snapshot()->Find(scene)->data)
            .scene.nodes.front()
            .local_transform;
    };

    const auto apply = [&](ed::UiTransformField field, std::string_view text) {
        auto command =
            ex::Take(ed::EditorWorkspace::TransformCommand(scene, child, current(), field, text));
        ex::Take(workspace.Execute({std::move(command)}, workspace.Project()->Revision()));
    };

    apply(ed::UiTransformField::PositionX, "-2");
    apply(ed::UiTransformField::PositionY, "4");
    apply(ed::UiTransformField::ScaleY, "2");
    apply(ed::UiTransformField::ScaleX, "-1");
    Check(current().scale.x == -1);
    ex::Take(workspace.Undo());
    for (const auto text : {"nan", "inf", "-inf", "1e100", "1e-100", "bad"})
        Check(!ed::EditorWorkspace::TransformCommand(scene, child, current(),
                                                     ed::UiTransformField::PositionX, text));
    Check(!ed::EditorWorkspace::TransformCommand(scene, child, current(),
                                                 ed::UiTransformField::ScaleX, "0"));
    auto tilted = current();
    tilted.rotation = {0.70710678f, 0, 0, 0.70710678f};

    Check(!ed::EditorWorkspace::TransformCommand(scene, child, tilted,
                                                 ed::UiTransformField::RotationZDegrees, "90"));
    const auto turn = ex::Take(
        ed::EditorWorkspace::TransformCommand(scene, parent, runtime::Transform{},
                                              ed::UiTransformField::RotationZDegrees, "450"));
    runtime::SceneNodeSnapshot group;
    group.id = parent;
    group.local_transform = std::get<ed::SetSceneTransform>(turn).transform;
    group.local_transform.position = {10, 20, 0};

    group.local_transform.scale = {2, 3, 1};

    comp::PrefabOverrides overrides;
    overrides.components.push_back(
        {ex::Id("enemy"), schema.health, {}, {schema.points, std::int64_t{77}}});
    ex::Take(
        workspace.Execute({ed::AddSceneNode{scene, group}, ed::SetSceneParent{scene, child, parent},
                           ed::PlacePrefab{scene, {child, prefab, overrides}}},
                          workspace.Project()->Revision()));
    ex::Take(workspace.Save());
    const auto saved = workspace.Project()->Snapshot();
    apply(ed::UiTransformField::PositionX, "-3");
    ex::Take(workspace.Undo());
    const auto revision = workspace.Project()->Revision();
    const auto reject = [&](std::vector<ed::Command> commands) {
        Check(!workspace.Execute(std::move(commands), revision));
        Check(workspace.Project()->Snapshot() == saved &&
              workspace.Project()->Revision() == revision && !workspace.Project()->IsDirty() &&
              workspace.Project()->CanRedo());
    };

    auto invalid = current();
    invalid.scale.x = 0;
    reject({ed::RenameDocument{scene, "Rejected"}, ed::SetSceneTransform{scene, child, invalid}});
    reject({ed::SetSceneParent{scene, parent, child}}); // Cycle.
    reject({ed::SetSceneParent{scene, child, runtime::SceneNodeId{999}}});
    invalid = current();
    invalid.position.x = std::numeric_limits<float>::infinity();
    reject({ed::SetSceneTransform{scene, child, invalid}});
    invalid.position.x = std::numeric_limits<float>::quiet_NaN();
    reject({ed::SetSceneTransform{scene, child, invalid}});
    ex::Take(workspace.Redo());
    Check(current().position.x == -3);
    ex::Take(workspace.Undo());
    ex::Take(workspace.Open(file));
    Check(current().position == runtime::Vec3{-2, 4, 0});
    const auto &restored = std::get<game::composition::SceneCompositionDefinition>(
        workspace.Project()->Snapshot()->Find(scene)->data);
    Check(restored.scene.nodes.front().id == child &&
          restored.scene.nodes.front().parent == parent &&
          std::get<std::int64_t>(
              restored.placements.front().overrides.components.front().property.value) == 77);
    ex::Take(workspace.Play());
    const auto *runtime_scene = workspace.Session()->Scene();
    if (!runtime_scene)
        throw std::runtime_error("missing runtime Scene");
    const auto world =
        runtime_scene->scene.transforms->GetWorldTransform(runtime_scene->nodes.at(child));
    if (!world)
        throw std::runtime_error("missing world transform");
    Check(std::abs(world->position.x + 2) < 0.0001f && std::abs(world->position.y - 16) < 0.0001f &&
          world->scale == runtime::Vec3{2, 6, 1});
    apply(ed::UiTransformField::PositionX, "-5");
    Check(runtime_scene->scene.transforms->GetWorldTransform(runtime_scene->nodes.at(child)) ==
          world);
    const auto authored = workspace.Project()->Snapshot();
    ex::Take(workspace.Stop());
    Check(workspace.Project()->Snapshot() == authored);
    ex::Take(workspace.Undo());
    Check(!workspace.Project()->IsDirty());
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
    if (!definition)
        throw std::runtime_error("missing placement definition");
    const auto definition_selection = *definition;
    Check(definition_selection.document == prefab &&
          definition_selection.element == ex::Id("enemy").Raw());
    Check(workspace.Select(definition_selection));
    Check(!workspace.PrefabDefinitionSelection());
    const ed::UiSelection health{prefab,
                                 ed::UiElement::PrefabComponent,
                                 ex::Id("enemy").Raw(),
                                 schema.health,
                                 {}};

    Check(workspace.Select(health));
    Check(!workspace.Select(
        {prefab, ed::UiElement::PrefabComponent, ex::Id("missing").Raw(), schema.health, {}}));
    Check(!workspace.Select(
        {prefab, ed::UiElement::PrefabNode, ex::Id("enemy").Raw(), schema.health, {}}));
    Check(workspace.Selection() == health);
    ex::Take(workspace.Execute({ed::DeleteComponent{prefab, ex::Id("enemy"), schema.health, {}}},
                               workspace.Project()->Revision()));
    Check(workspace.Selection() ==
          definition_selection); // Deletion stays at the owning Entity, not the first Scene.
    Check(!workspace.Select(health));
    ex::Take(workspace.Undo());
    Check(workspace.Select(health));
    ex::Take(workspace.Redo());
    Check(workspace.Selection() == definition_selection);
    ex::Take(workspace.Undo());
    const auto child = ex::Id("ui.child");
    auto child_node = ex::Enemy(schema, child);
    child_node.behaviors.clear();
    ex::Take(workspace.Execute({ed::AddEntity{prefab, ex::Id("enemy"), std::move(child_node)}},
                               workspace.Project()->Revision()));
    Check(workspace.Select({prefab, ed::UiElement::PrefabNode, child.Raw()}));
    ex::Take(workspace.Execute({ed::DeleteEntity{prefab, child}}, workspace.Project()->Revision()));
    Check(workspace.Selection() == ed::UiSelection{prefab});
    ex::Take(workspace.Undo());
    Check(workspace.Select({prefab, ed::UiElement::PrefabNode, child.Raw()}));
    ex::Take(workspace.Undo()); // Restore the initial saved state before the existing
                                // command/runtime scenarios.
    Check(workspace.Selection() == ed::UiSelection{prefab});
    Check(workspace.Select({scene, ed::UiElement::SceneNode, 42}));
    ex::Take(workspace.Execute({ed::DeleteSceneNode{scene, runtime::SceneNodeId{42}}},
                               workspace.Project()->Revision()));
    Check(workspace.Selection() == ed::UiSelection{scene} &&
          !workspace.PrefabDefinitionSelection());
    ex::Take(workspace.Undo());
    Check(workspace.Select({scene, ed::UiElement::SceneNode, 42}));
    Check(workspace.PrefabDefinitionSelection() == definition);
    ex::Take(workspace.Redo());
    Check(workspace.Selection() == ed::UiSelection{scene});
    ex::Take(workspace.Undo());
    Check(workspace.Select({prefab, ed::UiElement::PrefabNode, ex::Id("enemy").Raw()}));
    const auto revision = workspace.Project()->Revision();
    auto edit = ed::SetProperty{prefab,
                                ex::Id("enemy"),
                                schema.health,
                                {},
                                {schema.points, std::int64_t{90}, {}}};

    ex::Take(workspace.Execute({edit}, revision));
    const auto edited = workspace.Project()->Snapshot();
    Check(workspace.Project()->IsDirty());
    Check(!workspace.Execute({edit}, revision)); // Stale views cannot overwrite a newer command.
    Check(workspace.Project()->Snapshot() == edited);
    auto invalid = edit;
    invalid.property.value = std::int64_t{-1};

    Check(!workspace.Execute({ed::RenameDocument{prefab, "Rejected"}, invalid},
                             workspace.Project()->Revision()));
    Check(workspace.Project()->Snapshot() == edited);
    ex::Take(workspace.Undo());
    Check(!workspace.Project()->IsDirty());
    ex::Take(workspace.Redo());
    Check(workspace.Project()->Snapshot() == edited);
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
    if (!cancelled)
        throw std::runtime_error("missing cancelled activation report");
    Check(activation->IsFinished() &&
          cancelled->status == runtime::graph::ExecutionStatus::Cancelled);
    ex::Take(workspace.Stop());
    Check(workspace.Project()->Snapshot() == edited && !workspace.Project()->IsDirty());
    ex::Take(workspace.Open(file));
    Check(workspace.Selection().kind == ed::UiElement::Document && !workspace.Session());
    ex::Take(workspace.Play());
    (void)ex::Take(workspace.Send(runtime::SceneNodeId{42}, {ex::Id("hit"), 1}));
    ex::Take(workspace.Tick({}));
    ex::Take(workspace.Tick(runtime::GameDuration{1}));
    const auto completed = workspace.Activations().at(0)->Report();
    if (!completed)
        throw std::runtime_error("missing completed activation report");
    Check(completed->status == runtime::graph::ExecutionStatus::Completed);
    ex::Take(workspace.Stop());
    Check(schema.entities.AllEntities().empty());
    ex::Take(workspace.Play());
    (void)ex::Take(workspace.Send(runtime::SceneNodeId{42},
                                  {ex::Id("hit"), std::numeric_limits<std::int64_t>::min()}));
    ex::Take(workspace.Tick({})); // Native failure is observable independently of Tick success.
    const auto failed = workspace.Activations().at(0)->Report();
    if (!failed || !failed->diagnostic)
        throw std::runtime_error("missing failed activation diagnostic");
    Check(failed->status == runtime::graph::ExecutionStatus::Failed);
    Check(failed->diagnostic->error.code == "example.damage_range");
    ex::Take(workspace.Stop());
    Check(!workspace.Project()->IsDirty());
    using K = comp::PropertyKind;
    for (const auto value : {"nan", "inf", "-inf", "", "1junk", "1,5"})
        Check(!ed::EditorWorkspace::ParseValue(K::Real, value));
    Check(!ed::EditorWorkspace::ParseValue(K::Real, std::string_view{}));
    for (const auto value : {"-9223372036854775808", "9223372036854775807"})
        Check(ed::EditorWorkspace::ParseValue(K::Integer, value).HasValue());
    Check(!ed::EditorWorkspace::ParseValue(K::Integer, "9223372036854775808"));
    Check(!ed::EditorWorkspace::ParseValue(K::Boolean, "1"));
    Check(std::get<std::string>(ex::Take(ed::EditorWorkspace::ParseValue(K::String, "  text  "))) ==
          "  text  ");
    for (const comp::PropertyValue value :
         {comp::PropertyValue{0.1}, comp::PropertyValue{std::int64_t{-45}},
          comp::PropertyValue{true}}) {
        const auto kind = static_cast<K>(value.index());
        Check(ex::Take(ed::EditorWorkspace::ParseValue(kind, ed::EditorWorkspace::FormatValue(
                                                                 value))) == value);
    }

}

} // namespace

void RuntimePlacement() {
    ex::Schema schema;
    auto definition = ex::Definition(schema);
    const auto scene_id = runtime::AssetId::FromString("example.scene");
    for (auto &document : definition.documents)
        if (document.id == scene_id) {
            auto &value = std::get<game::composition::SceneCompositionDefinition>(document.data);
            const auto base = value.scene.nodes.front();
            const auto prefab = value.placements.front().prefab;
            runtime::SceneNodeSnapshot parent = base;
            parent.id = {100};

            parent.parent = {};

            parent.local_transform = {};

            parent.local_transform.position = {10, 20, 0};

            parent.local_transform.scale = {2, 2, 1};

            value.scene.nodes = {parent};

            value.placements.clear();
            for (std::uint64_t i = 0; i < 3; ++i) {
                auto node = base;
                node.id = {42 + i};

                node.parent = parent.id;
                node.local_transform = {};

                node.local_transform.position.x = static_cast<float>(i) * 3 - 3;
                value.scene.nodes.push_back(node);
                value.placements.push_back({node.id, prefab, {}});
            }

        }

    ed::PlayContext *context = nullptr;
    ed::EditorWorkspace workspace{schema.View(), [&] {
                                      auto options = ex::Options(schema);
                                      auto factory = std::make_shared<decltype(options.bindings)>(
                                          std::move(options.bindings));
                                      options.bindings = [&context,
                                                          factory](ed::PlayContext &value,
                                                                   runtime::AssetId asset) {
                                          context = &value;
                                          return (*factory)(value, asset);
                                      };

                                      return options;
                                  }};

    const auto file =
        std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY} / "runtime-placement.epidemic";
    ex::Take(workspace.New(file, std::move(definition)));
    ex::Take(workspace.Save());
    const auto source = workspace.Project()->Snapshot();
    ex::Take(workspace.Play());
    Check(context != nullptr);
    std::array<gameplay::entities::EntityHandle, 3> owners{};

    for (std::uint64_t i = 0; i < 3; ++i) {
        const auto *instance = workspace.Session()->Instance(runtime::SceneNodeId{42 + i});
        Check(instance != nullptr);
        const auto *root = instance->Find(instance->Definition()->root.id);
        Check(root != nullptr);
        owners[i] = root->handle;
        const auto world = ex::Take(context->WorldTransform(root->handle));
        Check(world.position.x == 4 + static_cast<float>(i) * 6 && world.position.y == 20);
    }

    auto moved = ex::Take(context->WorldTransform(owners[1]));
    moved.position = {25, 30, 0};

    game::composition::SceneCompositionInstance empty;
    Check(!game::composition::BindSceneEntity(empty, context->entities, owners[0],
                                              runtime::SceneNodeId{1}));
    Check(!game::composition::GetEntityWorldTransform(empty, context->entities, owners[0]));
    ex::Take(context->SetWorldTransform(owners[1], moved));
    Check(ex::Take(context->WorldTransform(owners[1])).position.x == 25);
    Check(ex::Take(context->WorldTransform(owners[0])).position.x == 4);
    const auto entity = context->entities.Resolve(owners[1]);
    if (!entity)
        throw std::runtime_error("Runtime placement Entity missing");
    ex::Take(
        context->entities.RequestDestroy(entity->id,
                                         gameplay::entities::EntityDestroyReason::OwnerRequest));
    Check(!context->WorldTransform(owners[1]));
    ex::Take(workspace.Tick(runtime::GameDuration{0}));
    Check(context->scene->entities.size() == 2 && workspace.Project()->Snapshot() == source);
    ex::Take(workspace.Stop());
    Check(!workspace.IsDirty());
}

void FolderWorkspace() {
    ex::Schema schema;
    const auto root = std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY} / "workspace-folder";
    const auto clone =
        std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY} / "workspace-folder-clone";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::remove_all(clone, ec);
    const auto scene = runtime::AssetId::FromString("example.scene");
    {
        ed::EditorWorkspace workspace{schema.View(), [&] { return ex::Options(schema); }};

        ex::Take(workspace.NewFolder(root, "Folder UI", ex::Definition(schema)));
        Check(workspace.Folder() && !workspace.IsDirty());
        ex::Take(workspace.Execute({ed::RenameDocument{scene, "Folder scene"}},
                                   workspace.Project()->Revision()));
        Check(workspace.IsDirty() && !workspace.Open(root / "Project.epidemicproj"));
        ex::Take(workspace.Undo());
        Check(!workspace.IsDirty());
        ex::Take(workspace.Redo());
        Check(workspace.IsDirty());
        ex::Take(workspace.Save());
        Check(!workspace.IsDirty());
        ex::Take(workspace.CloneFolder(clone));
        ex::Take(workspace.Open(clone / "Project.epidemicproj"));
        Check(workspace.Folder() &&
              workspace.Project()->Snapshot()->Find(scene)->name == "Folder scene");
        ex::Take(workspace.Play());
        ex::Take(workspace.Stop());
        Check(!workspace.IsDirty());
#ifdef _WIN32
        constexpr unsigned char png[] = {137, 80, 78, 71,  13, 10,  26,  10,  0,   0,   0,   13,
                                         73,  72, 68, 82,  0,  0,   0,   1,   0,   0,   0,   1,
                                         8,   6,  0,  0,   0,  31,  21,  196, 137, 0,   0,   0,
                                         13,  73, 68, 65,  84, 120, 156, 99,  248, 207, 192, 208,
                                         0,   0,  4,  129, 1,  128, 44,  85,  206, 176, 0,   0,
                                         0,   0,  73, 69,  78, 68,  174, 66,  96,  130};

        const auto external = root / "external.png";
        {
            std::ofstream out(external, std::ios::binary);
            out.write(reinterpret_cast<const char *>(png), sizeof(png));
        }

        ex::Take(workspace.ImportPng(external, "sprite.png"));
        ex::Take(workspace.Save());
        const auto asset = workspace.Folder()->Sources().front();
        ex::Take(
            workspace.AssignPrefabSprite(runtime::AssetId::FromString("example.prefab"), asset.id));
        ex::Take(workspace.Save());
        workspace.EnableContentWatch(false);
        ex::Take(workspace.Play());
        ex::Take(workspace.Stop());
        ex::Take(workspace.ReimportPng(
            asset.id));              // Explicit jobs work with automatic watching disabled.
        Check(!workspace.IsDirty()); // Acknowledges queueing, not publication.
        const auto await = [&](ed::HotAssetEventKind kind) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};

            while (std::chrono::steady_clock::now() < deadline) {
                for (const auto &event : workspace.PollContent())
                    if (event.id == asset.id && event.kind == kind)
                        return true;
                std::this_thread::sleep_for(std::chrono::milliseconds{5});
            }

            return false;
        };

        Check(await(ed::HotAssetEventKind::Recovered));
        Check(workspace.Folder()->Sources().front().generation == asset.generation &&
              !workspace.IsDirty());
        ex::Take(workspace.Play());
        const auto frozen = ex::Take(workspace.Session()->ReadSprite(runtime::SceneNodeId{42}));
        Check(frozen.texture == asset.id && frozen.generation == asset.generation &&
              frozen.width == 1 && frozen.height == 1);
        ex::Take(workspace.ReimportPng(asset.id));
        Check(await(ed::HotAssetEventKind::Recovered));
        workspace.EnableContentWatch(true);
        const auto authored = workspace.Project()->Snapshot();
        constexpr unsigned char blue[] = {137, 80, 78, 71,  13, 10,  26,  10,  0,   0,  0,   13,
                                          73,  72, 68, 82,  0,  0,   0,   1,   0,   0,  0,   1,
                                          8,   6,  0,  0,   0,  31,  21,  196, 137, 0,  0,   0,
                                          13,  73, 68, 65,  84, 120, 156, 99,  96,  96, 248, 223,
                                          0,   0,  2,  131, 1,  128, 67,  187, 248, 3,  0,   0,
                                          0,   0,  73, 69,  78, 68,  174, 66,  96,  130};

        {
            std::ofstream out(clone / "Content/sprite.png", std::ios::binary);
            out.write(reinterpret_cast<const char *>(blue), sizeof(blue));
        }

        Check(await(ed::HotAssetEventKind::Updated));
        Check(workspace.Session()->IsPlaying() && workspace.Project()->Snapshot() == authored);
        const auto updated = workspace.Folder()->Sources().front();
        Check(updated.generation == asset.generation + 1 && updated.id == asset.id &&
              updated.cooked_key != asset.cooked_key);
        const auto stable = ex::Take(workspace.Session()->ReadSprite(runtime::SceneNodeId{42}));
        Check(stable.cooked_key == frozen.cooked_key && stable.generation == frozen.generation);
        ex::Take(workspace.Stop());
        Check(workspace.Session() && !workspace.Session()->IsPlaying() &&
              !workspace.Session()->ReadSprite(runtime::SceneNodeId{42}));
        ex::Take(workspace.Play());
        Check(ex::Take(workspace.Session()->ReadSprite(runtime::SceneNodeId{42})).cooked_key ==
              updated.cooked_key);
        ex::Take(workspace.Save());
        {
            std::ofstream out(clone / "Content/sprite.png", std::ios::binary);
            out << "broken";
        }

        ex::Take(workspace.ReimportPng(asset.id));
        Check(await(ed::HotAssetEventKind::Failed));
        ex::Take(workspace.Stop());
        Check(workspace.Folder()->Sources().front().cooked_key == updated.cooked_key &&
              !workspace.IsDirty());
#endif
    }

    ed::EditorWorkspace reopened{schema.View(), [&] { return ex::Options(schema); }};

    ex::Take(reopened.Open(root / "Project.epidemicproj"));
    Check(reopened.Project()->Snapshot()->Find(scene)->name == "Folder scene");
}

int main() {
    try {
        FolderWorkspace();
        RuntimePlacement();
        Spatial();
        Run();
        InspectorCases();
        PrefabCases();
        std::puts("PASS: workspace, schema Inspector, Prefab parameters/overrides, persistence and "
                  "runtime isolation");
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }

}
