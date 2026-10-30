#include "Epidemic/Game/Editor/content_dependencies.h"
#include <algorithm>

namespace epidemic::game::editor {
std::vector<ContentReference> ContentDependencyIndex::Incoming(runtime::AssetId id) const {
    std::vector<ContentReference> out;
    for (const auto &v : references)
        if (v.source == id)
            out.push_back(v);
    return out;
}

std::vector<ContentReference> ContentDependencyIndex::Outgoing(runtime::AssetId id) const {
    std::vector<ContentReference> out;
    for (const auto &v : references)
        if (v.dependent == id)
            out.push_back(v);
    return out;
}

ContentDependencyIndex BuildContentDependencyIndex(const ProjectFolder &project) {
    ContentDependencyIndex result;
    const auto snapshot = project.Snapshot();
    if (!snapshot)
        return result;
    for (const auto &binding : project.SpriteBindings())
        result.references.push_back({binding.texture, binding.prefab, "prefab.sprite", true});
    for (const auto &document : snapshot->definition.documents) {
        const auto meta = snapshot->assets->FindById(document.id);
        if (!meta)
            continue;
        for (const auto &dep : meta->dependencies)
            result.references.push_back(
                {dep.asset_id, document.id, "document.dependency", dep.required});
    }

    std::sort(result.references.begin(), result.references.end(), [](const auto &a, const auto &b) {
        if (a.source.Raw() != b.source.Raw())
            return a.source.Raw() < b.source.Raw();
        if (a.dependent.Raw() != b.dependent.Raw())
            return a.dependent.Raw() < b.dependent.Raw();
        return a.origin < b.origin;
    });
    result.references.erase(std::unique(result.references.begin(), result.references.end(),
                                        [](const auto &a, const auto &b) {
                                            return a.source == b.source &&
                                                   a.dependent == b.dependent &&
                                                   a.origin == b.origin;
                                        }),
                            result.references.end());
    return result;
}

} // namespace epidemic::game::editor
