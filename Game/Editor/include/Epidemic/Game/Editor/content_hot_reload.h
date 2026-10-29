#pragma once
#include "Epidemic/Game/Editor/image_resource.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace epidemic::game::editor {
enum class HotAssetEventKind { Queued, Updated, Missing, Failed, Recovered, Cancelled };
struct HotAssetEvent {
    runtime::AssetId id{};
    HotAssetEventKind kind=HotAssetEventKind::Updated;
    std::string diagnostic;
};
// P04.4/P04.5: one outstanding decode/cook at a time. The worker may read
// source files and publish an *unreferenced*, content-addressed cooked blob;
// it must never touch ProjectFolder, Runtime Resources or Renderer.
// Only Poll on the owner thread may update the source manifest. Futures are
// drained when the editor changes folders or shuts down.
class ContentHotReload final {
  public:
    ContentHotReload()=default;
    ContentHotReload(const ContentHotReload&)=delete;
    ContentHotReload& operator=(const ContentHotReload&)=delete;
    ~ContentHotReload();
    void Reset(const ProjectFolder &folder);
    void SetEnabled(bool enabled) noexcept;
    [[nodiscard]] bool Enabled() const noexcept { return enabled_; }
    [[nodiscard]] bool Busy() const noexcept { return pending_!=nullptr; }
    [[nodiscard]] std::vector<HotAssetEvent> Poll(ProjectFolder &folder);
  private:
    struct Observation {
        std::string path;
        std::uintmax_t size=0;
        std::filesystem::file_time_type write{};
        std::uint32_t stable=0;
        bool seen=false, missing=false, failed=false, missing_reported=false;
    };
    struct Pending {
        runtime::AssetId id{};
        std::string relative;
        std::uint64_t initial_generation=0;
        std::string initial_cooked_key;
        std::uintmax_t size=0;
        std::filesystem::file_time_type write{};
        std::shared_ptr<std::atomic_bool> cancel;
        std::future<foundation::Result<PreparedCookedPng>> worker;
    };
    void CancelPending() noexcept;
    std::unordered_map<std::uint64_t,Observation> tracked_;
    std::unique_ptr<Pending> pending_;
    std::filesystem::path root_;
    std::size_t cursor_=0;
    bool enabled_=true;
};
} // namespace epidemic::game::editor
