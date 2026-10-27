#include "project_archive.h"
#include "Epidemic/Platform/iplatform_runtime.h"

namespace epidemic::game::editor {
namespace detail {
foundation::Result<void> PublishFile(const std::filesystem::path &path, std::span<const std::byte> bytes) {
    return platform::CommitFilePackage(foundation::Path{path}, bytes, runtime::kMaxDocumentBytes);
}
foundation::Result<std::vector<std::byte>> ReadFile(const std::filesystem::path &path) {
    return platform::ReadFilePackage(foundation::Path{path}, runtime::kMaxDocumentBytes);
}
}
foundation::Result<void> EditorProject::Save() {
    // Prepare all memory and serialization before the atomic platform commit.
    // A failed disk publication never advances the save point.
    std::unordered_map<std::uint64_t, std::uint64_t> documents;
    for (const auto &document : current_->definition.documents) documents.emplace(document.id.Raw(), document.revision);
    auto encoded = detail::WriteProject({current_->definition, {used_ids_.begin(), used_ids_.end()}}, schema_);
    if (!encoded) return foundation::Result<void>::Failure(encoded.GetError());
    const auto published = detail::PublishFile(file_, encoded.Value());
    if (!published) return published;
    saved_documents_.swap(documents); saved_state_ = current_->state_id;
    return foundation::Result<void>::Success();
}
foundation::Result<std::unique_ptr<EditorProject>> EditorProject::Open(const std::filesystem::path &file, const ProjectSchema &schema) {
    using Result = foundation::Result<std::unique_ptr<EditorProject>>;
    if (file.empty() || file.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos)
        return Result::Failure(foundation::Error::Create("editor.project_path", "invalid project path"));
    auto bytes = detail::ReadFile(file); if (!bytes) return Result::Failure(bytes.GetError());
    auto decoded = detail::ReadProject(bytes.Value(), schema); if (!decoded) return Result::Failure(decoded.GetError());
    auto created = Create(file, std::move(decoded.Value().definition), schema); if (!created) return created;
    auto &project = *created.Value();
    // Persist retired identities as well as live ones; malicious omissions cannot
    // remove live IDs because Create has already registered every current ID.
    project.used_ids_.insert(decoded.Value().used_ids.begin(), decoded.Value().used_ids.end());
    project.saved_state_ = project.current_->state_id;
    for (const auto &document : project.current_->definition.documents) project.saved_documents_.emplace(document.id.Raw(), document.revision);
    return created;
}
} // namespace epidemic::game::editor
