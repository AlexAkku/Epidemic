#include "Epidemic/Game/Editor/content_hot_reload.h"
#include <algorithm>
#include <chrono>
#include <system_error>

namespace epidemic::game::editor {
namespace {
struct Signature {
    bool present=false;
    std::uintmax_t size=0;
    std::filesystem::file_time_type write{};
};
Signature Observe(const ProjectFolder &folder,std::string_view relative) {
    const auto p=folder.ResolveMount("Content",relative);
    if(!p)return {};
    std::error_code ec;
    if(!std::filesystem::is_regular_file(p.Value(),ec)||ec)return {};
    const auto size=std::filesystem::file_size(p.Value(),ec);
    if(ec)return {};
    const auto write=std::filesystem::last_write_time(p.Value(),ec);
    if(ec)return {};
    return {true,size,write};
}
}
ContentHotReload::~ContentHotReload(){ CancelPending(); }
void ContentHotReload::CancelPending() noexcept {
    if(!pending_)return;
    pending_->cancel->store(true,std::memory_order_relaxed);
    // Cooperative cancellation: Windows WIC can still finish one bounded decode
    // before the task observes the flag. Never let a worker outlive root state.
    try { if(pending_->worker.valid())pending_->worker.wait(); }catch(...) { pending_.reset();return; }
    pending_.reset();
}
void ContentHotReload::SetEnabled(bool enabled) noexcept {
    enabled_=enabled;
    if(!enabled_)CancelPending();
}
void ContentHotReload::Reset(const ProjectFolder &folder) {
    CancelPending();
    root_=folder.Root();tracked_.clear();cursor_=0;
    for(const auto &source:folder.Sources()){
        Observation value;value.path=source.relative_path;
        const auto sig=Observe(folder,source.relative_path.substr(8));
        value.seen=sig.present;value.missing=!sig.present;
        value.size=sig.size;value.write=sig.write;
        tracked_.emplace(source.id.Raw(),std::move(value));
    }
}
std::vector<HotAssetEvent> ContentHotReload::Poll(ProjectFolder &folder) {
    std::vector<HotAssetEvent> events;
    if(!enabled_)return events;
    if(root_!=folder.Root()){Reset(folder);return events;}
    if(pending_ && pending_->worker.valid() &&
        pending_->worker.wait_for(std::chrono::milliseconds{0})==std::future_status::ready) {
        auto complete=std::move(pending_); // Release the job slot before callbacks.
        auto finished=[&] {
            try{return complete->worker.get();}
            catch(const std::exception &error){return foundation::Result<PreparedCookedPng>::Failure(foundation::Error::Create("editor.asset_worker_exception",error.what()));}
        }();
        auto &state=tracked_[complete->id.Raw()];
        const auto sig=Observe(folder,complete->relative);
        // An external writer replaced the file during cooking. Discard the
        // orphan cooked blob and require two new stable observations.
        if(!sig.present || sig.size!=complete->size || sig.write!=complete->write){
            state.stable=0;state.seen=false;
        }else if(complete->cancel->load(std::memory_order_relaxed)){
            events.push_back({complete->id,HotAssetEventKind::Cancelled,"editor.asset_cancelled"});
        }else if(!finished){
            state.failed=true;
            events.push_back({complete->id,HotAssetEventKind::Failed,finished.GetError().code+": "+complete->relative});
        }else {
            // Asset identity / path could have changed while the worker ran.
            const auto &assets=folder.Sources();
            const auto found=std::find_if(assets.begin(),assets.end(),[&](const auto &a){return a.id==complete->id;});
            if(found!=assets.end() && found->relative_path=="Content/"+complete->relative &&
                found->generation==complete->initial_generation && found->cooked_key==complete->initial_cooked_key){
                const auto committed=folder.PublishCooked(complete->id,finished.Value().key);
                if(committed){
                    state.failed=false;
                    events.push_back({complete->id,HotAssetEventKind::Updated,"editor.asset_updated: "+complete->relative});
                } else {
                    state.failed=true;
                    events.push_back({complete->id,HotAssetEventKind::Failed,committed.GetError().code+": "+complete->relative});
                }
            }
        }
    }
    constexpr std::size_t kStatsPerPoll=32;
    const auto &sources=folder.Sources();
    if(sources.empty())return events;
    const auto count=std::min(kStatsPerPoll,sources.size());
    for(std::size_t i=0;i<count;++i){
        const auto &source=sources[(cursor_+i)%sources.size()];
        if(source.type!=runtime::AssetType{foundation::StringId::FromString("epidemic.texture.png")})continue;
        auto &state=tracked_[source.id.Raw()];
        if(state.path!=source.relative_path){state={};state.path=source.relative_path;}
        const auto relative=source.relative_path.substr(8);
        const auto sig=Observe(folder,relative);
        if(!sig.present){
            if(!state.missing_reported)events.push_back({source.id,HotAssetEventKind::Missing,"editor.asset_missing: "+source.relative_path});
            state.missing_reported=true;state.missing=true;state.seen=false;state.stable=0;
            continue;
        }
        if(!state.seen){
            state.seen=true;state.size=sig.size;state.write=sig.write;
            state.stable=state.missing?1:0;state.missing=false;state.missing_reported=false;state.failed=false;
            continue;
        }
        if(state.size!=sig.size || state.write!=sig.write){
            state.size=sig.size;state.write=sig.write;state.stable=1;state.failed=false;
            continue;
        }
        // One in-flight job (backpressure); a burst is processed incrementally.
        if(state.stable==0 || state.failed || pending_)continue;
        if(++state.stable<2)continue;
        state.stable=0;
        auto next=std::make_unique<Pending>();
        next->id=source.id;next->relative=relative;next->size=sig.size;next->write=sig.write;
        next->initial_generation=source.generation;next->initial_cooked_key=source.cooked_key;
        next->cancel=std::make_shared<std::atomic_bool>(false);
        const auto root=folder.Root();
        const auto name=next->relative;
        const auto cancelled=next->cancel;
        try {
            next->worker=std::async(std::launch::async,&PrepareCookedPng,root,name,cancelled.get());
            pending_=std::move(next);
            events.push_back({source.id,HotAssetEventKind::Queued,"editor.asset_queued: "+relative});
        }catch(const std::exception&){
            state.failed=true;
            events.push_back({source.id,HotAssetEventKind::Failed,"editor.asset_worker_unavailable"});
        }
    }
    cursor_=(cursor_+count)%sources.size();
    return events;
}
} // namespace epidemic::game::editor
