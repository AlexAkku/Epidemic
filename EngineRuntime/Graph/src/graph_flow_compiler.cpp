#include "graph_flow.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace epidemic::runtime::graph::detail {
namespace {
struct Shape { std::size_t data_in = 0; std::size_t data_out = 0; std::size_t control_in = 0; std::size_t control_out = 0; std::size_t event_in = 0; };
std::optional<Shape> Expected(Operation operation) {
    switch (operation) {
    case Operation::Input: return Shape{0, 1, 0, 0};
    case Operation::Add:
    case Operation::Multiply: return Shape{2, 1, 0, 0};
    case Operation::Entry: return Shape{0, 0, 0, 1};
    case Operation::EventEntry: return Shape{0, 1, 0, 1, 1};
    case Operation::Repeat: return Shape{1, 1, 1, 2};
    case Operation::Wait: return Shape{2, 1, 1, 1};
    case Operation::Sequence: return Shape{0, 0, 1, 0}; // Variable nonempty ordered outputs.
    case Operation::Branch: return Shape{1, 0, 1, 2};
    case Operation::NativeCall: return Shape{1, 1, 1, 1};
    case Operation::Output: return Shape{1, 0, 1, 0};
    default: return std::nullopt;
    }
}
foundation::Result<CompiledGraphPtr> Reject(std::string_view code, ElementId node = {}) {
    return foundation::Result<CompiledGraphPtr>::Failure(foundation::Error::Create(code, "control graph rejected", "node=" + std::to_string(node.Raw())));
}
bool Acyclic(const std::vector<std::vector<std::uint32_t>> &edges) {
    std::vector<std::size_t> degree(edges.size());
    for (const auto &list : edges) for (const auto target : list) ++degree[target];
    std::queue<std::uint32_t> ready;
    for (std::size_t i = 0; i < degree.size(); ++i) if (degree[i] == 0) ready.push(static_cast<std::uint32_t>(i));
    std::size_t visited = 0;
    while (!ready.empty()) {
        const auto node = ready.front(); ready.pop(); ++visited;
        for (const auto target : edges[node]) if (--degree[target] == 0) ready.push(target);
    }
    return visited == edges.size();
}
struct Ports {
    std::vector<ElementId> data_in;
    std::vector<ElementId> data_out;
    std::vector<ElementId> control_in;
    std::vector<ElementId> control_out;
    std::vector<ElementId> events;
};
struct PortOwner { std::uint32_t node = 0; PortKind kind = PortKind::Data; };
} // namespace
bool FlowCompiler::UsesFlow(const GraphDefinition &definition, std::span<const NodeBinding> bindings) {
    std::unordered_set<TypeId> control_types;
    for (const auto &binding : bindings)
        if (binding.operation == Operation::Entry || binding.operation == Operation::Sequence || binding.operation == Operation::Branch ||
            binding.operation == Operation::EventEntry || binding.operation == Operation::Repeat)
            control_types.insert(binding.node_type);
    for (const auto &node : definition.nodes) if (control_types.contains(node.type_id)) return true;
    return false;
}
foundation::Result<CompiledGraphPtr> FlowCompiler::Compile(const GraphDefinition &definition, std::span<const NodeBinding> bindings) {
    std::unordered_map<TypeId, const NodeBinding *> catalog;
    std::vector<TypeId> types{kI64Type};
    for (const auto &binding : bindings) {
        if (!binding.node_type.IsValid() || !catalog.emplace(binding.node_type, &binding).second ||
            static_cast<unsigned>(binding.operation) > static_cast<unsigned>(Operation::Repeat) ||
            ((binding.operation == Operation::NativeCall) != static_cast<bool>(binding.native))) return Reject("graph.compile.invalid_binding");
        types.push_back(binding.node_type);
    }
    if (const auto valid = ValidateGraph(definition, types); !valid)
        return foundation::Result<CompiledGraphPtr>::Failure(valid.GetError());
    std::vector<const NodeDefinition *> ordered;
    for (const auto &node : definition.nodes) ordered.push_back(&node);
    std::sort(ordered.begin(), ordered.end(), [](const auto *a, const auto *b) { return a->id.Raw() < b->id.Raw(); });
    auto program = std::shared_ptr<CompiledGraph>(new CompiledGraph);
    auto flow = std::make_shared<FlowProgram>();
    program->asset_ = definition.asset_id;
    flow->nodes.resize(ordered.size());
    std::vector<Ports> ports(ordered.size());
    std::unordered_map<ElementId, PortOwner> owners;
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        const auto &node = *ordered[i];
        const auto found = catalog.find(node.type_id);
        if (found == catalog.end()) return Reject("graph.compile.unbound_node", node.id);
        const auto &binding = *found->second;
        const auto shape = Expected(binding.operation);
        if (!shape) return Reject("graph.flow.unsupported_operation", node.id);
        const auto index = static_cast<std::uint32_t>(i);
        auto &p = ports[i];
        for (const auto &port : node.ports) {
            if (port.kind == PortKind::Event) {
                if (binding.operation != Operation::EventEntry || port.direction != PortDirection::Input || port.type_id != kI64Type)
                    return Reject("graph.flow.unsupported_event", node.id);
                p.events.push_back(port.id);
            } else if (port.kind == PortKind::Data) {
                if (port.type_id != kI64Type) return Reject("graph.compile.unsupported_type", node.id);
                (port.direction == PortDirection::Input ? p.data_in : p.data_out).push_back(port.id);
            } else (port.direction == PortDirection::Input ? p.control_in : p.control_out).push_back(port.id);
            owners.emplace(port.id, PortOwner{index, port.kind});
        }
        if (p.data_in.size() != shape->data_in || p.data_out.size() != shape->data_out || p.control_in.size() != shape->control_in || p.events.size() != shape->event_in ||
            (binding.operation == Operation::Sequence ? p.control_out.empty() : p.control_out.size() != shape->control_out))
            return Reject("graph.compile.node_shape", node.id);
        if (binding.operation == Operation::Entry) {
            if (flow->entry != kNoNode) return Reject("graph.flow.multiple_entries", node.id);
            flow->entry = index;
        }
        if (binding.operation == Operation::EventEntry) flow->events.push_back({p.events[0], index});
        Instruction instruction{binding.operation, node.id, index, 0, 0, 0};
        if (binding.operation == Operation::NativeCall) {
            instruction.callable = static_cast<std::uint32_t>(program->native_sites_.size());
            program->native_sites_.push_back({node.id, binding.native});
        }
        program->instructions_.push_back(instruction);
        if (!p.data_out.empty()) program->slots_.push_back({p.data_out[0], index, binding.operation == Operation::Input});
        if (binding.operation == Operation::Output) program->slots_.push_back({p.data_in[0], index, false});
        flow->nodes[i].next.resize(p.control_out.size(), kNoNode);
    }
    if (flow->entry == kNoNode && flow->events.empty()) return Reject("graph.flow.missing_entry");
    std::unordered_map<ElementId, ElementId> incoming;
    std::unordered_map<ElementId, std::uint32_t> outgoing;
    std::vector<std::vector<std::uint32_t>> data_edges(ordered.size()), control_edges(ordered.size());
    std::vector<bool> control_connected(ordered.size());
    for (const auto &edge : definition.connections) {
        const auto source = owners.at(edge.output_port); const auto target = owners.at(edge.input_port);
        if (source.kind == PortKind::Data) {
            incoming.emplace(edge.input_port, edge.output_port); data_edges[source.node].push_back(target.node);
        } else if (source.kind == PortKind::Control) {
            if (!outgoing.emplace(edge.output_port, target.node).second) return Reject("graph.flow.control_fanout", ordered[source.node]->id);
            control_edges[source.node].push_back(target.node); control_connected[target.node] = true;
        } else return Reject("graph.flow.event_connection");
    }
    if (!Acyclic(data_edges)) return Reject("graph.compile.data_cycle");
    if (!Acyclic(control_edges)) return Reject("graph.flow.control_cycle");
    std::vector<bool> reached(ordered.size());
    std::vector<std::uint32_t> pending;
    if (flow->entry != kNoNode) pending.push_back(flow->entry);
    for (const auto &event : flow->events) pending.push_back(event.node);
    while (!pending.empty()) {
        const auto node = pending.back(); pending.pop_back();
        if (reached[node]) continue;
        reached[node] = true;
        for (const auto next : control_edges[node]) pending.push_back(next);
    }
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        const auto &p = ports[i]; auto &node = flow->nodes[i];
        if (!p.control_in.empty() && (!control_connected[i] || !reached[i])) return Reject("graph.flow.unreachable_control", ordered[i]->id);
        for (const auto input : p.data_in) {
            const auto found = incoming.find(input);
            if (found == incoming.end()) return Reject("graph.compile.unconnected_input", ordered[i]->id);
            node.data.push_back(owners.at(found->second).node);
        }
        if (!node.data.empty()) program->instructions_[i].left = node.data[0];
        if (node.data.size() > 1) program->instructions_[i].right = node.data[1];
        for (std::size_t n = 0; n < p.control_out.size(); ++n) {
            const auto found = outgoing.find(p.control_out[n]);
            if (found != outgoing.end()) node.next[n] = found->second;
        }
        if (program->instructions_[i].operation == Operation::Repeat && node.next[0] != kNoNode) {
            std::vector<bool> member(ordered.size());
            std::vector<std::uint32_t> body{node.next[0]};
            while (!body.empty()) {
                const auto current = body.back(); body.pop_back();
                if (member[current]) continue;
                member[current] = true; node.clear.push_back(current);
                for (const auto next : control_edges[current]) body.push_back(next);
            }
        }
    }
    program->flow_ = std::move(flow);
    return foundation::Result<CompiledGraphPtr>::Success(std::move(program));
}
} // namespace epidemic::runtime::graph::detail
