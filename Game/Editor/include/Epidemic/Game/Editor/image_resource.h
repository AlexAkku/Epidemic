#pragma once
#include "Epidemic/Game/Editor/png_import.h"
#include "Epidemic/Runtime/Resources/resource_loader.h"
#include <atomic>
#include <utility>

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
    [[nodiscard]] runtime::ResourceType GetResourceType() const override;
    [[nodiscard]] foundation::Result<runtime::ResourceLoadArtifact> Load(runtime::ResourceRequest request) override;
  private:
    std::filesystem::path root_;
    std::vector<std::pair<runtime::ResourceId,std::string>> keys_;
};
// Same PBGRA source pixels, nearest reduction; bounded to a 128x128 preview.
[[nodiscard]] DecodedPng MakeImageThumbnail(const DecodedPng &image);
} // namespace epidemic::game::editor
