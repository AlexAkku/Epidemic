#pragma once
#include "Epidemic/Game/Editor/editor_project.h"
#include "Epidemic/Game/Composition/gameplay.h"
#include <chrono>
#include <map>
#include <set>
#include <deque>

namespace epidemic::game::editor {
struct PlayInput { double x=0,y=0,pointer_x=0,pointer_y=0;bool pointer_down=false,attack=false; };
enum class PlayPhase : std::uint8_t { Stopped, Starting, Running, Paused, Stopping, Failed };
struct PlayField { std::string component,name,value; std::uint64_t type=0,part=0; };
struct PlayEntity { std::uint64_t placement=0,node=0,runtime_high=0,runtime_low=0;std::string name;runtime::Transform world{};std::string sprite;float ppu=64,pivot_x=.5f,pivot_y=.5f;std::vector<PlayField> fields; };
struct PlayTrace { std::uint64_t sequence=0,timestamp_ms=0,tick=0,activation=0,graph=0,node=0,entity=0;std::string state,detail;std::uint8_t severity=0; };
struct PlayDebugValue { std::uint64_t port=0;std::string type,value; };
struct PlayDebugActivation { std::uint64_t id=0,graph=0,node=0,instructions=0,native_calls=0;std::int64_t wake=-1;std::string state;std::vector<PlayDebugValue> values; };
struct PlayRuntimeSnapshot { std::uint64_t session=0,scene=0,tick=0,graph_steps=0,dropped_ticks=0,total_entities=0,total_activations=0;PlayPhase phase=PlayPhase::Stopped;std::vector<PlayEntity> entities;std::vector<PlayDebugActivation> activations;std::vector<PlayTrace> trace;std::vector<std::pair<std::uint64_t,std::uint64_t>> breakpoints;PlayInput input; };
class PlayActivation final {
  public:
    [[nodiscard]] std::uint64_t Id() const noexcept {
        return id_;
    }

    [[nodiscard]] bool IsFinished() const noexcept {
        return finished_;
    }

    [[nodiscard]] std::optional<runtime::graph::ExecutionReport> Report() const {
        return report_;
    }

  private:
    explicit PlayActivation(std::uint64_t id) : id_(id) {
    }

    std::uint64_t id_;
    bool finished_ = false;
    std::optional<runtime::graph::ExecutionReport> report_;
    friend class PlaySession;
};

struct PlayContext {
    PlayContext(gameplay::entities::EntityService &entity_service,
                comp::ComponentStore &component_store, comp::GraphBehaviorService &behavior_service,
                comp::PrefabComposer &composer,
                gameplay::runtime_bridge::RuntimeBridgeService &runtime_bridge)
        : entities(entity_service), components(component_store), behaviors(behavior_service),
          prefabs(composer), bridge(runtime_bridge) {
    }

    gameplay::entities::EntityService &entities;
    comp::ComponentStore &components;
    comp::GraphBehaviorService &behaviors;
    comp::PrefabComposer &prefabs;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge;
    composition::SceneCompositionInstance *scene = nullptr;
    composition::arena::Gameplay *gameplay = nullptr;

    [[nodiscard]] foundation::Result<runtime::Transform>
    WorldTransform(gameplay::entities::EntityHandle entity) const {
        if (scene)
            return composition::GetEntityWorldTransform(*scene, entities, entity);
        return foundation::Result<runtime::Transform>::Failure(
            foundation::Error::Create("editor.play_scene_unavailable",
                                      "Play Scene is not initialized"));
    }

    [[nodiscard]] foundation::Result<void>
    SetWorldTransform(gameplay::entities::EntityHandle entity,
                      const runtime::Transform &transform) {
        if (scene)
            return composition::SetEntityWorldTransform(*scene, entities, entity, transform);
        return foundation::Result<void>::Failure(
            foundation::Error::Create("editor.play_scene_unavailable",
                                      "Play Scene is not initialized"));
    }

};

struct PlayOptions {
    std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> backend;
    // Called once per Graph with this session's services. Native callables must
    // capture this runtime context, never an authoring Entity/ComponentStore.
    std::function<std::vector<composition::NodeBinding>(PlayContext &, runtime::AssetId)> bindings;
    bool start_events = false;
    runtime::AssetId scene{};
    // The game supplies native fixed-update behavior; no UI timer executes it.
    std::function<foundation::Result<void>(PlayContext &,runtime::GameDuration,const PlayInput &)> fixed_update;
};

// Detached visual metadata. Pixels remain owned by the session's Resources;
// source edits cannot mutate this generation while Play is active.
struct PlaySprite {
    runtime::SceneNodeId placement{};

    runtime::AssetId texture{};

    std::uint64_t generation = 0, region_id = 0;
    std::string cooked_key;
    std::uint32_t x = 0, y = 0, width = 0, height = 0;
    float pivot_x = 0.5f, pivot_y = 0.5f, pixels_per_unit = 64;
    runtime::Transform world{};
    runtime::AssetId prefab{};

};

class PlaySession final {
  public:
    ~PlaySession();
    PlaySession(const PlaySession &) = delete;
    PlaySession &operator=(const PlaySession &) = delete;
    [[nodiscard]] bool IsPlaying() const noexcept;
    [[nodiscard]] std::uint64_t Id() const noexcept {return session_id_;}
    static constexpr std::size_t kCompletedActivationRetention = 64;
    // Includes all live activations and the last 64 terminal reports. Handles
    // own only Editor report values, survive eviction/Stop/session destruction,
    // and update after Send/Tick/Stop. IDs are monotonic within this session.
    [[nodiscard]] std::vector<std::shared_ptr<const PlayActivation>> Activations() const;
    [[nodiscard]] foundation::Result<void> Tick(runtime::GameDuration advance = {});
    [[nodiscard]] foundation::Result<std::size_t> Send(runtime::SceneNodeId placement,
                                                       const comp::GameEvent &event);
    [[nodiscard]] foundation::Result<void> Stop();
    [[nodiscard]] PlayContext *Context() noexcept;
    [[nodiscard]] const comp::PrefabInstance *
    Instance(runtime::SceneNodeId placement) const noexcept;
    // Detached Inspector value; no runtime handle or mutable service escapes.
    [[nodiscard]] foundation::Result<comp::PropertyValue>
    ReadComponentProperty(runtime::SceneNodeId placement, comp::PrefabNodeId node,
                          gameplay::TypeId type, gameplay::entities::EntityPartId part,
                          comp::PropertyId property) const;
    [[nodiscard]] const composition::SceneCompositionInstance *Scene() const noexcept;
    [[nodiscard]] foundation::Result<PlaySprite> ReadSprite(runtime::SceneNodeId placement) const;
    [[nodiscard]] foundation::Result<void> Pause();
    [[nodiscard]] foundation::Result<void> Resume();
    [[nodiscard]] foundation::Result<void> StepTick();
    [[nodiscard]] foundation::Result<void> StepToNode(std::uint64_t activation=0);
    [[nodiscard]] foundation::Result<void> Pump(std::chrono::nanoseconds elapsed);
    [[nodiscard]] foundation::Result<void> SetInput(double x,double y);
    [[nodiscard]] foundation::Result<void> SetPointer(double x,double y,bool pressed);
    [[nodiscard]] foundation::Result<void> SetKey(std::uint16_t key,bool pressed);
    [[nodiscard]] foundation::Result<void> SetTimeScale(double scale);
    [[nodiscard]] double TimeScale() const noexcept {return time_scale_;}
    void SetBreakpoint(runtime::AssetId graph,runtime::graph::ElementId node,bool enabled);
    [[nodiscard]] PlayRuntimeSnapshot RuntimeSnapshot() const;
    [[nodiscard]] PlayPhase Phase() const noexcept { return phase_; }
    static constexpr std::uint64_t kFixedTicksPerSecond=60;

  private:
    struct Impl;
    explicit PlaySession(std::unique_ptr<Impl> impl);
    [[nodiscard]] static foundation::Result<std::unique_ptr<PlaySession>>
    Start(std::shared_ptr<const ProjectSnapshot> snapshot, const ProjectSchema &schema,
          PlayOptions options);
    std::unique_ptr<Impl> impl_;
    bool ticking_ = false;
    PlayPhase phase_=PlayPhase::Starting;
    PlayRuntimeSnapshot stopped_snapshot_;
    std::uint64_t session_id_=0,graph_steps_=0,dropped_ticks_=0;
    std::int64_t accumulator_=0;
    PlayInput input_;
    std::set<std::uint16_t> keys_;
    double time_scale_=1;
    bool debug_step_=false;
    std::uint64_t step_activation_=0,step_node_=0,blocked_activation_=0,blocked_node_=0;
    std::map<std::uint64_t,std::uint64_t> debug_nodes_,activation_entities_;
    std::set<std::pair<std::uint64_t,std::uint64_t>> breakpoints_;
    std::deque<PlayTrace> trace_;
    std::uint64_t trace_sequence_=0;
    void Trace(std::string state,std::uint64_t activation=0,std::uint64_t graph=0,std::uint64_t node=0,std::string detail={},std::uint8_t severity=0);
    bool DebugGate(std::uint64_t activation,std::uint64_t graph,const runtime::graph::Instruction &step);
    friend class EditorProject;
    friend class ProjectFolder;
    [[nodiscard]] foundation::Result<void> InstallSprites(const std::filesystem::path &root,
                                                          std::vector<PlaySprite> sprites);
};

} // namespace epidemic::game::editor
