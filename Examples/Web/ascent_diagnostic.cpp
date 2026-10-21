// External-project qualification adapter. The ASCENT simulation sources are unmodified.
// Canvas2D below is diagnostic geometry, not the engine's production Presentation backend.
#include "gameplay.h"
#include <Epidemic/EngineBase/engine_base_support.h>
#include <Epidemic/Platform/browser_platform_runtime.h>
#include <cstdio>
#include <emscripten.h>
#include <memory>
#include <stdexcept>
#include <utility>

namespace
{
using namespace epidemic;
core::Application app;
std::shared_ptr<platform::BrowserPlatformRuntime> host;
std::shared_ptr<platform::IWindow> window;
std::shared_ptr<input::IInputSystem> input_system;
std::unique_ptr<ascent::Run> game;
unsigned steps = 0;
double accumulator = 0;
bool jump = false, dash = false;
std::vector<float> geometry;
void Start()
{
    game = std::make_unique<ascent::Run>();
    game->Start(1917);
    game->godMode = true;
    game->player.weapons[0] = 2;
    accumulator = 0;
}
void Shape(double x, double y, double w, double h, unsigned color, bool circle = false)
{
    const double scale = 1280 / game->viewWidth;
    x = (x - game->camX) * scale;
    y = (y - game->camY) * scale;
    w *= scale;
    h *= scale;
    if (x + w < 0 || x - w > 1280 || y + h < 0 || y - h > 720)
        return;
    geometry.insert(geometry.end(), {static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                                     static_cast<float>(h), static_cast<float>(color), circle ? 1.f : 0.f});
}
void Draw()
{
    geometry.clear();
    for (const auto &p : game->world.platforms)
        if (p.active && p.opacity > .1)
            Shape(p.x, p.y, p.w, p.h, p.hazard ? 0xed7896 : 0x388e8e);
    for (const auto &h : game->world.hazards)
        if (h.active)
            Shape(h.x, h.y, h.r, h.r, 0xf1b765, true);
    for (const auto &e : game->enemies)
        if (e.hp > 0)
            Shape(e.x, e.y, e.r, e.r, e.boss ? 0xe8a767 : 0xb17deb, true);
    for (const auto &b : game->bullets)
        Shape(b.x, b.y, std::max(3., b.r), std::max(3., b.r), b.color, true);
    for (const auto &b : game->hostile)
        Shape(b.x, b.y, std::max(3., b.r), std::max(3., b.r), 0xff6485, true);
    for (const auto &g : game->gems)
        Shape(g.x, g.y, 4, 4, 0x98f0c5, true);
    Shape(game->player.x - ascent::PlayerW * .5, game->player.y, ascent::PlayerW, ascent::PlayerH, 0x9bedf1);
    EM_ASM(
        {
            const ctx = Module.canvas.getContext('2d');
            const c = Module.canvas;
            ctx.setTransform(c.width / 1280, 0, 0, c.height / 720, 0, 0);
            ctx.fillStyle = '#081722';
            ctx.fillRect(0, 0, 1280, 720);
            const a = HEAPF32.subarray($0 >> 2, ($0 >> 2) + $1);
            for (let i = 0; i < a.length; i += 6)
            {
                ctx.fillStyle = '#' + Math.round(a[i + 4]).toString(16).padStart(6, '0');
                if (a[i + 5])
                {
                    ctx.beginPath();
                    ctx.ellipse(a[i], a[i + 1], Math.max(1, a[i + 2]), Math.max(1, a[i + 3]), 0, 0, Math.PI * 2);
                    ctx.fill();
                }
                else
                    ctx.fillRect(a[i], a[i + 1], a[i + 2], a[i + 3]);
            }
            document.querySelector('#status').textContent = 'ASCENT 0.1.20 / Epidemic 1.0.1 · ' + $2 +
                                                            ' fixed steps · enemies ' + $3 + ' · altitude ' + $4 +
                                                            ' m · ' + ($5 ? 'focus' : 'paused / click canvas');
            document.querySelector('#upgrade').hidden = !$6;
        },
        geometry.data(), geometry.size(), steps, game->enemies.size(), static_cast<int>(game->Altitude()),
        window->HasFocus(), game->state == ascent::State::Upgrade);
}
} // namespace
extern "C"
{
    EMSCRIPTEN_KEEPALIVE unsigned ascent_web_steps()
    {
        return steps;
    }
    EMSCRIPTEN_KEEPALIVE double ascent_web_x()
    {
        return game->player.x;
    }
    EMSCRIPTEN_KEEPALIVE double ascent_web_y()
    {
        return game->player.y;
    }
    EMSCRIPTEN_KEEPALIVE unsigned ascent_web_enemies()
    {
        return static_cast<unsigned>(game->enemies.size());
    }
    EMSCRIPTEN_KEEPALIVE int ascent_web_state()
    {
        return static_cast<int>(game->state);
    }
    EMSCRIPTEN_KEEPALIVE void ascent_web_restart()
    {
        Start();
    }
    EMSCRIPTEN_KEEPALIVE void ascent_web_boss()
    {
        game->SpawnBoss(0);
    }
}
int main()
{
    try
    {
        static_cast<void>(enginebase::RegisterEngineBase(app, {.runtime_name = "ASCENT WASM diagnostic"}));
        host = std::dynamic_pointer_cast<platform::BrowserPlatformRuntime>(enginebase::RegisterBrowserRuntime(app));
        input_system = enginebase::RegisterInputRuntime(app);
        auto created = enginebase::CreateMainWindow(app, {.title = "ASCENT — C++ WebAssembly diagnostic"});
        if (!created.HasValue())
            throw std::runtime_error(created.GetError().message);
        window = std::move(created).Value();
        enginebase::RegisterPlatformFrameLoop(app);
        enginebase::RegisterInputFrameLoop(app);
        app.Bootstrap();
        app.Initialize();
        Start();
        host->StartMainLoop([](double dt) {
            app.Tick();
            const auto &keys = input_system->CurrentSnapshot().keyboard;
            using K = input::KeyCode;
            if (keys.WasPressedThisFrame(K::R))
                Start();
            if (keys.WasPressedThisFrame(K::B))
                game->SpawnBoss(0);
            if (game->state == ascent::State::Upgrade)
            {
                if (keys.WasPressedThisFrame(K::Digit1))
                    game->Upgrade(0);
                if (keys.WasPressedThisFrame(K::Digit2))
                    game->Upgrade(1);
                if (keys.WasPressedThisFrame(K::Digit3))
                    game->Upgrade(2);
            }
            if (!window->HasFocus() || window->IsMinimized())
            {
                accumulator = 0;
                jump = dash = false;
                Draw();
                return;
            }
            game->viewWidth = ascent::WorldWidth;
            game->viewHeight = ascent::WorldWidth * 720 / 1280;
            jump = jump || keys.WasPressedThisFrame(K::Space);
            dash = dash || keys.WasPressedThisFrame(K::Shift);
            ascent::Controls c;
            c.left = keys.IsKeyDown(K::A) || keys.IsKeyDown(K::Left);
            c.right = keys.IsKeyDown(K::D) || keys.IsKeyDown(K::Right);
            c.up = keys.IsKeyDown(K::W);
            c.drop = keys.IsKeyDown(K::S);
            c.holdJump = keys.IsKeyDown(K::Space);
            accumulator += dt * ascent::Run::SimulationSpeed;
            while (accumulator >= 1. / 60)
            {
                c.jump = std::exchange(jump, false);
                c.dash = std::exchange(dash, false);
                game->Step(1. / 60, c);
                accumulator -= 1. / 60;
                ++steps;
            }
            Draw();
        });
        return 0;
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "ASCENT WASM bootstrap failed: %s\n", e.what());
        EM_ASM({ document.querySelector('#status').textContent = 'Boot failed: ' + UTF8ToString($0); }, e.what());
        return 1;
    }
}
