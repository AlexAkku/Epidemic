#pragma once
#include "Epidemic/Game/Editor/project_folder.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace epidemic::game::editor {
struct DecodedPng {
  std::uint32_t width = 0, height = 0;
  // Premultiplied BGRA8, top-down. The caller owns the pixels.
  std::vector<std::byte> pixels;
};
// Windows WIC decoder. Decode is bounded and never modifies the project.
[[nodiscard]] foundation::Result<DecodedPng>
DecodePng(const std::filesystem::path &path);
// Import a previously placed PNG under Content/ with typed texture identity.
// The caller may use ProjectFolder::Save to publish its source manifest.
[[nodiscard]] foundation::Result<runtime::AssetId>
ImportProjectPng(ProjectFolder &project, std::string_view relative);
// External import copies a validated PNG into an existing Content directory,
// then indexes it. No editor executable rebuild or .rc registration required.
[[nodiscard]] foundation::Result<runtime::AssetId>
ImportExternalPng(ProjectFolder &project, const std::filesystem::path &external,
                  std::string_view content_relative);
} // namespace epidemic::game::editor
