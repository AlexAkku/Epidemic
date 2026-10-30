#pragma once
#include "Epidemic/Game/Editor/image_resource.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <functional>
#include <chrono>
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
// cancelled without waiting on folder changes. Only final destruction drains.
class ContentHotReload final {
  public:
    using Prepare=std::function<foundation::Result<PreparedCookedPng>(const std::filesystem::path &,
        std::string_view,const std::atomic_bool *,std::string_view)>;
    explicit ContentHotReload(Prepare prepare=PrepareCookedPng,
        std::chrono::milliseconds audit_interval=std::chrono::seconds{5});
    ContentHotReload(const ContentHotReload&)=delete;
    ContentHotReload& operator=(const ContentHotReload&)=delete;
    ~ContentHotReload();
    void Reset(const ProjectFolder &folder);
    void SetEnabled(bool enabled) noexcept;
    void Request(runtime::AssetId asset);
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
        bool dirty=true,force=false;
        std::chrono::steady_clock::time_point next_audit{};
    };
    struct Pending {
        runtime::AssetId id{};
        std::string relative;
        std::uint64_t initial_generation=0;
        std::string initial_cooked_key;
        std::uint64_t initial_source_hash=0,epoch=0;
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
    std::uint64_t epoch_=0;
    Prepare prepare_;
    std::chrono::milliseconds audit_interval_;
    bool enabled_=true;
};
} // namespace epidemic::game::editor
