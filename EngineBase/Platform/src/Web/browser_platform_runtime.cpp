#include <Epidemic/Platform/browser_platform_runtime.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdexcept>
#include <utility>

namespace epidemic::platform {
namespace {
constexpr char Canvas[] = "#canvas";
class BrowserWindow final : public IWindow {
public:
  WindowId Id() const noexcept override { return 1; }
  std::string_view Title() const noexcept override { return title; }
  NativeWindowHandle GetNativeHandle() const noexcept override {
    return NativeWindowHandle{const_cast<char *>(Canvas)};
  }
  std::uint32_t ClientWidth() const noexcept override { return width; }
  std::uint32_t ClientHeight() const noexcept override { return height; }
  std::uint32_t Dpi() const noexcept override { return dpi; }
  bool HasFocus() const noexcept override { return focused; }
  bool IsMinimized() const noexcept override { return hidden; }
  bool IsCloseRequested() const noexcept override { return closed; }
  void Show() override {
    EM_ASM({ document.querySelector('#canvas').hidden = false; });
  }
  void Close() override { closed = true; }
  std::string title;
  std::uint32_t width = 1280, height = 720, dpi = 96;
  bool focused = false, hidden = false, closed = false;
};
foundation::Error Error(std::string_view code, std::string_view message) {
  return foundation::Error::Create(std::string(code), std::string(message));
}
} // namespace
struct BrowserPlatformRuntime::Impl {
  ProcessInfo process{.executable_path = foundation::Path{"/index.html"},
                      .working_directory = foundation::Path{"/"},
                      .arguments = {}};
  std::shared_ptr<BrowserWindow> window;
  std::vector<PlatformEvent> events;
  std::function<void(double)> frame;
  bool exit = false, running = false;
  double last = 0, css_width = 1280, css_height = 720;
  void Push(PlatformEvent event) {
    // Resynchronize input instead of keeping an unbounded backlog or losing a
    // release.
    if (events.size() >= 4096) {
      events.clear();
      events.push_back({.type = PlatformEventType::WindowFocusChanged,
                        .window_id = 1,
                        .focused = false});
      events.push_back({.type = PlatformEventType::WindowFocusChanged,
                        .window_id = 1,
                        .focused = window->focused});
    }
    if (event.type == PlatformEventType::MouseMoved && !events.empty() &&
        events.back().type == event.type) {
      event.mouse_delta_x += events.back().mouse_delta_x;
      event.mouse_delta_y += events.back().mouse_delta_y;
      events.back() = event;
    } else
      events.push_back(event);
  }
  void Resize() {
    if (!window)
      return;
    if (emscripten_get_element_css_size(Canvas, &css_width, &css_height) !=
        EMSCRIPTEN_RESULT_SUCCESS)
      return;
    const double ratio =
        std::clamp(emscripten_get_device_pixel_ratio(), .5, 4.);
    const auto width = static_cast<std::uint32_t>(
        std::clamp(std::round(css_width * ratio), 1., 4096.));
    const auto height = static_cast<std::uint32_t>(
        std::clamp(std::round(css_height * ratio), 1., 4096.));
    window->dpi = static_cast<std::uint32_t>(std::round(96 * ratio));
    if (width == window->width && height == window->height)
      return;
    window->width = width;
    window->height = height;
    emscripten_set_canvas_element_size(Canvas, static_cast<int>(width),
                                       static_cast<int>(height));
    Push({.type = PlatformEventType::WindowResized,
          .window_id = 1,
          .client_width = width,
          .client_height = height});
  }
  static EM_BOOL ResizeEvent(int, const EmscriptenUiEvent *, void *data) {
    static_cast<Impl *>(data)->Resize();
    return EM_FALSE;
  }
  static EM_BOOL FocusEvent(int type, const EmscriptenFocusEvent *,
                            void *data) {
    auto &self = *static_cast<Impl *>(data);
    if (!self.window)
      return EM_FALSE;
    self.window->focused = type == EMSCRIPTEN_EVENT_FOCUS;
    self.Push({.type = PlatformEventType::WindowFocusChanged,
               .window_id = 1,
               .focused = self.window->focused});
    return EM_FALSE;
  }
  static EM_BOOL VisibilityEvent(int,
                                 const EmscriptenVisibilityChangeEvent *event,
                                 void *data) {
    auto &self = *static_cast<Impl *>(data);
    if (!self.window)
      return EM_FALSE;
    self.window->hidden = event->hidden;
    self.last = emscripten_get_now();
    self.window->focused = !event->hidden && EM_ASM_INT({
      return document.querySelector('#canvas').matches(':focus') &&
             document.hasFocus();
    });
    self.Push({.type = event->hidden ? PlatformEventType::WindowMinimized
                                     : PlatformEventType::WindowRestored,
               .window_id = 1});
    self.Push({.type = PlatformEventType::WindowFocusChanged,
               .window_id = 1,
               .focused = self.window->focused});
    return EM_FALSE;
  }
  static EM_BOOL KeyEvent(int type, const EmscriptenKeyboardEvent *event,
                          void *data) {
    auto &self = *static_cast<Impl *>(data);
    if (!self.window || !self.window->focused)
      return EM_FALSE;
    self.Push({.type = type == EMSCRIPTEN_EVENT_KEYDOWN
                           ? PlatformEventType::KeyPressed
                           : PlatformEventType::KeyReleased,
               .window_id = 1,
               .key_code = static_cast<std::uint32_t>(event->keyCode),
               .scan_code = static_cast<std::uint32_t>(event->location),
               .repeated = static_cast<bool>(event->repeat)});
    return event->keyCode == 32 ||
           (event->keyCode >= 37 && event->keyCode <= 40);
  }
  static EM_BOOL MouseEvent(int type, const EmscriptenMouseEvent *event,
                            void *data) {
    auto &self = *static_cast<Impl *>(data);
    if (!self.window)
      return EM_FALSE;
    const double x = self.window->width / std::max(1., self.css_width),
                 y = self.window->height / std::max(1., self.css_height);
    auto button = static_cast<std::uint8_t>(event->button == 1   ? 2
                                            : event->button == 2 ? 1
                                                                 : 0);
    self.Push({.type = type == EMSCRIPTEN_EVENT_MOUSEDOWN
                           ? PlatformEventType::MouseButtonPressed
                       : type == EMSCRIPTEN_EVENT_MOUSEUP
                           ? PlatformEventType::MouseButtonReleased
                           : PlatformEventType::MouseMoved,
               .window_id = 1,
               .mouse_x = static_cast<std::int32_t>(event->targetX * x),
               .mouse_y = static_cast<std::int32_t>(event->targetY * y),
               .mouse_delta_x = static_cast<std::int32_t>(event->movementX * x),
               .mouse_delta_y = static_cast<std::int32_t>(event->movementY * y),
               .mouse_button = button});
    return EM_FALSE;
  }
  static EM_BOOL WheelEvent(int, const EmscriptenWheelEvent *event,
                            void *data) {
    auto &self = *static_cast<Impl *>(data);
    if (!self.window)
      return EM_FALSE;
    const double pixels =
        event->deltaY * (event->deltaMode == 1   ? 16
                         : event->deltaMode == 2 ? self.css_height
                                                 : 1);
    self.Push({.type = PlatformEventType::MouseWheel,
               .window_id = 1,
               .wheel_delta = static_cast<std::int32_t>(
                   std::clamp(-pixels, -32767., 32767.))});
    return EM_TRUE;
  }
  static void Tick(void *data) {
    auto &self = *static_cast<Impl *>(data);
    const double now = emscripten_get_now();
    const double dt = self.window && self.window->hidden
                          ? 0
                          : std::clamp((now - self.last) / 1000., 0., .1);
    self.last = now;
    // A callback may stop the loop, which clears self.frame.
    try {
      const auto frame = self.frame;
      if (frame)
        frame(dt);
    } catch (const std::exception &e) {
      emscripten_log(EM_LOG_ERROR, "Browser frame failed: %s", e.what());
      self.exit = true;
    } catch (...) {
      emscripten_log(EM_LOG_ERROR, "Browser frame failed");
      self.exit = true;
    }
    if (self.exit) {
      emscripten_cancel_main_loop();
      self.running = false;
    }
  }
};
BrowserPlatformRuntime::BrowserPlatformRuntime()
    : impl_(std::make_unique<Impl>()) {}
BrowserPlatformRuntime::~BrowserPlatformRuntime() {
  StopMainLoop();
  if (!impl_->window)
    return;
  emscripten_set_keydown_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_keyup_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_mousedown_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_mouseup_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_mousemove_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_wheel_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_focus_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_blur_callback(Canvas, nullptr, false, nullptr);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false,
                                 nullptr);
  emscripten_set_visibilitychange_callback(nullptr, false, nullptr);
}
std::string_view BrowserPlatformRuntime::Name() const { return "Browser"; }
const ProcessInfo &BrowserPlatformRuntime::GetProcessInfo() const {
  return impl_->process;
}
foundation::TimePoint BrowserPlatformRuntime::Now() const {
  return std::chrono::steady_clock::now();
}
foundation::Result<DynamicLibraryPtr>
BrowserPlatformRuntime::LoadDynamicLibrary(const foundation::Path &) {
  return foundation::Result<DynamicLibraryPtr>::Failure(
      Error("platform.web.dynamic_library_unsupported",
            "Browser bootstrap does not support native dynamic libraries"));
}
void BrowserPlatformRuntime::PumpEvents() {
  impl_->Resize();
  if (impl_->window && impl_->window->closed && !impl_->exit) {
    impl_->Push(
        {.type = PlatformEventType::WindowCloseRequested, .window_id = 1});
    impl_->exit = true;
  }
}
bool BrowserPlatformRuntime::IsExitRequested() const { return impl_->exit; }
foundation::Result<WindowPtr>
BrowserPlatformRuntime::CreateWindow(const WindowCreateInfo &info) {
  if (impl_->window || info.client_width == 0 || info.client_height == 0)
    return foundation::Result<WindowPtr>::Failure(
        Error("platform.web.invalid_window",
              "Browser bootstrap supports one nonzero-sized canvas"));
  if (!EM_ASM_INT({ return !!document.querySelector('#canvas'); }))
    return foundation::Result<WindowPtr>::Failure(
        Error("platform.web.canvas_missing",
              "The HTML shell must provide canvas#canvas"));
  impl_->window = std::make_shared<BrowserWindow>();
  impl_->window->title = info.title;
  EM_ASM(
      {
        document.title = UTF8ToString($0);
        const c = document.querySelector('#canvas');
        c.tabIndex = 0;
        c.hidden = !$1;
      },
      info.title.c_str(), info.visible);
  impl_->window->width = info.client_width;
  impl_->window->height = info.client_height;
  emscripten_set_canvas_element_size(Canvas,
                                     static_cast<int>(info.client_width),
                                     static_cast<int>(info.client_height));
  emscripten_set_keydown_callback(Canvas, impl_.get(), false, Impl::KeyEvent);
  emscripten_set_keyup_callback(Canvas, impl_.get(), false, Impl::KeyEvent);
  emscripten_set_mousedown_callback(Canvas, impl_.get(), false,
                                    Impl::MouseEvent);
  emscripten_set_mouseup_callback(Canvas, impl_.get(), false, Impl::MouseEvent);
  emscripten_set_mousemove_callback(Canvas, impl_.get(), false,
                                    Impl::MouseEvent);
  emscripten_set_wheel_callback(Canvas, impl_.get(), false, Impl::WheelEvent);
  emscripten_set_focus_callback(Canvas, impl_.get(), false, Impl::FocusEvent);
  emscripten_set_blur_callback(Canvas, impl_.get(), false, Impl::FocusEvent);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, impl_.get(),
                                 false, Impl::ResizeEvent);
  emscripten_set_visibilitychange_callback(impl_.get(), false,
                                           Impl::VisibilityEvent);
  // A launcher may focus the canvas before the runtime installs its callbacks.
  impl_->window->hidden = EM_ASM_INT({ return document.hidden; });
  impl_->window->focused = EM_ASM_INT({
    return !document.hidden &&
           document.querySelector('#canvas').matches(':focus') &&
           document.hasFocus();
  });
  impl_->Push({.type = PlatformEventType::WindowFocusChanged,
               .window_id = 1,
               .focused = impl_->window->focused});
  impl_->Resize();
  return foundation::Result<WindowPtr>::Success(impl_->window);
}
std::vector<PlatformEvent> BrowserPlatformRuntime::DrainEvents() {
  std::vector<PlatformEvent> events;
  events.swap(impl_->events);
  return events;
}
bool BrowserPlatformRuntime::HasPendingEvents() const noexcept {
  return !impl_->events.empty();
}
std::size_t BrowserPlatformRuntime::WindowCount() const noexcept {
  return impl_->window && !impl_->window->closed ? 1 : 0;
}
void BrowserPlatformRuntime::StartMainLoop(std::function<void(double)> frame) {
  if (!frame || impl_->running)
    throw std::logic_error(
        "Browser main loop requires a callback and a single owner");
  impl_->frame = std::move(frame);
  impl_->last = emscripten_get_now();
  impl_->running = true;
  impl_->exit = false;
  emscripten_set_main_loop_arg(Impl::Tick, impl_.get(), 0, false);
}
void BrowserPlatformRuntime::StopMainLoop() noexcept {
  if (impl_->running) {
    emscripten_cancel_main_loop();
    impl_->running = false;
  }
  impl_->frame = {};
}
} // namespace epidemic::platform
