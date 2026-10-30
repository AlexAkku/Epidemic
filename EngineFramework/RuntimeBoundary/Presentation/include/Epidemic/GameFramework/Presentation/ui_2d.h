#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace epidemic::gameplay::presentation {
struct UiRect {
    double x = 0, y = 0, width = 0, height = 0;
    bool Contains(double px, double py) const {
        return px >= x && py >= y && px < x + width && py < y + height;
    }
};
struct UiTarget {
    std::uint64_t id = 0; // Stable nonzero identity, independent of label/locale.
    UiRect bounds;
    bool enabled = true, focusable = true;
};

// Client maps physical input to logical coordinates before calling this model.
// Targets are supplied in painter order; pointer hit-testing prefers the last.
class UiFocusModel {
    std::vector<UiTarget> targets_;
    std::uint64_t focused_ = 0;
    bool CanFocus(std::uint64_t id) const {
        return std::any_of(targets_.begin(), targets_.end(), [id](const UiTarget &t) {
            return t.id == id && t.enabled && t.focusable;
        });
    }
  public:
    void Update(std::span<const UiTarget> targets) {
        for (std::size_t i = 0; i < targets.size(); ++i) {
            const auto &t = targets[i];
            const auto &b = t.bounds;
            if (!t.id || !std::isfinite(b.x) || !std::isfinite(b.y) ||
                !std::isfinite(b.width) || !std::isfinite(b.height) || b.width < 0 || b.height < 0)
                throw std::invalid_argument("Invalid UI target");
            for (std::size_t j = 0; j < i; ++j)
                if (targets[j].id == t.id)
                    throw std::invalid_argument("Duplicate UI target identity");
        }
        targets_.assign(targets.begin(), targets.end());
        if (!CanFocus(focused_)) focused_ = 0;
    }
    std::uint64_t Focused() const { return focused_; }
    bool Focus(std::uint64_t id) {
        if (!CanFocus(id)) return false;
        focused_ = id;
        return true;
    }
    std::uint64_t HitTest(double x, double y) const {
        for (auto i = targets_.rbegin(); i != targets_.rend(); ++i)
            if (i->enabled && i->bounds.Contains(x, y)) return i->id;
        return 0;
    }
    std::uint64_t FocusAt(double x, double y) {
        const auto id = HitTest(x, y);
        if (id) Focus(id);
        return id;
    }
    std::uint64_t Move(bool forward = true) {
        const auto count = targets_.size();
        if (!count) return focused_ = 0;
        auto current = std::find_if(targets_.begin(), targets_.end(), [&](const UiTarget &t) {
            return t.id == focused_;
        });
        std::size_t index = current == targets_.end() ? (forward ? count - 1 : 0) :
                            static_cast<std::size_t>(current - targets_.begin());
        for (std::size_t n = 0; n < count; ++n) {
            index = forward ? (index + 1) % count : (index + count - 1) % count;
            if (targets_[index].enabled && targets_[index].focusable)
                return focused_ = targets_[index].id;
        }
        return focused_ = 0;
    }
};

struct LocalizedUiText {
    std::string_view locale;
    std::wstring_view key, value;
};
// No I/O or global locale. Caller owns string storage; missing text returns key.
inline std::wstring_view LocalizeUi(std::span<const LocalizedUiText> entries, std::string_view locale,
                                   std::wstring_view key, std::string_view fallback = "en") {
    for (const auto language : {locale, fallback})
        for (const auto &entry : entries)
            if (entry.locale == language && entry.key == key) return entry.value;
    return key;
}
} // namespace epidemic::gameplay::presentation
