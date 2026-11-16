#include "Epidemic/Game/Editor/editor_commands.h"
#include "project_archive.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace epidemic::game::editor {
namespace {
using IdSet = std::set<std::pair<std::uint64_t, std::uint64_t>>;

foundation::Error Error(std::string_view code) {
    return foundation::Error::Create(code, "editor command rejected");
}

template <class T, class Predicate> T *Find(std::vector<T> &values, Predicate predicate) {
    const auto at = std::find_if(values.begin(), values.end(), predicate);
    return at == values.end() ? nullptr : &*at;
}

template <class T, class Predicate> bool Erase(std::vector<T> &values, Predicate predicate) {
    return std::erase_if(values, predicate) != 0;
}

comp::PrefabNode *Node(comp::PrefabNode &root, comp::PrefabNodeId id, std::size_t depth = 0) {
    if (root.id == id)
        return &root;
    if (depth >= 32)
        return nullptr;
    for (auto &child : root.children)
        if (auto *node = Node(child, id, depth + 1))
            return node;
    return nullptr;
}

bool Delete(comp::PrefabNode &root, comp::PrefabNodeId id, std::size_t depth = 0) {
    if (Erase(root.children, [&](const auto &child) { return child.id == id; }))
        return true;
    if (depth >= 32)
        return false;
    for (auto &child : root.children)
        if (Delete(child, id, depth + 1))
            return true;
    return false;
}

comp::PrefabComponent *Component(comp::PrefabNode &node, gameplay::TypeId type,
                                 gameplay::entities::EntityPartId part) {
    return Find(node.components,
                [&](const auto &value) { return value.type == type && value.part == part; });
}

void NodeIds(const comp::PrefabNode &node, std::uint64_t doc, IdSet &ids, std::size_t depth = 0) {
    if (!node.id.IsValid() || depth > 32)
        return;
    ids.emplace(doc, node.id.Raw());
    for (const auto &child : node.children)
        NodeIds(child, doc, ids, depth + 1);
}

IdSet Ids(const ProjectDefinition &definition) {
    IdSet ids;
    for (const auto &doc : definition.documents) {
        const auto raw = doc.id.Raw();
        ids.emplace(raw, 0);
        if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc.data))
            NodeIds(prefab->root, raw, ids);
        else if (const auto *scene =
                     std::get_if<composition::SceneCompositionDefinition>(&doc.data))
            for (const auto &node : scene->scene.nodes)
                ids.emplace(raw, node.id.Raw());
        else if (const auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc.data)) {
            for (const auto &node : graph->nodes) {
                ids.emplace(raw, node.id.Raw());
                for (const auto &port : node.ports)
                    ids.emplace(raw, port.id.Raw());
            }

            for (const auto &connection : graph->connections)
                ids.emplace(raw, connection.id.Raw());
        }

    }

    return ids;
}

template <class C> bool Apply(ProjectDefinition &project, const C &command) {
    if constexpr (std::is_same_v<C, RegisterUserSchema>) {
        for (const auto &old : project.user_schemas) {
            if(old.inherit_defaults)continue;
            const auto next=std::find_if(command.schemas.begin(),command.schemas.end(),[&](const auto &v){return v.component.name==old.component.name;});
            if(next==command.schemas.end())continue;
            for(auto &doc:project.documents)if(auto *prefab=std::get_if<comp::PrefabDefinition>(&doc.data)) {
                const auto migrate=[&](auto &self,comp::PrefabNode &node)->void {
                    for(auto &component:node.components)if(component.type==gameplay::TypeId::FromString(old.component.name))
                        std::erase_if(component.properties,[&](const auto &property){
                            const auto previous=std::find_if(old.component.fields.begin(),old.component.fields.end(),[&](const auto &f){return comp::PropertyId::FromString(f.name)==property.property;});
                            const auto updated=std::find_if(next->component.fields.begin(),next->component.fields.end(),[&](const auto &f){return comp::PropertyId::FromString(f.name)==property.property;});
                            return !property.parameter.IsValid()&&previous!=old.component.fields.end()&&updated!=next->component.fields.end()&&property.value==previous->initial;
                        });
                    for(auto &child:node.children)self(self,child);
                };migrate(migrate,prefab->root);
            }
        }
        project.user_schemas = command.schemas;
        for(auto &schema:project.user_schemas)schema.inherit_defaults=true;
        return true;
    } else if constexpr (std::is_same_v<C, CreateDocument>) {
        if (Find(project.documents, [&](const auto &doc) { return doc.id == command.document.id; }))
            return false;
        project.documents.push_back(command.document);
        return true;
    } else if constexpr (std::is_same_v<C, DiscardDocument>) {
        return false; // Requires the native save point; handled by Execute.
    } else if constexpr (std::is_same_v<C, SetEntryScene>) {
        project.entry_scene = command.document;
        return true;
    } else if constexpr (std::is_same_v<C, EditSystem>) {
        const auto matches = [&](const auto &system) { return system.id == command.system.id; };
        auto *existing = Find(project.systems,matches);
        if (command.document != command.system.scene || (existing && existing->scene != command.document))
            return false;
        if (command.remove)
            return existing && Erase(project.systems,matches);
        if (existing)
            *existing = command.system;
        else
            project.systems.push_back(command.system);
        return true;
    } else if constexpr (std::is_same_v<C, EditPrefabLink>) {
        if (command.document != command.link.owner)
            return false;
        const auto matches = [&](const auto &link) { return link.id == command.link.id && link.owner == command.document; };
        if (command.remove)
            return Erase(project.prefab_links,matches);
        if (Find(project.prefab_links,matches))
            return false;
        project.prefab_links.push_back(command.link);
        return true;
    } else if constexpr (std::is_same_v<C, DeleteDocument>) {
        if (project.entry_scene == command.document ||
            !Erase(project.documents, [&](const auto &doc) { return doc.id == command.document; }))
            return false;
        Erase(project.systems, [&](const auto &system) { return system.scene == command.document; });
        Erase(project.prefab_links, [&](const auto &link) { return link.owner == command.document; });
        return true;
    } else if constexpr (std::is_same_v<C, DuplicateDocument>) {
        const auto *source = Find(project.documents, [&](const auto &doc) { return doc.id == command.document; });
        if (!source || Find(project.documents, [&](const auto &doc) { return doc.id == command.copy; }))
            return false;
        auto copy = *source;
        copy.id = command.copy;
        copy.name = command.name;
        copy.revision = 0;
        if (auto *scene = std::get_if<composition::SceneCompositionDefinition>(&copy.data))
            scene->asset_id = copy.id;
        else if (auto *prefab = std::get_if<comp::PrefabDefinition>(&copy.data))
            prefab->id = copy.id.value;
        else {
            auto &graph = std::get<runtime::graph::GraphDefinition>(copy.data);
            graph.asset_id = copy.id;
            graph.name = copy.name;
        }
        const auto scoped_id = [&](foundation::StringId id) {
            return foundation::StringId::FromString("editor.document.copy." +
                std::to_string(command.copy.Raw()) + "." + std::to_string(id.Raw()));
        };
        for (std::size_t i = 0, count = project.systems.size(); i < count; ++i)
            if (auto system = project.systems[i]; system.scene == command.document) {
                system.scene = command.copy;
                system.id = scoped_id(system.id);
                project.systems.push_back(system);
            }
        for (std::size_t i = 0, count = project.prefab_links.size(); i < count; ++i)
            if (auto link = project.prefab_links[i]; link.owner == command.document) {
                link.owner = command.copy;
                link.id = scoped_id(link.id);
                project.prefab_links.push_back(link);
            }
        project.documents.push_back(std::move(copy));
        return true;
    } else {
        auto *doc = Find(project.documents,
                         [&](const auto &value) { return value.id == command.document; });
        if (!doc)
            return false;
        if constexpr (std::is_same_v<C, RenameDocument>) {
            doc->name = command.name;
            return true;
        } else if constexpr (std::is_same_v<C, SetPrefabParameter>) {
            auto *prefab = std::get_if<comp::PrefabDefinition>(&doc->data);
            if (!prefab || !command.parameter.id.IsValid())
                return false;
            auto *parameter = Find(prefab->parameters,
                                   [&](const auto &p) { return p.id == command.parameter.id; });
            if (parameter) {
                if (parameter->value.index() != command.parameter.value.index())
                    return false;
                *parameter = command.parameter;
            } else
                prefab->parameters.push_back(command.parameter);
            return true;
        } else if constexpr (std::is_same_v<C, AddEntity> || std::is_same_v<C, DeleteEntity> ||
                             std::is_same_v<C, ReparentPrefabNode> ||
                             std::is_same_v<C, SetComponent> ||
                             std::is_same_v<C, DeleteComponent> || std::is_same_v<C, SetProperty> ||
                             std::is_same_v<C, SetBehavior> || std::is_same_v<C, DeleteBehavior>) {
            auto *prefab = std::get_if<comp::PrefabDefinition>(&doc->data);
            if (!prefab)
                return false;
            if constexpr (std::is_same_v<C, ReparentPrefabNode>) {
                auto *source = Node(prefab->root,command.node);
                if (!source || prefab->root.id == command.node || Node(*source,command.parent) ||
                    !Node(prefab->root,command.parent))
                    return false;
                auto subtree = *source;
                const auto detach = [&](auto &&self, comp::PrefabNode &parent) -> bool {
                    if (Erase(parent.children,[&](const auto &child) {return child.id == command.node;}))
                        return true;
                    for (auto &child : parent.children)
                        if (self(self,child))
                            return true;
                    return false;
                };
                if (!detach(detach,prefab->root))
                    return false;
                Node(prefab->root,command.parent)->children.push_back(std::move(subtree));
                return true;
            } else if constexpr (std::is_same_v<C, AddEntity>) {
                if (!command.parent.IsValid()) {
                    if (prefab->root.id.IsValid())
                        return false;
                    prefab->root = command.node;
                    return true;
                }

                auto *parent = Node(prefab->root, command.parent);
                if (!parent)
                    return false;
                parent->children.push_back(command.node);
                return true;
            } else if constexpr (std::is_same_v<C, DeleteEntity>) {
                if (prefab->root.id == command.node) {
                    prefab->root = {};

                    return true;
                }

                return Delete(prefab->root, command.node);
            } else {
                auto *node = Node(prefab->root, command.node);
                if (!node)
                    return false;
                if constexpr (std::is_same_v<C, SetComponent>) {
                    auto *component =
                        Component(*node, command.component.type, command.component.part);
                    if (component)
                        *component = command.component;
                    else
                        node->components.push_back(command.component);
                    return true;
                } else if constexpr (std::is_same_v<C, DeleteComponent>) {
                    return Erase(node->components, [&](const auto &value) {
                        return value.type == command.type && value.part == command.part;
                    });
                } else if constexpr (std::is_same_v<C, SetProperty>) {
                    auto *component = Component(*node, command.component, command.part);
                    if (!component)
                        return false;
                    auto *property = Find(component->properties, [&](const auto &value) {
                        return value.property == command.property.property;
                    });
                    if (property)
                        *property = command.property;
                    else
                        component->properties.push_back(command.property);
                    return true;
                } else {
                    auto *behaviors = &node->behaviors;
                    if (command.component.IsValid()) {
                        auto *component = Component(*node, command.component, command.part);
                        if (!component)
                            return false;
                        behaviors = &component->behaviors;
                    } else if (command.part.IsValid())
                        return false;
                    if constexpr (std::is_same_v<C, SetBehavior>) {
                        auto *behavior = Find(*behaviors, [&](const auto &value) {
                            return value.id == command.behavior.id;
                        });
                        if (behavior)
                            *behavior = command.behavior;
                        else
                            behaviors->push_back(command.behavior);
                        return true;
                    } else
                        return Erase(*behaviors, [&](const auto &value) {
                            return value.id == command.behavior;
                        });
                }

            }

        } else if constexpr (std::is_same_v<C, AddGraphNode> ||
                             std::is_same_v<C, DeleteGraphNode> ||
                             std::is_same_v<C, ConnectPorts> ||
                             std::is_same_v<C, DisconnectPorts> || std::is_same_v<C, MoveGraphNode> ||
                             std::is_same_v<C, SetGraphNodeSettings> || std::is_same_v<C, SetGraphPortDefault>) {
            auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc->data);
            if (!graph)
                return false;
            if constexpr (std::is_same_v<C, SetGraphPortDefault>) {
                for(auto &node:graph->nodes)for(auto &port:node.ports)if(port.id==command.port){port.default_value=command.value;return true;}return false;
            } else if constexpr (std::is_same_v<C, AddGraphNode>) {
                graph->nodes.push_back(command.node);
                return true;
            } else if constexpr (std::is_same_v<C, MoveGraphNode>) {
                if (!std::isfinite(command.x) || !std::isfinite(command.y) ||
                    std::abs(command.x) > 1000000 || std::abs(command.y) > 1000000 ||
                    !Find(graph->nodes, [&](const auto &node) { return node.id == command.node; }))
                    return false;
                auto *position = Find(doc->graph_layout, [&](const auto &p) { return p.node == command.node.Raw(); });
                const GraphNodePosition value{command.node.Raw(), command.x, command.y};
                if (position) *position = value;
                else doc->graph_layout.push_back(value);
                return true;
            } else if constexpr (std::is_same_v<C, SetGraphNodeSettings> || std::is_same_v<C, SetGraphPortDefault>) {
                auto *node = Find(graph->nodes, [&](const auto &n) { return n.id == command.node; });
                if (!node || command.settings.node != command.node.Raw() || command.name.empty() ||
                    command.name.size() > 1024 || command.settings.comment.size() > 1024 || command.settings.group.size() > 128)
                    return false;
                node->name = command.name;
                auto *settings = Find(doc->graph_settings, [&](const auto &n) { return n.node == command.node.Raw(); });
                if (settings) *settings = command.settings;
                else doc->graph_settings.push_back(command.settings);
                return true;
            } else if constexpr (std::is_same_v<C, ConnectPorts>) {
                graph->connections.push_back(command.connection);
                return true;
            } else if constexpr (std::is_same_v<C, DisconnectPorts> || std::is_same_v<C, MoveGraphNode>)
                return Erase(graph->connections,
                             [&](const auto &value) { return value.id == command.connection; });
            else {
                auto *node =
                    Find(graph->nodes, [&](const auto &value) { return value.id == command.node; });
                if (!node)
                    return false;
                const auto ports = node->ports;
                std::erase_if(doc->graph_layout, [&](const auto &p) { return p.node == command.node.Raw(); });
                std::erase_if(doc->graph_settings, [&](const auto &p) { return p.node == command.node.Raw(); });
                Erase(graph->connections, [&](const auto &edge) {
                    return std::any_of(ports.begin(), ports.end(), [&](const auto &port) {
                        return port.id == edge.input_port || port.id == edge.output_port;
                    });
                });
                return Erase(graph->nodes,
                             [&](const auto &value) { return value.id == command.node; });
            }

        } else {
            auto *scene = std::get_if<composition::SceneCompositionDefinition>(&doc->data);
            if (!scene)
                return false;
            if constexpr (std::is_same_v<C, AddSceneNode>) {
                scene->scene.nodes.push_back(command.node);
                return true;
            } else if constexpr (std::is_same_v<C, SetComponentOverride> ||
                                 std::is_same_v<C, SetParameterOverride>) {
                auto *placement = Find(scene->placements,
                                       [&](const auto &p) { return p.node == command.placement; });
                if (!placement || placement->prefab != command.prefab)
                    return false;
                if constexpr (std::is_same_v<C, SetParameterOverride>) {
                    const auto matches = [&](const auto &p) {
                        return p.property == command.value.property;
                    };

                    if (command.reset)
                        return Erase(placement->overrides.parameters, matches);
                    auto *value = Find(placement->overrides.parameters, matches);
                    if (value)
                        *value = command.value;
                    else
                        placement->overrides.parameters.push_back(command.value);
                } else {
                    const auto matches = [&](const auto &p) {
                        return p.node == command.value.node &&
                               p.component == command.value.component &&
                               p.part == command.value.part &&
                               p.property.property == command.value.property.property;
                    };

                    if (command.reset)
                        return Erase(placement->overrides.components, matches);
                    auto *value = Find(placement->overrides.components, matches);
                    if (value)
                        *value = command.value;
                    else
                        placement->overrides.components.push_back(command.value);
                }

                return true;
            } else if constexpr (std::is_same_v<C, PlacePrefab>) {
                auto *placement = Find(scene->placements, [&](const auto &value) {
                    return value.node == command.placement.node;
                });
                if (placement)
                    *placement = command.placement;
                else
                    scene->placements.push_back(command.placement);
                return true;
            } else if constexpr (std::is_same_v<C, OrderSceneNode>) {
                auto &nodes = scene->scene.nodes;
                const auto source = std::ranges::find(nodes,command.node,&runtime::SceneNodeSnapshot::id);
                const auto before = command.before.IsValid()
                    ? std::ranges::find(nodes,command.before,&runtime::SceneNodeSnapshot::id) : nodes.end();
                if (source == nodes.end() ||
                    (command.before.IsValid() && (before == nodes.end() || before->parent != source->parent)))
                    return false;
                if (source < before)
                    std::rotate(source,source+1,before);
                else if (before < source)
                    std::rotate(before,source,source+1);
                return true;
            } else {
                auto *node = Find(scene->scene.nodes,
                                  [&](const auto &value) { return value.id == command.node; });
                if (!node)
                    return false;
                if constexpr (std::is_same_v<C, SetSceneParent>) {
                    node->parent = command.parent;
                    return true;
                } else if constexpr (std::is_same_v<C, RenameSceneNode>) {
                    if (command.name.empty() || command.name.size() > 256 ||
                        command.name.find_first_not_of(" \t\r\n") == std::string::npos ||
                        command.name.find_first_of("\t\r\n") != std::string::npos)
                        return false;
                    scene->node_names.insert_or_assign(node->id.Raw(), command.name);
                    return true;
                } else if constexpr (std::is_same_v<C, SetSceneTransform>) {
                    node->local_transform = command.transform;
                    return true;
                } else {
                    std::set<std::uint64_t> removed{command.node.Raw()};

                    for (std::size_t i = 0; i < scene->scene.nodes.size(); ++i)
                        for (const auto &item : scene->scene.nodes)
                            if (removed.contains(item.parent.Raw()))
                                removed.insert(item.id.Raw());
                    Erase(scene->placements,
                          [&](const auto &value) { return removed.contains(value.node.Raw()); });
                    Erase(scene->scene.nodes,
                          [&](const auto &value) { return removed.contains(value.id.Raw()); });
                    for (const auto id : removed)
                        scene->node_names.erase(id);
                    return true;
                }

            }

        }
    }
}

} // namespace

void EditorProject::RememberIds(const ProjectDefinition &definition) {
    const auto ids = Ids(definition);
    used_ids_.insert(ids.begin(), ids.end());
}

foundation::Result<EditReceipt> EditorProject::Execute(const EditRequest &request) {
    using Result = foundation::Result<EditReceipt>;
    if (request.expected_revision && request.expected_revision != epoch_)
        return Result::Failure(Error("editor.stale_revision"));
    if (request.commands.empty() || request.commands.size() > 256 ||
        epoch_ == std::numeric_limits<std::uint64_t>::max())
        return Result::Failure(Error("editor.transaction_limit"));
    auto candidate = current_->definition;
    auto used = used_ids_;
    auto before = Ids(candidate);
    std::set<std::uint64_t> changed;
    std::set<std::uint64_t> discarded;
    for (const auto &command : request.commands) {
        if(const auto *registration=std::get_if<RegisterUserSchema>(&command))
            for(const auto &old:candidate.user_schemas) {
                const auto next=std::find_if(registration->schemas.begin(),registration->schemas.end(),[&](const auto &v){return v.component.name==old.component.name;});
                if(next==registration->schemas.end())continue;
                for(const auto &field:old.component.fields) {
                    const auto match=std::find_if(next->component.fields.begin(),next->component.fields.end(),[&](const auto &f){return f.name==field.name;});
                    if(match==next->component.fields.end()||match->initial.index()!=field.initial.index())return Result::Failure(Error("component.migration_required"));
                }
            }
        const auto *discard = std::get_if<DiscardDocument>(&command);
        auto allowed_restored_ids = IdSet{};
        bool applied = false;
        if (discard) {
            if (!saved_snapshot_ || request.commands.size() != 1)
                return Result::Failure(Error("editor.discard_scope"));
            const auto *saved = saved_snapshot_->Find(discard->document);
            auto *live = Find(candidate.documents,
                              [&](const auto &doc) { return doc.id == discard->document; });
            if (!live)
                return Result::Failure(Error("editor.command_target"));
            std::erase_if(candidate.systems,[&](const auto &s) { return s.scene == discard->document; });
            std::erase_if(candidate.prefab_links,[&](const auto &link) { return link.owner == discard->document; });
            if (saved) {
                *live = *saved;
                if (saved->Kind() == DocumentKind::Scene) {
                    for (const auto &system : saved_snapshot_->definition.systems)
                        if (system.scene == discard->document)
                            candidate.systems.push_back(system);
                }
                if (saved->Kind() == DocumentKind::Prefab)
                    for (const auto &link : saved_snapshot_->definition.prefab_links)
                        if (link.owner == discard->document)
                            candidate.prefab_links.push_back(link);
                allowed_restored_ids = Ids(saved_snapshot_->definition);
            } else {
                std::erase_if(candidate.documents,
                              [&](const auto &doc) { return doc.id == discard->document; });
            }
            discarded.insert(discard->document.Raw());
            applied = true;
        } else {
            applied =
                std::visit([&](const auto &value) { return Apply(candidate, value); }, command);
        }
        if (!applied)
            return Result::Failure(Error("editor.command_target"));
        std::visit(
            [&](const auto &value) {
                using C = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<C, RegisterUserSchema>) {
                    for(const auto &doc:candidate.documents)if(doc.Kind()==DocumentKind::Prefab)changed.insert(doc.id.Raw());
                } else if constexpr (std::is_same_v<C, CreateDocument>)
                    changed.insert(value.document.id.Raw());
                else if constexpr (std::is_same_v<C, DuplicateDocument>)
                    changed.insert(value.copy.Raw());
                else if constexpr (!std::is_same_v<C, SetEntryScene>)
                    changed.insert(value.document.Raw());
            },
            command);
        auto after = Ids(candidate);
        for (const auto &id : after)
            if (!before.contains(id) && !used.insert(id).second &&
                !allowed_restored_ids.contains(id))
                return Result::Failure(Error("editor.retired_id"));
        before = std::move(after);
        if (used.size() > 65536)
            return Result::Failure(Error("editor.identity_limit"));
    }

    std::stable_sort(candidate.systems.begin(),candidate.systems.end(),
                     [](const auto &a,const auto &b) { return a.scene.Raw() < b.scene.Raw(); });
    std::stable_sort(candidate.prefab_links.begin(),candidate.prefab_links.end(),
                     [](const auto &a,const auto &b) { return a.owner.Raw() < b.owner.Raw(); });
    const auto valid = ValidateProject(candidate, schema_);
    if (!valid)
        return Result::Failure(valid.GetError());
    const auto persistable = detail::ValidateForPersistence(candidate, used, schema_);
    if (!persistable)
        return Result::Failure(persistable.GetError());
    const auto installed=InstallUserSchemas(candidate,schema_);
    if(!installed)return Result::Failure(installed.GetError());
    auto document_revision = next_document_;
    for (auto &doc : candidate.documents)
        if (changed.contains(doc.id.Raw()) && !discarded.contains(doc.id.Raw()))
            doc.revision = document_revision++;
    // Discarding the last dirty document returns to a clean native save point;
    // folder asset/visual dirty state remains owned by ProjectFolder.
    bool at_save_point = !discarded.empty() && saved_snapshot_ &&
                         candidate.entry_scene == saved_snapshot_->definition.entry_scene &&
                         candidate.systems == saved_snapshot_->definition.systems &&
                         candidate.prefab_links == saved_snapshot_->definition.prefab_links &&
                         candidate.user_schemas == saved_snapshot_->definition.user_schemas &&
                         candidate.documents.size() == saved_documents_.size();
    if (at_save_point)
        for (const auto &doc : candidate.documents) {
            const auto saved = saved_documents_.find(doc.id.Raw());
            if (saved == saved_documents_.end() || saved->second != doc.revision)
                at_save_point = false;
        }
    auto snapshot = std::make_shared<const ProjectSnapshot>(
        ProjectSnapshot{std::move(candidate), valid.Value(),
                        at_save_point ? saved_state_ : next_state_});
    EditReceipt receipt{epoch_ + 1, {}};

    for (const auto id : changed)
        receipt.changed_documents.push_back(runtime::AssetId{foundation::StringId{id}});
    undo_.push_back(current_);
    if (undo_.size() > 64)
        undo_.erase(undo_.begin());
    current_ = std::move(snapshot);
    used_ids_ = std::move(used);
    redo_.clear();
    next_document_ = document_revision;
    ++next_state_;
    ++epoch_;
    return Result::Success(std::move(receipt));
}

foundation::Result<void> EditorProject::Undo() {
    if (undo_.empty())
        return foundation::Result<void>::Failure(Error("editor.no_undo"));
    const auto installed=InstallUserSchemas(undo_.back()->definition,schema_);if(!installed)return installed;
    redo_.push_back(current_);
    current_ = undo_.back();
    undo_.pop_back();
    ++epoch_;
    return foundation::Result<void>::Success();
}

foundation::Result<void> EditorProject::Redo() {
    if (redo_.empty())
        return foundation::Result<void>::Failure(Error("editor.no_redo"));
    const auto installed=InstallUserSchemas(redo_.back()->definition,schema_);if(!installed)return installed;
    undo_.push_back(current_);
    current_ = redo_.back();
    redo_.pop_back();
    ++epoch_;
    return foundation::Result<void>::Success();
}

} // namespace epidemic::game::editor
