#include "pcm_wave.h"
#include "sound.h"
#include "blob_store.h"
#include <objbase.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace epidemic::runtime::presentation;

void Check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

std::vector<std::byte> Wave() {
    std::vector<std::byte> data(8044);
    auto put16 = [&](std::size_t p, unsigned value) {
        data[p] = static_cast<std::byte>(value & 255);
        data[p + 1] = static_cast<std::byte>((value >> 8) & 255);
    };

    auto put32 = [&](std::size_t p, unsigned value) {
        put16(p, value);
        put16(p + 2, value >> 16);
    };

    std::memcpy(data.data(), "RIFF", 4);
    put32(4, 8036);
    std::memcpy(data.data() + 8, "WAVEfmt ", 8);
    put32(16, 16);
    put16(20, 1);
    put16(22, 1);
    put32(24, 8000);
    put32(28, 16000);
    put16(32, 2);
    put16(34, 16);
    std::memcpy(data.data() + 36, "data", 4);
    put32(40, 8000);
    return data;
}

int main() {
    try {
        auto valid = Wave();
        Check(detail::ParsePcmWave(valid).has_value(), "valid PCM");
        auto missing = valid;
        std::memcpy(missing.data() + 12, "JUNK", 4);
        Check(!detail::ParsePcmWave(missing), "fmt chunk is mandatory even after a valid parse");
        auto malformed = valid;
        malformed[32] = std::byte{4};

        Check(!detail::ParsePcmWave(malformed), "block alignment must match channel/sample size");
        malformed = valid;
        malformed[28] = std::byte{0};

        Check(!detail::ParsePcmWave(malformed), "byte rate must match sample rate and alignment");
        Check(!detail::ParsePcmWave(std::span<const std::byte>(valid).first(valid.size() - 1)),
              "truncated RIFF");
        const auto com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        struct Com {
            bool owned;

            ~Com() {
                if (owned)
                    CoUninitialize();
            }

        } apartment{SUCCEEDED(com)};

        Music music;
        Check(music.State() == PlaybackState::Unloaded, "new music is unloaded");
        Check(!music.LoadPcm(missing) && music.State() == PlaybackState::Failed && !music.Playing(),
              "malformed load fails cleanly");
        Check(music.Failure().operation == "ParsePcmWave" && music.Position() == 0,
              "parse error is observable");
        if (music.LoadPcm(valid, true)) {
            music.Tick(0, true, 0);
            Check(music.Playing(), "one-shot starts immediately");
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
            music.Tick(.06, true, 0);
            Check(music.Position() > 0, "audio sample clock advances");
            music.Tick(0, false, 0);
            const auto cursor = music.Position();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            music.Tick(.03, false, 0);
            Check(music.State() == PlaybackState::Paused && music.Position() == cursor,
                  "cinematic pause retains cursor");
            music.Tick(0, true, 0);
            Check(music.Playing(), "cinematic resumes");
            std::this_thread::sleep_for(std::chrono::milliseconds(550));
            music.Tick(.55, true, 0);
            Check(music.State() == PlaybackState::Ended,
                  "one-shot reaches an observable end state");
            Check(music.LoadPcm(valid), "menu loads after cinematic");
            music.Tick(5, true, 0);
            Check(music.Playing(), "menu starts after initial delay");
            music.Tick(0, false, 0);
            Check(!music.Playing() && music.Position() == 0, "menu exit flushes playback");
            music.Tick(0, true, 0);
            Check(music.Playing() && music.Position() == 0, "menu restarts immediately at zero");
            Check(!music.LoadPcm(missing) && !music.Playing() && music.Position() == 0,
                  "invalid reload cannot reuse old format or voice");
        } else {
            std::cout << "SKIP: playback device unavailable; parser/failure contracts checked\n";
        }

        const auto profile = std::filesystem::path("presentation-roundtrip-" +
                                                   std::to_string(GetCurrentProcessId()) + ".bin");
        const std::vector<std::byte> first{std::byte{1}, std::byte{2}}, second{std::byte{3}};

        SaveBlob(profile, first);
        Check(LoadBlob(profile) == first, "relative filename without parent directory round trips");
        SaveBlob(profile, second);
        Check(LoadBlob(profile) == second, "existing durable save can be atomically replaced");
        std::filesystem::remove(profile);
        std::cout << "PASS: PCM validation, playback transitions, and relative durable saves\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

}
