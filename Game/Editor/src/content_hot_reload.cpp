#include "Epidemic/Game/Editor/content_hot_reload.h"
#include <algorithm>
#include <chrono>
#include <system_error>
#include <array>

namespace epidemic::game::editor {
namespace {
struct Signature {
    bool present = false;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type write{};

};

Signature Observe(const ProjectFolder &folder, std::string_view relative) {
    const auto path = folder.ResolveMount("Content", relative);
    if (!path)
        return {};

    std::error_code error;
    if (!std::filesystem::is_regular_file(path.Value(), error) || error)
        return {};

    const auto size = std::filesystem::file_size(path.Value(), error);
    if (error)
        return {};

    const auto write = std::filesystem::last_write_time(path.Value(), error);
    return error ? Signature{} : Signature{true, size, write};

}

} // namespace

ContentHotReload::ContentHotReload(Prepare prepare, std::chrono::milliseconds audit_interval)
    : prepare_(std::move(prepare)),
      audit_interval_(std::max(audit_interval, std::chrono::milliseconds{1})) {
}

ContentHotReload::~ContentHotReload() {
    CancelPending();
    // Final owner teardown only: never detach a worker or let it outlive its
    // cancellation state. Reset/disable retain the slot and never wait here.
    // Pending's async future joins during member destruction.
}

void ContentHotReload::CancelPending() noexcept {
    if (!pending_)
        return;
    pending_->cancel->store(true, std::memory_order_relaxed);
    try {
        if (pending_->worker.wait_for(std::chrono::milliseconds{0}) == std::future_status::ready)
            pending_.reset();
    } catch (...) {
        return;
    } // Retain ownership if the readiness query fails.

}

void ContentHotReload::SetEnabled(bool enabled) noexcept {
    if (enabled_ == enabled) {
        if (!enabled && pending_ && (!pending_->manual || pending_->cancel->load()))
            CancelPending();
        return;
    }

    enabled_ = enabled;
    if (!enabled_) {
        if (pending_ && !pending_->manual)
            CancelPending();
    } else
        for (auto &[id, state] : tracked_) {
            (void)id;
            state.dirty = true;
        }

}

void ContentHotReload::Cancel() noexcept {
    ++epoch_;
    CancelPending();
    manual_.clear();
    for (auto &[id, state] : tracked_) {
        (void)id;
        state.force = false;
        state.dirty = false;
        state.next_audit = std::chrono::steady_clock::now() + audit_interval_;
    }

}

foundation::Result<void> ContentHotReload::Request(const ProjectFolder &folder,
                                                   runtime::AssetId asset) {
    using R = foundation::Result<void>;
    const auto found = std::find_if(folder.Sources().begin(), folder.Sources().end(),
                                    [&](const auto &source) { return source.id == asset; });
    if (found == folder.Sources().end() || found->IsTrashed() ||
        found->type != runtime::AssetType{foundation::StringId::FromString("epidemic.texture.png")})
        return R::Failure(
            foundation::Error::Create("editor.image_type", "Reimport requires an indexed PNG"));
    if (root_ != folder.Root())
        Reset(folder);
    auto &state = tracked_[asset.Raw()];
    if (!state.force) {
        if (manual_.size() >= 128)
            return R::Failure(
                foundation::Error::Create("editor.asset_queue_full", "Wait for pending imports"));
        manual_.push_back(asset);
        state.force = true;
    }

    state.dirty = true;
    return R::Success();
}

void ContentHotReload::Reset(const ProjectFolder &folder) {
    ++epoch_;
    CancelPending();
    root_ = folder.Root();
    tracked_.clear();
    manual_.clear();
    cursor_ = 0;
    // First observations are audit candidates, not trusted content baselines.
}

std::vector<HotAssetEvent> ContentHotReload::Poll(ProjectFolder &folder) {
    std::vector<HotAssetEvent> events;
    if (root_ != folder.Root())
        Reset(folder);
    const auto now = std::chrono::steady_clock::now();
    if (pending_ &&
        pending_->worker.wait_for(std::chrono::milliseconds{0}) == std::future_status::ready) {
        auto complete = std::move(pending_);
        auto finished = [&] {
            try {
                return complete->worker.get();
            } catch (const std::exception &error) {
                return foundation::Result<PreparedCookedPng>::Failure(
                    foundation::Error::Create("editor.asset_worker_exception", error.what()));
            } catch (...) {
                return foundation::Result<PreparedCookedPng>::Failure(
                    foundation::Error::Create("editor.asset_worker_exception",
                                              "Unknown worker exception"));
            }

        }();
        if (complete->epoch != epoch_ || complete->cancel->load(std::memory_order_relaxed) ||
            (!enabled_ && !complete->manual)) {
            events.push_back(
                {complete->id, HotAssetEventKind::Cancelled, "editor.asset_cancelled"});
        } else {
            auto &state = tracked_[complete->id.Raw()];
            const auto &sources = folder.Sources();
            const auto found = std::find_if(sources.begin(), sources.end(), [&](const auto &asset) {
                return asset.id == complete->id;
            });
            const auto signature = Observe(folder, complete->relative);
            if (found == sources.end() || found->IsTrashed() ||
                found->relative_path != "Content/" + complete->relative ||
                found->generation != complete->initial_generation ||
                found->cooked_key != complete->initial_cooked_key ||
                found->source_hash != complete->initial_source_hash || !signature.present ||
                signature.size != complete->size || signature.write != complete->write) {
                state.dirty = true;
                state.seen = false;
                state.stable = 0;
            } else if (!finished) {
                state.failed = true;
                state.next_audit = now + audit_interval_;
                events.push_back({complete->id, HotAssetEventKind::Failed,
                                  finished.GetError().code + ": " + complete->relative});
            } else {
                const auto before = found->generation;
                const auto was_failed = state.failed;
                const auto published = folder.PublishCooked(complete->id, finished.Value());
                state.next_audit = now + audit_interval_;
                state.failed = !published;
                if (!published)
                    events.push_back({complete->id, HotAssetEventKind::Failed,
                                      published.GetError().code + ": " + complete->relative});
                else if (found->generation != before)
                    events.push_back({complete->id, HotAssetEventKind::Updated,
                                      "editor.asset_updated: " + complete->relative});
                else if (was_failed || finished.Value().Rebuilt())
                    events.push_back({complete->id, HotAssetEventKind::Recovered,
                                      "editor.asset_verified: " + complete->relative});
            }

        }

        // 'finished' and its source read lease are released before another job.
    }

    if (!enabled_ && manual_.empty())
        return events;
    const auto &sources = folder.Sources();
    if (sources.empty())
        return events;
    constexpr std::size_t kStatsPerPoll = 32;
    std::array<std::size_t, kStatsPerPoll + 1> candidates{};

    std::size_t count = 0;
    if (!manual_.empty()) {
        const auto source = std::find_if(sources.begin(), sources.end(), [&](const auto &asset) {
            return asset.id == manual_.front();
        });
        if (source == sources.end()) {
            events.push_back({manual_.front(), HotAssetEventKind::Failed, "editor.asset_unknown"});
            manual_.pop_front();
        } else
            candidates[count++] = static_cast<std::size_t>(source - sources.begin());
    }

    const auto stats = enabled_ ? std::min(kStatsPerPoll, sources.size()) : 0;
    for (std::size_t i = 0; i < stats; ++i)
        candidates[count++] = (cursor_ + i) % sources.size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto index = candidates[i];
        const auto &source = sources[index];
        if (source.IsTrashed() ||
            source.type !=
                runtime::AssetType{foundation::StringId::FromString("epidemic.texture.png")})
            continue;
        auto &state = tracked_[source.id.Raw()];
        if (state.path != source.relative_path) {
            const auto force = state.force;
            state = {};

            state.path = source.relative_path;
            state.force = force;
        }

        const auto relative = source.relative_path.substr(8);
        const auto signature = Observe(folder, relative);
        if (!signature.present) {
            if (!state.missing_reported)
                events.push_back({source.id, HotAssetEventKind::Missing,
                                  "editor.asset_missing: " + source.relative_path});
            state.missing_reported = true;
            state.missing = true;
            state.seen = false;
            state.stable = 0;
            state.dirty = true;
            state.failed = true;
            if (state.force) {
                state.force = false;
                if (!manual_.empty() && manual_.front() == source.id)
                    manual_.pop_front();
            }

            continue;
        }

        if (!state.seen || state.size != signature.size || state.write != signature.write) {
            state.seen = true;
            state.size = signature.size;
            state.write = signature.write;
            state.stable = 1;
            state.dirty = true;
            state.missing = false;
            state.missing_reported = false;
            continue;
        }

        state.stable = std::min(2U, state.stable + 1);
        if (state.stable < 2 || pending_ || (!state.dirty && now < state.next_audit))
            continue;
        auto next = std::make_unique<Pending>();
        next->id = source.id;
        next->relative = relative;
        next->size = signature.size;
        next->write = signature.write;
        next->epoch = epoch_;
        next->initial_generation = source.generation;
        next->manual = state.force;
        next->initial_cooked_key = source.cooked_key;
        next->initial_source_hash = source.source_hash;
        next->cancel = std::make_shared<std::atomic_bool>(false);
        state.dirty = false;
        state.next_audit = now + audit_interval_;
        try {
            next->worker =
                std::async(std::launch::async, prepare_, root_, next->relative, next->cancel.get(),
                           state.force ? std::string{} : next->initial_cooked_key);
            state.force = false;
            if (next->manual && !manual_.empty() && manual_.front() == source.id)
                manual_.pop_front();
            pending_ = std::move(next);
            cursor_ = (index + 1) % sources.size();
            events.push_back(
                {source.id, HotAssetEventKind::Queued, "editor.asset_queued: " + relative});
            return events; // Fair round-robin even when all assets are due for audit.
        } catch (const std::exception &) {
            state.failed = true;
            events.push_back(
                {source.id, HotAssetEventKind::Failed, "editor.asset_worker_unavailable"});
        }

    }

    cursor_ = (cursor_ + stats) % sources.size();
    return events;
}

} // namespace epidemic::game::editor
