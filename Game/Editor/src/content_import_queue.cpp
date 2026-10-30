#include "Epidemic/Game/Editor/content_import_queue.h"
#include <algorithm>
#include "Epidemic/Game/Editor/project_path.h"
#include <limits>
#include <utility>

namespace epidemic::game::editor {
ContentImportQueue::~ContentImportQueue(){Cancel();} // Future drains before cancel-state destruction.
void ContentImportQueue::Cancel() noexcept {
    ++epoch_;queued_.clear();
    if(!pending_)return;
    pending_->cancel->store(true,std::memory_order_relaxed);
    try{if(pending_->worker.wait_for(std::chrono::milliseconds{0})==std::future_status::ready)pending_.reset();}
    catch(...){return;}
}
void ContentImportQueue::Reset(const std::filesystem::path &root){Cancel();root_=root;}
foundation::Result<std::uint64_t> ContentImportQueue::Enqueue(const ProjectFolder &folder,
    const std::filesystem::path &source,std::string destination,runtime::AssetId duplicate_of){
    using R=foundation::Result<std::uint64_t>;
    if(root_!=folder.Root())Reset(folder.Root());
    const auto resolved=folder.ResolveMount("Content",destination);
    std::error_code error;
    if(!resolved || destination.empty() || IsPrivateContentPath(destination) ||
        std::filesystem::exists(resolved.Value(),error) || error)
        return R::Failure(foundation::Error::Create("editor.import_destination","Destination must be a new Content path"));
    if(queued_.size()>=128 || next_job_==std::numeric_limits<std::uint64_t>::max())
        return R::Failure(foundation::Error::Create("editor.import_queue_full","Wait for pending source copies"));
    if(std::any_of(queued_.begin(),queued_.end(),[&](const auto &item){return item.destination==destination;}) ||
        (pending_ && pending_->request.epoch==epoch_ && pending_->request.destination==destination))
        return R::Failure(foundation::Error::Create("editor.import_destination_pending","Destination already queued"));
    const auto job=next_job_;
    queued_.push_back({job,epoch_,folder.Root(),source,std::move(destination),duplicate_of,folder.AssetRevision()});++next_job_;
    return R::Success(job);
}
std::vector<HotAssetEvent> ContentImportQueue::Poll(ProjectFolder &folder){
    if(root_!=folder.Root())Reset(folder.Root());
    std::vector<HotAssetEvent> events;
    if(pending_ && pending_->worker.wait_for(std::chrono::milliseconds{0})==std::future_status::ready){
        auto completed=std::exchange(pending_,nullptr);
        auto candidate=[&]{
            try{return completed->worker.get();}
            catch(const std::exception &error){return foundation::Result<PreparedCookedPng>::Failure(foundation::Error::Create("editor.import_worker",error.what()));}
            catch(...){return foundation::Result<PreparedCookedPng>::Failure(foundation::Error::Create("editor.import_worker","Unknown worker exception"));}
        }();
        if(completed->request.epoch!=epoch_ || completed->cancel->load(std::memory_order_relaxed))
            events.push_back({{},HotAssetEventKind::Cancelled,"editor.import_cancelled"});
        else if(!candidate)events.push_back({{},HotAssetEventKind::Failed,candidate.GetError().code+": "+completed->request.destination});
        else{
            const auto imported=folder.CommitImportedPng(std::move(candidate).Value(),completed->request.destination,completed->request.duplicate_of,completed->request.asset_revision);
            if(imported)events.push_back({imported.Value(),HotAssetEventKind::Updated,"editor.asset_imported: "+completed->request.destination});
            else events.push_back({{},HotAssetEventKind::Failed,imported.GetError().code+": "+completed->request.destination});
        }
    }
    if(!pending_ && !queued_.empty()){
        auto next=std::make_unique<Pending>();next->request=std::move(queued_.front());queued_.pop_front();
        next->cancel=std::make_shared<std::atomic_bool>(false);
        try{
            next->worker=std::async(std::launch::async,prepare_,next->request.root,next->request.source,next->cancel.get());
            pending_=std::move(next);events.push_back({{},HotAssetEventKind::Queued,"editor.asset_importing: "+pending_->request.destination});
        }catch(const std::exception &error){events.push_back({{},HotAssetEventKind::Failed,std::string("editor.import_worker: ")+error.what()});}
    }
    return events;
}
} // namespace epidemic::game::editor
