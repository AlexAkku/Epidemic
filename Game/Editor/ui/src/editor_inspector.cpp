#include "Epidemic/Game/Editor/editor_inspector.h"
#include <algorithm>

namespace epidemic::game::editor {
namespace {
const comp::PrefabNode *Node(const comp::PrefabNode &root, std::uint64_t id) {
    if (root.id.Raw() == id)
        return &root;
    for (const auto &child : root.children)
        if (const auto *found = Node(child, id))
            return found;
    return nullptr;
}

std::vector<const comp::PrefabNode *> Nodes(const comp::PrefabNode &root) {
    std::vector<const comp::PrefabNode *> result{&root};

    for (std::size_t i = 0; i < result.size(); ++i)
        for (const auto &child : result[i]->children)
            result.push_back(&child);
    return result;
}

std::string Id(std::uint64_t id) {
    return std::to_string(id);
}

bool SingleLine(const comp::PropertyValue &value) {
    const auto *text = std::get_if<std::string>(&value);
    return !text || std::none_of(text->begin(), text->end(),
                                 [](char c) { return c == '\0' || c == '\r' || c == '\n'; });
}

void Behaviors(std::vector<InspectorEntry> &rows, const ProjectSnapshot &snapshot,
               const std::vector<comp::GraphBehaviorDefinition> &behaviors, std::string_view prefix,
               bool effective = false) {
    const std::string_view source = effective ? "effective authoring" : "definition";
    for (const auto &behavior : behaviors) {
        const runtime::AssetId asset{behavior.graph};

        const auto *doc = snapshot.Find(asset);
        InspectorEntry row;
        row.name = std::string(prefix) + "GraphBehavior [" + Id(behavior.id.Raw()) + "]";
        row.source = source;
        row.readonly_reason = "Graph reference; use the Graph picker to assign";
        row.value = (doc ? doc->name : "Missing Graph") + " [" + Id(asset.Raw()) + "]";
        row.reference = asset;
        rows.push_back(std::move(row));
        for (const auto &route : behavior.events) {
            InspectorEntry event;
            event.name = std::string(prefix) + "Event [" + Id(route.event.Raw()) + "]";
            event.source = source;
            event.readonly_reason = "Event route";
            event.value = Id(route.port.Raw());
            rows.push_back(std::move(event));
        }

    }

}

void Properties(std::vector<InspectorEntry> &rows, const comp::ComponentRegistry &registry,
                const comp::PrefabDefinition &definition, const comp::PrefabNode &node,
                const comp::PrefabComponent &component, const comp::PrefabOverrides *overrides,
                runtime::AssetId document, bool editable, std::string_view prefix) {
    const auto *info = registry.Find(component.type);
    if (!info)
        return;
    for (const auto &property : info->properties) {
        InspectorEntry row;
        row.name = std::string(prefix) + info->name;
        if (component.part.IsValid())
            row.name += " / part [" + Id(component.part.Raw()) + "]";
        row.name += " / " + property.name;
        row.kind = property.kind;
        row.property = property.id;
        row.owner = {document, UiElement::PrefabComponent, node.id.Raw(), component.type,
                     component.part};

        row.editable = editable;
        row.source = "factory default unavailable";
        if (!editable)
            row.readonly_reason = "Effective authoring value; choose Edit Instance Override or "
                                  "open its Prefab definition";
        const auto authored =
            std::find_if(component.properties.begin(), component.properties.end(),
                         [&](const auto &p) { return p.property == property.id; });
        if (authored != component.properties.end()) {
            row.value = authored->value;
            row.source = "definition";
            if (authored->parameter.IsValid()) {
                row.value.reset();
                row.source = "parameter [" + Id(authored->parameter.Raw()) + "]";
                row.binding = authored->parameter;
                row.editable = false;
                row.readonly_reason =
                    "Bound property; edit its parameter default or detach binding";
                for (const auto &parameter : definition.parameters)
                    if (parameter.id == authored->parameter)
                        row.value = parameter.value;
            }

        }

        row.definition_value = row.value;
        if (overrides && row.binding.IsValid())
            for (const auto &parameter : overrides->parameters)
                if (parameter.property == row.binding) {
                    row.value = parameter.value;
                    row.source = "parameter override [" + Id(row.binding.Raw()) + "]";
                }

        if (overrides)
            for (const auto &override : overrides->components)
                if (override.node == node.id && override.component == component.type &&
                    override.part == component.part && override.property.property == property.id) {
                    row.value = override.property.value;
                    row.source = "instance override";
                    row.overridden = true;
                }

        if (row.value && !SingleLine(*row.value)) {
            row.editable = false;
            row.readonly_reason = "Single-line Inspector cannot edit NUL/multiline strings";
        }

        rows.push_back(std::move(row));
    }

}

void Parameters(std::vector<InspectorEntry> &rows, const comp::PrefabDefinition &definition,
                runtime::AssetId document,
                const composition::ScenePrefabPlacement *placement = nullptr) {
    for (const auto &parameter : definition.parameters) {
        InspectorEntry row;
        row.name = "Parameter [" + Id(parameter.id.Raw()) + "]";
        row.source = "definition parameter";
        row.owner = {document};

        row.property = parameter.id;
        row.value = parameter.value;
        row.definition_value = parameter.value;
        row.kind = static_cast<comp::PropertyKind>(parameter.value.index());
        row.target = InspectorTarget::Parameter;
        row.editable = SingleLine(parameter.value);
        if (!row.editable)
            row.readonly_reason = "Single-line Inspector cannot edit NUL/multiline strings";
        if (placement) {
            row.target = InspectorTarget::ParameterOverride;
            for (const auto &value : placement->overrides.parameters)
                if (value.property == parameter.id) {
                    row.value = value.value;
                    row.source = "instance parameter override";
                    row.overridden = true;
                }

        }

        rows.push_back(std::move(row));
    }

}

} // namespace

std::vector<InspectorEntry> InspectComponents(const ProjectSnapshot &snapshot,
                                              const UiSelection &selection,
                                              const comp::ComponentRegistry &registry,
                                              const PlaySession *session, bool edit_overrides) {
    std::vector<InspectorEntry> rows;
    const auto *doc = snapshot.Find(selection.document);
    if (!doc)
        return rows;
    if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc->data)) {
        if (const auto *node = Node(prefab->root, selection.element)) {
            for (const auto &component : node->components) {
                if (selection.kind == UiElement::PrefabComponent &&
                    (component.type != selection.component || component.part != selection.part))
                    continue;
                Properties(rows, registry, *prefab, *node, component, nullptr, doc->id, true, "");
                Behaviors(rows, snapshot, component.behaviors,
                          "Component [" + Id(component.type.Raw()) + " / " +
                              Id(component.part.Raw()) + "] / ");
            }

            if (selection.kind != UiElement::PrefabComponent)
                Behaviors(rows, snapshot, node->behaviors, "");
        }

        Parameters(rows, *prefab, doc->id);
    } else if (const auto *scene = std::get_if<composition::SceneCompositionDefinition>(&doc->data);
               scene && selection.kind == UiElement::SceneNode) {
        for (const auto &placement : scene->placements)
            if (placement.node.Raw() == selection.element) {
                const auto *prefab_doc = snapshot.Find(placement.prefab);
                const auto *definition =
                    prefab_doc ? std::get_if<comp::PrefabDefinition>(&prefab_doc->data) : nullptr;
                const auto first = rows.size();
                if (definition) {
                    Parameters(rows, *definition, prefab_doc->id, &placement);
                    for (const auto *node : Nodes(definition->root)) {
                        const auto prefix = "Node [" + Id(node->id.Raw()) + "] / ";
                        for (const auto &component : node->components) {
                            Properties(rows, registry, *definition, *node, component,
                                       &placement.overrides, prefab_doc->id, false, prefix);
                            Behaviors(rows, snapshot, component.behaviors,
                                      prefix + "Component [" + Id(component.type.Raw()) + " / " +
                                          Id(component.part.Raw()) + "] / ",
                                      true);
                        }

                        Behaviors(rows, snapshot, node->behaviors, prefix, true);
                    }

                }

                for (auto i = first; i < rows.size(); ++i)
                    if (rows[i].kind) {
                        auto &row = rows[i];
                        row.placement = selection;
                        if (row.target != InspectorTarget::ParameterOverride)
                            row.target = InspectorTarget::ComponentOverride;
                        row.editable = edit_overrides && (!row.value || SingleLine(*row.value));
                        if (row.editable)
                            row.readonly_reason.clear();
                        else if (!edit_overrides)
                            row.readonly_reason = "View Definition; choose Edit Instance Override "
                                                  "to change only this placement";
                    }

            }

        const auto *instance = session && session->IsPlaying()
                                   ? session->Instance(runtime::SceneNodeId{selection.element})
                                   : nullptr;
        if (instance)
            for (const auto *node : Nodes(instance->Definition()->root))
                for (const auto &component : node->components) {
                    const auto *info = registry.Find(component.type);
                    if (!info)
                        continue;
                    for (const auto &property : info->properties) {
                        InspectorEntry row;
                        row.name = "Runtime node [" + Id(node->id.Raw()) + "] / " + info->name +
                                   " / " + property.name;
                        if (component.part.IsValid())
                            row.name += " / part [" + Id(component.part.Raw()) + "]";
                        row.source = "runtime snapshot";
                        row.kind = property.kind;
                        row.property = property.id;
                        row.owner = {selection.document, UiElement::SceneNode, selection.element,
                                     component.type, component.part};

                        row.readonly_reason = "Runtime values are read-only; refresh explicitly";
                        const auto value =
                            session->ReadComponentProperty(runtime::SceneNodeId{selection.element},
                                                           node->id, component.type, component.part,
                                                           property.id);
                        if (value)
                            row.value = value.Value();
                        else
                            row.readonly_reason = value.GetError().code;
                        rows.push_back(std::move(row));
                    }

                }

    }

    return rows;
}

std::string InspectorText(const InspectorEntry &entry) {
    if (!entry.value)
        return entry.editable ? "" : "<value unavailable>";
    auto text = EditorWorkspace::FormatValue(*entry.value);
    std::string result;
    for (const char c : text) {
        if (c == '\0')
            result += "\\0";
        else if (c == '\r')
            result += "\\r";
        else if (c == '\n')
            result += "\\n";
        else
            result += c;
    }

    return result;
}

foundation::Result<Command> InspectorCommand(const InspectorEntry &entry, std::string_view text) {
    using Result = foundation::Result<Command>;
    if (!entry.editable || !entry.kind)
        return Result::Failure(
            foundation::Error::Create("editor.inspector_readonly",
                                      "This value is read-only: " + entry.readonly_reason));
    const auto value = EditorWorkspace::ParseValue(*entry.kind, text);
    if (!value)
        return Result::Failure(value.GetError());
    if (!SingleLine(value.Value()))
        return Result::Failure(
            foundation::Error::Create("editor.inspector_string",
                                      "Use single-line text without NUL in this Inspector."));
    if (entry.target == InspectorTarget::Parameter)
        return Result::Success(
            SetPrefabParameter{entry.owner.document, {entry.property, value.Value()}});
    if (entry.target == InspectorTarget::ParameterOverride)
        return Result::Success(SetParameterOverride{entry.placement.document,
                                                    runtime::SceneNodeId{entry.placement.element},
                                                    entry.owner.document,
                                                    {entry.property, value.Value()}});
    if (entry.target == InspectorTarget::ComponentOverride)
        return Result::Success(SetComponentOverride{entry.placement.document,
                                                    runtime::SceneNodeId{entry.placement.element},
                                                    entry.owner.document,
                                                    {comp::PrefabNodeId{entry.owner.element},
                                                     entry.owner.component,
                                                     entry.owner.part,
                                                     {entry.property, value.Value()}}});
    return Result::Success(SetProperty{entry.owner.document,
                                       comp::PrefabNodeId{entry.owner.element},
                                       entry.owner.component,
                                       entry.owner.part,
                                       {entry.property, value.Value(), {}}});
}

foundation::Result<Command> InspectorResetCommand(const InspectorEntry &entry) {
    using Result = foundation::Result<Command>;
    if (!entry.overridden || !entry.placement.document.IsValid())
        return Result::Failure(foundation::Error::Create("editor.no_override",
                                                         "Select an overridden authoring field."));
    if (entry.target == InspectorTarget::ParameterOverride)
        return Result::Success(SetParameterOverride{entry.placement.document,
                                                    runtime::SceneNodeId{entry.placement.element},
                                                    entry.owner.document,
                                                    {entry.property, std::int64_t{}},
                                                    true});
    if (entry.target == InspectorTarget::ComponentOverride)
        return Result::Success(SetComponentOverride{entry.placement.document,
                                                    runtime::SceneNodeId{entry.placement.element},
                                                    entry.owner.document,
                                                    {comp::PrefabNodeId{entry.owner.element},
                                                     entry.owner.component,
                                                     entry.owner.part,
                                                     {entry.property, std::int64_t{}}},
                                                    true});
    return Result::Failure(
        foundation::Error::
            Create("editor.no_override",
                   "Runtime and definition values cannot be reset as placement overrides."));
}

foundation::Result<std::vector<Command>> InspectorBinding(const InspectorEntry &entry,
                                                          comp::PropertyId parameter) {
    using Result = foundation::Result<std::vector<Command>>;
    if (entry.target != InspectorTarget::Property ||
        entry.owner.kind != UiElement::PrefabComponent || !entry.value || !entry.kind ||
        (parameter.IsValid() ? (!entry.editable || entry.binding.IsValid())
                             : !entry.binding.IsValid()))
        return Result::Failure(
            foundation::Error::Create("editor.parameter_binding",
                                      "Select an authored property to expose, or a bound "
                                      "definition property to detach."));
    std::vector<Command> commands;
    if (parameter.IsValid())
        commands.emplace_back(SetPrefabParameter{entry.owner.document, {parameter, *entry.value}});
    commands.emplace_back(SetProperty{entry.owner.document,
                                      comp::PrefabNodeId{entry.owner.element},
                                      entry.owner.component,
                                      entry.owner.part,
                                      {entry.property, *entry.value, parameter}});
    return Result::Success(std::move(commands));
}

foundation::Result<std::vector<Command>> ComponentCommands(const ProjectSnapshot &snapshot,
                                                           const UiSelection &selection,
                                                           const comp::PrefabComponent &recipe,
                                                           bool replace) {
    using Result = foundation::Result<std::vector<Command>>;
    const auto failure = [] {
        return Result::Failure(
            foundation::Error::Create("editor.component_target",
                                      "Select an Entity to add or a Component to replace; target "
                                      "slot must be unused and type must differ."));
    };

    const auto *doc = snapshot.Find(selection.document);
    const auto *prefab = doc ? std::get_if<comp::PrefabDefinition>(&doc->data) : nullptr;
    const auto *node = prefab ? Node(prefab->root, selection.element) : nullptr;
    if (!node || (replace ? selection.kind != UiElement::PrefabComponent
                          : selection.kind != UiElement::PrefabNode))
        return failure();
    auto candidate = recipe;
    if (replace)
        candidate.part = selection.part;
    const auto occupied =
        std::any_of(node->components.begin(), node->components.end(), [&](const auto &c) {
            return c.type == candidate.type && c.part == candidate.part;
        });
    if (occupied || !candidate.type.IsValid())
        return failure();
    std::vector<Command> commands;
    if (replace) {
        const auto old =
            std::find_if(node->components.begin(), node->components.end(), [&](const auto &c) {
                return c.type == selection.component && c.part == selection.part;
            });
        if (old == node->components.end())
            return failure();
        commands.emplace_back(
            DeleteComponent{doc->id, node->id, selection.component, selection.part});
    }

    commands.emplace_back(SetComponent{doc->id, node->id, std::move(candidate)});
    return Result::Success(std::move(commands));
}

} // namespace epidemic::game::editor
