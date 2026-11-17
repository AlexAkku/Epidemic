#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/play_session.h"
#include "../../Composition/tests/user_components.h"
#include <fstream>
#include <iostream>
#include <array>
#include <limits>
#include <algorithm>
#include "../src/project_archive.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Support/graph_serialization.h"

using namespace composition_tests;
namespace ed = epidemic::game::editor;

ed::ProjectDefinition Project(const Fixture &f, const ComponentsFixture &types) {
    auto graph = Definition();
    comp::PrefabDefinition
        prefab{Id("editor.prefab"),
               {},
               {Id("enemy"),
                f.archetype,
                {{types.health, {}, {{types.points, std::int64_t{100}, {}}}, {}}},
                {{Id("behavior"), graph.asset_id.value, {{Id("hit"), Id("event.in")}}, {}}},
                {}}};

    gc::SceneCompositionDefinition scene;
    scene.asset_id = AssetId::FromString("editor.scene");
    SceneNodeSnapshot node;
    node.id = SceneNodeId{42};

    scene.scene.nodes.push_back(node);
    scene.placements.push_back({node.id, AssetId{prefab.id}, {}});
    return {Id("editor.project"),
            scene.asset_id,
            {{scene.asset_id, "Scene", 0, scene},
             {AssetId{prefab.id}, "Enemy", 0, prefab},
             {graph.asset_id, "Graph", 0, graph}}};

}

ed::ProjectSchema Schema(Fixture &f, const ComponentsFixture &types) {
    return {f.entities,
            types.registry,
            {{Id("event.type"), graph::Operation::EventEntry},
             {Id("call.type"), graph::Operation::NativeCall},
             {Id("wait.type"), graph::Operation::Wait}}};

}

void GraphLayoutMigration() {
    namespace rt = epidemic::runtime;
    Fixture fixture;
    ComponentsFixture components;
    const auto schema = Schema(fixture, components);
    auto graph = Definition();
    auto services = Take(rt::CreateSerializationServices());
    std::vector<graph::TypeId> types{graph::kI64Type};
    for (const auto &[type, operation] : schema.operations) {
        (void)operation;
        types.push_back(type);
    }
    rt::GraphDefinitionSerializer codec{types};
    const auto graph_bytes = Take(rt::EncodeDocument(Take(rt::SerializeToDocument(codec, graph, *services.archives))));
    const auto archive = [&](std::uint64_t layout_version, std::uint64_t authoring_version=0) {
        auto writer = services.archives->CreateWriter();
        Take(writer->WriteUInt64("id", Id("layout.project").Raw()));
        Take(writer->WriteUInt64("entry", 0));
        Take(writer->BeginArray("documents", 1));
        Take(writer->BeginArrayElement(0));
        Take(writer->WriteUInt64("id", graph.asset_id.Raw()));
        Take(writer->WriteString("name", "Graph"));
        Take(writer->WriteUInt64("kind", 2));
        Take(writer->BeginObject("data"));
        Take(writer->WriteBytes("archive", graph_bytes));
        Take(writer->EndObject());
        if (layout_version) {
            Take(writer->BeginObject("graph_layout"));
            Take(writer->WriteUInt64("version", layout_version));
            Take(writer->BeginArray("positions", 1));
            Take(writer->BeginArrayElement(0));
            Take(writer->WriteUInt64("node", graph.nodes.front().id.Raw()));
            Take(writer->WriteDouble("x", 12.5));
            Take(writer->WriteDouble("y", -24.25));
            Take(writer->EndArrayElement());
            Take(writer->EndArray());
            Take(writer->EndObject());
        }
        if(authoring_version){
            Take(writer->BeginObject("graph_authoring"));Take(writer->WriteUInt64("version",authoring_version));
            Take(writer->BeginArray("nodes",1));Take(writer->BeginArrayElement(0));
            Take(writer->WriteUInt64("node",graph.nodes.front().id.Raw()));Take(writer->WriteInt64("preview_default",42));
            Take(writer->WriteString("comment","Comment"));Take(writer->WriteString("group","Group"));
            Take(writer->WriteBool("bookmark",true));Take(writer->WriteBool("breakpoint",true));
            Take(writer->EndArrayElement());Take(writer->EndArray());Take(writer->EndObject());
        }
        Take(writer->EndArrayElement());
        Take(writer->EndArray());
        Take(writer->BeginArray("identities", 0));
        Take(writer->EndArray());
        return Take(rt::EncodeDocument(Take(writer->Finalize(Id("epidemic.editor.project"), {1, 0, 0}))));
    };
    auto old = Take(ed::detail::ReadProject(archive(0), schema));
    Check(old.definition.documents.front().graph_layout.empty() &&
          std::get<graph::GraphDefinition>(old.definition.documents.front().data) == graph,
          "v0 migration preserves native graph and implicit deterministic positions");
    auto migrated = Take(ed::detail::ReadProject(Take(ed::detail::WriteProject(old, schema)), schema));
    Check(migrated.definition.documents.front().graph_layout.empty(), "migration is stable on second read");
    auto current = Take(ed::detail::ReadProject(archive(1), schema));
    Check(current.definition.documents.front().graph_layout ==
          std::vector<ed::GraphNodePosition>{{graph.nodes.front().id.Raw(), 12.5, -24.25}},
          "v1 coordinates preserved");
    const auto future = ed::detail::ReadProject(archive(2), schema);
    Check(!future && future.GetError().HasCode("editor.invalid_archive"), "unknown layout version rejected");
    Check(old.definition.documents.front().graph_settings.empty(),"v0 authoring metadata migration");
    auto settings=Take(ed::detail::ReadProject(archive(1,1),schema));
    const auto persisted=Take(ed::detail::ReadProject(Take(ed::detail::WriteProject(settings,schema)),schema));
    Check(settings.definition.documents.front().graph_settings==persisted.definition.documents.front().graph_settings &&
          persisted.definition.documents.front().graph_settings.front().preview_default==42,"v1 authoring metadata roundtrip");
    const auto future_settings=ed::detail::ReadProject(archive(1,2),schema);
    Check(!future_settings&&future_settings.GetError().HasCode("editor.invalid_archive"),"unknown authoring version rejected");
}

void Documents() {
    Fixture f;
    ComponentsFixture types;
    auto definition = Project(f, types);
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/project.epidemic", definition,
                                                  Schema(f, types)));
    const auto snapshot = project->Snapshot();
    Check(snapshot->definition.documents.size() == 3 && snapshot->assets->IsSealed(),
          "existing Assets catalog backs document identity");
    Check(project->IsDirty() && project->IsDirty(definition.documents[0].id),
          "new documents are unsaved");
    const auto manifest = Take(snapshot->assets->BuildDependencyManifest(definition.entry_scene));
    (void)manifest;
    definition.documents.clear();
    Check(snapshot->definition.documents.size() == 3 && f.entities.AllEntities().empty(),
          "detached authoring snapshot never creates gameplay Entities");
    auto invalid = snapshot->definition;
    std::get<comp::PrefabDefinition>(invalid.documents[1].data).root.behaviors[0].graph =
        Id("missing");
    ErrorIs(ed::ValidateProject(invalid, Schema(f, types)), "editor.graph_reference");
    invalid = snapshot->definition;
    std::get<gc::SceneCompositionDefinition>(invalid.documents[0].data).scene.nodes[0].parent =
        SceneNodeId{999};

    ErrorIs(ed::ValidateProject(invalid, Schema(f, types)), "composition.scene_parent");
    auto repeated = snapshot->definition;
    auto &scene = std::get<gc::SceneCompositionDefinition>(repeated.documents[0].data);
    for (const auto id : {43u, 44u}) {
        SceneNodeSnapshot node;
        node.id = SceneNodeId{id};

        scene.scene.nodes.push_back(node);
        scene.placements.push_back({node.id, repeated.documents[1].id, {}});
    }

    auto &prefab = std::get<comp::PrefabDefinition>(repeated.documents[1].data);
    auto behavior = prefab.root.behaviors.front();
    behavior.id = Id("another.behavior");
    prefab.root.behaviors.push_back(behavior);
    const auto catalog = Take(ed::ValidateProject(repeated, Schema(f, types)));
    const auto scene_asset = catalog->FindById(scene.asset_id),
               prefab_asset = catalog->FindById(AssetId{prefab.id});
    if (!scene_asset || !prefab_asset)
        throw std::runtime_error("repeated asset references lost catalog identity");
    Check(scene_asset->dependencies.size() == 1 && prefab_asset->dependencies.size() == 1,
          "repeated Prefab placements and behaviors share a single asset dependency");
}

void DocumentDiscard() {
    Fixture f;
    ComponentsFixture types;
    const auto prefab = AssetId::FromString("editor.prefab");
    const auto graph_id = Definition().asset_id;
    const auto child = Id("discard.saved.child");
    auto project =
        Take(ed::EditorProject::Create(std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY} /
                                           "discard.epidemic",
                                       Project(f, types), Schema(f, types)));
    Take(project->Execute(
        {0, {ed::AddEntity{prefab, Id("enemy"), {child, f.archetype, {}, {}, {}}}}}));
    Take(project->Save());
    Take(project->Execute(
        {0,
         {ed::DeleteEntity{prefab, child}, ed::RenameDocument{graph_id, "Other dirty document"}}}));
    Take(project->Execute({project->Revision(), {ed::DiscardDocument{prefab}}}));
    Check(!project->IsDirty(prefab) && project->IsDirty(graph_id) && project->IsDirty(),
          "discard is scoped to one document");
    Check(std::get<comp::PrefabDefinition>(project->Snapshot()->Find(prefab)->data)
                  .root.children.size() == 1,
          "discard restores saved retired child identity");
    Take(project->Undo());
    Check(std::get<comp::PrefabDefinition>(project->Snapshot()->Find(prefab)->data)
              .root.children.empty(),
          "discard has one undo transaction");
    Take(project->Redo());
    const auto before = project->Snapshot();
    Check(!project->Execute(
              {project->Revision(),
               {ed::DiscardDocument{prefab}, ed::RenameDocument{graph_id, "Forbidden batch"}}}) &&
              project->Snapshot() == before,
          "discard cannot be injected into an unrelated batch");
    Take(project->Save());
    Take(project->Execute({0, {ed::DeleteEntity{prefab, child}}}));
    Take(project->Execute({0, {ed::DiscardDocument{prefab}}}));
    Check(!project->IsDirty(), "last document discard returns to durable save point");
    auto reopened = Take(ed::EditorProject::Open(project->File(), Schema(f, types)));
    Check(!reopened->IsDirty() &&
              std::get<comp::PrefabDefinition>(reopened->Snapshot()->Find(prefab)->data)
                      .root.children[0]
                      .id == child,
          "discard did not rewrite committed identity/data");
}

void Commands() {
    Fixture f;
    ComponentsFixture types;
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/commands.epidemic",
                                                  Project(f, types), Schema(f, types)));
    const auto before = project->Snapshot();
    const auto prefab = AssetId::FromString("editor.prefab");
    const auto graph_id = Definition().asset_id;
    const auto revision = project->Revision();
    ErrorIs(project->Execute(
                {revision, {ed::RenameDocument{prefab, "changed"}, ed::DeleteDocument{graph_id}}}),
            "editor.graph_reference");
    Check(project->Snapshot() == before && !project->CanUndo() && project->Revision() == revision,
          "failed composite has no effects or history");
    Take(project->Execute({revision,
                           {ed::SetProperty{prefab,
                                            Id("enemy"),
                                            types.health,
                                            {},
                                            {types.points, std::int64_t{50}, {}}},
                            ed::RenameDocument{prefab, "modified"}}}));
    const auto after = project->Snapshot();
    Check(std::get<std::int64_t>(std::get<comp::PrefabDefinition>(after->Find(prefab)->data)
                                     .root.components[0]
                                     .properties[0]
                                     .value) == 50,
          "typed command changes component property");
    Take(project->Undo());
    Check(project->Snapshot() == before && project->Revision() > revision,
          "undo restores exact identity snapshot with monotonic automation revision");
    Take(project->Redo());
    Check(project->Snapshot() == after, "redo restores composite state");
    ErrorIs(project->Execute({revision, {ed::RenameDocument{prefab, "stale"}}}),
            "editor.stale_revision");
    auto node = graph::NodeDefinition{Id("new.node"), Id("call.type"), "unused", {}};

    Take(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}));
    Take(project->Undo());
    ErrorIs(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}), "editor.retired_id");
    Check(project->CanRedo(), "rejected ID reuse preserves redo");
    Take(project->Redo());
    Take(project->Execute({0, {ed::DeleteGraphNode{graph_id, node.id}}}));
    ErrorIs(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}), "editor.retired_id");
    // These probes document current container-scoped identities: reuse is currently accepted.
    const auto parameter = comp::PrefabParameter{Id("probe.parameter"), std::int64_t{42}};

    Take(project->Execute({0, {ed::SetPrefabParameter{prefab, parameter}}}));
    Take(project->Undo());
    Check(static_cast<bool>(project->Execute({0, {ed::SetPrefabParameter{prefab, parameter}}})),
          "parameter ID reuse after Undo is currently accepted");
    auto behavior = std::get<comp::PrefabDefinition>(project->Snapshot()->Find(prefab)->data)
                        .root.behaviors.front();
    behavior.id = Id("probe.behavior");
    Take(project->Execute({0, {ed::SetBehavior{prefab, Id("enemy"), {}, {}, behavior}}}));
    Take(project->Execute({0, {ed::DeleteBehavior{prefab, Id("enemy"), {}, {}, behavior.id}}}));
    Check(static_cast<bool>(
              project->Execute({0, {ed::SetBehavior{prefab, Id("enemy"), {}, {}, behavior}}})),
          "behavior ID reuse after delete is currently accepted");
    Take(project->Execute({0, {ed::DeleteBehavior{prefab, Id("enemy"), {}, {}, behavior.id}}}));
    Take(project->Execute({0,
                           {ed::DeleteBehavior{prefab, Id("enemy"), {}, {}, Id("behavior")},
                            ed::DeleteDocument{graph_id}}}));
    Take(project->Undo());
    Check(project->Snapshot()->Find(graph_id) != nullptr,
          "undo restores referenced graph and behavior together");
}

void Persistence() {
    Fixture f;
    ComponentsFixture types;
    const auto directory = std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY};

    std::filesystem::create_directories(directory);
    const auto file = directory / "unit.epidemic";
    auto definition = Project(f, types);
    auto &authored_prefab = std::get<comp::PrefabDefinition>(definition.documents[1].data);
    authored_prefab.parameters.push_back({Id("start"), std::int64_t{100}});
    authored_prefab.root.components[0].properties[0].parameter = Id("start");
    authored_prefab.root.components.push_back(
        {types.movement, {}, {{types.speed, 2.5, {}}, {types.label, std::string{"враг"}, {}}}, {}});
    std::get<std::string>(authored_prefab.root.components[1].properties[1].value)
        .append(70000, 'x');
    for (std::size_t i = 0; i < 511; ++i)
        authored_prefab.root.behaviors[0].events.push_back(
            {Id("event." + std::to_string(i)), Id("event.in")});
    authored_prefab.root.children.push_back({Id("nested"), f.archetype, {}, {}, {}});
    auto &authored_scene = std::get<gc::SceneCompositionDefinition>(definition.documents[0].data);
    authored_scene.scene.nodes[0].local_transform.position = {1.25f, -2, 3};

    authored_scene.scene.nodes[0].local_bounds = Aabb{{-1, -1, -1}, {1, 1, 1}};

    SceneNodeSnapshot child;
    child.id = SceneNodeId{43};

    child.parent = SceneNodeId{42};

    authored_scene.scene.nodes.push_back(child);
    authored_scene.placements[0].overrides.parameters.push_back({Id("start"), std::int64_t{85}});
    auto project = Take(ed::EditorProject::Create(file, definition, Schema(f, types)));
    const auto graph_id = Definition().asset_id;
    const auto prefab = AssetId::FromString("editor.prefab");
    const graph::NodeDefinition node{Id("retired.saved"), Id("call.type"), "Temporary", {}};

    Take(project->Execute({0, {ed::AddGraphNode{graph_id, node}}}));
    Take(project->Undo());
    Take(project->Save());
    Check(!project->IsDirty() && !project->IsDirty(prefab),
          "save point recorded after publication");
    Take(project->Execute({0, {ed::RenameDocument{prefab, "new name"}}}));
    Check(project->IsDirty(prefab) && !project->IsDirty(graph_id), "per-document dirty state");
    Take(project->Undo());
    Check(!project->IsDirty(), "undo to saved state is clean");
    Take(project->Redo());
    auto restored = Take(ed::EditorProject::Open(file, Schema(f, types)));
    Check(!restored->IsDirty() && !restored->CanUndo() &&
              restored->Snapshot()->Find(prefab)->name == "Enemy",
          "disk contains committed save point");
    Check(std::get<graph::GraphDefinition>(restored->Snapshot()->Find(graph_id)->data) ==
              Definition(),
          "Graph uses existing serializer without changing IDs");
    const auto &restored_prefab =
        std::get<comp::PrefabDefinition>(restored->Snapshot()->Find(prefab)->data);
    Check(restored_prefab.root.children[0].id == Id("nested") &&
              restored_prefab.root.components[0].properties[0].parameter == Id("start") &&
              std::get<std::string>(restored_prefab.root.components[1].properties[1].value) ==
                  std::get<std::string>(authored_prefab.root.components[1].properties[1].value) &&
              restored_prefab.root.behaviors[0].events.size() == 512,
          "nested prefab parameters, large typed properties and supported event routes survive "
          "reload");
    const auto &restored_scene = std::get<gc::SceneCompositionDefinition>(
        restored->Snapshot()->Find(definition.entry_scene)->data);
    Check(restored_scene.scene.nodes[1].parent == SceneNodeId{42} &&
              restored_scene.scene.nodes[0].local_bounds.has_value() &&
              restored_scene.scene.nodes[0].local_transform.position == Vec3{1.25f, -2, 3} &&
              std::get<std::int64_t>(restored_scene.placements[0].overrides.parameters[0].value) ==
                  85,
          "Scene references transforms bounds and instance overrides survive reload");
    ErrorIs(restored->Execute({0, {ed::AddGraphNode{graph_id, node}}}), "editor.retired_id");
    auto failed = Take(ed::EditorProject::Create(directory / "absent" / "child.epidemic",
                                                 Project(f, types), Schema(f, types)));
    Check(!failed->Save() && failed->IsDirty(), "failed disk write does not mark clean");
    {
        std::ofstream broken(file, std::ios::binary);
        broken << "invalid";
    }

    Check(!ed::EditorProject::Open(file, Schema(f, types)), "corrupt project rejected");
    Check(restored->Snapshot()->Find(prefab)->name == "Enemy",
          "failed open does not affect existing session");
}

ed::PlayOptions Options(const ComponentsFixture &types) {
    ed::PlayOptions options;
    options.backend = std::make_unique<Backend>();
    options.bindings = [&](ed::PlayContext &context, AssetId) {
        auto bindings = Bindings(
            [store = &context.components, health = types.health,
             points = types.points](const gc::OperationContext &operation, std::int64_t amount) {
                const auto entity =
                    fw::entities::EntityService::FromGameplayObjectRef(operation.scope.Object());
                const auto component = store->Find(entity, health);
                if (!component)
                    return foundation::Result<std::int64_t>::Failure(
                        foundation::Error::Create("test.no_health", "missing Health"));
                const auto current = store->Read<std::int64_t>(*component, points);
                if (!current)
                    return foundation::Result<std::int64_t>::Failure(current.GetError());
                const auto changed =
                    store->WriteProperty(*component, {points, current.Value() - amount});
                if (!changed)
                    return foundation::Result<std::int64_t>::Failure(changed.GetError());
                return foundation::Result<std::int64_t>::Success(amount);
            });
        return std::vector<gc::NodeBinding>{bindings.begin(), bindings.end()};

    };

    return options;
}

std::int64_t HealthOf(ed::PlaySession &session, const ComponentsFixture &types) {
    const auto *instance = session.Instance(SceneNodeId{42});
    Check(instance != nullptr, "placed prefab exists in Play");
    const auto component =
        session.Context()->components.Find(instance->Objects()[0].entity, types.health);
    if (!component)
        throw std::runtime_error("runtime Health missing");
    return Take(session.Context()->components.Read<std::int64_t>(*component, types.points));
}

void GraphSceneReferences() {
    for(bool component : {false,true}) {
        Fixture f;ComponentsFixture types;
        auto definition=Project(f,types);
        auto &scene=std::get<gc::SceneCompositionDefinition>(definition.documents[0].data);
        auto second=scene.scene.nodes.front();second.id=SceneNodeId{43};scene.scene.nodes.push_back(second);
        scene.placements.push_back({second.id,scene.placements.front().prefab,{}});
        auto &graph=std::get<graph::GraphDefinition>(definition.documents[2].data);
        graph::GraphReference reference{component?graph::ReferenceKind::Component:graph::ReferenceKind::SceneEntity,scene.asset_id,Id("43"),scene.placements.front().prefab,Id("enemy"),{}, {}};
        reference.placement=graph::ElementId{43};
        if(component)reference.component=graph::TypeId{types.health.Raw()};
        const auto valueType=graph::ReferenceValueType(reference);
        graph.nodes.push_back({Id("reference"),graph::kReferenceNodeType,"Reference",{{Id("ref.out"),"reference",graph::PortKind::Data,graph::PortDirection::Output,valueType}},reference});
        graph.nodes.push_back({Id("ref.check"),graph::ReferenceCheckType(valueType),"Is Valid",{{Id("ref.in"),"reference",graph::PortKind::Data,graph::PortDirection::Input,valueType},{Id("ref.valid"),"valid",graph::PortKind::Data,graph::PortDirection::Output,graph::kI64Type}}});
        for(auto &edge:graph.connections)if(edge.input_port==Id("before.arg"))edge.output_port=Id("ref.valid");
        graph.connections.push_back({Id("ref.edge"),Id("ref.out"),Id("ref.in")});
        auto project=Take(ed::EditorProject::Create("Temp/editor-tests/references.epidemic",definition,Schema(f,types)));
        const auto source=project->Snapshot();auto session=Take(project->Play(Options(types)));
        Take(session->Send(SceneNodeId{42},{Id("hit"),1}));Take(session->Tick());Take(session->Tick(GameDuration{1}));
        Check(HealthOf(*session,types)==98,"native Play resolves explicit second placement/component");
        if(component) {
            const auto *object=session->Instance(SceneNodeId{43})->Find(Id("enemy"));
            const auto handle=session->Context()->components.Find(object->entity,types.health);
            Check(handle.has_value(),"target component exists");Take(session->Context()->components.Remove(*handle));
        } else Take(session->Context()->prefabs.Destroy(*session->Instance(SceneNodeId{43})));
        Take(session->Tick(GameDuration{1}));
        Take(session->Send(SceneNodeId{42},{Id("hit"),1}));Take(session->Tick());Take(session->Tick(GameDuration{1}));
        Check(HealthOf(*session,types)==98,"runtime reference observes removal instead of cached authoring/volatile handle");
        Take(session->Stop());Check(project->Snapshot()==source,"Stop cannot persist runtime reference state");
    }
}


void RuntimeControls() {
    Fixture f;ComponentsFixture types;auto project=Take(ed::EditorProject::Create("Temp/editor-tests/runtime.epidemic",Project(f,types),Schema(f,types)));
    auto source=project->Snapshot();auto session=Take(project->Play(Options(types)));
    Take(session->Send(SceneNodeId{42},{Id("hit"),7}));Take(session->Pump(std::chrono::milliseconds{17}));
    Check(HealthOf(*session,types)==93&&session->RuntimeSnapshot().tick==1,"native fixed pump executes damage without UI tick");
    Take(session->Pause());auto tick=session->RuntimeSnapshot().tick;Take(session->Pump(std::chrono::seconds{2}));Check(session->RuntimeSnapshot().tick==tick,"Pause freezes Runtime clock");
    Take(session->StepToNode());Check(session->RuntimeSnapshot().tick==tick,"StepToNode cannot advance Wait time");
    for(unsigned i=0;i<7;++i){auto before=session->RuntimeSnapshot().tick;Take(session->StepTick());Check(session->RuntimeSnapshot().tick==before+1,"StepTick is exactly one Runtime tick");}
    Check(HealthOf(*session,types)==86,"second damage follows native Wait deadline");
    const auto view=session->RuntimeSnapshot();Check(!view.trace.empty()&&!view.entities.empty()&&view.activations.back().native_calls==2,"immutable runtime fields and native trace counters");
    Take(session->Stop());Check(session->RuntimeSnapshot().phase==ed::PlayPhase::Stopped&&session->RuntimeSnapshot().tick==8,"Stop retains tagged history");
    Check(project->Snapshot()==source,"Runtime snapshot and damage never mutate authoring");
    auto second=Take(project->Play(Options(types)));Take(second->Pause());
    // Use the actual fixture graph identity rather than a title/display label.
    const auto graph=std::get<graph::GraphDefinition>(source->definition.documents.back().data).asset_id;
    second->SetBreakpoint(graph,Id("before"),true);Take(second->Send(SceneNodeId{42},{Id("hit"),7}));Take(second->Resume());Take(second->Pump(std::chrono::milliseconds{17}));
    Check(second->Phase()==ed::PlayPhase::Paused&&HealthOf(*second,types)==100,"breakpoint yields before native side effect");
    const auto paused_tick=second->RuntimeSnapshot().tick;
    for(unsigned i=0;i<16&&HealthOf(*second,types)==100;++i)Take(second->StepToNode());
    Check(HealthOf(*second,types)==93&&second->RuntimeSnapshot().tick==paused_tick,"bounded node stepping executes once without advancing time");
    Take(second->Stop());Take(second->Stop());Check(!second->Context(),"Stop from breakpoint is idempotent and releases Runtime");
}

void RuntimeLoadBaseline(){
    using Clock=std::chrono::steady_clock;
    for(const unsigned count:{10u,100u,1000u}){
        Fixture fixture;ComponentsFixture types;auto definition=Project(fixture,types);auto &scene=std::get<gc::SceneCompositionDefinition>(definition.documents[0].data);
        for(unsigned i=1;i<count;++i){SceneNodeSnapshot node;node.id=SceneNodeId{1000+i};scene.scene.nodes.push_back(node);scene.placements.push_back({node.id,definition.documents[1].id,{}});}
        auto project=Take(ed::EditorProject::Create("Temp/editor-tests/runtime-load.epidemic",definition,Schema(fixture,types)));auto start=Clock::now();auto session=Take(project->Play(Options(types)));const auto load=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
        Check(session->RuntimeSnapshot().total_entities==count,"native load baseline entity count");
        const unsigned activation_count=count==10?1u:100u;for(unsigned i=0;i<activation_count;++i)Take(session->Send(i==0?SceneNodeId{42}:SceneNodeId{1000+i},{Id("hit"),7}));
        std::vector<double> durations;for(unsigned i=0;i<64;++i){start=Clock::now();Take(session->Tick({1}));durations.push_back(std::chrono::duration<double,std::micro>(Clock::now()-start).count());}
        std::sort(durations.begin(),durations.end());auto frame=session->RuntimeSnapshot();Check(frame.entities.size()==count&&frame.activations.size()<=64&&frame.trace.size()<=256,"native DTO bounds under load");
        start=Clock::now();Take(session->Stop());const auto stop=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
        std::cout<<"FII4_BASELINE entities="<<count<<" activations="<<activation_count<<" load_ms="<<load<<" tick_p50_us="<<durations[31]<<" tick_p95_us="<<durations[60]<<" stop_ms="<<stop<<'\n';
        Check(!session->Context(),"load Stop releases Runtime and preserves authoring");
    }
}

void Play() {
    Fixture f;
    ComponentsFixture types;
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/play.epidemic",
                                                  Project(f, types), Schema(f, types)));
    const auto source = project->Snapshot();
    auto session = Take(project->Play(Options(types)));
    Check(session->Scene()->nodes.contains(SceneNodeId{42}) && HealthOf(*session, types) == 100,
          "Scene and Prefab use existing runtime services");
    Check(Take(session->Send(SceneNodeId{42}, {Id("hit"), 1})) == 1, "gameplay event enters graph");
    Take(session->Tick());
    Check(HealthOf(*session, types) == 99, "side effect before Wait");
    Take(session->Tick(GameDuration{1}));
    Check(HealthOf(*session, types) == 98, "resume after Wait");
    Check(project->Snapshot() == source && f.entities.AllEntities().empty(),
          "Play isolates authoring data and Entities");
    auto second = Take(project->Play(Options(types)));
    Check(HealthOf(*second, types) == 100, "independent Play sessions");
    const auto *instance = session->Instance(SceneNodeId{42});
    auto activation =
        Take(session->Context()->behaviors.Send(instance->Objects()[0].behaviors[0].handle,
                                                {Id("hit"), 1}));
    Take(session->Tick());
    Check(Status(activation) == graph::ExecutionStatus::Waiting, "activation awaiting Stop");
    Take(session->Stop());
    Check(Status(activation) == graph::ExecutionStatus::Cancelled,
          "Stop cancels pending runtime activations");
    Check(!session->IsPlaying() && session->Context() == nullptr && !session->Tick(),
          "stopped session releases mutable runtime");
    Take(session->Stop());
    auto restarted = Take(project->Play(Options(types)));
    Check(HealthOf(*restarted, types) == 100, "Play after Stop starts from authoring state");
    Take(project->Execute({0,
                           {ed::SetProperty{AssetId::FromString("editor.prefab"),
                                            Id("enemy"),
                                            types.health,
                                            {},
                                            {types.points, std::int64_t{75}, {}}}}}));
    Check(HealthOf(*restarted, types) == 100,
          "authoring edits during Play cannot mutate running instance");
    const auto destroyed = restarted->Instance(SceneNodeId{42})->Objects()[0].entity;
    Take(restarted->Context()
             ->entities.RequestDestroy(destroyed, fw::entities::EntityDestroyReason::Destroyed));
    Take(restarted->Stop());
    Check(!restarted->IsPlaying(), "Stop accepts Entities already pending destruction");
    auto draft = Definition();
    draft.asset_id = AssetId::FromString("editor.unused.graph");
    Take(project->Execute({0, {ed::CreateDocument{{draft.asset_id, "Unused graph", 0, draft}}}}));
    auto unused_failure = Options(types);
    auto normal = unused_failure.bindings;
    unused_failure.bindings = [normal, draft](ed::PlayContext &ctx, AssetId id) {
        if (id == draft.asset_id)
            throw std::runtime_error("unused factory failure");
        return normal(ctx, id);
    };

    ErrorIs(project->Play(std::move(unused_failure)), "editor.play_exception");
    auto unused_mismatch = Options(types);
    normal = unused_mismatch.bindings;
    unused_mismatch.bindings = [normal, draft](ed::PlayContext &ctx, AssetId id) {
        auto result = normal(ctx, id);
        if (id == draft.asset_id && !result.empty())
            result[0].operation = graph::Operation::Wait;
        return result;
    };

    ErrorIs(project->Play(std::move(unused_mismatch)), "editor.play_binding");
    auto throwing = Options(types);
    throwing.bindings = [](ed::PlayContext &, AssetId) -> std::vector<gc::NodeBinding> {
        throw std::runtime_error("binding failed");
    };

    ErrorIs(project->Play(std::move(throwing)), "editor.play_exception");
    Check(project->Snapshot()->Find(AssetId::FromString("editor.prefab"))->name == "Enemy",
          "failed Play does not mutate authoring");
}

void PlacementDiagnostic() {
    Fixture f;
    ComponentsFixture types;
    auto definition = Project(f, types);
    auto &scene = std::get<gc::SceneCompositionDefinition>(definition.documents[0].data);
    scene.scene.nodes[0].local_transform.position = {4, 0, 0};

    SceneNodeSnapshot parent;
    parent.id = SceneNodeId{99};

    parent.local_transform.position = {10, 0, 0};

    scene.scene.nodes.push_back(parent);
    SceneNodeSnapshot child;
    child.id = SceneNodeId{43};

    child.parent = parent.id;
    child.local_transform.position = {2, 0, 0};

    scene.scene.nodes.push_back(child);
    SceneNodeSnapshot third;
    third.id = SceneNodeId{44};

    third.local_transform.position = {-3, 0, 0};

    scene.scene.nodes.push_back(third);
    for (auto id : {43u, 44u})
        scene.placements.push_back({SceneNodeId{id}, definition.documents[1].id, {}});
    auto project = Take(ed::EditorProject::Create("Temp/editor-tests/placement-diagnostic.epidemic",
                                                  definition, Schema(f, types)));
    auto play = Take(project->Play(Options(types)));
    const auto *runtime = play->Scene();
    for (const auto &[id, x] :
         std::array<std::pair<unsigned, float>, 3>{{{42u, 4.0f}, {43u, 12.0f}, {44u, -3.0f}}}) {
        const auto node = runtime->nodes.at(SceneNodeId{id});
        const auto world = runtime->scene.transforms->GetWorldTransform(node);
        Check(world && world->position.x == x && play->Instance(SceneNodeId{id}),
              "scene world transform and placement instance exist");
    }

    const auto a = play->Instance(SceneNodeId{42})->Objects()[0].entity;
    const auto b = play->Instance(SceneNodeId{43})->Objects()[0].entity;
    const auto c = play->Instance(SceneNodeId{44})->Objects()[0].entity;
    Check(a != b && a != c && b != c, "placement instances own independent entities; spatial "
                                      "Entity binding is not implemented");
}

#include "stabilization_cases.h"
#include "p04_spike.h"

void UserSchemas() {
    std::filesystem::create_directories("Temp/editor-tests");
    Fixture fixture;ComponentsFixture components;auto schema=Schema(fixture,components);schema.user_components=&components.registry;
    auto project=Take(ed::EditorProject::Create("Temp/editor-tests/user-schema.epidemic",Project(fixture,components),schema));
    ed::UserSchema type;type.component={"qa.Turret",1,{{"Damage",std::int64_t{25}},{"Range",150.0},{"Enabled",true},{"Label",std::string{"Turret"}}}};
    type.assembly=".epidemic/scripts/v1/GameScripts.dll";type.fingerprint=std::string(64,'a');
    type.operations.push_back({"qa.Turret.ApplyDamage.v1","Turret.ApplyDamage","Turret","ApplyDamage","Scripts/Turret.cs",1});
    Take(project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{type}}}}));
    const auto component=epidemic::gameplay::TypeId::FromString(type.component.name);const auto prefab=AssetId::FromString("editor.prefab");
    Take(project->Execute({project->Revision(),{ed::SetComponent{prefab,Id("enemy"),{component,{},{},{}}}}}));
    Take(project->Execute({project->Revision(),{ed::SetProperty{prefab,Id("enemy"),component,{}, {Id("Damage"),std::int64_t{37},{}}}}}));
    Take(project->Undo());Take(project->Redo());
    std::vector<ed::Command> typed_edits{
        ed::SetProperty{prefab,Id("enemy"),component,{}, {Id("Range"),17.25,{}}},
        ed::SetProperty{prefab,Id("enemy"),component,{}, {Id("Enabled"),false,{}}},
        ed::SetProperty{prefab,Id("enemy"),component,{}, {Id("Label"),std::string{"Башня (v2)"},{}}}};
    Take(project->Execute({project->Revision(),std::move(typed_edits)}));
    Take(project->Undo());Take(project->Redo());
    const auto with_values=project->Snapshot();
    Check(!project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{}}}})&&project->Snapshot()==with_values,"referenced user type cannot disappear or lose data");
    auto invalid=type;invalid.component.fields[0].initial=std::string{"incompatible"};
    const auto before=project->Snapshot();
    Check(!project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{invalid}}}})&&project->Snapshot()==before,"incompatible schema is atomic");
    auto updated=type;updated.component.fields[0].initial=std::int64_t{50};updated.component.fields.push_back({"Heat",std::int64_t{2}});
    Take(project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{updated}}}}));
    Check(components.registry.Find(component)->properties.size()==5,"compatible field addition updates registry");
    Check(std::get<std::int64_t>(Take(components.registry.Defaults(component))[0].value)==50,"default update reaches current descriptor");
    Take(project->Undo());Check(components.registry.Find(component)->properties.size()==4,"schema Undo restores descriptor");Take(project->Redo());
    Take(project->Execute({project->Revision(),{ed::SetProperty{prefab,Id("enemy"),component,{}, {Id("Heat"),std::int64_t{2},{}}}}}));
    auto second_update=updated;second_update.component.fields.back().initial=std::int64_t{4};
    Take(project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{second_update}}}}));
    const auto &authored=std::get<comp::PrefabDefinition>(project->Snapshot()->Find(prefab)->data).root.components.back().properties;
    Check(std::any_of(authored.begin(),authored.end(),[](const auto &p){return p.property==Id("Heat")&&std::get<std::int64_t>(p.value)==2;}),"explicit value equal to old default survives later default change");
    type=second_update;
    auto duplicate=type;duplicate.component.name="qa.Other";
    Check(!project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{type,duplicate}}}}),"duplicate operation rejected");
    auto nan=type;nan.component.name="qa.NonFinite";nan.operations.clear();nan.component.fields[1].initial=std::numeric_limits<double>::quiet_NaN();
    Check(!project->Execute({project->Revision(),{ed::RegisterUserSchema{{},{nan}}}}),"NaN user default rejected");
    Take(project->Save());
    Fixture other;ComponentsFixture other_components;auto reopened_schema=Schema(other,other_components);reopened_schema.user_components=&other_components.registry;
    auto reopened=Take(ed::EditorProject::Open(project->File(),reopened_schema));
    Check(reopened->Snapshot()->definition.user_schemas.size()==1&&other_components.registry.Find(component),"persisted descriptors install without loading managed code");
    const auto &node=std::get<comp::PrefabDefinition>(reopened->Snapshot()->Find(prefab)->data).root;
    const auto found=std::find_if(node.components.begin(),node.components.end(),[&](const auto &entry){return entry.type==component;});
    Check(found!=node.components.end()&&std::get<std::int64_t>(found->properties.front().value)==37,"typed user values persist with Undo/Redo");
    const auto value=[&](std::string_view key)->const comp::PropertyValue&{const auto property=std::find_if(found->properties.begin(),found->properties.end(),[&](const auto &field){return field.property==Id(key);});if(property==found->properties.end())throw std::runtime_error("user field missing");return property->value;};
    Check(std::get<double>(value("Range"))==17.25&&!std::get<bool>(value("Enabled"))&&std::get<std::string>(value("Label"))=="Башня (v2)","real/bool/string user fields persist exactly");
    auto legacy=project->Snapshot()->definition;legacy.user_schemas.front().inherit_defaults=false;
    auto &legacy_component=std::get<comp::PrefabDefinition>(legacy.documents[1].data).root.components.back();
    for(auto &p:legacy_component.properties)if(p.property==Id("Range"))p.value=150.0;
    auto old=Take(ed::EditorProject::Create("Temp/editor-tests/legacy-schema.epidemic",std::move(legacy),schema));
    auto migration=type;migration.component.fields[1].initial=175.0;
    Take(old->Execute({old->Revision(),{ed::RegisterUserSchema{{},{migration}}}}));
    const auto &migrated=std::get<comp::PrefabDefinition>(old->Snapshot()->Find(prefab)->data).root.components.back();
    Check(std::none_of(migrated.properties.begin(),migrated.properties.end(),[](const auto &p){return p.property==Id("Range");})&&old->Snapshot()->definition.user_schemas.front().inherit_defaults,"legacy copied default becomes inherited once");
    Take(old->Undo());Check(!old->Snapshot()->definition.user_schemas.front().inherit_defaults,"Undo restores legacy policy and values");

}

int main(int argc, char **argv) {
    try {
        if (argc == 1) {
            UserSchemas();
            GraphLayoutMigration();
            Documents();
            Commands();
            Persistence();
            DocumentDiscard();
            GraphSceneReferences();
            RuntimeControls();
            RuntimeLoadBaseline();
            Play();
            PlacementDiagnostic();
        } else if (argc == 2 && std::string_view{argv[1]} == "p04-write")
            p04_spike::Write();
        else if (argc == 2 && std::string_view{argv[1]} == "p04-read")
            p04_spike::ReadAndReimport();
        else if (argc == 2 && std::string_view{argv[1]} == "failures")
            StabilizationFailures();
        else if (argc == 2 && std::string_view{argv[1]} == "authoring")
            StabilizationAuthoring();
        else if (argc == 2 && std::string_view{argv[1]} == "stop")
            StabilizationStop();
        else if (argc == 2 && std::string_view{argv[1]} == "dependencies")
            StabilizationDependencies();
        else
            throw std::runtime_error("unknown editor test case");
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }

    return 0;
}
