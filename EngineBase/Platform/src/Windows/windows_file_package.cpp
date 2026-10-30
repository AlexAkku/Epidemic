#include "Epidemic/Foundation/error.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <exception>
#include <string>
#include <system_error>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
#include "shadow_probe.h"
#endif

namespace epidemic::platform {
namespace detail {
thread_local std::string_view atomic_file_fault_point;
void SetAtomicFileFaultPointForTesting(std::string_view point) {
  atomic_file_fault_point = point;
}
void ClearAtomicFileFaultPointForTesting() noexcept {
  atomic_file_fault_point = {};
}
} // namespace detail
namespace {
bool Fault(std::string_view point) noexcept {
  return detail::atomic_file_fault_point == point;
}
foundation::Error FileError(std::string_view phase,
                            const std::filesystem::path &path,
                            unsigned long os = 0) {
  auto context = std::string(phase) + "; path=" + path.string();
  if (os)
    context += "; os=" + std::to_string(os) + " (" +
               std::system_category().message(static_cast<int>(os)) + ")";
  return foundation::Error::Create("platform.file." + std::string(phase),
                                   "bounded package I/O failed", context);
}
struct FileHandle {
  HANDLE value = INVALID_HANDLE_VALUE;
  ~FileHandle() {
    if (value != INVALID_HANDLE_VALUE)
      CloseHandle(value);
  }
};
struct TempFile {
  std::filesystem::path path;
  FileHandle file;
  bool owned = false;
  ~TempFile() {
    if (file.value != INVALID_HANDLE_VALUE) {
      CloseHandle(file.value);
      file.value = INVALID_HANDLE_VALUE;
    }
    // Never delete a colliding pre-existing file: ownership starts only
    // after CREATE_NEW succeeds and covers this exact temp path.
    if (owned)
      DeleteFileW(path.c_str());
  }
};
std::atomic<std::uint64_t> temp_serial{0};
} // namespace
foundation::Result<std::vector<std::byte>>
ReadFilePackage(const foundation::Path &input, std::size_t max_bytes) {
  const auto &path = input.Native();
  try {
    if (input.Empty() || max_bytes == 0)
      return foundation::Result<std::vector<std::byte>>::Failure(
          FileError("invalid_read", path));
    FileHandle file;
    file.value =
        CreateFileW(path.c_str(), GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.value == INVALID_HANDLE_VALUE)
      return foundation::Result<std::vector<std::byte>>::Failure(
          FileError("open_read", path, GetLastError()));
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file.value, &size))
      return foundation::Result<std::vector<std::byte>>::Failure(
          FileError("size", path, GetLastError()));
    if (size.QuadPart < 0 ||
        static_cast<std::uint64_t>(size.QuadPart) > max_bytes)
      return foundation::Result<std::vector<std::byte>>::Failure(
          FileError("size_limit", path));
    std::vector<std::byte> bytes(static_cast<std::size_t>(size.QuadPart));
    std::size_t position = 0;
    while (position < bytes.size()) {
      const auto count = static_cast<DWORD>(
          std::min<std::size_t>(bytes.size() - position, 1024U * 1024U));
      DWORD received = 0;
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
      auditor::shadow::delay_io(path.c_str());
#endif
      if (!ReadFile(file.value, bytes.data() + position, count, &received,
                    nullptr) ||
          received == 0)
        return foundation::Result<std::vector<std::byte>>::Failure(
            FileError("read", path, GetLastError()));
      position += received;
    }
    std::byte extra{};
    DWORD received = 0;
    if (!ReadFile(file.value, &extra, 1, &received, nullptr) || received != 0)
      return foundation::Result<std::vector<std::byte>>::Failure(
          FileError("read_changed_size", path, GetLastError()));
    return foundation::Result<std::vector<std::byte>>::Success(
        std::move(bytes));
  } catch (const std::exception &error) {
    return foundation::Result<std::vector<std::byte>>::Failure(
        foundation::Error::Create("platform.file.exception",
                                  "package read exception", error.what()));
  }
}
foundation::Result<void> CommitFilePackage(const foundation::Path &input,
                                           std::span<const std::byte> package,
                                           std::size_t max_bytes) {
  const auto &path = input.Native();
  try {
    if (input.Empty() || max_bytes == 0 || package.size() > max_bytes)
      return foundation::Result<void>::Failure(
          FileError("invalid_commit", path));
    TempFile temporary;
    temporary.path = path;
    temporary.path += ".tmp-" + std::to_string(GetCurrentProcessId()) + "-" +
                      std::to_string(temp_serial.fetch_add(1));
    if (Fault("open"))
      return foundation::Result<void>::Failure(
          FileError("temp_open_injected", path));
    temporary.file.value =
        CreateFileW(temporary.path.c_str(), GENERIC_WRITE, 0, nullptr,
                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (temporary.file.value == INVALID_HANDLE_VALUE)
      return foundation::Result<void>::Failure(
          FileError("temp_open", temporary.path, GetLastError()));
    temporary.owned = true;
    std::size_t position = 0;
    while (position < package.size()) {
      const auto count = static_cast<DWORD>(std::min<std::size_t>(
          package.size() - position, Fault("write") ? 17 : 1024 * 1024));
      DWORD written = 0;
      if (!WriteFile(temporary.file.value, package.data() + position, count,
                     &written, nullptr) ||
          written == 0)
        return foundation::Result<void>::Failure(
            FileError("temp_write", temporary.path, GetLastError()));
      position += written;
      if (Fault("write"))
        return foundation::Result<void>::Failure(
            FileError("temp_write_injected", temporary.path));
    }
    if (Fault("flush"))
      return foundation::Result<void>::Failure(
          FileError("temp_flush_injected", temporary.path));
    if (!FlushFileBuffers(temporary.file.value))
      return foundation::Result<void>::Failure(
          FileError("temp_flush", temporary.path, GetLastError()));
    if (!CloseHandle(temporary.file.value))
      return foundation::Result<void>::Failure(
          FileError("temp_close", temporary.path, GetLastError()));
    temporary.file.value = INVALID_HANDLE_VALUE;
    if (Fault("interrupt"))
      std::_Exit(75); // explicit helper-child test seam
    if (Fault("replace"))
      return foundation::Result<void>::Failure(
          FileError("replace_injected", path));
    if (!MoveFileExW(temporary.path.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
      return foundation::Result<void>::Failure(
          FileError("replace", path, GetLastError()));
    temporary.owned = false;
    return foundation::Result<void>::Success();
  } catch (const std::exception &error) {
    return foundation::Result<void>::Failure(foundation::Error::Create(
        "platform.file.exception", "package commit exception", error.what()));
  }
}
} // namespace epidemic::platform
