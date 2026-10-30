#include <Epidemic/GameFramework/Presentation/presentation.h>
#include <Epidemic/GameFramework/Session/profile_store.h>
#include <Epidemic/build_info.h>
#include <Epidemic/GameFramework/Presentation/rig_2d.h>
#include <Epidemic/GameFramework/Presentation/ui_2d.h>
#include <Epidemic/GameFramework/Presentation/frame_clocks.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
void RunRigPreview() {
    using namespace epidemic::gameplay::presentation;
    Host host("Epidemic 2D rig preview");
    Canvas canvas(host, {}, 0);
    host.ConfigureStartup(false);
    const std::array<CutoutBone, 3> bones{{{-1, 0, 0, 0}, {0, 0, -70, 0}, {1, 65, 0, 0}}};

    const std::array<CutoutKey, 3> keys{{{0, -35, 0, 0}, {.5, 35, 0, 0}, {1, -35, 0, 0}}};

    const std::array<CutoutTrack, 3> tracks{{{}, CutoutTrack(keys), {}}};

    const std::array<UiTarget, 2> buttons{{{1, {90, 90, 260, 65}}, {2, {380, 90, 260, 65}}}};

    const std::array<LocalizedUiText, 2> labels{
        {{"en", L"mirror", L"Mirror"}, {"en", L"pause", L"Pause"}}};

    UiFocusModel focus;
    focus.Update(buttons);
    focus.Focus(1);
    FrameClocks clocks;
    bool mirrored = false, paused = false;
    auto previous = std::chrono::steady_clock::now();
    host.Run(
        [&](const auto &events) {
            for (const auto &event : events) {
                if (event.type == EventType::WindowCloseRequested)
                    host.RequestStop();
                if (event.type == EventType::WindowResized)
                    canvas.Resize();
            }

        },
        [&] {
            const auto &input = host.CurrentSnapshot();
            if (input.keyboard.WasPressedThisFrame(KeyCode::Escape))
                host.RequestStop();
            if (input.keyboard.WasPressedThisFrame(KeyCode::Tab))
                focus.Move(!input.keyboard.IsKeyDown(KeyCode::Shift));
            std::uint64_t activated = 0;
            if (input.mouse.WasPressedThisFrame(MouseButton::Left)) {
                const auto mouse = canvas.MousePoint();
                activated = focus.FocusAt(mouse.x, mouse.y);
            }

            if (input.keyboard.WasPressedThisFrame(KeyCode::Enter))
                activated = focus.Focused();
            if (activated == 1)
                mirrored = !mirrored;
            if (activated == 2)
                paused = !paused;
            const auto now = std::chrono::steady_clock::now();
            const auto frame = clocks.Advance(std::chrono::duration<double>(now - previous).count(),
                                              1, paused || !host.HasFocus());
            previous = now;
            auto pose = EvaluateCutoutPose(bones, tracks, frame.simulation_time, 2);
            if (mirrored)
                pose = MirrorCutoutPose(pose, 0);
            canvas.Begin();
            const auto extent = canvas.LogicalExtent();
            canvas.Rect(0, 0, extent.x, extent.y, 0x102030);
            for (const auto &button : buttons) {
                const auto &r = button.bounds;
                canvas.Rect(static_cast<float>(r.x), static_cast<float>(r.y),
                            static_cast<float>(r.width), static_cast<float>(r.height),
                            focus.Focused() == button.id ? 0x365f88 : 0x24354a);
                canvas.Text(LocalizeUi(labels, "en", button.id == 1 ? L"mirror" : L"pause"),
                            static_cast<float>(r.x + 15), static_cast<float>(r.y + 15), 28,
                            0xffffff);
            }

            DrawCutoutRigPreview(canvas, bones, pose, {extent.x * .5f, extent.y * .6f}, 2);
            // The same matrix is used for artwork and its attachment anchors.
            const auto anchor =
                TransformCutoutAnchor(CutoutSpriteMatrix(pose.back(), {0, 0}, mirrored), {20, 0});
            canvas.Circle(extent.x * .5f + anchor.x * 2, extent.y * .6f + anchor.y * 2, 6,
                          0xff6b8a);
            canvas.Text(L"Tab / Shift+Tab: focus   Enter / click: activate   Escape: exit", 90, 190,
                        24, 0xc6d9ef);
            canvas.End();
        });
}

} // namespace

int main(int argc, char **argv) {
    try {
        using namespace epidemic::gameplay;
        std::cout << "Epidemic " << epidemic::build::Version << " ABI "
                  << epidemic::build::AbiVersion << '\n';
        if (argc > 1 && std::string_view(argv[1]) == "--headless") {
            return 0;
        }

        if (argc > 2 && std::string_view(argv[1]) == "--profile") {
            session::SaveProfilePayload(argv[2], "minimal-client");
            if (session::LoadProfilePayload(argv[2]) != "minimal-client") {
                throw std::runtime_error("Profile round trip failed");
            }

            return 0;
        }

        if (argc > 1 && std::string_view(argv[1]) == "--rig-preview") {
            RunRigPreview();
            return 0;
        }

        presentation::Host host("Epidemic minimal game");
        presentation::Canvas canvas(host, {}, 0);
        if (argc > 1 && std::string_view(argv[1]) == "--tool") {
            host.ConfigureStartup(false);
        }

        host.Run([](const auto &) {},
                 [&] {
                     canvas.Begin();
                     const auto extent = canvas.LogicalExtent();
                     canvas.Rect(0, 0, extent.x, extent.y, 0x123344);
                     canvas.Text(L"Epidemic SDK client", 120, 200, 40, 0xffffff);
                     canvas.End();
                     host.RequestStop();
                 });
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

}
