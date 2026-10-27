#pragma once
#include "Epidemic/GameFramework/Integration/prefab.h"
#include "Epidemic/Runtime/Scene/scene_services.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include <unordered_map>

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
    runtime::SceneServices scene;
    std::unordered_map<runtime::SceneNodeId, runtime::SceneNodeId> nodes;
};
[[nodiscard]] foundation::Result<SceneCompositionInstance> CreateSceneComposition(const SceneCompositionDefinition &definition);
} // namespace epidemic::game::composition
