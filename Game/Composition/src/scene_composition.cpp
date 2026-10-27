#include "Epidemic/Game/Composition/scene_composition.h"

namespace epidemic::game::composition {
foundation::Result<SceneCompositionInstance> CreateSceneComposition(const SceneCompositionDefinition &definition) {
    using Result = foundation::Result<SceneCompositionInstance>;
    if (!definition.asset_id.IsValid() || definition.scene.nodes.size() > 4096)
        return Result::Failure(foundation::Error::Create("composition.invalid_scene", "invalid Scene asset or node count"));
    auto services = runtime::CreateSceneServices();
    if (!services) return Result::Failure(services.GetError());
    SceneCompositionInstance instance{std::move(services).Value(), {}};
    for (const auto &node : definition.scene.nodes) {
        if (!node.id.IsValid() || instance.nodes.contains(node.id)) return Result::Failure(
            foundation::Error::Create("composition.scene_identity", "Scene node identity is invalid or duplicated"));
        const auto created = instance.scene.nodes->CreateNode();
        if (!created) return Result::Failure(created.GetError());
        instance.nodes.emplace(node.id, created.Value());
        const auto transformed = instance.scene.transforms->SetLocalTransform(created.Value(), node.local_transform);
        if (!transformed) return Result::Failure(transformed.GetError());
        const auto mobility = instance.scene.nodes->SetMobility(created.Value(), node.mobility);
        if (!mobility) return Result::Failure(mobility.GetError());
        const auto visibility = instance.scene.nodes->SetVisibility(created.Value(), node.visibility);
        if (!visibility) return Result::Failure(visibility.GetError());
        if (node.local_bounds) {
            const auto bounds = instance.scene.spatial->SetLocalBounds(created.Value(), *node.local_bounds);
            if (!bounds) return Result::Failure(bounds.GetError());
        }
    }
    for (const auto &node : definition.scene.nodes) if (node.parent.IsValid()) {
        const auto parent = instance.nodes.find(node.parent);
        if (parent == instance.nodes.end()) return Result::Failure(
            foundation::Error::Create("composition.scene_parent", "Scene parent is missing"));
        const auto attached = instance.scene.nodes->AttachNode(instance.nodes.at(node.id), parent->second, runtime::ReparentMode::KeepLocal);
        if (!attached) return Result::Failure(attached.GetError());
    }
    return Result::Success(std::move(instance));
}
} // namespace epidemic::game::composition
