#pragma once
#include "Epidemic/Game/Editor/project_folder.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace epidemic::game::editor {
// Detached Content Browser view: authoritative state remains ProjectFolder.
struct BrowserFilter {
    std::string path_prefix;
    std::string search;
    std::optional<runtime::AssetType> type;
    bool show_trashed=false;
};
struct BrowserItem {
    runtime::AssetId id{};
    runtime::AssetType type{};
    std::string path;
    std::uint64_t generation=0;
    std::uint64_t source_hash=0;
    bool present=false;
    bool trashed=false;
};
struct BrowserListing {
    std::vector<BrowserItem> items;
    std::vector<std::string> folders;
};
// Case-insensitive ASCII substring search. The returned copy is stable across
// ProjectFolder edits and sorted by path then AssetId.
[[nodiscard]] BrowserListing BrowseContent(const ProjectFolder &project,const BrowserFilter &filter={});
// These operations use the ProjectFolder as the sole authority; they do not
// introduce another catalog or reinterpret the current authoring archive.
[[nodiscard]] foundation::Result<void> RenameContentAsset(ProjectFolder &project,runtime::AssetId id,std::string_view path);
} // namespace epidemic::game::editor
