#pragma once
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>
namespace epidemic::runtime::presentation {
std::vector<std::byte> LoadBlob(const std::filesystem::path &path);
void SaveBlob(const std::filesystem::path &path,
              std::span<const std::byte> bytes);
} // namespace epidemic::runtime::presentation
