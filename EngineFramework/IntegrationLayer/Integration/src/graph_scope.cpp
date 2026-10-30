#include "Epidemic/GameFramework/Integration/graph_scope.h"
#include <algorithm>

namespace epidemic::gameplay::composition {
GraphScope::GraphScope(entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part)
    : entities_(&entities), entity_(entity), handle_(entities.HandleOf(entity)), part_(part) {}
GraphScope::~GraphScope() { Invalidate(); }
foundation::Result<std::shared_ptr<GraphScope>> GraphScope::ForEntity(
    entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part) {
    auto scope = ForEntityState(entities, entity, part);
    if (!scope) return scope;
    if (!scope.Value()->IsAlive()) return foundation::Result<std::shared_ptr<GraphScope>>::Failure(
        foundation::Error::Create("composition.invalid_entity", "an active Entity/Part is required"));
    return scope;
}
foundation::Result<std::shared_ptr<GraphScope>> GraphScope::ForEntityState(
    entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part) {
    auto scope = std::shared_ptr<GraphScope>(new GraphScope(entities, entity, part));
    if (!scope->IsPresent()) return foundation::Result<std::shared_ptr<GraphScope>>::Failure(
        foundation::Error::Create("composition.invalid_entity", "an existing Entity/Part is required"));
    return foundation::Result<std::shared_ptr<GraphScope>>::Success(std::move(scope));
}
bool GraphScope::IsAlive() const { return Check(true); }
bool GraphScope::IsPresent() const { return Check(false); }
bool GraphScope::Check(bool require_active) const {
    if (!enabled_) return false;
    if (depth_ != 0) { const auto parent = parent_.lock(); if (!parent || !parent->Check(require_active)) return false; }
    if (kind_ != GraphScopeKind::Entity) return !owner_alive_ || owner_alive_();
    const auto record = entities_->Resolve(handle_);
    return record && record->id == entity_ && (record->lifecycle == entities::EntityLifecycleState::Alive ||
           (!require_active && record->lifecycle == entities::EntityLifecycleState::Dormant)) &&
           (!part_.IsValid() || entities_->GetPart(entity_, part_).has_value());
}
GameplayObjectRef GraphScope::Object() const noexcept {
    return kind_ == GraphScopeKind::Entity ? entities::EntityService::ToGameplayObjectRef(entity_) : GameplayObjectRef{};
}
bool GraphScope::SameTarget(const GraphScope &other) const noexcept {
    if (kind_ != other.kind_) return false;
    if (kind_ != GraphScopeKind::Entity) return system_identity_ == other.system_identity_;
    return entities_ == other.entities_ && entity_ == other.entity_ && handle_ == other.handle_ && part_ == other.part_;
}
foundation::Result<std::shared_ptr<GraphScope>> GraphScope::ForSystem(
    GraphScopeKind kind, GameplayObjectId owner, foundation::StringId instance, std::function<bool()> owner_alive) {
    using Result = foundation::Result<std::shared_ptr<GraphScope>>;
    if ((kind != GraphScopeKind::Scene && kind != GraphScopeKind::World) || !owner.IsValid() || !instance.IsValid())
        return Result::Failure(foundation::Error::Create("composition.invalid_system", "system needs Scene/World owner and instance identity"));
    auto scope = std::shared_ptr<GraphScope>(new GraphScope);
    scope->kind_ = kind; scope->scope_owner_ = owner; scope->system_id_ = instance;
    scope->system_identity_ = std::make_shared<const int>(0); scope->owner_alive_ = std::move(owner_alive);
    if (!scope->IsAlive()) return Result::Failure(foundation::Error::Create("composition.owner_expired", "system owner expired"));
    return Result::Success(std::move(scope));
}
foundation::Result<std::shared_ptr<GraphScope>> GraphScope::Child(const std::shared_ptr<GraphScope> &parent) {
    using Result = foundation::Result<std::shared_ptr<GraphScope>>;
    if (!parent || !parent->IsAlive() || parent->depth_ >= 32) return Result::Failure(
        foundation::Error::Create("composition.owner_expired", "invalid parent scope or scope depth"));
    auto child = std::shared_ptr<GraphScope>(new GraphScope);
    child->entities_ = parent->entities_; child->entity_ = parent->entity_; child->part_ = parent->part_;
    child->kind_ = parent->kind_; child->scope_owner_ = parent->scope_owner_; child->system_id_ = parent->system_id_;
    child->system_identity_ = parent->system_identity_;
    child->handle_ = parent->handle_; child->parent_ = parent; child->depth_ = parent->depth_ + 1;
    std::erase_if(parent->children_, [](const auto &weak) { return weak.expired(); });
    parent->children_.push_back(child);
    return Result::Success(std::move(child));
}
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
    try { if (!IsPresent()) Invalidate(); else if (!IsAlive()) CancelExecutions(); }
    catch (...) { Invalidate(); } // Fail closed; never throw during a native callback's unwind.
}
void GraphScope::CancelExecutions() noexcept {
    for (const auto &weak : children_) if (const auto child = weak.lock()) child->CancelExecutions();
    for (const auto &weak : activations_) if (const auto active = weak.lock()) active->RequestCancel();
    activations_.clear();
}
void GraphScope::Invalidate() noexcept { enabled_ = false; CancelExecutions(); }
} // namespace epidemic::gameplay::composition
