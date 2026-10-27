#pragma once
#include "Epidemic/Game/Editor/editor_project.h"
#include "Epidemic/Runtime/Serialization/document_codec.h"

namespace epidemic::game::editor::detail {
struct ProjectArchive {
    ProjectDefinition definition;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> used_ids;
};
foundation::Result<std::vector<std::byte>> WriteProject(ProjectArchive project, const ProjectSchema &schema);
foundation::Result<ProjectArchive> ReadProject(std::span<const std::byte> bytes, const ProjectSchema &schema);
foundation::Result<void> PublishFile(const std::filesystem::path &path, std::span<const std::byte> bytes);
foundation::Result<std::vector<std::byte>> ReadFile(const std::filesystem::path &path);
} // namespace epidemic::game::editor::detail
