#include "Epidemic/Game/Editor/content_media.h"
#include <array>
#include <cstdio>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace epidemic::game::editor::media;
template <class T>
void Check(const T &value,
           const std::source_location at = std::source_location::current()) {
  if (!static_cast<bool>(value))
    throw std::runtime_error(std::string(at.file_name()) + ":" +
                             std::to_string(at.line()));
}
int main() {
  try {
    std::array<unsigned char, 48> wav{
        'R',  'I',  'F', 'F', 40,   0,    0, 0, 'W', 'A', 'V', 'E',
        'f',  'm',  't', ' ', 16,   0,    0, 0, 1,   0,   1,   0,
        0x40, 0x1f, 0,   0,   0x80, 0x3e, 0, 0, 2,   0,   16,  0,
        'd',  'a',  't', 'a', 4,    0,    0, 0, 0,   0,   1,   0};
    auto w = DecodeWavPcm(
        {reinterpret_cast<const std::byte *>(wav.data()), wav.size()});
    Check(w && w.value.sample_rate == 8000 && w.value.bits_per_sample == 16);
    wav[20] = 3;
    Check(!DecodeWavPcm(
        {reinterpret_cast<const std::byte *>(wav.data()), wav.size()}));
    std::array<std::byte, 4> bad{};
    Check(!ValidateSfntFont(bad));
    Check(!LooksLikeOgg(bad));
    Check(!LooksLikeWebP(bad));
    std::cout << "bounded WAV PCM/media probing PASS\n";
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  } catch (...) {
    std::fputs("Unexpected test failure\n", stderr);
    return 1;
  }
}
