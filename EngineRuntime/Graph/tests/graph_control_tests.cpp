#include "Epidemic/Runtime/Graph/graph_vm.h"
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
    if (!value) { std::fputs(message, stderr); std::fputc('\n', stderr); throw std::runtime_error(message); }
}
template <typename T> void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
    Check(!value && value.GetError().HasCode(code), "unexpected control graph error");
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
    Call(std::shared_ptr<Log> log, std::int64_t marker) : log_(std::move(log)), marker_(marker) {}
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
    Factory(std::shared_ptr<Log> log, std::int64_t marker) : log_(std::move(log)), marker_(marker) {}
    foundation::Result<std::unique_ptr<INativeInvocation>> CreateInvocation() const override {
        return foundation::Result<std::unique_ptr<INativeInvocation>>::Success(std::make_unique<Call>(log_, marker_));
    }
  private:
    std::shared_ptr<Log> log_;
    std::int64_t marker_;
};
struct Example {
    GraphDefinition graph{AssetId::FromString("control.example"), "Control", {}, {}};
    std::shared_ptr<Log> log = std::make_shared<Log>();
    std::vector<NodeBinding> bindings;
    Example() {
        using D = PortDirection;
        graph.nodes = {
            {Id("entry"), Id("entry.type"), "Entry", {Control("entry.next", D::Output)}},
            {Id("sequence"), Id("sequence.type"), "Sequence", {Control("sequence.in", D::Input), Control("sequence.first", D::Output), Control("sequence.second", D::Output)}},
            {Id("branch"), Id("branch.type"), "Branch", {Control("branch.in", D::Input), Data("branch.condition", D::Input), Control("branch.true", D::Output), Control("branch.false", D::Output)}},
            {Id("condition"), Id("input.type"), "Condition", {Data("condition.out", D::Output)}},
            {Id("value"), Id("input.type"), "Value", {Data("value.out", D::Output)}},
            {Id("double"), Id("add.type"), "Pure", {Data("double.a", D::Input), Data("double.b", D::Input), Data("double.out", D::Output)}}};
        for (const auto name : {"true", "false", "tail"}) {
            const std::string prefix{name};
            graph.nodes.push_back({Id(prefix), Id(prefix + ".type"), prefix,
                {Control(prefix + ".in", D::Input), Data(prefix + ".value", D::Input), Data(prefix + ".out", D::Output), Control(prefix + ".next", D::Output)}});
        }
        const auto edge = [&](std::string_view name, std::string_view from, std::string_view to) {
            graph.connections.push_back({Id(name), Id(from), Id(to)});
        };
        edge("c1", "entry.next", "sequence.in"); edge("c2", "sequence.first", "branch.in");
        edge("c3", "sequence.second", "tail.in"); edge("c4", "branch.true", "true.in"); edge("c5", "branch.false", "false.in");
        edge("d1", "condition.out", "branch.condition"); edge("d2", "value.out", "double.a"); edge("d3", "value.out", "double.b");
        edge("d4", "double.out", "true.value"); edge("d5", "value.out", "false.value"); edge("d6", "value.out", "tail.value");
        bindings = {{Id("entry.type"), Operation::Entry, {}}, {Id("sequence.type"), Operation::Sequence, {}},
                    {Id("branch.type"), Operation::Branch, {}}, {Id("input.type"), Operation::Input, {}}, {Id("add.type"), Operation::Add, {}},
                    {Id("true.type"), Operation::NativeCall, std::make_shared<Factory>(log, 10)},
                    {Id("false.type"), Operation::NativeCall, std::make_shared<Factory>(log, 20)},
                    {Id("tail.type"), Operation::NativeCall, std::make_shared<Factory>(log, 30)}};
    }
    std::unique_ptr<GraphInstance> Instance(std::int64_t condition, std::int64_t value) {
        auto result = GraphInstance::Create(CompileGraph(graph, bindings).Value()).Value();
        Check(result->SetInput(Id("condition.out"), condition).HasValue() && result->SetInput(Id("value.out"), value).HasValue(), "control inputs");
        return result;
    }
};
void ControlFlow() {
    Example example;
    auto first = example.Instance(1, 4);
    Check(first->Run().status == ExecutionStatus::Completed && *example.log == Log{{10, 8}, {30, 4}}, "Sequence/true Branch side-effect order");
    first->Cancel(); Check(first->Run().status == ExecutionStatus::Completed && example.log->size() == 2, "completed flow cannot replay");
    example.log->clear();
    auto second = example.Instance(0, std::numeric_limits<std::int64_t>::max());
    Check(second->Run().status == ExecutionStatus::Completed && *example.log == Log{{20, std::numeric_limits<std::int64_t>::max()}, {30, std::numeric_limits<std::int64_t>::max()}},
          "unchosen pure overflow and native effect do not execute");
    example.log->clear();
    auto bounded = example.Instance(1, 3);
    Check(bounded->Run({1000, 0}).status == ExecutionStatus::BudgetExhausted && example.log->empty(), "native budget before effect");
    ExecutionReport report;
    std::size_t total = 0;
    do { report = bounded->Run({1, 1}); total += report.instructions; Check(total < 100, "bounded progress"); }
    while (report.status == ExecutionStatus::BudgetExhausted);
    Check(report.status == ExecutionStatus::Completed && *example.log == Log{{10, 6}, {30, 3}}, "one-instruction resumptions do not replay effects");
    auto overflow = example.Instance(1, std::numeric_limits<std::int64_t>::max());
    const auto failed = overflow->Run();
    Check(failed.status == ExecutionStatus::Failed && failed.diagnostic && failed.diagnostic->node == Id("double") &&
          failed.diagnostic->error.HasCode("graph.vm.integer_overflow"), "pure overflow diagnosed only on chosen path");
    Example unavailable;
    unavailable.graph.connections.back().output_port = Id("true.out");
    auto consumer = unavailable.Instance(0, 4);
    const auto missing = consumer->Run();
    Check(missing.status == ExecutionStatus::Failed && missing.diagnostic && missing.diagnostic->error.HasCode("graph.flow.value_unavailable") &&
          *unavailable.log == Log{{20, 4}}, "data evaluation must never execute an unchosen effect to obtain its output");
}
void InvalidControl() {
    Example example;
    example.graph.connections.push_back({Id("bad.cycle"), Id("true.next"), Id("branch.in")});
    ErrorIs(CompileGraph(example.graph, example.bindings), "graph.flow.control_cycle");
    example = Example{};
    example.graph.connections.push_back({Id("bad.fanout"), Id("sequence.first"), Id("false.in")});
    ErrorIs(CompileGraph(example.graph, example.bindings), "graph.flow.control_fanout");
    example = Example{}; example.graph.connections.erase(example.graph.connections.begin() + 1);
    ErrorIs(CompileGraph(example.graph, example.bindings), "graph.flow.unreachable_control");
    example = Example{}; example.graph.connections[6].output_port = Id("double.out");
    ErrorIs(CompileGraph(example.graph, example.bindings), "graph.compile.data_cycle");
}
} // namespace
int main() {
    try { ControlFlow(); InvalidControl(); return 0; }
    catch (const std::exception &error) { std::fputs(error.what(), stderr); std::fputc('\n', stderr); return 1; }
}
