#include "Epidemic/Game/Editor/play_session.h"
#include <unordered_set>
#include <algorithm>
#include <exception>
#include <deque>

namespace epidemic::game::editor {
namespace {
struct Failure : std::exception {
    foundation::Error error;
    explicit Failure(foundation::Error value) : error(std::move(value)) {}
};
foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "editor play session rejected"); }
template<class T> T Take(foundation::Result<T> result) { if (!result) throw Failure{result.GetError()}; return std::move(result).Value(); }
void Take(const foundation::Result<void> &result) { if (!result) throw Failure{result.GetError()}; }
class Clock final : public runtime::simulation::ISimulationClock {
  public:
    explicit Clock(std::shared_ptr<runtime::IGameClock> clock) : clock_(std::move(clock)) {}
    runtime::simulation::SimulationTime Now() const override { return clock_->Now(); }
  private:
    std::shared_ptr<runtime::IGameClock> clock_;
};
runtime::simulation::SimulationServices Simulation(const runtime::TimeServices &time) {
    runtime::simulation::SimulationDependencies dependencies; dependencies.clock = std::make_shared<Clock>(time.clock);
    auto services = Take(runtime::simulation::CreateSimulationServices({}, dependencies)); services.scheduler->SetBudget({16, 512}); return services;
}
void Archetypes(const comp::PrefabNode &node, const ProjectSchema &schema, gameplay::entities::EntityService &entities,
    std::unordered_set<std::uint64_t> &registered) {
    if (registered.insert(node.archetype.Raw()).second) {
        const auto *type = schema.entities.FindArchetype(node.archetype); if (!type) throw Failure{Error("editor.play_archetype")};
        (void)Take(entities.RegisterArchetype(*type));
    }
    for (const auto &child : node.children) Archetypes(child, schema, entities, registered);
}
struct PlayRuntime {
    std::shared_ptr<const ProjectSnapshot> source;
    std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> backend;
    gameplay::entities::EntityService entities;
    gameplay::runtime_bridge::RuntimeBridgeService bridge;
    runtime::TimeServices time = Take(runtime::CreateTimeServices());
    runtime::simulation::SimulationServices simulation = Simulation(time);
    runtime::GraphScheduler scheduler{time.clock, simulation, runtime::SimulationZoneId{1}};
    composition::GraphExecutionHost host{scheduler, bridge};
    comp::ComponentStore components;
    comp::GraphBehaviorService behaviors{host};
    comp::PrefabComposer prefabs;
    PlayContext context;
    composition::SceneCompositionInstance scene;
    std::unordered_map<runtime::SceneNodeId, std::shared_ptr<const comp::PrefabInstance>> instances;
    explicit PlayRuntime(const ProjectSchema &schema, std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> representation)
        : backend(std::move(representation)), bridge(*backend), components(entities, schema.components),
          prefabs(entities, schema.components, components, behaviors), context{entities, components, behaviors, prefabs, bridge} {}
    ~PlayRuntime() {
        for (const auto &[id, instance] : instances) { (void)id; for (const auto &object : instance->Objects()) object.scope->Invalidate(); }
    }
};
}
struct PlaySession::Impl {
    struct Live { std::shared_ptr<composition::GraphActivation> execution; std::shared_ptr<PlayActivation> handle; };
    std::unique_ptr<PlayRuntime> runtime;
    std::vector<Live> live;
    std::deque<std::shared_ptr<PlayActivation>> completed;
    std::uint64_t next_id = 1;
    Impl(const ProjectSchema &schema, std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> backend)
        : runtime(std::make_unique<PlayRuntime>(schema, std::move(backend))) {}
    ~Impl() {
        // Destruction cannot leave Editor handles pending, even if explicit
        // shutdown failed. This path allocates nothing and runs no native code.
        for (auto &item : live) {
            item.execution->RequestCancel();
            if (!item.handle->finished_) {
                item.handle->report_.emplace(runtime::graph::ExecutionReport{runtime::graph::ExecutionStatus::Cancelled});
                item.handle->finished_ = true;
            }
        }
    }
    void Capture() {
        if (!runtime) return;
        for (auto &execution : runtime->host.Activations()) {
            if (execution->IsFinished() || std::any_of(live.begin(), live.end(), [&](const auto &item) { return item.execution == execution; })) continue;
            live.push_back({execution, std::shared_ptr<PlayActivation>(new PlayActivation(next_id++))});
        }
    }
    void Refresh() {
        for (auto at = live.begin(); at != live.end();) {
            auto report = at->execution->Report();
            if (report) at->handle->report_ = std::move(report);
            if (at->execution->IsFinished()) {
                at->handle->finished_ = true;
                completed.push_back(at->handle);
                if (completed.size() > PlaySession::kCompletedActivationRetention) completed.pop_front();
                at = live.erase(at); // Finished Editor handles never retain a VM/callable/context.
            } else ++at;
        }
    }
};
PlaySession::PlaySession(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
PlaySession::~PlaySession() {
    try { (void)Stop(); } catch (...) {
        // Complete fallback cancellation and release services immediately.
        // Explicit Stop reports errors; destruction cannot propagate them.
        impl_.reset();
    }
}
foundation::Result<std::unique_ptr<PlaySession>> EditorProject::Play(PlayOptions options) const {
    return PlaySession::Start(current_, schema_, std::move(options));
}
foundation::Result<std::unique_ptr<PlaySession>> PlaySession::Start(std::shared_ptr<const ProjectSnapshot> snapshot,
    const ProjectSchema &schema, PlayOptions options) {
    using Result = foundation::Result<std::unique_ptr<PlaySession>>;
    if (!snapshot || !options.backend || !options.bindings) return Result::Failure(Error("editor.play_options"));
    try {
        const auto assets = Take(ValidateProject(snapshot->definition, schema));
        const auto *entry = snapshot->Find(snapshot->definition.entry_scene);
        if (!entry || entry->Kind() != DocumentKind::Scene) return Result::Failure(Error("editor.entry_scene"));
        const auto manifest = Take(assets->BuildDependencyManifest(entry->id));
        std::unordered_set<std::uint64_t> required{entry->id.Raw()};
        for (const auto &dependency : manifest.dependencies) required.insert(dependency.asset_id.Raw());
        std::vector<runtime::graph::TypeId> known_types{runtime::graph::kI64Type};
        for (const auto &[type, operation] : schema.operations) { (void)operation; known_types.push_back(type); }
        auto session = std::unique_ptr<PlaySession>(new PlaySession(std::make_unique<Impl>(schema, std::move(options.backend))));
        auto &state = *session->impl_->runtime; state.source = std::move(snapshot);
        std::unordered_set<std::uint64_t> registered;
        for (const auto &doc : state.source->definition.documents) if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc.data))
            Archetypes(prefab->root, schema, state.entities, registered);
        state.entities.Freeze();
        for (const auto &doc : state.source->definition.documents) if (const auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc.data)) {
            auto bindings = options.bindings(state.context, doc.id);
            for (const auto &binding : bindings) {
                const auto expected = std::find_if(schema.operations.begin(), schema.operations.end(), [&](const auto &item) { return item.first == binding.type; });
                if (expected == schema.operations.end() || expected->second != binding.operation) throw Failure{Error("editor.play_binding")};
            }
            if (required.contains(doc.id.Raw())) (void)Take(state.host.RegisterGraph(*graph, bindings));
            else (void)Take(state.host.RegisterGraphDeferred(*graph, bindings, known_types));
        }
        for (const auto &doc : state.source->definition.documents) if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc.data)) (void)Take(state.prefabs.Register(*prefab));
        const auto &definition = std::get<composition::SceneCompositionDefinition>(entry->data);
        state.scene = Take(composition::CreateSceneComposition(definition));
        for (const auto &placement : definition.placements) state.instances.emplace(placement.node, Take(state.prefabs.Spawn(placement.prefab.value, placement.overrides)));
        return Result::Success(std::move(session));
    } catch (const Failure &failure) { return Result::Failure(failure.error); }
    catch (const std::exception &error) { return Result::Failure(foundation::Error::Create("editor.play_exception", "session initialization or binding factory failed", error.what())); }
    catch (...) { return Result::Failure(Error("editor.play_exception")); }
}
bool PlaySession::IsPlaying() const noexcept { return impl_ && impl_->runtime; }
PlayContext *PlaySession::Context() noexcept { return IsPlaying() ? &impl_->runtime->context : nullptr; }
const composition::SceneCompositionInstance *PlaySession::Scene() const noexcept { return IsPlaying() ? &impl_->runtime->scene : nullptr; }
const comp::PrefabInstance *PlaySession::Instance(runtime::SceneNodeId placement) const noexcept {
    if (!IsPlaying()) return nullptr; const auto at = impl_->runtime->instances.find(placement); return at == impl_->runtime->instances.end() ? nullptr : at->second.get();
}
foundation::Result<comp::PropertyValue> PlaySession::ReadComponentProperty(runtime::SceneNodeId placement,
    comp::PrefabNodeId node,gameplay::TypeId type,gameplay::entities::EntityPartId part,comp::PropertyId property) const {
    using Result=foundation::Result<comp::PropertyValue>;
    if (!IsPlaying() || ticking_) return Result::Failure(Error("editor.play_inactive_or_busy"));
    const auto *instance=Instance(placement); const auto *object=instance?instance->Find(node):nullptr;
    if (!object) return Result::Failure(Error("editor.play_object_missing"));
    const auto handle=std::find_if(object->components.begin(),object->components.end(),[&](const auto &value) { return value.type==type && value.part==part; });
    if (handle==object->components.end()) return Result::Failure(Error("editor.play_component_missing"));
    return impl_->runtime->context.components.ReadProperty(*handle,property);
}
std::vector<std::shared_ptr<const PlayActivation>> PlaySession::Activations() const {
    std::vector<std::shared_ptr<const PlayActivation>> handles;
    handles.reserve(impl_->completed.size() + impl_->live.size());
    for (const auto &item : impl_->completed) handles.push_back(item);
    for (const auto &item : impl_->live) handles.push_back(item.handle);
    std::sort(handles.begin(), handles.end(), [](const auto &a, const auto &b) { return a->Id() < b->Id(); });
    return handles;
}
foundation::Result<std::size_t> PlaySession::Send(runtime::SceneNodeId placement, const comp::GameEvent &event) {
    if (!IsPlaying() || ticking_) return foundation::Result<std::size_t>::Failure(Error("editor.play_inactive_or_busy"));
    const auto *instance = Instance(placement); if (!instance) return foundation::Result<std::size_t>::Failure(Error("editor.play_placement"));
    const auto *root = instance->Find(instance->Definition()->root.id);
    auto result = impl_->runtime->behaviors.SendTo(*root->scope, event);
    impl_->Capture(); impl_->Refresh(); return result;
}
foundation::Result<void> PlaySession::Tick(runtime::GameDuration advance) {
    if (!IsPlaying() || ticking_) return foundation::Result<void>::Failure(Error("editor.play_inactive_or_busy"));
    struct Guard { bool &flag; ~Guard() { flag = false; } } guard{ticking_}; ticking_ = true;
    try {
        impl_->Capture(); impl_->Refresh();
        auto &state = *impl_->runtime;
        Take(state.time.runtime->Skip(advance));
        state.components.Synchronize(); state.behaviors.Synchronize();
        (void)Take(state.host.Prepare()); const auto tick = Take(state.simulation.runtime->Tick());
        impl_->Capture(); impl_->Refresh();
        if (!tick.failures.empty()) return foundation::Result<void>::Failure(Error("editor.play_execution_failed"));
        return foundation::Result<void>::Success();
    } catch (const Failure &failure) { impl_->Refresh(); return foundation::Result<void>::Failure(failure.error); }
}
foundation::Result<void> PlaySession::Stop() {
    if (ticking_) return foundation::Result<void>::Failure(Error("editor.play_inactive_or_busy"));
    if (!IsPlaying()) return foundation::Result<void>::Success();
    struct Guard { bool &flag; ~Guard() { flag = false; } } guard{ticking_}; ticking_ = true;
    impl_->Capture();
    auto &state = *impl_->runtime;
    for (const auto &entity : state.entities.AllEntities()) {
        if (entity.lifecycle != gameplay::entities::EntityLifecycleState::Alive && entity.lifecycle != gameplay::entities::EntityLifecycleState::Dormant) continue;
        const auto destroyed = state.entities.RequestDestroy(entity.id, gameplay::entities::EntityDestroyReason::OwnerRequest);
        if (!destroyed) return destroyed;
    }
    state.components.Synchronize(); state.behaviors.Synchronize();
    for (const auto &[id, instance] : state.instances) { (void)id; for (const auto &object : instance->Objects()) object.scope->Invalidate(); }
    const auto cancelled = state.host.CancelAll();
    impl_->Refresh();
    if (!cancelled) return cancelled;
    impl_->runtime.reset(); return foundation::Result<void>::Success();
}
} // namespace epidemic::game::editor
