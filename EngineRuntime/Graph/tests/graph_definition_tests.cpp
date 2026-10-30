#include "Epidemic/Runtime/Graph/graph_definition.h"

#include <array>
#include <bit>
#include <cstdio>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime;
using namespace epidemic::runtime::graph;
namespace {
const std::array types{
    TypeId::FromString("type.node"), TypeId::FromString("type.int"),
    TypeId::FromString("type.bool"), TypeId::FromString("type.event")};
ElementId Id(std::string_view value) { return ElementId::FromString(value); }
void Check(bool value, const char *message) {
  if (!value) {
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
    throw std::runtime_error(message);
  }
}
void ErrorIs(const foundation::Result<void> &value, std::string_view code) {
  Check(!value && value.GetError().HasCode(code),
        "unexpected graph validation error");
}
GraphDefinition Example() {
  GraphDefinition definition{
      AssetId::FromString("identity.graph.1"), "Example", {}, {}};
  definition.nodes = {{Id("node.source"),
                       types[0],
                       "Source",
                       {{Id("out.data"), "Value", PortKind::Data,
                         PortDirection::Output, types[1]},
                        {Id("out.control"),
                         "Next",
                         PortKind::Control,
                         PortDirection::Output,
                         {}},
                        {Id("out.event"), "Changed", PortKind::Event,
                         PortDirection::Output, types[3]}}},
                      {Id("node.target"),
                       types[0],
                       "Target",
                       {{Id("in.data"), "Value", PortKind::Data,
                         PortDirection::Input, types[1]},
                        {Id("in.control"),
                         "Enter",
                         PortKind::Control,
                         PortDirection::Input,
                         {}},
                        {Id("in.event"), "Changed", PortKind::Event,
                         PortDirection::Input, types[3]}}}};
  definition.connections = {
      {Id("edge.data"), Id("out.data"), Id("in.data")},
      {Id("edge.control"), Id("out.control"), Id("in.control")},
      {Id("edge.event"), Id("out.event"), Id("in.event")}};
  return definition;
}
void ValidDefinitions() {
  auto definition = Example();
  Check(ValidateGraph(definition, types).HasValue(),
        "all three typed port kinds");
  definition.nodes[1].ports.push_back({Id("in.extra"), "Second", PortKind::Data,
                                       PortDirection::Input, types[1]});
  definition.connections.push_back(
      {Id("edge.fanout"), Id("out.data"), Id("in.extra")});
  Check(ValidateGraph(definition, types).HasValue(), "data output fan-out");
  definition.nodes[0].ports.push_back(
      {Id("in.loop"), "Loop", PortKind::Control, PortDirection::Input, {}});
  definition.nodes[1].ports.push_back(
      {Id("out.loop"), "Loop", PortKind::Control, PortDirection::Output, {}});
  definition.connections.push_back(
      {Id("edge.loop"), Id("out.loop"), Id("in.loop")});
  Check(ValidateGraph(definition, types).HasValue(),
        "structural control cycle executes nothing");
  const auto original_id = definition.nodes[0].id;
  definition.nodes[0].name = "Renamed";
  Check(definition.nodes[0].id == original_id &&
            ValidateGraph(definition, types).HasValue(),
        "rename preserves identity");
  auto other = definition;
  other.asset_id = AssetId::FromString("identity.graph.2");
  Check(ValidateGraph(other, types).HasValue(),
        "element identity is scoped to asset");
  GraphDefinition empty{AssetId::FromString("empty"), {}, {}, {}};
  Check(ValidateGraph(empty, {}).HasValue(), "empty authored graph");
}
void InvalidDefinitions() {
  auto definition = Example();
  definition.asset_id = {};
  ErrorIs(ValidateGraph(definition, types), "graph.invalid_asset");
  definition = Example();
  definition.nodes[0].id = {};
  ErrorIs(ValidateGraph(definition, types), "graph.invalid_id");
  definition = Example();
  definition.nodes[1].id = definition.nodes[0].ports[0].id;
  ErrorIs(ValidateGraph(definition, types), "graph.duplicate_id");
  definition = Example();
  definition.connections[0].id = definition.nodes[0].id;
  ErrorIs(ValidateGraph(definition, types), "graph.duplicate_id");
  definition = Example();
  definition.nodes[0].type_id = Id("unknown.type");
  ErrorIs(ValidateGraph(definition, types), "graph.unknown_type");
  definition = Example();
  definition.nodes[0].ports[0].type_id = {};
  ErrorIs(ValidateGraph(definition, types), "graph.unknown_type");
  definition = Example();
  definition.nodes[0].ports[1].type_id = types[1];
  ErrorIs(ValidateGraph(definition, types), "graph.control_payload");
  definition = Example();
  definition.nodes[0].ports[0].kind =
      std::bit_cast<PortKind>(std::uint8_t{255});
  ErrorIs(ValidateGraph(definition, types), "graph.invalid_port_kind");
  definition = Example();
  definition.nodes[0].ports[0].direction =
      std::bit_cast<PortDirection>(std::uint8_t{255});
  ErrorIs(ValidateGraph(definition, types), "graph.invalid_port_direction");
  definition = Example();
  definition.connections[0].input_port = Id("missing");
  ErrorIs(ValidateGraph(definition, types), "graph.missing_port");
  definition = Example();
  definition.connections[0].output_port = Id("in.data");
  ErrorIs(ValidateGraph(definition, types), "graph.connection_direction");
  definition = Example();
  definition.connections[0].input_port = Id("in.event");
  ErrorIs(ValidateGraph(definition, types), "graph.connection_kind");
  definition = Example();
  definition.nodes[1].ports[0].type_id = types[2];
  ErrorIs(ValidateGraph(definition, types), "graph.connection_type");
  definition = Example();
  definition.connections.push_back(
      {Id("duplicate.edge"), Id("out.data"), Id("in.data")});
  ErrorIs(ValidateGraph(definition, types), "graph.duplicate_connection");
  definition = Example();
  definition.nodes[0].ports.push_back({Id("out.second"), "Second",
                                       PortKind::Data, PortDirection::Output,
                                       types[1]});
  definition.connections.push_back(
      {Id("edge.second"), Id("out.second"), Id("in.data")});
  ErrorIs(ValidateGraph(definition, types), "graph.data_input_connected");
  definition = Example();
  definition.nodes.resize(kMaxNodes + 1U);
  ErrorIs(ValidateGraph(definition, types), "graph.limit_exceeded");
  definition = Example();
  definition.name.resize(kMaxLabelBytes + 1U);
  ErrorIs(ValidateGraph(definition, types), "graph.limit_exceeded");
}
} // namespace
int main() {
  try {
    ValidDefinitions();
    InvalidDefinitions();
    return 0;
  } catch (...) {
    std::fputs("Graph Core tests failed\n", stderr);
    return 1;
  }
}
