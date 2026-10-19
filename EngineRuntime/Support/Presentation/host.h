#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace epidemic::runtime::presentation {
struct FrameInput {
    int wheelDelta=0;
    bool down[24]{}, pressed[24]{}, mousePressed = false, mouseDown = false;
};
struct WindowEvent {
    int type;
    unsigned width, height;
    bool focused;
    char text = 0;
};
class Host {
    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    explicit Host(std::string title);
    ~Host();
    void Run(std::function<void(const std::vector<WindowEvent> &)> events, std::function<void()> tick);
    void Stop();
    bool Focused() const;
    bool Minimized() const;
    FrameInput Input() const;
    void *WindowHandle() const;
    static std::filesystem::path ExeDirectory();
    static std::filesystem::path UserDirectory(std::string_view application);
    static std::uint64_t Seed();
    static std::vector<std::wstring> Arguments();
    static bool AnimationEnabled();
    static void ShowError(std::string_view message);
};
}
