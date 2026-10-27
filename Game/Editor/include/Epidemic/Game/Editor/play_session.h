#pragma once
#include "Epidemic/Game/Editor/editor_project.h"

namespace epidemic::game::editor {
struct PlayContext {
    PlayContext(gameplay::entities::EntityService &entity_service, comp::ComponentStore &component_store,
        comp::GraphBehaviorService &behavior_service, comp::PrefabComposer &composer,
        gameplay::runtime_bridge::RuntimeBridgeService &runtime_bridge)
        : entities(entity_service), components(component_store), behaviors(behavior_service), prefabs(composer), bridge(runtime_bridge) {}
    gameplay::entities::EntityService &entities;
    comp::ComponentStore &components;
    comp::GraphBehaviorService &behaviors;
    comp::PrefabComposer &prefabs;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge;
};
struct PlayOptions {
    std::unique_ptr<gameplay::runtime_bridge::IRuntimeBridgeBackend> backend;
    // Called once per Graph with this session's services. Native callables must
    // capture this runtime context, never an authoring Entity/ComponentStore.
    std::function<std::vector<composition::NodeBinding>(PlayContext &, runtime::AssetId)> bindings;
};
class PlaySession final {
  public:
    ~PlaySession();
    PlaySession(const PlaySession &) = delete;
    PlaySession &operator=(const PlaySession &) = delete;
    [[nodiscard]] bool IsPlaying() const noexcept;
    [[nodiscard]] foundation::Result<void> Tick(runtime::GameDuration advance = {});
    [[nodiscard]] foundation::Result<std::size_t> Send(runtime::SceneNodeId placement, const comp::GameEvent &event);
    [[nodiscard]] foundation::Result<void> Stop();
    [[nodiscard]] PlayContext *Context() noexcept;
    [[nodiscard]] const comp::PrefabInstance *Instance(runtime::SceneNodeId placement) const noexcept;
    [[nodiscard]] const composition::SceneCompositionInstance *Scene() const noexcept;
  private:
    struct Impl;
    explicit PlaySession(std::unique_ptr<Impl> impl);
    [[nodiscard]] static foundation::Result<std::unique_ptr<PlaySession>> Start(
        std::shared_ptr<const ProjectSnapshot> snapshot, const ProjectSchema &schema, PlayOptions options);
    std::unique_ptr<Impl> impl_;
    bool ticking_ = false;
    friend class EditorProject;
};
} // namespace epidemic::game::editor
