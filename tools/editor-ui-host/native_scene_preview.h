#pragma once
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Runtime/Resources/resource_services.h"
#include "renderer.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <future>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace epidemic::editor_host {
namespace ed = game::editor;
namespace rt = runtime;

struct PreviewSprite {
    std::string key;
    // Native world XY/scale/angle, pixels-per-unit and pivot. Read projection only.
    std::array<float, 8> geometry{};
    bool operator==(const PreviewSprite &) const = default;
};

// Retained ResourceServices/ImageResourceLoader and Renderer, without a project
// owner. Only the immutable cooked reader runs on the worker. No source decode,
// filesystem enumeration or authoring I/O occurs on EDT.
class ScenePreview final {
    template <class T> static T Take(foundation::Result<T> result) {
        if (!result)
            throw std::runtime_error(result.GetError().code);
        return std::move(result).Value();
    }

    static void Take(const foundation::Result<void> &result) {
        if (!result)
            throw std::runtime_error(result.GetError().code);
    }

    struct Prepared {
        rt::ResourceLoadArtifact artifact;
        unsigned width = 0, height = 0;
    };

    struct Texture {
        rt::ResourceLease lease;
        rt::presentation::RendererTexturePtr image;
        unsigned width = 0, height = 0;
    };

    std::filesystem::path root_;
    ed::ImageResourceLoader loader_;
    rt::ResourceServices resources_;
    std::vector<PreviewSprite> sprites_;
    std::map<std::string, Texture> textures_;
    std::set<std::string> failed_;
    std::future<Prepared> worker_;
    std::string loading_;
    std::uint64_t epoch_ = 0, loading_epoch_ = 0;

  public:
    explicit ScenePreview(std::filesystem::path root)
        : root_(std::move(root)), loader_(root_),
          resources_(Take(rt::CreateResourceServices({std::size_t{16} * 1024u * 1024u}))) {
        Take(resources_.loaders->RegisterLoader(loader_));
        Take(resources_.loaders->Freeze());
    }

    ~ScenePreview() {
        for (const auto &[key, texture] : textures_) {
            (void)key;
            (void)resources_.manager->Release(texture.lease);
        }
    }

    void Update(std::vector<PreviewSprite> sprites) {
        if (sprites == sprites_)
            return;
        ++epoch_;
        sprites_ = std::move(sprites);
        failed_.clear();
        std::set<std::string> live;
        for (const auto &sprite : sprites_)
            live.insert(sprite.key);
        for (auto item = textures_.begin(); item != textures_.end();) {
            if (!live.contains(item->first)) {
                Take(resources_.manager->Release(item->second.lease));
                item = textures_.erase(item);
            } else
                ++item;
        }
        loader_.CollectUnused(*resources_.manager);
    }

    rt::ResourceMemoryStats Memory() const {
        return resources_.manager->GetMemoryStatistics();
    }

    std::size_t TextureCount() const {
        return textures_.size();
    }

    const std::filesystem::path &Root() const {
        return root_;
    }

    void Pump(rt::presentation::Renderer &renderer) {
        if (worker_.valid() &&
            worker_.wait_for(std::chrono::milliseconds{0}) == std::future_status::ready) {
            try {
                auto prepared = worker_.get();
                if (loading_epoch_ == epoch_) {
                    const auto id = Take(loader_.Register(loading_));
                    Take(loader_.StagePrepared(std::move(prepared.artifact)));
                    const auto lease =
                        Take(resources_.manager->RequestLease({id, loader_.GetResourceType()}));
                    try {
                        Take(resources_.manager->ProcessPendingLoads());
                        const auto payload = std::dynamic_pointer_cast<const ed::ImagePayload>(
                            resources_.manager->GetPayload(lease.resource));
                        if (!payload)
                            throw std::runtime_error("preview payload unavailable");
                        auto texture =
                            renderer.CreateTexture(payload->image.width, payload->image.height,
                                                   payload->image.pixels);
                        textures_.emplace(loading_, Texture{lease, std::move(texture),
                                                            prepared.width, prepared.height});
                    } catch (...) {
                        (void)resources_.manager->Release(lease);
                        throw;
                    }
                }
            } catch (const std::exception &error) {
                failed_.insert(loading_);
                loader_.CollectUnused(*resources_.manager);
                std::fprintf(stderr, "Scene preview: %s\n", error.what());
            }
            loading_.clear();
        }
        if (!worker_.valid() && textures_.size() < 128) {
            for (const auto &sprite : sprites_)
                if (!textures_.contains(sprite.key) && !failed_.contains(sprite.key)) {
                    loading_ = sprite.key;
                    loading_epoch_ = epoch_;
                    worker_ = std::async(
                        std::launch::async,
                        [](std::filesystem::path root, std::string key) {
                            ed::ImageResourceLoader reader(std::move(root));
                            const auto id = Take(reader.Register(std::move(key)));
                            auto artifact = Take(reader.Load({id, reader.GetResourceType()}));
                            const auto source =
                                std::dynamic_pointer_cast<const ed::ImagePayload>(artifact.payload);
                            if (!source)
                                throw std::runtime_error("preview image payload missing");
                            const auto w = source->image.width, h = source->image.height;
                            auto reduced = std::make_shared<ed::ImagePayload>();
                            reduced->image = ed::MakeImageThumbnail(source->image);
                            artifact.payload = std::move(reduced);
                            return Prepared{std::move(artifact), w, h};
                        },
                        root_, loading_);
                    break;
                }
        }
    }

    void Draw(rt::presentation::Renderer &renderer, D2D1_SIZE_F size) const {
        const auto width = size.width, height = size.height;
        double extent = .001;
        for (const auto &sprite : sprites_)
            if (const auto item = textures_.find(sprite.key); item != textures_.end()) {
                const auto &g = sprite.geometry;
                const double w = static_cast<double>(item->second.width) *
                                 std::abs(static_cast<double>(g[2])) / g[5];
                const double h = static_cast<double>(item->second.height) *
                                 std::abs(static_cast<double>(g[3])) / g[5];
                const auto radius = std::hypot(w, h);
                extent = std::max(extent, std::max(std::abs(static_cast<double>(g[0])) + radius,
                                                   std::abs(static_cast<double>(g[1])) + radius));
            }
        const double scale = std::min(width, height) * .4 / extent;
        for (const auto &sprite : sprites_)
            if (const auto item = textures_.find(sprite.key); item != textures_.end()) {
                const auto &g = sprite.geometry;
                const float x = static_cast<float>(width * .5 + g[0] * scale),
                            y = static_cast<float>(height * .5 - g[1] * scale);
                const float w =
                    static_cast<float>(static_cast<double>(item->second.width) *
                                       std::abs(static_cast<double>(g[2])) * scale / g[5]);
                const float h =
                    static_cast<float>(static_cast<double>(item->second.height) *
                                       std::abs(static_cast<double>(g[3])) * scale / g[5]);
                renderer.PushTransform(D2D1::Matrix3x2F::Scale(g[2] < 0 ? -1.0f : 1.0f,
                                                               g[3] < 0 ? -1.0f : 1.0f,
                                                               D2D1::Point2F(x, y)) *
                                       D2D1::Matrix3x2F::Rotation(-g[4], D2D1::Point2F(x, y)));
                renderer.Texture(item->second.image, x - g[6] * w, y - (1 - g[7]) * h, w, h);
                renderer.PopTransform();
            }
    }
};
} // namespace epidemic::editor_host
