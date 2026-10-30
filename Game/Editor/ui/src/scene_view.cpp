#include "Epidemic/Game/Editor/scene_view.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <unordered_map>

namespace epidemic::game::editor {
namespace {
bool Finite(ViewPoint p) { return std::isfinite(p.x) && std::isfinite(p.y); }
std::string Identity(runtime::SceneNodeId id) {
  char text[32];
  const auto end =
      std::to_chars(std::begin(text), std::end(text), id.Raw(), 36).ptr;
  return {text, end};
}
} // namespace
foundation::Result<void> SceneView::Update(const ProjectSnapshot &project,
                                           runtime::AssetId scene) {
  using Result = foundation::Result<void>;
  const bool changed = document_ != scene;
  items_.clear();
  document_ = scene;
  const auto *doc = project.Find(scene);
  const auto *definition =
      doc ? std::get_if<composition::SceneCompositionDefinition>(&doc->data)
          : nullptr;
  if (!definition) {
    Clear();
    return Result::Success();
  }
  auto instance = composition::CreateSceneComposition(*definition);
  if (!instance)
    return Result::Failure(instance.GetError());
  std::unordered_map<runtime::SceneNodeId, std::string> names;
  for (const auto &placement : definition->placements) {
    const auto *prefab = project.Find(placement.prefab);
    names.emplace(placement.node, prefab ? prefab->name : "Missing Prefab");
  }
  for (const auto &node : definition->scene.nodes) {
    if (node.visibility == runtime::SceneVisibilityState::Hidden)
      continue;
    const auto id = instance.Value().nodes.at(node.id);
    const auto world = instance.Value().scene.transforms->GetWorldTransform(id);
    if (!world)
      continue;
    const auto bounds =
        instance.Value().scene.spatial->GetWorldBounds(id).value_or(
            runtime::TransformAabb(
                *world, runtime::Aabb{{-.5f, -.5f, -.5f}, {.5f, .5f, .5f}}));
    const ViewRect rect{bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y};
    if (!Finite({rect.left, rect.top}) || !Finite({rect.right, rect.bottom}))
      continue;
    const auto name = names.find(node.id);
    items_.push_back({node.id,
                      (name == names.end() ? "Scene node" : name->second) +
                          " [" + Identity(node.id) + "]",
                      rect, *world});
  }
  // Ascending identity is painter order; the greatest ID wins overlap picks.
  std::sort(items_.begin(), items_.end(), [](const auto &a, const auto &b) {
    return a.id.Raw() < b.id.Raw();
  });
  if (changed)
    Fit();
  return Result::Success();
}
bool SceneView::Preview(const composition::SceneCompositionInstance &scene) {
  scratch_.resize(items_.size());
  for (std::size_t i = 0; i < items_.size(); ++i) {
    const auto found = scene.nodes.find(items_[i].id);
    if (found == scene.nodes.end())
      return false;
    const auto world = scene.scene.transforms->GetWorldTransform(found->second);
    if (!world)
      return false;
    const auto bounds =
        scene.scene.spatial->GetWorldBounds(found->second)
            .value_or(runtime::TransformAabb(
                *world, runtime::Aabb{{-.5f, -.5f, -.5f}, {.5f, .5f, .5f}}));
    const ViewRect rect{bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y};
    if (!Finite({rect.left, rect.top}) || !Finite({rect.right, rect.bottom}))
      return false;
    scratch_[i] = {rect, *world};
  }
  for (std::size_t i = 0; i < items_.size(); ++i) {
    items_[i].bounds = scratch_[i].bounds;
    items_[i].world = scratch_[i].world;
  }
  return true;
}
void SceneView::Clear() {
  items_.clear();
  document_ = {};
  center_ = {};
  scale_ = 40;
}
void SceneView::Resize(double width, double height) {
  if (Finite({width, height}) && width > 0 && height > 0)
    extent_ = {width, height};
}
ViewPoint SceneView::ToScreen(ViewPoint p) const {
  return {(p.x - center_.x) * scale_ + extent_.x * .5,
          (center_.y - p.y) * scale_ + extent_.y * .5};
}
ViewPoint SceneView::ToWorld(ViewPoint p) const {
  return {(p.x - extent_.x * .5) / scale_ + center_.x,
          center_.y - (p.y - extent_.y * .5) / scale_};
}
void SceneView::Pan(ViewPoint delta) {
  if (!Finite(delta))
    return;
  const ViewPoint next{center_.x - delta.x / scale_,
                       center_.y + delta.y / scale_};
  if (Finite(next))
    center_ = next;
}
void SceneView::Zoom(ViewPoint cursor, double wheel_steps) {
  if (!Finite(cursor) || !std::isfinite(wheel_steps) || cursor.x < 0 ||
      cursor.y < 0 || cursor.x > extent_.x || cursor.y > extent_.y)
    return;
  const auto anchor = ToWorld(cursor);
  scale_ = std::clamp(
      scale_ * std::pow(1.2, std::clamp(wheel_steps, -20., 20.)), 1e-36, 1e6);
  const auto moved = ToWorld(cursor);
  center_.x += anchor.x - moved.x;
  center_.y += anchor.y - moved.y;
}
void SceneView::Fit(runtime::SceneNodeId selection) {
  std::optional<ViewRect> union_bounds;
  for (const auto &item : items_)
    if (!selection.IsValid() || item.id == selection) {
      if (!union_bounds)
        union_bounds = item.bounds;
      else {
        union_bounds->left = std::min(union_bounds->left, item.bounds.left);
        union_bounds->top = std::min(union_bounds->top, item.bounds.top);
        union_bounds->right = std::max(union_bounds->right, item.bounds.right);
        union_bounds->bottom =
            std::max(union_bounds->bottom, item.bounds.bottom);
      }
      if (selection.IsValid()) {
        union_bounds->left = std::min(
            union_bounds->left, static_cast<double>(item.world.position.x));
        union_bounds->right = std::max(
            union_bounds->right, static_cast<double>(item.world.position.x));
        union_bounds->top = std::min(
            union_bounds->top, static_cast<double>(item.world.position.y));
        union_bounds->bottom = std::max(
            union_bounds->bottom, static_cast<double>(item.world.position.y));
      }
    }
  if (!union_bounds) {
    if (!selection.IsValid()) {
      center_ = {};
      scale_ = 40;
    }
    return;
  }
  const auto &b = *union_bounds;
  center_ = {(b.left + b.right) * .5, (b.top + b.bottom) * .5};
  const double margin = selection.IsValid() ? 160 : 64;
  scale_ = std::clamp(
      std::min(
          std::max(1., extent_.x - margin) / std::max(1., b.right - b.left),
          std::max(1., extent_.y - margin) / std::max(1., b.bottom - b.top)),
      1e-36, 1e6);
}
ViewRect SceneView::ScreenBounds(const SceneViewItem &item) const {
  const auto a = ToScreen({item.bounds.left, item.bounds.bottom}),
             b = ToScreen({item.bounds.right, item.bounds.top});
  // The visible icon and its hit region share a minimum of 14 DIPs.
  const ViewPoint c{(a.x + b.x) * .5, (a.y + b.y) * .5};
  const double x = std::max(7., (b.x - a.x) * .5),
               y = std::max(7., (b.y - a.y) * .5);
  return {c.x - x, c.y - y, c.x + x, c.y + y};
}
runtime::SceneNodeId SceneView::Pick(ViewPoint p) const {
  if (!Finite(p) || p.x < 0 || p.y < 0 || p.x > extent_.x || p.y > extent_.y)
    return {};
  for (auto i = items_.rbegin(); i != items_.rend(); ++i) {
    const auto b = ScreenBounds(*i);
    if (p.x >= b.left && p.x <= b.right && p.y >= b.top && p.y <= b.bottom)
      return i->id;
  }
  return {};
}
} // namespace epidemic::game::editor
