#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace epidemic::gameplay::presentation {
struct Point {
  float x = 0, y = 0;
};
struct Matrix {
  float _11 = 1, _12 = 0, _21 = 0, _22 = 1, _31 = 0, _32 = 0;
  static Matrix Identity() { return {}; }
  static Matrix Translation(float x, float y) { return {1, 0, 0, 1, x, y}; }
  static Matrix Scale(float x, float y, Point center = {}) {
    return {x, 0, 0, y, center.x * (1 - x), center.y * (1 - y)};
  }
  static Matrix Rotation(float degrees, Point center = {}) {
    const float r = degrees * .0174532925199433f, c = std::cos(r),
                s = std::sin(r);
    return {c,
            s,
            -s,
            c,
            center.x * (1 - c) + center.y * s,
            center.y * (1 - c) - center.x * s};
  }
  Matrix operator*(const Matrix &b) const {
    return {
        _11 * b._11 + _12 * b._21,         _11 * b._12 + _12 * b._22,
        _21 * b._11 + _22 * b._21,         _21 * b._12 + _22 * b._22,
        _31 * b._11 + _32 * b._21 + b._31, _31 * b._12 + _32 * b._22 + b._32};
  }
};
enum class DisplayMode { Windowed, Borderless, Fullscreen };
const wchar_t *ModeName(DisplayMode mode);
struct SpriteEntry {
  std::string_view name;
  int x, y;
  int width = 256, height = 256, anchorX = 128, anchorY = 128;
};
struct AudioEntry {
  std::string_view id;
  int resource;
  double gain, gap;
};
enum class KeyCode {
  A,
  D,
  S,
  W,
  Left,
  Right,
  Down,
  Up,
  Space,
  Shift,
  F11,
  F3,
  M,
  Escape,
  P,
  Enter,
  Digit1,
  Digit2,
  Digit3,
  Numpad1,
  Numpad2,
  Numpad3,
  Tab,
  E,
  Count
};
enum class MouseButton { Left };
struct Keyboard {
  std::array<bool, static_cast<std::size_t>(KeyCode::Count)> down{}, pressed{};
  bool IsKeyDown(KeyCode code) const {
    return down[static_cast<std::size_t>(code)];
  }
  bool WasPressedThisFrame(KeyCode code) const {
    return pressed[static_cast<std::size_t>(code)];
  }
};
struct Mouse {
  int wheelDelta = 0;
  bool pressed = false, down = false;
  bool IsButtonDown(MouseButton) const { return down; }
  bool WasPressedThisFrame(MouseButton) const { return pressed; }
};
struct InputSnapshot {
  Keyboard keyboard;
  Mouse mouse;
};
enum class EventType {
  WindowCloseRequested,
  WindowResized,
  WindowFocusChanged,
  TextInput
};
struct WindowEvent {
  EventType type;
  unsigned client_width = 0, client_height = 0;
  bool focused = false;
  char text = 0;
};
class Canvas;
enum class BuildPlatform { Desktop, Browser };
struct PlatformCapabilities {
  BuildPlatform platform;
  bool displayModes;
};
class Host {
  struct Impl;
  std::unique_ptr<Impl> impl_;
  friend class Canvas;

public:
  static constexpr PlatformCapabilities Capabilities() {
#if defined(__EMSCRIPTEN__)
    return {BuildPlatform::Browser, false};
#else
    return {BuildPlatform::Desktop, true};
#endif
  }
  explicit Host(std::string title);
  ~Host();
  Host(const Host &) = delete;
  Host &operator=(const Host &) = delete;
  void Run(std::function<void(const std::vector<WindowEvent> &)> events,
           std::function<void()> tick);
  // Enabled by default for every graphical application. Disable for
  // tools/benchmarks.
  void SetAudioTick(std::function<void(double)> tick);
  void ConfigureStartup(bool enabled = true, bool motion = true,
                        bool advanceInBackground = false);
  double StartupAge() const;
  void RequestStop();
  bool HasFocus() const;
  bool IsMinimized() const;
  const InputSnapshot &CurrentSnapshot() const;
  static std::filesystem::path ExeDirectory();
  static std::filesystem::path UserDirectory(std::string_view application);
  static std::uint64_t Seed();
  static std::vector<std::wstring> Arguments();
  static bool AnimationEnabled();
  static void ShowError(std::string_view message);
};
class Canvas {
  friend class Host;
  Host &host_;
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  Canvas(Host &host, std::span<const SpriteEntry> sprites, int atlasResource);
  ~Canvas();
  void SetClientExtent(unsigned width, unsigned height);
  Point ClientExtent() const;
  Point LogicalExtent() const;
  void Resize();
  void SetMode(DisplayMode mode);
  DisplayMode Mode() const;
  bool IsExclusive() const;
  void EnableCaptures(bool enabled);
  void Begin();
  void End();
  // Shared engine artwork, also available for deterministic captures.
  void DrawStartup(double age, bool motion = true);
  void PushTransform(const Matrix &transform, float opacity = 1);
  void PopTransform();
  void BeginContour();
  void EndContour();
  void BeginBlur();
  void EndBlur(float radius = 2.2f);
  void BeginPerspective(float rx, float ry, Point center);
  void EndPerspective();
  void Clip(float x, float y, float w, float h);
  void Unclip();
  void ClipPolygon(const std::vector<Point> &points);
  void UnclipPolygon();
  void Polygon(const std::vector<Point> &points, unsigned color,
               float alpha = 1);
  void Ellipse(float x, float y, float rx, float ry, unsigned color,
               float thickness = 2, float alpha = 1);
  void RoundRect(float x, float y, float w, float h, float radius,
                 unsigned color, float alpha = 1);
  void Gradient(float x, float y, float w, float h, unsigned top,
                unsigned bottom, float alpha = 1);
  void Bezier(Point start, Point a, Point b, Point end, unsigned color,
              float thickness = 2, float alpha = 1);
  void Vignette(float w, float h);
  void Rect(float x, float y, float w, float h, unsigned color,
            float alpha = 1);
  void Line(float x, float y, float x2, float y2, unsigned color,
            float thickness = 1, float alpha = 1);
  void Circle(float x, float y, float radius, unsigned color, float alpha = 1);
  void Outline(float x, float y, float radius, unsigned color,
               float thickness = 2, float alpha = 1);
  // Continuous displacement of one premultiplied sprite, with an anchored core.
  void WarpedSprite(std::string_view name, float x, float y, float size,
                    float phase, float strength, float coreRadius,
                    float alpha = 1);
  void Image(int resource, float x, float y, float width, float height,
             float alpha = 1);
  void Sprite(std::string_view name, float x, float y, float size = 256,
              float alpha = 1, bool platform = false, float height = 256,
              float rotation = 0);
  void Paragraph(std::wstring_view text, float x, float y, float size,
                 unsigned color, float width, float height,
                 bool center = false);
  void Text(std::wstring_view text, float x, float y, float size,
            unsigned color, float width = 1100);
  Point MousePoint() const;
  void Capture(const std::filesystem::path &path);
};
class Audio {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit Audio(std::span<const AudioEntry> entries);
  ~Audio();
  bool LoadMusic(int resource);
  bool LoadCinematic(int resource);
  void TickCinematic(double dt, bool active, double volume);
  double CinematicPosition() const;
  void TickMusic(double dt, bool menu, double volume);
  std::array<float, 8> MusicBands() const;
  bool MusicPlaying() const;
  std::string MusicError() const;
  std::string CinematicError() const;
  bool Enabled() const;
  void Play(const std::string &id, int rank, double gain, double x,
            double heroX);
  void Tick(double dt, double volume);
  std::size_t Voices() const;
};
} // namespace epidemic::gameplay::presentation
