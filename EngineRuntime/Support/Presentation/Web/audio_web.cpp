#include "sound.h"
#include <cmath>
#include <emscripten.h>
#include <map>

namespace epidemic::runtime::presentation
{
namespace
{
EM_JS(int,CreateMusic,(),{ return Module.epidemicAudio.createMusic();});
EM_JS(int,LoadMusic,(int id,int resource,int once),{ return Module.epidemicAudio.load(id, resource, !!once);});
EM_JS(double,MusicPosition,(int id),{const m=Module.epidemicAudio.music[id];return (m.audio.ended||m.finished)&&Number.isFinite(m.duration)?m.duration:m.audio.currentTime;});
EM_JS(int,MusicState,(int id),{ return Module.epidemicAudio.state(id);});
EM_JS(void,TickMusic,(int id,double dt,int active,double volume),{ Module.epidemicAudio.tick(id, dt, !!active, volume);});
EM_JS(void,ReadBands,(int id,float *bands),{ Module.epidemicAudio.bands(id, bands);});
EM_JS(void,DestroyMusic,(int id),{ Module.epidemicAudio.destroy(id);});
EM_JS(void,PlaySound,(int resource,double gain,double pan),{ Module.epidemicAudio.play(resource, gain, pan);});
} // namespace
struct Music::Impl
{
    int id = CreateMusic();
};
Music::Music() : impl_(std::make_unique<Impl>())
{
}
Music::~Music()
{
    DestroyMusic(impl_->id);
}
bool Music::Load(int resource, bool once)
{
    return LoadMusic(impl_->id, resource, once) != 0;
}
bool Music::LoadPcm(std::span<const std::byte> wave, bool once)
{
    const auto pointer = wave.data();
    const auto bytes = wave.size();
    const int id = impl_->id;
    return EM_ASM_INT({const url=URL.createObjectURL(new Blob([HEAPU8.slice($1,$1+$2)],{type:'audio/wav'}));return Module.epidemicAudio.loadUrl($0,url,!!$3,true);},id,pointer,bytes,once)!=0;
}
double Music::Position() const
{
    return MusicPosition(impl_->id);
}
void Music::Tick(double dt, bool menu, double volume)
{
    TickMusic(impl_->id, dt, menu, volume);
}
std::array<float, 8> Music::Bands() const
{
    std::array<float, 8> bands{};
    ReadBands(impl_->id, bands.data());
    return bands;
}
bool Music::Playing() const
{
    return State() == PlaybackState::Playing;
}
PlaybackState Music::State() const
{
    return static_cast<PlaybackState>(MusicState(impl_->id));
}
AudioFailure Music::Failure() const
{
    return State() == PlaybackState::Failed ? AudioFailure{"Browser media playback", -1} : AudioFailure{};
}
struct Sound::Impl
{
    std::map<std::string, AudioEntry, std::less<>> entries;
    std::map<std::string, double, std::less<>> played;
    double time = 0, volume = .65;
    explicit Impl(std::span<const AudioEntry> list)
    {
        for (auto e : list)
            entries.emplace(e.id, e);
    }
};
Sound::Sound(std::span<const AudioEntry> entries) : impl_(std::make_unique<Impl>(entries))
{
}
Sound::~Sound() = default;
bool Sound::Enabled() const
{
    return EM_ASM_INT({ return !!Module.epidemicAudio && Module.epidemicAudio.context.state != 'closed';})!= 0;
}
void Sound::Tick(double dt, double volume)
{
    impl_->time += std::max(0., dt);
    impl_->volume = std::clamp(volume, 0., 1.);
    EM_ASM({ Module.epidemicAudio.sfxGain.gain.value = $0;},impl_->volume);
}
std::size_t Sound::Voices() const
{
    return static_cast<std::size_t>(EM_ASM_INT({ return Module.epidemicAudio.voices.size;}));
}
void Sound::Play(const std::string &id, int rank, double gain, double x, double hero)
{
    auto it = impl_->entries.find(id);
    if (it == impl_->entries.end() || Voices() >= 25)
        return;
    auto old = impl_->played.find(id);
    if (old != impl_->played.end() && impl_->time - old->second < it->second.gap / 1000)
        return;
    impl_->played[id] = impl_->time;
    const bool spatial = x < 1e90;
    const auto distance = spatial ? std::abs(x - hero) : 0.;
    PlaySound(
        it->second.resource,
        std::clamp(gain * it->second.gain * (1 + .08 * (std::clamp(rank, 1, 4) - 1)) / (1 + distance / 1000), 0., 2.),
        spatial ? std::clamp((x - hero) / 800., -1., 1.) : 0);
}
} // namespace epidemic::runtime::presentation
