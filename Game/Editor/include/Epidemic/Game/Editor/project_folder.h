#pragma once

#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/play_session.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace epidemic::game::editor {
struct ProjectSourceAsset {
    runtime::AssetId id{};
    runtime::AssetType type{};
    std::string relative_path;
    std::uint64_t source_hash = 0;
    std::uint64_t generation = 1;
};

// P04.0/P04.1: durable folder workspace. The existing EditorProject is still the
// only authoring model; immutable revision files and a last-published commit
// record provide crash recovery across the multi-file project envelope.
// This is NOT the content importer/browser or a security boundary against
// adversarial filesystem replacement (TOCTOU/reparse-point attacks).
class ProjectFolder final {
  public:
    [[nodiscard]] static foundation::Result<std::unique_ptr<ProjectFolder>> Create(
        const std::filesystem::path &root, std::string title,
        ProjectDefinition definition, const ProjectSchema &schema);
    // Explicit copy of legacy authoring, including retired IDs. Never mutates
    // the source .epidemic file, and refuses an existing destination.
    [[nodiscard]] static foundation::Result<std::unique_ptr<ProjectFolder>> ImportLegacy(
        const std::filesystem::path &root, const std::filesystem::path &legacy,
        std::string title, const ProjectSchema &schema);
    [[nodiscard]] static foundation::Result<std::unique_ptr<ProjectFolder>> Open(
        const std::filesystem::path &root, const ProjectSchema &schema);

    ProjectFolder(const ProjectFolder &) = delete;
    ProjectFolder &operator=(const ProjectFolder &) = delete;
    ~ProjectFolder();

    [[nodiscard]] foundation::Result<void> Save();
    // Clone committed portable data only; refuses pending authoring edits.
    [[nodiscard]] foundation::Result<void> CloneTo(const std::filesystem::path &destination) const;
    [[nodiscard]] foundation::Result<EditReceipt> Execute(const EditRequest &request);
    [[nodiscard]] foundation::Result<void> Undo();
    [[nodiscard]] foundation::Result<void> Redo();
    [[nodiscard]] foundation::Result<std::unique_ptr<PlaySession>> Play(PlayOptions options) const;

    [[nodiscard]] const EditorProject &Authoring() const noexcept { return *editor_; }
    [[nodiscard]] std::shared_ptr<const ProjectSnapshot> Snapshot() const noexcept { return editor_->Snapshot(); }
    [[nodiscard]] const std::filesystem::path &Root() const noexcept { return root_; }
    [[nodiscard]] const std::string &Title() const noexcept { return title_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] bool IsDirty() const noexcept { return asset_dirty_ || editor_->IsDirty(); }
    [[nodiscard]] const std::vector<ProjectSourceAsset> &Sources() const noexcept { return assets_; }
    // P04.1 source indexing (not image decoding/cooking). The caller puts a
    // real file inside Content/ first. Identity is allocated independently
    // from the path; reindex keeps it stable and publishes on Save.
    [[nodiscard]] foundation::Result<runtime::AssetId> IndexSource(
        std::string_view content_relative_path, runtime::AssetType type);
    [[nodiscard]] foundation::Result<void> ReindexSource(runtime::AssetId id);
    // Renames or moves a source within Content while preserving its AssetId.
    // Rejects destination conflicts and does not affect authored references.
    [[nodiscard]] foundation::Result<void> MoveSource(runtime::AssetId id, std::string_view new_content_path);
    // Fresh sealed snapshot; never unseal or mutate Runtime Assets catalog.
    [[nodiscard]] foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>> BuildSourceCatalog() const;
    [[nodiscard]] std::uint64_t Revision() const noexcept { return editor_->Revision(); }
    // Fixed P04.1 mount names. For externally linked files and arbitrary mount
    // aliases a separate future policy and I/O sandbox must be defined.
    [[nodiscard]] foundation::Result<std::filesystem::path> ResolveMount(
        std::string_view mount, std::string_view relative) const;

  private:
    struct WriterLock;
    ProjectFolder(std::filesystem::path root, std::string title, const ProjectSchema &schema);
    [[nodiscard]] static foundation::Result<std::unique_ptr<ProjectFolder>> CreateWithIds(
        const std::filesystem::path &root, std::string title, ProjectDefinition definition,
        const ProjectSchema &schema, const std::vector<std::pair<std::uint64_t, std::uint64_t>> &retired);
    [[nodiscard]] foundation::Result<void> AcquireLock();
    std::filesystem::path root_;
    std::string title_;
    ProjectSchema schema_;
    std::unique_ptr<EditorProject> editor_;
    std::unique_ptr<WriterLock> lock_;
    std::uint64_t generation_ = 0;
    std::uint64_t next_asset_number_ = 1;
    std::vector<ProjectSourceAsset> assets_;
    bool asset_dirty_ = false;
};
} // namespace epidemic::game::editor
