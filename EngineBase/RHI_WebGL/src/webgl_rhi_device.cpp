#include <Epidemic/RHI_WebGL/webgl_rhi_device.h>
#include <GLES3/gl3.h>
#include <emscripten/html5.h>
#include <memory>
#include <utility>

namespace epidemic::rhi::webgl {
namespace {
foundation::Result<void> Fail(const char *code, const char *message) {
    return foundation::Result<void>::Failure(foundation::Error::Create(code, message));
}

struct Surface {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = 0;
    std::string selector;
    std::uint32_t width = 0, height = 0;

    ~Surface() {
        if (context)
            emscripten_webgl_destroy_context(context);
    }

    foundation::Result<void> Bind() const {
        if (!context || emscripten_is_webgl_context_lost(context))
            return Fail("rhi.webgl.context_lost", "WebGL2 context is unavailable or lost");
        if (emscripten_webgl_make_context_current(context) != EMSCRIPTEN_RESULT_SUCCESS)
            return Fail("rhi.webgl.bind_failed", "Cannot bind WebGL2 context");
        return foundation::Result<void>::Success();
    }

};

struct DeviceState {
    std::weak_ptr<Surface> surface;
};

class SwapChain final : public IRhiSwapChain {
    RhiSwapChainDesc desc_;
    std::shared_ptr<Surface> surface_;

  public:
    SwapChain(RhiSwapChainDesc desc, std::shared_ptr<Surface> surface)
        : desc_(std::move(desc)), surface_(std::move(surface)) {
    }

    foundation::Result<void> Present() override {
        return surface_->Bind();
    } // Browser presents at the end of rAF.

    foundation::Result<void> Resize(std::uint32_t w, std::uint32_t h) override {
        auto candidate = desc_;
        candidate.width = w;
        candidate.height = h;
        if (auto checked = Validate(candidate); !checked.HasValue())
            return checked;
        if (w > 4096 || h > 4096)
            return Fail("rhi.webgl.surface_limit",
                        "Bootstrap canvas limit is 4096 pixels per axis");
        if (auto bound = surface_->Bind(); !bound.HasValue())
            return bound;
        if (emscripten_set_canvas_element_size(surface_->selector.c_str(), static_cast<int>(w),
                                               static_cast<int>(h)) != EMSCRIPTEN_RESULT_SUCCESS)
            return Fail("rhi.webgl.resize_failed", "Cannot resize browser canvas");
        surface_->width = w;
        surface_->height = h;
        desc_ = std::move(candidate);
        return foundation::Result<void>::Success();
    }

    std::uint32_t Width() const noexcept override {
        return desc_.width;
    }

    std::uint32_t Height() const noexcept override {
        return desc_.height;
    }

    std::uint32_t BufferCount() const noexcept override {
        return desc_.buffer_count;
    }

    RhiPixelFormat ColorFormat() const noexcept override {
        return desc_.color_format;
    }

};

class Context final : public IRhiCommandContext {
    std::shared_ptr<DeviceState> state_;
    // Retain the frame's surface even if its swap-chain owner is released mid-frame.
    std::shared_ptr<Surface> frame_;

  public:
    explicit Context(std::shared_ptr<DeviceState> state) : state_(std::move(state)) {
    }

    foundation::Result<void> BeginFrame() override {
        if (frame_)
            return Fail("rhi.frame_already_active", "BeginFrame requires an inactive context");
        auto surface = state_->surface.lock();
        if (!surface)
            return Fail("rhi.webgl.surface_required", "Create a live swap chain first");
        if (auto bound = surface->Bind(); !bound.HasValue())
            return bound;
        glViewport(0, 0, static_cast<GLsizei>(surface->width),
                   static_cast<GLsizei>(surface->height));
        frame_ = std::move(surface);
        return foundation::Result<void>::Success();
    }

    foundation::Result<void> Clear(const RhiClearDesc &desc) override {
        if (!frame_)
            return Fail("rhi.frame_not_active", "Clear requires BeginFrame");
        if (auto checked = Validate(desc); !checked.HasValue())
            return checked;
        if (auto bound = frame_->Bind(); !bound.HasValue())
            return bound;
        if (desc.clear_color) {
            glClearColor(desc.color.red, desc.color.green, desc.color.blue, desc.color.alpha);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        return foundation::Result<void>::Success();
    }

    foundation::Result<void> EndFrame() override {
        if (!frame_)
            return Fail("rhi.frame_not_active", "EndFrame requires BeginFrame");
        auto result = frame_->Bind();
        frame_.reset();
        return result;
    }

    bool IsFrameActive() const noexcept override {
        return static_cast<bool>(frame_);
    }

};

class Device final : public IRhiDevice {
    RhiDeviceDesc desc_;
    std::shared_ptr<DeviceState> state_ = std::make_shared<DeviceState>();

  public:
    explicit Device(RhiDeviceDesc desc) : desc_(std::move(desc)) {
    }

    std::string_view BackendName() const noexcept override {
        return "WebGL2";
    }

    const RhiDeviceDesc &Descriptor() const noexcept override {
        return desc_;
    }

    foundation::Result<std::shared_ptr<IRhiCommandContext>> CreateCommandContext() override {
        return foundation::Result<std::shared_ptr<IRhiCommandContext>>::Success(
            std::make_shared<Context>(state_));
    }

    foundation::Result<std::shared_ptr<IRhiSwapChain>>
    CreateSwapChain(const RhiSwapChainDesc &desc) override {
        using Result = foundation::Result<std::shared_ptr<IRhiSwapChain>>;
        if (auto checked = Validate(desc); !checked.HasValue())
            return Result::Failure(checked.GetError());
        if (desc.color_format != RhiPixelFormat::R8G8B8A8_UNorm)
            return Result::Failure(
                foundation::Error::Create("rhi.webgl.unsupported_format",
                                          "Browser canvas requires R8G8B8A8_UNorm"));
        if (!state_->surface.expired())
            return Result::Failure(
                foundation::Error::Create("rhi.webgl.single_surface",
                                          "Bootstrap WebGL2 device supports one live swap chain"));
        if (desc.width > 4096 || desc.height > 4096)
            return Result::Failure(
                foundation::Error::Create("rhi.webgl.surface_limit",
                                          "Bootstrap canvas limit is 4096 pixels per axis"));
        auto surface = std::make_shared<Surface>();
        surface->selector = desc.surface_handle.As<const char *>();
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.alpha = false;
        attrs.antialias = false;
        attrs.depth = false;
        attrs.stencil = false;
        surface->context = emscripten_webgl_create_context(surface->selector.c_str(), &attrs);
        if (surface->context <= 0) {
            surface->context = 0;
            return Result::Failure(
                foundation::Error::Create("rhi.webgl.create_failed",
                                          "Browser does not provide a WebGL2 context"));
        }

        auto swap = std::make_shared<SwapChain>(desc, surface);
        if (auto resized = swap->Resize(desc.width, desc.height); !resized.HasValue())
            return Result::Failure(resized.GetError());
        state_->surface = surface;
        return Result::Success(std::move(swap));
    }

};

} // namespace

foundation::Result<std::shared_ptr<IRhiDevice>> CreateWebGL2RhiDevice(const RhiDeviceDesc &desc) {
    using Result = foundation::Result<std::shared_ptr<IRhiDevice>>;
    if (auto checked = Validate(desc); !checked.HasValue())
        return Result::Failure(checked.GetError());
    return Result::Success(std::make_shared<Device>(desc));
}

} // namespace epidemic::rhi::webgl
