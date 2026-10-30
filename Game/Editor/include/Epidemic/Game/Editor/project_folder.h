#pragma once

#include "Epidemic/Game/Editor/editor_commands.h"
#include "Epidemic/Game/Editor/sprite_sheet.h"
#include "Epidemic/Game/Editor/play_session.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace epidemic::game::editor {
class PreparedCookedPng;
struct AssetEditRequest;
struct ProjectSourceAsset {
    runtime::AssetId id{};
    runtime::AssetType type{};
    std::string relative_path;
    std::uint64_t source_hash = 0;
    std::uint64_t generation = 1;
    // SHA-256 key of immutable serialized image; empty for uncooked sources.
    std::string cooked_key;
    std::string original_path; // Nonempty: staged delete; bytes retained under Content/.Trash.
    [[nodiscard]] bool IsTrashed() const noexcept {return !original_path.empty();}
    [[nodiscard]] bool operator==(const ProjectSourceAsset &) const = default;
};

// P04.3c: named subassets belong to one PNG AssetId, never a filename.
struct SpriteSheetDefinition {
    runtime::AssetId texture{};
    std::vector<sheets::Region> regions;
    [[nodiscard]] bool operator==(const SpriteSheetDefinition &) const = default;
};
// Typed Prefab -> texture assignment. Sprite identity is a stable AssetId,
// not a filename or a generic component string property. The association is
// stored inside the versioned Sources manifest of each immutable commit.
struct PrefabSpriteBinding {
    runtime::AssetId prefab{};
    runtime::AssetId texture{};
    std::uint64_t region_id=0; // 0 draws the complete texture
    float pivot_x=0.5f, pivot_y=0.5f;
    float pixels_per_unit=64.0f;
    [[nodiscard]] bool operator==(const PrefabSpriteBinding &) const = default;
};

// P04.0/P04.1: durable folder workspace. The existing EditorProject is still the
// only authoring model; immutable revision files and a last-published commit
// record provide crash recovery across the multi-file project envelope.
// This is NOT the content importer/browser or a security boundary against
// adversarial filesystem replacement (TOCTOU/reparse-point attacks).
class ProjectFolder final {
    struct VisualState {
        std::vector<PrefabSpriteBinding> sprites;
        std::vector<SpriteSheetDefinition> sheets;
        [[nodiscard]] bool operator==(const VisualState &) const = default;
    };
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
    [[nodiscard]] foundation::Result<void> Execute(const AssetEditRequest &request);
    [[nodiscard]] std::uint64_t AssetRevision() const noexcept { return asset_revision_; }
    [[nodiscard]] bool CanUndo() const noexcept {return edit_cursor_!=0;}
    [[nodiscard]] bool CanRedo() const noexcept {return edit_cursor_<edits_.size();}
    [[nodiscard]] foundation::Result<void> Undo();
    [[nodiscard]] foundation::Result<void> Redo();
    [[nodiscard]] foundation::Result<std::unique_ptr<PlaySession>> Play(PlayOptions options) const;

    [[nodiscard]] const EditorProject &Authoring() const noexcept { return *editor_; }
    [[nodiscard]] std::shared_ptr<const ProjectSnapshot> Snapshot() const noexcept { return editor_->Snapshot(); }
    [[nodiscard]] const std::filesystem::path &Root() const noexcept { return root_; }
    [[nodiscard]] const std::string &Title() const noexcept { return title_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] bool IsDirty() const noexcept { return asset_dirty_ || sprite_dirty_ || editor_->IsDirty(); }
    [[nodiscard]] const std::vector<ProjectSourceAsset> &Sources() const noexcept { return assets_; }
    // Borrowed until the next owner-thread publication; never cache element pointers.
    [[nodiscard]] const std::vector<PrefabSpriteBinding> &SpriteBindings() const noexcept { return visuals_->sprites; }
    [[nodiscard]] const std::vector<SpriteSheetDefinition> &SpriteSheets() const noexcept { return visuals_->sheets; }
    // Whole-image binding remains backward compatible. Region ID 0 means full image.
    [[nodiscard]] foundation::Result<void> SetPrefabSpriteRegion(runtime::AssetId prefab,
        runtime::AssetId texture,std::uint64_t region_id,float pivot_x=0.5f,
        float pivot_y=0.5f,float pixels_per_unit=64.0f);
    [[nodiscard]] foundation::Result<void> SetSpriteSheet(runtime::AssetId texture,
        std::vector<sheets::Region> regions,std::uint32_t image_width,std::uint32_t image_height);
    // Replaces a Prefab's typed texture slot; texture={} clears it.
    // Failed validation never changes the commit candidate.
    [[nodiscard]] foundation::Result<void> SetPrefabSprite(runtime::AssetId prefab,
        runtime::AssetId texture, float pivot_x=0.5f, float pivot_y=0.5f,
        float pixels_per_unit=64.0f);
    // P04.1 source indexing (not image decoding/cooking). The caller puts a
    // real file inside Content/ first. Identity is allocated independently
    // from the path; reindex keeps it stable and publishes on Save.
    [[nodiscard]] foundation::Result<runtime::AssetId> IndexSource(
        std::string_view content_relative_path, runtime::AssetType type, std::string cooked_key = {});
    [[nodiscard]] foundation::Result<void> ReindexSource(runtime::AssetId id);
    [[nodiscard]] foundation::Result<void> PublishCooked(runtime::AssetId id, std::string key);
    // Worker-verified source lease: metadata-only publication, no content I/O.
    [[nodiscard]] foundation::Result<void> PublishCooked(runtime::AssetId id, const PreparedCookedPng &candidate);
    // Renames or moves a source within Content while preserving its AssetId.
    // Rejects destination conflicts and does not affect authored references.
    [[nodiscard]] foundation::Result<void> MoveSource(runtime::AssetId id, std::string_view new_content_path);
    [[nodiscard]] foundation::Result<void> UndoAssetMove();
    [[nodiscard]] foundation::Result<void> RedoAssetMove();
    [[nodiscard]] foundation::Result<void> StageDeleteSource(runtime::AssetId id,bool allow_broken_references=false);
    [[nodiscard]] foundation::Result<void> RestoreSource(runtime::AssetId id);
    [[nodiscard]] foundation::Result<runtime::AssetId> CommitImportedPng(PreparedCookedPng &&candidate,
        std::string_view destination,runtime::AssetId duplicate_of={});
    // Fresh sealed snapshot; never unseal or mutate Runtime Assets catalog.
    [[nodiscard]] foundation::Result<std::shared_ptr<const runtime::IAssetCatalog>> BuildSourceCatalog() const;
    [[nodiscard]] std::uint64_t Revision() const noexcept { return editor_->Revision(); }
    // Fixed P04.1 mount names. For externally linked files and arbitrary mount
    // aliases a separate future policy and I/O sandbox must be defined.
    [[nodiscard]] foundation::Result<std::filesystem::path> ResolveMount(
        std::string_view mount, std::string_view relative) const;

  private:
    struct WriterLock;
    struct MoveRecord { runtime::AssetId id; std::string before, after; };
    struct EditStep {
        enum class Kind {Document,Move,Visual,Lifecycle};Kind kind=Kind::Document;
        MoveRecord move;
        std::shared_ptr<const VisualState> before,after;
        ProjectSourceAsset before_source,after_source;
    };
    [[nodiscard]] std::vector<EditStep> PrepareEdit(EditStep step) const;
    void RefreshVisualDirty() noexcept;
    void RefreshAssetDirty() noexcept;
    [[nodiscard]] foundation::Result<void> RestoreEdit(const EditStep &step,bool forward);
    [[nodiscard]] foundation::Result<void> ApplySourceLifecycle(const ProjectSourceAsset &target);
    [[nodiscard]] foundation::Result<void> ApplySourceMove(runtime::AssetId id, std::string_view relative);
    std::vector<std::pair<std::string,std::string>> pending_moves_;
    std::vector<EditStep> edits_;
    std::size_t edit_cursor_=0;
    std::vector<ProjectSourceAsset> saved_assets_;
    std::shared_ptr<const VisualState> visuals_=std::make_shared<VisualState>();
    std::shared_ptr<const VisualState> saved_visuals_=visuals_;
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
    std::uint64_t asset_revision_=0;
    std::vector<ProjectSourceAsset> assets_;
    bool asset_dirty_ = false;
    bool sprite_dirty_ = false;
};
} // namespace epidemic::game::editor
