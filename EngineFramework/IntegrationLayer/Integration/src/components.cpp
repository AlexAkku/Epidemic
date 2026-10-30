#include "Epidemic/GameFramework/Integration/components.h"
#include <limits>
#include <unordered_set>

namespace epidemic::gameplay::composition {
const ComponentTypeInfo *ComponentRegistry::Find(TypeId type) const noexcept {
    const auto found = types_.find(type.Raw());
    return found == types_.end() ? nullptr : &found->second->info;
}

ComponentStore::~ComponentStore() {
    for (const auto &[_, entry] : entries_)
        entry.scope->Invalidate();
}

std::optional<ComponentHandle> ComponentStore::Find(entities::EntityId entity, TypeId type,
                                                    entities::EntityPartId part) const {
    const auto handle = entities_.HandleOf(entity);
    for (const auto &[_, entry] : entries_)
        if (entry.handle.entity == handle && entry.handle.type == type &&
            entry.handle.part == part && entry.scope->IsPresent())
            return entry.handle;
    return std::nullopt;
}

const ComponentStore::Entry *ComponentStore::Resolve(const ComponentHandle &handle) const {
    const auto found = entries_.find(handle.generation);
    return found != entries_.end() && found->second.handle == handle &&
                   found->second.scope->IsPresent()
               ? &found->second
               : nullptr;
}

foundation::Result<void>
ComponentRegistry::Validate(TypeId type, std::span<const PropertyAssignment> properties) const {
    if (!IsFrozen())
        return foundation::Result<void>::Failure(Error("component.registry_not_frozen"));
    const auto descriptor = types_.find(type.Raw());
    if (descriptor == types_.end())
        return foundation::Result<void>::Failure(Error("component.unknown_type"));
    std::unordered_set<std::uint64_t> assigned;
    try {
        for (const auto &property : properties) {
            const auto found = descriptor->second->properties.find(property.property.Raw());
            if (found == descriptor->second->properties.end())
                return foundation::Result<void>::Failure(Error("component.unknown_property"));
            if (!assigned.insert(property.property.Raw()).second)
                return foundation::Result<void>::Failure(Error("component.duplicate_property"));
            const auto valid = found->second.validate(property.value);
            if (!valid)
                return valid;
        }

    } catch (const std::bad_alloc &) {
        throw;
    } catch (...) {
        return foundation::Result<void>::Failure(Error("component.property_exception"));
    }

    return foundation::Result<void>::Success();
}

foundation::Result<ComponentRegistry::Prepared>
ComponentRegistry::Prepare(TypeId type, std::span<const PropertyAssignment> properties) const {
    using Result = foundation::Result<Prepared>;
    const auto valid = Validate(type, properties);
    if (!valid)
        return Result::Failure(valid.GetError());
    const auto &descriptor = *types_.at(type.Raw());
    Prepared prepared;
    prepared.registry_ = this;
    prepared.type_ = type;
    try {
        prepared.object_ = descriptor.create();
        if (!prepared.object_ || prepared.object_.use_count() != 1)
            return Result::Failure(Error("component.invalid_factory"));
        for (const auto &property : properties) {
            const auto written = descriptor.properties.at(property.property.Raw())
                                     .write(prepared.object_.get(), property.value);
            if (!written)
                return Result::Failure(written.GetError());
        }

        for (const auto &[_, property] : descriptor.properties) {
            const auto checked = property.validate(property.read(prepared.object_.get()));
            if (!checked)
                return Result::Failure(checked.GetError());
        }

    } catch (const std::bad_alloc &) {
        throw;
    } catch (...) {
        return Result::Failure(Error("component.factory_exception"));
    }

    return Result::Success(std::move(prepared));
}

foundation::Result<ComponentHandle>
ComponentStore::Attach(entities::EntityId entity, TypeId type,
                       std::span<const PropertyAssignment> properties,
                       entities::EntityPartId part) {
    auto prepared = registry_.Prepare(type, properties);
    if (!prepared)
        return foundation::Result<ComponentHandle>::Failure(prepared.GetError());
    return AttachPrepared(entity, std::move(prepared).Value(), part);
}

foundation::Result<ComponentHandle>
ComponentStore::AttachPrepared(entities::EntityId entity, ComponentRegistry::Prepared &&prepared,
                               entities::EntityPartId part) {
    using Result = foundation::Result<ComponentHandle>;
    if (prepared.registry_ != &registry_ || !prepared.object_)
        return Result::Failure(Error("component.invalid_prepared"));
    const auto scope = GraphScope::ForEntityState(entities_, entity, part);
    if (!scope)
        return Result::Failure(scope.GetError());
    Synchronize();
    for (const auto &[_, entry] : entries_)
        if (entry.handle.entity == scope.Value()->Handle() && entry.handle.part == part &&
            entry.handle.type == prepared.type_)
            return Result::Failure(Error("component.duplicate"));
    if (entries_.size() >= 65536 || next_ == std::numeric_limits<std::uint64_t>::max())
        return Result::Failure(Error("component.capacity"));
    const ComponentHandle handle{next_, prepared.type_, scope.Value()->Handle(), part};

    // Allocate the map node before consuming a prepared object.
    entries_.emplace(next_, Entry{handle, registry_.types_.at(prepared.type_.Raw()),
                                  prepared.object_, scope.Value()});
    prepared.object_.reset();
    ++next_;
    return Result::Success(handle);
}

foundation::Result<void> ComponentStore::Remove(const ComponentHandle &handle) {
    const auto found = entries_.find(handle.generation);
    if (found == entries_.end() || found->second.handle != handle)
        return foundation::Result<void>::Failure(Error("component.stale_handle"));
    found->second.scope->Invalidate();
    entries_.erase(found);
    return foundation::Result<void>::Success();
}

void ComponentStore::Synchronize() {
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (!it->second.scope->IsPresent()) {
            it->second.scope->Invalidate();
            it = entries_.erase(it);
        } else {
            it->second.scope->Synchronize();
            ++it;
        }

    }

}

foundation::Result<std::shared_ptr<GraphScope>>
ComponentStore::ScopeOf(const ComponentHandle &handle) const {
    const auto *entry = Resolve(handle);
    if (!entry)
        return foundation::Result<std::shared_ptr<GraphScope>>::Failure(
            Error("component.stale_handle"));
    return foundation::Result<std::shared_ptr<GraphScope>>::Success(entry->scope);
}

foundation::Result<PropertyValue> ComponentStore::ReadProperty(const ComponentHandle &handle,
                                                               PropertyId property) const {
    const auto *entry = Resolve(handle);
    if (!entry)
        return foundation::Result<PropertyValue>::Failure(Error("component.stale_handle"));
    const auto found = entry->descriptor->properties.find(property.Raw());
    if (found == entry->descriptor->properties.end())
        return foundation::Result<PropertyValue>::Failure(Error("component.unknown_property"));
    return foundation::Result<PropertyValue>::Success(found->second.read(entry->object.get()));
}

foundation::Result<void> ComponentStore::WriteProperty(const ComponentHandle &handle,
                                                       const PropertyAssignment &property) {
    const auto *entry = Resolve(handle);
    if (!entry)
        return foundation::Result<void>::Failure(Error("component.stale_handle"));
    const auto found = entry->descriptor->properties.find(property.property.Raw());
    if (found == entry->descriptor->properties.end())
        return foundation::Result<void>::Failure(Error("component.unknown_property"));
    try {
        return found->second.write(entry->object.get(), property.value);
    } catch (const std::bad_alloc &) {
        throw;
    } catch (...) {
        return foundation::Result<void>::Failure(Error("component.property_exception"));
    }

}

} // namespace epidemic::gameplay::composition
