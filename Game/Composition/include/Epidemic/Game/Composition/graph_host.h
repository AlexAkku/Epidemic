#pragma once
#include "Epidemic/GameFramework/Integration/graph_scope.h"
#include "Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h"
#include "Epidemic/Runtime/Support/graph_scheduler.h"
#include <functional>

namespace epidemic::game::composition {
using GraphScope = gameplay::composition::GraphScope;
struct OperationContext {
    const GraphScope &scope;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge;
};
using DomainOperation = std::function<foundation::Result<std::int64_t>(const OperationContext &, std::int64_t)>;
struct NodeBinding {
    runtime::graph::TypeId type{};
    runtime::graph::Operation operation = runtime::graph::Operation::Input;
    DomainOperation invoke{};
};
class BoundGraph final {
  private:
    runtime::graph::CompiledGraphPtr program_;
    std::weak_ptr<GraphScope> scope_;
    std::shared_ptr<const int> host_;
    friend class GraphExecutionHost;
};
using BoundGraphPtr = std::shared_ptr<const BoundGraph>;
class GraphActivation final : public gameplay::composition::IGraphActivation {
  public:
    void RequestCancel() noexcept override { owner_.Invalidate(); }
    [[nodiscard]] bool IsFinished() const noexcept override;
    [[nodiscard]] const runtime::GraphTask &Task() const noexcept { return *task_; }
  private:
    runtime::GraphActivationOwner owner_;
    std::shared_ptr<runtime::GraphTask> task_;
    friend class GraphExecutionHost;
};
// Composition root: Framework supplies object capabilities; Runtime owns all IR,
// execution, budgets and waits. Neither engine layer depends on this Game adapter.
class GraphExecutionHost final {
  public:
    GraphExecutionHost(runtime::GraphScheduler &scheduler, gameplay::runtime_bridge::RuntimeBridgeService &bridge)
        : scheduler_(scheduler), bridge_(bridge) {}
    ~GraphExecutionHost();
    GraphExecutionHost(const GraphExecutionHost &) = delete;
    GraphExecutionHost &operator=(const GraphExecutionHost &) = delete;
    [[nodiscard]] foundation::Result<BoundGraphPtr> Compile(
        const std::shared_ptr<GraphScope> &scope, const runtime::graph::GraphDefinition &definition, std::span<const NodeBinding> bindings);
    [[nodiscard]] foundation::Result<std::shared_ptr<GraphActivation>> Start(
        const BoundGraphPtr &program,
        runtime::graph::ElementId event_port, std::int64_t payload, const runtime::GraphScheduleOptions &options = {});
    [[nodiscard]] foundation::Result<std::size_t> Prepare();
  private:
    struct Active { std::weak_ptr<GraphScope> scope; std::shared_ptr<GraphActivation> activation; };
    runtime::GraphScheduler &scheduler_;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge_;
    std::vector<Active> active_;
    std::shared_ptr<const int> identity_ = std::make_shared<const int>(0);
};
} // namespace epidemic::game::composition
