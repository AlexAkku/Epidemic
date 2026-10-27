#include "Epidemic/Game/Editor/editor_project.h"
#include <algorithm>
#include <unordered_set>

namespace epidemic::game::editor {
namespace {
foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "editor document rejected"); }
const EditorDocument *Find(const ProjectDefinition &definition, runtime::AssetId id) {
    const auto found = std::find_if(definition.documents.begin(), definition.documents.end(), [&](const auto &document) { return document.id == id; });
    return found == definition.documents.end() ? nullptr : &*found;
}
class References final : public comp::IGraphBehaviorHost {
  public:
    References(const ProjectDefinition &definition, const ProjectSchema &schema) : definition_(definition), schema_(schema) {}
    foundation::Result<void> CheckDefinition(const comp::GraphBehaviorDefinition &definition) const override {
        const auto *document = Find(definition_, runtime::AssetId{definition.graph});
        if (!document || document->Kind() != DocumentKind::Graph) return foundation::Result<void>::Failure(Error("editor.graph_reference"));
        const auto &graph = std::get<runtime::graph::GraphDefinition>(document->data);
        const auto has_port = [&](foundation::StringId id, runtime::graph::Operation operation) {
            for (const auto &node : graph.nodes) {
                const auto binding = std::find_if(schema_.operations.begin(), schema_.operations.end(), [&](const auto &item) { return item.first == node.type_id; });
                if (binding != schema_.operations.end() && binding->second == operation)
                    for (const auto &port : node.ports) if (port.id == id &&
                        (operation == runtime::graph::Operation::EventEntry ? port.kind == runtime::graph::PortKind::Event && port.direction == runtime::graph::PortDirection::Input
                                                                         : port.kind == runtime::graph::PortKind::Data && port.direction == runtime::graph::PortDirection::Output)) return true;
            }
            return false;
        };
        for (const auto &event : definition.events) if (!has_port(event.port, runtime::graph::Operation::EventEntry))
            return foundation::Result<void>::Failure(Error("editor.port_reference"));
        for (const auto &input : definition.inputs) if (!has_port(input.port, runtime::graph::Operation::Input))
            return foundation::Result<void>::Failure(Error("editor.port_reference"));
        return foundation::Result<void>::Success();
    }
    foundation::Result<std::shared_ptr<const comp::IGraphProgram>> Bind(const std::shared_ptr<comp::GraphScope> &, const comp::GraphBehaviorDefinition &) override {
        return foundation::Result<std::shared_ptr<const comp::IGraphProgram>>::Failure(Error("editor.authoring_only"));
    }
    foundation::Result<std::shared_ptr<comp::IGraphActivation>> Dispatch(const std::shared_ptr<const comp::IGraphProgram> &, const comp::GameEvent &) override {
        return foundation::Result<std::shared_ptr<comp::IGraphActivation>>::Failure(Error("editor.authoring_only"));
    }
  private:
    const ProjectDefinition &definition_;
    const ProjectSchema &schema_;
};
void Dependencies(const comp::PrefabNode &node, std::vector<runtime::AssetDependency> &dependencies) {
    for (const auto &behavior : node.behaviors) dependencies.push_back({runtime::AssetId{behavior.graph}, true});
    for (const auto &component : node.components) for (const auto &behavior : component.behaviors) dependencies.push_back({runtime::AssetId{behavior.graph}, true});
    for (const auto &child : node.children) Dependencies(child, dependencies);
}
}
const EditorDocument *ProjectSnapshot::Find(runtime::AssetId id) const noexcept { return editor::Find(definition, id); }
foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>> ValidateProject(const ProjectDefinition &definition, const ProjectSchema &schema) {
    using Result = foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>>;
    if (!definition.id.IsValid() || definition.documents.size() > 64 || !schema.entities.IsFrozen() || !schema.components.IsFrozen())
        return Result::Failure(Error("editor.invalid_project"));
    std::unordered_set<std::uint64_t> ids;
    std::unordered_set<std::uint64_t> operation_types;
    if (schema.operations.size() > 256) return Result::Failure(Error("editor.operation_schema"));
    std::vector<runtime::graph::TypeId> types{runtime::graph::kI64Type};
    for (const auto &[type, operation] : schema.operations) {
        if (!type.IsValid() || !operation_types.insert(type.Raw()).second || operation > runtime::graph::Operation::Repeat)
            return Result::Failure(Error("editor.operation_schema"));
        types.push_back(type);
    }
    References references{definition, schema};
    comp::GraphBehaviorService behaviors{references};
    comp::ComponentStore components{schema.entities, schema.components};
    comp::PrefabComposer prefabs{schema.entities, schema.components, components, behaviors};
    auto assets = runtime::CreateAssetServices();
    if (!assets) return Result::Failure(assets.GetError());
    for (const auto &document : definition.documents) {
        if (!document.id.IsValid() || document.name.empty() || document.name.size() > 256 || !ids.insert(document.id.Raw()).second)
            return Result::Failure(Error("editor.document_identity"));
        if (document.Kind() == DocumentKind::Graph) {
            const auto &graph = std::get<runtime::graph::GraphDefinition>(document.data);
            if (graph.asset_id != document.id) return Result::Failure(Error("editor.document_identity"));
            const auto valid = runtime::graph::ValidateGraph(graph, types);
            if (!valid) return Result::Failure(valid.GetError());
        } else if (document.Kind() == DocumentKind::Prefab) {
            const auto &prefab = std::get<comp::PrefabDefinition>(document.data);
            if (prefab.id != document.id.value) return Result::Failure(Error("editor.document_identity"));
            const auto valid = prefabs.Register(prefab);
            if (!valid) return Result::Failure(valid.GetError());
        }
    }
    for (const auto &document : definition.documents) {
        runtime::AssetMetadata metadata;
        metadata.id = document.id; metadata.state = runtime::AssetState::Validated; metadata.version = 1;
        metadata.location.kind = runtime::AssetLocationKind::VirtualPath;
        metadata.location.mount_id = definition.id;
        metadata.location.path = "document/" + std::to_string(document.id.Raw());
        if (document.Kind() == DocumentKind::Scene) {
            const auto &scene = std::get<composition::SceneCompositionDefinition>(document.data);
            if (scene.asset_id != document.id) return Result::Failure(Error("editor.document_identity"));
            const auto validated = composition::CreateSceneComposition(scene);
            if (!validated) return Result::Failure(validated.GetError());
            std::unordered_set<std::uint64_t> placed;
            for (const auto &placement : scene.placements) {
                const auto *prefab = Find(definition, placement.prefab);
                if (!validated.Value().nodes.contains(placement.node) || !placed.insert(placement.node.Raw()).second ||
                    !prefab || prefab->Kind() != DocumentKind::Prefab) return Result::Failure(Error("editor.prefab_reference"));
                const auto overrides = prefabs.Validate(placement.prefab.value, placement.overrides);
                if (!overrides) return Result::Failure(overrides.GetError());
                metadata.dependencies.push_back({placement.prefab, true});
            }
            metadata.type = {foundation::StringId::FromString("editor.scene")};
        } else if (document.Kind() == DocumentKind::Prefab) {
            Dependencies(std::get<comp::PrefabDefinition>(document.data).root, metadata.dependencies);
            metadata.type = {foundation::StringId::FromString("editor.prefab")};
        } else metadata.type = {foundation::StringId::FromString("editor.graph")};
        const auto registered = assets.Value().writer->RegisterAsset(metadata);
        if (!registered) return Result::Failure(registered.GetError());
    }
    if (definition.entry_scene.IsValid()) {
        const auto *entry = Find(definition, definition.entry_scene);
        if (!entry || entry->Kind() != DocumentKind::Scene) return Result::Failure(Error("editor.entry_scene"));
    }
    const auto sealed = assets.Value().writer->Seal();
    if (!sealed) return Result::Failure(sealed.GetError());
    return Result::Success(assets.Value().catalog);
}
foundation::Result<std::unique_ptr<EditorProject>> EditorProject::Create(std::filesystem::path file,
    ProjectDefinition definition, const ProjectSchema &schema) {
    using Result = foundation::Result<std::unique_ptr<EditorProject>>;
    const auto valid = ValidateProject(definition, schema);
    if (!valid) return Result::Failure(valid.GetError());
    if (file.empty() || file.filename().empty()) return Result::Failure(Error("editor.project_path"));
    if (file.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos)
        return Result::Failure(Error("editor.project_path"));
    std::error_code error;
    auto absolute = std::filesystem::absolute(file, error);
    if (error) return Result::Failure(Error("editor.project_path"));
    auto project = std::unique_ptr<EditorProject>(new EditorProject(absolute.lexically_normal(), schema));
    for (auto &document : definition.documents) document.revision = project->next_document_++;
    project->RememberIds(definition);
    if (project->used_ids_.size() > 65536) return Result::Failure(Error("editor.identity_limit"));
    project->current_ = std::make_shared<const ProjectSnapshot>(ProjectSnapshot{std::move(definition), valid.Value(), 1});
    return Result::Success(std::move(project));
}
bool EditorProject::IsDirty(runtime::AssetId document) const {
    const auto *current = current_->Find(document); const auto saved = saved_documents_.find(document.Raw());
    return current ? saved == saved_documents_.end() || saved->second != current->revision : saved != saved_documents_.end();
}
} // namespace epidemic::game::editor
