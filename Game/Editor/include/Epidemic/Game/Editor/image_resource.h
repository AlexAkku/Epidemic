#pragma once
#include "Epidemic/Game/Editor/png_import.h"
#include "Epidemic/Runtime/Resources/resource_loader.h"
#include "Epidemic/Runtime/Resources/resource_manager.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <unordered_map>

namespace epidemic::game::editor {
// Recipe v1 is intentionally bounded: sRGB, PBGRA8, top-down, clamp, nearest,
// no mipmaps/trim; pivot and world scale belong to Sprite authoring (P04.3b).
struct ImagePayload final : runtime::IResourcePayload {
    DecodedPng image;
    [[nodiscard]] std::size_t GetSizeBytes() const noexcept override { return image.pixels.size(); }
};
[[nodiscard]] foundation::Result<runtime::AssetId> CookProjectPng(ProjectFolder &project,
    std::string_view relative, const std::atomic_bool *cancel = nullptr);
// Immutable serialized image artifacts. Does not decode source files or publish
// authoring data. Owner thread registers keys before requesting their leases.
class ImageResourceLoader final : public runtime::IResourceLoader {
  public:
    explicit ImageResourceLoader(std::filesystem::path root) : root_(std::move(root)) {}
    [[nodiscard]] foundation::Result<runtime::ResourceId> Register(std::string key);
    [[nodiscard]] foundation::Result<void> StagePrepared(runtime::ResourceLoadArtifact artifact);
    // Call after releasing preview leases. Referenced/queued resources survive;
    // retired registrations can be re-created by Register before a new request.
    void CollectUnused(runtime::IResourceManager &manager);
    [[nodiscard]] std::size_t RegistrationCount() const noexcept { return keys_.size(); }
    [[nodiscard]] runtime::ResourceType GetResourceType() const override;
    [[nodiscard]] foundation::Result<runtime::ResourceLoadArtifact> Load(runtime::ResourceRequest request) override;
  private:
    std::filesystem::path root_;
    std::unordered_map<std::uint64_t,std::string> keys_;
    std::optional<runtime::ResourceLoadArtifact> prepared_;
};
// Produces an unreferenced, immutable cooked PNG on a worker thread.
// No ProjectFolder, AssetCatalog, Resources or Renderer mutation takes place.
// The read lease prevents replacement/writes until owner publication or discard.
// Only the preparation function can construct this metadata publication proof.
class PreparedCookedPng final {
  public:
    [[nodiscard]] const std::string &Key() const noexcept { return key_; }
    [[nodiscard]] bool Rebuilt() const noexcept { return rebuilt_; }
  private:
    friend class ProjectFolder;
    friend foundation::Result<PreparedCookedPng> PrepareCookedPng(
        const std::filesystem::path &,std::string_view,const std::atomic_bool *,std::string_view);
    friend foundation::Result<PreparedCookedPng> PrepareImportedPng(
        const std::filesystem::path &,const std::filesystem::path &,const std::atomic_bool *);
    std::string key_,relative_;
    std::filesystem::path root_;
    std::uint64_t source_hash_=0;
    std::shared_ptr<const std::filesystem::path> staging_cleanup_;
    std::shared_ptr<void> source_read_; // Released before staging_cleanup_.
    bool rebuilt_=false;
};
[[nodiscard]] foundation::Result<PreparedCookedPng> PrepareCookedPng(
    const std::filesystem::path &root, std::string_view relative,
    const std::atomic_bool *cancel = nullptr, std::string_view previous_key = {});
[[nodiscard]] foundation::Result<PreparedCookedPng> PrepareImportedPng(
    const std::filesystem::path &root,const std::filesystem::path &external,
    const std::atomic_bool *cancel=nullptr);
// Same PBGRA source pixels, nearest reduction; bounded to a 128x128 preview.
[[nodiscard]] DecodedPng MakeImageThumbnail(const DecodedPng &image);
} // namespace epidemic::game::editor
