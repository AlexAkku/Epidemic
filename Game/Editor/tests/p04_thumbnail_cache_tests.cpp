#include "Epidemic/Game/Editor/content_thumbnail_cache.h"
#include <cstdio>
#include <stdexcept>
#include <source_location>
#include <cstddef>
#include <string>
using namespace epidemic::game::editor;

static ThumbnailPixels P(std::uint32_t width = 2, std::uint32_t height = 2) {
    return {width, height,
            std::vector<std::byte>(static_cast<std::size_t>(width) * height * 4, std::byte{42})};

}

template <class T>
void Check(const T &value, const std::source_location at = std::source_location::current()) {
    if (!static_cast<bool>(value))
        throw std::runtime_error(std::string(at.file_name()) + ":" + std::to_string(at.line()));
}

int main() {
    try {
        ThumbnailCache cache{32, 2};

        Check(cache.Put("a", P()));
        Check(cache.Put("b", P()));
        Check(cache.Count() == 2 && cache.ResidentBytes() == 32);
        Check(cache.Get("a"));
        Check(cache.Put("c", P()));
        Check(cache.Get("a"));
        Check(!cache.Get("b"));
        Check(cache.Get("c"));
        Check(cache.Put("c", P(1, 1)));
        Check(cache.Count() == 2 && cache.ResidentBytes() == 20);
        Check(!cache.Put("bad", P(4, 4)));
        Check(!cache.Get("bad"));
        Check(!cache.Put("", P()));
        Check(!cache.Put("zero", {}));
        cache.Clear();
        Check(cache.Count() == 0 && cache.ResidentBytes() == 0);
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        std::fputs("Unexpected test failure\n", stderr);
        return 1;
    }

}
