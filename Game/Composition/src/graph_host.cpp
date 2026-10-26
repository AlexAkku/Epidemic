#include "Epidemic/Game/Composition/graph_host.h"
#include <algorithm>

namespace epidemic::game::composition {
namespace {
foundation::Error Expired() { return foundation::Error::Create("composition.owner_expired", "graph owner expired"); }
class Invocation final : public runtime::graph::INativeInvocation {
  public:
    Invocation(std::weak_ptr<GraphScope> scope, gameplay::runtime_bridge::RuntimeBridgeService &bridge, DomainOperation invoke)
        : scope_(std::move(scope)), bridge_(bridge), invoke_(std::move(invoke)) {}
    foundation::Result<std::int64_t> Invoke(std::int64_t value) override {
        const auto scope = scope_.lock();
        if (!scope || !scope->IsAlive()) {
            if (scope) scope->Synchronize();
            return foundation::Result<std::int64_t>::Failure(Expired());
        }
        // Synchronize even if a domain operation throws after destroying Entity.
        struct Guard { GraphScope &scope; ~Guard() { scope.Synchronize(); } } guard{*scope};
        return invoke_({*scope, bridge_}, value);
    }
  private:
    std::weak_ptr<GraphScope> scope_;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge_;
    DomainOperation invoke_;
};
class Factory final : public runtime::graph::INativeCallableFactory {
  public:
    Factory(std::weak_ptr<GraphScope> scope, gameplay::runtime_bridge::RuntimeBridgeService &bridge, DomainOperation invoke)
        : scope_(std::move(scope)), bridge_(bridge), invoke_(std::move(invoke)) {}
    foundation::Result<std::unique_ptr<runtime::graph::INativeInvocation>> CreateInvocation() const override {
        return foundation::Result<std::unique_ptr<runtime::graph::INativeInvocation>>::Success(
            std::make_unique<Invocation>(scope_, bridge_, invoke_));
    }
  private:
    std::weak_ptr<GraphScope> scope_;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge_;
    DomainOperation invoke_;
};
foundation::Result<runtime::graph::CompiledGraphPtr> CompileDefinition(const std::weak_ptr<GraphScope> &scope,
    gameplay::runtime_bridge::RuntimeBridgeService &bridge, const runtime::graph::GraphDefinition &definition,
    std::span<const NodeBinding> bindings) {
    std::vector<runtime::graph::NodeBinding> native;
    for (const auto &binding : bindings) {
        if ((binding.operation == runtime::graph::Operation::NativeCall) != static_cast<bool>(binding.invoke))
            return foundation::Result<runtime::graph::CompiledGraphPtr>::Failure(
                foundation::Error::Create("composition.invalid_binding", "only native operations have callbacks"));
        native.push_back({binding.type, binding.operation, binding.invoke ? std::make_shared<Factory>(scope, bridge, binding.invoke) : nullptr});
    }
    return runtime::graph::CompileGraph(definition, native);
}
bool HasPort(const runtime::graph::GraphDefinition &definition, std::span<const NodeBinding> bindings,
    foundation::StringId port, runtime::graph::Operation operation) {
    for (const auto &node : definition.nodes) {
        const auto binding = std::find_if(bindings.begin(), bindings.end(), [&](const auto &item) { return item.type == node.type_id; });
        if (binding != bindings.end() && binding->operation == operation)
            for (const auto &candidate : node.ports) if (candidate.id == port &&
                (operation == runtime::graph::Operation::Input ? candidate.kind == runtime::graph::PortKind::Data && candidate.direction == runtime::graph::PortDirection::Output
                                                              : candidate.kind == runtime::graph::PortKind::Event && candidate.direction == runtime::graph::PortDirection::Input)) return true;
    }
    return false;
}
} // namespace
bool GraphActivation::IsFinished() const noexcept {
    if (!task_ || !task_->LastReport()) return false;
    const auto status = task_->LastReport()->status;
    return status == runtime::graph::ExecutionStatus::Completed || status == runtime::graph::ExecutionStatus::Failed ||
           status == runtime::graph::ExecutionStatus::Cancelled;
}
GraphExecutionHost::~GraphExecutionHost() { for (const auto &active : active_) active.activation->RequestCancel(); }
foundation::Result<BoundGraphPtr> GraphExecutionHost::Compile(
    const std::shared_ptr<GraphScope> &scope, const runtime::graph::GraphDefinition &definition, std::span<const NodeBinding> bindings) {
    using Result = foundation::Result<BoundGraphPtr>;
    if (!scope || !scope->IsAlive()) return Result::Failure(Expired());
    const auto compiled = CompileDefinition(scope, bridge_, definition, bindings);
    if (!compiled) return Result::Failure(compiled.GetError());
    auto bound = std::make_shared<BoundGraph>();
    bound->program_ = compiled.Value(); bound->scope_ = scope; bound->host_ = identity_;
    return Result::Success(std::move(bound));
}
foundation::Result<gameplay::composition::GraphAssetId> GraphExecutionHost::RegisterGraph(
    runtime::graph::GraphDefinition definition, std::span<const NodeBinding> bindings) {
    using Result = foundation::Result<gameplay::composition::GraphAssetId>;
    if (assets_.size() >= 1024 || assets_.contains(definition.asset_id.Raw())) return Result::Failure(
        foundation::Error::Create("composition.graph_duplicate_or_capacity", "graph catalog cannot accept this asset"));
    const auto valid = CompileDefinition({}, bridge_, definition, bindings);
    if (!valid) return Result::Failure(valid.GetError());
    const auto id = definition.asset_id.value;
    Asset asset{std::move(definition), {bindings.begin(), bindings.end()}};
    assets_.emplace(id.Raw(), std::move(asset));
    return Result::Success(id);
}
foundation::Result<std::shared_ptr<const gameplay::composition::IGraphProgram>> GraphExecutionHost::Bind(
    const std::shared_ptr<GraphScope> &scope, const gameplay::composition::GraphBehaviorDefinition &definition) {
    using Result = foundation::Result<std::shared_ptr<const gameplay::composition::IGraphProgram>>;
    const auto found = assets_.find(definition.graph.Raw());
    if (found == assets_.end()) return Result::Failure(foundation::Error::Create("behavior.unknown_graph", "graph asset is not registered"));
    const auto &asset = found->second;
    for (const auto &input : definition.inputs) if (!HasPort(asset.definition, asset.bindings, input.port, runtime::graph::Operation::Input))
        return Result::Failure(foundation::Error::Create("behavior.invalid_inputs", "input port is not an activation input"));
    for (const auto &event : definition.events) if (!HasPort(asset.definition, asset.bindings, event.port, runtime::graph::Operation::EventEntry))
        return Result::Failure(foundation::Error::Create("behavior.invalid_routes", "event port is not an EventEntry input"));
    const auto compiled = Compile(scope, asset.definition, asset.bindings);
    if (!compiled) return Result::Failure(compiled.GetError());
    auto bound = std::make_shared<BoundGraph>();
    bound->program_ = compiled.Value()->program_; bound->scope_ = scope; bound->host_ = identity_;
    bound->inputs_ = definition.inputs; bound->events_ = definition.events;
    return Result::Success(std::move(bound));
}
foundation::Result<std::shared_ptr<gameplay::composition::IGraphActivation>> GraphExecutionHost::Dispatch(
    const std::shared_ptr<const gameplay::composition::IGraphProgram> &program, const gameplay::composition::GameEvent &event) {
    using Result = foundation::Result<std::shared_ptr<gameplay::composition::IGraphActivation>>;
    const auto bound = std::dynamic_pointer_cast<const BoundGraph>(program);
    if (!bound) return Result::Failure(foundation::Error::Create("behavior.invalid_program", "unknown host program"));
    const auto route = std::find_if(bound->events_.begin(), bound->events_.end(), [&](const auto &item) { return item.event == event.type; });
    if (route == bound->events_.end()) return Result::Failure(foundation::Error::Create("behavior.event_unbound", "event has no entry"));
    auto activation = Start(bound, route->port, event.payload);
    if (!activation) return Result::Failure(activation.GetError());
    return Result::Success(std::move(activation).Value());
}
foundation::Result<std::shared_ptr<GraphActivation>> GraphExecutionHost::Start(
    const BoundGraphPtr &program,
    runtime::graph::ElementId event_port, std::int64_t payload, const runtime::GraphScheduleOptions &options) {
    using Result = foundation::Result<std::shared_ptr<GraphActivation>>;
    if (program && program->host_ != identity_) return Result::Failure(
        foundation::Error::Create("composition.foreign_program", "program belongs to another graph host"));
    const auto scope = program ? program->scope_.lock() : nullptr;
    if (!scope || !scope->IsAlive()) return Result::Failure(Expired());
    auto instance = runtime::graph::GraphInstance::CreateForEvent(program->program_, event_port, payload);
    if (!instance) return Result::Failure(instance.GetError());
    for (const auto &input : program->inputs_) {
        const auto configured = instance.Value()->SetInput(input.port, input.value);
        if (!configured) return Result::Failure(configured.GetError());
    }
    auto activation = std::make_shared<GraphActivation>();
    std::erase_if(active_, [](const auto &active) { return active.activation->IsFinished(); });
    active_.reserve(active_.size() + 1);
    const auto tracked = scope->Track(activation);
    if (!tracked) return Result::Failure(tracked.GetError());
    auto task = scheduler_.SubmitOwned(std::move(instance).Value(), activation->owner_, options);
    if (!task) return Result::Failure(task.GetError());
    activation->task_ = std::move(task).Value();
    active_.push_back({scope, activation});
    return Result::Success(std::move(activation));
}
foundation::Result<std::size_t> GraphExecutionHost::Prepare() {
    for (const auto &active : active_) {
        if (const auto scope = active.scope.lock()) scope->Synchronize();
        else active.activation->RequestCancel();
    }
    const auto prepared = scheduler_.Prepare();
    std::erase_if(active_, [](const auto &active) { return active.activation->IsFinished(); });
    return prepared;
}
} // namespace epidemic::game::composition
