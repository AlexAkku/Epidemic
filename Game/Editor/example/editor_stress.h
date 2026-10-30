#pragma once
#include "editor_demo.h"

namespace epidemic::game::editor::example {
// Bounded initial test data, validated by EditorProject::Create/New. Published
// authoring edits still use Command API; this is not an alternate Scene model.
inline foundation::Result<ProjectDefinition>
StressDefinition(const std::filesystem::path &destination,
                 const ProjectSchema &schema, std::size_t count) {
  using Result = foundation::Result<ProjectDefinition>;
  if (count != 200 && count != 1000)
    return Result::Failure(foundation::Error::Create(
        "editor.stress_count",
        "Use 200 placements, or the separate 1000-placement diagnostic."));
  auto result = DemoDefinition(destination, schema);
  if (!result)
    return result;
  auto definition = std::move(result).Value();
  auto &scene = std::get<gc::SceneCompositionDefinition>(
      definition.documents.front().data);
  for (std::size_t i = 3; i < count; ++i) {
    runtime::SceneNodeSnapshot node;
    node.id = runtime::SceneNodeId{1000 + i};
    const auto columns = count == 200 ? 15u : 32u;
    const auto column = (i - 3) % columns, row = (i - 3) / columns;
    node.local_transform.position = {static_cast<float>(column) * 3 - 21,
                                     static_cast<float>(row) * 3 + 6, 0};
    comp::PrefabOverrides overrides;
    overrides.parameters.push_back(
        {Id("editor.demo.health"), static_cast<std::int64_t>(80 + i % 100)});
    overrides.components.push_back(
        {Id("enemy"),
         gameplay::TypeId::FromString("editor.example.movement"),
         {},
         {comp::PropertyId::FromString("label"),
          std::string{"Enemy "} + std::to_string(i)}});
    scene.scene.nodes.push_back(node);
    scene.placements.push_back({node.id,
                                runtime::AssetId::FromString("example.prefab"),
                                std::move(overrides)});
  }
  return Result::Success(std::move(definition));
}
} // namespace epidemic::game::editor::example
