#pragma once
#include <limits>

ed::PlayOptions StabilizationBindings(gc::DomainOperation invoke) {
  ed::PlayOptions options;
  options.backend = std::make_unique<Backend>();
  options.bindings = [invoke = std::move(invoke)](ed::PlayContext &, AssetId) {
    auto values = Bindings(invoke);
    return std::vector<gc::NodeBinding>{values.begin(), values.end()};
  };
  return options;
}
void CheckNativeFailure(
    const std::shared_ptr<const ed::PlayActivation> &handle) {
  const auto report = handle->Report();
  if (!report || !report->diagnostic)
    throw std::runtime_error("native diagnostic missing from Editor handle");
  Check(handle->IsFinished() &&
            report->status == graph::ExecutionStatus::Failed,
        "Editor activation exposes Failed");
  Check(report->diagnostic->error.HasCode("review.native_failure") &&
            report->diagnostic->asset == Definition().asset_id &&
            report->diagnostic->node == Id("before"),
        "native diagnostic retains original error/asset/node");
}
void StabilizationFailures() {
  Fixture fixture;
  ComponentsFixture types;
  auto project = Take(ed::EditorProject::Create(
      "Temp/editor-tests/failures.epidemic", Project(fixture, types),
      Schema(fixture, types)));
  auto token = std::make_shared<int>(0);
  const std::weak_ptr<int> lifetime = token;
  auto session = Take(project->Play(StabilizationBindings(
      [token](const gc::OperationContext &, std::int64_t) {
        ++*token;
        return foundation::Result<std::int64_t>::Failure(
            foundation::Error::Create("review.native_failure",
                                      "deliberate native error"));
      })));
  token.reset();
  Check(Take(session->Send(SceneNodeId{42}, {Id("hit"), 1})) == 1,
        "ordinary Send accepts activation");
  auto handles = session->Activations();
  Check(handles.size() == 1, "Send exposes one activation through Editor API");
  const auto first = handles[0];
  Check(!first->IsFinished() && !first->Report(),
        "queued activation has no execution report");
  Take(
      session->Tick()); // Simulation success is independent from Graph failure.
  CheckNativeFailure(first);
  const auto original = first->Report();
  if (!original || !original->diagnostic)
    throw std::runtime_error("diagnostic missing");
  for (std::size_t i = 0;
       i < ed::PlaySession::kCompletedActivationRetention + 5; ++i) {
    Take(session->Send(SceneNodeId{42}, {Id("hit"), 1}));
    Take(session->Tick());
  }
  handles = session->Activations();
  Check(handles.size() == ed::PlaySession::kCompletedActivationRetention &&
            handles.front()->Id() > first->Id(),
        "session retains only the last bounded completed reports");
  CheckNativeFailure(first); // An evicted handle is owned by its caller.
  Take(session->Stop());
  Take(session->Stop());
  CheckNativeFailure(first);
  const auto after = first->Report();
  if (!after || !after->diagnostic)
    throw std::runtime_error("terminal diagnostic missing after Stop");
  Check(after->diagnostic->instruction == original->diagnostic->instruction,
        "Stop preserves instruction diagnostics for Failed activation");
  session.reset();
  Check(lifetime.expired(),
        "Editor report handles never retain native callables/runtime context");
  CheckNativeFailure(first);
}
struct StabilizationValue {
  double number = 0;
};
void StabilizationAuthoring() {
  Fixture fixture;
  comp::ComponentRegistry registry;
  const auto type =
      Take(registry.Register<StabilizationValue>("stabilization.value"));
  const auto number = Take(
      registry.RegisterProperty(type, "number", &StabilizationValue::number));
  registry.Freeze();
  const auto asset = AssetId::FromString("stabilization.prefab"),
             scene_id = AssetId::FromString("stabilization.scene");
  const auto parameter = Id("initial");
  comp::PrefabDefinition prefab{asset.value,
                                {{parameter, 1.0}},
                                {Id("root"),
                                 fixture.archetype,
                                 {{type, {}, {{number, 1.0, parameter}}, {}}},
                                 {},
                                 {}}};
  gc::SceneCompositionDefinition scene;
  scene.asset_id = scene_id;
  SceneNodeSnapshot node;
  node.id = SceneNodeId{42};
  scene.scene.nodes.push_back(node);
  scene.placements.push_back({node.id, asset, {}});
  const auto directory = std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY};
  std::filesystem::create_directories(directory);
  const ed::ProjectSchema schema{fixture.entities, registry, {}};
  auto definition = ed::ProjectDefinition{
      Id("stabilization.project"),
      scene_id,
      {{asset, "Prefab", 0, prefab}, {scene_id, "Scene", 0, scene}}};
  auto project = Take(ed::EditorProject::Create(
      directory / "authoring.epidemic", definition, schema));
  Take(project->Save());
  const auto reject = [&](std::vector<ed::Command> commands) {
    const auto before = project->Snapshot();
    const auto revision = project->Revision();
    const auto undo = project->CanUndo(), redo = project->CanRedo(),
               dirty = project->IsDirty(), doc_dirty = project->IsDirty(asset);
    const auto changed = project->Execute({revision, std::move(commands)});
    Check(!changed,
          "non-finite values must be rejected before authoring publication");
    Check(changed.GetError().HasCode("editor.invalid_archive"),
          "authoring uses the actual persistence policy");
    Check(project->Snapshot() == before && project->Revision() == revision &&
              project->IsDirty() == dirty &&
              project->IsDirty(asset) == doc_dirty &&
              project->CanUndo() == undo && project->CanRedo() == redo,
          "rejection preserves snapshot/revision/history/dirty/save points");
  };
  const double values[]{std::numeric_limits<double>::quiet_NaN(),
                        std::numeric_limits<double>::infinity(),
                        -std::numeric_limits<double>::infinity()};
  for (const auto invalid : values) {
    const comp::PropertyAssignment runtime_value{number, invalid};
    Check(registry.Validate(type, std::span{&runtime_value, std::size_t{1}})
              .HasValue(),
          "runtime value semantics remain unchanged");
    reject(
        {ed::SetProperty{asset, Id("root"), type, {}, {number, invalid, {}}}});
    reject(
        {ed::RenameDocument{asset, "partial"},
         ed::SetProperty{asset, Id("root"), type, {}, {number, invalid, {}}}});
    reject({ed::SetComponent{
        asset, Id("root"), {type, {}, {{number, invalid, {}}}, {}}}});
    reject({ed::AddEntity{asset,
                          Id("root"),
                          {Id("child"),
                           fixture.archetype,
                           {{type, {}, {{number, invalid, {}}}, {}}},
                           {},
                           {}}}});
    auto malformed = prefab;
    malformed.id = Id("malformed.prefab");
    malformed.parameters[0].value = invalid;
    reject({ed::CreateDocument{
        {AssetId{malformed.id}, "Malformed", 0, malformed}}});
    comp::PrefabOverrides overrides;
    overrides.parameters.push_back({parameter, invalid});
    reject({ed::PlacePrefab{scene_id, {node.id, asset, overrides}}});
    overrides = {};
    overrides.components.push_back({Id("root"), type, {}, {number, invalid}});
    reject({ed::PlacePrefab{scene_id, {node.id, asset, overrides}}});
    auto invalid_definition = definition;
    std::get<comp::PrefabDefinition>(invalid_definition.documents[0].data)
        .parameters[0]
        .value = invalid;
    Check(!ed::EditorProject::Create(directory / "invalid.epidemic",
                                     invalid_definition, schema),
          "Create rejects non-persistable initial parameters");
  }
  Take(project->Execute(
      {0, {ed::SetProperty{asset, Id("root"), type, {}, {number, 2.5, {}}}}}));
  Take(project->Save());
  Take(project->Undo());
  Check(project->CanRedo() && project->IsDirty(),
        "saved valid edit has a redo branch");
  reject(
      {ed::RenameDocument{asset, "partial"},
       ed::SetProperty{asset, Id("root"), type, {}, {number, values[0], {}}}});
  Take(project->Redo());
  Check(!project->IsDirty(), "redo restores save point after rejected edit");
  Take(project->Save());
  auto reopened =
      Take(ed::EditorProject::Open(directory / "authoring.epidemic", schema));
  const auto &restored =
      std::get<comp::PrefabDefinition>(reopened->Snapshot()->Find(asset)->data);
  Check(std::get<double>(restored.root.components[0].properties[0].value) ==
                2.5 &&
            !reopened->IsDirty(),
        "ordinary double saves and reloads after rejection");
}
void StabilizationStop() {
  Fixture fixture;
  ComponentsFixture types;
  auto project = Take(ed::EditorProject::Create(
      "Temp/editor-tests/stop.epidemic", Project(fixture, types),
      Schema(fixture, types)));
  int calls = 0;
  const gc::DomainOperation invoke = [&](const gc::OperationContext &,
                                         std::int64_t value) {
    ++calls;
    if (value < 0)
      return foundation::Result<std::int64_t>::Failure(
          foundation::Error::Create("review.native_failure",
                                    "deliberate native error"));
    return foundation::Result<std::int64_t>::Success(value);
  };
  auto session = Take(project->Play(StabilizationBindings(invoke)));
  bool reenter = false, attempted = false, rejected = false;
  auto world = Take(comp::GraphScope::ForSystem(
      comp::GraphScopeKind::World,
      fw::GameplayObjectId::FromString("world.stabilization"), Id("wave"), [&] {
        if (reenter && !attempted) {
          attempted = true;
          rejected = !session->Stop();
        }
        return true;
      }));
  auto scene = Take(comp::GraphScope::ForSystem(
      comp::GraphScopeKind::Scene,
      fw::GameplayObjectId::FromString("scene.stabilization"), Id("director")));
  const comp::GraphBehaviorDefinition behavior{Id("system.behavior"),
                                               Definition().asset_id.value,
                                               {{Id("hit"), Id("event.in")}},
                                               {}};
  const auto world_handle =
      Take(session->Context()->behaviors.Attach(world, behavior));
  const auto scene_handle =
      Take(session->Context()->behaviors.Attach(scene, behavior));
  const auto entity_handle =
      session->Instance(SceneNodeId{42})->Objects()[0].behaviors[0].handle;
  std::vector<std::shared_ptr<comp::IGraphActivation>> waiting;
  for (const auto handle : {world_handle, scene_handle, entity_handle})
    waiting.push_back(
        Take(session->Context()->behaviors.Send(handle, {Id("hit"), 1})));
  const auto failed =
      Take(session->Context()->behaviors.Send(world_handle, {Id("hit"), -1}));
  Take(session->Tick());
  for (const auto &activation : waiting)
    Check(Status(activation) == graph::ExecutionStatus::Waiting,
          "Entity/Scene/World activations wait independently");
  Check(Status(failed) == graph::ExecutionStatus::Failed,
        "failed activation precedes shutdown");
  auto editor_handles = session->Activations();
  Check(editor_handles.size() == 4,
        "Editor observes Context-started activations at Tick");
  waiting.push_back(Take(session->Context()->behaviors.Send(
      world_handle, {Id("hit"), 1}))); // Still queued at Stop.
  reenter = true;
  const auto before = calls;
  Take(session->Stop());
  Check(attempted && rejected,
        "Stop rejects reentry from lifetime predicates before teardown");
  for (const auto &activation : waiting)
    Check(activation->IsFinished() &&
              Status(activation) == graph::ExecutionStatus::Cancelled,
          "Stop publishes terminal cancellation without external Tick");
  Check(Status(failed) == graph::ExecutionStatus::Failed,
        "Stop never replaces a Failed terminal report");
  for (const auto &handle : editor_handles)
    Check(handle->IsFinished(),
          "previous Editor handles finalize before teardown");
  Check(
      calls == before && !session->IsPlaying() && world->IsAlive() &&
          scene->IsAlive(),
      "cleanup runs no native effects and leaves external parent scopes alive");
  Take(session->Stop());
  session.reset();
  for (const auto &handle : editor_handles)
    Check(handle->IsFinished() && handle->Report().has_value(),
          "Editor reports survive session destruction");
  auto owned = Take(project->Play(StabilizationBindings(invoke)));
  auto sibling = Take(project->Play(StabilizationBindings(invoke)));
  Take(owned->Send(SceneNodeId{42}, {Id("hit"), 1}));
  Take(owned->Tick());
  Take(sibling->Send(SceneNodeId{42}, {Id("hit"), 1}));
  Take(sibling->Tick());
  auto entity_reports = owned->Activations();
  const auto entity = owned->Instance(SceneNodeId{42})->Objects()[0].entity;
  Take(owned->Context()->entities.RequestDestroy(
      entity, fw::entities::EntityDestroyReason::Destroyed));
  Take(owned->Tick());
  const auto cancelled = entity_reports[0]->Report();
  Check(cancelled && cancelled->status == graph::ExecutionStatus::Cancelled &&
            entity_reports[0]->IsFinished(),
        "Entity destruction finalizes its activations");
  Take(owned->Stop());
  Check(sibling->IsPlaying() && !sibling->Activations()[0]->IsFinished(),
        "Stop of one session never cancels a sibling session");
  const auto sibling_handle = sibling->Activations()[0];
  sibling.reset();
  const auto destroyed_report = sibling_handle->Report();
  Check(sibling_handle->IsFinished() && destroyed_report &&
            destroyed_report->status == graph::ExecutionStatus::Cancelled,
        "destructor finalizes Editor handles without external Tick");
}
void StabilizationDependencies() {
  Fixture fixture;
  ComponentsFixture types;
  auto definition = Project(fixture, types);
  const auto empty_id = AssetId::FromString("draft.empty"),
             draft_id = AssetId::FromString("draft.unconnected"),
             dynamic_id = AssetId::FromString("dynamic.graph");
  definition.documents.push_back(
      {empty_id, "Empty draft", 0,
       graph::GraphDefinition{empty_id, "Draft", {}, {}}});
  auto draft = Definition();
  draft.asset_id = draft_id;
  draft.connections.clear();
  definition.documents.push_back({draft_id, "Unconnected draft", 0, draft});
  auto dynamic = Definition();
  dynamic.asset_id = dynamic_id;
  definition.documents.push_back({dynamic_id, "Dynamic graph", 0, dynamic});
  auto draft_prefab =
      std::get<comp::PrefabDefinition>(definition.documents[1].data);
  draft_prefab.id = Id("draft.prefab");
  draft_prefab.root.behaviors[0].graph = draft_id.value;
  definition.documents.push_back(
      {AssetId{draft_prefab.id}, "Draft prefab", 0, draft_prefab});
  auto dynamic_prefab = draft_prefab;
  dynamic_prefab.id = Id("dynamic.prefab");
  dynamic_prefab.root.behaviors[0].graph = dynamic_id.value;
  definition.documents.push_back(
      {AssetId{dynamic_prefab.id}, "Dynamic prefab", 0, dynamic_prefab});
  auto project =
      Take(ed::EditorProject::Create("Temp/editor-tests/dependencies.epidemic",
                                     definition, Schema(fixture, types)));
  auto session = Take(project->Play(Options(types)));
  Take(session->Send(SceneNodeId{42}, {Id("hit"), 1}));
  Take(session->Tick());
  Check(HealthOf(*session, types) == 99,
        "unused empty/unconnected graphs do not block the entry Scene");
  const auto spawned =
      Take(session->Context()->prefabs.Spawn(dynamic_prefab.id));
  const auto activation = Take(session->Context()->behaviors.Send(
      spawned->Objects()[0].behaviors[0].handle, {Id("hit"), 1}));
  Take(session->Tick());
  Check(Status(activation) == graph::ExecutionStatus::Waiting,
        "dynamically spawned unplaced Prefab binds its deferred graph");
  Check(!session->Context()->prefabs.Spawn(draft_prefab.id),
        "a deferred invalid graph fails when actually used");
  Take(session->Stop());
  Check(Status(activation) == graph::ExecutionStatus::Cancelled,
        "dynamic activation participates in Stop");
  auto broken = Project(fixture, types);
  std::get<graph::GraphDefinition>(broken.documents[2].data)
      .connections.clear();
  auto invalid =
      Take(ed::EditorProject::Create("Temp/editor-tests/used-invalid.epidemic",
                                     broken, Schema(fixture, types)));
  Check(!invalid->Play(Options(types)),
        "an invalid graph in the entry dependency cone blocks Play");
}
