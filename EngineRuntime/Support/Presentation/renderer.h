#pragma once
#if defined(__EMSCRIPTEN__)
#include "Web/renderer_web.h"
#else
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <filesystem>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>
#include <span>
#include <cstddef>
#include <map>

namespace epidemic::runtime::presentation {
struct RendererTexture;
using RendererTexturePtr = std::shared_ptr<const RendererTexture>;
using RendererPoint = D2D1_POINT_2F;
using RendererMatrix = D2D1_MATRIX_3X2_F;

struct SpriteEntry {
    std::string_view name;
    int x = 0, y = 0;
    int width = 256, height = 256, anchorX = 128, anchorY = 128;
};

enum class DisplayMode { Windowed, Borderless, Fullscreen };

const wchar_t *ModeName(DisplayMode mode);

class Renderer {
    template <class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    HWND window_{};

    std::vector<SpriteEntry> sprites_;
    int atlasResource_;
    struct FrameDriver;
    std::unique_ptr<FrameDriver> frame_;
    std::vector<std::function<void()>> commands_;
    bool recording_ = false;
    void DrawBegin();
    void DrawEnd();
    Ptr<ID3D11Device> device_;
    Ptr<ID3D11DeviceContext> context_;
    Ptr<IDXGISwapChain> swap_;
    Ptr<ID2D1Factory1> factory_;
    Ptr<ID2D1Device> d2dDevice_;
    Ptr<ID2D1DeviceContext> canvas_;
    Ptr<ID2D1Bitmap1> target_;
    Ptr<ID2D1Bitmap1> atlas_;
    std::map<int, Ptr<ID2D1Bitmap1>> images_;
    Ptr<ID2D1CommandList> blur_, contour_;
    Ptr<ID2D1Image> contourTarget_;
    D2D1_MATRIX_3X2_F contourTransform_{};

    Ptr<ID2D1Image> blurTarget_;
    D2D1_MATRIX_3X2_F blurTransform_{};

    Ptr<ID2D1CommandList> perspective_;
    Ptr<ID2D1Image> perspectiveTarget_;
    D2D1_MATRIX_3X2_F perspectiveTransform_{};

    float perspectiveX_ = 0, perspectiveY_ = 0, perspectiveCenterX_ = 0, perspectiveCenterY_ = 0;
    Ptr<ID3D11Texture2D> capturedFrame_;
    bool captureEnabled_ = false;
    Ptr<ID2D1SolidColorBrush> brush_;
    Ptr<IDWriteFactory> textFactory_;
    std::vector<std::pair<float, Ptr<IDWriteTextFormat>>> fonts_;
    RECT windowedRect_{};

    DisplayMode mode_ = DisplayMode::Windowed;
    UINT width_ = 0, height_ = 0;
    float scale_ = 1, offsetX_ = 0, offsetY_ = 0;
    float opacity_ = 1;
    std::vector<std::pair<D2D1_MATRIX_3X2_F, float>> transforms_;
    void Target();
    IDWriteTextFormat *Font(float size, bool bold = false);

  public:
    Renderer(void *window, std::span<const SpriteEntry> sprites, int atlasResource);
    ~Renderer();
    void SetClientExtent(unsigned width, unsigned height);

    D2D1_POINT_2F ClientExtent() const {
        return {static_cast<float>(width_), static_cast<float>(height_)};

    }

    D2D1_POINT_2F LogicalExtent() const {
        return {width_ / scale_, height_ / scale_};

    }

    void Resize();
    void SetMode(DisplayMode mode);

    DisplayMode Mode() const {
        return mode_;
    }

    bool IsExclusive() const;

    void EnableCaptures(bool enabled) {
        captureEnabled_ = enabled;
    }

    void Begin();
    void End();
    void PushTransform(const D2D1_MATRIX_3X2_F &transform, float opacity = 1);
    void PopTransform();
    void BeginContour();
    void EndContour();
    void BeginBlur();
    void EndBlur(float radius);
    void BeginPerspective(float rx, float ry, D2D1_POINT_2F center);
    void EndPerspective();
    void Clip(float x, float y, float w, float h);
    void Unclip();
    void ClipPolygon(const std::vector<D2D1_POINT_2F> &points);
    void UnclipPolygon();
    void Polygon(const std::vector<D2D1_POINT_2F> &points, unsigned color, float alpha = 1);
    void Ellipse(float x, float y, float rx, float ry, unsigned color, float thickness = 2,
                 float alpha = 1);
    void RoundRect(float x, float y, float w, float h, float radius, unsigned color,
                   float alpha = 1);
    void Gradient(float x, float y, float w, float h, unsigned top, unsigned bottom,
                  float alpha = 1);
    void Bezier(D2D1_POINT_2F start, D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_POINT_2F end,
                unsigned color, float thickness = 2, float alpha = 1);
    void Vignette(float w, float h);
    void Rect(float x, float y, float w, float h, unsigned color, float alpha = 1);
    void Line(float x, float y, float x2, float y2, unsigned color, float thickness = 1,
              float alpha = 1);
    void Circle(float x, float y, float radius, unsigned color, float alpha = 1);
    void Outline(float x, float y, float radius, unsigned color, float thickness = 2,
                 float alpha = 1);
    // Continuous displacement of one premultiplied sprite, with an anchored core.
    void WarpedSprite(std::string_view name, float x, float y, float size, float phase,
                      float strength, float coreRadius, float alpha = 1);
    // Owner/render thread only. Input is top-down premultiplied BGRA8; pixels
    // are copied to the GPU. Texture ownership may outlive a recorded frame.
    [[nodiscard]] RendererTexturePtr CreateTexture(unsigned width, unsigned height,
                                                   std::span<const std::byte> pixels);
    void Texture(RendererTexturePtr texture, float x, float y, float width, float height,
                 float alpha = 1);
    void Image(int resource, float x, float y, float width, float height, float alpha = 1);
    void Sprite(std::string_view name, float x, float y, float size = 256, float alpha = 1,
                bool platform = false, float height = 256, float rotation = 0);
    void Paragraph(std::wstring_view text, float x, float y, float size, unsigned color,
                   float width, float height, bool center = false);
    void Text(std::wstring_view text, float x, float y, float size, unsigned color,
              float width = 1100);
    D2D1_POINT_2F MousePoint() const;
    void Capture(const std::filesystem::path &path);
};

} // namespace epidemic::runtime::presentation

#endif
