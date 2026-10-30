#pragma once
#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/archive_value.h"
#include <cstddef>
#include <span>
#include <vector>

namespace epidemic::runtime {
// Canonical, little-endian archive-tree encoding. Object fields are sorted;
// version, checksum, exact length, depth and allocation limits are checked.
inline constexpr std::size_t kMaxDocumentBytes = std::size_t{16} * 1024 * 1024;
[[nodiscard]] foundation::Result<std::vector<std::byte>>
EncodeDocument(const SerializedDocument &document);
[[nodiscard]] foundation::Result<SerializedDocument>
DecodeDocument(std::span<const std::byte> bytes);
} // namespace epidemic::runtime
