#pragma once
#include "Epidemic/Game/Editor/editor_project.h"

namespace epidemic::game::editor {
struct ViewPoint { double x=0, y=0; };
struct ViewRect { double left=0, top=0, right=0, bottom=0; };
struct SceneViewItem {
    runtime::SceneNodeId id{};
    std::string label;
    ViewRect bounds; // World XY, with top <= bottom (Y increases upwards).
    runtime::Transform world;
};
// Disposable drawing/picking projection, never a second authoring Scene.
// Runtime Scene computes world TRS/bounds; no runtime handles escape Update.
class SceneView final {
  public:
    [[nodiscard]] foundation::Result<void> Update(const ProjectSnapshot &project, runtime::AssetId scene);
    [[nodiscard]] bool Preview(const composition::SceneCompositionInstance &scene);
    void Clear();
    void Resize(double width, double height); // Device independent pixels.
    void Pan(ViewPoint delta);
    void Zoom(ViewPoint cursor, double wheel_steps);
    void Fit(runtime::SceneNodeId selection={});
    [[nodiscard]] ViewPoint ToScreen(ViewPoint world) const;
    [[nodiscard]] ViewPoint ToWorld(ViewPoint screen) const;
    [[nodiscard]] ViewRect ScreenBounds(const SceneViewItem &item) const;
    [[nodiscard]] runtime::SceneNodeId Pick(ViewPoint screen) const;
    [[nodiscard]] const std::vector<SceneViewItem> &Items() const noexcept { return items_; }
    [[nodiscard]] double Scale() const noexcept { return scale_; }
    [[nodiscard]] ViewPoint Extent() const noexcept { return extent_; }
    [[nodiscard]] runtime::AssetId Document() const noexcept { return document_; }
  private:
    runtime::AssetId document_{};
    std::vector<SceneViewItem> items_;
    struct Geometry { ViewRect bounds; runtime::Transform world; };
    std::vector<Geometry> scratch_;
    ViewPoint center_{}, extent_{1,1};
    double scale_=40;
};
} // namespace epidemic::game::editor
