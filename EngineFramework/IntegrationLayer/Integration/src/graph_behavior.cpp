#include "Epidemic/GameFramework/Integration/graph_behavior.h"
#include <algorithm>
#include <limits>
#include <unordered_set>

namespace epidemic::gameplay::composition {
namespace {
foundation::Error Error(std::string_view code) {
  return foundation::Error::Create(code, "graph behavior rejected");
}
bool Routed(const GraphBehaviorDefinition &definition,
            foundation::StringId event) {
  return std::any_of(definition.events.begin(), definition.events.end(),
                     [&](const auto &route) { return route.event == event; });
}
} // namespace
GraphBehaviorService::~GraphBehaviorService() {
  for (const auto &[_, behavior] : behaviors_)
    behavior.scope->Invalidate();
}
foundation::Result<void> GraphBehaviorService::Validate(
    const GraphBehaviorDefinition &definition) const {
  using Result = foundation::Result<void>;
  if (!definition.id.IsValid() || !definition.graph.IsValid() ||
      definition.events.empty() || definition.events.size() > 1024 ||
      definition.inputs.size() > 1024)
    return Result::Failure(Error("behavior.invalid_definition"));
  std::unordered_set<std::uint64_t> events, inputs;
  for (const auto &route : definition.events)
    if (!route.event.IsValid() || !route.port.IsValid() ||
        !events.insert(route.event.Raw()).second)
      return Result::Failure(Error("behavior.invalid_routes"));
  for (const auto &input : definition.inputs)
    if (!input.port.IsValid() || !inputs.insert(input.port.Raw()).second)
      return Result::Failure(Error("behavior.invalid_inputs"));
  return host_.CheckDefinition(definition);
}
foundation::Result<BehaviorHandle>
GraphBehaviorService::Attach(const std::shared_ptr<GraphScope> &owner,
                             const GraphBehaviorDefinition &definition) {
  using Result = foundation::Result<BehaviorHandle>;
  const auto valid = Validate(definition);
  if (!valid)
    return Result::Failure(valid.GetError());
  Synchronize();
  if (behaviors_.size() >= 4096 ||
      next_ == std::numeric_limits<std::uint64_t>::max())
    return Result::Failure(Error("behavior.capacity"));
  const auto scope = GraphScope::Child(owner);
  if (!scope)
    return Result::Failure(scope.GetError());
  const auto program = host_.Bind(scope.Value(), definition);
  if (!program)
    return Result::Failure(program.GetError());
  if (!program.Value())
    return Result::Failure(Error("behavior.invalid_program"));
  const BehaviorHandle handle{next_};
  behaviors_.emplace(
      next_, Behavior{definition, owner, scope.Value(), program.Value()});
  ++next_;
  return Result::Success(handle);
}
foundation::Result<void> GraphBehaviorService::Detach(BehaviorHandle behavior) {
  const auto found = behaviors_.find(behavior.generation);
  if (found == behaviors_.end())
    return foundation::Result<void>::Failure(Error("behavior.stale_handle"));
  found->second.scope->Invalidate();
  behaviors_.erase(found);
  return foundation::Result<void>::Success();
}
foundation::Result<std::shared_ptr<IGraphActivation>>
GraphBehaviorService::Send(BehaviorHandle behavior, const GameEvent &event) {
  using Result = foundation::Result<std::shared_ptr<IGraphActivation>>;
  const auto found = behaviors_.find(behavior.generation);
  if (found == behaviors_.end() || !found->second.scope->IsPresent())
    return Result::Failure(Error("behavior.stale_handle"));
  if (!found->second.scope->IsAlive())
    return Result::Failure(Error("behavior.owner_inactive"));
  if (!Routed(found->second.definition, event.type))
    return Result::Failure(Error("behavior.event_unbound"));
  return host_.Dispatch(found->second.program, event);
}
foundation::Result<std::size_t>
GraphBehaviorService::SendTo(const GraphScope &target, const GameEvent &event) {
  using Result = foundation::Result<std::size_t>;
  if (!target.IsAlive())
    return Result::Failure(Error("composition.owner_expired"));
  std::vector<std::uint64_t> recipients;
  for (const auto &[id, behavior] : behaviors_)
    if (behavior.scope->IsAlive() && behavior.scope->SameTarget(target) &&
        Routed(behavior.definition, event.type))
      recipients.push_back(id);
  std::sort(
      recipients.begin(),
      recipients.end()); // Stable attachment order, never hash iteration order.
  std::vector<std::shared_ptr<IGraphActivation>> accepted;
  accepted.reserve(recipients.size());
  for (const auto id : recipients) {
    auto activation = Send({id}, event);
    if (!activation) {
      for (const auto &active : accepted)
        active->RequestCancel();
      return Result::Failure(activation.GetError());
    }
    accepted.push_back(std::move(activation).Value());
  }
  return Result::Success(accepted.size());
}
void GraphBehaviorService::Synchronize() {
  for (auto it = behaviors_.begin(); it != behaviors_.end();) {
    if (!it->second.scope->IsPresent()) {
      it->second.scope->Invalidate();
      it = behaviors_.erase(it);
    } else {
      it->second.scope->Synchronize();
      ++it;
    }
  }
}
} // namespace epidemic::gameplay::composition
