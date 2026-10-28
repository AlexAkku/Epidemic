#pragma once
#include "Epidemic/Game/Editor/scene_drag.h"
#include <windows.h>
#include <functional>
#include <memory>

namespace epidemic::runtime::presentation { class Renderer; }
namespace epidemic::game::editor {
// Child surface in the existing Platform window/message loop. Paint on demand.
class NativeSceneView final {
  public:
    NativeSceneView(HWND window,EditorWorkspace &workspace,std::function<bool(runtime::SceneNodeId)> select,
        std::function<void(std::string)> error,std::function<void()> changed);
    ~NativeSceneView();
    void Sync(std::shared_ptr<const ProjectSnapshot> snapshot, runtime::AssetId scene,
        runtime::SceneNodeId selected, UINT dpi, bool reset=false);
    void Frame(bool all);
    void Cancel();
    [[nodiscard]] foundation::Result<void> Configure(GizmoTool tool,GizmoSpace space,double snap);
    [[nodiscard]] double SnapStep() const noexcept { return snap_; }
    // Diagnostic capture uses the existing renderer, including its retained GPU frame.
    void Capture(const std::filesystem::path &file);
    [[nodiscard]] const SceneView &Projection() const noexcept { return view_; }
  private:
    static LRESULT CALLBACK Procedure(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR) noexcept;
    LRESULT Message(UINT,WPARAM,LPARAM);
    void Paint();
    void Resize();
    void Restore();
    void Finish();
    void Move(ViewPoint cursor);
    void DrawGizmo();
    [[nodiscard]] std::optional<GizmoFrame> Gizmo() const;
    [[nodiscard]] ViewPoint Point(LPARAM position) const;
    HWND window_;
    EditorWorkspace &workspace_;
    std::function<bool(runtime::SceneNodeId)> select_;
    std::function<void(std::string)> error_;
    std::function<void()> changed_;
    std::unique_ptr<runtime::presentation::Renderer> renderer_;
    std::shared_ptr<const ProjectSnapshot> snapshot_;
    SceneView view_;
    SceneDrag drag_;
    GizmoTool tool_=GizmoTool::Move;
    GizmoSpace space_=GizmoSpace::World;
    double snap_=0;
    unsigned arrow_keys_=0;
    ViewPoint keyboard_cursor_{};
    bool editable_=false;
    runtime::SceneNodeId selected_{};
    UINT dpi_=96;
    bool panning_=false;
    ViewPoint last_{};
    std::string failure_;
};
} // namespace epidemic::game::editor
