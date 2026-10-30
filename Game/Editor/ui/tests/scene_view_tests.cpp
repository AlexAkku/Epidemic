#include "../../example/demo_game.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "Epidemic/Game/Editor/scene_view.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "../../../../EngineRuntime/Support/Presentation/renderer.h"
static_assert([] {
  epidemic::runtime::presentation::SpriteEntry sprite;
  return sprite.x == 0 && sprite.y == 0;
}());
#endif
#include <cmath>
#include <cstdio>
#include <limits>
#include <source_location>

using namespace epidemic;
namespace ed = game::editor;
namespace ex = ed::example;
void DragCases();
namespace {
void Check(bool value,
           const std::source_location at = std::source_location::current()) {
  if (!value)
    throw std::runtime_error("Scene View invariant at line " +
                             std::to_string(at.line()));
}
void Near(ed::ViewPoint a, ed::ViewPoint b) {
  Check(std::abs(a.x - b.x) < 1e-7 && std::abs(a.y - b.y) < 1e-7);
}
void Run() {
  ex::Schema schema;
  ed::EditorWorkspace workspace{schema.View(),
                                [&] { return ex::Options(schema); }};
  const runtime::AssetId scene = runtime::AssetId::FromString("example.scene"),
                         prefab =
                             runtime::AssetId::FromString("example.prefab");
  const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY
                                   "/scene-view.epidemic"};
  std::filesystem::create_directories(file.parent_path());
  ex::Take(workspace.New(file, ex::Definition(schema)));
  runtime::SceneNodeSnapshot second;
  second.id = runtime::SceneNodeId{43};
  second.local_transform.position = {5, 3, 0};
  second.local_bounds = runtime::Aabb{{-1, -1, 0}, {1, 1, 0}};
  runtime::SceneNodeSnapshot third;
  third.id = runtime::SceneNodeId{44};
  third.local_transform.position = {-4, 2, 0};
  runtime::SceneNodeSnapshot hidden;
  hidden.id = runtime::SceneNodeId{45};
  hidden.visibility = runtime::SceneVisibilityState::Hidden;
  ex::Take(workspace.Execute({ed::AddSceneNode{scene, third},
                              ed::AddSceneNode{scene, hidden},
                              ed::AddSceneNode{scene, second},
                              ed::PlacePrefab{scene, {second.id, prefab, {}}},
                              ed::PlacePrefab{scene, {third.id, prefab, {}}}},
                             workspace.Project()->Revision()));
  const auto snapshot = workspace.Project()->Snapshot();
  const auto revision = workspace.Project()->Revision();
  ed::SceneView view;
  view.Resize(600, 400);
  ex::Take(view.Update(*snapshot, scene));
  Check(view.Items().size() == 3 && view.Items()[0].id.Raw() == 42 &&
        view.Items()[2].id == third.id);
  Check(view.Items()[1].bounds.right - view.Items()[1].bounds.left == 2);
  for (const auto &item : view.Items()) {
    const ed::ViewPoint center{(item.bounds.left + item.bounds.right) * .5,
                               (item.bounds.top + item.bounds.bottom) * .5};
    Check(view.Pick(view.ToScreen(center)) == item.id);
    Check(item.label.starts_with("Enemy ["));
  }
  const auto point = view.ToScreen({5, 3});
  const auto anchor = view.ToWorld(point);
  view.Zoom(point, 3.5);
  Near(view.ToWorld(point), anchor);
  Check(view.Pick(point) == second.id);
  view.Pan({31, -25});
  Check(view.Pick(view.ToScreen({5, 3})) == second.id);
  for (const ed::ViewPoint extent :
       {ed::ViewPoint{320, 180}, ed::ViewPoint{900, 500},
        ed::ViewPoint{600, 400}}) {
    view.Resize(extent.x, extent.y);
    view.Fit();
    Check(view.Pick(view.ToScreen({5, 3})) == second.id);
  }
  view.Fit(second.id);
  Near(view.ToScreen({5, 3}), {300, 200});
  // Overlap winner is stable identity, independent of input order and
  // selection.
  ex::Take(workspace.Execute(
      {ed::SetSceneTransform{scene, third.id, second.local_transform}},
      revision));
  ex::Take(view.Update(*workspace.Project()->Snapshot(), scene));
  Check(view.Pick(view.ToScreen({5, 3})) == third.id);
  // Existing Runtime parenting computes this child's world translation.
  ex::Take(workspace.Execute({ed::SetSceneParent{scene, third.id, second.id}},
                             workspace.Project()->Revision()));
  ex::Take(view.Update(*workspace.Project()->Snapshot(), scene));
  view.Fit();
  Check(view.Pick(view.ToScreen({10, 6})) == third.id);
  Check(!view.Pick({-1, 0}).IsValid());
  view.Zoom({0, 0}, std::numeric_limits<double>::infinity());
  view.Pan({std::numeric_limits<double>::quiet_NaN(), 0});
  view.Fit();
  Check(std::isfinite(view.Scale()));
  Check(workspace.Project()
            ->IsDirty()); // Camera/selection never write authoring or history.
  ex::Take(workspace.Save());
  const auto saved = workspace.Project()->Snapshot();
  view.Pan({9, 8});
  view.Zoom({12, 13}, -2);
  view.Fit(second.id);
  Check(workspace.Project()->Snapshot() == saved &&
        !workspace.Project()->IsDirty());
  ex::Take(workspace.Open(file));
  ex::Take(view.Update(*workspace.Project()->Snapshot(), scene));
  view.Fit();
  Check(view.Pick(view.ToScreen({10, 6})) == third.id);
  ex::Take(workspace.Play());
  for (const runtime::SceneNodeId id :
       {runtime::SceneNodeId{42}, second.id, third.id}) {
    Check(workspace.Session()->Instance(id) != nullptr);
    Check(ex::Take(workspace.Send(id, {ex::Id("hit"), 1})) == 1);
  }
  ex::Take(workspace.Tick({}));
  Check(workspace.Activations().size() == 3);
  ex::Take(workspace.Tick(runtime::GameDuration{1}));
  for (const auto &activation : workspace.Activations())
    Check(activation->IsFinished());
  ex::Take(workspace.Stop());
  Check(schema.entities.AllEntities().empty() &&
        !workspace.Project()->IsDirty());
  auto missing = *snapshot;
  std::erase_if(missing.definition.documents,
                [&](const auto &doc) { return doc.id == prefab; });
  ex::Take(view.Update(missing, scene));
  Check(view.Items().size() == 3 &&
        view.Items()[0].label.starts_with("Missing Prefab ["));
  ex::Take(view.Update(*snapshot, prefab));
  Check(view.Items().empty() && !view.Document().IsValid());
}
} // namespace
int main() {
  try {
    Run();
    DragCases();
    std::puts("PASS: Scene View, transactional drag, gizmos, validation, "
              "history and Play isolation");
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
