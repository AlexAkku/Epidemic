#include "host.h"
#include <Epidemic/EngineBase/engine_base_support.h>
#include <Epidemic/Platform/browser_platform_runtime.h>
#include <chrono>
#include <emscripten.h>
#include <stdexcept>

namespace epidemic::runtime::presentation
{
namespace {
EM_JS(void, InstallTextInput, (), {
    Module.epidemicTextQueue=[];
    Module.epidemicTextHandler=function(event){
        if(document.activeElement!=Module.canvas||event.ctrlKey||event.metaKey)return;
        const special={Backspace:8,Enter:13,Escape:27,Tab:9,ArrowUp:1,ArrowDown:2};
        let code=event.code=='Backquote'?96:special[event.key];
        if((event.key=='ArrowUp'||event.key=='ArrowDown')&&event.repeat)return;
        if(code==undefined&&event.key.length==1){const key=event.key.toLowerCase();code=key.charCodeAt(0);if(code<32||code>126)return;}
        if(code!=undefined&&Module.epidemicTextQueue.length<4096)Module.epidemicTextQueue.push(code);
    };
    Module.epidemicTextBlur=function(){Module.epidemicTextQueue.length=0;};
    Module.canvas.addEventListener('keydown',Module.epidemicTextHandler);
    Module.canvas.addEventListener('blur',Module.epidemicTextBlur);
});
EM_JS(int, DrainTextInput, (unsigned char *output,int capacity), {
    const queue=Module.epidemicTextQueue,count=Math.min(queue.length,capacity);
    HEAPU8.set(queue.splice(0,count),output);return count;
});
EM_JS(void, RemoveTextInput, (), {
    Module.canvas.removeEventListener('keydown',Module.epidemicTextHandler);
    Module.canvas.removeEventListener('blur',Module.epidemicTextBlur);
});
}
struct Host::Impl
{
    core::Application app;
    std::shared_ptr<platform::BrowserPlatformRuntime> browser;
    std::shared_ptr<platform::IWindow> window;
    std::shared_ptr<input::IInputSystem> input;
    double syncAge = 0;
    bool stopped = false;
    explicit Impl(std::string title)
    {
        static_cast<void>(enginebase::RegisterEngineBase(app, {.runtime_name = "BrowserGameHost"}));
        browser = std::dynamic_pointer_cast<platform::BrowserPlatformRuntime>(enginebase::RegisterBrowserRuntime(app));
        input = enginebase::RegisterInputRuntime(app);
        auto result = enginebase::CreateMainWindow(app, {std::move(title), 1280, 720, true});
        if (!result.HasValue())
            throw std::runtime_error(result.GetError().message);
        window = std::move(result).Value();
        enginebase::RegisterPlatformFrameLoop(app);
        enginebase::RegisterInputFrameLoop(app);
        InstallTextInput();
    }
};
Host::Host(std::string title) : impl_(std::make_unique<Impl>(std::move(title)))
{
}
Host::~Host()
{
    RemoveTextInput();
    impl_->browser->StopMainLoop();
    impl_->app.Shutdown();
}
void Host::Run(std::function<void(const std::vector<WindowEvent> &)> events, std::function<void()> tick)
{
    auto platformEvents = impl_->app.Services().Get<enginebase::FramePlatformEvents>();
    impl_->app.AddFramePhaseHandler(
        core::FramePhase::PumpPlatformEvents,
        [this, platformEvents, events = std::move(events)](const auto &) {
            std::vector<WindowEvent> translated;
            for (const auto &e : platformEvents->events)
            {
                using platform::PlatformEventType;
                if (e.type == PlatformEventType::WindowCloseRequested)
                    translated.push_back({0, 0, 0, false});
                if (e.type == PlatformEventType::WindowResized)
                    translated.push_back({1, e.client_width, e.client_height, false});
                if (e.type == PlatformEventType::WindowFocusChanged)
                    translated.push_back({2, 0, 0, e.focused});
            }
            unsigned char text[4096];
            const int count=DrainTextInput(text,4096);
            for(int i=0;i<count;++i)translated.push_back({3,0,0,true,static_cast<char>(text[i])});
            events(translated);
        },
        "BrowserPresentation::Events");
    impl_->app.AddFramePhaseHandler(
        core::FramePhase::TickModules, [tick = std::move(tick)](const auto &) { tick(); }, "BrowserPresentation::Game");
    impl_->app.Bootstrap();
    impl_->app.Initialize();
    impl_->browser->StartMainLoop([this](double dt) {
        impl_->app.Tick();
        impl_->syncAge += dt;
        // A game's closing tick can write its profile after RequestStop().
        if (impl_->stopped || impl_->syncAge >= 1)
        {
            impl_->syncAge = 0;
            EM_ASM({if(Module.epidemicSync)Module.epidemicSync();});
        }
    });
}
void Host::Stop()
{
    impl_->stopped = true;
    impl_->browser->StopMainLoop();
    EM_ASM({
        if(Module.epidemicAudio){
            for(const music of Module.epidemicAudio.music.values())music.audio.pause();
            for(const voice of Module.epidemicAudio.voices)voice.stop();
            Module.epidemicAudio.context.suspend().catch(console.error);
        }
        const status=document.querySelector('#status');
        if(status)status.textContent='Game stopped. Reload to restart.';
        if(Module.epidemicOnExit)Promise.resolve().then(()=>Module.epidemicOnExit()).catch(console.error);
        else if(document.fullscreenElement&&document.exitFullscreen)document.exitFullscreen().catch(console.error);
    });
}
bool Host::Focused() const
{
    return impl_->window->HasFocus();
}
bool Host::Minimized() const
{
    return impl_->window->IsMinimized();
}
void *Host::WindowHandle() const
{
    return impl_->window->GetNativeHandle().Value();
}
FrameInput Host::Input() const
{
    using input::KeyCode;
    constexpr KeyCode keys[] = {KeyCode::A,       KeyCode::D,       KeyCode::S,      KeyCode::W,      KeyCode::Left,
                                KeyCode::Right,   KeyCode::Down,    KeyCode::Up,     KeyCode::Space,  KeyCode::Shift,
                                KeyCode::F11,     KeyCode::F3,      KeyCode::M,      KeyCode::Escape, KeyCode::P,
                                KeyCode::Enter,   KeyCode::Digit1,  KeyCode::Digit2, KeyCode::Digit3, KeyCode::Numpad1,
                                KeyCode::Numpad2, KeyCode::Numpad3, KeyCode::Tab,    KeyCode::E};
    const auto &s = impl_->input->CurrentSnapshot();
    FrameInput result;
    for (std::size_t i = 0; i < std::size(keys); ++i)
    {
        result.down[i] = s.keyboard.IsKeyDown(keys[i]);
        result.pressed[i] = s.keyboard.WasPressedThisFrame(keys[i]);
    }
    result.wheelDelta = s.mouse.WheelDelta();
    result.mousePressed = s.mouse.WasPressedThisFrame(input::MouseButton::Left);
    result.mouseDown = s.mouse.IsButtonDown(input::MouseButton::Left);
    return result;
}
std::filesystem::path Host::ExeDirectory()
{
    return "/";
}
std::filesystem::path Host::UserDirectory(std::string_view application)
{
    auto p = std::filesystem::path("/saves") / application;
    std::filesystem::create_directories(p);
    return p;
}
std::uint64_t Host::Seed()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}
std::vector<std::wstring> Host::Arguments()
{
    return {L"ascent"};
}
bool Host::AnimationEnabled()
{
    return !EM_ASM_INT({ return matchMedia('(prefers-reduced-motion: reduce)').matches;});
}
void Host::ShowError(std::string_view message)
{
    std::string text(message);
    EM_ASM({ document.querySelector('#status').textContent = UTF8ToString($0);},text.c_str());
}
} // namespace epidemic::runtime::presentation
