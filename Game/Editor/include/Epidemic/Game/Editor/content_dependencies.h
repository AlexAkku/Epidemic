#pragma once
#include "Epidemic/Game/Editor/project_folder.h"
#include <string>
#include <vector>

namespace epidemic::game::editor {
// Detached, reproducible reverse index. Never stores pointers to live Scenes,
// Prefab instances or a mutable Assets catalog. Rebuild after Snapshot/asset
// generation changes. The UI can show exactly which document uses an asset.
struct ContentReference {
    runtime::AssetId source{};

    runtime::AssetId dependent{};

    std::string origin;
    bool required = true;
};

struct ContentDependencyIndex {
    std::vector<ContentReference> references;
    [[nodiscard]] std::vector<ContentReference> Incoming(runtime::AssetId id) const;
    [[nodiscard]] std::vector<ContentReference> Outgoing(runtime::AssetId id) const;
};

[[nodiscard]] ContentDependencyIndex BuildContentDependencyIndex(const ProjectFolder &project);
} // namespace epidemic::game::editor
