#include "renderer.h"
#include <d2d1effects.h>
#include <d2d1effects_2.h>
#include <Epidemic/Runtime/Renderer/renderer_services.h>
#include <d2d1_1helper.h>
#include <wincodec.h>
#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace epidemic::runtime::presentation {
namespace {
void Check(HRESULT hr, const char *action) {
    if (FAILED(hr)) {
        char message[180]{};
        sprintf_s(message, "%s (HRESULT 0x%08lX)", action, static_cast<unsigned long>(hr));
        throw std::runtime_error(message);
    }
}
D2D1_COLOR_F Color(unsigned hex, float alpha = 1) {
    return D2D1::ColorF(static_cast<float>((hex >> 16) & 255) / 255, static_cast<float>((hex >> 8) & 255) / 255,
                        static_cast<float>(hex & 255) / 255, alpha);
}
}
const wchar_t *ModeName(DisplayMode mode) {
    if (mode == DisplayMode::Borderless)
        return L"Без рамок";
    if (mode == DisplayMode::Fullscreen)
        return L"Полноэкранный";
    return L"Оконный";
}

namespace rr = epidemic::runtime::renderer;
namespace {
constexpr auto CanvasMeshId = epidemic::runtime::ResourceId::FromString("presentation.canvas.commands");
constexpr auto CanvasMaterialId = epidemic::runtime::ResourceId::FromString("presentation.canvas.material");
template <class T> T Require(epidemic::foundation::Result<T> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
    return std::move(result).Value();
}
void Require(epidemic::foundation::Result<void> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
}
}
struct Renderer::FrameDriver {
    struct Mesh final : rr::IRenderMeshResource {
        std::vector<std::function<void()>> &commands;
        explicit Mesh(Renderer &renderer) : commands(renderer.commands_) {
        }
    };
    struct Material final : rr::IRenderMaterialResource {
        Microsoft::WRL::ComPtr<ID2D1DeviceContext> canvas;
        explicit Material(Renderer &renderer) : canvas(renderer.canvas_) {
        }
    };
    struct Bridge final : rr::IRenderResourceBridge,
                          rr::IRenderSceneSource,
                          rr::IRenderPoseSource,
                          rr::IRenderCommandSink {
        Renderer &renderer;
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        unsigned holds = 0;
        bool open = false;
        explicit Bridge(Renderer &owner)
            : renderer(owner), mesh(std::make_shared<Mesh>(owner)), material(std::make_shared<Material>(owner)) {
        }
        epidemic::foundation::Result<void> AcquirePayloads(epidemic::runtime::ResourceId m,
                                                           epidemic::runtime::ResourceId a) override {
            if (m != CanvasMeshId || a != CanvasMaterialId)
                return Failure("Unknown canvas resource");
            ++holds;
            return Success();
        }
        epidemic::foundation::Result<void> ReleasePayloads(epidemic::runtime::ResourceId m,
                                                           epidemic::runtime::ResourceId a) override {
            if (m != CanvasMeshId || a != CanvasMaterialId || !holds)
                return Failure("Canvas resource hold mismatch");
            --holds;
            return Success();
        }
        epidemic::foundation::Result<rr::RenderResourcePayloads>
        GetPayloads(epidemic::runtime::ResourceId m, epidemic::runtime::ResourceId a) const override {
            if (m != CanvasMeshId || a != CanvasMaterialId || !holds)
                return epidemic::foundation::Result<rr::RenderResourcePayloads>::Failure(
                    epidemic::foundation::Error::Create("canvas.resource", "Canvas resource is not held"));
            return epidemic::foundation::Result<rr::RenderResourcePayloads>::Success({mesh, material});
        }
        epidemic::foundation::Result<rr::RenderTransformSnapshot>
        GetTransformSnapshot(rr::RenderTransformId node) const override {
            if (node.value != 1)
                return epidemic::foundation::Result<rr::RenderTransformSnapshot>::Failure(
                    epidemic::foundation::Error::Create("canvas.transform", "Unknown canvas transform"));
            return epidemic::foundation::Result<rr::RenderTransformSnapshot>::Success({.node = node, .revision = 1});
        }
        epidemic::foundation::Result<std::shared_ptr<const rr::RenderPoseBuffer>>
        GetPose(epidemic::runtime::RuntimeObjectId) const override {
            return epidemic::foundation::Result<std::shared_ptr<const rr::RenderPoseBuffer>>::Success(nullptr);
        }
        epidemic::foundation::Result<void> BeginFrame(const rr::RenderFrameContext &) override {
            if (open)
                return Failure("Canvas frame already open");
            try {
                renderer.DrawBegin();
                open = true;
                return Success();
            } catch (const std::exception &e) {
                return Failure(e.what());
            }
        }
        epidemic::foundation::Result<void> SubmitProxy(const rr::RenderProxySubmission &submission) override {
            if (!open || submission.payloads.mesh != mesh || submission.payloads.material != material)
                return Failure("Canvas submission ownership mismatch");
            try {
                for (const auto &command : mesh->commands)
                    command();
                return Success();
            } catch (const std::exception &e) {
                return Failure(e.what());
            }
        }
        epidemic::foundation::Result<void> EndFrame() override {
            if (!open)
                return Failure("Canvas frame is not open");
            // EndDraw closes D2D even when it reports failure; Present owns no open D2D frame.
            open = false;
            try {
                renderer.DrawEnd();
                return Success();
            } catch (const std::exception &e) {
                return Failure(e.what());
            }
        }
        epidemic::foundation::Result<void> AbortFrame() override {
            if (open) {
                open = false;
                static_cast<void>(renderer.canvas_->EndDraw());
            }
            renderer.transforms_.clear();
            renderer.opacity_ = 1;
            return Success();
        }
        static epidemic::foundation::Result<void> Success() {
            return epidemic::foundation::Result<void>::Success();
        }
        static epidemic::foundation::Result<void> Failure(std::string message) {
            return epidemic::foundation::Result<void>::Failure(
                epidemic::foundation::Error::Create("canvas.backend", std::move(message)));
        }
    };
    std::shared_ptr<Bridge> bridge;
    rr::RendererServices services;
    rr::RenderProxyId proxy;
    rr::ViewId view;
    explicit FrameDriver(Renderer &renderer) : bridge(std::make_shared<Bridge>(renderer)) {
        services = Require(rr::CreateRendererServices({}, {bridge, bridge, bridge, bridge}));
        proxy = Require(services.scene->RegisterProxy({.owner = epidemic::runtime::RuntimeObjectId{1},
                                                       .mesh = CanvasMeshId,
                                                       .material = CanvasMaterialId,
                                                       .transform_node = rr::RenderTransformId{1},
                                                       .layer = rr::RenderLayer::Transparent}));
        view = Require(services.views->CreateView({.transform_node = rr::RenderTransformId{1}}));
        Require(services.views->SetMainView(view));
    }
    ~FrameDriver() {
        static_cast<void>(services.runtime->Shutdown());
        static_cast<void>(services.scene->DestroyProxy(proxy));
        static_cast<void>(services.scene->FlushDeferredDestroys());
        static_cast<void>(services.views->DestroyView(view));
    }
    void Render() {
        Require(services.runtime->PrepareFrame());
        Require(services.runtime->RenderFrame());
    }
};
void Renderer::Begin() {
    if (recording_)
        throw std::logic_error("Canvas command frame already recording");
    commands_.clear();
    recording_ = true;
}
void Renderer::End() {
    if (!recording_)
        throw std::logic_error("Canvas command frame not recording");
    recording_ = false;
    frame_->Render();
}

Renderer::Renderer(void *window, std::span<const SpriteEntry> sprites, int atlasResource)
    : window_(static_cast<HWND>(window)), sprites_(sprites.begin(), sprites.end()), atlasResource_(atlasResource) {
    GetWindowRect(window_, &windowedRect_);
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.OutputWindow = window_;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    D3D_FEATURE_LEVEL feature{};
    HRESULT created = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                                    D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                                                    &description, &swap_, &device_, &feature, &context_);
    if (FAILED(created))
        created = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                                                D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                                                &description, &swap_, &device_, &feature, &context_);
    Check(created, "Create D3D11 renderer");
    Ptr<IDXGIDevice> dxgiDevice;
    Check(device_.As(&dxgiDevice), "Get DXGI device");
    Ptr<IDXGIAdapter> adapter;
    Check(dxgiDevice->GetAdapter(&adapter), "Get adapter");
    Ptr<IDXGIFactory> dxgiFactory;
    Check(adapter->GetParent(IID_PPV_ARGS(&dxgiFactory)), "Get DXGI factory");
    Check(dxgiFactory->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER), "Configure mode ownership");
    Check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf()), "Create Direct2D factory");
    Check(factory_->CreateDevice(dxgiDevice.Get(), &d2dDevice_), "Create Direct2D device");
    Check(d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &canvas_), "Create drawing context");
    Check(canvas_->CreateSolidColorBrush(Color(0xffffff), &brush_), "Create brush");
    Check(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                              reinterpret_cast<IUnknown **>(textFactory_.GetAddressOf())),
          "Create text factory");
    Resize();
    frame_ = std::make_unique<FrameDriver>(*this);
}
Renderer::~Renderer() {
    frame_.reset();
    if (swap_)
        static_cast<void>(swap_->SetFullscreenState(FALSE, nullptr));
}
bool Renderer::IsExclusive() const {
    BOOL active = FALSE;
    Check(swap_->GetFullscreenState(&active, nullptr), "Read fullscreen state");
    return active != FALSE;
}
void Renderer::Target() {
    Ptr<IDXGISurface> surface;
    Check(swap_->GetBuffer(0, IID_PPV_ARGS(&surface)), "Get backbuffer");
    const auto properties =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96, 96);
    Check(canvas_->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &target_), "Bind Direct2D backbuffer");
    canvas_->SetTarget(target_.Get());
    canvas_->SetDpi(96, 96);
}
void Renderer::SetClientExtent(unsigned width,unsigned height){
    if(mode_!=DisplayMode::Windowed || width<320 || height<320 || width>8192 || height>8192)throw std::invalid_argument("Invalid windowed client extent");
    RECT rect{0,0,static_cast<LONG>(width),static_cast<LONG>(height)};
    if(!AdjustWindowRectEx(&rect,static_cast<DWORD>(GetWindowLongPtrW(window_,GWL_STYLE)),FALSE,static_cast<DWORD>(GetWindowLongPtrW(window_,GWL_EXSTYLE))))throw std::runtime_error("Window frame size");
    if(!SetWindowPos(window_,nullptr,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE))throw std::runtime_error("Window client resize");
    Resize();
}
void Renderer::Resize() {
    RECT rect{};
    GetClientRect(window_, &rect);
    const UINT w = static_cast<UINT>(rect.right), h = static_cast<UINT>(rect.bottom);
    if (!w || !h || (width_ == w && height_ == h && target_))
        return;
    canvas_->SetTarget(nullptr);
    target_.Reset();
    context_->ClearState();
    context_->Flush();
    Check(swap_->ResizeBuffers(2, w, h, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH),
          "Resize backbuffer");
    width_ = w;
    height_ = h;
    Target();
    scale_ = std::min(static_cast<float>(width_) / std::min(1280.f,static_cast<float>(width_)), static_cast<float>(height_) / 720);
    offsetX_ = (static_cast<float>(width_) - std::min(1280.f,static_cast<float>(width_)) * scale_) * .5f;
    offsetY_ = (static_cast<float>(height_) - 720 * scale_) * .5f;
}
void Renderer::SetMode(DisplayMode mode) {
    if (mode == mode_)
        return;
    if (mode_ == DisplayMode::Windowed)
        GetWindowRect(window_, &windowedRect_);
    const auto oldMode = mode_;
    Check(swap_->SetFullscreenState(FALSE, nullptr), "Leave fullscreen");
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &info);
    SetLastError(0);
    const LONG_PTR style = mode == DisplayMode::Windowed ? WS_OVERLAPPEDWINDOW : WS_POPUP;
    if (!SetWindowLongPtrW(window_, GWL_STYLE, style) && GetLastError())
        throw std::runtime_error("Cannot change window style");
    const RECT destination = mode == DisplayMode::Windowed ? windowedRect_ : info.rcMonitor;
    if (!SetWindowPos(window_, HWND_NOTOPMOST, destination.left, destination.top, destination.right - destination.left,
                      destination.bottom - destination.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW))
        throw std::runtime_error("Cannot resize window");
    mode_ = mode;
    Resize();
    if (mode == DisplayMode::Fullscreen) {
        ShowWindow(window_, SW_RESTORE);
        SetForegroundWindow(window_);
        SetActiveWindow(window_);
        SetFocus(window_);
        if (GetForegroundWindow() != window_) {
            const DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
            const DWORD currentThread = GetCurrentThreadId();
            if (foregroundThread && foregroundThread != currentThread &&
                AttachThreadInput(currentThread, foregroundThread, TRUE)) {
                SetForegroundWindow(window_);
                SetActiveWindow(window_);
                SetFocus(window_);
                AttachThreadInput(currentThread, foregroundThread, FALSE);
            }
        }
        const HRESULT status = swap_->SetFullscreenState(TRUE, nullptr);
        if (FAILED(status)) {
            Ptr<IDXGIDevice> dxgiDevice;
            Ptr<IDXGIAdapter> adapter;
            DXGI_ADAPTER_DESC adapterInfo{};
            static_cast<void>(device_.As(&dxgiDevice));
            if (dxgiDevice && SUCCEEDED(dxgiDevice->GetAdapter(&adapter)))
                static_cast<void>(adapter->GetDesc(&adapterInfo));
            char detail[200]{};
            sprintf_s(detail,
                      "Enter exclusive fullscreen: focus=%d foreground=%d visible=%d remote=%d vendor=%u (0x%08lX)",
                      GetFocus() == window_, GetForegroundWindow() == window_, IsWindowVisible(window_) != FALSE,
                      GetSystemMetrics(SM_REMOTESESSION), adapterInfo.VendorId, static_cast<unsigned long>(status));
            mode_ = DisplayMode::Borderless;
            if (oldMode == DisplayMode::Windowed)
                SetMode(DisplayMode::Windowed);
            throw std::runtime_error(detail);
        }
        Resize();
    }
}
void Renderer::DrawBegin() {
    opacity_ = 1;
    transforms_.clear();
    canvas_->BeginDraw();
    canvas_->SetTransform(D2D1::Matrix3x2F::Identity());
    canvas_->Clear(Color(0x060d17));
    canvas_->SetTransform(D2D1::Matrix3x2F::Scale(scale_, scale_) * D2D1::Matrix3x2F::Translation(offsetX_, offsetY_));
}
void Renderer::DrawEnd() {
    Check(canvas_->EndDraw(), "Draw frame");
    // DXGI_SWAP_EFFECT_DISCARD does not preserve the back buffer after Present.
    // Retain a GPU copy only in capture runs; normal gameplay pays no copy cost.
    if (captureEnabled_) {
        Ptr<ID3D11Texture2D> buffer;
        Check(swap_->GetBuffer(0, IID_PPV_ARGS(&buffer)), "Retain capture buffer");
        D3D11_TEXTURE2D_DESC desc{}, previous{};
        buffer->GetDesc(&desc);
        if (capturedFrame_)
            capturedFrame_->GetDesc(&previous);
        if (!capturedFrame_ || desc.Width != previous.Width || desc.Height != previous.Height) {
            capturedFrame_.Reset();
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            Check(device_->CreateTexture2D(&desc, nullptr, &capturedFrame_), "Retain rendered frame");
        }
        context_->CopyResource(capturedFrame_.Get(), buffer.Get());
    }
    const HRESULT presented = swap_->Present(0, 0);
    if (presented != DXGI_STATUS_OCCLUDED)
        Check(presented, "Present frame");
}
void Renderer::PushTransform(const D2D1_MATRIX_3X2_F &transform, float opacity) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            PushTransform(transform, opacity);
        });
        return;
    }
    D2D1_MATRIX_3X2_F previous{};
    canvas_->GetTransform(&previous);
    transforms_.emplace_back(previous, opacity_);
    opacity_ *= opacity;
    canvas_->SetTransform(
        D2D1::Matrix3x2F(transform._11, transform._12, transform._21, transform._22, transform._31, transform._32) *
        previous);
}
void Renderer::PopTransform() {
    if (recording_) {
        commands_.emplace_back([=, this] {
            PopTransform();
        });
        return;
    }
    canvas_->SetTransform(transforms_.back().first);
    opacity_ = transforms_.back().second;
    transforms_.pop_back();
}
void Renderer::Clip(float x, float y, float w, float h) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Clip(x, y, w, h);
        });
        return;
    }
    canvas_->PushAxisAlignedClip(D2D1::RectF(x, y, x + w, y + h), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}
void Renderer::Unclip() {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Unclip();
        });
        return;
    }
    canvas_->PopAxisAlignedClip();
}
void Renderer::ClipPolygon(const std::vector<D2D1_POINT_2F> &points) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            ClipPolygon(points);
        });
        return;
    }
    Ptr<ID2D1PathGeometry> geometry;
    Ptr<ID2D1GeometrySink> sink;
    Check(factory_->CreatePathGeometry(&geometry), "Clip geometry");
    Check(geometry->Open(&sink), "Clip sink");
    sink->BeginFigure(points.front(), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    Check(sink->Close(), "Clip close");
    canvas_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), geometry.Get()), nullptr);
}
void Renderer::UnclipPolygon() {
    if (recording_) {
        commands_.emplace_back([=, this] {
            UnclipPolygon();
        });
        return;
    }
    canvas_->PopLayer();
}
void Renderer::Polygon(const std::vector<D2D1_POINT_2F> &points, unsigned color, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Polygon(points, color, alpha);
        });
        return;
    }
    if (points.size() < 3)
        return;
    Ptr<ID2D1PathGeometry> geometry;
    Ptr<ID2D1GeometrySink> sink;
    Check(factory_->CreatePathGeometry(&geometry), "Polygon geometry");
    Check(geometry->Open(&sink), "Polygon sink");
    sink->BeginFigure(points.front(), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    Check(sink->Close(), "Polygon close");
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->FillGeometry(geometry.Get(), brush_.Get());
}
void Renderer::Ellipse(float x, float y, float rx, float ry, unsigned color, float thickness, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Ellipse(x, y, rx, ry, color, thickness, alpha);
        });
        return;
    }
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->DrawEllipse(D2D1::Ellipse({x, y}, rx, ry), brush_.Get(), thickness);
}
void Renderer::RoundRect(float x, float y, float w, float h, float radius, unsigned color, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            RoundRect(x, y, w, h, radius, color, alpha);
        });
        return;
    }
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), radius, radius), brush_.Get());
}
void Renderer::Gradient(float x, float y, float w, float h, unsigned top, unsigned bottom, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Gradient(x, y, w, h, top, bottom, alpha);
        });
        return;
    }
    const D2D1_GRADIENT_STOP stops[] = {{0, Color(top, alpha * opacity_)}, {1, Color(bottom, alpha * opacity_)}};
    Ptr<ID2D1GradientStopCollection> colors;
    Ptr<ID2D1LinearGradientBrush> gradient;
    Check(canvas_->CreateGradientStopCollection(stops, 2, &colors), "Gradient stops");
    Check(canvas_->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({x, y}, {x + w * .35f, y + h}),
                                             colors.Get(), &gradient),
          "Gradient brush");
    canvas_->FillRectangle(D2D1::RectF(x, y, x + w, y + h), gradient.Get());
}
void Renderer::Bezier(D2D1_POINT_2F start, D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_POINT_2F end, unsigned color,
                      float thickness, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Bezier(start, a, b, end, color, thickness, alpha);
        });
        return;
    }
    Ptr<ID2D1PathGeometry> geometry;
    Ptr<ID2D1GeometrySink> sink;
    Check(factory_->CreatePathGeometry(&geometry), "Curve geometry");
    Check(geometry->Open(&sink), "Curve sink");
    sink->BeginFigure(start, D2D1_FIGURE_BEGIN_HOLLOW);
    sink->AddBezier({a, b, end});
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    Check(sink->Close(), "Curve close");
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->DrawGeometry(geometry.Get(), brush_.Get(), thickness);
}
void Renderer::Vignette(float w, float h) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Vignette(w, h);
        });
        return;
    }
    const D2D1_GRADIENT_STOP stops[] = {
        {0, Color(0x030810, 0)}, {h * .14f / (w * .88f), Color(0x030810, 0)}, {1, Color(0x030810, .7f * opacity_)}};
    Ptr<ID2D1GradientStopCollection> colors;
    Ptr<ID2D1RadialGradientBrush> gradient;
    Check(canvas_->CreateGradientStopCollection(stops, 3, &colors), "Vignette stops");
    Check(canvas_->CreateRadialGradientBrush(
              D2D1::RadialGradientBrushProperties({w * .5f, h * .47f}, {0, 0}, w * .88f, w * .88f), colors.Get(),
              &gradient),
          "Vignette brush");
    canvas_->FillRectangle(D2D1::RectF(0, 0, w, h), gradient.Get());
}
void Renderer::Rect(float x, float y, float w, float h, unsigned color, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Rect(x, y, w, h, color, alpha);
        });
        return;
    }
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush_.Get());
}
void Renderer::Line(float x, float y, float x2, float y2, unsigned color, float thickness, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Line(x, y, x2, y2, color, thickness, alpha);
        });
        return;
    }
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->DrawLine({x, y}, {x2, y2}, brush_.Get(), thickness);
}
void Renderer::Circle(float x, float y, float radius, unsigned color, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Circle(x, y, radius, color, alpha);
        });
        return;
    }
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->FillEllipse(D2D1::Ellipse({x, y}, radius, radius), brush_.Get());
}
void Renderer::Outline(float x, float y, float radius, unsigned color, float thickness, float alpha) {
    if (recording_) {
        commands_.emplace_back([=, this] {
            Outline(x, y, radius, color, thickness, alpha);
        });
        return;
    }
    brush_->SetColor(Color(color, alpha * opacity_));
    canvas_->DrawEllipse(D2D1::Ellipse({x, y}, radius, radius), brush_.Get(), thickness);
}
void Renderer::BeginContour(){
    if(recording_){commands_.emplace_back([this]{BeginContour();});return;}
    if(contour_)throw std::logic_error("Nested contour groups are unsupported");
    canvas_->GetTarget(&contourTarget_);canvas_->GetTransform(&contourTransform_);
    Check(canvas_->CreateCommandList(&contour_),"Contour command list");canvas_->SetTarget(contour_.Get());
}
void Renderer::EndContour(){
    if(recording_){commands_.emplace_back([this]{EndContour();});return;}
    if(!contour_)throw std::logic_error("Contour group was not begun");
    canvas_->SetTarget(contourTarget_.Get());contourTarget_.Reset();auto list=std::move(contour_);Check(list->Close(),"Close contour list");
    Ptr<ID2D1Effect> gray,edge;Check(canvas_->CreateEffect(CLSID_D2D1ColorMatrix,&gray),"Contour luminance");
    gray->SetInput(0,list.Get());const D2D1_MATRIX_5X4_F matrix={.2126f,.2126f,.2126f,0,.7152f,.7152f,.7152f,0,.0722f,.0722f,.0722f,0,0,0,0,1,0,0,0,0};
    Check(gray->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX,matrix),"Luminance matrix");
    Check(canvas_->CreateEffect(CLSID_D2D1EdgeDetection,&edge),"Contour edges");edge->SetInputEffect(0,gray.Get());
    Check(edge->SetValue(D2D1_EDGEDETECTION_PROP_STRENGTH,1.8f),"Contour strength");
    Check(edge->SetValue(D2D1_EDGEDETECTION_PROP_BLUR_RADIUS,.35f),"Contour radius");
    Check(edge->SetValue(D2D1_EDGEDETECTION_PROP_OVERLAY_EDGES,FALSE),"Contour-only world");
    canvas_->SetTransform(D2D1::Matrix3x2F::Identity());canvas_->DrawImage(edge.Get());canvas_->SetTransform(contourTransform_);
}
void Renderer::BeginBlur() {
    if(recording_){commands_.emplace_back([this]{BeginBlur();});return;}
    if(blur_)throw std::logic_error("Nested blur groups are unsupported");
    canvas_->GetTarget(&blurTarget_);canvas_->GetTransform(&blurTransform_);
    Check(canvas_->CreateCommandList(&blur_),"Blur command list");canvas_->SetTarget(blur_.Get());
}
void Renderer::EndBlur(float radius) {
    if(recording_){commands_.emplace_back([=,this]{EndBlur(radius);});return;}
    if(!blur_)throw std::logic_error("Blur group was not begun");
    canvas_->SetTarget(blurTarget_.Get());blurTarget_.Reset();
    auto list=std::move(blur_);Check(list->Close(),"Close blur command list");
    Ptr<ID2D1Effect> effect;Check(canvas_->CreateEffect(CLSID_D2D1GaussianBlur,&effect),"Backdrop blur");
    effect->SetInput(0,list.Get());
    Check(effect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,std::clamp(radius,0.f,12.f)),"Blur radius");
    canvas_->SetTransform(D2D1::Matrix3x2F::Identity());canvas_->DrawImage(effect.Get());canvas_->SetTransform(blurTransform_);
}
void Renderer::BeginPerspective(float rx,float ry,D2D1_POINT_2F center) {
    if(recording_){commands_.emplace_back([=,this]{BeginPerspective(rx,ry,center);});return;}
    if(perspective_)throw std::logic_error("Nested perspective groups are unsupported");
    canvas_->GetTarget(&perspectiveTarget_);canvas_->GetTransform(&perspectiveTransform_);
    perspectiveX_=rx;perspectiveY_=ry;
    perspectiveCenterX_=center.x*perspectiveTransform_._11+center.y*perspectiveTransform_._21+perspectiveTransform_._31;
    perspectiveCenterY_=center.x*perspectiveTransform_._12+center.y*perspectiveTransform_._22+perspectiveTransform_._32;
    Check(canvas_->CreateCommandList(&perspective_),"Perspective command list");canvas_->SetTarget(perspective_.Get());
}
void Renderer::EndPerspective() {
    if(recording_){commands_.emplace_back([this]{EndPerspective();});return;}
    if(!perspective_)throw std::logic_error("Perspective group was not begun");
    canvas_->SetTarget(perspectiveTarget_.Get());perspectiveTarget_.Reset();
    auto list=std::move(perspective_);Check(list->Close(),"Close perspective command list");
    Ptr<ID2D1Effect> effect;Check(canvas_->CreateEffect(CLSID_D2D13DTransform,&effect),"Perspective effect");
    effect->SetInput(0,list.Get());
    const auto matrix=D2D1::Matrix4x4F::Translation(-perspectiveCenterX_,-perspectiveCenterY_,0)*
        D2D1::Matrix4x4F::RotationX(perspectiveX_)*D2D1::Matrix4x4F::RotationY(perspectiveY_)*
        D2D1::Matrix4x4F::PerspectiveProjection(1000)*D2D1::Matrix4x4F::Translation(perspectiveCenterX_,perspectiveCenterY_,0);
    Check(effect->SetValue(D2D1_3DTRANSFORM_PROP_TRANSFORM_MATRIX,matrix),"Perspective matrix");
    canvas_->SetTransform(D2D1::Matrix3x2F::Identity());canvas_->DrawImage(effect.Get());canvas_->SetTransform(perspectiveTransform_);
}
void Renderer::WarpedSprite(std::string_view name,float x,float y,float size,float phase,float strength,float coreRadius,float alpha){
    if(recording_){commands_.emplace_back([=,name=std::string(name),this]{WarpedSprite(name,x,y,size,phase,strength,coreRadius,alpha);});return;}
    const auto entry=std::find_if(sprites_.begin(),sprites_.end(),[&](const auto &sprite){return sprite.name==name;});if(entry==sprites_.end())return;
    if(strength<=0){Sprite(name,x,y,size,alpha);return;}
    const float width=entry->width*size/256,height=entry->height*size/256,left=x-entry->anchorX*size/256,top=y-entry->anchorY*size/256;
    D2D1_MATRIX_3X2_F previous{};canvas_->GetTransform(&previous);Ptr<ID2D1Image> target;canvas_->GetTarget(&target);
    Ptr<ID2D1CommandList> image;Check(canvas_->CreateCommandList(&image),"Deform sprite command list");canvas_->SetTarget(image.Get());canvas_->SetTransform(D2D1::Matrix3x2F::Identity());
    Sprite(name,x,y,size,alpha);canvas_->SetTarget(target.Get());canvas_->SetTransform(previous);Check(image->Close(),"Close deform sprite");
    // A shared continuous field bends the silhouette; there are no independently
    // rotated sectors, clipped joints or gaps. The protected core keeps eyes stable.
    constexpr unsigned side=64;std::vector<unsigned char> pixels(side*side*4);
    for(unsigned row=0;row<side;++row)for(unsigned col=0;col<side;++col){
        const float px=left+width*(col+.5f)/side-x,py=top+height*(row+.5f)/side-y;
        const float t=std::clamp((std::hypot(px,py)-coreRadius)/std::max(1.f,std::max(width,height)*.55f),0.f,1.f),weight=t*t*(3-2*t);
        const auto i=(row*side+col)*4;pixels[i]=128;pixels[i+1]=static_cast<unsigned char>(128+100*weight*std::sin(phase*1.31f+px*.014f-py*.006f));pixels[i+2]=static_cast<unsigned char>(128+100*weight*std::sin(phase*1.57f+py*.016f+px*.005f));pixels[i+3]=255;
    }
    Ptr<ID2D1Bitmap1> field;const auto properties=D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED));
    Check(canvas_->CreateBitmap(D2D1::SizeU(side,side),pixels.data(),side*4,&properties,&field),"Deform field bitmap");
    Ptr<ID2D1Effect> border,map,deform;Check(canvas_->CreateEffect(CLSID_D2D1Border,&border),"Deform field boundary");border->SetInput(0,field.Get());
    Check(border->SetValue(D2D1_BORDER_PROP_EDGE_MODE_X,D2D1_BORDER_EDGE_MODE_CLAMP),"Deform field X clamp");Check(border->SetValue(D2D1_BORDER_PROP_EDGE_MODE_Y,D2D1_BORDER_EDGE_MODE_CLAMP),"Deform field Y clamp");
    Check(canvas_->CreateEffect(CLSID_D2D12DAffineTransform,&map),"Deform field transform");map->SetInputEffect(0,border.Get());
    Check(map->SetValue(D2D1_2DAFFINETRANSFORM_PROP_TRANSFORM_MATRIX,D2D1::Matrix3x2F::Scale(width/side,height/side)*D2D1::Matrix3x2F::Translation(left,top)),"Deform field coordinates");
    Check(canvas_->CreateEffect(CLSID_D2D1DisplacementMap,&deform),"Continuous sprite displacement");deform->SetInput(0,image.Get());deform->SetInputEffect(1,map.Get());
    Check(deform->SetValue(D2D1_DISPLACEMENTMAP_PROP_SCALE,strength*2),"Deform strength");Check(deform->SetValue(D2D1_DISPLACEMENTMAP_PROP_X_CHANNEL_SELECT,D2D1_CHANNEL_SELECTOR_R),"Deform X");Check(deform->SetValue(D2D1_DISPLACEMENTMAP_PROP_Y_CHANNEL_SELECT,D2D1_CHANNEL_SELECTOR_G),"Deform Y");
    canvas_->DrawImage(deform.Get());
}
struct RendererTexture {
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> context;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
};
RendererTexturePtr Renderer::CreateTexture(unsigned width,unsigned height,std::span<const std::byte> pixels) {
    if(recording_ || !width || !height || width>4096 || height>4096 || pixels.size()!=static_cast<std::size_t>(width)*height*4)
        throw std::invalid_argument("Invalid external BGRA texture");
    auto texture=std::make_shared<RendererTexture>();texture->context=canvas_;
    const auto properties=D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED));
    Check(canvas_->CreateBitmap(D2D1::SizeU(width,height),pixels.data(),width*4,&properties,&texture->bitmap),"Project texture upload");
    return texture;
}
void Renderer::Texture(RendererTexturePtr texture,float x,float y,float width,float height,float alpha) {
    if(!texture)return;
    if(texture->context.Get()!=canvas_.Get())throw std::invalid_argument("Texture belongs to another renderer");
    if(recording_){commands_.emplace_back([=,this]{Texture(texture,x,y,width,height,alpha);});return;}
    if(width<=0 || height<=0 || alpha<=0)return;
    canvas_->DrawBitmap(texture->bitmap.Get(),D2D1::RectF(x,y,x+width,y+height),std::clamp(alpha*opacity_,0.f,1.f),D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
}
void Renderer::Image(int id,float x,float y,float width,float height,float alpha){
    if(recording_){commands_.emplace_back([=,this]{Image(id,x,y,width,height,alpha);});return;}
    if(alpha<=0||width<=0||height<=0)return;
    auto &bitmap=images_[id];
    if(!bitmap){
        const auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(id),RT_RCDATA);
        if(!resource)throw std::runtime_error("Embedded image missing");
        auto *data=static_cast<BYTE*>(LockResource(LoadResource(nullptr,resource)));const auto bytes=SizeofResource(nullptr,resource);
        Ptr<IWICImagingFactory> imaging;Check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&imaging)),"Image WIC");
        Ptr<IWICStream> stream;Check(imaging->CreateStream(&stream),"Image stream");Check(stream->InitializeFromMemory(data,bytes),"Image memory");
        Ptr<IWICBitmapDecoder> decoder;Check(imaging->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder),"Image decoder");
        Ptr<IWICBitmapFrameDecode> frame;Check(decoder->GetFrame(0,&frame),"Image frame");Ptr<IWICFormatConverter> converter;Check(imaging->CreateFormatConverter(&converter),"Image converter");
        Check(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom),"Image format");
        Check(canvas_->CreateBitmapFromWicBitmap(converter.Get(),nullptr,&bitmap),"Image bitmap");
    }
    canvas_->DrawBitmap(bitmap.Get(),D2D1::RectF(x,y,x+width,y+height),std::clamp(alpha*opacity_,0.f,1.f),D2D1_INTERPOLATION_MODE_LINEAR);
}
void Renderer::Sprite(std::string_view name, float x, float y, float size, float alpha, bool platform, float height,
                      float rotation) {
    if (recording_) {
        commands_.emplace_back([=, name = std::string(name), this] {
            Sprite(name, x, y, size, alpha, platform, height, rotation);
        });
        return;
    }
    if (!atlas_) {
        const auto resource = FindResourceW(nullptr, MAKEINTRESOURCEW(atlasResource_), RT_RCDATA);
        if (!resource)
            throw std::runtime_error("Embedded sprite atlas missing");
        const auto loaded = LoadResource(nullptr, resource);
        auto *data = static_cast<BYTE *>(LockResource(loaded));
        const DWORD bytes = SizeofResource(nullptr, resource);
        Ptr<IWICImagingFactory> imaging;
        Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imaging)),
              "Atlas WIC");
        Ptr<IWICStream> stream;
        Check(imaging->CreateStream(&stream), "Atlas stream");
        Check(stream->InitializeFromMemory(data, bytes), "Atlas memory");
        Ptr<IWICBitmapDecoder> decoder;
        Check(imaging->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder),
              "Atlas PNG");
        Ptr<IWICBitmapFrameDecode> frame;
        Check(decoder->GetFrame(0, &frame), "Atlas frame");
        Ptr<IWICFormatConverter> converter;
        Check(imaging->CreateFormatConverter(&converter), "Atlas converter");
        Check(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0,
                                    WICBitmapPaletteTypeCustom),
              "Atlas format");
        Check(canvas_->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &atlas_), "Atlas bitmap");
    }
    const auto it = std::find_if(sprites_.begin(), sprites_.end(), [&](const auto &s) {
        return s.name == name;
    });
    if (it == sprites_.end())
        return;
    const float sx = static_cast<float>(it->x), sy = static_cast<float>(it->y);
    const auto src = D2D1::RectF(sx, sy, sx + it->width, sy + (platform ? 48 : it->height));
    const auto dest = platform ? D2D1::RectF(x, y, x + size, y + height)
                               : D2D1::RectF(x-it->anchorX*size/256, y-it->anchorY*size/256, x+(it->width-it->anchorX)*size/256, y+(it->height-it->anchorY)*size/256);
    D2D1_MATRIX_3X2_F previous{};
    canvas_->GetTransform(&previous);
    if (rotation)
        canvas_->SetTransform(D2D1::Matrix3x2F::Rotation(rotation * 57.2957795f, {x, y}) * previous);
    canvas_->DrawBitmap(atlas_.Get(), dest, alpha * opacity_, D2D1_INTERPOLATION_MODE_LINEAR, src);
    if (rotation)
        canvas_->SetTransform(previous);
}
void Renderer::Paragraph(std::wstring_view text, float x, float y, float size, unsigned color, float width,
                         float height, bool center) {
    if (recording_) {
        commands_.emplace_back([=, text = std::wstring(text), this] {
            Paragraph(text, x, y, size, color, width, height, center);
        });
        return;
    }
    Ptr<IDWriteTextLayout> layout;
    Check(textFactory_->CreateTextLayout(text.data(), static_cast<UINT32>(text.size()), Font(size), width, height,
                                         &layout),
          "Paragraph layout");
    Check(layout->SetWordWrapping(center?DWRITE_WORD_WRAPPING_NO_WRAP:DWRITE_WORD_WRAPPING_WRAP), "Paragraph wrap");
    if(center){Check(layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER),"Center text");Check(layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER),"Center paragraph");}
    brush_->SetColor(Color(color, opacity_));
    canvas_->DrawTextLayout({x, y}, layout.Get(), brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
IDWriteTextFormat *Renderer::Font(float size, bool bold) {
    const float key = size + (bold ? 1000 : 0);
    for (const auto &item : fonts_)
        if (item.first == key)
            return item.second.Get();
    Ptr<IDWriteTextFormat> format;
    Check(textFactory_->CreateTextFormat(L"Segoe UI", nullptr,
                                         bold || size >= 20 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                         DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"ru-ru", &format),
          "Create font");
    Check(format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP), "Configure text");
    fonts_.emplace_back(key, format);
    return format.Get();
}
void Renderer::Text(std::wstring_view text, float x, float y, float size, unsigned color, float width) {
    if (recording_) {
        commands_.emplace_back([=, text = std::wstring(text), this] {
            Text(text, x, y, size, color, width);
        });
        return;
    }
    brush_->SetColor(Color(color, opacity_));
    canvas_->DrawTextW(text.data(), static_cast<UINT32>(text.size()), Font(size, true),
                       D2D1::RectF(x, y, x + width, y + size * 1.8f), brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
D2D1_POINT_2F Renderer::MousePoint() const {
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(window_, &cursor);
    return {(static_cast<float>(cursor.x) - offsetX_) / scale_, (static_cast<float>(cursor.y) - offsetY_) / scale_};
}
void Renderer::Capture(const std::filesystem::path &path) {
    std::filesystem::create_directories(path.parent_path());
    Ptr<ID3D11Texture2D> buffer;
    if (capturedFrame_)
        buffer = capturedFrame_;
    else
        Check(swap_->GetBuffer(0, IID_PPV_ARGS(&buffer)), "Capture buffer");
    D3D11_TEXTURE2D_DESC desc{};
    buffer->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    Ptr<ID3D11Texture2D> staging;
    Check(device_->CreateTexture2D(&desc, nullptr, &staging), "Capture texture");
    context_->CopyResource(staging.Get(), buffer.Get());
    D3D11_MAPPED_SUBRESOURCE pixels{};
    Check(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &pixels), "Read pixels");
    struct Unmap {
        ID3D11DeviceContext *c;
        ID3D11Texture2D *t;
        ~Unmap() {
            c->Unmap(t, 0);
        }
    } unmap{context_.Get(), staging.Get()};
    Ptr<IWICImagingFactory> imaging;
    Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imaging)),
          "Capture WIC");
    Ptr<IWICStream> stream;
    Check(imaging->CreateStream(&stream), "Capture stream");
    Check(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE), "Capture file");
    Ptr<IWICBitmapEncoder> encoder;
    Check(imaging->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder), "Capture PNG");
    Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache), "Initialize PNG");
    Ptr<IWICBitmapFrameEncode> frame;
    Check(encoder->CreateNewFrame(&frame, nullptr), "Create PNG frame");
    Check(frame->Initialize(nullptr), "Initialize frame");
    Check(frame->SetSize(desc.Width, desc.Height), "Set PNG size");
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    Check(frame->SetPixelFormat(&format), "Set PNG format");
    Check(frame->WritePixels(desc.Height, pixels.RowPitch, pixels.RowPitch * desc.Height,
                             static_cast<BYTE *>(pixels.pData)),
          "Write PNG pixels");
    Check(frame->Commit(), "Commit PNG frame");
    Check(encoder->Commit(), "Commit PNG");
}
}
