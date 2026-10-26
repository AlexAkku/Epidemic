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
    std::vector<runtime::graph::NodeBinding> native;
    for (const auto &binding : bindings) {
        if ((binding.operation == runtime::graph::Operation::NativeCall) != static_cast<bool>(binding.invoke))
            return Result::Failure(foundation::Error::Create("composition.invalid_binding", "only native operations have callbacks"));
        native.push_back({binding.type, binding.operation, binding.invoke ? std::make_shared<Factory>(scope, bridge_, binding.invoke) : nullptr});
    }
    const auto compiled = runtime::graph::CompileGraph(definition, native);
    if (!compiled) return Result::Failure(compiled.GetError());
    auto bound = std::make_shared<BoundGraph>();
    bound->program_ = compiled.Value(); bound->scope_ = scope; bound->host_ = identity_;
    return Result::Success(std::move(bound));
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
    auto activation = std::make_shared<GraphActivation>();
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
