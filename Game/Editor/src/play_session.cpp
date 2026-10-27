#include "Epidemic/Game/Editor/play_session.h"
#include <unordered_set>
#include <algorithm>
#include <exception>

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
}
struct PlaySession::Impl {
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
    explicit Impl(const ProjectSchema &schema, std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> representation)
        : backend(std::move(representation)), bridge(*backend), components(entities, schema.components),
          prefabs(entities, schema.components, components, behaviors), context{entities, components, behaviors, prefabs, bridge} {}
    ~Impl() {
        // Invalidate scopes while their EntityService still exists. External
        // activation handles remain cancelled after the session is released.
        for (const auto &[id, instance] : instances) { (void)id; for (const auto &object : instance->Objects()) object.scope->Invalidate(); }
    }
};
PlaySession::PlaySession(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
PlaySession::~PlaySession() {
    try { (void)Stop(); } catch (...) {
        // Explicit Stop reports errors. Destruction must still release services;
        // their scope/host destructors cancel activations before dependencies die.
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
        (void)Take(ValidateProject(snapshot->definition, schema));
        const auto *entry = snapshot->Find(snapshot->definition.entry_scene);
        if (!entry || entry->Kind() != DocumentKind::Scene) return Result::Failure(Error("editor.entry_scene"));
        auto session = std::unique_ptr<PlaySession>(new PlaySession(std::make_unique<Impl>(schema, std::move(options.backend))));
        auto &state = *session->impl_; state.source = std::move(snapshot);
        std::unordered_set<std::uint64_t> registered;
        for (const auto &doc : state.source->definition.documents) if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc.data))
            Archetypes(prefab->root, schema, state.entities, registered);
        state.entities.Freeze();
        for (const auto &doc : state.source->definition.documents) if (const auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc.data)) {
            auto bindings = options.bindings(state.context, doc.id);
            // Authored operation semantics cannot silently change on Play.
            for (const auto &binding : bindings) {
                const auto expected = std::find_if(schema.operations.begin(), schema.operations.end(), [&](const auto &item) { return item.first == binding.type; });
                if (expected == schema.operations.end() || expected->second != binding.operation) throw Failure{Error("editor.play_binding")};
            }
            (void)Take(state.host.RegisterGraph(*graph, bindings));
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
bool PlaySession::IsPlaying() const noexcept { return impl_ != nullptr; }
PlayContext *PlaySession::Context() noexcept { return impl_ ? &impl_->context : nullptr; }
const composition::SceneCompositionInstance *PlaySession::Scene() const noexcept { return impl_ ? &impl_->scene : nullptr; }
const comp::PrefabInstance *PlaySession::Instance(runtime::SceneNodeId placement) const noexcept {
    if (!impl_) return nullptr; const auto at = impl_->instances.find(placement); return at == impl_->instances.end() ? nullptr : at->second.get();
}
foundation::Result<std::size_t> PlaySession::Send(runtime::SceneNodeId placement, const comp::GameEvent &event) {
    if (!impl_ || ticking_) return foundation::Result<std::size_t>::Failure(Error("editor.play_inactive_or_busy"));
    const auto *instance = Instance(placement); if (!instance) return foundation::Result<std::size_t>::Failure(Error("editor.play_placement"));
    const auto *root = instance->Find(instance->Definition()->root.id);
    return impl_->behaviors.SendTo(*root->scope, event);
}
foundation::Result<void> PlaySession::Tick(runtime::GameDuration advance) {
    if (!impl_ || ticking_) return foundation::Result<void>::Failure(Error("editor.play_inactive_or_busy"));
    struct Guard { bool &flag; ~Guard() { flag = false; } } guard{ticking_}; ticking_ = true;
    try {
        Take(impl_->time.runtime->Skip(advance));
        impl_->components.Synchronize(); impl_->behaviors.Synchronize();
        (void)Take(impl_->host.Prepare()); const auto tick = Take(impl_->simulation.runtime->Tick());
        if (!tick.failures.empty()) return foundation::Result<void>::Failure(Error("editor.play_execution_failed"));
        return foundation::Result<void>::Success();
    } catch (const Failure &failure) { return foundation::Result<void>::Failure(failure.error); }
}
foundation::Result<void> PlaySession::Stop() {
    if (ticking_) return foundation::Result<void>::Failure(Error("editor.play_inactive_or_busy"));
    if (!impl_) return foundation::Result<void>::Success();
    for (const auto &entity : impl_->entities.AllEntities()) {
        if (entity.lifecycle != gameplay::entities::EntityLifecycleState::Alive && entity.lifecycle != gameplay::entities::EntityLifecycleState::Dormant) continue;
        const auto destroyed = impl_->entities.RequestDestroy(entity.id, gameplay::entities::EntityDestroyReason::OwnerRequest);
        if (!destroyed) return destroyed;
    }
    impl_->components.Synchronize(); impl_->behaviors.Synchronize();
    for (const auto &[id, instance] : impl_->instances) { (void)id; for (const auto &object : instance->Objects()) object.scope->Invalidate(); }
    // Prepare observes owner invalidation before any timer can resume a graph.
    const auto prepared = impl_->host.Prepare();
    if (!prepared) return foundation::Result<void>::Failure(prepared.GetError());
    impl_.reset(); return foundation::Result<void>::Success();
}
} // namespace epidemic::game::editor
