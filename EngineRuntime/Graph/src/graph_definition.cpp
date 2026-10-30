#include "Epidemic/Runtime/Graph/graph_definition.h"

#include <unordered_map>
#include <unordered_set>

namespace epidemic::runtime::graph {
namespace {
foundation::Result<void> Failure(std::string_view code) {
  return foundation::Result<void>::Failure(
      foundation::Error::Create(code, "graph definition rejected"));
}
struct EdgeKey {
  ElementId output;
  ElementId input;
  bool operator==(const EdgeKey &) const = default;
};
struct EdgeHash {
  std::size_t operator()(const EdgeKey &key) const noexcept {
    return std::hash<std::uint64_t>{}(key.output.Raw()) ^
           (std::hash<std::uint64_t>{}(key.input.Raw()) << 1U);
  }
};
} // namespace

foundation::Result<void> ValidateGraph(const GraphDefinition &definition,
                                       std::span<const TypeId> known_types) {
  if (!definition.asset_id.IsValid())
    return Failure("graph.invalid_asset");
  if (definition.nodes.size() > kMaxNodes ||
      definition.connections.size() > kMaxConnections ||
      definition.name.size() > kMaxLabelBytes)
    return Failure("graph.limit_exceeded");
  const std::unordered_set<TypeId> types(known_types.begin(),
                                         known_types.end());
  std::unordered_set<ElementId> ids;
  std::unordered_map<ElementId, const PortDefinition *> ports;
  const auto add_id = [&](ElementId id) -> foundation::Result<void> {
    if (!id.IsValid())
      return Failure("graph.invalid_id");
    if (!ids.insert(id).second)
      return Failure("graph.duplicate_id");
    return foundation::Result<void>::Success();
  };
  for (const auto &node : definition.nodes) {
    if (auto added = add_id(node.id); !added)
      return added;
    if (!node.type_id.IsValid() || !types.contains(node.type_id))
      return Failure("graph.unknown_type");
    if (node.name.size() > kMaxLabelBytes ||
        node.ports.size() > kMaxPorts - ports.size())
      return Failure("graph.limit_exceeded");
    for (const auto &port : node.ports) {
      if (auto added = add_id(port.id); !added)
        return added;
      if (port.name.size() > kMaxLabelBytes)
        return Failure("graph.limit_exceeded");
      if (port.direction != PortDirection::Input &&
          port.direction != PortDirection::Output)
        return Failure("graph.invalid_port_direction");
      switch (port.kind) {
      case PortKind::Control:
        if (port.type_id.IsValid())
          return Failure("graph.control_payload");
        break;
      case PortKind::Data:
      case PortKind::Event:
        if (!port.type_id.IsValid() || !types.contains(port.type_id))
          return Failure("graph.unknown_type");
        break;
      default:
        return Failure("graph.invalid_port_kind");
      }
      ports.emplace(port.id, &port);
    }
  }
  std::unordered_set<EdgeKey, EdgeHash> edges;
  std::unordered_set<ElementId> data_inputs;
  for (const auto &connection : definition.connections) {
    if (auto added = add_id(connection.id); !added)
      return added;
    const auto output = ports.find(connection.output_port);
    const auto input = ports.find(connection.input_port);
    if (output == ports.end() || input == ports.end())
      return Failure("graph.missing_port");
    if (output->second->direction != PortDirection::Output ||
        input->second->direction != PortDirection::Input)
      return Failure("graph.connection_direction");
    if (output->second->kind != input->second->kind)
      return Failure("graph.connection_kind");
    if (output->second->type_id != input->second->type_id)
      return Failure("graph.connection_type");
    if (!edges.insert({connection.output_port, connection.input_port}).second)
      return Failure("graph.duplicate_connection");
    if (input->second->kind == PortKind::Data &&
        !data_inputs.insert(connection.input_port).second)
      return Failure("graph.data_input_connected");
  }
  return foundation::Result<void>::Success();
}
} // namespace epidemic::runtime::graph
