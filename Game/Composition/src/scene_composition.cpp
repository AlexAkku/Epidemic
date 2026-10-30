#include "Epidemic/Game/Composition/scene_composition.h"

namespace epidemic::game::composition {
foundation::Result<SceneCompositionInstance> CreateSceneComposition(const SceneCompositionDefinition &definition) {
    using Result = foundation::Result<SceneCompositionInstance>;
    if (!definition.asset_id.IsValid() || definition.scene.nodes.size() > 4096)
        return Result::Failure(foundation::Error::Create("composition.invalid_scene", "invalid Scene asset or node count"));
    auto services = runtime::CreateSceneServices();
    if (!services) return Result::Failure(services.GetError());
    SceneCompositionInstance instance;
    instance.scene = std::move(services).Value();
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
namespace {
std::optional<runtime::SceneNodeId> EntityNode(const SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity){
    if(!scene.scene.nodes || !scene.scene.transforms)return {};
    const auto owner=entities.Resolve(entity);
    if(!owner || (owner->lifecycle!=gameplay::entities::EntityLifecycleState::Alive && owner->lifecycle!=gameplay::entities::EntityLifecycleState::Dormant))return {};
    const auto found=scene.entities.find(owner->id);
    if(found==scene.entities.end() || found->second.entity!=entity || !scene.scene.nodes->Exists(found->second.node))return {};
    return found->second.node;
}
foundation::Error SpatialError(){return foundation::Error::Create("composition.entity_scene_missing","Entity has no active Scene binding");}
}
foundation::Result<void> BindSceneEntity(SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity,runtime::SceneNodeId node){
    const auto owner=entities.Resolve(entity);
    if(!owner || !scene.scene.nodes || !scene.scene.transforms || (owner->lifecycle!=gameplay::entities::EntityLifecycleState::Alive && owner->lifecycle!=gameplay::entities::EntityLifecycleState::Dormant) || !scene.scene.nodes->Exists(node))
        return foundation::Result<void>::Failure(SpatialError());
    const auto found=scene.entities.find(owner->id);
    if(found!=scene.entities.end() && (found->second.entity!=entity || found->second.node!=node))
        return foundation::Result<void>::Failure(foundation::Error::Create("composition.entity_already_bound","Entity is bound to another Scene node"));
    for(const auto &[id,link]:scene.entities)if(id!=owner->id && link.node==node)
        return foundation::Result<void>::Failure(foundation::Error::Create("composition.scene_already_owned","Scene node already represents another Entity"));
    scene.entities.insert_or_assign(owner->id,SceneCompositionInstance::EntityLink{entity,node});
    return foundation::Result<void>::Success();
}
foundation::Result<runtime::Transform> GetEntityWorldTransform(const SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity){
    const auto node=EntityNode(scene,entities,entity);
    if(node)if(const auto transform=scene.scene.transforms->GetWorldTransform(*node))return foundation::Result<runtime::Transform>::Success(*transform);
    return foundation::Result<runtime::Transform>::Failure(SpatialError());
}
foundation::Result<void> SetEntityWorldTransform(SceneCompositionInstance &scene,
    const gameplay::entities::EntityService &entities,gameplay::entities::EntityHandle entity,const runtime::Transform &transform){
    const auto node=EntityNode(scene,entities,entity);
    return node?scene.scene.transforms->SetWorldTransform(*node,transform):foundation::Result<void>::Failure(SpatialError());
}
foundation::Result<void> SynchronizeSceneEntities(SceneCompositionInstance &scene,const gameplay::entities::EntityService &entities){
    if(!scene.scene.nodes)return foundation::Result<void>::Failure(SpatialError());
    for(auto found=scene.entities.begin();found!=scene.entities.end();){
        if(EntityNode(scene,entities,found->second.entity)){++found;continue;}
        if(scene.scene.nodes->Exists(found->second.node)){
            const auto hidden=scene.scene.nodes->SetVisibility(found->second.node,runtime::SceneVisibilityState::Hidden);
            if(!hidden)return hidden;
        }
        found=scene.entities.erase(found);
    }
    return foundation::Result<void>::Success();
}
} // namespace epidemic::game::composition
