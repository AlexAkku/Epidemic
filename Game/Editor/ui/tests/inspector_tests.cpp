#include "../../example/demo_game.h"
#include "Epidemic/Game/Editor/editor_inspector.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <source_location>

using namespace epidemic;
namespace ed = game::editor;
namespace comp = gameplay::composition;
namespace ex = ed::example;
namespace {
struct UserSettings {
  std::int64_t lives = 5;
  double strength = 2.5;
  bool enabled = true;
  std::string caption = "user";
};
struct Other {};
bool ValidLives(const std::int64_t &value) {
  return value >= 0 && value <= 100;
}
bool ValidStrength(const double &value) {
  return std::isfinite(value) && value >= 0 && value <= 1000;
}
void Check(bool value,
           const std::source_location at = std::source_location::current()) {
  if (!value)
    throw std::runtime_error("Inspector invariant at line " +
                             std::to_string(at.line()));
}
comp::PropertyValue Value(const ed::InspectorEntry &entry) {
  if (!entry.value)
    throw std::runtime_error("Missing Inspector value: " + entry.name);
  return *entry.value;
}
} // namespace
void InspectorCases() {
  gameplay::entities::EntityService entities;
  comp::ComponentRegistry registry;
  int factories = 0;
  gameplay::entities::EntityArchetypeDefinition archetype;
  archetype.canonical_name = "studio.unit";
  const auto archetype_id = ex::Take(entities.RegisterArchetype(archetype));
  entities.Freeze();
  const auto type =
      ex::Take(registry.Register<UserSettings>("studio.user.settings", [&] {
        ++factories;
        return std::make_shared<UserSettings>();
      }));
  const auto lives = ex::Take(registry.RegisterProperty(
      type, "lives", &UserSettings::lives, &ValidLives));
  const auto strength = ex::Take(registry.RegisterProperty(
      type, "strength", &UserSettings::strength, &ValidStrength));
  const auto enabled = ex::Take(
      registry.RegisterProperty(type, "enabled", &UserSettings::enabled));
  const auto caption = ex::Take(
      registry.RegisterProperty(type, "caption", &UserSettings::caption));
  const auto other = ex::Take(registry.Register<Other>("studio.other"));
  registry.Freeze();
  const ed::ProjectSchema schema{entities, registry, {}};
  ed::EditorWorkspace workspace{
      schema, [] {
        ed::PlayOptions options;
        options.backend = std::make_unique<ex::Backend>();
        options.bindings = [](ed::PlayContext &, runtime::AssetId) {
          return std::vector<game::composition::NodeBinding>{};
        };
        return options;
      }};
  const runtime::AssetId prefab{ex::Id("inspector.prefab")},
      scene{ex::Id("inspector.scene")};
  const auto node = ex::Id("user.node"), parameter = ex::Id("power");
  const comp::PrefabComponent recipe{type,
                                     {},
                                     {{lives, std::int64_t{5}, {}},
                                      {strength, 2.5, {}},
                                      {enabled, true, {}},
                                      {caption, std::string{"user"}, {}}},
                                     {}};
  comp::PrefabDefinition definition{
      prefab.value, {{parameter, 4.5}}, {node, archetype_id, {recipe}, {}, {}}};
  game::composition::SceneCompositionDefinition scene_definition;
  scene_definition.asset_id = scene;
  runtime::SceneNodeSnapshot placement;
  placement.id = runtime::SceneNodeId{42};
  scene_definition.scene.nodes.push_back(placement);
  scene_definition.placements.push_back({placement.id, prefab, {}});
  const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY
                                   "/inspector.epidemic"};
  ex::Take(workspace.New(file, {ex::Id("inspector.project"),
                                scene,
                                {{scene, "Scene", 0, scene_definition},
                                 {prefab, "User", 0, definition}}}));
  ex::Take(workspace.Save());
  const ed::UiSelection selection{
      prefab, ed::UiElement::PrefabComponent, node.Raw(), type, {}};
  Check(workspace.Select(selection));
  const auto entries = [&](const ed::UiSelection &selected) {
    return ed::InspectComponents(*workspace.Project()->Snapshot(), selected,
                                 registry, workspace.Session());
  };
  const auto property = [&](comp::PropertyId id) {
    for (const auto &row : entries(selection))
      if (row.property == id)
        return row;
    throw std::runtime_error("Missing schema field");
  };
  Check(entries(selection).size() == 5 &&
        factories == 0); // Four fields plus the definition parameter.
  const auto apply = [&](comp::PropertyId id, std::string_view text) {
    ex::Take(
        workspace.Execute({ex::Take(ed::InspectorCommand(property(id), text))},
                          workspace.Project()->Revision()));
  };
  apply(lives, "42");
  apply(strength, "3.75");
  apply(enabled, "false");
  apply(caption, "  Unicode: Привет  ");
  Check(std::get<std::int64_t>(Value(property(lives))) == 42 &&
        std::get<double>(Value(property(strength))) == 3.75);
  Check(!std::get<bool>(Value(property(enabled))) &&
        std::get<std::string>(Value(property(caption))) ==
            "  Unicode: Привет  ");
  const auto checkpoint = workspace.Project()->Snapshot();
  const auto epoch = workspace.Project()->Revision();
  for (const auto text : {"nan", "inf", "-inf", "1e999"})
    Check(!ed::InspectorCommand(property(strength), text));
  Check(!ed::InspectorCommand(property(lives), "9223372036854775808"));
  Check(!ed::InspectorCommand(property(enabled), "1"));
  const auto invalid = ex::Take(ed::InspectorCommand(property(lives), "-1"));
  Check(!workspace.Execute(
      {ex::Take(ed::InspectorCommand(property(caption), "must roll back")),
       invalid},
      epoch));
  Check(workspace.Project()->Snapshot() == checkpoint &&
        workspace.Project()->Revision() == epoch && factories == 0);
  ex::Take(workspace.Save());
  apply(lives, "43");
  ex::Take(workspace.Undo());
  Check(!workspace.Project()->IsDirty());
  ex::Take(workspace.Redo());
  ex::Take(workspace.Save());
  ex::Take(workspace.Open(file));
  Check(workspace.Select(selection));
  Check(std::get<std::int64_t>(Value(property(lives))) == 43 && factories == 0);
  ex::Take(workspace.Execute(
      {ed::SetProperty{prefab, node, type, {}, {strength, 999., parameter}}},
      workspace.Project()->Revision()));
  Check(std::get<double>(Value(property(strength))) == 4.5 &&
        !property(strength).editable);
  Check(!ed::InspectorCommand(property(strength), "6"));
  auto placed = scene_definition.placements[0];
  placed.overrides.parameters.push_back({parameter, 7.5});
  ex::Take(workspace.Execute({ed::PlacePrefab{scene, placed}},
                             workspace.Project()->Revision()));
  const ed::UiSelection scene_selection{scene, ed::UiElement::SceneNode, 42};
  const auto effective = [&] {
    for (const auto &row : entries(scene_selection))
      if (row.property == strength && row.source != "runtime snapshot")
        return row;
    throw std::runtime_error("Missing effective field");
  };
  Check(std::get<double>(Value(effective())) == 7.5 &&
        effective().source.starts_with("parameter override"));
  placed.overrides.components.push_back({node, type, {}, {strength, 8.5}});
  ex::Take(workspace.Execute({ed::PlacePrefab{scene, placed}},
                             workspace.Project()->Revision()));
  Check(std::get<double>(Value(effective())) == 8.5 &&
        effective().source == "instance override" && !effective().editable);
  ex::Take(workspace.Play());
  Check(factories == 1);
  Check(ex::Take(workspace.Session()->ReadComponentProperty(
            placement.id, node, type, {}, strength)) ==
        comp::PropertyValue{8.5});
  const auto detached = ex::Take(workspace.Session()->ReadComponentProperty(
      placement.id, node, type, {}, caption));
  const auto runtime_rows = entries(scene_selection);
  Check(std::any_of(runtime_rows.begin(), runtime_rows.end(),
                    [](const auto &row) {
                      return row.source == "runtime snapshot" && !row.editable;
                    }));
  apply(lives, "50");
  Check(ex::Take(workspace.Session()->ReadComponentProperty(placement.id, node,
                                                            type, {}, lives)) ==
        comp::PropertyValue{std::int64_t{43}});
  ex::Take(workspace.Stop());
  Check(!workspace.Session()->ReadComponentProperty(placement.id, node, type,
                                                    {}, lives));
  Check(std::get<std::string>(detached) == "  Unicode: Привет  ");
  // Add/replace uses one atomic command transaction; collisions cannot
  // overwrite slots.
  const ed::UiSelection entity_selection{prefab, ed::UiElement::PrefabNode,
                                         node.Raw()};
  Check(!ed::ComponentCommands(*workspace.Project()->Snapshot(),
                               entity_selection, recipe));
  const auto referenced = workspace.Project()->Snapshot();
  auto commands = ex::Take(
      ed::ComponentCommands(*referenced, selection, {other, {}, {}, {}}, true));
  Check(!workspace.Execute(commands, workspace.Project()->Revision()) &&
        workspace.Project()->Snapshot() == referenced);
  // Existing instance overrides must not become dangling component references.
  placed.overrides = {};
  ex::Take(workspace.Execute({ed::PlacePrefab{scene, placed}},
                             workspace.Project()->Revision()));
  const auto before_replace = workspace.Project()->Snapshot();
  commands = ex::Take(ed::ComponentCommands(*before_replace, selection,
                                            {other, {}, {}, {}}, true));
  Check(commands.size() == 2);
  ex::Take(workspace.Execute(commands, workspace.Project()->Revision()));
  ex::Take(workspace.Undo());
  Check(workspace.Project()->Snapshot() == before_replace);
  auto unknown = recipe;
  unknown.type = gameplay::TypeId::FromString("unregistered");
  const auto rejected = ex::Take(
      ed::ComponentCommands(*before_replace, selection, unknown, true));
  Check(!workspace.Execute(rejected, workspace.Project()->Revision()) &&
        workspace.Project()->Snapshot() == before_replace);
  // Omitted defaults stay unknown; no component factory is called to invent a
  // value.
  auto omitted = recipe;
  omitted.properties.erase(omitted.properties.begin() + 1);
  ex::Take(workspace.Execute({ed::SetComponent{prefab, node, omitted}},
                             workspace.Project()->Revision()));
  Check(!property(strength).value && property(strength).editable &&
        property(strength).source == "factory default unavailable");
  const auto count = factories;
  apply(strength, "2.25");
  Check(factories == count);
  Check(!ed::InspectorCommand(property(caption), "line one\nline two"));
  ex::Take(workspace.Execute(
      {ed::SetProperty{prefab,
                       node,
                       type,
                       {},
                       {caption, std::string{"line one\nline two"}, {}}}},
      workspace.Project()->Revision()));
  Check(!property(caption).editable &&
        ed::InspectorText(property(caption)).find("\\n") != std::string::npos);
}

void PrefabCases() {
  ex::Schema schema;
  ed::EditorWorkspace workspace{schema.View(),
                                [&] { return ex::Options(schema); }};
  const runtime::AssetId prefab =
                             runtime::AssetId::FromString("example.prefab"),
                         scene = runtime::AssetId::FromString("example.scene");
  const auto node = ex::Id("enemy"), parameter = ex::Id("m6.health");
  const ed::UiSelection root{
      prefab, ed::UiElement::PrefabComponent, node.Raw(), schema.health, {}},
      a{scene, ed::UiElement::SceneNode, 42},
      b{scene, ed::UiElement::SceneNode, 43};
  ex::Take(workspace.New(EPIDEMIC_EDITOR_TEST_DIRECTORY
                         "/prefab-overrides.epidemic",
                         ex::Definition(schema)));
  const auto execute = [&](std::vector<ed::Command> commands) {
    ex::Take(workspace.Execute(std::move(commands),
                               workspace.Project()->Revision()));
  };
  const auto field = [&](const ed::UiSelection &selection,
                         comp::PropertyId property,
                         ed::InspectorTarget target) {
    for (const auto &entry :
         ed::InspectComponents(*workspace.Project()->Snapshot(), selection,
                               schema.components, nullptr, true))
      if (entry.property == property && entry.target == target)
        return entry;
    throw std::runtime_error("Missing Prefab Inspector field");
  };
  auto health = field(root, schema.points, ed::InspectorTarget::Property);
  const auto unbound = workspace.Project()->Snapshot();
  execute(ex::Take(ed::InspectorBinding(health, parameter)));
  health = field(root, schema.points, ed::InspectorTarget::Property);
  Check(health.binding == parameter && !health.editable &&
        Value(health) == comp::PropertyValue{std::int64_t{100}});
  ex::Take(workspace.Undo());
  Check(workspace.Project()->Snapshot() == unbound);
  ex::Take(workspace.Redo());
  runtime::SceneNodeSnapshot second;
  second.id = runtime::SceneNodeId{43};
  second.local_transform.position = {5, 0, 0};
  execute({ed::AddSceneNode{scene, second},
           ed::PlacePrefab{scene, {second.id, prefab, {}}}});
  const auto edit = [&](const ed::UiSelection &selected,
                        comp::PropertyId property, ed::InspectorTarget target,
                        std::string_view text) {
    execute({ex::Take(
        ed::InspectorCommand(field(selected, property, target), text))});
  };
  edit(a, parameter, ed::InspectorTarget::ParameterOverride, "75");
  edit(a, schema.points, ed::InspectorTarget::ComponentOverride, "60");
  edit(a, schema.speed, ed::InspectorTarget::ComponentOverride, "2.5");
  edit(b, schema.points, ed::InspectorTarget::ComponentOverride, "80");
  edit(b, schema.speed, ed::InspectorTarget::ComponentOverride, "5");
  Check(
      Value(field(a, schema.points, ed::InspectorTarget::ComponentOverride)) ==
      comp::PropertyValue{std::int64_t{60}});
  const auto with_overrides = workspace.Project()->Snapshot();
  execute({ex::Take(ed::InspectorResetCommand(
      field(a, schema.points, ed::InspectorTarget::ComponentOverride)))});
  Check(
      Value(field(a, schema.points, ed::InspectorTarget::ComponentOverride)) ==
      comp::PropertyValue{std::int64_t{75}});
  Check(
      Value(field(b, schema.points, ed::InspectorTarget::ComponentOverride)) ==
      comp::PropertyValue{std::int64_t{80}});
  Check(Value(field(a, schema.speed, ed::InspectorTarget::ComponentOverride)) ==
        comp::PropertyValue{2.5});
  ex::Take(workspace.Undo());
  Check(workspace.Project()->Snapshot() == with_overrides);
  ex::Take(workspace.Redo());
  execute({ex::Take(ed::InspectorResetCommand(
      field(a, parameter, ed::InspectorTarget::ParameterOverride)))});
  Check(!ed::InspectorResetCommand(
      field(a, schema.points, ed::InspectorTarget::ComponentOverride)));
  edit(root, parameter, ed::InspectorTarget::Parameter, "90");
  Check(
      Value(field(a, schema.points, ed::InspectorTarget::ComponentOverride)) ==
      comp::PropertyValue{std::int64_t{90}});
  Check(
      Value(field(b, schema.points, ed::InspectorTarget::ComponentOverride)) ==
      comp::PropertyValue{std::int64_t{80}});
  ex::Take(workspace.Save());
  const auto saved = workspace.Project()->Snapshot();
  const auto revision = workspace.Project()->Revision();
  const auto invalid = ex::Take(ed::InspectorCommand(
      field(a, schema.points, ed::InspectorTarget::ComponentOverride), "-1"));
  Check(!workspace.Execute(
      {ed::RenameDocument{prefab, "must roll back"}, invalid}, revision));
  Check(workspace.Project()->Snapshot() == saved &&
        workspace.Project()->Revision() == revision &&
        !workspace.Project()->IsDirty());
  Check(!workspace.Execute({ed::SetPrefabParameter{prefab, {parameter, 1.5}}},
                           revision)); // Stable parameter kind.
  Check(!workspace.Execute(
      {ed::SetPrefabParameter{
          prefab,
          {ex::Id("bad.parameter"), std::numeric_limits<double>::infinity()}}},
      revision));
  Check(!workspace.Execute(
      {ed::SetParameterOverride{scene,
                                runtime::SceneNodeId{42},
                                prefab,
                                {ex::Id("missing"), std::int64_t{2}}}},
      revision));
  Check(!workspace.Execute(
      {ed::SetComponentOverride{
          scene,
          runtime::SceneNodeId{999},
          prefab,
          {node, schema.health, {}, {schema.points, std::int64_t{2}}}}},
      revision));
  Check(!ed::InspectorCommand(
      field(a, schema.speed, ed::InspectorTarget::ComponentOverride), "inf"));
  execute({ed::RenameDocument{prefab, "Renamed enemy"},
           ed::SetSceneParent{scene, runtime::SceneNodeId{43},
                              runtime::SceneNodeId{42}}});
  Check(
      Value(field(b, schema.points, ed::InspectorTarget::ComponentOverride)) ==
      comp::PropertyValue{std::int64_t{80}});
  const auto linked = workspace.Project()->Snapshot();
  execute({ed::DeleteSceneNode{scene, runtime::SceneNodeId{43}}});
  ex::Take(workspace.Undo());
  Check(workspace.Project()->Snapshot() == linked);
  Check(!workspace.Execute({ed::DeleteDocument{prefab}},
                           workspace.Project()->Revision()));
  // A cached override command cannot target a replacement Prefab at the same
  // placement ID.
  const auto stale = ex::Take(ed::InspectorCommand(
      field(a, schema.speed, ed::InspectorTarget::ComponentOverride), "7"));
  auto other = ex::Enemy(schema);
  const runtime::AssetId other_id = runtime::AssetId::FromString("m6.other");
  execute(
      {ed::CreateDocument{{other_id, "Other", 0,
                           comp::PrefabDefinition{other_id.value, {}, other}}},
       ed::PlacePrefab{scene, {runtime::SceneNodeId{42}, other_id, {}}}});
  const auto reassigned = workspace.Project()->Snapshot();
  Check(!workspace.Execute({stale}, workspace.Project()->Revision()) &&
        workspace.Project()->Snapshot() == reassigned);
  ex::Take(workspace.Undo());
  Check(workspace.Project()->Snapshot() == linked);
  ex::Take(workspace.Save());
  const std::filesystem::path relocated{EPIDEMIC_EDITOR_TEST_DIRECTORY
                                        "/prefab-relocated.epidemic"};
  std::filesystem::copy_file(
      EPIDEMIC_EDITOR_TEST_DIRECTORY "/prefab-overrides.epidemic", relocated,
      std::filesystem::copy_options::overwrite_existing);
  ex::Take(workspace.Open(
      relocated)); // Asset IDs remain stable after moving the project file.
  ex::Take(workspace.Play());
  Check(workspace.Session()->Instance(runtime::SceneNodeId{42})->Definition() ==
        workspace.Session()->Instance(runtime::SceneNodeId{43})->Definition());
  const auto runtime_value = [&](std::uint64_t placement, gameplay::TypeId type,
                                 comp::PropertyId property) {
    return ex::Take(workspace.Session()->ReadComponentProperty(
        runtime::SceneNodeId{placement}, node, type, {}, property));
  };
  Check(runtime_value(42, schema.health, schema.points) ==
        comp::PropertyValue{std::int64_t{90}});
  Check(runtime_value(43, schema.health, schema.points) ==
        comp::PropertyValue{std::int64_t{80}});
  Check(runtime_value(42, schema.movement, schema.speed) ==
            comp::PropertyValue{2.5} &&
        runtime_value(43, schema.movement, schema.speed) ==
            comp::PropertyValue{5.});
  ex::Take(workspace.Send(runtime::SceneNodeId{42}, {ex::Id("hit"), 1}));
  ex::Take(workspace.Tick({}));
  Check(runtime_value(42, schema.health, schema.points) ==
            comp::PropertyValue{std::int64_t{89}} &&
        runtime_value(43, schema.health, schema.points) ==
            comp::PropertyValue{std::int64_t{80}});
  ex::Take(workspace.Stop());
  Check(!workspace.Project()->IsDirty());
  health = field(root, schema.points, ed::InspectorTarget::Property);
  execute(ex::Take(ed::InspectorBinding(health)));
  Check(field(root, schema.points, ed::InspectorTarget::Property).editable);
}
