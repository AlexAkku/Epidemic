#pragma once
#include "Epidemic/Game/Editor/editor_workspace.h"

namespace epidemic::game::editor {
struct InspectorEntry {
    std::string name, source, readonly_reason;
    std::optional<comp::PropertyKind> kind;
    std::optional<comp::PropertyValue> value;
    UiSelection owner;
    comp::PropertyId property{};
    runtime::AssetId reference{};
    bool editable=false;
};
// A detached projection of existing definitions/metadata. No factories, mutable
// snapshots, stored borrowed pointers, or second component/asset registry.
[[nodiscard]] std::vector<InspectorEntry> InspectComponents(const ProjectSnapshot &snapshot,
    const UiSelection &selection,const comp::ComponentRegistry &registry,const PlaySession *session=nullptr);
[[nodiscard]] std::string InspectorText(const InspectorEntry &entry);
[[nodiscard]] foundation::Result<Command> InspectorCommand(const InspectorEntry &entry,std::string_view text);
// Recipes contain authored defaults supplied by the host; descriptors and
// validators remain exclusively in ComponentRegistry and the Command pipeline.
[[nodiscard]] foundation::Result<std::vector<Command>> ComponentCommands(const ProjectSnapshot &snapshot,
    const UiSelection &selection,const comp::PrefabComponent &recipe,bool replace=false);
} // namespace epidemic::game::editor
