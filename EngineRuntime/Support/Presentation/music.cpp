#include "sound.h"
#include "pcm_wave.h"
#include <windows.h>
#include <xaudio2.h>
#include <wrl/client.h>
#include <algorithm>
#include <vector>
#include <complex>
#include <cmath>
#include <cstring>

namespace epidemic::runtime::presentation {
struct Music::Impl {
    Microsoft::WRL::ComPtr<IXAudio2> engine;
    IXAudio2MasteringVoice *master = nullptr;
    IXAudio2SourceVoice *voice = nullptr;
    std::vector<short> pcm;
    WAVEFORMATEX format{};

    std::array<float, 8> bands{}, average{};

    std::array<double, 8> cooldown{};

    double wait = 5, analyse = 0, position = 0;
    bool once = false, finished = false, menuActive = true;
    UINT64 sampleOrigin = 0;
    bool started = false, playing = false, failed = false;
    AudioFailure failure;

    bool Check(HRESULT status, const char *operation) {
        if (SUCCEEDED(status))
            return true;
        failure = {operation, static_cast<std::int32_t>(status)};

        failed = true;
        started = playing = false;
        bands = {};

        Clear();
        return false;
    }

    ~Impl() {
        Clear();
    }

    void Clear() {
        if (voice) {
            voice->DestroyVoice();
            voice = nullptr;
        }

        if (master) {
            master->DestroyVoice();
            master = nullptr;
        }

        engine.Reset();
    }

    void Spectrum(UINT64 cursor, double dt, double gain) {
        for (int b = 0; b < 8; ++b) {
            bands[b] *= static_cast<float>(std::exp(-dt * 7));
            cooldown[b] -= dt;
        }

        analyse += dt;
        if (analyse < .04)
            return;
        analyse = 0;
        constexpr int n = 2048;
        constexpr double pi = 3.141592653589793;
        std::array<std::complex<double>, n> fft{};

        const auto frames = pcm.size() / format.nChannels;
        for (int i = 0; i < n; ++i) {
            const auto at = static_cast<long long>(cursor) - n + i;
            double sample = 0;
            if (at >= 0 && static_cast<std::size_t>(at) < frames)
                for (int c = 0; c < format.nChannels; ++c)
                    sample += pcm[static_cast<std::size_t>(at) * format.nChannels + c] / 32768.;
            fft[i] = sample / format.nChannels * (.5 - .5 * std::cos(2 * pi * i / (n - 1)));
        }

        for (int i = 1, j = 0; i < n; ++i) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(fft[i], fft[j]);
        }

        for (int len = 2; len <= n; len <<= 1) {
            const auto step = std::polar(1., -2 * pi / len);
            for (int i = 0; i < n; i += len) {
                std::complex<double> w = 1;
                for (int j = 0; j < len / 2; ++j) {
                    auto a = fft[i + j], b = fft[i + j + len / 2] * w;
                    fft[i + j] = a + b;
                    fft[i + j + len / 2] = a - b;
                    w *= step;
                }

            }

        }

        constexpr double edges[] = {35, 90, 180, 360, 720, 1440, 2880, 5760, 11025};

        for (int b = 0; b < 8; ++b) {
            double energy = 0;
            int bins = 0;
            for (int k = 1; k < n / 2; ++k) {
                const double hz = k * static_cast<double>(format.nSamplesPerSec) / n;
                if (hz >= edges[b] && hz < edges[b + 1]) {
                    energy += std::norm(fft[k]);
                    ++bins;
                }

            }

            const float value = static_cast<float>(std::sqrt(energy / std::max(1, bins)) / 32);
            const float prior = average[b];
            average[b] += (value - average[b]) * .06f;
            if (gain > .0001 && value > .004f && value > prior * 1.12f && cooldown[b] <= 0) {
                bands[b] = static_cast<float>(std::clamp((value - prior) * 6. + .22, 0., 1.) *
                                              std::sqrt(gain));
                cooldown[b] = .14;
            }

        }

    }

};

Music::Music() : impl_(std::make_unique<Impl>()) {
}

Music::~Music() = default;

bool Music::Load(int resource, bool once) {
    const auto res = FindResourceW(nullptr, MAKEINTRESOURCEW(resource), RT_RCDATA);
    if (!res) {
        LoadPcm({}, once);
        impl_->failure = {"FindResourceW", static_cast<std::int32_t>(
                                               HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND))};

        return false;
    }

    const auto size = SizeofResource(nullptr, res);
    const auto data = static_cast<const unsigned char *>(LockResource(LoadResource(nullptr, res)));
    return LoadPcm(data
                       ? std::span<const std::byte>(reinterpret_cast<const std::byte *>(data), size)
                       : std::span<const std::byte>{},
                   once);
}

bool Music::LoadPcm(std::span<const std::byte> wave, bool once) {
    auto &m = *impl_;
    m.Clear();
    m.pcm.clear();
    m.format = {};

    m.failure = {};

    m.failed = true;
    m.started = m.playing = m.finished = false;
    m.position = 0;
    m.bands = {};

    m.average = {};

    m.cooldown = {};

    const auto parsed = detail::ParsePcmWave(wave);
    if (!parsed || parsed->bits != 16 || parsed->samples.size() > XAUDIO2_MAX_BUFFER_BYTES) {
        m.failure = {"ParsePcmWave", static_cast<std::int32_t>(E_INVALIDARG)};

        return false;
    }

    m.format = {WAVE_FORMAT_PCM,
                parsed->channels,
                parsed->sampleRate,
                parsed->bytesPerSecond,
                parsed->blockAlign,
                parsed->bits,
                0};

    m.pcm.resize(parsed->samples.size() / 2);
    std::memcpy(m.pcm.data(), parsed->samples.data(), parsed->samples.size());
    if (!m.Check(XAudio2Create(&m.engine), "XAudio2Create") ||
        !m.Check(m.engine->CreateMasteringVoice(&m.master, 2), "CreateMasteringVoice") ||
        !m.Check(m.engine->CreateSourceVoice(&m.voice, &m.format), "CreateSourceVoice"))
        return false;
    m.wait = once ? 0 : 5;
    m.once = once;
    m.menuActive = true;
    m.finished = false;
    m.position = 0;
    m.started = m.playing = false;
    m.failed = false;
    return true;
}

void Music::Tick(double dt, bool menu, double volume) {
    auto &m = *impl_;
    if (m.failed || !m.voice || m.finished)
        return;
    dt = std::max(0., dt);
    volume = std::clamp(volume, 0., 1.);
    // Menu sessions restart from sample zero; cinematic playback retains its pause cursor.
    if (!menu && !m.once) {
        if (m.menuActive) {
            if (!m.Check(m.voice->Stop(), "Stop") ||
                !m.Check(m.voice->FlushSourceBuffers(), "FlushSourceBuffers"))
                return;
            m.started = m.playing = false;
            m.wait = 0;
            m.position = 0;
            m.bands = {};

            m.average = {};

            m.cooldown = {};

        }

        m.menuActive = false;
        return;
    }

    if (menu)
        m.menuActive = true;
    // Initial countdown includes startup. A fresh menu session fades in from sample zero.
    if (!m.started) {
        m.wait = std::max(0., m.wait - dt);
        if (m.wait > 0 || !menu) {
            m.bands = {};

            return;
        }

        XAUDIO2_VOICE_STATE initial{};

        m.voice->GetState(&initial);
        m.sampleOrigin = initial.SamplesPlayed;
        XAUDIO2_BUFFER buffer{};

        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.AudioBytes = static_cast<UINT32>(m.pcm.size() * 2);
        buffer.pAudioData = reinterpret_cast<const BYTE *>(m.pcm.data());
        if (!m.Check(m.voice->SubmitSourceBuffer(&buffer), "SubmitSourceBuffer"))
            return;
        m.started = true;
        m.average = {};

        m.cooldown = {};

    }

    if (!menu) {
        if (m.playing && !m.Check(m.voice->Stop(), "Stop"))
            return;
        m.playing = false;
        m.bands = {};

        return;
    }

    XAUDIO2_VOICE_STATE state{};

    m.voice->GetState(&state);
    if (!state.BuffersQueued) {
        m.started = m.playing = false;
        m.wait = 60;
        m.bands = {};

        m.finished = m.once;
        m.position =
            static_cast<double>(m.pcm.size()) / m.format.nChannels / m.format.nSamplesPerSec;
        return;
    }

    const double cursor =
        static_cast<double>(state.SamplesPlayed - m.sampleOrigin) / m.format.nSamplesPerSec;
    const double duration =
        static_cast<double>(m.pcm.size()) / m.format.nChannels / m.format.nSamplesPerSec;
    m.position = cursor;
    const double gain = volume * (m.once ? std::min(std::clamp(cursor / .02, 0., 1.),
                                                    std::clamp((duration - cursor) / 2., 0., 1.))
                                         : MusicFade(cursor, duration));
    if (!m.Check(m.voice->SetVolume(static_cast<float>(gain * .62)), "SetVolume"))
        return;
    if (!m.playing) {
        if (!m.Check(m.voice->Start(), "Start"))
            return;
        m.playing = true;
    }

    m.Spectrum(state.SamplesPlayed - m.sampleOrigin, dt, gain);
}

std::array<float, 8> Music::Bands() const {
    return impl_->bands;
}

bool Music::Playing() const {
    return impl_->playing;
}

double Music::Position() const {
    return impl_->position;
}

PlaybackState Music::State() const {
    const auto &m = *impl_;
    if (m.failed)
        return PlaybackState::Failed;
    if (!m.voice)
        return PlaybackState::Unloaded;
    if (m.finished)
        return PlaybackState::Ended;
    if (m.playing)
        return PlaybackState::Playing;
    return m.started ? PlaybackState::Paused : PlaybackState::Waiting;
}

AudioFailure Music::Failure() const {
    return impl_->failure;
}

} // namespace epidemic::runtime::presentation
