#pragma once
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "Epidemic/Game/Editor/scene_view.h"

namespace epidemic::game::editor {
enum class GizmoTool { Move, Rotate, Scale };
enum class GizmoSpace { World, Local };
enum class GizmoHandle { None, Free, X, Y, Uniform, Ring };
struct GizmoFrame { ViewPoint pivot, x, y; };
[[nodiscard]] GizmoFrame MakeGizmo(const SceneView &view,const runtime::Transform &world,GizmoSpace space);
[[nodiscard]] GizmoHandle PickGizmo(const GizmoFrame &frame,GizmoTool tool,ViewPoint cursor);

// Owns a transient Runtime Scene and an immutable source reference for one gesture.
// Move never executes an authoring command. Commit publishes at most one command.
class SceneDrag final {
  public:
    [[nodiscard]] static bool Supports(const ProjectSnapshot &snapshot,const UiSelection &selection);
    [[nodiscard]] foundation::Result<void> Begin(const EditorWorkspace &workspace,const SceneView &view,
        GizmoTool tool,GizmoSpace space,GizmoHandle handle,ViewPoint cursor,double threshold=3);
    [[nodiscard]] foundation::Result<void> Move(SceneView &view,ViewPoint cursor,double snap=0);
    [[nodiscard]] foundation::Result<bool> Commit(EditorWorkspace &workspace);
    void Cancel() noexcept;
    [[nodiscard]] bool Active() const noexcept { return scene_!=nullptr; }
    [[nodiscard]] bool Current(const EditorWorkspace &workspace) const;
  private:
    std::unique_ptr<composition::SceneCompositionInstance> scene_;
    std::shared_ptr<const ProjectSnapshot> source_;
    UiSelection selection_;
    std::uint64_t revision_=0;
    runtime::SceneNodeId runtime_node_{};
    runtime::Transform original_, world_, candidate_;
    GizmoTool tool_=GizmoTool::Move;
    GizmoSpace space_=GizmoSpace::World;
    GizmoHandle handle_=GizmoHandle::None;
    ViewPoint start_, pivot_, x_{1,0}, y_{0,1};
    double scale_=1, threshold_=3;
    bool moved_=false, valid_=true;
};
} // namespace epidemic::game::editor
