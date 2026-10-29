#pragma once
#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/play_session.h"
#include "Epidemic/Game/Editor/project_folder.h"

namespace epidemic::game::editor {
enum class UiElement { Document, SceneNode, PrefabNode, GraphNode, Connection, PrefabComponent };
enum class UiTransformField { PositionX, PositionY, PositionZ, ScaleX, ScaleY, ScaleZ, RotationZDegrees };
struct UiSelection {
    runtime::AssetId document{};
    UiElement kind = UiElement::Document;
    std::uint64_t element = 0;
    // Component selection addresses its owner node plus type/part; other kinds leave these empty.
    gameplay::TypeId component{};
    gameplay::entities::EntityPartId part{};
    [[nodiscard]] bool operator==(const UiSelection &) const = default;
};
// A command client and session owner. It never exposes mutable authoring state.
// Selection uses IDs rather than pointers into snapshots or native tree items.
class EditorWorkspace final {
  public:
    EditorWorkspace(const ProjectSchema &schema, std::function<PlayOptions()> play_options)
        : schema_(schema), play_options_(std::move(play_options)) {}
    [[nodiscard]] foundation::Result<void> Open(const std::filesystem::path &file, bool discard_dirty = false);
    [[nodiscard]] foundation::Result<void> New(const std::filesystem::path &file, ProjectDefinition definition, bool discard_dirty = false);
    [[nodiscard]] foundation::Result<void> OpenFolder(const std::filesystem::path &root, bool discard_dirty = false);
    [[nodiscard]] foundation::Result<void> NewFolder(const std::filesystem::path &root, std::string title, ProjectDefinition definition, bool discard_dirty = false);
    [[nodiscard]] foundation::Result<void> CloneFolder(const std::filesystem::path &destination);
    [[nodiscard]] foundation::Result<void> ImportPng(const std::filesystem::path &source, std::string_view relative);
    [[nodiscard]] foundation::Result<void> ReimportPng(runtime::AssetId asset);
    [[nodiscard]] foundation::Result<void> MoveAsset(runtime::AssetId asset, std::string_view relative);
    [[nodiscard]] foundation::Result<void> UndoAssetMove();
    [[nodiscard]] foundation::Result<void> RedoAssetMove();
    [[nodiscard]] const ProjectFolder *Folder() const noexcept { return folder_.get(); }
    [[nodiscard]] bool IsDirty() const noexcept { return folder_ ? folder_->IsDirty() : project_ && project_->IsDirty(); }
    [[nodiscard]] foundation::Result<void> Execute(std::vector<Command> commands, std::uint64_t expected_revision);
    [[nodiscard]] foundation::Result<void> Save();
    [[nodiscard]] foundation::Result<void> Undo();
    [[nodiscard]] foundation::Result<void> Redo();
    [[nodiscard]] foundation::Result<void> Play();
    [[nodiscard]] foundation::Result<void> Stop();
    [[nodiscard]] foundation::Result<void> Tick(runtime::GameDuration advance);
    [[nodiscard]] foundation::Result<std::size_t> Send(runtime::SceneNodeId placement, const comp::GameEvent &event);
    [[nodiscard]] bool Select(const UiSelection &selection);
    // An owned value must survive Execute/Open and later selection changes.
    // cppcheck-suppress returnByReference
    [[nodiscard]] UiSelection Selection() const noexcept { return selection_; }
    // Resolve a placement to its definition; never reuse a Play entity handle.
    [[nodiscard]] std::optional<UiSelection> PrefabDefinitionSelection() const;
    [[nodiscard]] const EditorProject *Project() const noexcept { return folder_ ? &folder_->Authoring() : project_.get(); }
    [[nodiscard]] const PlaySession *Session() const noexcept { return session_.get(); }
    [[nodiscard]] std::vector<std::shared_ptr<const PlayActivation>> Activations() const;
    [[nodiscard]] const std::optional<foundation::Error> &LastError() const noexcept { return error_; }
    [[nodiscard]] static foundation::Result<comp::PropertyValue> ParseValue(comp::PropertyKind kind, std::string_view text);
    [[nodiscard]] static std::string FormatValue(const comp::PropertyValue &value);
    [[nodiscard]] static foundation::Result<Command> TransformCommand(runtime::AssetId document, runtime::SceneNodeId node,
        const runtime::Transform &current, UiTransformField field, std::string_view text);
  private:
    [[nodiscard]] foundation::Result<void> Adopt(foundation::Result<std::unique_ptr<EditorProject>> candidate);
    [[nodiscard]] foundation::Result<void> Finish(foundation::Result<void> &&result, bool reconcile = false);
    [[nodiscard]] bool Exists(const UiSelection &selection) const;
    void Reconcile();
    ProjectSchema schema_;
    std::function<PlayOptions()> play_options_;
    std::unique_ptr<EditorProject> project_;
    std::unique_ptr<ProjectFolder> folder_;
    std::unique_ptr<PlaySession> session_;
    UiSelection selection_;
    std::optional<foundation::Error> error_;
};
} // namespace epidemic::game::editor
