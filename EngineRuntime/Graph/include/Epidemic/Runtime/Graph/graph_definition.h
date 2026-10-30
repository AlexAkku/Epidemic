#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <cstddef>
#include <cstdint>
#include <span>
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

struct PortDefinition {
  ElementId id{};
  std::string name;
  PortKind kind = PortKind::Data;
  PortDirection direction = PortDirection::Input;
  TypeId type_id{}; // Control is a payload-free signal; Data/Event require a
                    // known type.
  [[nodiscard]] bool operator==(const PortDefinition &) const = default;
};
struct NodeDefinition {
  ElementId id{};
  TypeId type_id{};
  std::string name;
  std::vector<PortDefinition> ports;
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

// All node/port/connection IDs share one namespace scoped to asset_id. Names
// are display metadata; the authoring owner preserves IDs through
// rename/move/reload. known_types is a borrowed snapshot of the caller's schema
// type IDs, not a second registry. Structural validation executes no node and
// imposes no cycle semantics. Allocations retain ordinary C++ exception
// semantics; input is never mutated.
[[nodiscard]] foundation::Result<void>
ValidateGraph(const GraphDefinition &definition,
              std::span<const TypeId> known_types);
} // namespace epidemic::runtime::graph
