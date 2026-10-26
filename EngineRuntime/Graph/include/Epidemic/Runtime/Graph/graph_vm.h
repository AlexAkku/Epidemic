#pragma once

#include "Epidemic/Runtime/Graph/graph_compiler.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include <optional>

namespace epidemic::runtime::graph {
struct ExecutionBudget {
    std::size_t instructions = 1024;
    std::size_t native_calls = 64;
};
enum class ExecutionStatus : std::uint8_t { Completed, BudgetExhausted, Failed, Waiting, Cancelled };
struct ExecutionDiagnostic {
    AssetId asset{};
    ElementId node{};
    std::size_t instruction = 0;
    foundation::Error error;
};
struct ExecutionReport {
    ExecutionStatus status = ExecutionStatus::Completed;
    std::size_t instructions = 0;
    std::size_t native_calls = 0;
    std::optional<ExecutionDiagnostic> diagnostic;
    std::optional<GameDuration> wait{};
};

// One-shot, externally synchronized invocation. Run seals input values; budget
// yields resume at the next unexecuted instruction. Completed/failed instances
// never replay a prefix. Native state and registers belong to this instance.
class GraphInstance final {
  public:
    GraphInstance(const GraphInstance &) = delete;
    GraphInstance &operator=(const GraphInstance &) = delete;
    GraphInstance(GraphInstance &&) = delete;
    GraphInstance &operator=(GraphInstance &&) = delete;
    [[nodiscard]] static foundation::Result<std::unique_ptr<GraphInstance>> Create(CompiledGraphPtr program);
    [[nodiscard]] foundation::Result<void> SetInput(ElementId port, std::int64_t value);
    // Read a node's output port, or the input port of a builtin Output node.
    [[nodiscard]] foundation::Result<std::int64_t> ReadValue(ElementId port) const;
    [[nodiscard]] ExecutionReport Run(ExecutionBudget budget = {});
    // Only the host scheduler acknowledges a due wait. Run alone cannot skip it.
    [[nodiscard]] foundation::Result<void> ResumeWait();
    void Cancel() noexcept;
    [[nodiscard]] const CompiledGraph &Program() const noexcept { return *program_; }
    [[nodiscard]] std::size_t InstructionPointer() const noexcept { return instruction_; }
    [[nodiscard]] bool HasStarted() const noexcept { return started_; }

  private:
    explicit GraphInstance(CompiledGraphPtr program);
    CompiledGraphPtr program_;
    std::vector<std::optional<std::int64_t>> values_;
    std::vector<std::unique_ptr<INativeInvocation>> native_;
    std::size_t instruction_ = 0;
    bool started_ = false;
    bool running_ = false;
    bool cancelled_ = false;
    struct PendingWait {
        GameDuration duration{};
        ElementId node{};
        std::size_t instruction = 0;
        std::uint32_t destination = 0;
        std::int64_t value = 0;
    };
    std::optional<PendingWait> wait_;
    std::optional<ExecutionDiagnostic> failure_;
};
} // namespace epidemic::runtime::graph
