#pragma once
#include "Epidemic/Game/Editor/scene_view.h"
#include <windows.h>
#include <functional>
#include <memory>

namespace epidemic::runtime::presentation { class Renderer; }
namespace epidemic::game::editor {
// Child surface in the existing Platform window/message loop. Paint on demand.
class NativeSceneView final {
  public:
    NativeSceneView(HWND window, std::function<void(runtime::SceneNodeId)> select,
        std::function<void(std::string)> error);
    ~NativeSceneView();
    void Sync(std::shared_ptr<const ProjectSnapshot> snapshot, runtime::AssetId scene,
        runtime::SceneNodeId selected, UINT dpi, bool reset=false);
    void Frame(bool all);
    // Diagnostic capture uses the existing renderer, including its retained GPU frame.
    void Capture(const std::filesystem::path &file);
    [[nodiscard]] const SceneView &Projection() const noexcept { return view_; }
  private:
    static LRESULT CALLBACK Procedure(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR) noexcept;
    LRESULT Message(UINT,WPARAM,LPARAM);
    void Paint();
    void Resize();
    [[nodiscard]] ViewPoint Point(LPARAM position) const;
    HWND window_;
    std::function<void(runtime::SceneNodeId)> select_;
    std::function<void(std::string)> error_;
    std::unique_ptr<runtime::presentation::Renderer> renderer_;
    std::shared_ptr<const ProjectSnapshot> snapshot_;
    SceneView view_;
    runtime::SceneNodeId selected_{};
    UINT dpi_=96;
    bool panning_=false;
    ViewPoint last_{};
    std::string failure_;
};
} // namespace epidemic::game::editor
