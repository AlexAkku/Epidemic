#include "host.h"
#include <Epidemic/Core/application.h>
#include <Epidemic/EngineBase/engine_base_support.h>
#include <Epidemic/Input/key_code.h>
#include <Epidemic/Input/mouse_button.h>
#include <Windows.h>
#include <chrono>
#include <mmsystem.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdexcept>
#include <system_error>
#include <thread>
namespace epidemic::runtime::presentation {
namespace {
struct ComApartment {
  ComApartment() {
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
      throw std::runtime_error("Cannot initialize COM");
  }
  ~ComApartment() { CoUninitialize(); }
  ComApartment(const ComApartment &) = delete;
  ComApartment &operator=(const ComApartment &) = delete;
};
struct TimerPeriod {
  bool active = timeBeginPeriod(1) == TIMERR_NOERROR;
  ~TimerPeriod() {
    if (active)
      timeEndPeriod(1);
  }
};
} // namespace
struct Host::Impl {
  // Acquired before and released after all COM/window-dependent members.
  ComApartment com;
  TimerPeriod timer;
  core::Application application;
  std::shared_ptr<platform::IWindow> window;
  std::shared_ptr<input::IInputSystem> input;
  std::chrono::steady_clock::time_point started;
  explicit Impl(std::string title) : application({.application_name = title}) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    static_cast<void>(enginebase::RegisterEngineBase(
        application, {.runtime_name = "GameHost",
                      .log_module = "GameHost",
                      .memory_tracking_enabled = true}));
    static_cast<void>(enginebase::RegisterWindowsRuntime(application));
    input = enginebase::RegisterInputRuntime(application);
    auto created = enginebase::CreateMainWindow(
        application, {std::move(title), 1280, 720, true});
    if (!created.HasValue())
      throw std::runtime_error(created.GetError().message);
    window = std::move(created).Value();
    // An application may provide icon 101; generic SDK hosts retain their
    // default.
    const HWND hwnd = window->GetNativeHandle().As<HWND>();
    for (const auto size : {ICON_SMALL, ICON_BIG}) {
      const int pixels =
          GetSystemMetrics(size == ICON_SMALL ? SM_CXSMICON : SM_CXICON);
      const auto icon = static_cast<HICON>(
          LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101),
                     IMAGE_ICON, pixels, pixels, LR_SHARED));
      if (icon)
        SendMessageW(hwnd, WM_SETICON, size, reinterpret_cast<LPARAM>(icon));
    }
    enginebase::RegisterPlatformFrameLoop(application);
    enginebase::RegisterInputFrameLoop(application);
  }
};
Host::Host(std::string title)
    : impl_(std::make_unique<Impl>(std::move(title))) {}
Host::~Host() = default;
void Host::Run(std::function<void(const std::vector<WindowEvent> &)> events,
               std::function<void()> tick) {
  auto &app = impl_->application;
  const auto platformEvents =
      app.Services().Get<enginebase::FramePlatformEvents>();
  app.AddFramePhaseHandler(
      core::FramePhase::PumpPlatformEvents,
      [&, platformEvents](const auto &) {
        impl_->started = std::chrono::steady_clock::now();
        std::vector<WindowEvent> translated;
        for (const auto &e : platformEvents->events) {
          if (e.window_id != impl_->window->Id())
            continue;
          using platform::PlatformEventType;
          if (e.type == PlatformEventType::KeyPressed) {
            const auto key = e.key_code;
            char text = 0;
            if (key >= 65 && key <= 90)
              text = static_cast<char>(key + 32);
            else if (key >= 48 && key <= 57)
              text = static_cast<char>(key);
            else if (key >= 96 && key <= 105)
              text = static_cast<char>(key - 96 + 48);
            else if (key == 192 && !e.repeated)
              text = '`';
            else if (key == 189 || key == 109)
              text = '-';
            else if (key == 187 || key == 107)
              text = '+';
            else if (key == 190 || key == 110)
              text = '.';
            else if (key == 32 || key == 8 || key == 13 || key == 27 ||
                     key == 9)
              text = static_cast<char>(key);
            else if (key == 38 && !e.repeated)
              text = 1;
            else if (key == 40 && !e.repeated)
              text = 2;
            if (key == 189 &&
                impl_->input->CurrentSnapshot().keyboard.IsKeyDown(
                    input::KeyCode::Shift))
              text = '_';
            if (text)
              translated.push_back({3, 0, 0, true, text});
          }
          if (e.type == PlatformEventType::WindowCloseRequested)
            translated.push_back({0, 0, 0, false});
          if (e.type == PlatformEventType::WindowResized)
            translated.push_back({1, e.client_width, e.client_height, false});
          if (e.type == PlatformEventType::WindowFocusChanged)
            translated.push_back({2, 0, 0, e.focused});
        }
        events(translated);
      },
      "RuntimePresentation::Events");
  app.AddFramePhaseHandler(
      core::FramePhase::TickModules, [&](const auto &) { tick(); },
      "RuntimePresentation::Tick");
  app.AddFramePhaseHandler(
      core::FramePhase::EndFrame,
      [&](const auto &) {
        const auto deadline =
            impl_->started + std::chrono::nanoseconds(16666667);
        const auto early = deadline - std::chrono::microseconds(500);
        if (std::chrono::steady_clock::now() < early)
          std::this_thread::sleep_until(early);
        while (std::chrono::steady_clock::now() < deadline)
          std::this_thread::yield();
      },
      "RuntimePresentation::60FPS");
  app.Bootstrap();
  app.Initialize();
  try {
    app.Run();
  } catch (...) {
    app.Shutdown();
    throw;
  }
  app.Shutdown();
}
void Host::Stop() { impl_->application.RequestStop(); }
bool Host::Focused() const { return impl_->window->HasFocus(); }
bool Host::Minimized() const { return impl_->window->IsMinimized(); }
void *Host::WindowHandle() const {
  return impl_->window->GetNativeHandle().As<HWND>();
}
FrameInput Host::Input() const {
  using input::KeyCode;
  constexpr KeyCode keys[] = {
      KeyCode::A,       KeyCode::D,       KeyCode::S,      KeyCode::W,
      KeyCode::Left,    KeyCode::Right,   KeyCode::Down,   KeyCode::Up,
      KeyCode::Space,   KeyCode::Shift,   KeyCode::F11,    KeyCode::F3,
      KeyCode::M,       KeyCode::Escape,  KeyCode::P,      KeyCode::Enter,
      KeyCode::Digit1,  KeyCode::Digit2,  KeyCode::Digit3, KeyCode::Numpad1,
      KeyCode::Numpad2, KeyCode::Numpad3, KeyCode::Tab,    KeyCode::E};
  const auto &snapshot = impl_->input->CurrentSnapshot();
  FrameInput result;
  for (std::size_t i = 0; i < std::size(keys); ++i) {
    result.down[i] = snapshot.keyboard.IsKeyDown(keys[i]);
    result.pressed[i] = snapshot.keyboard.WasPressedThisFrame(keys[i]);
  }
  result.wheelDelta = snapshot.mouse.WheelDelta();
  result.mousePressed =
      snapshot.mouse.WasPressedThisFrame(input::MouseButton::Left);
  result.mouseDown = snapshot.mouse.IsButtonDown(input::MouseButton::Left);
  return result;
}
std::filesystem::path Host::ExeDirectory() {
  std::wstring path(32768, L'\0');
  const auto length =
      GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
  if (!length || length >= path.size())
    throw std::runtime_error("Cannot locate executable");
  path.resize(length);
  return std::filesystem::path(path).parent_path();
}
std::filesystem::path Host::UserDirectory(std::string_view application) {
  PWSTR path = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
    throw std::runtime_error("Cannot locate user data");
  auto result = std::filesystem::path(path) / application;
  CoTaskMemFree(path);
  std::filesystem::create_directories(result);
  return result;
}
std::uint64_t Host::Seed() { return GetTickCount64(); }
std::vector<std::wstring> Host::Arguments() {
  int count = 0;
  auto arguments = CommandLineToArgvW(GetCommandLineW(), &count);
  if (!arguments)
    throw std::runtime_error("Cannot parse command line");
  std::vector<std::wstring> result(arguments, arguments + count);
  LocalFree(arguments);
  return result;
}
bool Host::AnimationEnabled() {
  BOOL enabled = TRUE;
  return !SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0) ||
         enabled != FALSE;
}
void Host::ShowError(std::string_view message) {
  MessageBoxA(nullptr, std::string(message).c_str(), "Game launch failed",
              MB_OK | MB_ICONERROR);
}
} // namespace epidemic::runtime::presentation
