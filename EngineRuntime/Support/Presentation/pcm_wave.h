#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace epidemic::runtime::presentation::detail {
struct PcmWave {
  std::uint16_t channels{}, bits{}, blockAlign{};
  std::uint32_t sampleRate{}, bytesPerSecond{};
  std::span<const std::byte> samples;
};

// A new value is parsed on each call. Never reuse a previous track's fmt chunk.
inline std::optional<PcmWave> ParsePcmWave(std::span<const std::byte> bytes) {
  auto u16 = [&](std::size_t p) {
    return static_cast<std::uint16_t>(
        std::to_integer<unsigned>(bytes[p]) |
        (std::to_integer<unsigned>(bytes[p + 1]) << 8));
  };
  auto u32 = [&](std::size_t p) {
    return static_cast<std::uint32_t>(u16(p)) |
           (static_cast<std::uint32_t>(u16(p + 2)) << 16);
  };
  if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) ||
      std::memcmp(bytes.data() + 8, "WAVE", 4))
    return {};
  const auto riff = u32(4);
  if (riff < 4 || riff > bytes.size() - 8)
    return {};
  const std::size_t end = static_cast<std::size_t>(riff) + 8;
  PcmWave result;
  bool hasFormat = false, hasData = false;
  std::size_t p = 12;
  while (p + 8 <= end) {
    const auto size = u32(p + 4);
    if (size > end - p - 8)
      return {};
    if (!std::memcmp(bytes.data() + p, "fmt ", 4)) {
      if (hasFormat || size < 16 || u16(p + 8) != 1)
        return {};
      result.channels = u16(p + 10);
      result.sampleRate = u32(p + 12);
      result.bytesPerSecond = u32(p + 16);
      result.blockAlign = u16(p + 20);
      result.bits = u16(p + 22);
      hasFormat = true;
    } else if (!std::memcmp(bytes.data() + p, "data", 4)) {
      if (hasData)
        return {};
      result.samples = bytes.subspan(p + 8, size);
      hasData = true;
    }
    p += 8 + static_cast<std::size_t>(size);
    if (size & 1) {
      if (p == end)
        return {};
      ++p;
    }
  }
  if (p != end || !hasFormat || !hasData || result.samples.empty() ||
      !result.sampleRate || result.channels < 1 || result.channels > 2 ||
      (result.bits != 8 && result.bits != 16 && result.bits != 24 &&
       result.bits != 32))
    return {};
  const auto block = result.channels * (result.bits / 8);
  if (result.blockAlign != block ||
      static_cast<std::uint64_t>(result.sampleRate) * block !=
          result.bytesPerSecond ||
      result.samples.size() % block != 0)
    return {};
  return result;
}
} // namespace epidemic::runtime::presentation::detail
