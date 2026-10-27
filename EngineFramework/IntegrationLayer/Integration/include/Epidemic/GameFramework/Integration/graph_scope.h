#pragma once
#include "Epidemic/GameFramework/Entities/entities.h"
#include <memory>
#include "Epidemic/Foundation/string_id.h"
#include <functional>

namespace epidemic::gameplay::composition {
enum class GraphScopeKind { Entity, Scene, World };
class IGraphActivation {
  public:
    virtual ~IGraphActivation() = default;
    virtual void RequestCancel() noexcept = 0;
    [[nodiscard]] virtual bool IsFinished() const noexcept = 0;
};

// A capability for an existing Entity/Part, never an entity registry. The
// EntityService outlives scopes. All calls are on the host's gameplay thread.
class GraphScope final {
  public:
    ~GraphScope();
    GraphScope(const GraphScope &) = delete;
    GraphScope &operator=(const GraphScope &) = delete;
    [[nodiscard]] static foundation::Result<std::shared_ptr<GraphScope>>
    ForEntity(entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part = {});
    [[nodiscard]] static foundation::Result<std::shared_ptr<GraphScope>>
    ForEntityState(entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part = {});
    [[nodiscard]] static foundation::Result<std::shared_ptr<GraphScope>> Child(const std::shared_ptr<GraphScope> &parent);
    [[nodiscard]] static foundation::Result<std::shared_ptr<GraphScope>> ForSystem(
        GraphScopeKind kind, GameplayObjectId owner, foundation::StringId instance, std::function<bool()> owner_alive = {});
    [[nodiscard]] GraphScopeKind Kind() const noexcept { return kind_; }
    [[nodiscard]] GameplayObjectId ScopeOwner() const noexcept { return scope_owner_; }
    [[nodiscard]] foundation::StringId SystemId() const noexcept { return system_id_; }
    [[nodiscard]] bool IsAlive() const;
    [[nodiscard]] bool IsPresent() const;
    [[nodiscard]] GameplayObjectRef Object() const noexcept;
    [[nodiscard]] entities::EntityHandle Handle() const noexcept { return handle_; }
    [[nodiscard]] entities::EntityPartId Part() const noexcept { return part_; }
    [[nodiscard]] bool SameTarget(const GraphScope &other) const noexcept;
    [[nodiscard]] foundation::Result<void> Track(const std::shared_ptr<IGraphActivation> &activation);
    void Synchronize() noexcept;
    void Invalidate() noexcept;
  private:
    GraphScope() = default;
    [[nodiscard]] bool Check(bool require_active) const;
    void CancelExecutions() noexcept;
    GraphScope(entities::EntityService &entities, entities::EntityId entity, entities::EntityPartId part);
    entities::EntityService *entities_ = nullptr;
    entities::EntityId entity_{};
    entities::EntityHandle handle_{};
    entities::EntityPartId part_{};
    bool enabled_ = true;
    std::vector<std::weak_ptr<IGraphActivation>> activations_;
    std::weak_ptr<GraphScope> parent_;
    std::vector<std::weak_ptr<GraphScope>> children_;
    std::uint32_t depth_ = 0;
    GraphScopeKind kind_ = GraphScopeKind::Entity;
    GameplayObjectId scope_owner_{};
    foundation::StringId system_id_{};
    std::shared_ptr<const int> system_identity_;
    std::function<bool()> owner_alive_;
};
} // namespace epidemic::gameplay::composition
