#include "Epidemic/Runtime/Assets/asset_services.h"
#include "Epidemic/Runtime/Schema/native_schema.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Support/asset_element_reference.h"
#include "Epidemic/Runtime/Support/graph_serialization.h"

#include <array>
#include <cstdio>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime;
namespace {
struct NodeTag {};
struct EventPayload {};
using Id = foundation::StringId;
Id Key(std::string_view name) { return Id::FromString(name); }
void Check(bool value, const char *message) {
  if (!value) {
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
    throw std::runtime_error(message);
  }
}
template <typename T>
void ErrorIs(const foundation::Result<T> &result, std::string_view code) {
  Check(!result && result.GetError().HasCode(code),
        "unexpected graph serialization error");
}
graph::GraphDefinition Definition(const std::array<Id, 3> &types) {
  using graph::PortDirection;
  using graph::PortKind;
  graph::GraphDefinition definition{
      AssetId::FromString("identity.graph.1"), "Graph", {}, {}};
  definition.nodes = {{Key("node.1"),
                       types[0],
                       "Source",
                       {{Key("out.data"), "Data", PortKind::Data,
                         PortDirection::Output, types[1]},
                        {Key("out.control"),
                         "Control",
                         PortKind::Control,
                         PortDirection::Output,
                         {}},
                        {Key("out.event"), "Event", PortKind::Event,
                         PortDirection::Output, types[2]}}},
                      {Key("node.2"),
                       types[0],
                       "Target",
                       {{Key("in.data"), "Data", PortKind::Data,
                         PortDirection::Input, types[1]},
                        {Key("in.control"),
                         "Control",
                         PortKind::Control,
                         PortDirection::Input,
                         {}},
                        {Key("in.event"), "Event", PortKind::Event,
                         PortDirection::Input, types[2]}}}};
  definition.connections = {
      {Key("edge.data"), Key("out.data"), Key("in.data")},
      {Key("edge.control"), Key("out.control"), Key("in.control")},
      {Key("edge.event"), Key("out.event"), Key("in.event")}};
  return definition;
}
void RoundTrip(const std::array<Id, 3> &types) {
  GraphDefinitionSerializer serializer{{types.begin(), types.end()}};
  const auto original = Definition(types);
  const AssetElementReference reference{original.asset_id,
                                        original.nodes[0].id};
  SerializedDocument saved;
  {
    auto serialization = CreateSerializationServices().Value();
    saved = SerializeToDocument(serializer, original, *serialization.archives)
                .Value();
  }
  auto fresh = CreateSerializationServices().Value();
  auto reader = fresh.archives->CreateReader(saved).Value();
  graph::GraphDefinition loaded;
  Check(DeserializeObject(serializer, *reader, loaded).HasValue(),
        "graph reload");
  Check(loaded == original,
        "all graph definitions, types, ports and edges round-trip");
  loaded.name = "Renamed graph";
  loaded.nodes[0].name = "Renamed node";
  loaded.nodes[0].ports[0].name = "Renamed port";
  Check(loaded.asset_id == reference.asset_id &&
            loaded.nodes[0].id == reference.element_id &&
            loaded.connections == original.connections,
        "rename preserves element/connection identity");
  const auto renamed =
      SerializeToDocument(serializer, loaded, *fresh.archives).Value();
  auto renamed_reader = fresh.archives->CreateReader(renamed).Value();
  graph::GraphDefinition reloaded;
  Check(DeserializeObject(serializer, *renamed_reader, reloaded).HasValue() &&
            reloaded == loaded,
        "renamed reload");

  auto assets = CreateAssetServices().Value();
  AssetMetadata metadata;
  metadata.id = reloaded.asset_id;
  metadata.type = {Key("asset.graph")};
  metadata.state = AssetState::Validated;
  metadata.version = 1;
  metadata.location.path = "moved/renamed.graph";
  Check(assets.writer->RegisterAsset(metadata).HasValue(),
        "register relocated graph asset");
  const std::array elements{reloaded.nodes[0].id, reloaded.nodes[1].id};
  Check(ResolveAssetElementReference(reference, {reloaded.asset_id, elements},
                                     *assets.location_resolver)
                .Value()
                .path == "moved/renamed.graph",
        "existing authored reference resolves graph element");
}
void RejectedDocuments(const std::array<Id, 3> &types) {
  GraphDefinitionSerializer serializer{{types.begin(), types.end()}};
  auto serialization = CreateSerializationServices().Value();
  const auto original = Definition(types);
  auto invalid = original;
  invalid.connections[0].input_port = Key("missing.port");
  ErrorIs(SerializeToDocument(serializer, invalid, *serialization.archives),
          "graph.missing_port");
  ErrorIs(SerializeToDocument(serializer, AssetElementReference{},
                              *serialization.archives),
          "serialization.cpp_type_mismatch");

  // Build an archive independently: the hostile count must fail before reading
  // or allocating any NodeDefinition. Empty initialized array entries are valid
  // archives.
  auto writer = serialization.archives->CreateWriter();
  Check(writer->WriteUInt64("asset_id", original.asset_id.Raw()).HasValue(),
        "write asset");
  Check(writer->WriteString("name", "hostile count").HasValue(), "write name");
  Check(writer->BeginArray("nodes", graph::kMaxNodes + 1U).HasValue(),
        "write oversized nodes");
  for (std::size_t index = 0; index <= graph::kMaxNodes; ++index) {
    Check(writer->BeginArrayElement(index).HasValue(), "initialize node entry");
    Check(writer->EndArrayElement().HasValue(), "finish node entry");
  }
  Check(writer->EndArray().HasValue(), "close node array");
  const auto oversized =
      writer->Finalize(serializer.GetTypeId(), serializer.GetSchemaVersion())
          .Value();
  auto reader = serialization.archives->CreateReader(oversized).Value();
  auto target = original;
  ErrorIs(DeserializeObject(serializer, *reader, target),
          "graph.limit_exceeded");
  Check(target == original, "failed graph load preserves destination");

  auto missing = serialization.archives->CreateWriter();
  Check(missing->WriteUInt64("asset_id", original.asset_id.Raw()).HasValue(),
        "write partial graph");
  const auto incomplete =
      missing->Finalize(serializer.GetTypeId(), serializer.GetSchemaVersion())
          .Value();
  auto missing_reader =
      serialization.archives->CreateReader(incomplete).Value();
  Check(!DeserializeObject(serializer, *missing_reader, target) &&
            target == original,
        "missing fields are atomic");
  const auto valid =
      SerializeToDocument(serializer, original, *serialization.archives)
          .Value();
  auto valid_reader = serialization.archives->CreateReader(valid).Value();
  int wrong_target = 12;
  ErrorIs(DeserializeObject(serializer, *valid_reader, wrong_target),
          "serialization.cpp_type_mismatch");
  Check(wrong_target == 12, "wrong C++ target unchanged");
  GraphDefinitionSerializer unknown_types{{}};
  auto unknown_reader = serialization.archives->CreateReader(valid).Value();
  ErrorIs(DeserializeObject(unknown_types, *unknown_reader, target),
          "graph.unknown_type");
  Check(target == original, "unknown schema type does not mutate destination");
}
} // namespace
int main() {
  try {
    schema::NativeSchemaRegistry registry;
    const std::array types{
        registry.RegisterType<NodeTag>("type.graph.node").Value(),
        registry.RegisterType<std::int32_t>("type.graph.int").Value(),
        registry.RegisterType<EventPayload>("type.graph.event").Value()};
    for (const auto type : types)
      Check(registry.FindType(type) != nullptr,
            "native Schema owns graph types");
    RoundTrip(types);
    RejectedDocuments(types);
    return 0;
  } catch (...) {
    std::fputs("Graph serialization tests failed\n", stderr);
    return 1;
  }
}
