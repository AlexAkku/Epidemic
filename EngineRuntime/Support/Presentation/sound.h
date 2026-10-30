#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
namespace epidemic::runtime::presentation {
enum class PlaybackState { Unloaded, Waiting, Playing, Paused, Ended, Failed };
struct AudioFailure {
  std::string operation;
  std::int32_t nativeCode{};
};
struct AudioEntry {
  std::string_view id;
  int resource;
  double gain, gap;
};
inline double MusicFade(double seconds, double duration) {
  return std::min(std::clamp(seconds / 10., 0., 1.),
                  std::clamp((duration - seconds) / 10., 0., 1.));
}
class Music {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  Music();
  ~Music();
  bool Load(int resource, bool once = false);
  bool LoadPcm(std::span<const std::byte> wave, bool once = false);
  double Position() const;
  void Tick(double dt, bool menu, double volume);
  std::array<float, 8> Bands() const;
  bool Playing() const;
  PlaybackState State() const;
  AudioFailure Failure() const;
};
class Sound {
  struct Impl;
  std::unique_ptr<Impl> impl_;

public:
  explicit Sound(std::span<const AudioEntry> entries);
  ~Sound();
  bool Enabled() const;
  void Play(const std::string &id, int rank, double gain, double x,
            double heroX);
  void Tick(double dt, double volume);
  std::size_t Voices() const;
};
} // namespace epidemic::runtime::presentation
