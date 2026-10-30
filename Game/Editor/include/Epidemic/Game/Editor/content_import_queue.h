#pragma once
#include "Epidemic/Game/Editor/content_hot_reload.h"
#include <deque>

namespace epidemic::game::editor {
// Owns only IO jobs. ProjectFolder remains the sole publisher/asset authority.
class ContentImportQueue final {
  public:
    using Prepare =
        std::function<foundation::Result<PreparedCookedPng>(const std::filesystem::path &,
                                                            const std::filesystem::path &,
                                                            const std::atomic_bool *)>;

    explicit ContentImportQueue(Prepare prepare = PrepareImportedPng)
        : prepare_(std::move(prepare)) {
    }

    ~ContentImportQueue();
    ContentImportQueue(const ContentImportQueue &) = delete;
    ContentImportQueue &operator=(const ContentImportQueue &) = delete;
    [[nodiscard]] foundation::Result<std::uint64_t> Enqueue(const ProjectFolder &folder,
                                                            const std::filesystem::path &source,
                                                            std::string destination,
                                                            runtime::AssetId duplicate_of = {});
    void Reset(const std::filesystem::path &root);
    void Cancel() noexcept;

    [[nodiscard]] bool Busy() const noexcept {
        return pending_ || !queued_.empty();
    }

    [[nodiscard]] std::vector<HotAssetEvent> Poll(ProjectFolder &folder);

  private:
    struct Request {
        std::uint64_t job = 0, epoch = 0;
        std::filesystem::path root, source;
        std::string destination;
        runtime::AssetId duplicate_of{};

        std::uint64_t asset_revision = 0;
    };

    struct Pending {
        Request request;
        std::shared_ptr<std::atomic_bool> cancel;
        std::future<foundation::Result<PreparedCookedPng>> worker;
    };

    Prepare prepare_;
    std::filesystem::path root_;
    std::deque<Request> queued_;
    std::unique_ptr<Pending> pending_;
    std::uint64_t next_job_ = 1, epoch_ = 0;
};

} // namespace epidemic::game::editor
