#include "Epidemic/GameFramework/Integration/prefab.h"
#include <algorithm>
#include <set>
#include <tuple>
#include <unordered_set>

namespace epidemic::gameplay::composition {
namespace {
foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "prefab composition rejected"); }
}
const PrefabInstance::Object *PrefabInstance::Find(PrefabNodeId node) const noexcept {
    const auto found = std::find_if(objects_.begin(), objects_.end(), [&](const auto &object) { return object.id == node; });
    return found == objects_.end() ? nullptr : &*found;
}
foundation::Result<std::vector<PrefabComposer::PlannedNode>> PrefabComposer::Plan(
    const PrefabDefinition &definition, const PrefabOverrides &overrides, bool prepare) const {
    using Result = foundation::Result<std::vector<PlannedNode>>;
    if (!definition.id.IsValid() || !registry_.IsFrozen() || !entities_.IsFrozen() || definition.parameters.size() > 256 ||
        overrides.parameters.size() > 256 || overrides.components.size() > 4096 || overrides.behaviors.size() > 4096)
        return Result::Failure(Error("prefab.invalid_definition"));
    std::unordered_map<std::uint64_t, PropertyValue> parameters;
    for (const auto &parameter : definition.parameters) if (!parameter.id.IsValid() || !parameters.emplace(parameter.id.Raw(), parameter.value).second)
        return Result::Failure(Error("prefab.invalid_parameter"));
    std::unordered_set<std::uint64_t> assigned;
    for (const auto &override : overrides.parameters) {
        const auto found = parameters.find(override.property.Raw());
        if (found == parameters.end() || !assigned.insert(override.property.Raw()).second) return Result::Failure(Error("prefab.invalid_parameter"));
        if (found->second.index() != override.value.index()) return Result::Failure(Error("prefab.parameter_type"));
        found->second = override.value;
    }
    std::set<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>> component_keys;
    for (const auto &override : overrides.components) if (!component_keys.emplace(override.node.Raw(), override.component.Raw(), override.part.Raw(), override.property.property.Raw()).second)
        return Result::Failure(Error("prefab.duplicate_override"));
    std::set<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t>> behavior_keys;
    for (const auto &override : overrides.behaviors) if (!behavior_keys.emplace(override.node.Raw(), override.component.Raw(), override.part.Raw(), override.behavior.Raw(), override.input.port.Raw()).second)
        return Result::Failure(Error("prefab.duplicate_override"));
    std::size_t matched_components = 0, matched_behaviors = 0;
    const auto bind_behaviors = [&](const std::vector<GraphBehaviorDefinition> &templates, PrefabNodeId node,
        TypeId component, entities::EntityPartId part) -> foundation::Result<std::vector<GraphBehaviorDefinition>> {
        using BindResult = foundation::Result<std::vector<GraphBehaviorDefinition>>;
        if (templates.size() > 64) return BindResult::Failure(Error("prefab.capacity"));
        auto bound = templates;
        std::unordered_set<std::uint64_t> ids;
        for (auto &behavior : bound) {
            if (!ids.insert(behavior.id.Raw()).second) return BindResult::Failure(Error("prefab.duplicate_behavior"));
            for (const auto &override : overrides.behaviors) if (override.node == node && override.component == component && override.part == part && override.behavior == behavior.id) {
                const auto found = std::find_if(behavior.inputs.begin(), behavior.inputs.end(), [&](const auto &input) { return input.port == override.input.port; });
                if (found == behavior.inputs.end()) behavior.inputs.push_back(override.input); else found->value = override.input.value;
                ++matched_behaviors;
            }
            const auto valid = behaviors_.Validate(behavior);
            if (!valid) return BindResult::Failure(valid.GetError());
        }
        return BindResult::Success(std::move(bound));
    };
    struct Pending { const PrefabNode *node = nullptr; PrefabNodeId parent{}; std::size_t depth = 0; };
    std::vector<Pending> pending{{&definition.root, {}, 0}};
    std::vector<PlannedNode> plan;
    std::unordered_set<std::uint64_t> nodes;
    while (!pending.empty()) {
        const auto next = pending.back(); pending.pop_back(); const auto &node = *next.node;
        if (next.depth >= 32 || plan.size() >= 1024 || node.children.size() > 1024 || node.components.size() > 64)
            return Result::Failure(Error("prefab.capacity"));
        if (!node.id.IsValid() || !nodes.insert(node.id.Raw()).second || !entities_.FindArchetype(node.archetype))
            return Result::Failure(Error("prefab.invalid_node"));
        PlannedNode planned; planned.definition = next.node; planned.parent = next.parent;
        auto behaviors = bind_behaviors(node.behaviors, node.id, {}, {});
        if (!behaviors) return Result::Failure(behaviors.GetError());
        planned.behaviors = std::move(behaviors).Value();
        std::set<std::pair<std::uint64_t, std::uint64_t>> types;
        for (const auto &component : node.components) {
            if (!types.emplace(component.type.Raw(), component.part.Raw()).second ||
                (component.part.IsValid() && !entities_.FindPartDefinition(node.archetype, component.part))) return Result::Failure(Error("prefab.invalid_component"));
            if (component.properties.size() > 256) return Result::Failure(Error("prefab.capacity"));
            std::vector<PropertyAssignment> properties;
            for (const auto &property : component.properties) {
                auto value = property.value;
                if (property.parameter.IsValid()) {
                    const auto found = parameters.find(property.parameter.Raw());
                    if (found == parameters.end()) return Result::Failure(Error("prefab.invalid_parameter"));
                    value = found->second;
                }
                properties.push_back({property.property, std::move(value)});
            }
            for (const auto &override : overrides.components) if (override.node == node.id && override.component == component.type && override.part == component.part) {
                const auto found = std::find_if(properties.begin(), properties.end(), [&](const auto &property) { return property.property == override.property.property; });
                if (found == properties.end()) properties.push_back(override.property); else found->value = override.property.value;
                ++matched_components;
            }
            const auto valid = registry_.Validate(component.type, properties);
            if (!valid) return Result::Failure(valid.GetError());
            auto attached = bind_behaviors(component.behaviors, node.id, component.type, component.part);
            if (!attached) return Result::Failure(attached.GetError());
            PlannedComponent prepared; prepared.definition = &component; prepared.behaviors = std::move(attached).Value();
            if (prepare) {
                auto value = registry_.Prepare(component.type, properties);
                if (!value) return Result::Failure(value.GetError());
                prepared.prepared.emplace(std::move(value).Value());
            }
            planned.components.push_back(std::move(prepared));
        }
        plan.push_back(std::move(planned));
        // These pointers refer to the immutable definition tree, not the local
        // traversal frame or an iterator stored in that frame.
        for (std::size_t child = next.node->children.size(); child > 0; --child)
            pending.push_back({&next.node->children[child - 1], node.id, next.depth + 1});
    }
    if (matched_components != overrides.components.size() || matched_behaviors != overrides.behaviors.size())
        return Result::Failure(Error("prefab.unknown_override"));
    return Result::Success(std::move(plan));
}
foundation::Result<PrefabId> PrefabComposer::Register(const PrefabDefinition &definition) {
    using Result = foundation::Result<PrefabId>;
    if (definitions_.size() >= 1024 || definitions_.contains(definition.id.Raw())) return Result::Failure(Error("prefab.duplicate_or_capacity"));
    const auto valid = Plan(definition, {}, false);
    if (!valid) return Result::Failure(valid.GetError());
    definitions_.emplace(definition.id.Raw(), std::make_shared<const PrefabDefinition>(definition));
    return Result::Success(definition.id);
}
std::shared_ptr<const PrefabDefinition> PrefabComposer::Find(PrefabId id) const {
    const auto found = definitions_.find(id.Raw()); return found == definitions_.end() ? nullptr : found->second;
}
foundation::Result<void> PrefabComposer::Rollback(PrefabInstance &instance) {
    std::optional<foundation::Error> failure;
    for (auto object = instance.objects_.rbegin(); object != instance.objects_.rend(); ++object) {
        if (object->scope) object->scope->Invalidate();
        for (const auto &behavior : object->behaviors) { const auto removed = behaviors_.Detach(behavior.handle); if (!removed && !failure) failure = removed.GetError(); }
        for (const auto &component : object->components) { const auto removed = components_.Remove(component); if (!removed && !failure) failure = removed.GetError(); }
        const auto record = entities_.Resolve(object->handle);
        if (record && (record->lifecycle == entities::EntityLifecycleState::Alive || record->lifecycle == entities::EntityLifecycleState::Dormant)) {
            const auto destroyed = entities_.RequestDestroy(object->entity, entities::EntityDestroyReason::OwnerRequest);
            if (!destroyed && !failure) failure = destroyed.GetError();
        }
    }
    if (failure) return foundation::Result<void>::Failure(*failure);
    return foundation::Result<void>::Success();
}
foundation::Result<std::shared_ptr<const PrefabInstance>> PrefabComposer::Spawn(PrefabId id, const PrefabOverrides &overrides) {
    using Result = foundation::Result<std::shared_ptr<const PrefabInstance>>;
    const auto definition = Find(id);
    if (!definition) return Result::Failure(Error("prefab.unknown_definition"));
    auto plan = Plan(*definition, overrides, true);
    if (!plan) return Result::Failure(plan.GetError());
    auto instance = std::make_shared<PrefabInstance>(); instance->definition_ = definition; instance->overrides_ = overrides;
    instance->objects_.reserve(plan.Value().size());
    const auto fail = [&](foundation::Error error) -> Result {
        const auto rollback = Rollback(*instance);
        if (!rollback) error.context += ";rollback=" + rollback.GetError().code;
        return Result::Failure(std::move(error));
    };
    try {
        for (auto &node : plan.Value()) {
            PrefabInstance::Object object; object.id = node.definition->id; object.parent = node.parent;
            object.components.reserve(node.components.size());
            std::size_t behavior_count = node.behaviors.size(); for (const auto &component : node.components) behavior_count += component.behaviors.size();
            object.behaviors.reserve(behavior_count);
            entities::CreateEntityRequest request; request.archetype = node.definition->archetype;
            const auto created = entities_.Create(request);
            if (!created) return fail(created.GetError());
            object.entity = created.Value().id; object.handle = created.Value().handle;
            instance->objects_.push_back(std::move(object));
            auto &published = instance->objects_.back();
            const auto scope = GraphScope::ForEntity(entities_, published.entity);
            if (!scope) return fail(scope.GetError());
            published.scope = scope.Value();
            for (auto &component : node.components) {
                if (!component.prepared) return fail(Error("prefab.invalid_prepared"));
                const auto attached = components_.AttachPrepared(published.entity, std::move(*component.prepared), component.definition->part);
                if (!attached) return fail(attached.GetError());
                published.components.push_back(attached.Value());
                const auto owner = components_.ScopeOf(attached.Value());
                if (!owner) return fail(owner.GetError());
                for (const auto &behavior : component.behaviors) {
                    const auto active = behaviors_.Attach(owner.Value(), behavior);
                    if (!active) return fail(active.GetError());
                    published.behaviors.push_back({behavior.id, component.definition->type, component.definition->part, active.Value()});
                }
            }
            for (const auto &behavior : node.behaviors) {
                const auto active = behaviors_.Attach(published.scope, behavior);
                if (!active) return fail(active.GetError());
                published.behaviors.push_back({behavior.id, {}, {}, active.Value()});
            }
        }
    } catch (const std::bad_alloc &) { (void)Rollback(*instance); throw; }
    catch (...) { return fail(Error("prefab.instantiate_exception")); }
    return Result::Success(std::move(instance));
}
foundation::Result<void> PrefabComposer::Destroy(const PrefabInstance &instance) {
    std::optional<foundation::Error> failure;
    for (auto object = instance.objects_.rbegin(); object != instance.objects_.rend(); ++object) {
        object->scope->Invalidate();
        const auto record = entities_.Resolve(object->handle);
        if (record && (record->lifecycle == entities::EntityLifecycleState::Alive || record->lifecycle == entities::EntityLifecycleState::Dormant)) {
            const auto destroyed = entities_.RequestDestroy(object->entity, entities::EntityDestroyReason::OwnerRequest);
            if (!destroyed && !failure) failure = destroyed.GetError();
        }
    }
    if (failure) return foundation::Result<void>::Failure(*failure);
    return foundation::Result<void>::Success();
}
} // namespace epidemic::gameplay::composition
