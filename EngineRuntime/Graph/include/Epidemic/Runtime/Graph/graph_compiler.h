#pragma once

#include "Epidemic/Runtime/Graph/graph_definition.h"
#include <memory>

namespace epidemic::runtime::graph {
namespace detail {
struct FlowProgram;
class FlowCompiler;
class FlowInstance;
} // namespace detail
inline constexpr TypeId kI64Type = TypeId::FromString("epidemic.graph.i64");
inline constexpr std::string_view kI64TypeName = "epidemic.graph.i64";
inline constexpr std::int64_t kMaxRepeatIterations = 1'000'000;
enum class Operation : std::uint8_t {
  Input,
  Add,
  Multiply,
  NativeCall,
  Output,
  Wait,
  Entry,
  Sequence,
  Branch,
  EventEntry,
  Repeat
};

class INativeInvocation {
public:
  virtual ~INativeInvocation() = default;
  [[nodiscard]] virtual foundation::Result<std::int64_t>
  Invoke(std::int64_t argument) = 0;
};
class INativeCallableFactory {
public:
  virtual ~INativeCallableFactory() = default;
  // Every call must return independently owned mutable invocation state.
  [[nodiscard]] virtual foundation::Result<std::unique_ptr<INativeInvocation>>
  CreateInvocation() const = 0;
};
struct NodeBinding {
  TypeId node_type{};
  Operation operation = Operation::Input;
  std::shared_ptr<const INativeCallableFactory> native;
};
struct Instruction {
  Operation operation = Operation::Input;
  ElementId node{};
  std::uint32_t destination = 0;
  std::uint32_t left = 0;
  std::uint32_t right = 0;
  std::uint32_t callable = 0;
};

class CompiledGraph;
using CompiledGraphPtr = std::shared_ptr<const CompiledGraph>;
[[nodiscard]] foundation::Result<CompiledGraphPtr>
CompileGraph(const GraphDefinition &definition,
             std::span<const NodeBinding> bindings);

// Process-local immutable IR, not a persisted wire format. Register operands
// are emitted only by CompileGraph. Node IDs retain authoring diagnostic
// identity.
class CompiledGraph final {
public:
  [[nodiscard]] AssetId Asset() const noexcept { return asset_; }
  [[nodiscard]] std::span<const Instruction> Instructions() const noexcept {
    return instructions_;
  }
  [[nodiscard]] std::size_t RegisterCount() const noexcept {
    return instructions_.size();
  }

private:
  CompiledGraph() = default;
  struct Slot {
    ElementId port{};
    std::uint32_t index = 0;
    bool input = false;
  };
  struct NativeSite {
    ElementId node;
    std::shared_ptr<const INativeCallableFactory> factory;
  };
  AssetId asset_{};
  std::vector<Instruction> instructions_;
  std::vector<Slot> slots_;
  std::vector<NativeSite> native_sites_;
  std::shared_ptr<const detail::FlowProgram> flow_;
  friend foundation::Result<CompiledGraphPtr>
  CompileGraph(const GraphDefinition &, std::span<const NodeBinding>);
  friend class GraphInstance;
  friend class detail::FlowCompiler;
  friend class detail::FlowInstance;
};
} // namespace epidemic::runtime::graph
