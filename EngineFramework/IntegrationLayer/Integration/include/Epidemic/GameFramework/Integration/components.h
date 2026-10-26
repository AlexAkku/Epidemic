#pragma once
#include "Epidemic/GameFramework/Integration/graph_scope.h"
#include "Epidemic/GameFramework/Foundation/type_registry.h"
#include "Epidemic/Foundation/string_id.h"
#include <functional>
#include <typeindex>
#include <variant>
#include <span>
#include <type_traits>

namespace epidemic::gameplay::composition {
using PropertyId = foundation::StringId;
using PropertyValue = std::variant<std::int64_t, double, bool, std::string>;
struct PropertyAssignment { PropertyId property{}; PropertyValue value; };
enum class PropertyKind { Integer, Real, Boolean, String };
struct PropertyInfo { PropertyId id{}; std::string name; PropertyKind kind = PropertyKind::Integer; };
struct ComponentTypeInfo { TypeId id{}; std::string name; std::vector<PropertyInfo> properties; };

class ComponentRegistry final {
  public:
    template <typename T> foundation::Result<TypeId> Register(std::string_view name,
        std::function<std::shared_ptr<T>()> create = [] { return std::make_shared<T>(); }) {
        using Result = foundation::Result<TypeId>;
        if (IsFrozen()) return Result::Failure(Error("component.registry_frozen"));
        if (!create) return Result::Failure(Error("component.invalid_factory"));
        const auto id = TypeId::FromString(name);
        auto descriptor = std::make_shared<Descriptor>();
        descriptor->info = {id, std::string(name), {}}; descriptor->native_type = typeid(T);
        descriptor->create = [create = std::move(create)] { return std::static_pointer_cast<void>(create()); };
        decltype(types_) staged; staged.emplace(id.Raw(), std::move(descriptor));
        types_.reserve(types_.size() + 1);
        const auto registered = names_.Register(name);
        if (!registered) return Result::Failure(registered.GetError());
        types_.insert(staged.extract(id.Raw()));
        return Result::Success(id);
    }
    template <typename T, typename V> foundation::Result<PropertyId> RegisterProperty(
        TypeId type, std::string_view name, V T::*member, bool (*validate)(const V &) = nullptr) {
        using Result = foundation::Result<PropertyId>;
        static_assert(std::is_same_v<V, std::int64_t> || std::is_same_v<V, double> ||
                      std::is_same_v<V, bool> || std::is_same_v<V, std::string>);
        if (IsFrozen()) return Result::Failure(Error("component.registry_frozen"));
        const auto found = types_.find(type.Raw());
        if (found == types_.end() || found->second->native_type != typeid(T) || !member)
            return Result::Failure(Error("component.type_mismatch"));
        auto &descriptor = *found->second;
        const auto id = PropertyId::FromString(name);
        Property property;
        property.read = [member](const void *object) { return PropertyValue{static_cast<const T *>(object)->*member}; };
        property.write = [member, validate](void *object, const PropertyValue &value) {
            const auto *typed = std::get_if<V>(&value);
            if (!typed) return foundation::Result<void>::Failure(Error("component.property_type"));
            if (validate && !validate(*typed)) return foundation::Result<void>::Failure(Error("component.value_rejected"));
            static_cast<T *>(object)->*member = *typed;
            return foundation::Result<void>::Success();
        };
        PropertyInfo info{id, std::string(name), static_cast<PropertyKind>(PropertyValue{V{}}.index())};
        decltype(descriptor.properties) staged; staged.emplace(id.Raw(), std::move(property));
        descriptor.properties.reserve(descriptor.properties.size() + 1);
        descriptor.info.properties.reserve(descriptor.info.properties.size() + 1);
        const auto registered = descriptor.names.Register(name);
        if (!registered) return Result::Failure(registered.GetError());
        descriptor.properties.insert(staged.extract(id.Raw()));
        descriptor.info.properties.push_back(std::move(info));
        return Result::Success(id);
    }
    void Freeze() noexcept { names_.Freeze(); }
    [[nodiscard]] bool IsFrozen() const noexcept { return names_.IsFrozen(); }
    [[nodiscard]] const ComponentTypeInfo *Find(TypeId type) const noexcept;
  private:
    static foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "component contract rejected"); }
    struct Property {
        std::function<PropertyValue(const void *)> read;
        std::function<foundation::Result<void>(void *, const PropertyValue &)> write;
    };
    struct Descriptor {
        ComponentTypeInfo info;
        std::type_index native_type{typeid(void)};
        std::function<std::shared_ptr<void>()> create;
        StableTypeRegistry<PropertyId> names;
        std::unordered_map<std::uint64_t, Property> properties;
    };
    StableTypeRegistry<TypeId> names_;
    std::unordered_map<std::uint64_t, std::shared_ptr<Descriptor>> types_;
    friend class ComponentStore;
};
struct ComponentHandle {
    std::uint64_t generation = 0;
    TypeId type{};
    entities::EntityHandle entity{};
    entities::EntityPartId part{};
    [[nodiscard]] bool operator==(const ComponentHandle &) const noexcept = default;
};
class ComponentStore final {
  public:
    ComponentStore(entities::EntityService &entities, const ComponentRegistry &registry) : entities_(entities), registry_(registry) {}
    ComponentStore(const ComponentStore &) = delete;
    ComponentStore &operator=(const ComponentStore &) = delete;
    ~ComponentStore();
    [[nodiscard]] foundation::Result<ComponentHandle> Attach(entities::EntityId entity, TypeId type,
        std::span<const PropertyAssignment> properties = {}, entities::EntityPartId part = {});
    [[nodiscard]] foundation::Result<void> Remove(const ComponentHandle &handle);
    void Synchronize();
    [[nodiscard]] foundation::Result<std::shared_ptr<GraphScope>> ScopeOf(const ComponentHandle &handle) const;
    [[nodiscard]] foundation::Result<PropertyValue> ReadProperty(const ComponentHandle &handle, PropertyId property) const;
    [[nodiscard]] foundation::Result<void> WriteProperty(const ComponentHandle &handle, const PropertyAssignment &property);
    template <typename T> [[nodiscard]] foundation::Result<const T *> Get(const ComponentHandle &handle) const {
        const auto *entry = Resolve(handle);
        if (!entry) return foundation::Result<const T *>::Failure(Error("component.stale_handle"));
        if (entry->descriptor->native_type != typeid(T)) return foundation::Result<const T *>::Failure(Error("component.type_mismatch"));
        return foundation::Result<const T *>::Success(static_cast<const T *>(entry->object.get()));
    }
    template <typename V> [[nodiscard]] foundation::Result<V> Read(const ComponentHandle &handle, PropertyId property) const {
        const auto value = ReadProperty(handle, property);
        if (!value) return foundation::Result<V>::Failure(value.GetError());
        const auto *typed = std::get_if<V>(&value.Value());
        if (!typed) return foundation::Result<V>::Failure(Error("component.property_type"));
        return foundation::Result<V>::Success(*typed);
    }
  private:
    static foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "component contract rejected"); }
    struct Entry {
        ComponentHandle handle;
        std::shared_ptr<const ComponentRegistry::Descriptor> descriptor;
        std::shared_ptr<void> object;
        std::shared_ptr<GraphScope> scope;
    };
    const Entry *Resolve(const ComponentHandle &handle) const;
    entities::EntityService &entities_;
    const ComponentRegistry &registry_;
    std::uint64_t next_ = 1;
    std::unordered_map<std::uint64_t, Entry> entries_;
};
} // namespace epidemic::gameplay::composition
