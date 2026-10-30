#pragma once
#include "Epidemic/Game/Editor/editor_workspace.h"

namespace epidemic::game::editor {
enum class InspectorTarget { Property, Parameter, ComponentOverride, ParameterOverride };

struct InspectorEntry {
    std::string name, source, readonly_reason;
    std::optional<comp::PropertyKind> kind;
    std::optional<comp::PropertyValue> value;
    UiSelection owner;
    comp::PropertyId property{};

    runtime::AssetId reference{};

    bool editable = false;
    InspectorTarget target = InspectorTarget::Property;
    UiSelection placement;
    comp::PropertyId binding{};

    std::optional<comp::PropertyValue> definition_value;
    bool overridden = false;
};

// A detached projection of existing definitions/metadata. No factories, mutable
// snapshots, stored borrowed pointers, or second component/asset registry.
[[nodiscard]] std::vector<InspectorEntry> InspectComponents(const ProjectSnapshot &snapshot,
                                                            const UiSelection &selection,
                                                            const comp::ComponentRegistry &registry,
                                                            const PlaySession *session = nullptr,
                                                            bool edit_overrides = false);
[[nodiscard]] std::string InspectorText(const InspectorEntry &entry);
[[nodiscard]] foundation::Result<Command> InspectorCommand(const InspectorEntry &entry,
                                                           std::string_view text);
[[nodiscard]] foundation::Result<Command> InspectorResetCommand(const InspectorEntry &entry);
// A valid new ID exposes a property as a parameter in one transaction. An
// invalid ID detaches its existing binding, retaining the definition value.
[[nodiscard]] foundation::Result<std::vector<Command>>
InspectorBinding(const InspectorEntry &entry, comp::PropertyId parameter = {});
// Recipes contain authored defaults supplied by the host; descriptors and
// validators remain exclusively in ComponentRegistry and the Command pipeline.
[[nodiscard]] foundation::Result<std::vector<Command>>
ComponentCommands(const ProjectSnapshot &snapshot, const UiSelection &selection,
                  const comp::PrefabComponent &recipe, bool replace = false);
} // namespace epidemic::game::editor
