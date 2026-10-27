#include "Epidemic/Game/Editor/editor_workspace.h"
#include "../../example/demo_game.h"
#include <cstdio>
#include <limits>

using namespace epidemic;
namespace ed = game::editor;
namespace ex = ed::example;
namespace comp = gameplay::composition;
namespace {
void Check(bool value) { if (!value) throw std::runtime_error("workspace invariant"); }
void Run() {
    ex::Schema schema;
    ed::EditorWorkspace workspace{schema.View(), [&] { return ex::Options(schema); }};
    const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY "/ui.epidemic"};
    std::filesystem::create_directories(file.parent_path());
    ex::Take(workspace.New(file, ex::Definition(schema)));
    ex::Take(workspace.Save());
    const auto prefab = runtime::AssetId::FromString("example.prefab");
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
int main() {
    try { Run(); std::puts("PASS: workspace commands, persistence, runtime isolation, identity, typed values"); }
    catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
