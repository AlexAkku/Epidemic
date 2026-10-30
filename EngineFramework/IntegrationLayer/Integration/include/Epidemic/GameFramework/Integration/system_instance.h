#pragma once
#include "Epidemic/GameFramework/Integration/graph_scope.h"

namespace epidemic::gameplay::composition {
// Owned by the host's Scene/World, never materialized as an Entity. Stop before
// releasing domain services, or provide a borrowed owner-liveness predicate.
class SystemInstance final {
public:
  [[nodiscard]] static foundation::Result<std::unique_ptr<SystemInstance>>
  Create(GraphScopeKind kind, GameplayObjectId owner,
         foundation::StringId instance,
         std::function<bool()> owner_alive = {}) {
    const auto scope =
        GraphScope::ForSystem(kind, owner, instance, std::move(owner_alive));
    if (!scope)
      return foundation::Result<std::unique_ptr<SystemInstance>>::Failure(
          scope.GetError());
    return foundation::Result<std::unique_ptr<SystemInstance>>::Success(
        std::unique_ptr<SystemInstance>(new SystemInstance(scope.Value())));
  }
  ~SystemInstance() { Stop(); }
  SystemInstance(const SystemInstance &) = delete;
  SystemInstance &operator=(const SystemInstance &) = delete;
  void Stop() noexcept { scope_->Invalidate(); }
  [[nodiscard]] const std::shared_ptr<GraphScope> &Scope() const noexcept {
    return scope_;
  }

private:
  explicit SystemInstance(std::shared_ptr<GraphScope> scope)
      : scope_(std::move(scope)) {}
  std::shared_ptr<GraphScope> scope_;
};
} // namespace epidemic::gameplay::composition
