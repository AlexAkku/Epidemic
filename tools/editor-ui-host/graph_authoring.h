#pragma once
#include "Epidemic/Runtime/Graph/graph_vm.h"
#include "Epidemic/Game/Editor/editor_project.h"
#include "Epidemic/Game/Composition/gameplay.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace epidemic::game::editor::graph_ui {
namespace g = runtime::graph;
struct Recipe { std::string name, category; g::Operation operation; g::NodeDefinition node; };
inline std::vector<Recipe> Catalogue(const ProjectSchema &schema) {
    std::vector<Recipe> result;
    for (const auto &[type, op] : schema.operations) {
        const auto descriptor=std::find_if(composition::arena::Operations().begin(),composition::arena::Operations().end(),[&](const auto &v){return foundation::StringId::FromString(v.id)==type;});
        const auto label = descriptor!=composition::arena::Operations().end()?std::string(descriptor->name):[op]() -> std::string {
            switch (op) {
            case g::Operation::EventEntry: return "Event";
            case g::Operation::NativeCall: return "Native Call";
            case g::Operation::Wait: return "Wait";
            case g::Operation::Input: return "Input";
            case g::Operation::Output: return "Output";
            case g::Operation::Add: return "Add";
            case g::Operation::Multiply: return "Multiply";
            case g::Operation::Entry: return "Entry";
            case g::Operation::Sequence: return "Sequence";
            case g::Operation::Branch: return "Branch";
            case g::Operation::Repeat: return "Repeat";
            case g::Operation::Reference:
            case g::Operation::ReferenceValid: return {};
            }
            return {};
        }();
        if (label.empty()) continue;
        Recipe r{label, op == g::Operation::EventEntry ? "Events" : op == g::Operation::Add || op == g::Operation::Multiply ? "Math" : op == g::Operation::NativeCall ? "Native" :
            (op == g::Operation::Input || op == g::Operation::Output || op == g::Operation::Add || op == g::Operation::Multiply) ? "Data" : "Control", op, {{},type,label,{}}};
        if(descriptor!=composition::arena::Operations().end())r.category=descriptor->category;
        const auto port = [&](std::string name, g::PortKind kind, g::PortDirection direction) {
            r.node.ports.push_back({foundation::StringId::FromString(label+"."+name),name,kind,direction,
                kind == g::PortKind::Control ? g::TypeId{} : g::kI64Type});
        };
        using K = g::PortKind; using D = g::PortDirection;
        if (op == g::Operation::EventEntry) { port("event",K::Event,D::Input); port("payload",K::Data,D::Output); }
        if (op == g::Operation::Input) port("value",K::Data,D::Output);
        if (op == g::Operation::Add || op == g::Operation::Multiply) {
            port("left",K::Data,D::Input); port("right",K::Data,D::Input); port("result",K::Data,D::Output);
        }
        if (op == g::Operation::Wait) {
            port("value",K::Data,D::Input); port("ticks",K::Data,D::Input); port("result",K::Data,D::Output);
        }
        if (op == g::Operation::NativeCall) { port("argument",K::Data,D::Input); port("result",K::Data,D::Output); }
        if (op == g::Operation::Output) port("value",K::Data,D::Input);
        if (op == g::Operation::Branch) port("condition",K::Data,D::Input);
        if (op == g::Operation::Repeat) { port("count",K::Data,D::Input); port("iteration",K::Data,D::Output); }
        if (op == g::Operation::Wait || op == g::Operation::NativeCall || op == g::Operation::Output ||
            op == g::Operation::Sequence || op == g::Operation::Branch || op == g::Operation::Repeat) port("in",K::Control,D::Input);
        if (op == g::Operation::Entry || op == g::Operation::EventEntry || op == g::Operation::Wait || op == g::Operation::NativeCall) port("next",K::Control,D::Output);
        if (op == g::Operation::Sequence) { port("first",K::Control,D::Output); port("second",K::Control,D::Output); }
        if (op == g::Operation::Branch) { port("true",K::Control,D::Output); port("false",K::Control,D::Output); }
        if (op == g::Operation::Repeat) { port("body",K::Control,D::Output); port("done",K::Control,D::Output); }
        if(op==g::Operation::Add||op==g::Operation::Multiply)for(auto &p:r.node.ports)if(p.kind==K::Data&&p.direction==D::Input)p.default_value=op==g::Operation::Multiply?1:0;
        result.push_back(std::move(r));
    }
    for(const auto &reference:ReferenceTypes(schema)) {
        const auto type=g::ReferenceCheckType(reference.type);
        Recipe r{"Is "+reference.name+" Valid","References",g::Operation::ReferenceValid,{{},type,"Is "+reference.name+" Valid",{}}};
        r.node.ports={{foundation::StringId::FromString(std::to_string(type.Raw())+".reference"),"reference",g::PortKind::Data,g::PortDirection::Input,reference.type},
                     {foundation::StringId::FromString(std::to_string(type.Raw())+".valid"),"valid",g::PortKind::Data,g::PortDirection::Output,g::kI64Type}};
        result.push_back(std::move(r));
    }
    return result;
}
class SnapshotReferenceResolver final : public g::IReferenceResolver {
  public:
    explicit SnapshotReferenceResolver(std::shared_ptr<const ProjectSnapshot> snapshot):snapshot_(std::move(snapshot)){}
    foundation::Result<bool> Resolve(const g::GraphReference &r) const override {
        return foundation::Result<bool>::Success(DescribeGraphReference(snapshot_->definition,r).available);
    }
  private:
    std::shared_ptr<const ProjectSnapshot> snapshot_;
};
inline std::vector<Recipe> Catalogue(const ProjectSchema &schema,const ProjectDefinition &project) {
    auto result=Catalogue(schema);
    for(const auto &type:project.user_schemas)for(const auto &op:type.operations) {
        Recipe recipe{op.name,"C#",g::Operation::NativeCall,{{},g::TypeId::FromString(op.id),op.name,{}}};
        using K=g::PortKind;using D=g::PortDirection;
        for(const auto &[name,kind,direction]:std::array{std::tuple{"argument",K::Data,D::Input},std::tuple{"result",K::Data,D::Output},std::tuple{"in",K::Control,D::Input},std::tuple{"next",K::Control,D::Output}})
            recipe.node.ports.push_back({foundation::StringId::FromString(op.id+"."+name),name,kind,direction,kind==K::Data?g::kI64Type:g::TypeId{}});
        result.push_back(std::move(recipe));
    }
    return result;
}
inline GraphNodeSettings Settings(const EditorDocument &doc, std::uint64_t node) {
    const auto found = std::find_if(doc.graph_settings.begin(),doc.graph_settings.end(),[&](const auto &s){return s.node == node;});
    return found == doc.graph_settings.end() ? GraphNodeSettings{node,0,{},{},false,false} : *found;
}
inline GraphNodePosition Position(const EditorDocument &doc, std::size_t index) {
    const auto node = std::get<g::GraphDefinition>(doc.data).nodes.at(index).id.Raw();
    const auto found = std::find_if(doc.graph_layout.begin(),doc.graph_layout.end(),[&](const auto &p){return p.node == node;});
    return found == doc.graph_layout.end() ? GraphNodePosition{node,40.0+280.0*static_cast<double>(index%4),80.0+240.0*std::floor(static_cast<double>(index)/4.0)} : *found;
}
} // namespace epidemic::game::editor::graph_ui
