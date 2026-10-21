#pragma once
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>
namespace epidemic::runtime::presentation
{
struct RendererPoint
{
    float x = 0, y = 0;
};
struct RendererMatrix
{
    float _11 = 1, _12 = 0, _21 = 0, _22 = 1, _31 = 0, _32 = 0;
};
struct SpriteEntry
{
    std::string_view name;
    int x, y;
    int width = 256, height = 256, anchorX = 128, anchorY = 128;
};
enum class DisplayMode
{
    Windowed,
    Borderless,
    Fullscreen
};
const wchar_t *ModeName(DisplayMode mode);
class Renderer
{
    struct Impl;
    std::unique_ptr<Impl> impl_;
    unsigned width_ = 1280, height_ = 720;
    float scale_ = 1, offsetX_ = 0, offsetY_ = 0;
    DisplayMode mode_ = DisplayMode::Windowed;
    bool captureEnabled_ = false;

  public:
    Renderer(void *window, std::span<const SpriteEntry> sprites, int atlasResource);
    ~Renderer();
    void SetClientExtent(unsigned width, unsigned height);
    RendererPoint ClientExtent() const
    {
        return {static_cast<float>(width_), static_cast<float>(height_)};
    }
    RendererPoint LogicalExtent() const
    {
        return {width_ / scale_, height_ / scale_};
    }
    void Resize();
    void SetMode(DisplayMode mode);
    DisplayMode Mode() const;
    bool IsExclusive() const;
    void EnableCaptures(bool enabled)
    {
        captureEnabled_ = enabled;
    }
    void Begin();
    void End();
    void PushTransform(const RendererMatrix &transform, float opacity = 1);
    void PopTransform();
    void BeginContour();
    void EndContour();
    void BeginBlur();
    void EndBlur(float radius);
    void BeginPerspective(float rx, float ry, RendererPoint center);
    void EndPerspective();
    void Clip(float x, float y, float w, float h);
    void Unclip();
    void ClipPolygon(const std::vector<RendererPoint> &points);
    void UnclipPolygon();
    void Polygon(const std::vector<RendererPoint> &points, unsigned color, float alpha = 1);
    void Ellipse(float x, float y, float rx, float ry, unsigned color, float thickness = 2, float alpha = 1);
    void RoundRect(float x, float y, float w, float h, float radius, unsigned color, float alpha = 1);
    void Gradient(float x, float y, float w, float h, unsigned top, unsigned bottom, float alpha = 1);
    void Bezier(RendererPoint start, RendererPoint a, RendererPoint b, RendererPoint end, unsigned color,
                float thickness = 2, float alpha = 1);
    void Vignette(float w, float h);
    void Rect(float x, float y, float w, float h, unsigned color, float alpha = 1);
    void Line(float x, float y, float x2, float y2, unsigned color, float thickness = 1, float alpha = 1);
    void Circle(float x, float y, float radius, unsigned color, float alpha = 1);
    void Outline(float x, float y, float radius, unsigned color, float thickness = 2, float alpha = 1);
    // Continuous displacement of one premultiplied sprite, with an anchored core.
    void WarpedSprite(std::string_view name, float x, float y, float size, float phase, float strength,
                      float coreRadius, float alpha = 1);
    void Image(int resource, float x, float y, float width, float height, float alpha = 1);
    void Sprite(std::string_view name, float x, float y, float size = 256, float alpha = 1, bool platform = false,
                float height = 256, float rotation = 0);
    void Paragraph(std::wstring_view text, float x, float y, float size, unsigned color, float width, float height,
                   bool center = false);
    void Text(std::wstring_view text, float x, float y, float size, unsigned color, float width = 1100);
    RendererPoint MousePoint() const;
    void Capture(const std::filesystem::path &path);
};
} // namespace epidemic::runtime::presentation
