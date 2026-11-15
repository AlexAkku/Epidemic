#include "Epidemic/Game/Editor/editor_project.h"
#include <algorithm>

namespace epidemic::game::editor {
namespace {
const EditorDocument *Document(const ProjectDefinition &p, runtime::AssetId id) {
    const auto at=std::find_if(p.documents.begin(),p.documents.end(),[&](const auto &d){return d.id==id;});
    return at==p.documents.end()?nullptr:&*at;
}
const comp::PrefabNode *Node(const comp::PrefabNode &root,comp::PrefabNodeId id) {
    if(root.id==id)return &root;
    for(const auto &child:root.children)if(const auto *node=Node(child,id))return node;
    return nullptr;
}
}
ReferenceDescription DescribeGraphReference(const ProjectDefinition &p,const runtime::graph::GraphReference &r) {
    using K=runtime::graph::ReferenceKind;
    if(!runtime::graph::ValidReferenceIdentity(r))return {false,"Missing Reference","editor.reference_identity"};
    const auto *owner=Document(p,r.document);
    if(!owner)return {false,"Missing Reference","editor.reference_document_missing"};
    std::string label=owner->name;
    if(r.kind==K::PrefabDefinition)return {owner->Kind()==DocumentKind::Prefab,label,""};
    if(owner->Kind()==DocumentKind::Scene) {
        const auto &scene=std::get<composition::SceneCompositionDefinition>(owner->data);
        const auto node=std::find_if(scene.scene.nodes.begin(),scene.scene.nodes.end(),[&](const auto &n){return n.id.Raw()==r.placement.Raw();});
        if(node==scene.scene.nodes.end())return {false,label,"editor.reference_entity_missing"};
        const auto name=scene.node_names.find(r.placement.Raw());
        label+=" / "+(name==scene.node_names.end()?std::to_string(r.placement.Raw()):name->second);
        const auto placement=std::find_if(scene.placements.begin(),scene.placements.end(),[&](const auto &v){return v.node.Raw()==r.placement.Raw();});
        if(!r.prefab.IsValid())return {true,label,""};
        if(placement==scene.placements.end()||placement->prefab!=r.prefab)return {false,label,"editor.reference_instance_missing"};
    } else if(owner->Kind()!=DocumentKind::Prefab || owner->id!=r.prefab)return {false,label,"editor.reference_scope"};
    auto prefab=MaterializePrefab(p,r.prefab);
    if(!prefab)return {false,label,"editor.reference_prefab_missing"};
    if(r.kind==K::PrefabInstance&&r.entity!=prefab.Value().root.id)return {false,label,"editor.reference_instance_root"};
    const auto *node=Node(prefab.Value().root,r.entity);
    if(!node)return {false,label,"editor.reference_entity_missing"};
    label+=" / Entity "+std::to_string(node->id.Raw());
    if(r.kind==K::Component) {
        const auto component=std::find_if(node->components.begin(),node->components.end(),[&](const auto &c){return c.type.Raw()==r.component.Raw()&&c.part.Raw()==r.part.Raw();});
        if(component==node->components.end())return {false,label,"editor.reference_component_missing"};
        label+=" / Component "+std::to_string(r.component.Raw())+" part="+std::to_string(r.part.Raw());
    }
    return {true,label,""};
}
foundation::Result<void> CheckGraphReferenceScope(const ProjectDefinition &p,runtime::AssetId graph,const runtime::graph::GraphReference &r) {
    const auto reject=[] {return foundation::Result<void>::Failure(foundation::Error::Create("editor.reference_scope","cross-scene or mixed prefab scopes are not supported"));};
    if(!runtime::graph::ValidReferenceIdentity(r))return reject();
    if(r.kind==runtime::graph::ReferenceKind::PrefabDefinition)return foundation::Result<void>::Success();
    for(const auto &system:p.systems)if(system.graph==graph&&system.scene.IsValid()&&system.scene!=r.document)return reject();
    if(const auto *doc=Document(p,graph);doc&&doc->Kind()==DocumentKind::Graph)
        for(const auto &node:std::get<runtime::graph::GraphDefinition>(doc->data).nodes)
            if(node.reference&&node.reference->kind!=runtime::graph::ReferenceKind::PrefabDefinition&&node.reference->document!=r.document)return reject();
    return foundation::Result<void>::Success();
}

std::vector<ReferenceCandidate> GraphReferenceCandidates(const ProjectDefinition &p,runtime::AssetId document,runtime::graph::ElementId element) {
    namespace g=runtime::graph;
    std::vector<ReferenceCandidate> result;
    const auto *doc=Document(p,document);
    if(!doc)return result;
    const auto add=[&](g::GraphReference r,std::string label) {
        auto description=DescribeGraphReference(p,r);
        if(description.available)result.push_back({r,std::move(label)+" · "+description.label});
    };
    runtime::AssetId prefab;
    g::ElementId placement;
    if(doc->Kind()==DocumentKind::Scene) {
        placement=element;
        const auto &scene=std::get<composition::SceneCompositionDefinition>(doc->data);
        const auto at=std::find_if(scene.placements.begin(),scene.placements.end(),[&](const auto &v){return v.node.Raw()==element.Raw();});
        if(at==scene.placements.end()) {add({g::ReferenceKind::SceneEntity,document,element,{},{},{},{}},"Reference Scene Entity");return result;}
        prefab=at->prefab;
    } else if(doc->Kind()==DocumentKind::Prefab)prefab=document;
    else return result;
    auto materialized=MaterializePrefab(p,prefab);
    if(!materialized)return result;
    const auto *root=placement.IsValid()?&materialized.Value().root:Node(materialized.Value().root,element);
    if(!root)return result;
    add({g::ReferenceKind::PrefabDefinition,prefab,{},prefab,{},{},{}},"Reference Prefab Definition");
    if(placement.IsValid())add({g::ReferenceKind::PrefabInstance,document,placement,prefab,materialized.Value().root.id,{},{}},"Reference Prefab Instance");
    const auto visit=[&](const auto &self,const comp::PrefabNode &node)->void {
        add({g::ReferenceKind::SceneEntity,document,placement,prefab,node.id,{},{}},"Reference Entity");
        for(const auto &c:node.components)add({g::ReferenceKind::Component,document,placement,prefab,node.id,g::TypeId{c.type.Raw()},g::ElementId{c.part.Raw()}},"Get Component");
        if(placement.IsValid())for(const auto &child:node.children)self(self,child);
    };
    visit(visit,*root);
    return result;
}
}
