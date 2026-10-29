#pragma once
#include "Epidemic/Game/Composition/graph_host.h"
#include "Epidemic/Game/Composition/scene_composition.h"
#include "Epidemic/Runtime/Assets/asset_services.h"
#include <filesystem>
#include <variant>
#include <set>

namespace epidemic::game::editor {
namespace comp = gameplay::composition;
struct EditRequest;
struct EditReceipt;
struct PlayOptions;
class PlaySession;
class ProjectFolder;
using DocumentData = std::variant<composition::SceneCompositionDefinition, comp::PrefabDefinition, runtime::graph::GraphDefinition>;
enum class DocumentKind { Scene, Prefab, Graph };
struct EditorDocument {
    runtime::AssetId id{};
    std::string name;
    std::uint64_t revision = 0;
    DocumentData data;
    [[nodiscard]] DocumentKind Kind() const noexcept { return static_cast<DocumentKind>(data.index()); }
};
struct ProjectDefinition {
    foundation::StringId id{};
    runtime::AssetId entry_scene{};
    std::vector<EditorDocument> documents;
};
struct ProjectSchema {
    gameplay::entities::EntityService &entities;
    const comp::ComponentRegistry &components;
    std::vector<std::pair<runtime::graph::TypeId, runtime::graph::Operation>> operations;
};
struct ProjectSnapshot {
    ProjectDefinition definition;
    std::shared_ptr<const runtime::IAssetCatalog> assets;
    std::uint64_t state_id = 0;
    [[nodiscard]] const EditorDocument *Find(runtime::AssetId id) const noexcept;
};
[[nodiscard]] foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>>
ValidateProject(const ProjectDefinition &definition, const ProjectSchema &schema);

class EditorProject final {
  public:
    [[nodiscard]] static foundation::Result<std::unique_ptr<EditorProject>> Create(
        std::filesystem::path file, ProjectDefinition definition, const ProjectSchema &schema);
    [[nodiscard]] static foundation::Result<std::unique_ptr<EditorProject>> Open(
        const std::filesystem::path &file, const ProjectSchema &schema);
    [[nodiscard]] foundation::Result<void> Save();
    [[nodiscard]] foundation::Result<std::unique_ptr<PlaySession>> Play(PlayOptions options) const;
    EditorProject(const EditorProject &) = delete;
    EditorProject &operator=(const EditorProject &) = delete;
    [[nodiscard]] std::shared_ptr<const ProjectSnapshot> Snapshot() const noexcept { return current_; }
    [[nodiscard]] const std::filesystem::path &File() const noexcept { return file_; }
    [[nodiscard]] bool IsDirty() const noexcept { return current_->state_id != saved_state_; }
    [[nodiscard]] bool IsDirty(runtime::AssetId document) const;
    [[nodiscard]] std::uint64_t Revision() const noexcept { return epoch_; }
    [[nodiscard]] foundation::Result<EditReceipt> Execute(const EditRequest &request);
    [[nodiscard]] foundation::Result<void> Undo();
    [[nodiscard]] foundation::Result<void> Redo();
    [[nodiscard]] bool CanUndo() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool CanRedo() const noexcept { return !redo_.empty(); }
  private:
    friend class ProjectFolder; // Folder commits preserve retired IDs and save points.
    EditorProject(std::filesystem::path file, const ProjectSchema &schema) : file_(std::move(file)), schema_(schema) {}
    std::filesystem::path file_;
    ProjectSchema schema_;
    std::shared_ptr<const ProjectSnapshot> current_;
    std::uint64_t epoch_ = 1, next_state_ = 2, next_document_ = 1, saved_state_ = 0;
    std::unordered_map<std::uint64_t, std::uint64_t> saved_documents_;
    std::vector<std::shared_ptr<const ProjectSnapshot>> undo_, redo_;
    // IDs are scoped by document, shared by all element kinds, and never reused
    // after deletion or after branching history. Undo restores snapshots verbatim.
    std::set<std::pair<std::uint64_t, std::uint64_t>> used_ids_;
    void RememberIds(const ProjectDefinition &definition);
};
} // namespace epidemic::game::editor
