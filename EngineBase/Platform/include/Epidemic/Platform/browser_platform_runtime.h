#pragma once
#include <Epidemic/Platform/iplatform_runtime.h>
#include <Epidemic/Platform/iwindow_system.h>
#include <functional>
#include <memory>

namespace epidemic::platform {
// One browser canvas, one owning runtime. Keep it alive until the last callback
// returns. File packages on Web are local virtual-filesystem data, not remote
// or durable saves.
class BrowserPlatformRuntime final : public IPlatformRuntime,
                                     public IWindowSystem {
public:
  BrowserPlatformRuntime();
  ~BrowserPlatformRuntime() override;
  BrowserPlatformRuntime(const BrowserPlatformRuntime &) = delete;
  BrowserPlatformRuntime &operator=(const BrowserPlatformRuntime &) = delete;
  std::string_view Name() const override;
  const ProcessInfo &GetProcessInfo() const override;
  foundation::TimePoint Now() const override;
  foundation::Result<DynamicLibraryPtr>
  LoadDynamicLibrary(const foundation::Path &path) override;
  void PumpEvents() override;
  bool IsExitRequested() const override;
  foundation::Result<WindowPtr>
  CreateWindow(const WindowCreateInfo &info) override;
  std::vector<PlatformEvent> DrainEvents() override;
  bool HasPendingEvents() const noexcept override;
  std::size_t WindowCount() const noexcept override;
  // requestAnimationFrame drives callbacks; dt is bounded and zero while
  // hidden.
  void StartMainLoop(std::function<void(double)> frame);
  void StopMainLoop() noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace epidemic::platform
