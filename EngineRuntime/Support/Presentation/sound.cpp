#include "sound.h"
#include "pcm_wave.h"
#include <Epidemic/Runtime/Audio/audio.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>
#include <windows.h>
#include <wrl/client.h>
#include <xapofx.h>
#include <xaudio2.h>
namespace epidemic::runtime::presentation {
namespace au = epidemic::runtime::audio;
using epidemic::foundation::Error;
using epidemic::foundation::Result;
using Duration = epidemic::runtime::RuntimeFrameDuration;
namespace {
Result<void> Status(HRESULT status) {
  return SUCCEEDED(status) ? Result<void>::Success()
                           : Result<void>::Failure(Error::Create(
                                 "ascent.audio", "XAudio2 operation failed"));
}
struct Clip final : au::IAudioClipResource {
  std::vector<std::byte> data;
  WAVEFORMATEX format{};
  std::size_t offset = 0, bytes = 0;
  float pitch = 1, filter = 4100;
  au::AudioClipFormat GetFormat() const override {
    return {format.nChannels, format.nSamplesPerSec, format.wBitsPerSample,
            false};
  }
  au::AudioClipStorage GetStorage() const override {
    return au::AudioClipStorage::InMemoryEncoded;
  }
  std::span<const std::byte> GetEncodedData() const override { return data; }
  std::shared_ptr<au::IAudioStreamSource> GetStreamSource() const override {
    return {};
  }
};
class Resources final : public au::IAudioResourceSource {
public:
  std::map<std::uint64_t, std::shared_ptr<Clip>> clips;
  explicit Resources(std::span<const AudioEntry> entries) {
    for (const auto &e : entries) {
      const auto resource =
          FindResourceW(nullptr, MAKEINTRESOURCEW(e.resource), RT_RCDATA);
      if (!resource)
        throw std::runtime_error("Embedded audio is missing");
      const auto handle = LoadResource(nullptr, resource);
      const auto *data = static_cast<const std::byte *>(LockResource(handle));
      const auto size = SizeofResource(nullptr, resource);
      const auto parsed =
          data ? detail::ParsePcmWave({data, size}) : std::nullopt;
      if (!parsed || parsed->samples.size() > XAUDIO2_MAX_BUFFER_BYTES)
        throw std::runtime_error("Invalid embedded WAV");
      Clip base;
      base.data.assign(data, data + size);
      base.format = {WAVE_FORMAT_PCM,
                     parsed->channels,
                     parsed->sampleRate,
                     parsed->bytesPerSecond,
                     parsed->blockAlign,
                     parsed->bits,
                     0};
      base.offset = static_cast<std::size_t>(parsed->samples.data() - data);
      base.bytes = parsed->samples.size();
      for (int rank = 0; rank < 4; ++rank) {
        auto clip = std::make_shared<Clip>(base);
        clip->pitch = 1 + .035f * static_cast<float>(rank);
        clip->filter = 4100 + 800.f * static_cast<float>(rank);
        clips.emplace(static_cast<std::uint64_t>(e.resource * 4 + rank),
                      std::move(clip));
      }
    }
  }
  au::SoundState GetSoundState(au::SoundId id) const override {
    return clips.contains(id.value) ? au::SoundState::Ready
                                    : au::SoundState::Missing;
  }
  Result<au::AudioClipPayload> LoadClip(au::SoundId id) const override {
    const auto it = clips.find(id.value);
    if (it == clips.end())
      return Result<au::AudioClipPayload>::Failure(
          Error::Create("ascent.audio.missing", "Sound not found"));
    return Result<au::AudioClipPayload>::Success(
        {id, au::SoundState::Ready, it->second});
  }
};
class Backend final : public au::IAudioBackend {
  struct Voice {
    IXAudio2SourceVoice *voice;
    std::shared_ptr<const Clip> clip;
  };
  Microsoft::WRL::ComPtr<IXAudio2> engine_;
  IXAudio2MasteringVoice *master_ = nullptr;
  Microsoft::WRL::ComPtr<IUnknown> limiter_;
  double masterVolume_ = .65;
  std::uint64_t next_ = 1;
  std::map<std::uint64_t, Voice> voices_;

public:
  ~Backend() override {
    for (auto &[id, v] : voices_) {
      (void)id;
      v.voice->DestroyVoice();
    }
    if (master_)
      master_->DestroyVoice();
  }
  bool IsEnabled() const override { return master_ != nullptr; }
  Result<void> Initialize(const au::AudioBackendOptions &) override {
    HRESULT hr = XAudio2Create(&engine_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
      return Status(hr);
    hr = engine_->CreateMasteringVoice(&master_, 2);
    if (FAILED(hr))
      return Status(hr);
    // Use the Windows mastering limiter inside the existing Runtime backend.
    // The SDK mixer manages gain/routing; it has no DSP compression contract.
    const auto module = GetModuleHandleW(L"XAudio2_9.dll");
    const auto address = module ? GetProcAddress(module, "CreateFX") : nullptr;
    using CreateEffect =
        HRESULT(__cdecl *)(REFCLSID, IUnknown **, const void *, UINT32);
    CreateEffect create = nullptr;
    static_assert(sizeof(create) == sizeof(address));
    std::memcpy(&create, &address, sizeof(create));
    if (!create)
      return Status(E_NOINTERFACE);
    hr = create(CLSID_FXMasteringLimiter, &limiter_, nullptr, 0);
    if (FAILED(hr))
      return Status(hr);
    XAUDIO2_EFFECT_DESCRIPTOR descriptor{limiter_.Get(), TRUE, 2};
    XAUDIO2_EFFECT_CHAIN chain{1, &descriptor};
    hr = master_->SetEffectChain(&chain);
    if (FAILED(hr))
      return Status(hr);
    const FXMASTERINGLIMITER_PARAMETERS parameters{
        FXMASTERINGLIMITER_DEFAULT_RELEASE,
        FXMASTERINGLIMITER_DEFAULT_LOUDNESS};
    return Status(
        master_->SetEffectParameters(0, &parameters, sizeof(parameters)));
  }
  Result<au::BackendVoiceHandle>
  CreateVoice(const au::AudioVoiceDesc &desc) override {
    const auto clip = std::dynamic_pointer_cast<const Clip>(desc.clip.resource);
    if (!clip)
      return Result<au::BackendVoiceHandle>::Failure(Error::Create(
          "ascent.audio.format", "Audio clip ownership mismatch"));
    IXAudio2SourceVoice *voice = nullptr;
    HRESULT hr = engine_->CreateSourceVoice(&voice, &clip->format,
                                            XAUDIO2_VOICE_USEFILTER, 2.f);
    if (FAILED(hr))
      return Result<au::BackendVoiceHandle>::Failure(
          Error::Create("ascent.audio.voice", "Cannot create voice"));
    XAUDIO2_BUFFER buffer{};
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = static_cast<UINT32>(clip->bytes);
    buffer.pAudioData =
        reinterpret_cast<const BYTE *>(clip->data.data() + clip->offset);
    buffer.LoopCount = desc.loop ? XAUDIO2_LOOP_INFINITE : 0;
    hr = voice->SubmitSourceBuffer(&buffer);
    if (SUCCEEDED(hr))
      hr = voice->SetFrequencyRatio(clip->pitch);
    if (SUCCEEDED(hr))
      hr = voice->SetVolume(desc.initial_gain);
    XAUDIO2_FILTER_PARAMETERS filter{
        LowPassFilter,
        std::clamp(
            static_cast<float>(2 * std::sin(3.141592653589793 * clip->filter /
                                            clip->format.nSamplesPerSec)),
            0.f, 1.f),
        1.5f};
    if (SUCCEEDED(hr))
      hr = voice->SetFilterParameters(&filter);
    if (FAILED(hr)) {
      voice->DestroyVoice();
      return Result<au::BackendVoiceHandle>::Failure(
          Error::Create("ascent.audio.buffer", "Cannot submit voice buffer"));
    }
    const auto id = next_++;
    try {
      voices_.emplace(id, Voice{voice, clip});
    } catch (...) {
      voice->DestroyVoice();
      throw;
    }
    // Epidemic supplies one-shot spatial state in CreateVoice; it does not
    // subsequently call SetSpatialState for these immutable events.
    const auto spatial = SetSpatialState({id}, desc.spatial);
    if (!spatial.HasValue()) {
      static_cast<void>(DestroyVoice({id}));
      return Result<au::BackendVoiceHandle>::Failure(spatial.GetError());
    }
    return Result<au::BackendVoiceHandle>::Success({id});
  }
  Result<void> DestroyVoice(au::BackendVoiceHandle h) override {
    const auto it = voices_.find(h.value);
    if (it != voices_.end()) {
      it->second.voice->DestroyVoice();
      voices_.erase(it);
    }
    return Result<void>::Success();
  }
  Result<void> Play(au::BackendVoiceHandle h) override {
    return Status(voices_.at(h.value).voice->Start());
  }
  Result<void> Pause(au::BackendVoiceHandle h) override {
    return Status(voices_.at(h.value).voice->Stop());
  }
  Result<void> Stop(au::BackendVoiceHandle h) override {
    const auto hr = voices_.at(h.value).voice->Stop();
    if (FAILED(hr))
      return Status(hr);
    return Status(voices_.at(h.value).voice->FlushSourceBuffers());
  }
  bool IsVoiceFinished(au::BackendVoiceHandle h) const override {
    const auto it = voices_.find(h.value);
    if (it == voices_.end())
      return true;
    XAUDIO2_VOICE_STATE state{};
    it->second.voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return state.BuffersQueued == 0;
  }
  Result<void> SetGain(au::BackendVoiceHandle h, float gain) override {
    return Status(voices_.at(h.value).voice->SetVolume(gain));
  }
  Result<void> SetSpatialState(au::BackendVoiceHandle h,
                               const au::AudioSpatialState &s) override {
    auto &v = voices_.at(h.value);
    if (!s.spatial)
      return Result<void>::Success();
    const float pan = std::clamp(s.transform.position.x, -.65f, .65f);
    const UINT32 channels = v.clip->format.nChannels;
    std::vector<float> matrix(channels * 2);
    if (channels == 1) {
      const double angle = (pan + 1) * 3.141592653589793 / 4;
      matrix[0] = static_cast<float>(std::cos(angle));
      matrix[1] = static_cast<float>(std::sin(angle));
    } else if (channels == 2) {
      const double angle = (pan <= 0 ? pan + 1 : pan) * 3.141592653589793 / 2;
      matrix[0] = pan <= 0 ? 1.f : static_cast<float>(std::cos(angle));
      matrix[1] = pan <= 0 ? 0.f : static_cast<float>(std::sin(angle));
      matrix[2] = pan <= 0 ? static_cast<float>(std::cos(angle)) : 0.f;
      matrix[3] = pan <= 0 ? static_cast<float>(std::sin(angle)) : 1.f;
    } else
      for (UINT32 i = 0; i < channels; ++i) {
        matrix[i * 2] = (1 - pan) / channels;
        matrix[i * 2 + 1] = (1 + pan) / channels;
      }
    return Status(
        v.voice->SetOutputMatrix(master_, channels, 2, matrix.data()));
  }
  Result<void>
  CreateBackendListener(au::AudioListenerHandle,
                        const epidemic::runtime::Transform &) override {
    return Result<void>::Success();
  }
  Result<void> DestroyBackendListener(au::AudioListenerHandle) override {
    return Result<void>::Success();
  }
  Result<void>
  SetBackendListenerTransform(au::AudioListenerHandle,
                              const epidemic::runtime::Transform &) override {
    return Result<void>::Success();
  }
  Result<void> Update(Duration) override { return Result<void>::Success(); }
  void Volume(double value, double dt) {
    masterVolume_ +=
        (value - masterVolume_) * (1 - std::exp(-std::max(0., dt) / .015));
    if (master_)
      static_cast<void>(
          master_->SetVolume(static_cast<float>(masterVolume_ * .62)));
  }
  std::size_t Voices() const { return voices_.size(); }
};
} // namespace
struct Sound::Impl {
  std::shared_ptr<Backend> backend = std::make_shared<Backend>();
  std::shared_ptr<Resources> resources;
  std::vector<AudioEntry> entries;
  au::AudioServices services;
  std::map<std::string, double> played;
  double time = 0;
  bool ready = false;
  explicit Impl(std::span<const AudioEntry> catalog)
      : resources(std::make_shared<Resources>(catalog)),
        entries(catalog.begin(), catalog.end()) {
    auto result = au::CreateAudioServices(
        {.enable_mock_backend = false,
         .max_queued_events = 25,
         .event_overflow_policy = au::AudioEventOverflowPolicy::DropNewest},
        {backend, resources, {}});
    if (result.HasValue()) {
      services = std::move(result).Value();
      ready = true;
      for (const auto &[id, clip] : resources->clips) {
        (void)clip;
        const auto registered =
            services.sounds->RegisterSound({{id}, au::SoundState::Ready});
        if (!registered.HasValue())
          throw std::runtime_error(registered.GetError().message);
      }
    }
  }
  ~Impl() {
    if (ready)
      static_cast<void>(services.runtime->Shutdown());
  }
};
Sound::Sound(std::span<const AudioEntry> entries)
    : impl_(std::make_unique<Impl>(entries)) {}
Sound::~Sound() = default;
bool Sound::Enabled() const { return impl_->ready; }
void Sound::Play(const std::string &id, int rank, double gain, double x,
                 double heroX) {
  if (!impl_->ready ||
      impl_->backend->Voices() + impl_->services.events->Events().size() >= 25)
    return;
  const auto entry =
      std::find_if(impl_->entries.begin(), impl_->entries.end(),
                   [&](const AudioEntry &e) { return e.id == id; });
  if (entry == impl_->entries.end())
    return;
  double gap = entry->gap / 1000;
  const auto last = impl_->played.find(id);
  if (last != impl_->played.end() && impl_->time - last->second < gap)
    return;
  impl_->played[id] = impl_->time;
  const int tier = std::clamp(rank, 1, 4) - 1;
  const bool spatial = x < 1e90;
  const double distance = spatial ? std::abs(x - heroX) : 0;
  au::AudioEvent event;
  event.sound = {static_cast<std::uint64_t>(entry->resource * 4 + tier)};
  event.space = spatial ? au::AudioEventSpace::WorldPosition
                        : au::AudioEventSpace::NonSpatial;
  event.position.x =
      static_cast<float>(std::clamp((x - heroX) / 650, -.65, .65));
  event.volume = static_cast<float>(entry->gain * std::clamp(gain, 0., 1.) *
                                    (1 + .035 * tier) / (1 + distance / 900));
  static_cast<void>(impl_->services.events->SubmitOneShot(event));
}
void Sound::Tick(double dt, double volume) {
  impl_->time += dt;
  impl_->backend->Volume(volume, dt);
  if (impl_->ready) {
    const auto result = impl_->services.runtime->Tick(
        {std::chrono::microseconds(static_cast<std::int64_t>(dt * 1000000))});
    if (!result.HasValue())
      throw std::runtime_error(result.GetError().message);
  }
}
std::size_t Sound::Voices() const { return impl_->backend->Voices(); }
} // namespace epidemic::runtime::presentation
