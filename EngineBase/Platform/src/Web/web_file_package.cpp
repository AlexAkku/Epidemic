#include <Epidemic/Platform/iplatform_runtime.h>
#include <filesystem>
#include <fstream>
#include <limits>

namespace epidemic::platform {
namespace {
foundation::Error Failure(std::string_view code, std::string_view message) {
  return foundation::Error::Create(std::string(code), std::string(message));
}
} // namespace
foundation::Result<std::vector<std::byte>>
ReadFilePackage(const foundation::Path &path, std::size_t max_bytes) {
  const auto file = path.Native();
  std::ifstream input(file, std::ios::binary | std::ios::ate);
  if (!input)
    return foundation::Result<std::vector<std::byte>>::Failure(Failure(
        "platform.package.read_failed", "Cannot open virtual file package"));
  const auto length = input.tellg();
  if (length < 0 || static_cast<std::uint64_t>(length) > max_bytes ||
      static_cast<std::uint64_t>(length) >
          static_cast<std::uint64_t>(
              std::numeric_limits<std::streamsize>::max()))
    return foundation::Result<std::vector<std::byte>>::Failure(
        Failure("platform.package.size_limit",
                "Virtual file package exceeds size budget"));
  std::vector<std::byte> bytes(static_cast<std::size_t>(length));
  input.seekg(0);
  if (!bytes.empty() && !input.read(reinterpret_cast<char *>(bytes.data()),
                                    static_cast<std::streamsize>(bytes.size())))
    return foundation::Result<std::vector<std::byte>>::Failure(Failure(
        "platform.package.short_read", "Virtual file package is truncated"));
  return foundation::Result<std::vector<std::byte>>::Success(std::move(bytes));
}
foundation::Result<void> CommitFilePackage(const foundation::Path &path,
                                           std::span<const std::byte> bytes,
                                           std::size_t max_bytes) {
  if (path.Empty() || bytes.size() > max_bytes ||
      bytes.size() >
          static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
    return foundation::Result<void>::Failure(
        Failure("platform.package.size_limit",
                "Invalid path or virtual package exceeds size budget"));
  const auto file = path.Native();
  auto temporary = file;
  temporary += ".epidemic-tmp";
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output ||
        (!bytes.empty() &&
         !output.write(reinterpret_cast<const char *>(bytes.data()),
                       static_cast<std::streamsize>(bytes.size()))) ||
        !output.flush()) {
      std::error_code ignored;
      std::filesystem::remove(temporary, ignored);
      return foundation::Result<void>::Failure(Failure(
          "platform.package.write_failed", "Virtual package write failed"));
    }
  }
  std::error_code error;
  std::filesystem::rename(temporary, file, error);
  if (error) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return foundation::Result<void>::Failure(
        Failure("platform.package.commit_failed", error.message()));
  }
  return foundation::Result<void>::Success();
}
} // namespace epidemic::platform
