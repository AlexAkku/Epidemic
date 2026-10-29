#pragma once
#include "Epidemic/Foundation/result.h"
#include <filesystem>
#include <string_view>
namespace epidemic::game::editor {
// Resolve portable project-relative paths without I/O side effects. Existing
// symlinks are rejected. Callers must still protect the subsequent
// open/commit against replacement races; this is not an atomic filesystem sandbox.
[[nodiscard]] foundation::Result<std::filesystem::path>
ResolveProjectPath(const std::filesystem::path &root, std::string_view relative);
}
