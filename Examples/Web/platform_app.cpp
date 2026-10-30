#include <Epidemic/EngineBase/engine_base_support.h>
#include <Epidemic/Platform/browser_platform_runtime.h>
#include <cstdio>
#include <emscripten.h>
#include <stdexcept>

namespace {
using namespace epidemic;
core::Application app;
std::shared_ptr<platform::BrowserPlatformRuntime> host;
std::shared_ptr<platform::IWindow> window;
std::shared_ptr<input::IInputSystem> input_system;
std::shared_ptr<rhi::IRhiSwapChain> swap;
auto paused = std::make_shared<bool>(false);
auto clear = std::make_shared<rhi::RhiClearDesc>();
unsigned frames = 0, key_events = 0, resizes = 0;

template <class T> T Take(foundation::Result<T> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
    return std::move(result).Value();
}

void Check(foundation::Result<void> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
}

} // namespace

extern "C" {
EMSCRIPTEN_KEEPALIVE unsigned epidemic_web_frames() {
    return frames;
}

EMSCRIPTEN_KEEPALIVE unsigned epidemic_web_key_events() {
    return key_events;
}

EMSCRIPTEN_KEEPALIVE unsigned epidemic_web_resizes() {
    return resizes;
}

EMSCRIPTEN_KEEPALIVE unsigned epidemic_web_width() {
    return window ? window->ClientWidth() : 0;
}

EMSCRIPTEN_KEEPALIVE int epidemic_web_focus() {
    return window && window->HasFocus();
}

EMSCRIPTEN_KEEPALIVE int epidemic_web_held() {
    return input_system && input_system->CurrentSnapshot().keyboard.IsKeyDown(input::KeyCode::A);
}

EMSCRIPTEN_KEEPALIVE void epidemic_web_stop() {
    host->StopMainLoop();
    app.Shutdown();
}

}

int main() {
    try {
        static_cast<void>(
            enginebase::RegisterEngineBase(app, {.runtime_name = "Epidemic Web 1.0.1"}));
        host = std::dynamic_pointer_cast<platform::BrowserPlatformRuntime>(
            enginebase::RegisterBrowserRuntime(app));
        input_system = enginebase::RegisterInputRuntime(app);
        window = Take(
            enginebase::CreateMainWindow(app, {.title = "Epidemic 1.0.1 — WebAssembly / WebGL2"}));
        auto graphics = Take(
            enginebase::RegisterGraphicsRuntime(app,
                                                {.backend = enginebase::GraphicsBackend::WebGL2}));
        swap = Take(enginebase::RegisterMainSwapChain(app, window,
                                                      {.width = window->ClientWidth(),
                                                       .height = window->ClientHeight(),
                                                       .color_format =
                                                           rhi::RhiPixelFormat::R8G8B8A8_UNorm}));
        enginebase::RegisterPlatformFrameLoop(app);
        enginebase::RegisterInputFrameLoop(app);
        enginebase::RegisterRhiFrameLoop(app, graphics.command_context, swap, paused, clear);
        app.Bootstrap();
        app.Initialize();
        host->StartMainLoop([](double) {
            *paused = window->IsMinimized();
            if (swap->Width() != window->ClientWidth() ||
                swap->Height() != window->ClientHeight()) {
                Check(swap->Resize(window->ClientWidth(), window->ClientHeight()));
                ++resizes;
            }

            const auto &keys = input_system->CurrentSnapshot().keyboard;
            clear->color = {keys.IsKeyDown(input::KeyCode::A) ? .32f : .025f,
                            keys.IsKeyDown(input::KeyCode::D) ? .5f : .12f,
                            keys.IsKeyDown(input::KeyCode::Space) ? .65f : .16f, 1};

            app.Tick();
            ++frames;
            key_events += static_cast<unsigned>(input_system->CurrentEvents().size());
            if (frames % 15 == 0) {
                EM_ASM(
                    {
                        document.querySelector('#status').textContent =
                            'Epidemic 1.0.1 · WASM · WebGL2 · ' + $0 + ' frames · ' + $1 +
                            ' input events';
                    },
                    frames, key_events);
            }

        });
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Web bootstrap failed: %s\n", e.what());
        EM_ASM(
            { document.querySelector('#status').textContent = 'Boot failed: ' + UTF8ToString($0); },
            e.what());
        return 1;
    }

}
