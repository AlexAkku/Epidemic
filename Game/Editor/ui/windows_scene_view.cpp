#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows_scene_view.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Game/Editor/sprite_sheet.h"
#include "Epidemic/Runtime/Resources/resource_services.h"
#include "renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <commctrl.h>
#include <cstdio>
#include <d2d1_1helper.h>
#include <optional>
#include <set>
#include <unordered_map>
#include <windowsx.h>

namespace epidemic::game::editor {
namespace {
std::wstring Wide(std::string_view text) {
  const auto size = MultiByteToWideChar(
      CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      result.data(), size);
  return result;
}
struct ScreenSprite {
  D2D1::Matrix3x2F transform;
  D2D1_RECT_F rectangle;
};
std::optional<ScreenSprite> SpriteGeometry(const SceneView &view,
                                           const SceneViewItem &item,
                                           D2D1_SIZE_U dimensions,
                                           D2D1_POINT_2F pivot, float ppu) {
  const auto point =
      view.ToScreen({item.world.position.x, item.world.position.y});
  const D2D1_POINT_2F anchor{static_cast<float>(point.x),
                             static_cast<float>(point.y)};
  const float width = static_cast<float>(dimensions.width) *
                      std::abs(item.world.scale.x) *
                      static_cast<float>(view.Scale()) / ppu;
  const float height = static_cast<float>(dimensions.height) *
                       std::abs(item.world.scale.y) *
                       static_cast<float>(view.Scale()) / ppu;
  if (!std::isfinite(anchor.x) || !std::isfinite(anchor.y) ||
      !std::isfinite(width) || !std::isfinite(height) || width <= 0 ||
      height <= 0)
    return std::nullopt;
  const auto &q = item.world.rotation;
  const float angle = -static_cast<float>(
      std::atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z)) *
      180 / 3.141592653589793);
  const auto transform =
      D2D1::Matrix3x2F::Scale(item.world.scale.x < 0 ? -1.f : 1.f,
                              item.world.scale.y < 0 ? -1.f : 1.f, anchor) *
      D2D1::Matrix3x2F::Rotation(angle, anchor);
  const auto rect = D2D1::RectF(
      anchor.x - width * pivot.x, anchor.y - height * pivot.y,
      anchor.x + width * (1 - pivot.x), anchor.y + height * (1 - pivot.y));
  const std::array<D2D1_POINT_2F, 4> corners{
      transform.TransformPoint({rect.left, rect.top}),
      transform.TransformPoint({rect.right, rect.top}),
      transform.TransformPoint({rect.left, rect.bottom}),
      transform.TransformPoint({rect.right, rect.bottom})};
  float left = corners[0].x, right = left, top = corners[0].y, bottom = top;
  for (const auto &corner : corners) {
    if (!std::isfinite(corner.x) || !std::isfinite(corner.y))
      return std::nullopt;
    left = std::min(left, corner.x);
    right = std::max(right, corner.x);
    top = std::min(top, corner.y);
    bottom = std::max(bottom, corner.y);
  }
  if (right < 0 || bottom < 0 || left > view.Extent().x ||
      top > view.Extent().y)
    return std::nullopt;
  return ScreenSprite{transform, rect};
}
} // namespace
// Render-thread cache holds Resources leases and immutable texture generations.
// The authored binding references AssetId; the cache uses cooked digest keys.
struct NativeSceneView::SpriteCache {
  struct Resident {
    runtime::ResourceLease lease;
    runtime::presentation::RendererTexturePtr texture;
    std::uint32_t width = 0, height = 0;
  };
  std::filesystem::path root;
  std::unique_ptr<ImageResourceLoader> loader;
  runtime::ResourceServices resources;
  std::unordered_map<std::string, Resident> textures;
  struct Draw {
    runtime::AssetId texture{};
    std::string key, cache_key;
    std::uint64_t region = 0;
    float pivot_x = 0.5f, pivot_y = 0.5f, ppu = 64.0f;
  };
  std::unordered_map<std::uint64_t, Draw> placement_keys;
  explicit SpriteCache(std::filesystem::path path)
      : root(std::move(path)),
        loader(std::make_unique<ImageResourceLoader>(root)) {
    auto created =
        runtime::CreateResourceServices({std::size_t{64} * 1024 * 1024});
    if (!created)
      throw std::runtime_error("Scene sprite resources unavailable");
    resources = std::move(created).Value();
    auto registered = resources.loaders->RegisterLoader(*loader);
    if (!registered)
      throw std::runtime_error("Scene sprite loader registration failed");
    auto frozen = resources.loaders->Freeze();
    if (!frozen)
      throw std::runtime_error("Scene sprite registry freeze failed");
  }
  ~SpriteCache() {
    textures.clear(); // GPU texture wrappers destroyed on the renderer thread
  }
  void ReleaseAll() {
    for (auto &[key, r] : textures) {
      r.texture.reset();
      (void)resources.manager->Release(r.lease);
    }
    textures.clear();
    placement_keys.clear();
    loader->CollectUnused(*resources.manager);
  }
};
NativeSceneView::NativeSceneView(
    HWND window, EditorWorkspace &workspace,
    std::function<bool(runtime::SceneNodeId)> select,
    std::function<void(std::string)> error, std::function<void()> changed)
    : window_(window), workspace_(workspace), select_(std::move(select)),
      error_(std::move(error)), changed_(std::move(changed)) {
  if (!SetWindowSubclass(window_, &Procedure, 2,
                         reinterpret_cast<DWORD_PTR>(this)))
    throw std::runtime_error("Cannot initialize Scene View");
}
NativeSceneView::~NativeSceneView() {
  drag_.Cancel();
  if (GetCapture() == window_)
    ReleaseCapture();
  if (sprites_)
    sprites_->ReleaseAll();
  sprites_.reset();
  renderer_.reset();
  RemoveWindowSubclass(window_, &Procedure, 2);
}
void NativeSceneView::Sync(std::shared_ptr<const ProjectSnapshot> snapshot,
                           runtime::AssetId scene,
                           runtime::SceneNodeId selected, UINT dpi,
                           bool reset) {
  const bool changed =
      reset || snapshot_ != snapshot || view_.Document() != scene;
  const bool redraw = changed || selected_ != selected || dpi_ != dpi;
  if (drag_.Active() && (redraw || !drag_.Current(workspace_)))
    Cancel();
  dpi_ = dpi;
  selected_ = selected;
  Resize();
  editable_ =
      snapshot && SceneDrag::Supports(*snapshot, workspace_.Selection());
  if (changed) {
    if (reset)
      view_.Clear();
    snapshot_ = std::move(snapshot);
    if (snapshot_ && scene.IsValid()) {
      const auto updated = view_.Update(*snapshot_, scene);
      if (!updated) {
        failure_ = updated.GetError().code + ": " + updated.GetError().message;
        error_(failure_);
      } else
        failure_.clear();
    } else {
      view_.Clear();
      snapshot_.reset();
    }
  }
  if (redraw || workspace_.Folder())
    InvalidateRect(window_, nullptr, FALSE);
}
void NativeSceneView::Resize() {
  RECT rect{};
  GetClientRect(window_, &rect);
  view_.Resize(static_cast<double>(rect.right) * 96 / dpi_,
               static_cast<double>(rect.bottom) * 96 / dpi_);
}
ViewPoint NativeSceneView::Point(LPARAM position) const {
  return {static_cast<double>(GET_X_LPARAM(position)) * 96 / dpi_,
          static_cast<double>(GET_Y_LPARAM(position)) * 96 / dpi_};
}
void NativeSceneView::Frame(bool all) {
  Cancel();
  view_.Fit(all ? runtime::SceneNodeId{} : selected_);
  InvalidateRect(window_, nullptr, FALSE);
}
foundation::Result<void>
NativeSceneView::Configure(GizmoTool tool, GizmoSpace space, double snap) {
  if (!std::isfinite(snap) || snap < 0 ||
      (tool != GizmoTool::Move && space != GizmoSpace::Local))
    return foundation::Result<void>::Failure(foundation::Error::Create(
        "editor.gizmo_settings", "Snap must be finite and nonnegative; "
                                 "Rotate/Scale use Local coordinates."));
  Cancel();
  tool_ = tool;
  space_ = space;
  snap_ = snap;
  InvalidateRect(window_, nullptr, FALSE);
  return foundation::Result<void>::Success();
}
void NativeSceneView::Restore() {
  if (snapshot_ && view_.Document().IsValid()) {
    const auto restored = view_.Update(*snapshot_, view_.Document());
    if (!restored)
      error_(restored.GetError().code + ": " + restored.GetError().message);
  }
  InvalidateRect(window_, nullptr, FALSE);
}
void NativeSceneView::Cancel() {
  const bool active = drag_.Active();
  drag_.Cancel();
  arrow_keys_ = 0;
  panning_ = false;
  if (GetCapture() == window_)
    ReleaseCapture();
  if (active)
    Restore();
}
void NativeSceneView::Finish() {
  const auto committed = drag_.Commit(workspace_);
  arrow_keys_ = 0;
  if (GetCapture() == window_)
    ReleaseCapture();
  if (!committed || !committed.Value())
    Restore();
  if (!committed)
    error_(committed.GetError().code + ": " + committed.GetError().message);
  else
    error_({});
  changed_();
}
void NativeSceneView::Move(ViewPoint cursor) {
  if (!drag_.Current(workspace_)) {
    Cancel();
    error_("Gesture cancelled: source revision or selection changed.");
    return;
  }
  const auto moved = drag_.Move(view_, cursor, snap_);
  if (!moved)
    error_(moved.GetError().code + ": " + moved.GetError().message);
  InvalidateRect(window_, nullptr, FALSE);
}
std::optional<GizmoFrame> NativeSceneView::Gizmo() const {
  if (!editable_)
    return std::nullopt;
  for (const auto &item : view_.Items())
    if (item.id == selected_) {
      const auto frame = MakeGizmo(view_, item.world, space_);
      const auto extent = view_.Extent();
      if (frame.pivot.x >= 0 && frame.pivot.x <= extent.x &&
          frame.pivot.y >= 24 && frame.pivot.y <= extent.y - 24)
        return frame;
    }
  return std::nullopt;
}
void NativeSceneView::DrawGizmo() {
  const auto frame = Gizmo();
  if (!frame)
    return;
  const auto p = frame->pivot;
  const auto px = static_cast<float>(p.x), py = static_cast<float>(p.y);
  if (tool_ == GizmoTool::Rotate) {
    renderer_->Outline(px, py, 52, 0xffd078, 3);
    renderer_->Circle(px, py, 3, 0xffd078);
    return;
  }
  const auto axis = [&](ViewPoint end, unsigned color,
                        std::wstring_view label) {
    const auto x = static_cast<float>(end.x), y = static_cast<float>(end.y);
    renderer_->Line(px, py, x, y, color, 3);
    if (tool_ == GizmoTool::Scale)
      renderer_->Rect(x - 5, y - 5, 10, 10, color);
    else
      renderer_->Circle(x, y, 4, color);
    renderer_->Text(label, x + 7, y - 8, 13, color, 25);
  };
  axis(frame->x, 0xf49791, L"X");
  axis(frame->y, 0x84d5ae, L"Y");
  renderer_->Rect(px - 5, py - 5, 10, 10, 0xffd078);
}
void NativeSceneView::Capture(const std::filesystem::path &file) {
  Paint();
  if (!renderer_ || !failure_.empty())
    throw std::runtime_error("Scene View cannot capture a failed surface");
  renderer_->EnableCaptures(true);
  try {
    Paint();
    renderer_->Capture(file);
  } catch (...) {
    renderer_->EnableCaptures(false);
    throw;
  }
  renderer_->EnableCaptures(false);
}
void NativeSceneView::RefreshSprites() {
  const auto *folder = workspace_.Folder();
  if (!folder || !snapshot_) {
    if (sprites_)
      sprites_->ReleaseAll();
    sprites_.reset();
    return;
  }
  if (!renderer_)
    return;
  if (!sprites_ || sprites_->root != folder->Root()) {
    if (sprites_)
      sprites_->ReleaseAll();
    sprites_.reset();
    sprites_ = std::make_unique<SpriteCache>(folder->Root());
  }
  // Derived frame lookups only: ProjectFolder remains the authoritative index.
  std::unordered_map<std::uint64_t, const ProjectSourceAsset *> sources;
  for (const auto &source : folder->Sources())
    if (!source.IsTrashed())
      sources.emplace(source.id.Raw(), &source);
  std::unordered_map<std::uint64_t, const SpriteSheetDefinition *> sheets;
  for (const auto &sheet : folder->SpriteSheets())
    sheets.emplace(sheet.texture.Raw(), &sheet);
  std::unordered_map<std::uint64_t, SpriteCache::Draw> bindings;
  std::unordered_map<std::string, const sheets::Region *> crops;
  for (const auto &binding : folder->SpriteBindings()) {
    const auto source = sources.find(binding.texture.Raw());
    if (source == sources.end() || source->second->cooked_key.empty())
      continue;
    const auto &digest = source->second->cooked_key;
    auto key = digest + "/" + std::to_string(binding.region_id);
    if (binding.region_id) {
      const auto sheet = sheets.find(binding.texture.Raw());
      if (sheet == sheets.end()) {
        error_("editor.missing_subasset");
        continue;
      }
      const auto region =
          std::find_if(sheet->second->regions.begin(),
                       sheet->second->regions.end(), [&](const auto &r) {
                         return r.id == binding.region_id && !r.missing;
                       });
      if (region == sheet->second->regions.end()) {
        error_("editor.missing_subasset");
        continue;
      }
      key += "/" + std::to_string(region->x) + "," + std::to_string(region->y) +
             "," + std::to_string(region->width) + "," +
             std::to_string(region->height);
      crops.emplace(key, &*region);
    }
    bindings.emplace(binding.prefab.Raw(),
                     SpriteCache::Draw{binding.texture, digest, key,
                                       binding.region_id, binding.pivot_x,
                                       binding.pivot_y,
                                       binding.pixels_per_unit});
  }
  std::unordered_map<std::uint64_t, SpriteCache::Draw> desired;
  const auto *doc = snapshot_->Find(view_.Document());
  const auto *scene =
      doc ? std::get_if<composition::SceneCompositionDefinition>(&doc->data)
          : nullptr;
  if (scene)
    for (const auto &placement : scene->placements) {
      const auto binding = bindings.find(placement.prefab.Raw());
      if (binding != bindings.end())
        desired.emplace(placement.node.Raw(), binding->second);
    }
  std::set<std::string> needed;
  for (const auto &[node, draw] : desired)
    needed.insert(draw.cache_key);
  // Keep old leases/textures until every replacement has loaded and uploaded.
  for (const auto &key : needed) {
    if (sprites_->textures.contains(key))
      continue;
    const auto resource = sprites_->loader->Register(key.substr(0, 64));
    if (!resource) {
      error_(resource.GetError().code);
      continue;
    }
    const auto lease = sprites_->resources.manager->RequestLease(
        {resource.Value(), sprites_->loader->GetResourceType()});
    if (!lease) {
      error_(lease.GetError().code);
      continue;
    }
    const auto loaded = sprites_->resources.manager->ProcessPendingLoads();
    const auto payload = std::dynamic_pointer_cast<const ImagePayload>(
        sprites_->resources.manager->GetPayload(lease.Value().resource));
    if (!loaded || !payload) {
      (void)sprites_->resources.manager->Release(lease.Value());
      error_("Sprite cooked resource could not load");
      continue;
    }
    try {
      std::uint32_t width = payload->image.width,
                    height = payload->image.height;
      std::span<const std::byte> pixels = payload->image.pixels;
      std::vector<std::byte> cropped;
      if (const auto crop = crops.find(key); crop != crops.end()) {
        auto candidate = sheets::ExtractPbgra(payload->image.pixels, width,
                                              height, *crop->second);
        if (!candidate) {
          error_(candidate.error);
          (void)sprites_->resources.manager->Release(lease.Value());
          continue;
        }
        cropped = std::move(candidate.value);
        width = crop->second->width;
        height = crop->second->height;
        pixels = cropped;
      }
      auto texture = renderer_->CreateTexture(width, height, pixels);
      sprites_->textures.emplace(key, SpriteCache::Resident{lease.Value(),
                                                            std::move(texture),
                                                            width, height});
    } catch (const std::exception &error) {
      (void)sprites_->resources.manager->Release(lease.Value());
      error_(error.what());
    } catch (...) {
      (void)sprites_->resources.manager->Release(lease.Value());
      throw;
    }
  }
  for (auto &[node, draw] : desired)
    if (!sprites_->textures.contains(draw.cache_key)) {
      const auto previous = sprites_->placement_keys.find(node);
      if (previous != sprites_->placement_keys.end() &&
          previous->second.texture == draw.texture &&
          previous->second.region == draw.region &&
          sprites_->textures.contains(previous->second.cache_key))
        draw = previous->second;
    }
  needed.clear();
  for (const auto &[node, draw] : desired)
    needed.insert(draw.cache_key);
  sprites_->placement_keys.swap(desired);
  for (auto it = sprites_->textures.begin(); it != sprites_->textures.end();) {
    if (needed.contains(it->first)) {
      ++it;
      continue;
    }
    it->second.texture.reset();
    (void)sprites_->resources.manager->Release(it->second.lease);
    it = sprites_->textures.erase(it);
  }
  sprites_->loader->CollectUnused(*sprites_->resources.manager);
}
void NativeSceneView::DrawSprites() {
  if (!sprites_)
    return;
  for (const auto &item : view_.Items()) {
    const auto draw = sprites_->placement_keys.find(item.id.Raw());
    if (draw == sprites_->placement_keys.end())
      continue;
    const auto texture = sprites_->textures.find(draw->second.cache_key);
    if (texture == sprites_->textures.end())
      continue;
    const auto geometry = SpriteGeometry(
        view_, item, D2D1::SizeU(texture->second.width, texture->second.height),
        {draw->second.pivot_x, draw->second.pivot_y}, draw->second.ppu);
    if (!geometry)
      continue;
    const auto &rect = geometry->rectangle;
    renderer_->PushTransform(geometry->transform);
    renderer_->Texture(texture->second.texture, rect.left, rect.top,
                       rect.right - rect.left, rect.bottom - rect.top);
    renderer_->PopTransform();
  }
}
runtime::SceneNodeId NativeSceneView::Pick(ViewPoint point) const {
  if (!std::isfinite(point.x) || !std::isfinite(point.y) || point.x < 0 ||
      point.y < 0 || point.x > view_.Extent().x || point.y > view_.Extent().y)
    return {};
  if (!sprites_)
    return view_.Pick(point);
  for (auto item = view_.Items().rbegin(); item != view_.Items().rend();
       ++item) {
    const auto draw = sprites_->placement_keys.find(item->id.Raw());
    if (draw != sprites_->placement_keys.end())
      if (const auto texture = sprites_->textures.find(draw->second.cache_key);
          texture != sprites_->textures.end()) {
        auto geometry = SpriteGeometry(
            view_, *item,
            D2D1::SizeU(texture->second.width, texture->second.height),
            {draw->second.pivot_x, draw->second.pivot_y}, draw->second.ppu);
        if (!geometry || !geometry->transform.Invert())
          continue;
        const auto local = geometry->transform.TransformPoint(
            {static_cast<float>(point.x), static_cast<float>(point.y)});
        const auto &rect = geometry->rectangle;
        if (local.x >= rect.left && local.x <= rect.right &&
            local.y >= rect.top && local.y <= rect.bottom)
          return item->id;
        continue;
      }
    const auto rect = view_.ScreenBounds(*item);
    if (point.x >= rect.left && point.x <= rect.right && point.y >= rect.top &&
        point.y <= rect.bottom)
      return item->id;
  }
  return {};
}

void NativeSceneView::Paint() {
  if (!failure_.empty())
    return;
  RECT client{};
  GetClientRect(window_, &client);
  if (client.right <= 0 || client.bottom <= 0)
    return;
  Resize();
  if (!renderer_)
    renderer_ = std::make_unique<runtime::presentation::Renderer>(
        window_, std::span<const runtime::presentation::SpriteEntry>{}, 0);
  renderer_->Resize();
  RefreshSprites();
  const auto physical = renderer_->ClientExtent(),
             logical = renderer_->LogicalExtent();
  const float base = physical.x / logical.x,
              dpi = static_cast<float>(dpi_) / 96;
  const float offset = (physical.x - std::min(1280.f, physical.x) * base) * .5f;
  const float offset_y = (physical.y - 720.f * base) * .5f;
  renderer_->Begin();
  // Cancel Presentation's game letterboxing, then draw in editor DIPs.
  renderer_->PushTransform(
      D2D1::Matrix3x2F::Scale(dpi / base, dpi / base) *
      D2D1::Matrix3x2F::Translation(-offset / base, -offset_y / base));
  const auto extent = view_.Extent();
  const auto w = static_cast<float>(extent.x), h = static_cast<float>(extent.y);
  renderer_->Rect(0, 0, w, h, 0x17222e);
  const auto world = view_.ToWorld({0, 0});
  const double step = std::pow(10., std::floor(std::log10(80 / view_.Scale()))),
               pixel = step * view_.Scale();
  const auto origin = view_.ToScreen({0, 0});
  // Modulo in screen space avoids loops proportional to world coordinates.
  const auto grid = [&](bool vertical) {
    const double length = vertical ? extent.y : extent.x,
                 start = std::fmod(vertical ? origin.x : origin.y, pixel);
    const int count =
        static_cast<int>(std::min(2048., std::ceil(length / pixel) + 2));
    for (int index = 0; index < count; ++index) {
      const double value = start + index * pixel;
      if (value < 0 || value > length)
        continue;
      const auto v = static_cast<float>(value);
      renderer_->Line(vertical ? v : 0, vertical ? 0 : v, vertical ? v : w,
                      vertical ? h : v, 0x344351);
    }
  };
  grid(true);
  grid(false);
  if (origin.x >= 0 && origin.x <= extent.x)
    renderer_->Line(static_cast<float>(origin.x), 0,
                    static_cast<float>(origin.x), h, 0x78bca5, 2);
  if (origin.y >= 0 && origin.y <= extent.y)
    renderer_->Line(0, static_cast<float>(origin.y), w,
                    static_cast<float>(origin.y), 0xe2a387, 2);
  DrawSprites();
  for (const auto &item : view_.Items()) {
    const auto b = view_.ScreenBounds(item);
    if (b.right < 0 || b.bottom < 0 || b.left > extent.x || b.top > extent.y)
      continue;
    const auto x = static_cast<float>(std::clamp(b.left, 0., extent.x)),
               y = static_cast<float>(std::clamp(b.top, 0., extent.y));
    const auto right = static_cast<float>(std::clamp(b.right, 0., extent.x)),
               bottom = static_cast<float>(std::clamp(b.bottom, 0., extent.y));
    const unsigned color = item.id == selected_ ? 0xffd078 : 0x9ac7e5;
    renderer_->Rect(x, y, right - x, bottom - y, color,
                    item.id == selected_ ? .24f : .10f);
    renderer_->Line(x, y, right, y, color, 2);
    renderer_->Line(right, y, right, bottom, color, 2);
    renderer_->Line(right, bottom, x, bottom, color, 2);
    renderer_->Line(x, bottom, x, y, color, 2);
    const auto identity = item.label.rfind(" [");
    const auto label_x = std::clamp(x + 4, 8.f, std::max(8.f, w - 148));
    const auto label_y = std::max(26.f, y - 42);
    renderer_->Text(Wide(item.label.substr(0, identity)), label_x, label_y, 13,
                    color, 140);
    if (identity != std::string::npos)
      renderer_->Text(Wide(item.label.substr(identity + 1)), label_x,
                      label_y + 18, 13, color, 140);
  }
  DrawGizmo();
  char coordinates[128];
  std::snprintf(coordinates, sizeof(coordinates),
                "XY authoring | grid %.3g | X %.4g  Y %.4g", step, world.x,
                world.y);
  renderer_->Rect(0, 0, w, 24, 0x17222e, .95f);
  renderer_->Text(Wide(coordinates), 8, 4, 12, 0xe2eaf0, w - 16);
  renderer_->Rect(0, h - 24, w, 24, 0x17222e, .95f);
  renderer_->Text(drag_.Active()
                      ? L"Preview | release commit | Esc / RMB cancel"
                      : L"MMB pan | wheel zoom | arrows move | F / Home",
                  8, h - 21, 12, 0xe2eaf0, w - 16);
  renderer_->PopTransform();
  renderer_->End();
}
LRESULT CALLBACK NativeSceneView::Procedure(HWND hwnd, UINT msg, WPARAM wp,
                                            LPARAM lp, UINT_PTR,
                                            DWORD_PTR data) noexcept {
  auto *self = reinterpret_cast<NativeSceneView *>(data);
  try {
    return self->Message(msg, wp, lp);
  } catch (const std::exception &error) {
    try {
      self->failure_ = error.what();
      self->error_("Scene View: " + self->failure_);
      InvalidateRect(hwnd, nullptr, FALSE);
    } catch (...) {
      OutputDebugStringA(
          "Scene View: failed to publish rendering diagnostic\n");
    }
    return 0;
  }
}
LRESULT NativeSceneView::Message(UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_ERASEBKGND)
    return 1;
  if (msg == WM_PAINT) {
    PAINTSTRUCT paint{};
    const auto dc = BeginPaint(window_, &paint);
    // Always balance BeginPaint, including renderer/device failures.
    try {
      if (!failure_.empty()) {
        RECT rect{};
        GetClientRect(window_, &rect);
        FillRect(dc, &rect, GetSysColorBrush(COLOR_WINDOW));
        const auto text = Wide(failure_);
        DrawTextW(dc, text.c_str(), -1, &rect, DT_WORDBREAK);
      } else
        Paint();
    } catch (...) {
      EndPaint(window_, &paint);
      throw;
    }
    EndPaint(window_, &paint);
    return 0;
  }
  if (msg == WM_SIZE) {
    Cancel();
    Resize();
    InvalidateRect(window_, nullptr, FALSE);
    return 0;
  }
  if (msg == WM_LBUTTONDOWN) {
    Cancel();
    SetFocus(window_);
    const auto point = Point(lp);
    if (point.y < 24 || point.y > view_.Extent().y - 24)
      return 0;
    const auto frame = Gizmo();
    const auto handle =
        frame ? PickGizmo(*frame, tool_, point) : GizmoHandle::None;
    if (handle != GizmoHandle::None) {
      if (!select_(selected_))
        return 0;
      const auto begun =
          drag_.Begin(workspace_, view_, tool_, space_, handle, point);
      if (begun)
        SetCapture(window_);
      else
        error_(begun.GetError().message);
      return 0;
    }
    const auto hit = Pick(point);
    if (!select_(hit))
      return 0;
    if (hit.IsValid() && tool_ == GizmoTool::Move && editable_) {
      const auto begun = drag_.Begin(workspace_, view_, GizmoTool::Move, space_,
                                     GizmoHandle::Free, point);
      if (begun)
        SetCapture(window_);
      else
        error_(begun.GetError().message);
    }
    if (!hit.IsValid()) {
      panning_ = true;
      last_ = Point(lp);
      SetCapture(window_);
    }
    return 0;
  }
  if (msg == WM_MBUTTONDOWN) {
    Cancel();
    SetFocus(window_);
    panning_ = true;
    last_ = Point(lp);
    SetCapture(window_);
    return 0;
  }
  if (msg == WM_LBUTTONUP && drag_.Active() && arrow_keys_ == 0) {
    Move(Point(lp));
    if (drag_.Active())
      Finish();
    return 0;
  }
  if (msg == WM_MBUTTONUP || msg == WM_LBUTTONUP) {
    panning_ = false;
    if (GetCapture() == window_)
      ReleaseCapture();
    return 0;
  }
  if (msg == WM_CAPTURECHANGED || msg == WM_CANCELMODE ||
      msg == WM_RBUTTONDOWN || msg == WM_KILLFOCUS) {
    Cancel();
    return DefSubclassProc(window_, msg, wp, lp);
  }
  if (msg == WM_MOUSEMOVE && drag_.Active() && arrow_keys_ == 0) {
    Move(Point(lp));
    return 0;
  }
  if (msg == WM_MOUSEMOVE && panning_) {
    const auto next = Point(lp);
    view_.Pan({next.x - last_.x, next.y - last_.y});
    last_ = next;
    InvalidateRect(window_, nullptr, FALSE);
    return 0;
  }
  if (msg == WM_MOUSEWHEEL) {
    Cancel();
    POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    ScreenToClient(window_, &point);
    view_.Zoom({static_cast<double>(point.x) * 96 / dpi_,
                static_cast<double>(point.y) * 96 / dpi_},
               static_cast<double>(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA);
    InvalidateRect(window_, nullptr, FALSE);
    return 0;
  }
  if (msg == WM_KEYDOWN && wp == VK_ESCAPE) {
    Cancel();
    return 0;
  }
  if ((msg == WM_KEYDOWN || msg == WM_KEYUP) && wp >= VK_LEFT &&
      wp <= VK_DOWN) {
    const unsigned bit = 1u << static_cast<unsigned>(wp - VK_LEFT);
    if (msg == WM_KEYUP) {
      if (arrow_keys_ != 0) {
        arrow_keys_ &= ~bit;
        if (arrow_keys_ == 0 && drag_.Active())
          Finish();
      }
      return 0;
    }
    if (GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_MENU) < 0)
      return DefSubclassProc(window_, msg, wp, lp);
    if (drag_.Active() && arrow_keys_ == 0)
      return 0;
    if (!drag_.Active()) {
      if (!select_(selected_))
        return 0;
      keyboard_cursor_ = {view_.Extent().x * .5, view_.Extent().y * .5};
      const auto begun =
          drag_.Begin(workspace_, view_, GizmoTool::Move, GizmoSpace::World,
                      GizmoHandle::Free, keyboard_cursor_, 0);
      if (!begun) {
        error_(begun.GetError().message);
        return 0;
      }
    }
    arrow_keys_ |= bit;
    const double step = (snap_ > 0 ? snap_ : .1) *
                        (GetKeyState(VK_SHIFT) < 0 ? 10 : 1) * view_.Scale();
    if (wp == VK_LEFT)
      keyboard_cursor_.x -= step;
    if (wp == VK_RIGHT)
      keyboard_cursor_.x += step;
    if (wp == VK_UP)
      keyboard_cursor_.y -= step;
    if (wp == VK_DOWN)
      keyboard_cursor_.y += step;
    Move(keyboard_cursor_);
    return 0;
  }
  if (msg == WM_KEYDOWN && (wp == 'F' || wp == VK_HOME)) {
    Frame(wp == VK_HOME);
    return 0;
  }
  return DefSubclassProc(window_, msg, wp, lp);
}
} // namespace epidemic::game::editor
