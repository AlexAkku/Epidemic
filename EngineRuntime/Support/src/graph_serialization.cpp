#include "Epidemic/Runtime/Support/graph_serialization.h"

#include <exception>

namespace epidemic::runtime {
namespace {
// Keep the archive's original Result error across the adapter's local transfer
// helpers. Allocation/other exceptions remain handled by the existing typed
// boundary.
struct TransferFailure final : std::exception {
  explicit TransferFailure(foundation::Error value) : error(std::move(value)) {}
  const char *what() const noexcept override { return error.message.c_str(); }
  foundation::Error error;
};
void Need(const foundation::Result<void> &result) {
  if (!result)
    throw TransferFailure{result.GetError()};
}
template <typename T> T Take(foundation::Result<T> result) {
  if (!result)
    throw TransferFailure{result.GetError()};
  return std::move(result).Value();
}
void Require(bool value, std::string_view code) {
  if (!value)
    throw TransferFailure{
        foundation::Error::Create(code, "graph archive rejected")};
}
graph::ElementId ReadId(IArchiveReader &reader, std::string_view field) {
  return graph::ElementId{Take(reader.ReadUInt64(field))};
}
std::string ReadLabel(IArchiveReader &reader) {
  auto name = Take(reader.ReadString("name"));
  Require(name.size() <= graph::kMaxLabelBytes, "graph.limit_exceeded");
  return name;
}
void WriteDefinition(const graph::GraphDefinition &definition,
                     IArchiveWriter &writer) {
  Need(writer.WriteUInt64("asset_id", definition.asset_id.Raw()));
  Need(writer.WriteString("name", definition.name));
  Need(writer.BeginArray("nodes", definition.nodes.size()));
  for (std::size_t index = 0; index < definition.nodes.size(); ++index) {
    const auto &node = definition.nodes[index];
    Need(writer.BeginArrayElement(index));
    Need(writer.WriteUInt64("id", node.id.Raw()));
    Need(writer.WriteUInt64("type_id", node.type_id.Raw()));
    Need(writer.WriteString("name", node.name));
    Need(writer.BeginArray("ports", node.ports.size()));
    for (std::size_t port_index = 0; port_index < node.ports.size();
         ++port_index) {
      const auto &port = node.ports[port_index];
      Need(writer.BeginArrayElement(port_index));
      Need(writer.WriteUInt64("id", port.id.Raw()));
      Need(writer.WriteString("name", port.name));
      Need(writer.WriteUInt64("kind", static_cast<std::uint64_t>(port.kind)));
      Need(writer.WriteUInt64("direction",
                              static_cast<std::uint64_t>(port.direction)));
      Need(writer.WriteUInt64("type_id", port.type_id.Raw()));
      Need(writer.EndArrayElement());
    }
    Need(writer.EndArray());
    Need(writer.EndArrayElement());
  }
  Need(writer.EndArray());
  Need(writer.BeginArray("connections", definition.connections.size()));
  for (std::size_t index = 0; index < definition.connections.size(); ++index) {
    const auto &connection = definition.connections[index];
    Need(writer.BeginArrayElement(index));
    Need(writer.WriteUInt64("id", connection.id.Raw()));
    Need(writer.WriteUInt64("output", connection.output_port.Raw()));
    Need(writer.WriteUInt64("input", connection.input_port.Raw()));
    Need(writer.EndArrayElement());
  }
  Need(writer.EndArray());
}
graph::GraphDefinition ReadDefinition(IArchiveReader &reader) {
  graph::GraphDefinition definition;
  definition.asset_id = AssetId{ReadId(reader, "asset_id")};
  definition.name = ReadLabel(reader);
  const auto node_count = Take(reader.BeginArray("nodes"));
  Require(node_count <= graph::kMaxNodes, "graph.limit_exceeded");
  definition.nodes.reserve(node_count);
  std::size_t total_ports = 0;
  for (std::size_t index = 0; index < node_count; ++index) {
    Need(reader.BeginArrayElement(index));
    graph::NodeDefinition node;
    node.id = ReadId(reader, "id");
    node.type_id = ReadId(reader, "type_id");
    node.name = ReadLabel(reader);
    const auto port_count = Take(reader.BeginArray("ports"));
    Require(port_count <= graph::kMaxPorts - total_ports,
            "graph.limit_exceeded");
    total_ports += port_count;
    node.ports.reserve(port_count);
    for (std::size_t port_index = 0; port_index < port_count; ++port_index) {
      Need(reader.BeginArrayElement(port_index));
      graph::PortDefinition port;
      port.id = ReadId(reader, "id");
      port.name = ReadLabel(reader);
      const auto kind = Take(reader.ReadUInt64("kind"));
      const auto direction = Take(reader.ReadUInt64("direction"));
      Require(kind <= static_cast<std::uint64_t>(graph::PortKind::Event),
              "graph.invalid_port_kind");
      Require(direction <=
                  static_cast<std::uint64_t>(graph::PortDirection::Output),
              "graph.invalid_port_direction");
      port.kind = static_cast<graph::PortKind>(kind);
      port.direction = static_cast<graph::PortDirection>(direction);
      port.type_id = ReadId(reader, "type_id");
      node.ports.push_back(std::move(port));
      Need(reader.EndArrayElement());
    }
    Need(reader.EndArray());
    definition.nodes.push_back(std::move(node));
    Need(reader.EndArrayElement());
  }
  Need(reader.EndArray());
  const auto connection_count = Take(reader.BeginArray("connections"));
  Require(connection_count <= graph::kMaxConnections, "graph.limit_exceeded");
  definition.connections.reserve(connection_count);
  for (std::size_t index = 0; index < connection_count; ++index) {
    Need(reader.BeginArrayElement(index));
    graph::ConnectionDefinition connection;
    connection.id = ReadId(reader, "id");
    connection.output_port = ReadId(reader, "output");
    connection.input_port = ReadId(reader, "input");
    definition.connections.push_back(connection);
    Need(reader.EndArrayElement());
  }
  Need(reader.EndArray());
  return definition;
}
} // namespace

foundation::StringId GraphDefinitionSerializer::GetTypeId() const {
  return foundation::StringId::FromString("epidemic.graph_definition");
}
SchemaVersion GraphDefinitionSerializer::GetSchemaVersion() const {
  return {1, 0, 0};
}
std::type_index GraphDefinitionSerializer::GetCppType() const {
  return typeid(graph::GraphDefinition);
}
foundation::Result<void>
GraphDefinitionSerializer::Serialize(const void *object,
                                     IArchiveWriter &writer) const {
  const auto &definition = *static_cast<const graph::GraphDefinition *>(object);
  try {
    Need(graph::ValidateGraph(definition, known_types_));
    WriteDefinition(definition, writer);
    return foundation::Result<void>::Success();
  } catch (const TransferFailure &failure) {
    return foundation::Result<void>::Failure(failure.error);
  }
}
foundation::Result<void>
GraphDefinitionSerializer::Deserialize(IArchiveReader &reader,
                                       void *object) const {
  try {
    auto candidate = ReadDefinition(reader);
    Need(graph::ValidateGraph(candidate, known_types_));
    *static_cast<graph::GraphDefinition *>(object) = std::move(candidate);
    return foundation::Result<void>::Success();
  } catch (const TransferFailure &failure) {
    return foundation::Result<void>::Failure(failure.error);
  }
}
} // namespace epidemic::runtime
