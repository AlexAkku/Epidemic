#include "Epidemic/Game/Editor/play_session.h"
#include "Epidemic/GameFramework/Integration/system_instance.h"
#include <unordered_set>
#include <algorithm>
#include <charconv>
#include <bit>
#include <exception>
#include <deque>
#include <atomic>
#include <sstream>
#include <cmath>
#include "Epidemic/Diagnostics/profiling.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Runtime/Resources/resource_services.h"

namespace epidemic::game::editor {
namespace {
std::string NumericText(runtime::graph::TypeId type,std::int64_t bits){
    if(type!=runtime::graph::kF64Type)return std::to_string(bits);
    char text[64]{};const auto converted=std::to_chars(text,text+sizeof(text),std::bit_cast<double>(bits),std::chars_format::general);
    return converted.ec==std::errc{}?std::string(text,converted.ptr):"Unavailable";
}
struct Failure : std::exception {
    foundation::Error error;

    explicit Failure(foundation::Error value) : error(std::move(value)) {
    }

};

foundation::Error Error(std::string_view code) {
    return foundation::Error::Create(code, "editor play session rejected");
}

template <class T> T Take(foundation::Result<T> result) {
    if (!result)
        throw Failure{result.GetError()};

    return std::move(result).Value();
}

void Take(const foundation::Result<void> &result) {
    if (!result)
        throw Failure{result.GetError()};

}

class Clock final : public runtime::simulation::ISimulationClock {
  public:
    explicit Clock(std::shared_ptr<runtime::IGameClock> clock) : clock_(std::move(clock)) {
    }

    runtime::simulation::SimulationTime Now() const override {
        return clock_->Now();
    }

  private:
    std::shared_ptr<runtime::IGameClock> clock_;
};

// Keep diagnostic/display limits on a UTF-8 codepoint boundary. Authoring is untouched.
std::string BoundedUtf8(std::string_view value,std::size_t limit){
    auto end=std::min(value.size(),limit);if(end<value.size())while(end>0&&(static_cast<unsigned char>(value[end])&0xc0u)==0x80u)--end;return std::string{value.substr(0,end)};
}

runtime::simulation::SimulationServices Simulation(const runtime::TimeServices &time) {
    runtime::simulation::SimulationDependencies dependencies;
    dependencies.clock = std::make_shared<Clock>(time.clock);
    auto services = Take(runtime::simulation::CreateSimulationServices({}, dependencies));
    // Game time must not advance merely because a sibling used an editor-wide
    // work slice. Every ready activation receives its own bounded VM quantum
    // (GraphScheduleOptions); max-active and per-task limits still apply.
    services.scheduler->SetBudget({0, 0});
    return services;
}

void Archetypes(const comp::PrefabNode &node, const ProjectSchema &schema,
                gameplay::entities::EntityService &entities,
                std::unordered_set<std::uint64_t> &registered) {
    if (registered.insert(node.archetype.Raw()).second) {
        const auto *type = schema.entities.FindArchetype(node.archetype);
        if (!type)
            throw Failure{Error("editor.play_archetype")};

        (void)Take(entities.RegisterArchetype(*type));
    }

    for (const auto &child : node.children)
        Archetypes(child, schema, entities, registered);
}

struct PlayRuntime {
    std::shared_ptr<const int> reference_lifetime = std::make_shared<const int>(0);
    std::shared_ptr<const ProjectSnapshot> source;
    std::unique_ptr<ImageResourceLoader> image_loader;
    runtime::ResourceServices images;
    std::vector<runtime::ResourceLease> image_leases;
    std::unordered_map<runtime::SceneNodeId, PlaySprite> sprites;
    std::unordered_map<runtime::AssetId, PlaySprite> prefab_sprites;
    std::set<std::uint64_t> pinned_prefabs;

    PlaySprite PinSprite(PlaySprite sprite) {
        if (sprite.cooked_key.empty())
            throw Failure{Error("editor.play_sprite_source")};
        if (sprite.prefab.IsValid() && pinned_prefabs.contains(sprite.prefab.Raw())) {
            auto cached = prefab_sprites.at(sprite.prefab);
            cached.placement = sprite.placement;
            return cached;
        }
        const auto id = Take(image_loader->Register(sprite.cooked_key));
        const auto lease =
            Take(images.manager->RequestLease({id, image_loader->GetResourceType()}));
        try {
            image_leases.push_back(lease);
        } catch (...) {
            (void)images.manager->Release(lease);
            throw;
        }

        Take(images.manager->ProcessPendingLoads());
        const auto image = std::dynamic_pointer_cast<const ImagePayload>(
            images.manager->GetPayload(lease.resource));
        if (!image)
            throw Failure{Error("editor.play_sprite_payload")};

        if (sprite.region_id == 0) {
            sprite.width = image->image.width;
            sprite.height = image->image.height;
        }

        if (sprite.width == 0 || sprite.height == 0 || sprite.x > image->image.width ||
            sprite.width > image->image.width - sprite.x || sprite.y > image->image.height ||
            sprite.height > image->image.height - sprite.y)
            throw Failure{Error("editor.play_sprite_region")};

        if (sprite.prefab.IsValid())
            pinned_prefabs.insert(sprite.prefab.Raw());
        return sprite;
    }

    std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> backend;
    gameplay::entities::EntityService entities;
    gameplay::runtime_bridge::RuntimeBridgeService bridge;
    runtime::TimeServices time = Take(runtime::CreateTimeServices({.game_ticks_per_real_second=60}));
    runtime::simulation::SimulationServices simulation = Simulation(time);
    runtime::GraphScheduler scheduler{time.clock, simulation, runtime::SimulationZoneId{1}};

    composition::GraphExecutionHost host{scheduler, bridge};

    comp::ComponentStore components;
    comp::GraphBehaviorService behaviors{host};

    comp::PrefabComposer prefabs;
    PlayContext context;
    composition::SceneCompositionInstance scene;
    std::unique_ptr<composition::arena::Gameplay> gameplay_owner;
    std::uint64_t spawn_serial=0;

    std::unordered_map<runtime::SceneNodeId, std::shared_ptr<const comp::PrefabInstance>> instances;
    std::vector<std::unique_ptr<comp::SystemInstance>> systems;

    foundation::Result<bool> Resolve(const composition::OperationContext &operation_context, const runtime::graph::GraphReference &r) const {
        using Result=foundation::Result<bool>;
        if(!DescribeGraphReference(source->definition,r).available)return Result::Success(false);
        if(r.kind==runtime::graph::ReferenceKind::PrefabDefinition)return Result::Success(true);
        const comp::PrefabInstance *instance=nullptr;
        if(r.document==r.prefab) {
            for(const auto &[placement,value]:instances) {
                (void)placement;
                if(value->Definition()->id!=r.prefab.value)continue;
                for(const auto &object:value->Objects())if(object.scope->SameTarget(operation_context.scope))instance=value.get();
            }
            if(!instance)return Result::Failure(Error("editor.reference_scope"));
        } else {
            if(r.document!=source->definition.entry_scene)return Result::Failure(Error("editor.reference_scope"));
            if(!r.prefab.IsValid())return Result::Success(scene.nodes.contains(runtime::SceneNodeId{r.placement.Raw()}));
            const auto at=instances.find(runtime::SceneNodeId{r.placement.Raw()});
            if(at!=instances.end()&&at->second->Definition()->id==r.prefab.value)instance=at->second.get();
        }
        const auto *object=instance?instance->Find(r.entity):nullptr;
        if(!object||!object->scope->IsPresent())return Result::Success(false);
        if(r.kind==runtime::graph::ReferenceKind::Component)return Result::Success(components.Find(object->entity,gameplay::TypeId{r.component.Raw()},gameplay::entities::EntityPartId{r.part.Raw()}).has_value());
        return Result::Success(true);
    }

    explicit PlayRuntime(
        const ProjectSchema &schema,
        std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> representation)
        : backend(std::move(representation)), bridge(*backend),
          components(entities, schema.components),
          prefabs(entities, schema.components, components, behaviors),
          context{entities, components, behaviors, prefabs, bridge} {
    }

    ~PlayRuntime() {
        reference_lifetime.reset();
        if (images.manager)
            for (const auto &lease : image_leases)
                (void)images.manager->Release(lease);
        for (const auto &[id, instance] : instances) {
            (void)id;
            for (const auto &object : instance->Objects())
                object.scope->Invalidate();
        }

    }

};

} // namespace

struct PlaySession::Impl {
    struct Live {
        std::shared_ptr<composition::GraphActivation> execution;
        std::shared_ptr<PlayActivation> handle;
    };

    PlaySession *owner=nullptr;
    struct DebugStat { std::uint64_t graph=0,instructions=0,calls=0,serial=0;std::string state="Started";std::vector<PlayDebugValue> values; };
    std::map<std::uint64_t,DebugStat> stats;
    const comp::ComponentRegistry *registry=nullptr;
    decltype(PlayOptions::fixed_update) fixed_update;
    std::unique_ptr<PlayRuntime> runtime;
    std::vector<Live> live;
    std::deque<std::shared_ptr<PlayActivation>> completed;
    std::uint64_t next_id = 1;

    Impl(const ProjectSchema &schema,
         std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> backend)
        : runtime(std::make_unique<PlayRuntime>(schema, std::move(backend))) {
    }

    ~Impl() {
        // Destruction cannot leave Editor handles pending, even if explicit
        // shutdown failed. This path allocates nothing and runs no native code.
        for (auto &item : live) {
            item.execution->RequestCancel();
            if (!item.handle->finished_) {
                item.handle->report_.emplace(
                    runtime::graph::ExecutionReport{runtime::graph::ExecutionStatus::Cancelled});
                item.handle->finished_ = true;
            }

        }

    }

    void Capture() {
        if (!runtime)
            return;
        for (auto &execution : runtime->host.Activations()) {
            if (execution->IsFinished() ||
                std::any_of(live.begin(), live.end(),
                            [&](const auto &item) { return item.execution == execution; }))
                continue;
            live.push_back({execution, std::shared_ptr<PlayActivation>(new PlayActivation(execution->Id()))});
            stats[execution->Id()].graph=execution->Asset().Raw();
        }

    }

    void Refresh() {
        for (auto at = live.begin(); at != live.end();) {
            auto report = at->execution->Report();
            if (report) {
                auto &stat=stats[at->handle->Id()];const auto *task=at->execution->Task();
                const auto serial=task?task->ExecutionSerial():0;

                const auto status=report->status==runtime::graph::ExecutionStatus::Completed?"Completed":report->status==runtime::graph::ExecutionStatus::Cancelled?"Canceled":report->status==runtime::graph::ExecutionStatus::Failed?"Failed":report->status==runtime::graph::ExecutionStatus::Waiting?"Waiting":"Active";
                if(serial!=stat.serial||stat.state!=status){
                    if(serial!=stat.serial){stat.instructions+=report->instructions;stat.calls+=report->native_calls;stat.serial=serial;}stat.values.clear();
                    const auto *doc=runtime->source->Find(runtime::AssetId{foundation::StringId{stat.graph}});const auto *graph=doc?std::get_if<runtime::graph::GraphDefinition>(&doc->data):nullptr;
                    if(graph&&task)for(const auto &node:graph->nodes)for(const auto &port:node.ports)if(port.kind==runtime::graph::PortKind::Data&&stat.values.size()<64){
                        auto source=port.id;const auto edge=std::find_if(graph->connections.begin(),graph->connections.end(),[&](const auto &e){return e.input_port==port.id;});if(edge!=graph->connections.end())source=edge->output_port;
                        std::string value="Unavailable",type=port.type_id==runtime::graph::kI64Type?"i64":port.type_id==runtime::graph::kF64Type?"f64":"reference";
                        if(type=="i64"||type=="f64"){auto read=task->ReadValue(source);if(read)value=NumericText(port.type_id,read.Value());else if(port.default_value&&edge==graph->connections.end())value="constant "+NumericText(port.type_id,*port.default_value);}
                        else{auto read=task->ReadReference(source);if(read){const auto &r=read.Value();const auto scope=at->execution->Scope();auto valid=scope?runtime->Resolve({*scope,runtime->bridge},r):foundation::Result<bool>::Success(false);value=std::string(valid&&valid.Value()?"Live ":"Missing ")+"document="+std::to_string(r.document.Raw())+" placement="+std::to_string(r.placement.Raw())+" entity="+std::to_string(r.entity.Raw())+" component="+std::to_string(r.component.Raw())+" part="+std::to_string(r.part.Raw());}}
                        stat.values.push_back({port.id.Raw(),type,std::move(value)});
                    }
                }
                if(stat.state!=status){stat.state=status;if(owner)owner->Trace(status,at->handle->Id(),stat.graph,report->diagnostic?report->diagnostic->node.Raw():0,report->diagnostic?report->diagnostic->error.code+" "+report->diagnostic->error.context:"",report->status==runtime::graph::ExecutionStatus::Failed?2:0);}
                if(report->status==runtime::graph::ExecutionStatus::Failed&&owner&&(owner->phase_==PlayPhase::Running||owner->phase_==PlayPhase::Paused)){owner->phase_=PlayPhase::Failed;owner->input_={};owner->accumulator_=0;}
                at->handle->report_ = std::move(report);
            }
            if (at->execution->IsFinished()) {
                at->handle->finished_ = true;
                completed.push_back(at->handle);
                if (completed.size() > PlaySession::kCompletedActivationRetention) {stats.erase(completed.front()->Id());if(owner){owner->debug_nodes_.erase(completed.front()->Id());owner->activation_entities_.erase(completed.front()->Id());}completed.pop_front();}
                at = live.erase(at); // Finished Editor handles never retain a VM/callable/context.
            } else
                ++at;
        }

    }

};

PlaySession::PlaySession(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {
    static std::atomic_uint64_t sequence{0};session_id_=++sequence;impl_->owner=this;Trace("Starting");
}

PlaySession::~PlaySession() {
    try {
        (void)Stop();
    } catch (...) {
        // Complete fallback cancellation and release services immediately.
        // Explicit Stop reports errors; destruction cannot propagate them.
        impl_.reset();
    }

}

foundation::Result<std::unique_ptr<PlaySession>> EditorProject::Play(PlayOptions options) const {
    return PlaySession::Start(current_, schema_, std::move(options));
}

foundation::Result<std::unique_ptr<PlaySession>>
PlaySession::Start(std::shared_ptr<const ProjectSnapshot> snapshot, const ProjectSchema &schema,
                   PlayOptions options) {
    using Result = foundation::Result<std::unique_ptr<PlaySession>>;
    if (!snapshot || !options.backend || !options.bindings)
        return Result::Failure(Error("editor.play_options"));
    try {
        if(options.scene.IsValid()&&options.scene!=snapshot->definition.entry_scene){auto selected=std::make_shared<ProjectSnapshot>(*snapshot);selected->definition.entry_scene=options.scene;snapshot=std::move(selected);}
        const auto assets = Take(ValidateProject(snapshot->definition, schema));
        const auto *entry = snapshot->Find(snapshot->definition.entry_scene);
        if (!entry || entry->Kind() != DocumentKind::Scene)
            return Result::Failure(Error("editor.entry_scene"));
        const auto manifest = Take(assets->BuildDependencyManifest(entry->id));
        std::unordered_set<std::uint64_t> required{entry->id.Raw()};

        for (const auto &dependency : manifest.dependencies)
            required.insert(dependency.asset_id.Raw());
        std::vector<runtime::graph::TypeId> known_types{runtime::graph::kI64Type,runtime::graph::kF64Type};

        for (const auto &[type, operation] : EffectiveOperations(snapshot->definition,schema)) {
            (void)operation;
            known_types.push_back(type);
        }
        for(const auto &reference:ReferenceTypes(schema))known_types.push_back(reference.type);

        auto session = std::unique_ptr<PlaySession>(
            new PlaySession(std::make_unique<Impl>(schema, std::move(options.backend))));
        auto &state = *session->impl_->runtime;
        state.source = std::move(snapshot);
        session->impl_->registry=&schema.components;session->impl_->fixed_update=std::move(options.fixed_update);
        state.host.SetInstanceObserver([owner=session.get()](runtime::graph::GraphInstance &vm,const composition::GraphScope &scope,std::uint64_t activation){
            const auto entity=gameplay::entities::EntityService::FromGameplayObjectRef(scope.Object());
            for(const auto &[placement,instance]:owner->impl_->runtime->instances)for(const auto &object:instance->Objects())if(object.entity==entity)owner->activation_entities_[activation]=placement.Raw();
            const auto graph=vm.Program().Asset().Raw();owner->Trace("Started",activation,graph);
            vm.SetDebugExecutionGate([owner,activation,graph](const runtime::graph::Instruction &instruction,bool boundary){return owner->DebugGate(activation,graph,instruction,boundary);});
        });
        for(const auto &doc:state.source->definition.documents)for(const auto &setting:doc.graph_settings)if(setting.breakpoint)session->breakpoints_.insert({doc.id.Raw(),setting.node});
        std::unordered_set<std::uint64_t> registered;
        for (const auto &doc : state.source->definition.documents)
            if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc.data))
                Archetypes(prefab->root, schema, state.entities, registered);
        state.entities.Freeze();
        const auto &definition = std::get<composition::SceneCompositionDefinition>(entry->data);
        state.scene = Take(composition::CreateSceneComposition(definition));
        state.context.scene = &state.scene;
        state.gameplay_owner=std::make_unique<composition::arena::Gameplay>(composition::arena::Gameplay::Services{
            state.entities,state.components,state.scene,
            [&state,owner=session.get()](runtime::AssetId prefab,const runtime::Transform &world)->foundation::Result<gameplay::entities::EntityHandle>{
                using Spawn=foundation::Result<gameplay::entities::EntityHandle>;
                if(state.instances.size()>=1000||!state.prefabs.Find(prefab.value))return Spawn::Failure(Error("gameplay.prefab_unavailable_or_limit"));
                std::optional<PlaySprite> visual;if(auto sprite=state.prefab_sprites.find(prefab);sprite!=state.prefab_sprites.end())visual=state.PinSprite(sprite->second);
                auto instance=state.prefabs.Spawn(prefab.value);if(!instance)return Spawn::Failure(instance.GetError());
                const auto *root=instance.Value()->Find(instance.Value()->Definition()->root.id);
                auto node=state.scene.scene.nodes->CreateNode();if(!node){(void)state.prefabs.Destroy(*instance.Value());return Spawn::Failure(node.GetError());}
                auto placement=runtime::SceneNodeId{foundation::StringId::FromString("runtime.spawn."+std::to_string(owner->Id())+"."+std::to_string(++state.spawn_serial)).Raw()};
                auto bound=composition::BindSceneEntity(state.scene,state.entities,root->handle,node.Value());
                auto moved=bound?composition::SetEntityWorldTransform(state.scene,state.entities,root->handle,world):bound;
                if(!moved){(void)state.prefabs.Destroy(*instance.Value());(void)state.scene.scene.nodes->DestroyNode(node.Value());return Spawn::Failure(moved.GetError());}
                state.scene.nodes.emplace(placement,node.Value());
                auto handle=root->handle;state.instances.emplace(placement,std::move(instance.Value()));
                if(visual){visual->placement=placement;state.prefab_sprites.insert_or_assign(prefab,*visual);state.sprites.emplace(placement,std::move(*visual));}
                return Spawn::Success(handle);
            },
            [&state](gameplay::entities::EntityHandle entity){
                for(auto it=state.instances.begin();it!=state.instances.end();++it)for(const auto &object:it->second->Objects())if(object.handle==entity){
                    auto result=state.prefabs.Destroy(*it->second);if(!result)return result;
                    for(const auto &part:it->second->Objects())state.scene.entities.erase(part.entity);
                    const auto node=state.scene.nodes.find(it->first);if(node!=state.scene.nodes.end()){auto removed=state.scene.scene.nodes->DestroyNode(node->second);if(!removed)return removed;state.scene.nodes.erase(node);}
                    state.sprites.erase(it->first);state.instances.erase(it);return foundation::Result<void>::Success();
                }
                return foundation::Result<void>::Failure(Error("gameplay.stale_entity"));
            },
            [owner=session.get(),&state](const composition::arena::Event &event){
                std::uint64_t placement=0;for(const auto &[id,instance]:state.instances)for(const auto &object:instance->Objects())if(object.handle==event.subject)placement=id.Raw();
                owner->Trace(event.name,0,0,0,std::to_string(event.value));if(!owner->trace_.empty())owner->trace_.back().entity=placement;
                for(const auto &operation:composition::arena::Operations())if(operation.kind==runtime::graph::Operation::EventEntry&&operation.name==event.name)
                    for(const auto &[id,instance]:state.instances){(void)id;for(const auto &object:instance->Objects())if(object.handle==event.subject&&object.scope->IsAlive())
                        (void)Take(state.behaviors.SendTo(*object.scope,{foundation::StringId::FromString(operation.id),event.value}));}
            }});
        state.context.gameplay=state.gameplay_owner.get();
        for (const auto &doc : state.source->definition.documents)
            if (const auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc.data)) {
                auto bindings = options.bindings(state.context, doc.id);
                const auto weak=std::weak_ptr<const int>(state.reference_lifetime);
                auto *runtime=&state;
                for(const auto &[type,operation]:EffectiveOperations(state.source->definition,schema)) {
                    if(operation!=runtime::graph::Operation::Reference&&operation!=runtime::graph::Operation::ReferenceValid)continue;
                    composition::NodeBinding binding{type,operation,{}};
                    if(operation==runtime::graph::Operation::Reference)binding.references=[weak,runtime](const auto &context,const auto &value) {
                        if(weak.expired())return foundation::Result<bool>::Failure(Error("editor.play_inactive_or_busy"));
                        return runtime->Resolve(context,value);
                    };
                    std::erase_if(bindings,[&](const auto &b){return b.type==type;});
                    bindings.push_back(std::move(binding));
                }
                const auto effective=EffectiveOperations(state.source->definition,schema);
                for (const auto &binding : bindings) {
                    const auto expected =
                        std::find_if(effective.begin(), effective.end(),
                                     [&](const auto &item) { return item.first == binding.type; });
                    if (expected == effective.end() ||
                        expected->second != binding.operation)
                        throw Failure{Error("editor.play_binding")};

                }

                if (required.contains(doc.id.Raw()))
                    (void)Take(state.host.RegisterGraph(*graph, bindings));
                else
                    (void)Take(state.host.RegisterGraphDeferred(*graph, bindings, known_types));
            }

        const auto fill_inputs=[&](comp::GraphBehaviorDefinition &behavior){
            const auto *doc=state.source->Find(runtime::AssetId{behavior.graph});const auto *graph=doc?std::get_if<runtime::graph::GraphDefinition>(&doc->data):nullptr;if(!graph)return;
            const auto operations=EffectiveOperations(state.source->definition,schema);
            for(const auto &node:graph->nodes)if(std::any_of(operations.begin(),operations.end(),[&](const auto &op){return op.first==node.type_id&&op.second==runtime::graph::Operation::Input;}))for(const auto &port:node.ports){
                if(std::any_of(behavior.inputs.begin(),behavior.inputs.end(),[&](const auto &v){return v.port==port.id;}))continue;
                const auto setting=std::find_if(doc->graph_settings.begin(),doc->graph_settings.end(),[&](const auto &v){return v.node==node.id.Raw();});behavior.inputs.push_back({port.id,setting==doc->graph_settings.end()?0:setting->preview_default});
            }
        };
        for (const auto &doc : state.source->definition.documents)if(doc.Kind()==DocumentKind::Prefab){
            auto prefab=Take(MaterializePrefab(state.source->definition,doc.id));
            const auto configure=[&](auto &self,comp::PrefabNode &node)->void{for(auto &b:node.behaviors)fill_inputs(b);for(auto &c:node.components)for(auto &b:c.behaviors)fill_inputs(b);for(auto &child:node.children)self(self,child);};configure(configure,prefab.root);
            (void)Take(state.prefabs.Register(prefab));
        }
        for (const auto &system : state.source->definition.systems) {
            if (!system.enabled || (system.scene.IsValid() && system.scene != entry->id))
                continue;
            const bool scene_scope = system.scene.IsValid();
            auto instance = Take(comp::SystemInstance::Create(
                scene_scope ? comp::GraphScopeKind::Scene : comp::GraphScopeKind::World,
                gameplay::GameplayObjectId::FromString("editor.system.owner." +
                    std::to_string(scene_scope ? entry->id.Raw() : state.source->definition.id.Raw())),
                system.id));
            auto behavior = Take(DefaultGraphBehavior(state.source->definition,schema,system.graph,system.id));fill_inputs(behavior);
            (void)Take(state.behaviors.Attach(instance->Scope(),behavior));
            state.systems.push_back(std::move(instance));
        }
        for (const auto &placement : definition.placements) {
            auto instance = Take(state.prefabs.Spawn(placement.prefab.value, placement.overrides));
            const auto *root = instance->Find(instance->Definition()->root.id);
            if (!root)
                throw Failure{Error("editor.play_prefab_root")};

            Take(composition::BindSceneEntity(state.scene, state.entities, root->handle,
                                              state.scene.nodes.at(placement.node)));
            state.instances.emplace(placement.node, std::move(instance));
        }

        session->phase_=PlayPhase::Running;session->Trace("Running");
        if(options.start_events) {
            for(const auto &[placement,instance]:state.instances){(void)placement;
                for(const auto &object:instance->Objects()) {
                    const auto find=[&](const auto &self,const comp::PrefabNode &node)->const comp::PrefabNode*{if(node.id==object.id)return &node;for(const auto &child:node.children)if(auto found=self(self,child))return found;return nullptr;};
                    const auto *node=find(find,instance->Definition()->root);
                    if(node){std::vector<const comp::GraphBehaviorDefinition*> definitions;for(const auto &b:node->behaviors)definitions.push_back(&b);for(const auto &c:node->components)for(const auto &b:c.behaviors)definitions.push_back(&b);
                        for(const auto &slot:object.behaviors)for(const auto *b:definitions)if(b->id==slot.definition)for(const auto &route:b->events)if(std::none_of(composition::arena::Operations().begin(),composition::arena::Operations().end(),[&](const auto &op){return op.kind==runtime::graph::Operation::EventEntry&&foundation::StringId::FromString(op.id)==route.event;}))(void)Take(state.behaviors.Send(slot.handle,{route.event,0}));}

                }
            }
            std::size_t index=0;for(const auto &system:state.source->definition.systems)if(system.enabled&&(!system.scene.IsValid()||system.scene==entry->id)){
                auto behavior=Take(DefaultGraphBehavior(state.source->definition,schema,system.graph,system.id));for(const auto &route:behavior.events)if(std::none_of(composition::arena::Operations().begin(),composition::arena::Operations().end(),[&](const auto &op){return op.kind==runtime::graph::Operation::EventEntry&&foundation::StringId::FromString(op.id)==route.event;}))(void)Take(state.behaviors.SendTo(*state.systems.at(index)->Scope(),{route.event,0}));++index;
            }
        }
        session->impl_->Capture();session->impl_->Refresh();
        return Result::Success(std::move(session));
    } catch (const Failure &failure) {
        return Result::Failure(failure.error);
    } catch (const std::exception &error) {
        return Result::Failure(
            foundation::Error::Create("editor.play_exception",
                                      "session initialization or binding factory failed",
                                      error.what()));
    } catch (...) {
        return Result::Failure(Error("editor.play_exception"));
    }

}

bool PlaySession::IsPlaying() const noexcept {
    return impl_ && impl_->runtime;
}

PlayContext *PlaySession::Context() noexcept {
    return IsPlaying() ? &impl_->runtime->context : nullptr;
}

const composition::SceneCompositionInstance *PlaySession::Scene() const noexcept {
    return IsPlaying() ? &impl_->runtime->scene : nullptr;
}

const comp::PrefabInstance *PlaySession::Instance(runtime::SceneNodeId placement) const noexcept {
    if (!IsPlaying())
        return nullptr;
    const auto at = impl_->runtime->instances.find(placement);
    return at == impl_->runtime->instances.end() ? nullptr : at->second.get();
}

foundation::Result<comp::PropertyValue>
PlaySession::ReadComponentProperty(runtime::SceneNodeId placement, comp::PrefabNodeId node,
                                   gameplay::TypeId type, gameplay::entities::EntityPartId part,
                                   comp::PropertyId property) const {
    using Result = foundation::Result<comp::PropertyValue>;
    if (!IsPlaying() || ticking_)
        return Result::Failure(Error("editor.play_inactive_or_busy"));
    const auto *instance = Instance(placement);
    const auto *object = instance ? instance->Find(node) : nullptr;
    if (!object)
        return Result::Failure(Error("editor.play_object_missing"));
    const auto handle =
        std::find_if(object->components.begin(), object->components.end(),
                     [&](const auto &value) { return value.type == type && value.part == part; });
    if (handle == object->components.end())
        return Result::Failure(Error("editor.play_component_missing"));
    return impl_->runtime->context.components.ReadProperty(*handle, property);
}

foundation::Result<void> PlaySession::InstallSprites(const std::filesystem::path &root,
                                                     std::vector<PlaySprite> sprites) {
    try {
        if (!IsPlaying() || impl_->runtime->image_loader)
            return foundation::Result<void>::Failure(Error("editor.play_sprite_state"));
        auto &state = *impl_->runtime;
        state.image_loader = std::make_unique<ImageResourceLoader>(root);
        state.images = Take(runtime::CreateResourceServices({std::size_t{64} * 1024 * 1024}));
        Take(state.images.loaders->RegisterLoader(*state.image_loader));
        Take(state.images.loaders->Freeze());
        for (auto &sprite : sprites) {
            if(sprite.placement.IsValid())sprite=state.PinSprite(std::move(sprite));
            if(sprite.prefab.IsValid())state.prefab_sprites.insert_or_assign(sprite.prefab,sprite);
            if (sprite.placement.IsValid()&&!state.sprites.emplace(sprite.placement, std::move(sprite)).second)
                throw Failure{Error("editor.play_sprite_duplicate")};

        }

        return foundation::Result<void>::Success();
    } catch (const Failure &failure) {
        return foundation::Result<void>::Failure(failure.error);
    } catch (const std::exception &error) {
        return foundation::Result<void>::Failure(
            foundation::Error::Create("editor.play_sprite_exception", error.what()));
    }

}

foundation::Result<PlaySprite> PlaySession::ReadSprite(runtime::SceneNodeId placement) const {
    using R = foundation::Result<PlaySprite>;
    if (!IsPlaying() || ticking_)
        return R::Failure(Error("editor.play_inactive_or_busy"));
    const auto &state = *impl_->runtime;
    const auto found = state.sprites.find(placement);
    if (found == state.sprites.end())
        return R::Failure(Error("editor.play_sprite_missing"));
    const auto *instance = Instance(placement);
    const auto *object = instance ? instance->Find(instance->Definition()->root.id) : nullptr;
    if (!object)
        return R::Failure(Error("editor.play_object_missing"));
    const auto transform =
        composition::GetEntityWorldTransform(state.scene, state.entities, object->handle);
    if (!transform)
        return R::Failure(transform.GetError());
    auto value = found->second;
    value.world = transform.Value();
    return R::Success(std::move(value));
}

std::vector<std::shared_ptr<const PlayActivation>> PlaySession::Activations() const {
    std::vector<std::shared_ptr<const PlayActivation>> handles;
    handles.reserve(impl_->completed.size() + impl_->live.size());
    for (const auto &item : impl_->completed)
        handles.push_back(item);
    for (const auto &item : impl_->live)
        handles.push_back(item.handle);
    std::sort(handles.begin(), handles.end(),
              [](const auto &a, const auto &b) { return a->Id() < b->Id(); });
    return handles;
}

foundation::Result<std::size_t> PlaySession::Send(runtime::SceneNodeId placement,
                                                  const comp::GameEvent &event) {
    if (!IsPlaying() || ticking_)
        return foundation::Result<std::size_t>::Failure(Error("editor.play_inactive_or_busy"));
    const auto *instance = Instance(placement);
    if (!instance)
        return foundation::Result<std::size_t>::Failure(Error("editor.play_placement"));
    const auto *root = instance->Find(instance->Definition()->root.id);
    auto result = impl_->runtime->behaviors.SendTo(*root->scope, event);
    impl_->Capture();
    impl_->Refresh();
    return result;
}

foundation::Result<void> PlaySession::Tick(runtime::GameDuration advance) {
    EPIDEMIC_PROFILE_SCOPE("Editor.Play.Tick");
    if (!IsPlaying() || ticking_ || (phase_==PlayPhase::Paused&&!debug_step_) || phase_==PlayPhase::Failed)
        return foundation::Result<void>::Failure(Error("editor.play_inactive_or_busy"));

    struct Guard {
        bool &flag;

        ~Guard() {
            flag = false;
        }

    } guard{ticking_};

    ticking_ = true;
    try {
        impl_->Capture();
        impl_->Refresh();
        auto &state = *impl_->runtime;
        Take(state.time.runtime->Skip(advance));
        if(advance.ticks>0)Take(state.gameplay_owner->Begin(static_cast<std::uint64_t>(state.time.clock->Now().ticks),{input_.x,input_.y,input_.attack||input_.pointer_down}));
        if(advance.ticks>0&&impl_->fixed_update)Take(impl_->fixed_update(state.context,advance,input_));
        state.components.Synchronize();
        state.behaviors.Synchronize();
        Take(composition::SynchronizeSceneEntities(state.scene, state.entities));
        (void)Take(state.host.Prepare());
        const auto tick = Take(state.simulation.runtime->Tick());
        if(advance.ticks>0)Take(state.gameplay_owner->End(phase_!=PlayPhase::Paused));
        const auto destroyed=Take(state.entities.CommitPendingDestruction());
        state.components.Synchronize();state.behaviors.Synchronize();
        for(const auto &entity:destroyed)Take(state.entities.Remove(entity.id));
        const auto cursor=state.entities.LatestChangeCursor();
        if(cursor.sequence>4096)state.entities.PruneChangesBefore(cursor.sequence-4096);
        Take(composition::SynchronizeSceneEntities(state.scene, state.entities));
        impl_->Capture();
        impl_->Refresh();
        if (!tick.failures.empty()){phase_=PlayPhase::Failed;Trace("Failed",0,0,0,"editor.play_execution_failed",2);return foundation::Result<void>::Failure(Error("editor.play_execution_failed"));}
        return foundation::Result<void>::Success();
    } catch (const Failure &failure) {
        impl_->Refresh();phase_=PlayPhase::Failed;Trace("Failed",0,0,0,failure.error.code,2);
        return foundation::Result<void>::Failure(failure.error);
    } catch(const std::exception &error){phase_=PlayPhase::Failed;Trace("Failed",0,0,0,error.what(),2);return foundation::Result<void>::Failure(Error("editor.play_exception"));}

}

foundation::Result<void> PlaySession::Stop() {
    if (ticking_)
        return foundation::Result<void>::Failure(Error("editor.play_inactive_or_busy"));
    if (!IsPlaying())
        return foundation::Result<void>::Success();

    auto final_snapshot=RuntimeSnapshot();phase_=PlayPhase::Stopping;Trace("Stopping");
    struct Guard {
        bool &flag;

        ~Guard() {
            flag = false;
        }

    } guard{ticking_};

    ticking_ = true;
    impl_->Capture();
    auto &state = *impl_->runtime;
    state.gameplay_owner->Cancel();
    for (const auto &entity : state.entities.AllEntities()) {
        if (entity.lifecycle != gameplay::entities::EntityLifecycleState::Alive &&
            entity.lifecycle != gameplay::entities::EntityLifecycleState::Dormant)
            continue;
        const auto destroyed =
            state.entities.RequestDestroy(entity.id,
                                          gameplay::entities::EntityDestroyReason::OwnerRequest);
        if (!destroyed)
            return destroyed;
    }

    state.components.Synchronize();
    state.behaviors.Synchronize();
    for (const auto &[id, instance] : state.instances) {
        (void)id;
        for (const auto &object : instance->Objects())
            object.scope->Invalidate();
    }

    const auto cancelled = state.host.CancelAll();
    impl_->Refresh();
    if (!cancelled)
        return cancelled;
    stopped_snapshot_=std::move(final_snapshot);for(auto &activation:stopped_snapshot_.activations){const auto stat=impl_->stats.find(activation.id);if(stat!=impl_->stats.end()){activation.state=stat->second.state;activation.values=stat->second.values;}}
    impl_->runtime.reset();phase_=PlayPhase::Stopped;input_={};accumulator_=0;Trace("Stopped");
    stopped_snapshot_.entities.clear();stopped_snapshot_.total_entities=0;stopped_snapshot_.total_activations=0;
    return foundation::Result<void>::Success();
}


void PlaySession::Trace(std::string state,std::uint64_t activation,std::uint64_t graph,std::uint64_t node,std::string detail,std::uint8_t severity) {
    detail=BoundedUtf8(std::move(detail),2048);
    const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto tick=IsPlaying()?static_cast<std::uint64_t>(impl_->runtime->time.clock->Now().ticks):stopped_snapshot_.tick;
    trace_.push_back({++trace_sequence_,static_cast<std::uint64_t>(stamp),tick,activation,graph,node,activation_entities_.contains(activation)?activation_entities_.at(activation):0,std::move(state),std::move(detail),severity});
    if(trace_.size()>256)trace_.pop_front();
}
bool PlaySession::DebugGate(std::uint64_t activation,std::uint64_t graph,const runtime::graph::Instruction &step,bool boundary) {
    if(!boundary)return debug_step_?(!step_activation_||step_activation_==activation):phase_!=PlayPhase::Paused;
    if(debug_step_) {
        if(step_activation_&&step_activation_!=activation)return false;
        if(!step_activation_)step_activation_=activation;
        if(step_node_&&step_node_!=step.node.Raw()){debug_step_=false;blocked_activation_=activation;blocked_node_=step.node.Raw();return false;}
        step_node_=step.node.Raw();
    } else {
        if(phase_==PlayPhase::Paused)return false;
        if(debug_nodes_[activation]!=step.node.Raw()&&breakpoints_.contains({graph,step.node.Raw()})){
            blocked_activation_=activation;blocked_node_=step.node.Raw();phase_=PlayPhase::Paused;accumulator_=0;
            Trace("Breakpoint",activation,graph,step.node.Raw());return false;
        }
    }
    if(debug_nodes_[activation]!=step.node.Raw()){
        debug_nodes_[activation]=step.node.Raw();
        Trace("Node",activation,graph,step.node.Raw(),"operation="+std::to_string(static_cast<unsigned>(step.operation)));
    }
    if(step.operation==runtime::graph::Operation::Branch){
        const auto *doc=impl_->runtime->source->Find(runtime::AssetId{foundation::StringId{graph}});const auto *definition=doc?std::get_if<runtime::graph::GraphDefinition>(&doc->data):nullptr;
        if(definition)for(const auto &node:definition->nodes)if(node.id==step.node)for(const auto &port:node.ports)if(port.kind==runtime::graph::PortKind::Data&&port.direction==runtime::graph::PortDirection::Input){
            const auto edge=std::find_if(definition->connections.begin(),definition->connections.end(),[&](const auto &e){return e.input_port==port.id;});
            for(const auto &execution:impl_->runtime->host.Activations())if(execution->Id()==activation)if(const auto *task=execution->Task()){const auto condition=edge==definition->connections.end()?foundation::Result<std::int64_t>::Success(port.default_value.value_or(0)):task->ReadValue(edge->output_port);if(condition)Trace("Branch",activation,graph,step.node.Raw(),condition.Value()!=0?"true":"false");}
        }
    }
    return true;
}
foundation::Result<void> PlaySession::Pause(){
    if(!IsPlaying()||phase_==PlayPhase::Failed)return foundation::Result<void>::Failure(Error("editor.play_inactive"));
    phase_=PlayPhase::Paused;accumulator_=0;input_={};keys_.clear();Trace("Paused");return foundation::Result<void>::Success();
}
foundation::Result<void> PlaySession::Resume(){
    if(!IsPlaying()||phase_==PlayPhase::Failed)return foundation::Result<void>::Failure(Error("editor.play_inactive"));
    if(blocked_activation_)debug_nodes_[blocked_activation_]=blocked_node_;
    blocked_activation_=blocked_node_=0;debug_step_=false;accumulator_=0;phase_=PlayPhase::Running;Trace("Running");return foundation::Result<void>::Success();
}
foundation::Result<void> PlaySession::StepTick(){
    if(!IsPlaying()||phase_!=PlayPhase::Paused)return foundation::Result<void>::Failure(Error("editor.play_step_requires_pause"));
    if(blocked_activation_)debug_nodes_[blocked_activation_]=blocked_node_;
    blocked_activation_=blocked_node_=0;
    phase_=PlayPhase::Running;auto result=Tick({1});if(phase_!=PlayPhase::Failed)phase_=PlayPhase::Paused;accumulator_=0;Trace("StepTick");return result;
}
foundation::Result<void> PlaySession::StepToNode(std::uint64_t activation){
    if(!IsPlaying()||phase_!=PlayPhase::Paused)return foundation::Result<void>::Failure(Error("editor.play_step_requires_pause"));
    impl_->Capture();if(impl_->live.empty())return foundation::Result<void>::Failure(Error("editor.play_no_activation"));
    if(blocked_activation_)debug_nodes_[blocked_activation_]=blocked_node_;
    blocked_activation_=blocked_node_=0;
    debug_step_=true;step_activation_=activation;step_node_=0;++graph_steps_;
    auto result=Tick({});debug_step_=false;if(phase_!=PlayPhase::Failed)phase_=PlayPhase::Paused;Trace("StepToNode",step_activation_,0,step_node_);return result;
}
foundation::Result<void> PlaySession::Pump(std::chrono::nanoseconds elapsed){
    if(!IsPlaying()||phase_!=PlayPhase::Running){accumulator_=0;return foundation::Result<void>::Success();}
    const auto nanos=std::clamp<std::int64_t>(elapsed.count(),0,250000000);
    accumulator_+=static_cast<std::int64_t>(static_cast<double>(nanos)*time_scale_)*static_cast<std::int64_t>(kFixedTicksPerSecond);
    unsigned count=0;
    while(accumulator_>=1000000000&&phase_==PlayPhase::Running&&count<8){accumulator_-=1000000000;++count;auto result=Tick({1});if(!result)return result;}
    if(accumulator_>=1000000000){dropped_ticks_+=static_cast<std::uint64_t>(accumulator_/1000000000);accumulator_%=1000000000;}
    return foundation::Result<void>::Success();
}
foundation::Result<void> PlaySession::SetInput(double x,double y){
    if(!IsPlaying()||!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1||std::abs(y)>1)return foundation::Result<void>::Failure(Error("editor.play_input"));
    input_.x=x;input_.y=y;return foundation::Result<void>::Success();
}
foundation::Result<void> PlaySession::SetPointer(double x,double y,bool pressed){
    if(!IsPlaying()||!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1e6||std::abs(y)>1e6)return foundation::Result<void>::Failure(Error("editor.play_pointer"));
    input_.pointer_x=x;input_.pointer_y=y;input_.pointer_down=pressed;return foundation::Result<void>::Success();
}
foundation::Result<void> PlaySession::SetTimeScale(double scale){
    if(!IsPlaying()||!std::isfinite(scale)||scale<.1||scale>4)return foundation::Result<void>::Failure(Error("gameplay.time_scale"));
    time_scale_=scale;accumulator_=0;return foundation::Result<void>::Success();
}
foundation::Result<void> PlaySession::SetKey(std::uint16_t key,bool pressed){
    if(!IsPlaying()||key>255||(key==0&&pressed))return foundation::Result<void>::Failure(Error("gameplay.input_key"));
    if(!key){keys_.clear();input_={};return foundation::Result<void>::Success();}
    if(phase_!=PlayPhase::Running&&pressed)return foundation::Result<void>::Success();
    if(pressed)keys_.insert(key);else keys_.erase(key);
    auto bindings=composition::arena::InputBindings{};bool custom=false;
    auto &state=*impl_->runtime;
    for(const auto &entity:state.entities.AllEntities())if(auto component=state.components.Find(entity.id,gameplay::TypeId::FromString("gameplay.inputbindings"))){
        const auto read=[&](const char *name,std::int64_t fallback){auto value=state.components.Read<std::int64_t>(*component,foundation::StringId::FromString(name));return value?value.Value():fallback;};
        bindings={read("left",65),read("right",68),read("up",87),read("down",83),read("attack",32)};custom=true;break;
    }
    const auto down=[&](std::int64_t k){return keys_.contains(static_cast<std::uint16_t>(k));};
    input_.x=(down(bindings.right)||(!custom&&down(39))?1.:0.)-(down(bindings.left)||(!custom&&down(37))?1.:0.);input_.y=(down(bindings.up)||(!custom&&down(38))?1.:0.)-(down(bindings.down)||(!custom&&down(40))?1.:0.);
    // Attack uses the same sampled input stream as pointer activation.
    input_.attack=down(bindings.attack);return foundation::Result<void>::Success();
}
void PlaySession::SetBreakpoint(runtime::AssetId graph,runtime::graph::ElementId node,bool enabled){
    if(enabled&&breakpoints_.size()<1024)breakpoints_.insert({graph.Raw(),node.Raw()});else if(!enabled)breakpoints_.erase({graph.Raw(),node.Raw()});
}
PlayRuntimeSnapshot PlaySession::RuntimeSnapshot() const {
    auto out=IsPlaying()?PlayRuntimeSnapshot{}:stopped_snapshot_;out.session=session_id_;out.phase=phase_;out.graph_steps=graph_steps_;out.dropped_ticks=dropped_ticks_;out.trace.assign(trace_.begin(),trace_.end());out.breakpoints.assign(breakpoints_.begin(),breakpoints_.end());out.input=input_;
    if(!IsPlaying()||ticking_)return out;
    const auto &state=*impl_->runtime;out.scene=state.source->definition.entry_scene.Raw();out.tick=static_cast<std::uint64_t>(state.time.clock->Now().ticks);
    std::size_t field_count=0;
    for(const auto &[placement,instance]:state.instances)for(const auto &object:instance->Objects()){
        if(!object.scope->IsPresent())continue;++out.total_entities;if(out.entities.size()>=1000)continue;
        auto world=state.context.WorldTransform(object.handle);if(!world)continue;
        PlayEntity entity{placement.Raw(),object.id.Raw(),object.entity.High(),object.entity.Low(),"Entity",world.Value(),{},64,.5f,.5f,{}};
        if(const auto *prefab=state.source->Find(runtime::AssetId{instance->Definition()->id}))entity.name=prefab->name;
        if(object.id==instance->Definition()->root.id){auto sprite=ReadSprite(placement);if(sprite){entity.sprite=sprite.Value().cooked_key;entity.ppu=sprite.Value().pixels_per_unit;entity.pivot_x=sprite.Value().pivot_x;entity.pivot_y=sprite.Value().pivot_y;}}
        if(const auto *doc=state.source->Find(state.source->definition.entry_scene)){
            const auto &scene=std::get<composition::SceneCompositionDefinition>(doc->data);auto name=scene.node_names.find(placement.Raw());if(name!=scene.node_names.end())entity.name=name->second;
        }
        for(const auto &component:object.components)if(const auto *info=impl_->registry->Find(component.type))for(const auto &property:info->properties){
            if(field_count++>=4096)break;
            auto value=state.components.ReadProperty(component,property.id);std::string text="Unavailable";
            if(value)text=std::visit([](const auto &v){using T=std::decay_t<decltype(v)>;if constexpr(std::is_same_v<T,std::string>)return BoundedUtf8(v,1024);else if constexpr(std::is_same_v<T,bool>)return std::string(v?"true":"false");else if constexpr(std::is_floating_point_v<T>){char text[64]{};const auto converted=std::to_chars(text,text+sizeof(text),v,std::chars_format::general);return converted.ec==std::errc{}?std::string(text,converted.ptr):std::string("Unavailable");}else return std::to_string(v);},value.Value());
            entity.fields.push_back({info->name,property.name,text,component.type.Raw(),component.part.Raw()});
        }
        out.entities.push_back(std::move(entity));
    }
    out.activations.clear();out.total_activations=impl_->live.size();
    std::vector<std::uint64_t> order;for(const auto &live:impl_->live)order.push_back(live.handle->Id());for(auto it=impl_->completed.rbegin();it!=impl_->completed.rend();++it)order.push_back((*it)->Id());
    for(const auto id:order){const auto &stat=impl_->stats.at(id);
        if(out.activations.size()>=64)break;
        PlayDebugActivation activation{id,stat.graph,0,stat.instructions,stat.calls,-1,stat.state,stat.values};
        const auto at=debug_nodes_.find(id);if(at!=debug_nodes_.end())activation.node=at->second;
        if(blocked_activation_==id)activation.node=blocked_node_;
        for(const auto &live:impl_->live)if(live.handle->Id()==id)if(const auto *task=live.execution->Task()){
            if(task->NextWake())activation.wake=task->NextWake()->ticks;
        }
        out.activations.push_back(std::move(activation));
    }
    return out;
}

} // namespace epidemic::game::editor
