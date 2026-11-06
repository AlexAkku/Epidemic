#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Runtime/Resources/resource_services.h"
#include "../example/demo_game.h"
#include "renderer.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <future>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace ed = epidemic::game::editor;
namespace rt = epidemic::runtime;
namespace ex = ed::example;
namespace {
void Check(bool value, const char *reason) {
    if (!value)
        throw std::runtime_error(reason);
}
struct ComScope final {
    ComScope() {
        if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
            throw std::runtime_error("Preview COM initialization failed");
    }
    ~ComScope() { CoUninitialize(); }
};
auto PreparePreview(std::filesystem::path root, std::string key) {
    ed::ImageResourceLoader worker{std::move(root)};
    const auto id = ex::Take(worker.Register(std::move(key)));
    return worker.Load({id, worker.GetResourceType()});
}
}

int main() {
    try {
        // Preserve the retired Content Browser's GPU/lease regression without
        // retaining its dialog, controls, event loop or another user-facing UI.
        ComScope com;
        const auto root = std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY} / "p04-image-relocated";
        ex::Schema schema;
        auto project = ex::Take(ed::ProjectFolder::Open(root, schema.View()));
        const auto &sources = project->Sources();
        const auto source = std::find_if(sources.begin(), sources.end(), [](const auto &item) {
            return !item.IsTrashed() && !item.cooked_key.empty();
        });
        Check(source != sources.end(), "Missing committed PNG preview fixture");
        const auto key = source->cooked_key;
        auto window = std::unique_ptr<std::remove_pointer_t<HWND>, decltype(&DestroyWindow)>(
            CreateWindowExW(0, L"STATIC", L"PNG resource regression", WS_POPUP,
                            0, 0, 320, 320, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr),
            &DestroyWindow);
        Check(window != nullptr, "Hidden GPU test surface creation failed");
        rt::presentation::Renderer renderer{window.get(), {}, 0};
        renderer.SetClientExtent(320, 320);
        renderer.Resize();
        ed::ImageResourceLoader loader{root};
        auto resources = ex::Take(rt::CreateResourceServices());
        ex::Take(resources.loaders->RegisterLoader(loader));
        ex::Take(resources.loaders->Freeze());
        for (int repeat = 0; repeat < 2; ++repeat) {
            auto prepared = std::async(std::launch::async, PreparePreview, root, key);
            const auto id = ex::Take(loader.Register(key));
            ex::Take(loader.StagePrepared(ex::Take(prepared.get())));
            const auto lease = ex::Take(resources.manager->RequestLease({id, loader.GetResourceType()}));
            ex::Take(resources.manager->ProcessPendingLoads());
            const auto payload = std::dynamic_pointer_cast<const ed::ImagePayload>(
                resources.manager->GetPayload(lease.resource));
            Check(payload != nullptr, "Prepared image publication failed");
            const auto thumbnail = ed::MakeImageThumbnail(payload->image);
            auto texture = renderer.CreateTexture(thumbnail.width, thumbnail.height, thumbnail.pixels);
            Check(texture != nullptr, "PNG GPU upload failed");
            renderer.Begin();
            renderer.Texture(texture, 8.0f, 8.0f, 128.0f, 128.0f);
            renderer.End();
            renderer.SetClientExtent(384, 384);
            renderer.Begin();
            renderer.Texture(texture, 8.0f, 8.0f, 128.0f, 128.0f);
            renderer.End();
            texture.reset();
            ex::Take(resources.manager->Release(lease));
            loader.CollectUnused(*resources.manager);
            const auto memory = resources.manager->GetMemoryStatistics();
            Check(memory.resident_bytes == 0 && memory.invariant_failure_count == 0,
                  "Preview release leaked resident image or broke resource invariants");
            Check(loader.RegistrationCount() == 0, "Retired preview key remained registered");
        }
    } catch (const std::exception &error) {
        std::fprintf(stderr, "PNG preview regression: %s\n", error.what());
        return 1;
    }
}
