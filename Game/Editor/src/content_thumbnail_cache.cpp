#include "Epidemic/Game/Editor/content_thumbnail_cache.h"
#include <limits>
#include <utility>

namespace epidemic::game::editor {
void ThumbnailCache::EvictOne() noexcept {
    if(lru_.empty())return;
    const auto found=entries_.find(lru_.back());
    if(found!=entries_.end()){
        used_-=found->second.image.bgra.size();entries_.erase(found);
    }
    lru_.pop_back();
}
void ThumbnailCache::Clear() noexcept {entries_.clear();lru_.clear();used_=0;}
std::optional<ThumbnailPixels> ThumbnailCache::Get(std::string_view key) {
    auto it=entries_.find(std::string(key));
    if(it==entries_.end())return std::nullopt;
    lru_.splice(lru_.begin(),lru_,it->second.lru);
    return it->second.image; // Detached value, never an internal view.
}
bool ThumbnailCache::Put(std::string key,ThumbnailPixels image) {
    const auto pixels=static_cast<std::uint64_t>(image.width)*image.height;
    if(key.empty()||key.size()>256||!image.width||!image.height ||
       image.width>1024 || image.height>1024 || pixels>std::numeric_limits<std::size_t>::max()/4 ||
       image.bgra.size()!=pixels*4 || max_entries_==0 || image.bgra.size()>max_bytes_)return false;
    // Strongly stage the key/entry before modifying the LRU budget.
    const auto bytes=image.bgra.size();
    if(auto old=entries_.find(key);old!=entries_.end()){
        used_-=old->second.image.bgra.size();lru_.erase(old->second.lru);entries_.erase(old);
    }
    lru_.push_front(std::move(key));
    try {entries_.emplace(lru_.front(),Record{std::move(image),lru_.begin()});}
    catch(...){lru_.pop_front();throw;}
    used_+=bytes;
    while((used_>max_bytes_ || entries_.size()>max_entries_)&&!entries_.empty())EvictOne();
    return true;
}
} // namespace epidemic::game::editor
