#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/play_session.h"
#include <cstdio>
#include <stdexcept>

namespace {
using namespace epidemic;
using namespace epidemic::runtime;
namespace ed = game::editor;
namespace comp = gameplay::composition;
namespace gc = game::composition;
namespace rb = gameplay::runtime_bridge;
foundation::StringId Id(std::string_view value) { return foundation::StringId::FromString(value); }
template<class T> T Take(foundation::Result<T> result) { if (!result) throw std::runtime_error(result.GetError().code); return std::move(result).Value(); }
void Take(const foundation::Result<void> &result) { if (!result) throw std::runtime_error(result.GetError().code); }
void Check(bool value) { if (!value) throw std::runtime_error("headless editor invariant"); }
struct Health { std::int64_t points = 100; };
bool ValidHealth(const std::int64_t &value) { return value >= 0 && value <= 10000; }
class Backend final : public rb::IRuntimeBridgeBackend {
  public:
    foundation::Result<rb::RuntimeMaterializationResult> Materialize(rb::RuntimePersistentObjectHandle) override { return foundation::Result<rb::RuntimeMaterializationResult>::Success({rb::RuntimeObjectHandle{next_++}, true}); }
    foundation::Result<void> Dematerialize(rb::RuntimeObjectHandle) override { return foundation::Result<void>::Success(); }
    foundation::Result<void> Destroy(rb::RuntimeObjectHandle) override { return foundation::Result<void>::Success(); }
    foundation::Result<void> ApplyImpulse(rb::RuntimePhysicsBodyHandle, rb::RuntimeVector3) override { return foundation::Result<void>::Success(); }
    foundation::Result<void> ProjectEnvironment(rb::RuntimeRegionHandle, const rb::RuntimeEnvironmentValues &, gameplay::Revision) override { return foundation::Result<void>::Success(); }
    std::vector<rb::RuntimeContactObservation> ConsumeContacts() override { return {}; }
  private:
    std::uint64_t next_ = 1;
};
struct Schema {
    gameplay::entities::EntityService entities;
    comp::ComponentRegistry components;
    gameplay::entities::EntityArchetypeId enemy;
    gameplay::TypeId health;
    comp::PropertyId points;
    Schema() {
        gameplay::entities::EntityArchetypeDefinition type; type.canonical_name = "editor.example.enemy";
        enemy = Take(entities.RegisterArchetype(type)); entities.Freeze();
        health = Take(components.Register<Health>("editor.example.health"));
        points = Take(components.RegisterProperty(health, "points", &Health::points, &ValidHealth)); components.Freeze();
    }
    ed::ProjectSchema View() { return {entities, components, {{Id("event.type"), graph::Operation::EventEntry}, {Id("native.type"), graph::Operation::NativeCall}, {Id("wait.type"), graph::Operation::Wait}}}; }
};
graph::PortDefinition Port(std::string_view name, graph::PortKind kind, graph::PortDirection direction) {
    return {Id(name), std::string(name), kind, direction, kind == graph::PortKind::Control ? graph::TypeId{} : graph::kI64Type};
}
void Write(const std::filesystem::path &file, Schema &schema) {
    const auto scene_id = AssetId::FromString("example.scene"), graph_id = AssetId::FromString("example.graph"), prefab_id = AssetId::FromString("example.prefab");
    gc::SceneCompositionDefinition scene; scene.asset_id = scene_id;
    graph::GraphDefinition graph{graph_id, "Enemy behavior", {}, {}};
    auto initial = Take(ed::EditorProject::Create(file, {Id("example.project"), scene_id, {{scene_id, "Scene", 0, scene}, {graph_id, "Graph", 0, graph}}}, schema.View()));
    Take(initial->Save()); initial.reset();
    auto project = Take(ed::EditorProject::Open(file, schema.View()));
    ed::EditRequest edit{project->Revision(), {}};
    comp::PrefabDefinition prefab; prefab.id = prefab_id.value;
    edit.commands = {ed::CreateDocument{{prefab_id, "Enemy", 0, prefab}},
        ed::AddEntity{prefab_id, {}, {Id("enemy"), schema.enemy, {}, {}, {}}},
        ed::SetComponent{prefab_id, Id("enemy"), {schema.health, {}, {{schema.points, std::int64_t{100}, {}}}, {}}},
        ed::SetBehavior{prefab_id, Id("enemy"), {}, {}, {Id("hit.behavior"), graph_id.value, {{Id("hit"), Id("event.in")}}, {}}}};
    using K = graph::PortKind; using D = graph::PortDirection;
    const auto data = [](std::string_view name, D direction) { return Port(name, K::Data, direction); };
    const auto control = [](std::string_view name, D direction) { return Port(name, K::Control, direction); };
    const std::vector<graph::NodeDefinition> nodes{
        {Id("event"), Id("event.type"), "Event", {Port("event.in", K::Event, D::Input), data("payload", D::Output), control("event.next", D::Output)}},
        {Id("before"), Id("native.type"), "Damage", {data("before.arg", D::Input), data("before.result", D::Output), control("before.in", D::Input), control("before.next", D::Output)}},
        {Id("wait"), Id("wait.type"), "Wait", {data("wait.value", D::Input), data("wait.ticks", D::Input), data("wait.result", D::Output), control("wait.in", D::Input), control("wait.next", D::Output)}},
        {Id("after"), Id("native.type"), "Damage again", {data("after.arg", D::Input), data("after.result", D::Output), control("after.in", D::Input), control("after.next", D::Output)}}};
    for (const auto &node : nodes) edit.commands.push_back(ed::AddGraphNode{graph_id, node});
    const auto edge = [&](std::string_view from, std::string_view to) {
        edit.commands.push_back(ed::ConnectPorts{graph_id, {Id(std::string(from) + "/" + std::string(to)), Id(from), Id(to)}});
    };
    edge("payload", "before.arg"); edge("payload", "wait.ticks"); edge("before.result", "wait.value"); edge("wait.result", "after.arg");
    edge("event.next", "before.in"); edge("before.next", "wait.in"); edge("wait.next", "after.in");
    SceneNodeSnapshot node; node.id = SceneNodeId{42}; edit.commands.push_back(ed::AddSceneNode{scene_id, node});
    edit.commands.push_back(ed::PlacePrefab{scene_id, {node.id, prefab_id, {}}});
    (void)Take(project->Execute(edit)); const auto edited = project->Snapshot();
    Take(project->Undo()); Check(project->Snapshot()->definition.documents.size() == 2 && !project->IsDirty());
    Take(project->Redo()); Check(project->Snapshot() == edited);
    Take(project->Save()); Check(!project->IsDirty());
    std::puts("PASS: open, commands, references, undo/redo, atomic save");
}
void ReadPlay(const std::filesystem::path &file, Schema &schema) {
    auto project = Take(ed::EditorProject::Open(file, schema.View())); Check(!project->IsDirty());
    ed::PlayOptions options; options.backend = std::make_unique<Backend>();
    options.bindings = [&](ed::PlayContext &context, AssetId) {
        gc::DomainOperation damage = [store = &context.components, health = schema.health, points = schema.points](const gc::OperationContext &operation, std::int64_t amount) {
            using Result = foundation::Result<std::int64_t>;
            const auto entity = gameplay::entities::EntityService::FromGameplayObjectRef(operation.scope.Object());
            const auto component = store->Find(entity, health);
            if (!component) return Result::Failure(foundation::Error::Create("example.health_missing", "Health required"));
            const auto current = store->Read<std::int64_t>(*component, points); if (!current) return Result::Failure(current.GetError());
            const auto changed = store->WriteProperty(*component, {points, current.Value() - amount}); if (!changed) return Result::Failure(changed.GetError());
            return Result::Success(amount);
        };
        return std::vector<gc::NodeBinding>{{Id("event.type"), graph::Operation::EventEntry, {}}, {Id("native.type"), graph::Operation::NativeCall, std::move(damage)}, {Id("wait.type"), graph::Operation::Wait, {}}};
    };
    auto session = Take(project->Play(std::move(options)));
    const auto entity = session->Instance(SceneNodeId{42})->Objects()[0].entity;
    const auto health = session->Context()->components.Find(entity, schema.health);
    if (!health) throw std::runtime_error("runtime Health missing");
    (void)Take(session->Send(SceneNodeId{42}, {Id("hit"), 1})); Take(session->Tick());
    Check(Take(session->Context()->components.Read<std::int64_t>(*health, schema.points)) == 99);
    Take(session->Tick(GameDuration{1})); Check(Take(session->Context()->components.Read<std::int64_t>(*health, schema.points)) == 98);
    Take(session->Stop()); Check(!project->IsDirty() && schema.entities.AllEntities().empty());
    std::puts("PASS: new process load, Entity/Health/GraphBehavior, Wait/resume, Stop, isolated authoring");
}
}
int main(int argc, char **argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: EpidemicEditorHeadless --write|--read-play <project path>");
        Schema schema; const std::filesystem::path file{argv[2]};
        if (std::string_view{argv[1]} == "--write") { std::filesystem::create_directories(file.parent_path()); Write(file, schema); }
        else if (std::string_view{argv[1]} == "--read-play") ReadPlay(file, schema);
        else throw std::runtime_error("unknown command");
    } catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
