#include "Epidemic/GameFramework/Integration/components.h"
#include <limits>
#include <unordered_set>

namespace epidemic::gameplay::composition {
const ComponentTypeInfo *ComponentRegistry::Find(TypeId type) const noexcept {
    const auto found = types_.find(type.Raw()); return found == types_.end() ? nullptr : &found->second->info;
}
ComponentStore::~ComponentStore() { for (const auto &[_, entry] : entries_) entry.scope->Invalidate(); }
const ComponentStore::Entry *ComponentStore::Resolve(const ComponentHandle &handle) const {
    const auto found = entries_.find(handle.generation);
    return found != entries_.end() && found->second.handle == handle && found->second.scope->IsAlive() ? &found->second : nullptr;
}
foundation::Result<ComponentHandle> ComponentStore::Attach(entities::EntityId entity, TypeId type,
    std::span<const PropertyAssignment> properties, entities::EntityPartId part) {
    using Result = foundation::Result<ComponentHandle>;
    if (!registry_.IsFrozen()) return Result::Failure(Error("component.registry_not_frozen"));
    const auto descriptor = registry_.types_.find(type.Raw());
    if (descriptor == registry_.types_.end()) return Result::Failure(Error("component.unknown_type"));
    const auto scope = GraphScope::ForEntity(entities_, entity, part);
    if (!scope) return Result::Failure(scope.GetError());
    Synchronize();
    for (const auto &[_, entry] : entries_) if (entry.handle.entity == scope.Value()->Handle() && entry.handle.part == part && entry.handle.type == type)
        return Result::Failure(Error("component.duplicate"));
    if (entries_.size() >= 65536 || next_ == std::numeric_limits<std::uint64_t>::max()) return Result::Failure(Error("component.capacity"));
    std::shared_ptr<void> object;
    try {
        object = descriptor->second->create();
        if (!object || object.use_count() != 1) return Result::Failure(Error("component.invalid_factory"));
        std::unordered_set<std::uint64_t> assigned;
        for (const auto &property : properties) {
            const auto found = descriptor->second->properties.find(property.property.Raw());
            if (found == descriptor->second->properties.end()) return Result::Failure(Error("component.unknown_property"));
            if (!assigned.insert(property.property.Raw()).second) return Result::Failure(Error("component.duplicate_property"));
            const auto written = found->second.write(object.get(), property.value);
            if (!written) return Result::Failure(written.GetError());
        }
    } catch (const std::bad_alloc &) { throw; }
    catch (...) { return Result::Failure(Error("component.factory_exception")); }
    const ComponentHandle handle{next_, type, scope.Value()->Handle(), part};
    entries_.emplace(next_, Entry{handle, descriptor->second, std::move(object), scope.Value()});
    ++next_;
    return Result::Success(handle);
}
foundation::Result<void> ComponentStore::Remove(const ComponentHandle &handle) {
    const auto found = entries_.find(handle.generation);
    if (found == entries_.end() || found->second.handle != handle) return foundation::Result<void>::Failure(Error("component.stale_handle"));
    found->second.scope->Invalidate(); entries_.erase(found);
    return foundation::Result<void>::Success();
}
void ComponentStore::Synchronize() {
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (!it->second.scope->IsAlive()) { it->second.scope->Invalidate(); it = entries_.erase(it); }
        else ++it;
    }
}
foundation::Result<std::shared_ptr<GraphScope>> ComponentStore::ScopeOf(const ComponentHandle &handle) const {
    const auto *entry = Resolve(handle);
    if (!entry) return foundation::Result<std::shared_ptr<GraphScope>>::Failure(Error("component.stale_handle"));
    return foundation::Result<std::shared_ptr<GraphScope>>::Success(entry->scope);
}
foundation::Result<PropertyValue> ComponentStore::ReadProperty(const ComponentHandle &handle, PropertyId property) const {
    const auto *entry = Resolve(handle);
    if (!entry) return foundation::Result<PropertyValue>::Failure(Error("component.stale_handle"));
    const auto found = entry->descriptor->properties.find(property.Raw());
    if (found == entry->descriptor->properties.end()) return foundation::Result<PropertyValue>::Failure(Error("component.unknown_property"));
    return foundation::Result<PropertyValue>::Success(found->second.read(entry->object.get()));
}
foundation::Result<void> ComponentStore::WriteProperty(const ComponentHandle &handle, const PropertyAssignment &property) {
    const auto *entry = Resolve(handle);
    if (!entry) return foundation::Result<void>::Failure(Error("component.stale_handle"));
    const auto found = entry->descriptor->properties.find(property.property.Raw());
    if (found == entry->descriptor->properties.end()) return foundation::Result<void>::Failure(Error("component.unknown_property"));
    try { return found->second.write(entry->object.get(), property.value); }
    catch (const std::bad_alloc &) { throw; }
    catch (...) { return foundation::Result<void>::Failure(Error("component.property_exception")); }
}
} // namespace epidemic::gameplay::composition
