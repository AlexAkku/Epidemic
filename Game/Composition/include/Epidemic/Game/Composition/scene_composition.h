#pragma once
#include "Epidemic/GameFramework/Integration/prefab.h"
#include "Epidemic/Runtime/Scene/scene_services.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include <unordered_map>
#include <map>

namespace epidemic::game::composition {
struct ScenePrefabPlacement {
    runtime::SceneNodeId node{};
    runtime::AssetId prefab{};
    gameplay::composition::PrefabOverrides overrides;
};
// Authoring composition of existing Scene snapshots and Prefab definitions.
// Spatial behavior is implemented only by Runtime Scene, Entity behavior only
// by the Framework EntityService/PrefabComposer.
struct SceneCompositionDefinition {
    runtime::AssetId asset_id{};
    runtime::SceneSnapshot scene;
    std::vector<ScenePrefabPlacement> placements;
};
struct SceneCompositionInstance {
    struct EntityLink { gameplay::entities::EntityHandle entity{};runtime::SceneNodeId node{}; };
    SceneCompositionInstance() = default;
    // MSVC's unordered_map move can allocate. Keep ordinary allocation failure
    // propagation instead of promising noexcept and terminating the process.
    SceneCompositionInstance(SceneCompositionInstance &&) noexcept(false) = default; // NOLINT(bugprone-unsafe-to-allow-exceptions)
    SceneCompositionInstance &operator=(SceneCompositionInstance &&) noexcept(false) = default; // NOLINT(bugprone-unsafe-to-allow-exceptions)
    runtime::SceneServices scene;
    std::unordered_map<runtime::SceneNodeId, runtime::SceneNodeId> nodes;
    // Identity links only. Transform/visibility remain in the existing Scene.
    std::map<gameplay::entities::EntityId,EntityLink> entities;
};
[[nodiscard]] foundation::Result<SceneCompositionInstance> CreateSceneComposition(const SceneCompositionDefinition &definition);
[[nodiscard]] foundation::Result<void> BindSceneEntity(SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity,runtime::SceneNodeId node);
[[nodiscard]] foundation::Result<runtime::Transform> GetEntityWorldTransform(const SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity);
[[nodiscard]] foundation::Result<void> SetEntityWorldTransform(SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity,const runtime::Transform &transform);
[[nodiscard]] foundation::Result<void> SynchronizeSceneEntities(SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities);
} // namespace epidemic::game::composition
