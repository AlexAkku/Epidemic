#pragma once
#include "Epidemic/GameFramework/Integration/graph_scope.h"
#include "Epidemic/GameFramework/Integration/graph_behavior.h"
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
class BoundGraph final : public gameplay::composition::IGraphProgram {
  private:
    runtime::graph::CompiledGraphPtr program_;
    std::weak_ptr<GraphScope> scope_;
    std::shared_ptr<const int> host_;
    std::vector<gameplay::composition::GraphInput> inputs_;
    std::vector<gameplay::composition::GraphEventRoute> events_;
    friend class GraphExecutionHost;
};
using BoundGraphPtr = std::shared_ptr<const BoundGraph>;
class GraphActivation final : public gameplay::composition::IGraphActivation {
  public:
    void RequestCancel() noexcept override;
    [[nodiscard]] bool IsFinished() const noexcept override;
    [[nodiscard]] const runtime::GraphTask *Task() const noexcept { return task_.get(); }
    [[nodiscard]] const std::optional<runtime::graph::ExecutionReport> &Report() const noexcept { return task_ ? task_->LastReport() : pending_report_; }
  private:
    runtime::GraphActivationOwner owner_;
    std::shared_ptr<runtime::GraphTask> task_;
    std::unique_ptr<runtime::graph::GraphInstance> pending_;
    std::optional<runtime::graph::ExecutionReport> pending_report_;
    friend class GraphExecutionHost;
};
// Composition root: Framework supplies object capabilities; Runtime owns all IR,
// execution, budgets and waits. Neither engine layer depends on this Game adapter.
class GraphExecutionHost final : public gameplay::composition::IGraphBehaviorHost {
  public:
    GraphExecutionHost(runtime::GraphScheduler &scheduler, gameplay::runtime_bridge::RuntimeBridgeService &bridge)
        : scheduler_(scheduler), bridge_(bridge) {}
    ~GraphExecutionHost();
    GraphExecutionHost(const GraphExecutionHost &) = delete;
    GraphExecutionHost &operator=(const GraphExecutionHost &) = delete;
    [[nodiscard]] foundation::Result<gameplay::composition::GraphAssetId> RegisterGraph(
        runtime::graph::GraphDefinition definition, std::span<const NodeBinding> bindings);
    // Keep a structurally valid authoring draft available for dynamic binding.
    // The caller supplies its schema snapshot; IR validation remains in Bind.
    [[nodiscard]] foundation::Result<gameplay::composition::GraphAssetId> RegisterGraphDeferred(
        runtime::graph::GraphDefinition definition, std::span<const NodeBinding> bindings,
        std::span<const runtime::graph::TypeId> known_types);
    [[nodiscard]] foundation::Result<void> CheckDefinition(const gameplay::composition::GraphBehaviorDefinition &definition) const override;
    [[nodiscard]] foundation::Result<std::shared_ptr<const gameplay::composition::IGraphProgram>>
    Bind(const std::shared_ptr<GraphScope> &scope, const gameplay::composition::GraphBehaviorDefinition &definition) override;
    [[nodiscard]] foundation::Result<std::shared_ptr<gameplay::composition::IGraphActivation>>
    Dispatch(const std::shared_ptr<const gameplay::composition::IGraphProgram> &program, const gameplay::composition::GameEvent &event) override;
    [[nodiscard]] foundation::Result<BoundGraphPtr> Compile(
        const std::shared_ptr<GraphScope> &scope, const runtime::graph::GraphDefinition &definition, std::span<const NodeBinding> bindings);
    [[nodiscard]] foundation::Result<std::shared_ptr<GraphActivation>> Start(
        const BoundGraphPtr &program,
        runtime::graph::ElementId event_port, std::int64_t payload, const runtime::GraphScheduleOptions &options = {});
    [[nodiscard]] foundation::Result<std::size_t> Prepare();
    [[nodiscard]] std::vector<std::shared_ptr<GraphActivation>> Activations() const;
    // Cancel every scope's work and finalize reports before destroying a host.
    // Externally synchronized; never called from inside a native callback.
    [[nodiscard]] foundation::Result<void> CancelAll();
  private:
    struct Active { std::weak_ptr<GraphScope> scope; std::shared_ptr<GraphActivation> activation; runtime::GraphScheduleOptions options{}; };
    runtime::GraphScheduler &scheduler_;
    gameplay::runtime_bridge::RuntimeBridgeService &bridge_;
    std::vector<Active> active_;
    std::shared_ptr<const int> identity_ = std::make_shared<const int>(0);
    struct Asset { runtime::graph::GraphDefinition definition; std::vector<NodeBinding> bindings; };
    std::unordered_map<std::uint64_t, Asset> assets_;
};
} // namespace epidemic::game::composition
