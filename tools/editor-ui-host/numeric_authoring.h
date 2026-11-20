#pragma once
#include "graph_authoring.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include <bit>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace epidemic::game::editor::numeric_ui {
namespace g=runtime::graph;
inline std::optional<g::Operation> Family(g::TypeId type){
    for(const auto &[name,op]:std::array{std::pair{"input.type",g::Operation::Input},std::pair{"output.type",g::Operation::Output},std::pair{"add.type",g::Operation::Add},std::pair{"multiply.type",g::Operation::Multiply}})
        if(type==g::TypeId::FromString(name))return op;
    for(const auto &recipe:g::kNumericRecipes)if(g::TypeId::FromString(recipe.id)==type&&recipe.operation!=g::Operation::ToFloat&&recipe.operation!=g::Operation::ToInteger)return recipe.operation;
    return {};
}
inline bool Floating(const g::NodeDefinition &node){return std::any_of(node.ports.begin(),node.ports.end(),[](const auto &p){return p.kind==g::PortKind::Data&&p.type_id==g::kF64Type;});}
inline g::TypeId Variant(g::Operation op,bool floating){
    if(!floating)for(const auto &[name,value]:std::array{std::pair{"input.type",g::Operation::Input},std::pair{"output.type",g::Operation::Output},std::pair{"add.type",g::Operation::Add},std::pair{"multiply.type",g::Operation::Multiply}})if(value==op)return g::TypeId::FromString(name);
    for(const auto &r:g::kNumericRecipes)if(r.operation==op&&r.floating==floating)return g::TypeId::FromString(r.id);
    throw std::runtime_error("ui.graph.numeric_family");
}
inline std::int64_t Convert(std::int64_t bits,bool from_float,bool to_float){
    if(from_float==to_float)return bits;
    if(to_float){
        const auto value=static_cast<double>(bits);
        if(value>=9223372036854775808.0||static_cast<std::int64_t>(value)!=bits)throw std::runtime_error("ui.graph.numeric_precision");
        return std::bit_cast<std::int64_t>(value);
    }
    const auto value=std::bit_cast<double>(bits);
    if(!std::isfinite(value)||value<-9223372036854775808.0||value>=9223372036854775808.0||std::trunc(value)!=value)throw std::runtime_error("ui.graph.numeric_locked_i64");
    return static_cast<std::int64_t>(value);
}
inline void Store(EditorDocument &doc,GraphNodeSettings value){
    auto found=std::find_if(doc.graph_settings.begin(),doc.graph_settings.end(),[&](const auto &s){return s.node==value.node;});
    if(found==doc.graph_settings.end())doc.graph_settings.push_back(std::move(value));else *found=std::move(value);
}
inline void Retype(EditorDocument &doc,g::NodeDefinition &node,bool floating){
    const auto family=Family(node.type_id);if(!family||Floating(node)==floating)return;
    auto settings=graph_ui::Settings(doc,node.id.Raw());const auto previous=Floating(node);
    if(*family==g::Operation::Input)settings.preview_default=Convert(settings.preview_default,previous,floating);
    for(auto &p:node.ports)if(p.kind==g::PortKind::Data){if(p.default_value)p.default_value=Convert(*p.default_value,previous,floating);p.type_id=floating?g::kF64Type:g::kI64Type;}
    node.type_id=Variant(*family,floating);Store(doc,settings);
}
inline std::vector<Command> Number(const EditorDocument &source,std::uint64_t address,bool literal,std::string_view text){
    auto doc=source;auto &graph=std::get<g::GraphDefinition>(doc.data);
    for(auto &node:graph.nodes){
        unsigned index=0;
        for(auto &port:node.ports){
            if(port.kind!=g::PortKind::Data)continue;
            const bool input=port.direction==g::PortDirection::Input;
            if((literal?port.id.Raw():node.id.Raw())==address){
                auto settings=graph_ui::Settings(doc,node.id.Raw());
                const bool parsed_float=settings.numeric_mode==2||(settings.numeric_mode==0&&text.find_first_of(".eE")!=std::string_view::npos);
                const auto parsed=EditorWorkspace::ParseValue(parsed_float?comp::PropertyKind::Real:comp::PropertyKind::Integer,text);
                if(!parsed)throw std::runtime_error(parsed.GetError().code);
                const auto bits=parsed_float?std::bit_cast<std::int64_t>(std::get<double>(parsed.Value())):std::get<std::int64_t>(parsed.Value());
                if(parsed_float&&!Floating(node))Retype(doc,node,true);
                settings=graph_ui::Settings(doc,node.id.Raw());
                const auto seed=1u<<(literal?index:0);
                settings.numeric_seed=static_cast<std::uint8_t>(parsed_float?settings.numeric_seed|seed:settings.numeric_seed&~seed);
                const auto value=Convert(bits,parsed_float,Floating(node));
                if(literal)port.default_value=value;else settings.preview_default=value;
                return {SetGraphNodeSettings{source.id,node.id,node.name,settings,node}};
            }
            if(input)++index;
        }
    }
    throw std::runtime_error("ui.graph_numeric_target");
}
// Plan only editor type annotations on an ephemeral candidate. The real graph,
// values, history and validation are still committed by one Workspace::Execute.
inline std::vector<Command> Infer(const EditorDocument &source,std::vector<Command> commands){
    auto doc=source;auto &graph=std::get<g::GraphDefinition>(doc.data);
    for(const auto &command:commands)std::visit([&](const auto &c){using T=std::decay_t<decltype(c)>;
        if constexpr(std::is_same_v<T,AddGraphNode>)graph.nodes.push_back(c.node);
        else if constexpr(std::is_same_v<T,DeleteGraphNode>){
            std::set<std::uint64_t> ports;for(const auto &n:graph.nodes)if(n.id==c.node)for(const auto &p:n.ports)ports.insert(p.id.Raw());
            std::erase_if(graph.nodes,[&](const auto &n){return n.id==c.node;});std::erase_if(graph.connections,[&](const auto &e){return ports.contains(e.input_port.Raw())||ports.contains(e.output_port.Raw());});
        }else if constexpr(std::is_same_v<T,ConnectPorts>)graph.connections.push_back(c.connection);
        else if constexpr(std::is_same_v<T,DisconnectPorts>)std::erase_if(graph.connections,[&](const auto &e){return e.id==c.connection;});
        else if constexpr(std::is_same_v<T,SetGraphNodeSettings>){Store(doc,c.settings);for(auto &n:graph.nodes)if(n.id==c.node){if(c.numeric_definition)n=*c.numeric_definition;n.name=c.name;}}
        else if constexpr(std::is_same_v<T,SetGraphPortDefault>)for(auto &n:graph.nodes)for(auto &p:n.ports)if(p.id==c.port)p.default_value=c.value;
    },command);
    std::vector<std::size_t> parent(graph.nodes.size());std::iota(parent.begin(),parent.end(),0);
    const auto root=[&](std::size_t i){while(parent[i]!=i)i=parent[i];return i;};
    std::map<std::uint64_t,std::pair<std::size_t,g::TypeId>> ports;
    for(std::size_t i=0;i<graph.nodes.size();++i)for(const auto &p:graph.nodes[i].ports)if(p.kind==g::PortKind::Data)ports.emplace(p.id.Raw(),std::pair{i,p.type_id});
    for(const auto &e:graph.connections)if(ports.contains(e.output_port.Raw())&&ports.contains(e.input_port.Raw())){
        const auto a=ports.at(e.output_port.Raw()).first,b=ports.at(e.input_port.Raw()).first;
        if(Family(graph.nodes[a].type_id)&&Family(graph.nodes[b].type_id))parent[root(a)]=root(b);
    }
    struct Constraint{bool floating=false,integer=false;};std::vector<Constraint> constraints(graph.nodes.size());
    for(std::size_t i=0;i<graph.nodes.size();++i)if(Family(graph.nodes[i].type_id)){
        const auto settings=graph_ui::Settings(doc,graph.nodes[i].id.Raw());auto &c=constraints[root(i)];
        c.floating=c.floating||settings.numeric_mode==2||settings.numeric_seed!=0;c.integer=c.integer||settings.numeric_mode==1;
    }
    for(const auto &e:graph.connections)if(ports.contains(e.output_port.Raw())&&ports.contains(e.input_port.Raw())){
        const auto a=ports.at(e.output_port.Raw()),b=ports.at(e.input_port.Raw());
        const bool na=Family(graph.nodes[a.first].type_id).has_value(),nb=Family(graph.nodes[b.first].type_id).has_value();
        if(na==nb)continue;auto &c=constraints[root(na?a.first:b.first)];const auto external=na?b.second:a.second;
        c.floating=c.floating||external==g::kF64Type;c.integer=c.integer||external==g::kI64Type;
    }
    for(std::size_t i=0;i<graph.nodes.size();++i)if(Family(graph.nodes[i].type_id)){
        const auto c=constraints[root(i)];if(c.floating&&c.integer)throw std::runtime_error("graph.connection_type numeric lock conflict");
        Retype(doc,graph.nodes[i],c.floating);
    }
    const auto &before=std::get<g::GraphDefinition>(source.data);
    for(const auto &node:graph.nodes)if(Family(node.type_id)){
        const auto old=std::find_if(before.nodes.begin(),before.nodes.end(),[&](const auto &n){return n.id==node.id;});
        const auto settings=graph_ui::Settings(doc,node.id.Raw());
        if(old==before.nodes.end()||old->type_id!=node.type_id||old->ports!=node.ports||graph_ui::Settings(source,node.id.Raw())!=settings)
            commands.push_back(SetGraphNodeSettings{source.id,node.id,node.name,settings,node});
    }
    return commands;
}
} // namespace epidemic::game::editor::numeric_ui
