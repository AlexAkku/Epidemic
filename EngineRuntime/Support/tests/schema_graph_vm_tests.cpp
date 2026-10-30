#include "Epidemic/Runtime/Support/schema_graph_callable.h"
#include "Epidemic/Runtime/Support/graph_serialization.h"
#include "Epidemic/Runtime/Graph/graph_vm.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include <array>
#include <cstdio>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime;

namespace {
struct InputTag {};

struct OutputTag {};

struct Owner {
    std::int64_t bias = 0;
    std::int64_t calls = 0;
};

struct WrongOwner {};

foundation::Result<std::int64_t> Native(Owner &owner, const std::int64_t &value) {
    if (value < 0)
        return foundation::Result<std::int64_t>::Failure(
            foundation::Error::Create("native.rejected", "negative"));
    return foundation::Result<std::int64_t>::Success(value + owner.bias + ++owner.calls);
}

foundation::Result<bool> WrongSignature(Owner &, const std::int64_t &) {
    return foundation::Result<bool>::Success(true);
}

foundation::StringId Id(std::string_view value) {
    return foundation::StringId::FromString(value);
}

void Check(bool value, const char *message) {
    if (!value) {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        throw std::runtime_error(message);
    }

}

template <typename T> void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
    Check(!value && value.GetError().HasCode(code), "unexpected Schema graph error");
}

void Run() {
    auto registry = std::make_shared<schema::NativeSchemaRegistry>();
    const auto input_type = registry->RegisterType<InputTag>("node.input").Value();
    const auto owner_type = registry->RegisterType<Owner>("node.native").Value();
    const auto output_type = registry->RegisterType<OutputTag>("node.output").Value();
    const auto scalar_type = registry->RegisterType<std::int64_t>(graph::kI64TypeName).Value();
    Check(scalar_type == graph::kI64Type, "reuse native Schema scalar identity");
    const auto operation = registry->RegisterOperation(owner_type, "native", &Native).Value();
    const auto wrong = registry->RegisterOperation(owner_type, "wrong", &WrongSignature).Value();
    Owner initial{10, 0};

    auto factory = CreateSchemaGraphCallable(registry, {owner_type, operation}, initial).Value();
    Check(initial.calls == 0, "factory validation executes no callable");
    ErrorIs(CreateSchemaGraphCallable<Owner>({}, {owner_type, operation}, initial),
            "graph.native.invalid_registry");
    ErrorIs(CreateSchemaGraphCallable(registry, {owner_type, wrong}, initial),
            "schema.type_mismatch");
    ErrorIs(CreateSchemaGraphCallable(registry, {owner_type, Id("missing")}, initial),
            "schema.member_not_found");
    ErrorIs(CreateSchemaGraphCallable(registry, {owner_type, operation}, WrongOwner{}),
            "schema.type_mismatch");
    initial.bias = 1000; // Factory owns the earlier value snapshot.

    using graph::PortDirection;
    using graph::PortKind;
    graph::GraphDefinition definition{AssetId::FromString("native.graph"), "Native", {}, {}};

    definition.nodes =
        {{Id("input"),
          input_type,
          "Input",
          {{Id("input.out"), "Value", PortKind::Data, PortDirection::Output, scalar_type}}},
         {Id("call"),
          owner_type,
          "Call",
          {{Id("call.in"), "Value", PortKind::Data, PortDirection::Input, scalar_type},
           {Id("call.out"), "Result", PortKind::Data, PortDirection::Output, scalar_type}}},
         {Id("output"),
          output_type,
          "Output",
          {{Id("output.in"), "Value", PortKind::Data, PortDirection::Input, scalar_type}}}};

    definition.connections = {{Id("edge.1"), Id("input.out"), Id("call.in")},
                              {Id("edge.2"), Id("call.out"), Id("output.in")}};

    GraphDefinitionSerializer serializer{{input_type, owner_type, output_type, scalar_type}};

    auto serialization = CreateSerializationServices().Value();
    const auto document =
        SerializeToDocument(serializer, definition, *serialization.archives).Value();
    auto reader = serialization.archives->CreateReader(document).Value();
    graph::GraphDefinition loaded;
    Check(DeserializeObject(serializer, *reader, loaded).HasValue() && loaded == definition,
          "authored round-trip before compile");
    std::array bindings{graph::NodeBinding{input_type, graph::Operation::Input, {}},
                        graph::NodeBinding{owner_type, graph::Operation::NativeCall, factory},
                        graph::NodeBinding{output_type, graph::Operation::Output, {}}};

    const auto program = graph::CompileGraph(loaded, bindings).Value();
    factory.reset();
    for (auto &binding : bindings)
        binding.native.reset();
    registry.reset(); // Program is now the only owner of the factory/registry chain.
    auto first = graph::GraphInstance::Create(program).Value();
    auto second = graph::GraphInstance::Create(program).Value();
    Check(&first->Program() == &second->Program(), "two instances share one program");
    Check(first->SetInput(Id("input.out"), 3).HasValue() &&
              second->SetInput(Id("input.out"), 20).HasValue(),
          "separate inputs");
    Check(first->Run({1, 0}).status == graph::ExecutionStatus::BudgetExhausted,
          "input prefix budget");
    Check(first->Run({1, 1}).status == graph::ExecutionStatus::BudgetExhausted,
          "one native callback");
    Check(first->Run({1, 0}).status == graph::ExecutionStatus::Completed &&
              first->ReadValue(Id("output.in")).Value() == 14,
          "first independently cloned owner");
    Check(second->Run().status == graph::ExecutionStatus::Completed &&
              second->ReadValue(Id("output.in")).Value() == 31,
          "second independently cloned owner");
    Check(initial.calls == 0 && initial.bias == 1000, "initial owner is not instance state");
    auto rejected = graph::GraphInstance::Create(program).Value();
    Check(rejected->SetInput(Id("input.out"), -1).HasValue(), "negative native input");
    const auto report = rejected->Run();
    Check(report.status == graph::ExecutionStatus::Failed && report.diagnostic &&
              report.diagnostic->node == Id("call") &&
              report.diagnostic->error.HasCode("native.rejected"),
          "native Schema failure retains source diagnostic");
}

} // namespace

int main() {
    try {
        Run();
        return 0;
    } catch (...) {
        std::fputs("Schema Graph VM tests failed\n", stderr);
        return 1;
    }

}
