#pragma once

#include "Epidemic/Runtime/Graph/graph_vm.h"
#include <limits>

namespace epidemic::runtime::graph::detail {
inline constexpr std::uint32_t kNoNode = std::numeric_limits<std::uint32_t>::max();
struct FlowNode {
    std::vector<std::uint32_t> data;
    std::vector<std::uint32_t> next;
};
struct FlowProgram {
    std::vector<FlowNode> nodes;
    std::uint32_t entry = kNoNode;
};
class FlowCompiler {
  public:
    static bool UsesFlow(const GraphDefinition &definition, std::span<const NodeBinding> bindings);
    static foundation::Result<CompiledGraphPtr> Compile(const GraphDefinition &definition, std::span<const NodeBinding> bindings);
};
class FlowInstance {
  public:
    explicit FlowInstance(GraphInstance &owner);
    ExecutionReport Run(ExecutionBudget budget);
    void Cancel() noexcept;
  private:
    enum class Kind { Execute, Apply, Evaluate, Finish };
    struct Work { Kind kind = Kind::Execute; std::uint32_t node = 0; };
    GraphInstance &owner_;
    std::vector<Work> work_;
    std::vector<std::uint64_t> cache_;
    std::uint64_t epoch_ = 1;
    bool completed_ = false;
    void Push(std::uint32_t node);
    void NextEpoch();
    ExecutionReport Fail(std::string_view code, ExecutionReport report);
    ExecutionReport Fail(foundation::Error error, ExecutionReport report);
    ExecutionDiagnostic Diagnostic(std::string_view code) const;
};
} // namespace epidemic::runtime::graph::detail
