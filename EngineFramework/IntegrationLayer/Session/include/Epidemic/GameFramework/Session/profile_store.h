#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace epidemic::gameplay::session {
// Game supplies its validated content schema. Framework owns capture/restore,
// wire validation and checksums; Runtime owns the atomic durable transaction.
std::string LoadProfilePayload(const std::filesystem::path &path);
void SaveProfilePayload(const std::filesystem::path &path, std::string_view payload);
} // namespace epidemic::gameplay::session
