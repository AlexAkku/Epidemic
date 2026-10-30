#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace epidemic::game::editor::media {
// Pure bounded probes: parsing does not publish an asset or touch the project.
// Playback and codec adapters are separate from this validation boundary.
struct PcmWave {
    std::uint16_t channels=0, bits_per_sample=0;
    std::uint32_t sample_rate=0;
    std::vector<std::byte> interleaved_pcm;
};
struct FontFace {std::uint32_t scaler=0; std::uint16_t tables=0;};
template<class T> struct Result {
    T value{};std::string error;
    [[nodiscard]] explicit operator bool() const noexcept {return error.empty();}
};
[[nodiscard]] Result<PcmWave> DecodeWavPcm(std::span<const std::byte> source);
[[nodiscard]] Result<FontFace> ValidateSfntFont(std::span<const std::byte> source);
// This only checks RIFF WebP / Ogg container framing, not a decoder.
[[nodiscard]] bool LooksLikeWebP(std::span<const std::byte> source) noexcept;
[[nodiscard]] bool LooksLikeOgg(std::span<const std::byte> source) noexcept;
} // namespace epidemic::game::editor::media
