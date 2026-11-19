#include "Epidemic/Game/Editor/editor_project.h"
#include "Epidemic/Game/Composition/gameplay.h"
#include "project_archive.h"
#include <algorithm>
#include <unordered_set>

namespace epidemic::game::editor {
namespace {
foundation::Error Error(std::string_view code) {
    return foundation::Error::Create(code, "editor document rejected");
}

const EditorDocument *Find(const ProjectDefinition &definition, runtime::AssetId id) {
    const auto found = std::find_if(definition.documents.begin(), definition.documents.end(),
                                    [&](const auto &document) { return document.id == id; });
    return found == definition.documents.end() ? nullptr : &*found;
}

class References final : public comp::IGraphBehaviorHost {
  public:
    References(const ProjectDefinition &definition, const ProjectSchema &schema)
        : definition_(definition), schema_(schema), operations_(EffectiveOperations(definition,schema)) {
    }

    foundation::Result<void>
    CheckDefinition(const comp::GraphBehaviorDefinition &definition) const override {
        const auto *document = Find(definition_, runtime::AssetId{definition.graph});
        if (!document || document->Kind() != DocumentKind::Graph)
            return foundation::Result<void>::Failure(Error("editor.graph_reference"));
        const auto &graph = std::get<runtime::graph::GraphDefinition>(document->data);
        const auto has_port = [&](foundation::StringId id, runtime::graph::Operation operation) {
            for (const auto &node : graph.nodes) {
                const auto binding =
                    std::find_if(operations_.begin(), operations_.end(),
                                 [&](const auto &item) { return item.first == node.type_id; });
                if (binding != operations_.end() && binding->second == operation)
                    for (const auto &port : node.ports)
                        if (port.id == id &&
                            (operation == runtime::graph::Operation::EventEntry
                                 ? port.kind == runtime::graph::PortKind::Event &&
                                       port.direction == runtime::graph::PortDirection::Input
                                 : port.kind == runtime::graph::PortKind::Data &&
                                       port.direction == runtime::graph::PortDirection::Output))
                            return true;
            }

            return false;
        };

        for (const auto &event : definition.events)
            if (!has_port(event.port, runtime::graph::Operation::EventEntry))
                return foundation::Result<void>::Failure(Error("editor.port_reference"));
        for (const auto &input : definition.inputs)
            if (!has_port(input.port, runtime::graph::Operation::Input))
                return foundation::Result<void>::Failure(Error("editor.port_reference"));
        return foundation::Result<void>::Success();
    }

    foundation::Result<std::shared_ptr<const comp::IGraphProgram>>
    Bind(const std::shared_ptr<comp::GraphScope> &,
         const comp::GraphBehaviorDefinition &) override {
        return foundation::Result<std::shared_ptr<const comp::IGraphProgram>>::Failure(
            Error("editor.authoring_only"));
    }

    foundation::Result<std::shared_ptr<comp::IGraphActivation>>
    Dispatch(const std::shared_ptr<const comp::IGraphProgram> &, const comp::GameEvent &) override {
        return foundation::Result<std::shared_ptr<comp::IGraphActivation>>::Failure(
            Error("editor.authoring_only"));
    }

  private:
    const ProjectDefinition &definition_;
    const ProjectSchema &schema_;
    std::vector<std::pair<runtime::graph::TypeId,runtime::graph::Operation>> operations_;
};

void Dependencies(const comp::PrefabNode &node,
                  std::vector<runtime::AssetDependency> &dependencies) {
    for (const auto &behavior : node.behaviors)
        dependencies.push_back({runtime::AssetId{behavior.graph}, true});
    for (const auto &component : node.components)
        for (const auto &behavior : component.behaviors)
            dependencies.push_back({runtime::AssetId{behavior.graph}, true});
    for (const auto &child : node.children)
        Dependencies(child, dependencies);
}

} // namespace

const EditorDocument *ProjectSnapshot::Find(runtime::AssetId id) const noexcept {
    return editor::Find(definition, id);
}

foundation::Result<comp::GraphBehaviorDefinition>
DefaultGraphBehavior(const ProjectDefinition &definition, const ProjectSchema &schema,
                     runtime::AssetId graph, foundation::StringId id) {
    using Result = foundation::Result<comp::GraphBehaviorDefinition>;
    const auto *document = Find(definition,graph);
    const auto *source = document ? std::get_if<runtime::graph::GraphDefinition>(&document->data) : nullptr;
    if (!source || !id.IsValid())
        return Result::Failure(Error("editor.graph_reference"));
    foundation::StringId entry;
    std::vector<comp::GraphEventRoute> routes;
    for (const auto &node : source->nodes) {
        const auto type = std::ranges::find(schema.operations,node.type_id,
            &std::pair<runtime::graph::TypeId,runtime::graph::Operation>::first);
        if (type == schema.operations.end() || type->second != runtime::graph::Operation::EventEntry)
            continue;
        const bool named=std::any_of(composition::arena::Operations().begin(),composition::arena::Operations().end(),[&](const auto &op){return op.kind==runtime::graph::Operation::EventEntry&&foundation::StringId::FromString(op.id)==node.type_id;});
        for (const auto &port : node.ports)
            if (port.kind == runtime::graph::PortKind::Event &&
                port.direction == runtime::graph::PortDirection::Input &&
                port.type_id == runtime::graph::kI64Type && !named &&
                (!entry.IsValid() || port.id.Raw() < entry.Raw()))
                entry = port.id;
        for(const auto &operation:composition::arena::Operations())if(operation.kind==runtime::graph::Operation::EventEntry&&foundation::StringId::FromString(operation.id)==node.type_id)
            for(const auto &port:node.ports)if(port.kind==runtime::graph::PortKind::Event&&port.direction==runtime::graph::PortDirection::Input)routes.push_back({node.type_id,port.id});
    }
    if (!entry.IsValid()&&routes.empty())
        return Result::Failure(Error("editor.graph_event_entry"));
    if(entry.IsValid())routes.push_back({entry,entry});
    return Result::Success({id,graph.value,std::move(routes),{}});
}

foundation::Result<comp::PrefabDefinition>
MaterializePrefab(const ProjectDefinition &definition, runtime::AssetId prefab,
                  LinkedPrefabSources *linked_sources) {
    using Result = foundation::Result<comp::PrefabDefinition>;
    const auto reject = [] {return Result::Failure(Error("editor.prefab_link"));};
    std::vector<runtime::AssetId> active;
    std::size_t count = 0;
    const auto find_node = [](auto &&self, auto &node, comp::PrefabNodeId id) -> decltype(&node) {
        if (node.id == id)
            return &node;
        for (auto &child : node.children)
            if (auto *found = self(self,child,id))
                return found;
        return nullptr;
    };
    const auto count_nodes = [&](auto &&self, const comp::PrefabNode &node,std::size_t depth) -> bool {
        if (depth > 32 || ++count > 1024)
            return false;
        return std::ranges::all_of(node.children,[&](const auto &child) {return self(self,child,depth+1);});
    };
    const auto expand = [&](auto &&self, runtime::AssetId id, LinkedPrefabSources *sources) -> Result {
        if (active.size() >= 32 || std::ranges::find(active,id) != active.end())
            return reject();
        const auto *document = Find(definition,id);
        const auto *source = document ? std::get_if<comp::PrefabDefinition>(&document->data) : nullptr;
        if (!source || !count_nodes(count_nodes,source->root,active.size()))
            return reject();
        active.push_back(id);
        auto result = *source;
        for (const auto &link : definition.prefab_links) {
            if (link.owner != id)
                continue;
            if (!find_node(find_node,source->root,link.parent))
                return reject();
            LinkedPrefabSources child_sources;
            auto nested = self(self, link.target, sources ? &child_sources : nullptr);
            if (!nested)
                return nested;
            auto &child = nested.Value();
            const auto remap = [&](auto &&visit, comp::PrefabNode &node) -> bool {
                const auto original = node.id;
                node.id = foundation::StringId::FromString("editor.prefab.link." +
                    std::to_string(link.id.Raw()) + "." + std::to_string(original.Raw()));
                if (sources) {
                    const auto found = child_sources.find(original.Raw());
                    sources->emplace(node.id.Raw(), found == child_sources.end() ? link.target : found->second);
                }
                for (auto &component : node.components)
                    for (auto &property : component.properties)
                        if (property.parameter.IsValid()) {
                            const auto parameter = std::ranges::find(child.parameters,property.parameter,&comp::PrefabParameter::id);
                            if (parameter == child.parameters.end())
                                return false;
                            property.value = parameter->value;
                            property.parameter = {};
                        }
                return std::ranges::all_of(node.children,[&](auto &descendant) {return visit(visit,descendant);});
            };
            if (!remap(remap,child.root))
                return reject();
            auto *parent = find_node(find_node,result.root,link.parent);
            if (!parent)
                return reject();
            parent->children.push_back(std::move(child.root));
        }
        active.pop_back();
        return Result::Success(std::move(result));
    };
    if (linked_sources)
        linked_sources->clear();
    return expand(expand, prefab, linked_sources);
}

foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>>
ValidatePreparedProject(const ProjectDefinition &definition, const ProjectSchema &schema) {
    using Result = foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>>;
    if (!definition.id.IsValid() || definition.documents.size() > 64 || definition.systems.size() > 128 ||
        definition.prefab_links.size() > 256 ||
        !schema.entities.IsFrozen() || !schema.components.IsFrozen())
        return Result::Failure(Error("editor.invalid_project"));
    std::set<std::string> user_operations;
    const auto relative=[](std::string path,std::string_view prefix){std::replace(path.begin(),path.end(),'\\','/');return path.starts_with(prefix)&&path.find("../")==std::string::npos&&path.find(":")==std::string::npos;};
    for(const auto &type:definition.user_schemas){
        if(!definition.user_schemas.empty()&&(type.assembly!=definition.user_schemas.front().assembly||type.fingerprint!=definition.user_schemas.front().fingerprint))return Result::Failure(Error("editor.user_generation_mismatch"));
        if(!relative(type.assembly,".epidemic/scripts/")||type.assembly.size()>1024||type.fingerprint.size()!=64||!std::all_of(type.fingerprint.begin(),type.fingerprint.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))return Result::Failure(Error("editor.user_schema_identity"));
        for(const auto &op:type.operations)
            if(op.id.empty()||op.id.size()>256||op.name.empty()||op.type.empty()||op.method.empty()||op.source.size()>1024||!relative(op.source,"Scripts/")||op.line==0||op.line>10000000||!user_operations.insert(op.id).second)
                return Result::Failure(Error("editor.user_operation"));
    }
    std::unordered_set<std::uint64_t> ids;
    std::unordered_set<std::uint64_t> operation_types;
    if (EffectiveOperations(definition,schema).size() > 256)
        return Result::Failure(Error("editor.operation_schema"));
    std::vector<runtime::graph::TypeId> types{runtime::graph::kI64Type,runtime::graph::kF64Type};

    for (const auto &[type, operation] : EffectiveOperations(definition,schema)) {
        if (!type.IsValid() || !operation_types.insert(type.Raw()).second ||
            operation > runtime::graph::Operation::ToInteger)
            return Result::Failure(Error("editor.operation_schema"));
        types.push_back(type);
    }
    for (const auto &reference : ReferenceTypes(schema)) types.push_back(reference.type);

    References references{definition, schema};

    comp::GraphBehaviorService behaviors{references};

    comp::ComponentStore components{schema.entities, schema.components};

    comp::PrefabComposer prefabs{schema.entities, schema.components, components, behaviors};
    std::unordered_set<std::uint64_t> links;
    for (const auto &link : definition.prefab_links) {
        const auto *owner = Find(definition,link.owner);
        const auto *target = Find(definition,link.target);
        if (!link.id.IsValid() || !links.insert(link.id.Raw()).second || !link.parent.IsValid() ||
            !owner || owner->Kind()!=DocumentKind::Prefab || !target || target->Kind()!=DocumentKind::Prefab)
            return Result::Failure(Error("editor.prefab_link"));
    }
    std::unordered_set<std::uint64_t> systems;
    for (const auto &system : definition.systems) {
        const auto *scene = Find(definition,system.scene);
        const auto *graph = Find(definition,system.graph);
        if (!system.id.IsValid() || !systems.insert(system.id.Raw()).second ||
            (system.scene.IsValid() && (!scene || scene->Kind()!=DocumentKind::Scene)) ||
            !graph || graph->Kind()!=DocumentKind::Graph)
            return Result::Failure(Error("editor.system_reference"));
        const auto behavior = DefaultGraphBehavior(definition,schema,system.graph,system.id);
        if (!behavior)
            return Result::Failure(behavior.GetError());
        const auto valid = behaviors.Validate(behavior.Value());
        if (!valid)
            return Result::Failure(valid.GetError());
    }

    auto assets = runtime::CreateAssetServices();
    if (!assets)
        return Result::Failure(assets.GetError());
    for (const auto &document : definition.documents) {
        if (!document.id.IsValid() || document.name.empty() || document.name.size() > 256 ||
            !ids.insert(document.id.Raw()).second)
            return Result::Failure(Error("editor.document_identity"));
        if (document.Kind() == DocumentKind::Graph) {
            const auto &graph = std::get<runtime::graph::GraphDefinition>(document.data);
            if (graph.asset_id != document.id)
                return Result::Failure(Error("editor.document_identity"));
            const auto valid = runtime::graph::ValidateGraph(graph, types);
            if (!valid)
                return Result::Failure(valid.GetError());
            for (const auto &node : graph.nodes) if (node.reference) {
                const auto scope = CheckGraphReferenceScope(definition,document.id,*node.reference);
                if (!scope) return Result::Failure(scope.GetError());
            }
        } else if (document.Kind() == DocumentKind::Prefab) {
            const auto &prefab = std::get<comp::PrefabDefinition>(document.data);
            if (prefab.id != document.id.value)
                return Result::Failure(Error("editor.document_identity"));
            const auto expanded = MaterializePrefab(definition,document.id);
            if (!expanded)
                return Result::Failure(expanded.GetError());
            const auto valid = prefabs.Register(expanded.Value());
            if (!valid)
                return Result::Failure(valid.GetError());
        }

    }

    for (const auto &document : definition.documents) {
        runtime::AssetMetadata metadata;
        metadata.id = document.id;
        metadata.state = runtime::AssetState::Validated;
        metadata.version = 1;
        metadata.location.kind = runtime::AssetLocationKind::VirtualPath;
        metadata.location.mount_id = definition.id;
        metadata.location.path = "document/" + std::to_string(document.id.Raw());
        if (document.Kind() == DocumentKind::Scene) {
            const auto &scene = std::get<composition::SceneCompositionDefinition>(document.data);
            if (scene.asset_id != document.id)
                return Result::Failure(Error("editor.document_identity"));
            const auto validated = composition::CreateSceneComposition(scene);
            if (!validated)
                return Result::Failure(validated.GetError());
            std::unordered_set<std::uint64_t> placed;
            for (const auto &placement : scene.placements) {
                const auto *prefab = Find(definition, placement.prefab);
                if (!validated.Value().nodes.contains(placement.node) ||
                    !placed.insert(placement.node.Raw()).second || !prefab ||
                    prefab->Kind() != DocumentKind::Prefab)
                    return Result::Failure(Error("editor.prefab_reference"));
                const auto overrides =
                    prefabs.Validate(placement.prefab.value, placement.overrides);
                if (!overrides)
                    return Result::Failure(overrides.GetError());
                metadata.dependencies.push_back({placement.prefab, true});
            }

            metadata.type = {foundation::StringId::FromString("editor.scene")};
            for (const auto &system : definition.systems)
                if (!system.scene.IsValid() || system.scene == document.id)
                    metadata.dependencies.push_back({system.graph,true});

        } else if (document.Kind() == DocumentKind::Prefab) {
            Dependencies(std::get<comp::PrefabDefinition>(document.data).root,
                         metadata.dependencies);
            for (const auto &link : definition.prefab_links)
                if (link.owner == document.id)
                    metadata.dependencies.push_back({link.target,true});
            metadata.type = {foundation::StringId::FromString("editor.prefab")};

        } else
            metadata.type = {foundation::StringId::FromString("editor.graph")};

        // Dependencies describe asset edges, not individual placements/behaviors.
        // Several instances may legitimately reference the same required asset.
        std::sort(metadata.dependencies.begin(), metadata.dependencies.end(),
                  [](const auto &a, const auto &b) { return a.asset_id.Raw() < b.asset_id.Raw(); });
        metadata.dependencies.erase(std::unique(metadata.dependencies.begin(),
                                                metadata.dependencies.end()),
                                    metadata.dependencies.end());
        const auto registered = assets.Value().writer->RegisterAsset(metadata);
        if (!registered)
            return Result::Failure(registered.GetError());
    }

    if (definition.entry_scene.IsValid()) {
        const auto *entry = Find(definition, definition.entry_scene);
        if (!entry || entry->Kind() != DocumentKind::Scene)
            return Result::Failure(Error("editor.entry_scene"));
    }

    const auto sealed = assets.Value().writer->Seal();
    if (!sealed)
        return Result::Failure(sealed.GetError());
    return Result::Success(assets.Value().catalog);
}

foundation::Result<void> InstallUserSchemas(const ProjectDefinition &project,const ProjectSchema &schema) {
    if(!schema.user_components||schema.user_components!=&schema.components)
        return project.user_schemas.empty()?foundation::Result<void>::Success():foundation::Result<void>::Failure(Error("editor.user_registry_unavailable"));
    std::vector<comp::DescribedComponent> descriptions;for(const auto &type:project.user_schemas)descriptions.push_back(type.component);
    return schema.user_components->RegisterDescriptions(descriptions);
}

foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>>
ValidateProject(const ProjectDefinition &definition,const ProjectSchema &schema) {
    using Result=foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>>;
    if(!schema.entities.IsFrozen()||!schema.components.IsFrozen())return Result::Failure(Error("editor.invalid_project"));
    if(!definition.user_schemas.empty()&&(!schema.user_components||schema.user_components!=&schema.components))return Result::Failure(Error("editor.user_registry_unavailable"));
    std::vector<comp::DescribedComponent> descriptions;for(const auto &type:definition.user_schemas)descriptions.push_back(type.component);
    auto staged=schema.components.WithDescriptions(descriptions);if(!staged)return Result::Failure(staged.GetError());
    ProjectSchema candidate{schema.entities,*staged.Value(),schema.operations,nullptr};
    return ValidatePreparedProject(definition,candidate);
}

foundation::Result<std::unique_ptr<EditorProject>>
EditorProject::Create(std::filesystem::path file, ProjectDefinition definition,
                      const ProjectSchema &schema) {
    using Result = foundation::Result<std::unique_ptr<EditorProject>>;
    const auto valid = ValidateProject(definition, schema);
    if (!valid)
        return Result::Failure(valid.GetError());
    if (file.empty() || file.filename().empty())
        return Result::Failure(Error("editor.project_path"));
    if (file.native().find(std::filesystem::path::value_type{}) !=
        std::filesystem::path::string_type::npos)
        return Result::Failure(Error("editor.project_path"));
    std::error_code error;
    auto absolute = std::filesystem::absolute(file, error);
    if (error)
        return Result::Failure(Error("editor.project_path"));
    auto project =
        std::unique_ptr<EditorProject>(new EditorProject(absolute.lexically_normal(), schema));
    for (auto &document : definition.documents)
        document.revision = project->next_document_++;
    project->RememberIds(definition);
    if (project->used_ids_.size() > 65536)
        return Result::Failure(Error("editor.identity_limit"));
    const auto persistable = detail::ValidateForPersistence(definition, project->used_ids_, schema);
    if (!persistable)
        return Result::Failure(persistable.GetError());
    const auto installed=InstallUserSchemas(definition,schema);if(!installed)return Result::Failure(installed.GetError());
    project->current_ = std::make_shared<const ProjectSnapshot>(
        ProjectSnapshot{std::move(definition), valid.Value(), 1});
    return Result::Success(std::move(project));
}

bool EditorProject::IsDirty(runtime::AssetId document) const {
    const auto *current = current_->Find(document);
    const auto saved = saved_documents_.find(document.Raw());
    return current ? saved == saved_documents_.end() || saved->second != current->revision
                   : saved != saved_documents_.end();
}

} // namespace epidemic::game::editor
