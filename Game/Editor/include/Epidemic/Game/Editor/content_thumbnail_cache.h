#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::game::editor {
// CPU thumbnail cache. Key must contain the cooked digest, recipe/profile and
// thumbnail size. Never retains Renderer or ResourceLease handles.
struct ThumbnailPixels {
    std::uint32_t width=0,height=0;
    std::vector<std::byte> bgra;
};
class ThumbnailCache final {
  public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named byte-budget and entry-count bounds retain the existing cache API.
    explicit ThumbnailCache(std::size_t max_bytes=std::size_t{16}*1024*1024,std::size_t max_entries=128)
        :max_bytes_(max_bytes),max_entries_(max_entries){}
    [[nodiscard]] std::optional<ThumbnailPixels> Get(std::string_view key);
    // Returns false without modifying the cache if item is malformed or too big.
    [[nodiscard]] bool Put(std::string key,ThumbnailPixels image);
    void Clear() noexcept;
    [[nodiscard]] std::size_t ResidentBytes() const noexcept {return used_;}
    [[nodiscard]] std::size_t Count() const noexcept {return entries_.size();}
  private:
    struct Record {ThumbnailPixels image;std::list<std::string>::iterator lru{};};
    std::size_t max_bytes_,max_entries_,used_=0;
    std::list<std::string> lru_;
    std::unordered_map<std::string,Record> entries_;
    void EvictOne() noexcept;
};
} // namespace epidemic::game::editor
