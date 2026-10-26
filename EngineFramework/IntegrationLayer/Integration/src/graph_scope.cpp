#include "Epidemic/GameFramework/Integration/graph_scope.h"
#include <algorithm>

namespace epidemic::gameplay::composition {
GraphScope::GraphScope(entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part)
    : entities_(entities), entity_(entity), handle_(entities.HandleOf(entity)), part_(part) {}
GraphScope::~GraphScope() { Invalidate(); }
foundation::Result<std::shared_ptr<GraphScope>> GraphScope::ForEntity(
    entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part) {
    auto scope = std::shared_ptr<GraphScope>(new GraphScope(entities, entity, part));
    if (!scope->IsAlive()) return foundation::Result<std::shared_ptr<GraphScope>>::Failure(
        foundation::Error::Create("composition.invalid_entity", "an alive Entity/Part is required"));
    return foundation::Result<std::shared_ptr<GraphScope>>::Success(std::move(scope));
}
bool GraphScope::IsAlive() const {
    if (!enabled_) return false;
    const auto record = entities_.Resolve(handle_);
    return record && record->id == entity_ && record->lifecycle == entities::EntityLifecycleState::Alive &&
           (!part_.IsValid() || entities_.GetPart(entity_, part_).has_value());
}
GameplayObjectRef GraphScope::Object() const noexcept { return entities::EntityService::ToGameplayObjectRef(entity_); }
foundation::Result<void> GraphScope::Track(const std::shared_ptr<IGraphActivation> &activation) {
    if (!activation || !IsAlive()) {
        if (activation) activation->RequestCancel();
        return foundation::Result<void>::Failure(foundation::Error::Create("composition.owner_expired", "graph owner expired"));
    }
    std::erase_if(activations_, [](const auto &weak) { const auto active = weak.lock(); return !active || active->IsFinished(); });
    activations_.push_back(activation);
    return foundation::Result<void>::Success();
}
void GraphScope::Synchronize() noexcept {
    try { if (!IsAlive()) Invalidate(); }
    catch (...) { Invalidate(); } // Fail closed; never throw during a native callback's unwind.
}
void GraphScope::Invalidate() noexcept {
    enabled_ = false;
    for (const auto &weak : activations_) if (const auto active = weak.lock()) active->RequestCancel();
    activations_.clear();
}
} // namespace epidemic::gameplay::composition
