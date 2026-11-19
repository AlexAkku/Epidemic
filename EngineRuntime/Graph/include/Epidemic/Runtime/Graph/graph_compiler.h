#pragma once

#include "Epidemic/Runtime/Graph/graph_definition.h"
#include <memory>
#include <array>

namespace epidemic::runtime::graph {
namespace detail {
struct FlowProgram;
class FlowCompiler;
class FlowInstance;
} // namespace detail

inline constexpr TypeId kI64Type = TypeId::FromString("epidemic.graph.i64");
inline constexpr TypeId kF64Type = TypeId::FromString("epidemic.graph.f64");
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
    Repeat,
    Reference,
    ReferenceValid,
    Subtract, Divide, IntegerDivide, RoundUp, RoundDown, RoundNearest, ToFloat, ToInteger
};

[[nodiscard]] constexpr bool IsMath(Operation op) {return op==Operation::Add||op==Operation::Multiply||op>=Operation::Subtract;}
[[nodiscard]] constexpr bool UnaryMath(Operation op) {return op>=Operation::RoundUp;}
[[nodiscard]] constexpr bool FloatCapable(Operation op) {return op==Operation::Input||op==Operation::Output||IsMath(op);}
struct NumericRecipe {std::string_view id,name;Operation operation=Operation::Input;bool floating=false;};
inline constexpr std::array kNumericRecipes{
    NumericRecipe{"epidemic.math.input.f64.v1","Input (f64)",Operation::Input,true},
    NumericRecipe{"epidemic.math.output.f64.v1","Output (f64)",Operation::Output,true},
    NumericRecipe{"epidemic.math.add.f64.v1","Add (f64)",Operation::Add,true},
    NumericRecipe{"epidemic.math.multiply.f64.v1","Multiply (f64)",Operation::Multiply,true},
    NumericRecipe{"epidemic.math.subtract.i64.v1","Subtract (i64)",Operation::Subtract,false},
    NumericRecipe{"epidemic.math.divide.i64.v1","Divide (i64)",Operation::Divide,false},
    NumericRecipe{"epidemic.math.quotient.i64.v1","Integer Divide (i64)",Operation::IntegerDivide,false},
    NumericRecipe{"epidemic.math.subtract.f64.v1","Subtract (f64)",Operation::Subtract,true},
    NumericRecipe{"epidemic.math.divide.f64.v1","Divide (f64)",Operation::Divide,true},
    NumericRecipe{"epidemic.math.quotient.f64.v1","Integer Divide (f64)",Operation::IntegerDivide,true},
    NumericRecipe{"epidemic.math.ceil.f64.v1","Round Up (f64)",Operation::RoundUp,true},
    NumericRecipe{"epidemic.math.floor.f64.v1","Round Down (f64)",Operation::RoundDown,true},
    NumericRecipe{"epidemic.math.round.f64.v1","Round (0.6 up, f64)",Operation::RoundNearest,true},
    NumericRecipe{"epidemic.math.convert.f64.v1","Convert i64 to f64",Operation::ToFloat,true},
    NumericRecipe{"epidemic.math.convert.i64.v1","Convert f64 to i64",Operation::ToInteger,false}
};
// Resolution stays in the native host's scope. The VM never converts a reference
// to an i64 or caches a volatile entity handle across a yield/Stop.
class IReferenceResolver {
  public:
    virtual ~IReferenceResolver() = default;
    [[nodiscard]] virtual foundation::Result<bool> Resolve(const GraphReference &reference) const = 0;
};

class INativeInvocation {
  public:
    virtual ~INativeInvocation() = default;
    [[nodiscard]] virtual foundation::Result<std::int64_t> Invoke(std::int64_t argument) = 0;
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
    std::shared_ptr<const IReferenceResolver> references{};
    std::optional<std::int64_t> constant_input{}; // Compiler-lowered literal, not a managed ABI argument.
};

struct Instruction {
    Operation operation = Operation::Input;
    ElementId node{};

    std::uint32_t destination = 0;
    std::uint32_t left = 0;
    std::uint32_t right = 0;
    std::uint32_t callable = 0;
    TypeId value_type = kI64Type;
};

class CompiledGraph;
using CompiledGraphPtr = std::shared_ptr<const CompiledGraph>;
[[nodiscard]] foundation::Result<CompiledGraphPtr>
CompileGraph(const GraphDefinition &definition, std::span<const NodeBinding> bindings);

// Process-local immutable IR, not a persisted wire format. Register operands are
// emitted only by CompileGraph. Node IDs retain authoring diagnostic identity.
class CompiledGraph final {
  public:
    [[nodiscard]] AssetId Asset() const noexcept {
        return asset_;
    }

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
        std::optional<std::int64_t> initial{};
    };

    struct NativeSite {
        ElementId node;
        std::shared_ptr<const INativeCallableFactory> factory;
    };
    struct ReferenceSite {
        GraphReference value;
        std::shared_ptr<const IReferenceResolver> resolver;
    };

    AssetId asset_{};

    std::vector<Instruction> instructions_;
    std::vector<Slot> slots_;
    std::vector<NativeSite> native_sites_;
    std::vector<ReferenceSite> references_;
    std::shared_ptr<const detail::FlowProgram> flow_;
    friend foundation::Result<CompiledGraphPtr> CompileGraph(const GraphDefinition &,
                                                             std::span<const NodeBinding>);
    friend class GraphInstance;
    friend class detail::FlowCompiler;
    friend class detail::FlowInstance;
};

} // namespace epidemic::runtime::graph
