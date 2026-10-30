#include "Epidemic/Runtime/Graph/graph_vm.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <stdexcept>

using namespace epidemic;
using epidemic::runtime::AssetId;
using namespace epidemic::runtime::graph;
namespace {
ElementId Id(std::string_view name) { return ElementId::FromString(name); }
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
        "unexpected control graph error");
}
PortDefinition Data(std::string_view name, PortDirection direction) {
  return {Id(name), std::string(name), PortKind::Data, direction, kI64Type};
}
PortDefinition Control(std::string_view name, PortDirection direction) {
  return {Id(name), std::string(name), PortKind::Control, direction, {}};
}
using Log = std::vector<std::pair<std::int64_t, std::int64_t>>;
class Call final : public INativeInvocation {
public:
  Call(std::shared_ptr<Log> log, std::int64_t marker)
      : log_(std::move(log)), marker_(marker) {}
  foundation::Result<std::int64_t> Invoke(std::int64_t value) override {
    log_->emplace_back(marker_, value);
    return foundation::Result<std::int64_t>::Success(value);
  }

private:
  std::shared_ptr<Log> log_;
  std::int64_t marker_;
};
class Factory final : public INativeCallableFactory {
public:
  Factory(std::shared_ptr<Log> log, std::int64_t marker)
      : log_(std::move(log)), marker_(marker) {}
  foundation::Result<std::unique_ptr<INativeInvocation>>
  CreateInvocation() const override {
    return foundation::Result<std::unique_ptr<INativeInvocation>>::Success(
        std::make_unique<Call>(log_, marker_));
  }

private:
  std::shared_ptr<Log> log_;
  std::int64_t marker_;
};
struct Example {
  GraphDefinition graph{
      AssetId::FromString("control.example"), "Control", {}, {}};
  std::shared_ptr<Log> log = std::make_shared<Log>();
  std::vector<NodeBinding> bindings;
  Example() {
    using D = PortDirection;
    graph.nodes = {
        {Id("entry"),
         Id("entry.type"),
         "Entry",
         {Control("entry.next", D::Output)}},
        {Id("sequence"),
         Id("sequence.type"),
         "Sequence",
         {Control("sequence.in", D::Input),
          Control("sequence.first", D::Output),
          Control("sequence.second", D::Output)}},
        {Id("branch"),
         Id("branch.type"),
         "Branch",
         {Control("branch.in", D::Input), Data("branch.condition", D::Input),
          Control("branch.true", D::Output),
          Control("branch.false", D::Output)}},
        {Id("condition"),
         Id("input.type"),
         "Condition",
         {Data("condition.out", D::Output)}},
        {Id("value"),
         Id("input.type"),
         "Value",
         {Data("value.out", D::Output)}},
        {Id("double"),
         Id("add.type"),
         "Pure",
         {Data("double.a", D::Input), Data("double.b", D::Input),
          Data("double.out", D::Output)}}};
    for (const auto name : {"true", "false", "tail"}) {
      const std::string prefix{name};
      graph.nodes.push_back(
          {Id(prefix),
           Id(prefix + ".type"),
           prefix,
           {Control(prefix + ".in", D::Input),
            Data(prefix + ".value", D::Input), Data(prefix + ".out", D::Output),
            Control(prefix + ".next", D::Output)}});
    }
    const auto edge = [&](std::string_view name, std::string_view from,
                          std::string_view to) {
      graph.connections.push_back({Id(name), Id(from), Id(to)});
    };
    edge("c1", "entry.next", "sequence.in");
    edge("c2", "sequence.first", "branch.in");
    edge("c3", "sequence.second", "tail.in");
    edge("c4", "branch.true", "true.in");
    edge("c5", "branch.false", "false.in");
    edge("d1", "condition.out", "branch.condition");
    edge("d2", "value.out", "double.a");
    edge("d3", "value.out", "double.b");
    edge("d4", "double.out", "true.value");
    edge("d5", "value.out", "false.value");
    edge("d6", "value.out", "tail.value");
    bindings = {{Id("entry.type"), Operation::Entry, {}},
                {Id("sequence.type"), Operation::Sequence, {}},
                {Id("branch.type"), Operation::Branch, {}},
                {Id("input.type"), Operation::Input, {}},
                {Id("add.type"), Operation::Add, {}},
                {Id("true.type"), Operation::NativeCall,
                 std::make_shared<Factory>(log, 10)},
                {Id("false.type"), Operation::NativeCall,
                 std::make_shared<Factory>(log, 20)},
                {Id("tail.type"), Operation::NativeCall,
                 std::make_shared<Factory>(log, 30)}};
  }
  std::unique_ptr<GraphInstance> Instance(std::int64_t condition,
                                          std::int64_t value) {
    auto result =
        GraphInstance::Create(CompileGraph(graph, bindings).Value()).Value();
    Check(result->SetInput(Id("condition.out"), condition).HasValue() &&
              result->SetInput(Id("value.out"), value).HasValue(),
          "control inputs");
    return result;
  }
};
void ControlFlow() {
  Example example;
  auto first = example.Instance(1, 4);
  Check(first->Run().status == ExecutionStatus::Completed &&
            *example.log == Log{{10, 8}, {30, 4}},
        "Sequence/true Branch side-effect order");
  first->Cancel();
  Check(first->Run().status == ExecutionStatus::Completed &&
            example.log->size() == 2,
        "completed flow cannot replay");
  example.log->clear();
  auto second = example.Instance(0, std::numeric_limits<std::int64_t>::max());
  Check(second->Run().status == ExecutionStatus::Completed &&
            *example.log == Log{{20, std::numeric_limits<std::int64_t>::max()},
                                {30, std::numeric_limits<std::int64_t>::max()}},
        "unchosen pure overflow and native effect do not execute");
  example.log->clear();
  auto bounded = example.Instance(1, 3);
  Check(bounded->Run({1000, 0}).status == ExecutionStatus::BudgetExhausted &&
            example.log->empty(),
        "native budget before effect");
  ExecutionReport report;
  std::size_t total = 0;
  do {
    report = bounded->Run({1, 1});
    total += report.instructions;
    Check(total < 100, "bounded progress");
  } while (report.status == ExecutionStatus::BudgetExhausted);
  Check(report.status == ExecutionStatus::Completed &&
            *example.log == Log{{10, 6}, {30, 3}},
        "one-instruction resumptions do not replay effects");
  auto overflow = example.Instance(1, std::numeric_limits<std::int64_t>::max());
  const auto failed = overflow->Run();
  Check(failed.status == ExecutionStatus::Failed && failed.diagnostic &&
            failed.diagnostic->node == Id("double") &&
            failed.diagnostic->error.HasCode("graph.vm.integer_overflow"),
        "pure overflow diagnosed only on chosen path");
  Example unavailable;
  unavailable.graph.connections.back().output_port = Id("true.out");
  auto consumer = unavailable.Instance(0, 4);
  const auto missing = consumer->Run();
  Check(missing.status == ExecutionStatus::Failed && missing.diagnostic &&
            missing.diagnostic->error.HasCode("graph.flow.value_unavailable") &&
            *unavailable.log == Log{{20, 4}},
        "data evaluation must never execute an unchosen effect to obtain its "
        "output");
}
void InvalidControl() {
  Example example;
  example.graph.connections.push_back(
      {Id("bad.cycle"), Id("true.next"), Id("branch.in")});
  ErrorIs(CompileGraph(example.graph, example.bindings),
          "graph.flow.control_cycle");
  example = Example{};
  example.graph.connections.push_back(
      {Id("bad.fanout"), Id("sequence.first"), Id("false.in")});
  ErrorIs(CompileGraph(example.graph, example.bindings),
          "graph.flow.control_fanout");
  example = Example{};
  example.graph.connections.erase(example.graph.connections.begin() + 1);
  ErrorIs(CompileGraph(example.graph, example.bindings),
          "graph.flow.unreachable_control");
  example = Example{};
  example.graph.connections[6].output_port = Id("double.out");
  ErrorIs(CompileGraph(example.graph, example.bindings),
          "graph.compile.data_cycle");
}
Example EventExample() {
  Example example;
  using D = PortDirection;
  example.bindings[0].operation = Operation::EventEntry;
  example.graph.nodes[0].ports = {
      {Id("event.in"), "Event", PortKind::Event, D::Input, kI64Type},
      Data("event.payload", D::Output),
      Control("entry.next", D::Output)};
  example.graph.nodes.push_back(
      {Id("repeat"),
       Id("repeat.type"),
       "Repeat",
       {Control("repeat.in", D::Input), Data("repeat.count", D::Input),
        Data("repeat.index", D::Output), Control("repeat.body", D::Output),
        Control("repeat.done", D::Output)}});
  example.bindings.push_back({Id("repeat.type"), Operation::Repeat, {}});
  example.graph.connections[3].input_port = Id("repeat.in");
  example.graph.connections[5].output_port = Id("event.payload");
  example.graph.connections[6].output_port = Id("repeat.index");
  example.graph.connections[7].output_port = Id("repeat.index");
  example.graph.connections[9].output_port = Id("event.payload");
  example.graph.connections[10].output_port = Id("event.payload");
  example.graph.connections.push_back(
      {Id("repeat.body.edge"), Id("repeat.body"), Id("true.in")});
  example.graph.connections.push_back(
      {Id("repeat.count.edge"), Id("event.payload"), Id("repeat.count")});
  return example;
}
void EventsAndLoops() {
  auto example = EventExample();
  const auto program = CompileGraph(example.graph, example.bindings).Value();
  ErrorIs(GraphInstance::Create(program), "graph.flow.event_required");
  ErrorIs(GraphInstance::CreateForEvent(program, Id("unknown.event"), 1),
          "graph.flow.unknown_event");
  auto first =
      GraphInstance::CreateForEvent(program, Id("event.in"), 3).Value();
  auto second =
      GraphInstance::CreateForEvent(program, Id("event.in"), 2).Value();
  Check(&first->Program() == &second->Program(),
        "event activations share immutable program");
  ErrorIs(first->SetInput(Id("event.payload"), 7), "graph.vm.unknown_input");
  const auto yielded = first->Run({1000, 1});
  Check(yielded.status == ExecutionStatus::BudgetExhausted &&
            yielded.native_calls == 1 &&
            first->ReadValue(Id("repeat.index")).Value() == 1,
        "Repeat resumes at next effect with its own loop variable");
  Check(second->Run().status == ExecutionStatus::Completed &&
            first->Run().status == ExecutionStatus::Completed &&
            *example.log == Log{{10, 0},
                                {10, 0},
                                {10, 2},
                                {30, 2},
                                {10, 2},
                                {10, 4},
                                {30, 3}},
        "independent event payloads, cursors, loop variables and "
        "pure-expression caches");
  ErrorIs(first->ReadValue(Id("repeat.index")), "graph.vm.value_unavailable");
  Check(first->ReadValue(Id("tail.out")).Value() == 3 &&
            second->ReadValue(Id("tail.out")).Value() == 2,
        "independent event outputs");
  example.log->clear();
  auto zero = GraphInstance::CreateForEvent(program, Id("event.in"), 0).Value();
  Check(zero->Run().status == ExecutionStatus::Completed &&
            *example.log == Log{{20, 0}, {30, 0}},
        "zero event chooses fallback without entering Repeat");
  for (const auto count :
       {-1LL, static_cast<long long>(kMaxRepeatIterations + 1)}) {
    auto bad =
        GraphInstance::CreateForEvent(program, Id("event.in"), count).Value();
    const auto failure = bad->Run();
    Check(failure.status == ExecutionStatus::Failed && failure.diagnostic &&
              failure.diagnostic->node == Id("repeat") &&
              failure.diagnostic->error.HasCode("graph.flow.repeat_count"),
          "Repeat count bounded and diagnosed");
  }
  using D = PortDirection;
  example.graph.nodes.push_back(
      {Id("other"),
       Id("entry.type"),
       "Other event",
       {{Id("other.event"), "Other", PortKind::Event, D::Input, kI64Type},
        Data("other.payload", D::Output),
        Control("other.next", D::Output)}});
  auto other = GraphInstance::CreateForEvent(
                   CompileGraph(example.graph, example.bindings).Value(),
                   Id("other.event"), 9)
                   .Value();
  Check(other->Run().status == ExecutionStatus::Completed &&
            other->ReadValue(Id("other.payload")).Value() == 9,
        "select event by stable port ID");

  auto nested = EventExample();
  nested.graph.nodes.push_back(
      {Id("inner"),
       Id("repeat.type"),
       "Inner",
       {Control("inner.in", D::Input), Data("inner.count", D::Input),
        Data("inner.index", D::Output), Control("inner.body", D::Output),
        Control("inner.done", D::Output)}});
  nested.graph.connections[11].input_port = Id("inner.in");
  nested.graph.connections[7].output_port = Id("inner.index");
  nested.graph.connections.push_back(
      {Id("inner.count.edge"), Id("event.payload"), Id("inner.count")});
  nested.graph.connections.push_back(
      {Id("inner.body.edge"), Id("inner.body"), Id("true.in")});
  auto activation = GraphInstance::CreateForEvent(
                        CompileGraph(nested.graph, nested.bindings).Value(),
                        Id("event.in"), 3)
                        .Value();
  ExecutionReport report;
  std::size_t total = 0;
  do {
    report = activation->Run({4, 1});
    total += report.instructions;
    Check(total < 1000, "nested loop makes bounded progress");
  } while (report.status == ExecutionStatus::BudgetExhausted);
  Check(report.status == ExecutionStatus::Completed &&
            *nested.log == Log{{10, 0},
                               {10, 1},
                               {10, 2},
                               {10, 1},
                               {10, 2},
                               {10, 3},
                               {10, 2},
                               {10, 3},
                               {10, 4},
                               {30, 3}},
        "nested loop variables and ordered continuation stack");
  auto empty = EventExample();
  std::erase_if(empty.graph.nodes,
                [](const auto &node) { return node.id == Id("true"); });
  std::erase_if(empty.graph.connections, [](const auto &edge) {
    return edge.input_port == Id("true.in") ||
           edge.input_port == Id("true.value");
  });
  auto large = GraphInstance::CreateForEvent(
                   CompileGraph(empty.graph, empty.bindings).Value(),
                   Id("event.in"), kMaxRepeatIterations)
                   .Value();
  const auto empty_yield = large->Run({100, 0});
  Check(empty_yield.status == ExecutionStatus::BudgetExhausted &&
            empty_yield.instructions == 100 &&
            large->ReadValue(Id("repeat.index")).Value() > 1 &&
            empty.log->empty(),
        "empty large Repeat cannot escape Run budget");
  large->Cancel();
  Check(large->Run().status == ExecutionStatus::Cancelled,
        "bounded activation can cancel");
}
void IterationValueScope() {
  auto example = EventExample();
  using D = PortDirection;
  example.graph.nodes.push_back(
      {Id("body.sequence"),
       Id("sequence.type"),
       "Body",
       {Control("body.in", D::Input), Control("body.first", D::Output),
        Control("body.second", D::Output)}});
  example.graph.nodes.push_back(
      {Id("body.branch"),
       Id("branch.type"),
       "Body branch",
       {Control("body.branch.in", D::Input), Data("body.condition", D::Input),
        Control("body.true", D::Output), Control("body.false", D::Output)}});
  example.graph.nodes.push_back({Id("minus"),
                                 Id("input.type"),
                                 "Minus one",
                                 {Data("minus.out", D::Output)}});
  example.graph.nodes.push_back(
      {Id("condition.expr"),
       Id("add.type"),
       "Index minus one",
       {Data("expr.a", D::Input), Data("expr.b", D::Input),
        Data("expr.out", D::Output)}});
  example.graph.nodes.push_back(
      {Id("consumer"),
       Id("tail.type"),
       "Consumer",
       {Control("consumer.in", D::Input), Data("consumer.value", D::Input),
        Data("consumer.out", D::Output), Control("consumer.next", D::Output)}});
  example.graph.connections[11].input_port = Id("body.in");
  const auto edge = [&](std::string_view name, std::string_view source,
                        std::string_view target) {
    example.graph.connections.push_back({Id(name), Id(source), Id(target)});
  };
  edge("body.c1", "body.first", "body.branch.in");
  edge("body.c2", "body.second", "consumer.in");
  edge("body.c3", "body.true", "true.in");
  edge("body.d1", "repeat.index", "expr.a");
  edge("body.d2", "minus.out", "expr.b");
  edge("body.d3", "expr.out", "body.condition");
  edge("body.d4", "true.out", "consumer.value");
  auto activation = GraphInstance::CreateForEvent(
                        CompileGraph(example.graph, example.bindings).Value(),
                        Id("event.in"), 2)
                        .Value();
  Check(activation->SetInput(Id("minus.out"), -1).HasValue(),
        "loop condition constant");
  const auto failure = activation->Run();
  Check(failure.status == ExecutionStatus::Failed && failure.diagnostic &&
            failure.diagnostic->error.HasCode("graph.flow.value_unavailable") &&
            *example.log == Log{{10, 0}, {30, 0}},
        "skipped action cannot leak its previous iteration value");
  auto zero = EventExample();
  zero.graph.connections[12].output_port = Id("value.out");
  auto skipped =
      GraphInstance::CreateForEvent(
          CompileGraph(zero.graph, zero.bindings).Value(), Id("event.in"), 1)
          .Value();
  Check(skipped->SetInput(Id("value.out"), 0).HasValue() &&
            skipped->Run().status == ExecutionStatus::Completed &&
            *zero.log == Log{{30, 1}},
        "zero Repeat runs done continuation without body");
}
} // namespace
int main() {
  try {
    ControlFlow();
    InvalidControl();
    EventsAndLoops();
    IterationValueScope();
    return 0;
  } catch (const std::exception &error) {
    std::fputs(error.what(), stderr);
    std::fputc('\n', stderr);
    return 1;
  }
}
