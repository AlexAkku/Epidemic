#include "Epidemic/Game/Editor/content_media.h"
#include <algorithm>
#include <array>
#include <limits>

namespace epidemic::game::editor::media {
namespace {
std::uint8_t B(std::span<const std::byte> s, std::size_t i) {
  return std::to_integer<std::uint8_t>(s[i]);
}
std::uint16_t U16(std::span<const std::byte> s, std::size_t i) {
  return static_cast<std::uint16_t>(std::uint32_t{B(s, i)} |
                                    (std::uint32_t{B(s, i + 1)} << 8U));
}
std::uint32_t U32(std::span<const std::byte> s, std::size_t i) {
  return static_cast<std::uint32_t>(std::uint32_t{B(s, i)} |
                                    (std::uint32_t{B(s, i + 1)} << 8U) |
                                    (std::uint32_t{B(s, i + 2)} << 16U) |
                                    (std::uint32_t{B(s, i + 3)} << 24U));
}
std::uint32_t BE32(std::span<const std::byte> s, std::size_t i) {
  return (static_cast<std::uint32_t>(B(s, i)) << 24U) |
         (std::uint32_t{B(s, i + 1)} << 16U) |
         (std::uint32_t{B(s, i + 2)} << 8U) | std::uint32_t{B(s, i + 3)};
}
bool Tag(std::span<const std::byte> s, std::size_t i, const char *label) {
  if (i + 4 > s.size())
    return false;
  for (std::size_t k = 0; k < 4; ++k)
    if (B(s, i + k) != static_cast<std::uint8_t>(label[k]))
      return false;
  return true;
}
} // namespace
Result<PcmWave> DecodeWavPcm(std::span<const std::byte> source) {
  if (source.size() < 44 || source.size() > std::size_t{128} * 1024 * 1024 ||
      !Tag(source, 0, "RIFF") || !Tag(source, 8, "WAVE") ||
      static_cast<std::uint64_t>(U32(source, 4)) + 8 > source.size())
    return {{}, "media.wav_header"};
  const std::size_t limit = static_cast<std::size_t>(U32(source, 4)) + 8;
  bool fmt = false, data = false;
  std::uint16_t codec = 0, channels = 0, bits = 0, alignment = 0;
  std::uint32_t sample_rate = 0, bytes_per_second = 0;
  std::span<const std::byte> pcm;
  for (std::size_t at = 12; at + 8 <= limit;) {
    const auto chunk = U32(source, at + 4);
    if (chunk > limit - at - 8)
      return {{}, "media.wav_chunk_bounds"};
    const auto payload = at + 8;
    if (Tag(source, at, "fmt ")) {
      if (fmt || chunk < 16)
        return {{}, "media.wav_fmt"};
      fmt = true;
      codec = U16(source, payload);
      channels = U16(source, payload + 2);
      sample_rate = U32(source, payload + 4);
      bytes_per_second = U32(source, payload + 8);
      alignment = U16(source, payload + 12);
      bits = U16(source, payload + 14);
    } else if (Tag(source, at, "data")) {
      if (data)
        return {{}, "media.wav_duplicate_data"};
      data = true;
      pcm = source.subspan(payload, chunk);
    }
    const auto advance = static_cast<std::size_t>(chunk) + (chunk & 1U);
    if (advance > limit - payload)
      return {{}, "media.wav_padding"};
    at = payload + advance;
  }
  if (!fmt || !data || codec != 1 || channels < 1 || channels > 2 ||
      sample_rate < 8000 || sample_rate > 192000 ||
      (bits != 8 && bits != 16 && bits != 24 && bits != 32) ||
      alignment != channels * (bits / 8) ||
      bytes_per_second != sample_rate * alignment ||
      pcm.size() % alignment != 0)
    return {{}, "media.wav_unsupported_pcm"};
  PcmWave result;
  result.channels = channels;
  result.bits_per_sample = bits;
  result.sample_rate = sample_rate;
  result.interleaved_pcm.assign(pcm.begin(), pcm.end());
  return {std::move(result), {}};
}
Result<FontFace> ValidateSfntFont(std::span<const std::byte> s) {
  if (s.size() < 12 || s.size() > std::size_t{64} * 1024 * 1024)
    return {{}, "media.font_size"};
  const auto version = BE32(s, 0);
  const auto tables = static_cast<std::uint16_t>(
      (std::uint32_t{B(s, 4)} << 8U) | std::uint32_t{B(s, 5)});
  if ((version != 0x00010000 && version != 0x4f54544f) || tables < 1 ||
      tables > 4096 ||
      static_cast<std::uint64_t>(12) + static_cast<std::uint64_t>(tables) * 16 >
          s.size())
    return {{}, "media.font_sfnt"};
  bool has_head = false, has_cmap = false;
  for (std::size_t i = 0; i < tables; ++i) {
    const auto pos = 12 + i * 16;
    const auto off = BE32(s, pos + 8), size = BE32(s, pos + 12);
    if (size == 0 || off > s.size() || size > s.size() - off)
      return {{}, "media.font_table_bounds"};
    has_head |= Tag(s, pos, "head");
    has_cmap |= Tag(s, pos, "cmap");
  }
  if (!has_head || !has_cmap)
    return {{}, "media.font_missing_tables"};
  return {{version, tables}, {}};
}
bool LooksLikeWebP(std::span<const std::byte> data) noexcept {
  return data.size() >= 16 && Tag(data, 0, "RIFF") && Tag(data, 8, "WEBP") &&
         static_cast<std::uint64_t>(U32(data, 4)) + 8 <= data.size() &&
         (Tag(data, 12, "VP8 ") || Tag(data, 12, "VP8L") ||
          Tag(data, 12, "VP8X"));
}
bool LooksLikeOgg(std::span<const std::byte> data) noexcept {
  return data.size() >= 27 && Tag(data, 0, "OggS") && B(data, 4) == 0 &&
         data.size() >= static_cast<std::size_t>(27) + B(data, 26);
}
} // namespace epidemic::game::editor::media
