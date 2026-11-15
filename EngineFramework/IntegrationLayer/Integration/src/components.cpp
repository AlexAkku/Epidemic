#include "Epidemic/GameFramework/Integration/components.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace epidemic::gameplay::composition {
foundation::Result<void> ComponentRegistry::RegisterDescriptions(std::span<const DescribedComponent> schemas) {
    if(!IsFrozen())return foundation::Result<void>::Failure(Error("component.registry_not_frozen"));
    using Object = std::unordered_map<std::uint64_t, PropertyValue>;
    auto staged = types_;
    auto descriptions = descriptions_;
    std::unordered_set<std::uint64_t> seen;
    if (schemas.size() > 64) return foundation::Result<void>::Failure(Error("component.schema_limit"));
    for (const auto &schema : schemas) {
        const auto id = TypeId::FromString(schema.name);
        if (schema.name.empty() || schema.name.size() > 256 || schema.version != 1 || schema.fields.empty() || schema.fields.size() > 64 || !seen.insert(id.Raw()).second)
            return foundation::Result<void>::Failure(Error("component.invalid_schema"));
        if (const auto old = descriptions.find(id.Raw()); old != descriptions.end()) {
            bool equal = old->second.version == schema.version && old->second.fields.size() == schema.fields.size();
            for (std::size_t index = 0; equal && index < schema.fields.size(); ++index)
                equal = old->second.fields[index].name == schema.fields[index].name && old->second.fields[index].initial == schema.fields[index].initial;
            if (!equal) return foundation::Result<void>::Failure(Error("component.migration_required"));
            continue;
        }
        if (staged.contains(id.Raw())) return foundation::Result<void>::Failure(Error("component.duplicate_type"));
        auto descriptor = std::make_shared<Descriptor>();
        descriptor->info = {id,schema.name,{}};
        Object defaults;
        for (const auto &field : schema.fields) {
            const auto key = PropertyId::FromString(field.name);
            if (field.name.empty() || field.name.size() > 256 || defaults.contains(key.Raw()))
                return foundation::Result<void>::Failure(Error("component.duplicate_property"));
            const auto validate = [kind=field.initial.index()](const PropertyValue &value) {
                bool valid=value.index()==kind;
                if(const auto *number=std::get_if<double>(&value))valid=valid&&std::isfinite(*number);
                if(const auto *text=std::get_if<std::string>(&value))valid=valid&&text->size()<=16384;
                return valid ? foundation::Result<void>::Success() : foundation::Result<void>::Failure(Error("component.property_type"));
            };
            auto valid=validate(field.initial);if(!valid)return valid;
            defaults.emplace(key.Raw(),field.initial);
            descriptor->info.properties.push_back({key,field.name,static_cast<PropertyKind>(field.initial.index())});
            descriptor->properties.emplace(key.Raw(),Property{validate,
                [key](const void *object){return static_cast<const Object *>(object)->at(key.Raw());},
                [key,validate](void *object,const PropertyValue &value){auto result=validate(value);if(result)static_cast<Object *>(object)->at(key.Raw())=value;return result;}});
        }
        descriptor->create=[initial=std::make_shared<const Object>(std::move(defaults))]{return std::static_pointer_cast<void>(std::make_shared<Object>(*initial));};
        descriptions.emplace(id.Raw(),schema);staged.emplace(id.Raw(),std::move(descriptor));
    }
    std::unordered_set<std::uint64_t> retained;
    for(const auto &schema:schemas)retained.insert(TypeId::FromString(schema.name).Raw());
    for(auto it=descriptions.begin();it!=descriptions.end();)if(!retained.contains(it->first)){staged.erase(it->first);it=descriptions.erase(it);}else ++it;
    types_.swap(staged);descriptions_.swap(descriptions);
    return foundation::Result<void>::Success();
}

foundation::Result<std::shared_ptr<ComponentRegistry>> ComponentRegistry::WithDescriptions(std::span<const DescribedComponent> schemas) const {
    using Result=foundation::Result<std::shared_ptr<ComponentRegistry>>;
    if(!IsFrozen())return Result::Failure(Error("component.registry_not_frozen"));
    auto staged=std::make_shared<ComponentRegistry>();staged->types_=types_;staged->descriptions_=descriptions_;staged->Freeze();
    auto valid=staged->RegisterDescriptions(schemas);if(!valid)return Result::Failure(valid.GetError());
    return Result::Success(std::move(staged));
}

std::vector<ComponentTypeInfo> ComponentRegistry::Types() const {
    std::vector<ComponentTypeInfo> result;
    result.reserve(types_.size());
    for (const auto &item : types_)
        result.push_back(item.second->info);
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return a.name < b.name; });
    return result;
}

foundation::Result<std::vector<PropertyAssignment>> ComponentRegistry::Defaults(TypeId type) const {
    using Result = foundation::Result<std::vector<PropertyAssignment>>;
    auto prepared = Prepare(type, {});
    if (!prepared)
        return Result::Failure(prepared.GetError());
    const auto &descriptor = *types_.at(type.Raw());
    std::vector<PropertyAssignment> result;
    result.reserve(descriptor.info.properties.size());
    for (const auto &property : descriptor.info.properties)
        result.push_back({property.id, descriptor.properties.at(property.id.Raw()).read(prepared.Value().object_.get())});
    return Result::Success(std::move(result));
}
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
