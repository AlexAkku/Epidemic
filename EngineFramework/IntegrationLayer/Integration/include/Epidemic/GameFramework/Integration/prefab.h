#pragma once
#include "Epidemic/GameFramework/Integration/components.h"
#include "Epidemic/GameFramework/Integration/graph_behavior.h"

namespace epidemic::gameplay::composition {
using PrefabId = foundation::StringId;
using PrefabNodeId = foundation::StringId;
struct PrefabParameter {
  PropertyId id{};
  PropertyValue value;
};
struct PrefabProperty {
  PropertyId property{};
  PropertyValue value;
  PropertyId parameter{};
};
struct PrefabComponent {
  TypeId type{};
  entities::EntityPartId part{};
  std::vector<PrefabProperty> properties;
  std::vector<GraphBehaviorDefinition> behaviors;
};
struct PrefabNode {
  PrefabNodeId id{};
  entities::EntityArchetypeId archetype{};
  std::vector<PrefabComponent> components;
  std::vector<GraphBehaviorDefinition> behaviors;
  std::vector<PrefabNode> children;
};
struct PrefabDefinition {
  PrefabId id{};
  std::vector<PrefabParameter> parameters;
  PrefabNode root;
};
struct ComponentOverride {
  PrefabNodeId node{};
  TypeId component{};
  entities::EntityPartId part{};
  PropertyAssignment property;
};
struct BehaviorInputOverride {
  PrefabNodeId node{};
  TypeId component{}; // Invalid: behavior belongs to the Entity itself.
  entities::EntityPartId part{};
  foundation::StringId behavior{};
  GraphInput input;
};
struct PrefabOverrides {
  std::vector<PropertyAssignment> parameters;
  std::vector<ComponentOverride> components;
  std::vector<BehaviorInputOverride> behaviors;
};
class PrefabInstance final {
public:
  struct BehaviorSlot {
    foundation::StringId definition{};
    TypeId component{};
    entities::EntityPartId part{};
    BehaviorHandle handle{};
  };
  struct Object {
    PrefabNodeId id{}, parent{};
    entities::EntityId entity{};
    entities::EntityHandle handle{};
    std::shared_ptr<GraphScope> scope;
    std::vector<ComponentHandle> components;
    std::vector<BehaviorSlot> behaviors;
  };
  [[nodiscard]] const std::shared_ptr<const PrefabDefinition> &
  Definition() const noexcept {
    return definition_;
  }
  [[nodiscard]] const PrefabOverrides &Overrides() const noexcept {
    return overrides_;
  }
  [[nodiscard]] const std::vector<Object> &Objects() const noexcept {
    return objects_;
  }
  [[nodiscard]] const Object *Find(PrefabNodeId node) const noexcept;

private:
  std::shared_ptr<const PrefabDefinition> definition_;
  PrefabOverrides overrides_;
  std::vector<Object> objects_;
  friend class PrefabComposer;
};
// A definition/instantiation adapter for the existing EntityService. Dropping
// instance metadata does not destroy Entities; Destroy requests their normal
// lifecycle transition. The host remains responsible for its usual destruction
// commit and RuntimeBridge projection processing.
class PrefabComposer final {
public:
  PrefabComposer(entities::EntityService &entities,
                 const ComponentRegistry &registry, ComponentStore &components,
                 GraphBehaviorService &behaviors)
      : entities_(entities), registry_(registry), components_(components),
        behaviors_(behaviors) {}
  PrefabComposer(const PrefabComposer &) = delete;
  PrefabComposer &operator=(const PrefabComposer &) = delete;
  [[nodiscard]] foundation::Result<PrefabId>
  Register(const PrefabDefinition &definition);
  [[nodiscard]] foundation::Result<void>
  Validate(PrefabId id, const PrefabOverrides &overrides = {}) const;
  [[nodiscard]] std::shared_ptr<const PrefabDefinition> Find(PrefabId id) const;
  [[nodiscard]] foundation::Result<std::shared_ptr<const PrefabInstance>>
  Spawn(PrefabId id, const PrefabOverrides &overrides = {});
  [[nodiscard]] foundation::Result<void>
  Destroy(const PrefabInstance &instance);

private:
  struct PlannedComponent {
    const PrefabComponent *definition = nullptr;
    std::optional<ComponentRegistry::Prepared> prepared;
    std::vector<GraphBehaviorDefinition> behaviors;
  };
  struct PlannedNode {
    const PrefabNode *definition = nullptr;
    PrefabNodeId parent{};
    std::vector<PlannedComponent> components;
    std::vector<GraphBehaviorDefinition> behaviors;
  };
  [[nodiscard]] foundation::Result<std::vector<PlannedNode>>
  Plan(const PrefabDefinition &definition, const PrefabOverrides &overrides,
       bool prepare) const;
  [[nodiscard]] foundation::Result<void> Rollback(PrefabInstance &instance);
  entities::EntityService &entities_;
  const ComponentRegistry &registry_;
  ComponentStore &components_;
  GraphBehaviorService &behaviors_;
  std::unordered_map<std::uint64_t, std::shared_ptr<const PrefabDefinition>>
      definitions_;
};
} // namespace epidemic::gameplay::composition
