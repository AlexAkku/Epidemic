#include "Epidemic/Runtime/Graph/graph_vm.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime;
using namespace epidemic::runtime::graph;
namespace {
ElementId Id(std::string_view text) { return ElementId::FromString(text); }
void Check(bool value, const char *message) {
  if (!value) {
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
    throw std::runtime_error(message);
  }
}
template <typename T>
void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
  Check(!value && value.GetError().HasCode(code),
        "unexpected graph compiler/VM error");
}
PortDefinition Port(std::string_view name, PortDirection direction) {
  return {Id(name), std::string(name), PortKind::Data, direction, kI64Type};
}
GraphDefinition Example() {
  GraphDefinition value{
      AssetId::FromString("graph.vm.example"), "Example", {}, {}};
  value.nodes = {
      {Id("a"), Id("input"), "A", {Port("a.out", PortDirection::Output)}},
      {Id("b"), Id("input"), "B", {Port("b.out", PortDirection::Output)}},
      {Id("sum"),
       Id("add"),
       "Sum",
       {Port("sum.a", PortDirection::Input),
        Port("sum.b", PortDirection::Input),
        Port("sum.out", PortDirection::Output)}},
      {Id("product"),
       Id("mul"),
       "Product",
       {Port("product.a", PortDirection::Input),
        Port("product.b", PortDirection::Input),
        Port("product.out", PortDirection::Output)}},
      {Id("native"),
       Id("native.type"),
       "Native",
       {Port("native.in", PortDirection::Input),
        Port("native.out", PortDirection::Output)}},
      {Id("result"),
       Id("output"),
       "Result",
       {Port("result.in", PortDirection::Input)}}};
  value.connections = {{Id("e1"), Id("a.out"), Id("sum.a")},
                       {Id("e2"), Id("b.out"), Id("sum.b")},
                       {Id("e3"), Id("sum.out"), Id("product.a")},
                       {Id("e4"), Id("b.out"), Id("product.b")},
                       {Id("e5"), Id("product.out"), Id("native.in")},
                       {Id("e6"), Id("native.out"), Id("result.in")}};
  return value;
}
enum class Mode {
  Count,
  DomainFailure,
  Throw,
  Reenter,
  Cancel,
  FactoryThrow,
  NullState
};
class Invocation final : public INativeInvocation {
public:
  Invocation(Mode mode, GraphInstance **instance)
      : mode_(mode), instance_(instance) {}
  foundation::Result<std::int64_t> Invoke(std::int64_t argument) override {
    if (mode_ == Mode::Throw)
      throw std::runtime_error("native failure");
    if (mode_ == Mode::DomainFailure)
      return foundation::Result<std::int64_t>::Failure(
          foundation::Error::Create("test.native.rejected", "rejected"));
    if (mode_ == Mode::Reenter)
      (void)(*instance_)->Run();
    if (mode_ == Mode::Cancel)
      (*instance_)->Cancel();
    return foundation::Result<std::int64_t>::Success(argument + ++calls_);
  }

private:
  Mode mode_;
  GraphInstance **instance_;
  std::int64_t calls_ = 0;
};
class Factory final : public INativeCallableFactory {
public:
  explicit Factory(Mode mode = Mode::Count, GraphInstance **instance = nullptr)
      : mode_(mode), instance_(instance) {}
  foundation::Result<std::unique_ptr<INativeInvocation>>
  CreateInvocation() const override {
    if (mode_ == Mode::FactoryThrow)
      throw std::runtime_error("factory failure");
    if (mode_ == Mode::NullState)
      return foundation::Result<std::unique_ptr<INativeInvocation>>::Success(
          nullptr);
    return foundation::Result<std::unique_ptr<INativeInvocation>>::Success(
        std::make_unique<Invocation>(mode_, instance_));
  }

private:
  Mode mode_;
  GraphInstance **instance_;
};
std::vector<NodeBinding> Bindings(std::shared_ptr<const INativeCallableFactory>
                                      factory = std::make_shared<Factory>()) {
  return {{Id("input"), Operation::Input, {}},
          {Id("add"), Operation::Add, {}},
          {Id("mul"), Operation::Multiply, {}},
          {Id("native.type"), Operation::NativeCall, std::move(factory)},
          {Id("output"), Operation::Output, {}}};
}
void Inputs(GraphInstance &instance, std::array<std::int64_t, 2> values) {
  Check(instance.SetInput(Id("a.out"), values[0]).HasValue(), "input A");
  Check(instance.SetInput(Id("b.out"), values[1]).HasValue(), "input B");
}
void CompileFailures() {
  auto value = Example();
  auto bindings = Bindings();
  bindings.pop_back();
  const auto unbound = CompileGraph(value, bindings);
  ErrorIs(unbound, "graph.compile.unbound_node");
  Check(!unbound.GetError().context.empty(), "compile diagnostic node context");
  bindings = Bindings();
  bindings.push_back(bindings[0]);
  ErrorIs(CompileGraph(value, bindings), "graph.compile.invalid_binding");
  bindings = Bindings();
  bindings[3].native.reset();
  ErrorIs(CompileGraph(value, bindings), "graph.compile.invalid_binding");
  value.connections.pop_back();
  ErrorIs(CompileGraph(value, Bindings()), "graph.compile.unconnected_input");
  value = Example();
  value.nodes[2].ports.pop_back();
  value.connections.erase(value.connections.begin() + 2);
  ErrorIs(CompileGraph(value, Bindings()), "graph.compile.node_shape");
  value = Example();
  for (auto &node : value.nodes)
    for (auto &port : node.ports) {
      port.kind = PortKind::Control;
      port.type_id = {};
    }
  ErrorIs(CompileGraph(value, Bindings()), "graph.compile.unsupported_port");
  value = Example();
  value.connections[0].output_port = Id("sum.out");
  ErrorIs(CompileGraph(value, Bindings()), "graph.compile.data_cycle");
  value = Example();
  std::reverse(value.nodes.begin(), value.nodes.end());
  const auto first = CompileGraph(value, Bindings()).Value();
  std::reverse(value.nodes.begin(), value.nodes.end());
  const auto second = CompileGraph(value, Bindings()).Value();
  for (std::size_t index = 0; index < first->Instructions().size(); ++index)
    Check(first->Instructions()[index].node ==
              second->Instructions()[index].node,
          "deterministic topological order");
}
void InstancesAndBudgets() {
  auto definition = Example();
  const auto program = CompileGraph(definition, Bindings()).Value();
  auto first = GraphInstance::Create(program).Value();
  auto second = GraphInstance::Create(program).Value();
  Check(&first->Program() == &second->Program() &&
            program->RegisterCount() == 6,
        "shared immutable IR");
  definition.nodes.clear();
  definition.connections.clear(); // Program owns its compiled snapshot.
  Inputs(*first, {2, 3});
  Inputs(*second, {10, 4});
  ErrorIs(first->SetInput(Id("sum.out"), 1), "graph.vm.unknown_input");
  ErrorIs(first->ReadValue(Id("result.in")), "graph.vm.value_unavailable");
  const auto zero = first->Run({0, 0});
  Check(zero.status == ExecutionStatus::BudgetExhausted &&
            zero.instructions == 0 && first->InstructionPointer() == 0,
        "zero budget makes no progress");
  ErrorIs(first->SetInput(Id("a.out"), 7), "graph.vm.inputs_sealed");
  const auto yielded = first->Run({100, 0});
  Check(yielded.status == ExecutionStatus::BudgetExhausted &&
            yielded.instructions == 4 && yielded.native_calls == 0 &&
            yielded.diagnostic && yielded.diagnostic->node == Id("native"),
        "native budget yields before callback");
  const auto native = first->Run({1, 1});
  Check(native.status == ExecutionStatus::BudgetExhausted &&
            native.native_calls == 1 &&
            first->ReadValue(Id("native.out")).Value() == 16,
        "native state advances once");
  ErrorIs(first->ReadValue(Id("result.in")), "graph.vm.value_unavailable");
  Check(first->Run({1, 0}).status == ExecutionStatus::Completed &&
            first->ReadValue(Id("result.in")).Value() == 16,
        "resume executes only remaining output");
  Check(first->Run().instructions == 0 &&
            first->ReadValue(Id("result.in")).Value() == 16,
        "completed instance never replays");
  Check(second->Run().status == ExecutionStatus::Completed &&
            second->ReadValue(Id("result.in")).Value() == 57,
        "second instance has independent registers and native counter");
}
void Failures() {
  ErrorIs(GraphInstance::Create({}), "graph.vm.invalid_program");
  const auto program = CompileGraph(Example(), Bindings()).Value();
  auto missing = GraphInstance::Create(program).Value();
  const auto failure = missing->Run();
  Check(failure.status == ExecutionStatus::Failed && failure.diagnostic &&
            failure.diagnostic->error.HasCode("graph.vm.missing_input") &&
            failure.diagnostic->asset == program->Asset(),
        "missing input source diagnostic");
  Check(missing->Run().instructions == 0, "failed instance never retries");
  for (const auto mode : {Mode::DomainFailure, Mode::Throw}) {
    auto instance =
        GraphInstance::Create(
            CompileGraph(Example(), Bindings(std::make_shared<Factory>(mode)))
                .Value())
            .Value();
    Inputs(*instance, {2, 3});
    const auto report = instance->Run();
    Check(report.status == ExecutionStatus::Failed &&
              report.native_calls == 1 && report.diagnostic &&
              report.diagnostic->node == Id("native") &&
              report.diagnostic->instruction == 4 &&
              report.diagnostic->error.HasCode(mode == Mode::Throw
                                                   ? "graph.vm.native_exception"
                                                   : "test.native.rejected"),
          "native failure retains source attribution and error code");
    Check(instance->Run().native_calls == 0, "native failure is terminal");
  }
  for (const auto mode : {Mode::FactoryThrow, Mode::NullState}) {
    const auto result = GraphInstance::Create(
        CompileGraph(Example(), Bindings(std::make_shared<Factory>(mode)))
            .Value());
    ErrorIs(result, mode == Mode::FactoryThrow
                        ? "graph.vm.factory_exception"
                        : "graph.vm.invalid_native_state");
    Check(!result.GetError().context.empty(),
          "factory diagnostic includes call site");
  }
  GraphInstance *active = nullptr;
  auto reentrant =
      GraphInstance::Create(
          CompileGraph(Example(), Bindings(std::make_shared<Factory>(
                                      Mode::Reenter, &active)))
              .Value())
          .Value();
  active = reentrant.get();
  Inputs(*reentrant, {2, 3});
  const auto rejected = reentrant->Run();
  Check(rejected.status == ExecutionStatus::Failed && rejected.diagnostic &&
            rejected.diagnostic->error.HasCode("graph.vm.reentrant"),
        "reentrant execution rejected");
}
void MultipleCallSites() {
  auto definition = Example();
  definition.nodes.push_back({Id("native.second"),
                              Id("native.type"),
                              "Second call",
                              {Port("second.in", PortDirection::Input),
                               Port("second.out", PortDirection::Output)}});
  definition.connections.back().input_port = Id("second.in");
  definition.connections.push_back(
      {Id("edge.second.call"), Id("second.out"), Id("result.in")});
  auto instance =
      GraphInstance::Create(CompileGraph(definition, Bindings()).Value())
          .Value();
  Inputs(*instance, {2, 3});
  const auto first = instance->Run({100, 1});
  Check(first.status == ExecutionStatus::BudgetExhausted &&
            first.native_calls == 1 && first.diagnostic &&
            first.diagnostic->node == Id("native.second"),
        "one-call budget stops before second call site");
  const auto second = instance->Run({100, 1});
  Check(second.status == ExecutionStatus::Completed &&
            second.native_calls == 1 &&
            instance->ReadValue(Id("result.in")).Value() == 17,
        "per-call-site state and nonreplaying resume");
}
void Arithmetic() {
  constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
  constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
  for (const auto operation : {Operation::Add, Operation::Multiply}) {
    auto value = Example();
    value.nodes.erase(value.nodes.begin() + 3, value.nodes.begin() + 5);
    value.nodes[2].type_id = Id("arithmetic");
    value.connections = {{Id("e1"), Id("a.out"), Id("sum.a")},
                         {Id("e2"), Id("b.out"), Id("sum.b")},
                         {Id("e3"), Id("sum.out"), Id("result.in")}};
    const std::array bindings{NodeBinding{Id("input"), Operation::Input, {}},
                              NodeBinding{Id("arithmetic"), operation, {}},
                              NodeBinding{Id("output"), Operation::Output, {}}};
    const auto program = CompileGraph(value, bindings).Value();
    const std::array<std::array<std::int64_t, 2>, 5> invalid =
        operation == Operation::Add
            ? std::array<std::array<std::int64_t, 2>, 5>{{{maximum, 1},
                                                          {minimum, -1},
                                                          {maximum, maximum},
                                                          {minimum, minimum},
                                                          {1, maximum}}}
            : std::array<std::array<std::int64_t, 2>, 5>{{{maximum, 2},
                                                          {minimum, -1},
                                                          {-1, minimum},
                                                          {minimum, 2},
                                                          {minimum, minimum}}};
    for (const auto pair : invalid) {
      auto instance = GraphInstance::Create(program).Value();
      Inputs(*instance, pair);
      const auto report = instance->Run();
      Check(report.status == ExecutionStatus::Failed && report.diagnostic &&
                report.diagnostic->error.HasCode("graph.vm.integer_overflow"),
            "checked i64 overflow");
    }
    auto valid = GraphInstance::Create(program).Value();
    Inputs(*valid, operation == Operation::Add
                       ? std::array<std::int64_t, 2>{maximum, -1}
                       : std::array<std::int64_t, 2>{minimum, 1});
    Check(valid->Run().status == ExecutionStatus::Completed &&
              valid->ReadValue(Id("result.in")).Value() ==
                  (operation == Operation::Add ? maximum - 1 : minimum),
          "i64 boundary result");
  }
}
void WaitAndCancellation() {
  auto bindings = Bindings();
  bindings[2].operation = Operation::Wait;
  const auto program = CompileGraph(Example(), bindings).Value();
  auto instance = GraphInstance::Create(program).Value();
  Inputs(*instance, {2, 3});
  ErrorIs(instance->ResumeWait(), "graph.vm.invalid_wait_resume");
  const auto wait = instance->Run();
  Check(wait.status == ExecutionStatus::Waiting &&
            wait.wait == GameDuration{3} && wait.diagnostic &&
            wait.diagnostic->node == Id("product") && wait.native_calls == 0,
        "Wait yields before downstream effects with authoring diagnostic");
  const auto pointer = instance->InstructionPointer();
  const auto blocked = instance->Run();
  Check(blocked.status == ExecutionStatus::Waiting &&
            blocked.instructions == 0 &&
            instance->InstructionPointer() == pointer,
        "Run cannot bypass Wait");
  ErrorIs(instance->ReadValue(Id("product.out")), "graph.vm.value_unavailable");
  Check(instance->ResumeWait().HasValue(), "scheduler acknowledges Wait");
  Check(instance->Run().status == ExecutionStatus::Completed &&
            instance->ReadValue(Id("result.in")).Value() == 6,
        "resume preserves registers and invokes native exactly once");
  instance->Cancel();
  Check(instance->Run().status == ExecutionStatus::Completed,
        "completed invocation stays completed");

  auto negative = GraphInstance::Create(program).Value();
  Inputs(*negative, {4, -1});
  const auto rejected = negative->Run();
  Check(rejected.status == ExecutionStatus::Failed && rejected.diagnostic &&
            rejected.diagnostic->error.HasCode("graph.vm.negative_wait"),
        "negative Wait rejected");
  auto zero = GraphInstance::Create(program).Value();
  Inputs(*zero, {4, 0});
  Check(zero->Run().status == ExecutionStatus::Waiting &&
            zero->ResumeWait().HasValue() &&
            zero->Run().status == ExecutionStatus::Completed,
        "zero Wait still yields once");

  auto cancelled = GraphInstance::Create(program).Value();
  Inputs(*cancelled, {2, 3});
  Check(cancelled->Run().status == ExecutionStatus::Waiting,
        "cancel waiting fixture");
  cancelled->Cancel();
  const auto cancellation = cancelled->Run();
  Check(cancellation.status == ExecutionStatus::Cancelled &&
            cancellation.instructions == 0 && cancellation.diagnostic &&
            cancellation.diagnostic->node == Id("product"),
        "cancel pending Wait");
  ErrorIs(cancelled->ResumeWait(), "graph.vm.invalid_wait_resume");
  Check(cancelled->Run().status == ExecutionStatus::Cancelled,
        "cancel is terminal");
  auto unstarted = GraphInstance::Create(program).Value();
  unstarted->Cancel();
  Check(unstarted->Run().status == ExecutionStatus::Cancelled,
        "cancel before first run");
  ErrorIs(unstarted->SetInput(Id("a.out"), 1), "graph.vm.inputs_sealed");

  auto last_wait_definition = Example();
  last_wait_definition.nodes.resize(4);
  last_wait_definition.connections.resize(4);
  auto last_wait = GraphInstance::Create(
                       CompileGraph(last_wait_definition, bindings).Value())
                       .Value();
  Inputs(*last_wait, {2, 3});
  Check(last_wait->Run().status == ExecutionStatus::Waiting,
        "final instruction can wait");
  last_wait->Cancel();
  Check(last_wait->Run().status == ExecutionStatus::Cancelled,
        "cancel Wait at end of IR");

  GraphInstance *raw = nullptr;
  auto self =
      GraphInstance::Create(
          CompileGraph(Example(),
                       Bindings(std::make_shared<Factory>(Mode::Cancel, &raw)))
              .Value())
          .Value();
  raw = self.get();
  Inputs(*self, {2, 3});
  const auto self_cancelled = self->Run();
  Check(self_cancelled.status == ExecutionStatus::Cancelled &&
            self_cancelled.native_calls == 1 && self->Run().native_calls == 0,
        "native cancellation stops suffix without replay");
  ErrorIs(self->ReadValue(Id("native.out")), "graph.vm.value_unavailable");
}
} // namespace
int main() {
  try {
    CompileFailures();
    InstancesAndBudgets();
    Failures();
    MultipleCallSites();
    Arithmetic();
    WaitAndCancellation();
    return 0;
  } catch (...) {
    std::fputs("Graph Compiler/VM tests failed\n", stderr);
    return 1;
  }
}
