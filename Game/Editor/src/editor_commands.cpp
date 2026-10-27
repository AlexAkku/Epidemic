#include "Epidemic/Game/Editor/editor_commands.h"
#include <algorithm>
#include <limits>
#include <type_traits>

namespace epidemic::game::editor {
namespace {
using IdSet = std::set<std::pair<std::uint64_t, std::uint64_t>>;
foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "editor command rejected"); }
template<class T, class Predicate> T *Find(std::vector<T> &values, Predicate predicate) {
    const auto at = std::find_if(values.begin(), values.end(), predicate);
    return at == values.end() ? nullptr : &*at;
}
template<class T, class Predicate> bool Erase(std::vector<T> &values, Predicate predicate) {
    return std::erase_if(values, predicate) != 0;
}
comp::PrefabNode *Node(comp::PrefabNode &root, comp::PrefabNodeId id, std::size_t depth = 0) {
    if (root.id == id) return &root;
    if (depth >= 32) return nullptr;
    for (auto &child : root.children) if (auto *node = Node(child, id, depth + 1)) return node;
    return nullptr;
}
bool Delete(comp::PrefabNode &root, comp::PrefabNodeId id, std::size_t depth = 0) {
    if (Erase(root.children, [&](const auto &child) { return child.id == id; })) return true;
    if (depth >= 32) return false;
    for (auto &child : root.children) if (Delete(child, id, depth + 1)) return true;
    return false;
}
comp::PrefabComponent *Component(comp::PrefabNode &node, gameplay::TypeId type, gameplay::entities::EntityPartId part) {
    return Find(node.components, [&](const auto &value) { return value.type == type && value.part == part; });
}
void NodeIds(const comp::PrefabNode &node, std::uint64_t doc, IdSet &ids, std::size_t depth = 0) {
    if (!node.id.IsValid() || depth > 32) return;
    ids.emplace(doc, node.id.Raw());
    for (const auto &child : node.children) NodeIds(child, doc, ids, depth + 1);
}
IdSet Ids(const ProjectDefinition &definition) {
    IdSet ids;
    for (const auto &doc : definition.documents) {
        const auto raw = doc.id.Raw(); ids.emplace(raw, 0);
        if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc.data)) NodeIds(prefab->root, raw, ids);
        else if (const auto *scene = std::get_if<composition::SceneCompositionDefinition>(&doc.data))
            for (const auto &node : scene->scene.nodes) ids.emplace(raw, node.id.Raw());
        else if (const auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc.data)) {
            for (const auto &node : graph->nodes) {
                ids.emplace(raw, node.id.Raw());
                for (const auto &port : node.ports) ids.emplace(raw, port.id.Raw());
            }
            for (const auto &connection : graph->connections) ids.emplace(raw, connection.id.Raw());
        }
    }
    return ids;
}
template<class C> bool Apply(ProjectDefinition &project, const C &command) {
    if constexpr (std::is_same_v<C, CreateDocument>) {
        if (Find(project.documents, [&](const auto &doc) { return doc.id == command.document.id; })) return false;
        project.documents.push_back(command.document); return true;
    } else if constexpr (std::is_same_v<C, SetEntryScene>) {
        project.entry_scene = command.document; return true;
    } else if constexpr (std::is_same_v<C, DeleteDocument>) {
        return Erase(project.documents, [&](const auto &doc) { return doc.id == command.document; });
    } else {
        auto *doc = Find(project.documents, [&](const auto &value) { return value.id == command.document; });
        if (!doc) return false;
        if constexpr (std::is_same_v<C, RenameDocument>) { doc->name = command.name; return true; }
        else if constexpr (std::is_same_v<C, AddEntity> || std::is_same_v<C, DeleteEntity> ||
            std::is_same_v<C, SetComponent> || std::is_same_v<C, DeleteComponent> || std::is_same_v<C, SetProperty> ||
            std::is_same_v<C, SetBehavior> || std::is_same_v<C, DeleteBehavior>) {
            auto *prefab = std::get_if<comp::PrefabDefinition>(&doc->data); if (!prefab) return false;
            if constexpr (std::is_same_v<C, AddEntity>) {
                if (!command.parent.IsValid()) { if (prefab->root.id.IsValid()) return false; prefab->root = command.node; return true; }
                auto *parent = Node(prefab->root, command.parent); if (!parent) return false;
                parent->children.push_back(command.node); return true;
            } else if constexpr (std::is_same_v<C, DeleteEntity>) {
                if (prefab->root.id == command.node) { prefab->root = {}; return true; }
                return Delete(prefab->root, command.node);
            } else {
                auto *node = Node(prefab->root, command.node); if (!node) return false;
                if constexpr (std::is_same_v<C, SetComponent>) {
                    auto *component = Component(*node, command.component.type, command.component.part);
                    if (component) *component = command.component; else node->components.push_back(command.component);
                    return true;
                } else if constexpr (std::is_same_v<C, DeleteComponent>) {
                    return Erase(node->components, [&](const auto &value) { return value.type == command.type && value.part == command.part; });
                } else if constexpr (std::is_same_v<C, SetProperty>) {
                    auto *component = Component(*node, command.component, command.part); if (!component) return false;
                    auto *property = Find(component->properties, [&](const auto &value) { return value.property == command.property.property; });
                    if (property) *property = command.property; else component->properties.push_back(command.property);
                    return true;
                } else {
                    auto *behaviors = &node->behaviors;
                    if (command.component.IsValid()) {
                        auto *component = Component(*node, command.component, command.part); if (!component) return false;
                        behaviors = &component->behaviors;
                    } else if (command.part.IsValid()) return false;
                    if constexpr (std::is_same_v<C, SetBehavior>) {
                        auto *behavior = Find(*behaviors, [&](const auto &value) { return value.id == command.behavior.id; });
                        if (behavior) *behavior = command.behavior; else behaviors->push_back(command.behavior); return true;
                    } else return Erase(*behaviors, [&](const auto &value) { return value.id == command.behavior; });
                }
            }
        } else if constexpr (std::is_same_v<C, AddGraphNode> || std::is_same_v<C, DeleteGraphNode> ||
            std::is_same_v<C, ConnectPorts> || std::is_same_v<C, DisconnectPorts>) {
            auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc->data); if (!graph) return false;
            if constexpr (std::is_same_v<C, AddGraphNode>) { graph->nodes.push_back(command.node); return true; }
            else if constexpr (std::is_same_v<C, ConnectPorts>) { graph->connections.push_back(command.connection); return true; }
            else if constexpr (std::is_same_v<C, DisconnectPorts>)
                return Erase(graph->connections, [&](const auto &value) { return value.id == command.connection; });
            else {
                auto *node = Find(graph->nodes, [&](const auto &value) { return value.id == command.node; }); if (!node) return false;
                const auto ports = node->ports;
                Erase(graph->connections, [&](const auto &edge) {
                    return std::any_of(ports.begin(), ports.end(), [&](const auto &port) { return port.id == edge.input_port || port.id == edge.output_port; });
                });
                return Erase(graph->nodes, [&](const auto &value) { return value.id == command.node; });
            }
        } else {
            auto *scene = std::get_if<composition::SceneCompositionDefinition>(&doc->data); if (!scene) return false;
            if constexpr (std::is_same_v<C, AddSceneNode>) { scene->scene.nodes.push_back(command.node); return true; }
            else if constexpr (std::is_same_v<C, PlacePrefab>) {
                auto *placement = Find(scene->placements, [&](const auto &value) { return value.node == command.placement.node; });
                if (placement) *placement = command.placement; else scene->placements.push_back(command.placement); return true;
            } else {
                auto *node = Find(scene->scene.nodes, [&](const auto &value) { return value.id == command.node; }); if (!node) return false;
                if constexpr (std::is_same_v<C, SetSceneParent>) { node->parent = command.parent; return true; }
                else if constexpr (std::is_same_v<C, SetSceneTransform>) { node->local_transform = command.transform; return true; }
                else {
                    std::set<std::uint64_t> removed{command.node.Raw()};
                    for (std::size_t i = 0; i < scene->scene.nodes.size(); ++i)
                        for (const auto &item : scene->scene.nodes) if (removed.contains(item.parent.Raw())) removed.insert(item.id.Raw());
                    Erase(scene->placements, [&](const auto &value) { return removed.contains(value.node.Raw()); });
                    Erase(scene->scene.nodes, [&](const auto &value) { return removed.contains(value.id.Raw()); }); return true;
                }
            }
        }
    }
}
}
void EditorProject::RememberIds(const ProjectDefinition &definition) { const auto ids = Ids(definition); used_ids_.insert(ids.begin(), ids.end()); }
foundation::Result<EditReceipt> EditorProject::Execute(const EditRequest &request) {
    using Result = foundation::Result<EditReceipt>;
    if (request.expected_revision && request.expected_revision != epoch_) return Result::Failure(Error("editor.stale_revision"));
    if (request.commands.empty() || request.commands.size() > 256 || epoch_ == std::numeric_limits<std::uint64_t>::max())
        return Result::Failure(Error("editor.transaction_limit"));
    auto candidate = current_->definition;
    auto used = used_ids_; auto before = Ids(candidate);
    std::set<std::uint64_t> changed;
    for (const auto &command : request.commands) {
        if (!std::visit([&](const auto &value) { return Apply(candidate, value); }, command)) return Result::Failure(Error("editor.command_target"));
        std::visit([&](const auto &value) {
            using C = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<C, CreateDocument>) changed.insert(value.document.id.Raw());
            else if constexpr (!std::is_same_v<C, SetEntryScene>) changed.insert(value.document.Raw());
        }, command);
        auto after = Ids(candidate);
        for (const auto &id : after) if (!before.contains(id) && !used.insert(id).second) return Result::Failure(Error("editor.retired_id"));
        before = std::move(after);
    }
    const auto valid = ValidateProject(candidate, schema_); if (!valid) return Result::Failure(valid.GetError());
    auto document_revision = next_document_;
    for (auto &doc : candidate.documents) if (changed.contains(doc.id.Raw())) doc.revision = document_revision++;
    auto snapshot = std::make_shared<const ProjectSnapshot>(ProjectSnapshot{std::move(candidate), valid.Value(), next_state_});
    EditReceipt receipt{epoch_ + 1, {}}; for (const auto id : changed) receipt.changed_documents.push_back(runtime::AssetId{foundation::StringId{id}});
    undo_.push_back(current_); if (undo_.size() > 64) undo_.erase(undo_.begin());
    current_ = std::move(snapshot); used_ids_ = std::move(used); redo_.clear();
    next_document_ = document_revision; ++next_state_; ++epoch_;
    return Result::Success(std::move(receipt));
}
foundation::Result<void> EditorProject::Undo() {
    if (undo_.empty()) return foundation::Result<void>::Failure(Error("editor.no_undo"));
    redo_.push_back(current_); current_ = undo_.back(); undo_.pop_back(); ++epoch_;
    return foundation::Result<void>::Success();
}
foundation::Result<void> EditorProject::Redo() {
    if (redo_.empty()) return foundation::Result<void>::Failure(Error("editor.no_redo"));
    undo_.push_back(current_); current_ = redo_.back(); redo_.pop_back(); ++epoch_;
    return foundation::Result<void>::Success();
}
} // namespace epidemic::game::editor
