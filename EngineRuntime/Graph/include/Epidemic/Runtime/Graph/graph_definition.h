#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <optional>
#include <string>
#include <vector>

namespace epidemic::runtime::graph {
using ElementId = foundation::StringId;
using TypeId = foundation::StringId;
inline constexpr std::size_t kMaxNodes = 1024;
inline constexpr std::size_t kMaxPorts = 8192;
inline constexpr std::size_t kMaxConnections = 16384;
inline constexpr std::size_t kMaxLabelBytes = 1024;

enum class PortKind : std::uint8_t { Data, Control, Event };

enum class PortDirection : std::uint8_t { Input, Output };

// Stable authoring identity only. Live handles, revisions and drag sessions never
// enter a GraphDefinition. Component identity includes its named part.
enum class ReferenceKind : std::uint8_t { SceneEntity, PrefabInstance, PrefabDefinition, Component };
struct GraphReference {
    ReferenceKind kind = ReferenceKind::SceneEntity;
    AssetId document{};
    ElementId placement{};
    AssetId prefab{};
    ElementId entity{};
    TypeId component{};
    ElementId part{};
    [[nodiscard]] bool operator==(const GraphReference &) const = default;
};
inline constexpr TypeId kReferenceNodeType = TypeId::FromString("epidemic.graph.reference");
inline constexpr TypeId kEntityReferenceType = TypeId::FromString("epidemic.graph.reference.entity");
inline constexpr TypeId kInstanceReferenceType = TypeId::FromString("epidemic.graph.reference.instance");
inline constexpr TypeId kPrefabReferenceType = TypeId::FromString("epidemic.graph.reference.prefab");
[[nodiscard]] inline TypeId ComponentReferenceType(TypeId component) {
    return TypeId::FromString("epidemic.graph.reference.component." + std::to_string(component.Raw()));
}
[[nodiscard]] inline TypeId ReferenceCheckType(TypeId value) {
    return TypeId::FromString("epidemic.graph.reference.valid." + std::to_string(value.Raw()));
}
[[nodiscard]] inline TypeId ReferenceValueType(const GraphReference &reference) {
    switch (reference.kind) {
    case ReferenceKind::SceneEntity: return kEntityReferenceType;
    case ReferenceKind::PrefabInstance: return kInstanceReferenceType;
    case ReferenceKind::PrefabDefinition: return kPrefabReferenceType;
    case ReferenceKind::Component: return ComponentReferenceType(reference.component);
    }
    return {};
}
[[nodiscard]] inline bool ValidReferenceIdentity(const GraphReference &r) {
    if (!r.document.IsValid() || r.kind > ReferenceKind::Component) return false;
    if (r.kind == ReferenceKind::PrefabDefinition)
        return r.prefab == r.document && !r.placement.IsValid() && !r.entity.IsValid() && !r.component.IsValid() && !r.part.IsValid();
    if (r.kind != ReferenceKind::Component && (r.component.IsValid() || r.part.IsValid())) return false;
    if (r.kind == ReferenceKind::Component && !r.component.IsValid()) return false;
    // A prefab-owned Entity/Component has no scene placement. A bare Scene node
    // has no prefab identity. Scene instances carry all three stable IDs.
    if (r.prefab.IsValid()) {
        if (r.prefab == r.document && !r.placement.IsValid()) return r.entity.IsValid() && r.kind != ReferenceKind::PrefabInstance;
        return r.placement.IsValid() && r.entity.IsValid();
    }
    return r.kind == ReferenceKind::SceneEntity && r.placement.IsValid() && !r.entity.IsValid();
}

struct PortDefinition {
    ElementId id{};

    std::string name;
    PortKind kind = PortKind::Data;
    PortDirection direction = PortDirection::Input;
    TypeId type_id{}; // Control is a payload-free signal; Data/Event require a known type.

    [[nodiscard]] bool operator==(const PortDefinition &) const = default;
};

struct NodeDefinition {
    ElementId id{};

    TypeId type_id{};

    std::string name;
    std::vector<PortDefinition> ports;
    std::optional<GraphReference> reference{};
    [[nodiscard]] bool operator==(const NodeDefinition &) const = default;
};

struct ConnectionDefinition {
    ElementId id{};

    ElementId output_port{};

    ElementId input_port{};

    [[nodiscard]] bool operator==(const ConnectionDefinition &) const = default;
};

struct GraphDefinition {
    AssetId asset_id{};

    std::string name;
    std::vector<NodeDefinition> nodes;
    std::vector<ConnectionDefinition> connections;
    [[nodiscard]] bool operator==(const GraphDefinition &) const = default;
};

// All node/port/connection IDs share one namespace scoped to asset_id. Names are
// display metadata; the authoring owner preserves IDs through rename/move/reload.
// known_types is a borrowed snapshot of the caller's schema type IDs, not a second
// registry. Structural validation executes no node and imposes no cycle semantics.
// Allocations retain ordinary C++ exception semantics; input is never mutated.
[[nodiscard]] foundation::Result<void> ValidateGraph(const GraphDefinition &definition,
                                                     std::span<const TypeId> known_types);
} // namespace epidemic::runtime::graph
