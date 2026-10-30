#include "Epidemic/Runtime/Graph/graph_compiler.h"
#include "graph_flow.h"

#include <array>
#include <queue>
#include <unordered_map>

namespace epidemic::runtime::graph {
namespace {
struct Location { ElementId node{}; ElementId port{}; };
foundation::Result<CompiledGraphPtr> Reject(std::string_view code, Location location = {}) {
    return foundation::Result<CompiledGraphPtr>::Failure(foundation::Error::Create(
        code, "i64 graph compilation rejected", "node=" + std::to_string(location.node.Raw()) + ";port=" + std::to_string(location.port.Raw())));
}
struct Shape { std::size_t inputs; std::size_t outputs; };
Shape Expected(Operation operation) {
    switch (operation) {
    case Operation::Input: return {0, 1};
    case Operation::Add:
    case Operation::Wait:
    case Operation::Multiply: return {2, 1};
    case Operation::NativeCall: return {1, 1};
    case Operation::Output: return {1, 0};
    case Operation::Entry:
    case Operation::Sequence:
    case Operation::EventEntry:
    case Operation::Repeat:
    case Operation::Branch: return {0, 0}; // Unused catalog bindings are valid in a data-only graph.
    }
    return {3, 3}; // Invalid binding; no supported node can have this shape.
}
struct NodeInfo {
    const NodeDefinition *definition = nullptr;
    const NodeBinding *binding = nullptr;
    std::array<ElementId, 2> inputs{};
    ElementId exposed_port{};
};
} // namespace

foundation::Result<CompiledGraphPtr> CompileGraph(const GraphDefinition &definition, std::span<const NodeBinding> bindings) {
    if (detail::FlowCompiler::UsesFlow(definition, bindings)) return detail::FlowCompiler::Compile(definition, bindings);
    std::unordered_map<TypeId, const NodeBinding *> catalog;
    std::vector<TypeId> types{kI64Type};
    for (const auto &binding : bindings) {
        if (!binding.node_type.IsValid() || !catalog.emplace(binding.node_type, &binding).second)
            return Reject("graph.compile.invalid_binding");
        const auto shape = Expected(binding.operation);
        if (shape.inputs == 3 || ((binding.operation == Operation::NativeCall) != static_cast<bool>(binding.native)))
            return Reject("graph.compile.invalid_binding");
        types.push_back(binding.node_type);
    }
    for (const auto &node : definition.nodes)
        if (!catalog.contains(node.type_id)) return Reject("graph.compile.unbound_node", {node.id});
    if (const auto valid = ValidateGraph(definition, types); !valid)
        return foundation::Result<CompiledGraphPtr>::Failure(valid.GetError());
    if (definition.nodes.empty()) return Reject("graph.compile.empty");
    std::vector<NodeInfo> nodes;
    std::unordered_map<ElementId, std::uint32_t> port_owner;
    for (const auto &node : definition.nodes) {
        const auto found = catalog.find(node.type_id);
        if (found == catalog.end()) return Reject("graph.compile.unbound_node", {node.id});
        NodeInfo info{&node, found->second, {}, {}};
        const auto shape = Expected(info.binding->operation);
        std::size_t input_count = 0;
        std::size_t output_count = 0;
        for (const auto &port : node.ports) {
            if (port.kind != PortKind::Data) return Reject("graph.compile.unsupported_port", {node.id, port.id});
            if (port.type_id != kI64Type) return Reject("graph.compile.unsupported_type", {node.id, port.id});
            if (port.direction == PortDirection::Input) {
                if (input_count < info.inputs.size()) info.inputs[input_count] = port.id;
                ++input_count;
            } else {
                info.exposed_port = port.id;
                ++output_count;
            }
            port_owner.emplace(port.id, static_cast<std::uint32_t>(nodes.size()));
        }
        if (input_count != shape.inputs || output_count != shape.outputs)
            return Reject("graph.compile.node_shape", {node.id});
        if (info.binding->operation == Operation::Output) info.exposed_port = info.inputs[0];
        nodes.push_back(info);
    }
    std::unordered_map<ElementId, ElementId> incoming;
    std::vector<std::vector<std::uint32_t>> dependents(nodes.size());
    std::vector<std::size_t> indegree(nodes.size(), 0);
    for (const auto &connection : definition.connections) {
        incoming.emplace(connection.input_port, connection.output_port);
        const auto source = port_owner.at(connection.output_port);
        const auto target = port_owner.at(connection.input_port);
        dependents[source].push_back(target);
        ++indegree[target];
    }
    for (const auto &node : nodes) {
        for (std::size_t input = 0; input < Expected(node.binding->operation).inputs; ++input) {
            if (!incoming.contains(node.inputs[input]))
                return Reject("graph.compile.unconnected_input", {node.definition->id, node.inputs[input]});
        }
    }
    const auto later = [&](std::uint32_t first, std::uint32_t second) {
        return nodes[first].definition->id.Raw() > nodes[second].definition->id.Raw();
    };
    std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, decltype(later)> ready(later);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        if (indegree[index] == 0) ready.push(static_cast<std::uint32_t>(index));
    auto program = std::shared_ptr<CompiledGraph>(new CompiledGraph);
    program->asset_ = definition.asset_id;
    program->slots_.reserve(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index)
        program->slots_.push_back({nodes[index].exposed_port, static_cast<std::uint32_t>(index),
                                  nodes[index].binding->operation == Operation::Input});
    while (!ready.empty()) {
        const auto index = ready.top();
        ready.pop();
        const auto &node = nodes[index];
        Instruction instruction{node.binding->operation, node.definition->id, index, 0, 0, 0};
        if (Expected(instruction.operation).inputs > 0)
            instruction.left = port_owner.at(incoming.at(node.inputs[0]));
        if (Expected(instruction.operation).inputs > 1)
            instruction.right = port_owner.at(incoming.at(node.inputs[1]));
        if (instruction.operation == Operation::NativeCall) {
            instruction.callable = static_cast<std::uint32_t>(program->native_sites_.size());
            program->native_sites_.push_back({node.definition->id, node.binding->native});
        }
        program->instructions_.push_back(instruction);
        for (const auto dependent : dependents[index]) {
            if (--indegree[dependent] == 0) ready.push(dependent);
        }
    }
    if (program->instructions_.size() != nodes.size()) {
        ElementId first{};
        for (std::size_t index = 0; index < nodes.size(); ++index)
            if (indegree[index] && (!first.IsValid() || nodes[index].definition->id.Raw() < first.Raw()))
                first = nodes[index].definition->id;
        return Reject("graph.compile.data_cycle", {first});
    }
    return foundation::Result<CompiledGraphPtr>::Success(std::move(program));
}
} // namespace epidemic::runtime::graph
