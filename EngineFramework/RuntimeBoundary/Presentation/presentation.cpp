#include "host.h"
#include "renderer.h"
#include "sound.h"
#include <Epidemic/GameFramework/Presentation/presentation.h>
#include <Epidemic/GameFramework/Presentation/startup_sequence.h>
#include <algorithm>
#include <chrono>
namespace epidemic::gameplay::presentation {
namespace rt = epidemic::runtime::presentation;
struct Host::Impl {
  rt::Host engine;
  mutable InputSnapshot input;
  Canvas *canvas = nullptr;
  std::function<void(double)> audioTick;
  bool startupEnabled = true, startupMotion = rt::Host::AnimationEnabled(),
       startupBackground = false;
  double startupAge = 0;
  mutable std::array<bool, static_cast<std::size_t>(KeyCode::Count)>
      suppressed{};
  mutable bool suppressMouse = false;
  explicit Impl(std::string title) : engine(std::move(title)) {}
};
Host::Host(std::string title)
    : impl_(std::make_unique<Impl>(std::move(title))) {}
Host::~Host() = default;
void Host::SetAudioTick(std::function<void(double)> tick) {
  impl_->audioTick = std::move(tick);
}
void Host::ConfigureStartup(bool enabled, bool motion, bool background) {
  impl_->startupEnabled = enabled;
  impl_->startupMotion = motion;
  impl_->startupBackground = background;
}
double Host::StartupAge() const { return impl_->startupAge; }
void Host::Run(std::function<void(const std::vector<WindowEvent> &)> events,
               std::function<void()> tick) {
  auto previous = std::make_shared<std::chrono::steady_clock::time_point>(
      std::chrono::steady_clock::now());
  auto active = [this] {
    return impl_->startupEnabled && impl_->canvas &&
           !StartupSequence::At(impl_->startupAge, impl_->startupMotion)
                .complete;
  };
  impl_->engine.Run(
      [active, events = std::move(events)](const auto &source) {
        std::vector<WindowEvent> result;
        for (const auto &e : source) {
          if (active() && e.type == 3)
            continue; // Startup owns text/skip input.
          result.push_back({static_cast<EventType>(e.type), e.width, e.height,
                            e.focused, e.text});
        }
        events(result);
      },
      [this, active, previous, tick = std::move(tick)] {
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::min(
            .1, std::chrono::duration<double>(now - *previous).count());
        *previous = now;
        if (impl_->audioTick)
          impl_->audioTick(dt);
        if (active()) {
          const auto raw = impl_->engine.Input();
          for (std::size_t i = 0; i < impl_->suppressed.size(); ++i)
            impl_->suppressed[i] =
                impl_->suppressed[i] || raw.down[i] || raw.pressed[i];
          impl_->suppressMouse =
              impl_->suppressMouse || raw.mouseDown || raw.mousePressed;
          const bool skip = raw.pressed[static_cast<int>(KeyCode::Enter)] ||
                            raw.pressed[static_cast<int>(KeyCode::Escape)] ||
                            raw.pressed[static_cast<int>(KeyCode::Space)];
          if (impl_->startupBackground || (HasFocus() && !IsMinimized()))
            impl_->startupAge =
                skip ? StartupSequence::Duration : impl_->startupAge + dt;
          const auto state =
              StartupSequence::At(impl_->startupAge, impl_->startupMotion);
          if (state.logo) {
            auto &c = *impl_->canvas;
            c.Begin();
            c.DrawStartup(impl_->startupAge, impl_->startupMotion);
            c.End();
            return;
          }
        }
        tick();
      });
}
void Host::RequestStop() { impl_->engine.Stop(); }
bool Host::HasFocus() const { return impl_->engine.Focused(); }
bool Host::IsMinimized() const { return impl_->engine.Minimized(); }
const InputSnapshot &Host::CurrentSnapshot() const {
  const auto input = impl_->engine.Input();
  static_assert(static_cast<int>(KeyCode::Count) == std::size(input.down));
  for (std::size_t i = 0; i < std::size(input.down); ++i) {
    if (!input.down[i] && !input.pressed[i])
      impl_->suppressed[i] = false;
    impl_->input.keyboard.down[i] = input.down[i] && !impl_->suppressed[i];
    impl_->input.keyboard.pressed[i] =
        input.pressed[i] && !impl_->suppressed[i];
  }
  if (!input.mouseDown && !input.mousePressed)
    impl_->suppressMouse = false;
  const bool blocked =
      (impl_->canvas && impl_->startupEnabled) &&
      !StartupSequence::At(impl_->startupAge, impl_->startupMotion).complete;
  impl_->input.mouse.wheelDelta = blocked ? 0 : input.wheelDelta;
  impl_->input.mouse.pressed =
      input.mousePressed && !impl_->suppressMouse && !blocked;
  impl_->input.mouse.down =
      input.mouseDown && !impl_->suppressMouse && !blocked;
  return impl_->input;
}
std::filesystem::path Host::ExeDirectory() { return rt::Host::ExeDirectory(); }
std::filesystem::path Host::UserDirectory(std::string_view application) {
  return rt::Host::UserDirectory(application);
}
std::uint64_t Host::Seed() { return rt::Host::Seed(); }
std::vector<std::wstring> Host::Arguments() { return rt::Host::Arguments(); }
bool Host::AnimationEnabled() { return rt::Host::AnimationEnabled(); }
void Host::ShowError(std::string_view message) { rt::Host::ShowError(message); }
const wchar_t *ModeName(DisplayMode mode) {
  return rt::ModeName(static_cast<rt::DisplayMode>(mode));
}
namespace {
std::vector<rt::SpriteEntry> Sprites(std::span<const SpriteEntry> source) {
  std::vector<rt::SpriteEntry> result;
  for (const auto &s : source)
    result.push_back(
        {s.name, s.x, s.y, s.width, s.height, s.anchorX, s.anchorY});
  return result;
}
std::vector<rt::AudioEntry> Sounds(std::span<const AudioEntry> source) {
  std::vector<rt::AudioEntry> result;
  for (const auto &s : source)
    result.push_back({s.id, s.resource, s.gain, s.gap});
  return result;
}
rt::RendererPoint Native(Point p) { return {p.x, p.y}; }
std::vector<rt::RendererPoint> Native(const std::vector<Point> &source) {
  std::vector<rt::RendererPoint> result;
  for (auto p : source)
    result.push_back(Native(p));
  return result;
}
} // namespace
struct Canvas::Impl {
  rt::Renderer renderer;
  Impl(void *window, std::span<const SpriteEntry> sprites, int atlas)
      : renderer(window, Sprites(sprites), atlas) {}
};
Canvas::Canvas(Host &host, std::span<const SpriteEntry> sprites,
               int atlasResource)
    : host_(host),
      impl_(std::make_unique<Impl>(host.impl_->engine.WindowHandle(), sprites,
                                   atlasResource)) {
  if (!host_.impl_->canvas)
    host_.impl_->canvas = this;
}
Canvas::~Canvas() {
  if (host_.impl_->canvas == this)
    host_.impl_->canvas = nullptr;
}
DisplayMode Canvas::Mode() const {
  return static_cast<DisplayMode>(impl_->renderer.Mode());
}
void Canvas::SetMode(DisplayMode mode) {
  impl_->renderer.SetMode(static_cast<rt::DisplayMode>(mode));
}
void Canvas::PushTransform(const Matrix &transform, float opacity) {
  impl_->renderer.PushTransform({transform._11, transform._12, transform._21,
                                 transform._22, transform._31, transform._32},
                                opacity);
}
void Canvas::SetClientExtent(unsigned width, unsigned height) {
  impl_->renderer.SetClientExtent(width, height);
}
Point Canvas::ClientExtent() const {
  const auto p = impl_->renderer.ClientExtent();
  return {p.x, p.y};
}
Point Canvas::LogicalExtent() const {
  const auto p = impl_->renderer.LogicalExtent();
  return {p.x, p.y};
}
Point Canvas::MousePoint() const {
  const auto p = impl_->renderer.MousePoint();
  return {p.x, p.y};
}
struct Audio::Impl {
  rt::Sound sound;
  rt::Music music, cinematic;
  explicit Impl(std::span<const AudioEntry> entries) : sound(Sounds(entries)) {}
};
Audio::Audio(std::span<const AudioEntry> entries)
    : impl_(std::make_unique<Impl>(entries)) {}
Audio::~Audio() = default;
void Canvas::Resize() { impl_->renderer.Resize(); }
bool Canvas::IsExclusive() const { return impl_->renderer.IsExclusive(); }
void Canvas::EnableCaptures(bool enabled) {
  impl_->renderer.EnableCaptures(enabled);
}
void Canvas::Begin() { impl_->renderer.Begin(); }
void Canvas::End() {
  const auto &startup = *host_.impl_;
  const auto state =
      StartupSequence::At(startup.startupAge, startup.startupMotion);
  if (startup.canvas == this && startup.startupEnabled && !state.logo &&
      !state.complete) {
    const auto e = LogicalExtent(), c = ClientExtent();
    PushTransform(Matrix::Translation(-(e.x - std::min(1280.f, c.x)) * .5f,
                                      -(e.y - 720) * .5f));
    Rect(0, 0, e.x, e.y, 0, static_cast<float>(state.curtain));
    PopTransform();
  }
  impl_->renderer.End();
}
void Canvas::BeginContour() { impl_->renderer.BeginContour(); }
void Canvas::EndContour() { impl_->renderer.EndContour(); }
void Canvas::BeginBlur() { impl_->renderer.BeginBlur(); }
void Canvas::EndBlur(float radius) { impl_->renderer.EndBlur(radius); }
void Canvas::BeginPerspective(float rx, float ry, Point center) {
  impl_->renderer.BeginPerspective(rx, ry, {center.x, center.y});
}
void Canvas::EndPerspective() { impl_->renderer.EndPerspective(); }
void Canvas::PopTransform() { impl_->renderer.PopTransform(); }
void Canvas::Clip(float x, float y, float w, float h) {
  impl_->renderer.Clip(x, y, w, h);
}
void Canvas::Unclip() { impl_->renderer.Unclip(); }
void Canvas::ClipPolygon(const std::vector<Point> &points) {
  impl_->renderer.ClipPolygon(Native(points));
}
void Canvas::UnclipPolygon() { impl_->renderer.UnclipPolygon(); }
void Canvas::Polygon(const std::vector<Point> &points, unsigned color,
                     float alpha) {
  impl_->renderer.Polygon(Native(points), color, alpha);
}
void Canvas::Ellipse(float x, float y, float rx, float ry, unsigned color,
                     float thickness, float alpha) {
  impl_->renderer.Ellipse(x, y, rx, ry, color, thickness, alpha);
}
void Canvas::RoundRect(float x, float y, float w, float h, float radius,
                       unsigned color, float alpha) {
  impl_->renderer.RoundRect(x, y, w, h, radius, color, alpha);
}
void Canvas::Gradient(float x, float y, float w, float h, unsigned top,
                      unsigned bottom, float alpha) {
  impl_->renderer.Gradient(x, y, w, h, top, bottom, alpha);
}
void Canvas::Bezier(Point start, Point a, Point b, Point end, unsigned color,
                    float thickness, float alpha) {
  impl_->renderer.Bezier(Native(start), Native(a), Native(b), Native(end),
                         color, thickness, alpha);
}
void Canvas::Vignette(float w, float h) { impl_->renderer.Vignette(w, h); }
void Canvas::Rect(float x, float y, float w, float h, unsigned color,
                  float alpha) {
  impl_->renderer.Rect(x, y, w, h, color, alpha);
}
void Canvas::Line(float x, float y, float x2, float y2, unsigned color,
                  float thickness, float alpha) {
  impl_->renderer.Line(x, y, x2, y2, color, thickness, alpha);
}
void Canvas::Circle(float x, float y, float radius, unsigned color,
                    float alpha) {
  impl_->renderer.Circle(x, y, radius, color, alpha);
}
void Canvas::Outline(float x, float y, float radius, unsigned color,
                     float thickness, float alpha) {
  impl_->renderer.Outline(x, y, radius, color, thickness, alpha);
}
void Canvas::WarpedSprite(std::string_view name, float x, float y, float size,
                          float phase, float strength, float coreRadius,
                          float alpha) {
  impl_->renderer.WarpedSprite(name, x, y, size, phase, strength, coreRadius,
                               alpha);
}
void Canvas::Sprite(std::string_view name, float x, float y, float size,
                    float alpha, bool platform, float height, float rotation) {
  impl_->renderer.Sprite(name, x, y, size, alpha, platform, height, rotation);
}
void Canvas::Paragraph(std::wstring_view text, float x, float y, float size,
                       unsigned color, float width, float height, bool center) {
  impl_->renderer.Paragraph(text, x, y, size, color, width, height, center);
}
void Canvas::Text(std::wstring_view text, float x, float y, float size,
                  unsigned color, float width) {
  impl_->renderer.Text(text, x, y, size, color, width);
}
void Canvas::Capture(const std::filesystem::path &path) {
  impl_->renderer.Capture(path);
}
bool Audio::Enabled() const { return impl_->sound.Enabled(); }
void Audio::Play(const std::string &id, int rank, double gain, double x,
                 double heroX) {
  impl_->sound.Play(id, rank, gain, x, heroX);
}
void Audio::Tick(double dt, double volume) { impl_->sound.Tick(dt, volume); }
std::size_t Audio::Voices() const { return impl_->sound.Voices(); }
} // namespace epidemic::gameplay::presentation

namespace epidemic::gameplay::presentation {
bool Audio::LoadCinematic(int resource) {
  return impl_->cinematic.Load(resource, true);
}
void Audio::TickCinematic(double dt, bool active, double volume) {
  impl_->cinematic.Tick(dt, active, volume);
}
double Audio::CinematicPosition() const { return impl_->cinematic.Position(); }
bool Audio::LoadMusic(int resource) { return impl_->music.Load(resource); }
void Audio::TickMusic(double dt, bool menu, double volume) {
  impl_->music.Tick(dt, menu, volume);
}
std::array<float, 8> Audio::MusicBands() const { return impl_->music.Bands(); }
bool Audio::MusicPlaying() const { return impl_->music.Playing(); }
std::string Audio::MusicError() const {
  const auto error = impl_->music.Failure();
  return error.operation.empty()
             ? std::string{}
             : error.operation + " (" + std::to_string(error.nativeCode) + ")";
}
std::string Audio::CinematicError() const {
  const auto error = impl_->cinematic.Failure();
  return error.operation.empty()
             ? std::string{}
             : error.operation + " (" + std::to_string(error.nativeCode) + ")";
}
} // namespace epidemic::gameplay::presentation

namespace epidemic::gameplay::presentation {
void Canvas::Image(int resource, float x, float y, float width, float height,
                   float alpha) {
  impl_->renderer.Image(resource, x, y, width, height, alpha);
}
} // namespace epidemic::gameplay::presentation
