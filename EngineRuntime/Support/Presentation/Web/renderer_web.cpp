#include "renderer_web.h"
#include <algorithm>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <map>
#include <stdexcept>

namespace epidemic::runtime::presentation {
namespace {
EM_JS(void, Submit, (const float *data, int length, const char *strings),
      { Module.epidemicRender(HEAPF32.subarray(data >> 2, (data >> 2) + length), strings); });

std::string Utf8(std::wstring_view value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        auto c = static_cast<unsigned>(value[i]);
        // The existing game's locale loader deliberately stores UTF-16 pairs.
        if (c >= 0xd800 && c <= 0xdbff && i + 1 < value.size()) {
            const auto low = static_cast<unsigned>(value[i + 1]);
            if (low >= 0xdc00 && low <= 0xdfff) {
                c = 0x10000 + ((c - 0xd800) << 10) + (low - 0xdc00);
                ++i;
            }

        }

        if (c < 128)
            out += static_cast<char>(c);
        else if (c < 2048) {
            out += static_cast<char>(192 | (c >> 6));
            out += static_cast<char>(128 | (c & 63));
        } else if (c < 65536) {
            out += static_cast<char>(224 | (c >> 12));
            out += static_cast<char>(128 | ((c >> 6) & 63));
            out += static_cast<char>(128 | (c & 63));
        } else {
            out += static_cast<char>(240 | (c >> 18));
            out += static_cast<char>(128 | ((c >> 12) & 63));
            out += static_cast<char>(128 | ((c >> 6) & 63));
            out += static_cast<char>(128 | (c & 63));
        }

    }

    return out;
}

} // namespace

struct Renderer::Impl {
    std::map<std::string_view, SpriteEntry, std::less<>> sprites;
    int atlas;
    std::vector<float> data;
    std::string strings;

    void Add(std::initializer_list<float> values) {
        data.insert(data.end(), values);
    }

    void Points(int op, const std::vector<RendererPoint> &points, unsigned color = 0,
                float alpha = 1) {
        Add({float(op), float(points.size()), float(color), alpha});
        for (auto p : points)
            Add({p.x, p.y});
    }

};

Renderer::Renderer(void *, std::span<const SpriteEntry> sprites, int atlas)
    : impl_(std::make_unique<Impl>()) {
    impl_->atlas = atlas;
    for (const auto &s : sprites)
        impl_->sprites.emplace(s.name, s);
    Resize();
}

Renderer::~Renderer() = default;

const wchar_t *ModeName(DisplayMode mode) {
    return mode == DisplayMode::Windowed ? L"Windowed" : L"Browser fullscreen";
}

void Renderer::Resize() {
    int w = 0, h = 0;
    emscripten_get_canvas_element_size("#canvas", &w, &h);
    width_ = static_cast<unsigned>(std::max(1, w));
    height_ = static_cast<unsigned>(std::max(1, h));
    scale_ = std::min(float(width_) / std::min(1280.f, float(width_)), float(height_) / 720.f);
    offsetX_ = (float(width_) - std::min(1280.f, float(width_)) * scale_) * .5f;
    offsetY_ = (float(height_) - 720.f * scale_) * .5f;
}

void Renderer::SetClientExtent(unsigned w, unsigned h) {
    if (w < 320 || h < 320 || w > 4096 || h > 4096)
        throw std::invalid_argument("Invalid browser extent");
    EM_ASM(
        {
            Module.canvas.style.maxWidth = $0 + 'px';
            Module.canvas.style.aspectRatio = $0 + '/' + $1;
        },
        w, h);
    Resize();
}

void Renderer::SetMode(DisplayMode mode) {
    mode_ = mode;
    EM_ASM(
        {
            if ($0) {
                if (!document.fullscreenElement)
                    Module.canvas.requestFullscreen().catch(
                        e = > { document.querySelector('#status').textContent = e.message; });
            } else if (document.fullscreenElement)
                document.exitFullscreen();
        },
        int(mode));
}

bool Renderer::IsExclusive() const {
    return false;
}

DisplayMode Renderer::Mode() const {
    return EM_ASM_INT({ return !!document.fullscreenElement; }) ? mode_ : DisplayMode::Windowed;
}

void Renderer::Begin() {
    Resize();
    impl_->data.clear();
    impl_->strings.clear();
    impl_->Add({0, float(width_), float(height_), scale_, offsetX_, offsetY_});
}

void Renderer::End() {
    Submit(impl_->data.data(), static_cast<int>(impl_->data.size()), impl_->strings.c_str());
}

void Renderer::PushTransform(const RendererMatrix &m, float opacity) {
    impl_->Add({1, m._11, m._12, m._21, m._22, m._31, m._32, opacity});
}

void Renderer::PopTransform() {
    impl_->Add({2});
}

void Renderer::Rect(float x, float y, float w, float h, unsigned c, float a) {
    impl_->Add({3, x, y, w, h, float(c), a});
}

void Renderer::Line(float x, float y, float xx, float yy, unsigned c, float t, float a) {
    impl_->Add({4, x, y, xx, yy, float(c), t, a});
}

void Renderer::Circle(float x, float y, float r, unsigned c, float a) {
    impl_->Add({5, x, y, r, float(c), a});
}

void Renderer::Outline(float x, float y, float r, unsigned c, float t, float a) {
    Ellipse(x, y, r, r, c, t, a);
}

void Renderer::Ellipse(float x, float y, float rx, float ry, unsigned c, float t, float a) {
    impl_->Add({6, x, y, rx, ry, float(c), t, a});
}

void Renderer::RoundRect(float x, float y, float w, float h, float r, unsigned c, float a) {
    impl_->Add({7, x, y, w, h, r, float(c), a});
}

void Renderer::Gradient(float x, float y, float w, float h, unsigned top, unsigned bottom,
                        float a) {
    impl_->Add({8, x, y, w, h, float(top), float(bottom), a});
}

void Renderer::Bezier(RendererPoint s, RendererPoint a, RendererPoint b, RendererPoint e,
                      unsigned c, float t, float alpha) {
    impl_->Add({9, s.x, s.y, a.x, a.y, b.x, b.y, e.x, e.y, float(c), t, alpha});
}

void Renderer::Polygon(const std::vector<RendererPoint> &p, unsigned c, float a) {
    impl_->Points(10, p, c, a);
}

void Renderer::Clip(float x, float y, float w, float h) {
    impl_->Add({11, x, y, w, h});
}

void Renderer::Unclip() {
    impl_->Add({2});
}

void Renderer::ClipPolygon(const std::vector<RendererPoint> &p) {
    impl_->Points(12, p);
}

void Renderer::UnclipPolygon() {
    impl_->Add({2});
}

void Renderer::Vignette(float w, float h) {
    impl_->Add({13, w, h});
}

void Renderer::Sprite(std::string_view name, float x, float y, float size, float alpha,
                      bool platform, float height, float rotation) {
    const auto it = impl_->sprites.find(name);
    if (it == impl_->sprites.end() || alpha <= 0)
        return;
    const auto &s = it->second;
    impl_->Add({14, float(impl_->atlas), float(s.x), float(s.y), float(s.width),
                float(platform ? 48 : s.height), x, y, platform ? 0 : float(s.anchorX) * size / 256,
                platform ? 0 : float(s.anchorY) * size / 256,
                platform ? size : float(s.width) * size / 256,
                platform ? height : float(s.height) * size / 256, rotation, alpha, 0, 0, 0});
}

void Renderer::WarpedSprite(std::string_view name, float x, float y, float size, float phase,
                            float strength, float core, float alpha) {
    const auto start = impl_->data.size();
    Sprite(name, x, y, size, alpha, false, 256, 0);
    if (impl_->data.size() > start) {
        auto &d = impl_->data;
        d[d.size() - 3] = phase;
        d[d.size() - 2] = strength;
        d[d.size() - 1] = core;
    }

}

void Renderer::Image(int id, float x, float y, float w, float h, float a) {
    impl_->Add({15, float(id), x, y, w, h, a});
}

void Renderer::Paragraph(std::wstring_view text, float x, float y, float size, unsigned c, float w,
                         float h, bool center) {
    auto utf = Utf8(text);
    const auto offset = impl_->strings.size();
    impl_->strings += utf;
    impl_->strings += '\0';
    impl_->Add(
        {16, x, y, size, float(c), w, h, center ? 1.f : 0.f, float(offset), float(utf.size()), 1});
}

void Renderer::Text(std::wstring_view text, float x, float y, float size, unsigned c, float w) {
    Paragraph(text, x, y, size, c, w, size * 1.5f, false);
    impl_->data.back() = 0;
}

void Renderer::BeginBlur() {
    impl_->Add({17, 0, 0, 0, 0, 0});
}

void Renderer::EndBlur(float radius) {
    impl_->Add({18, std::clamp(radius, 0.f, 12.f)});
}

void Renderer::BeginContour() {
    impl_->Add({17, 1, 0, 0, 0, 0});
}

void Renderer::EndContour() {
    impl_->Add({18, 0});
}

void Renderer::BeginPerspective(float rx, float ry, RendererPoint center) {
    impl_->Add({17, 2, rx, ry, center.x, center.y});
}

void Renderer::EndPerspective() {
    impl_->Add({18, 0});
}

RendererPoint Renderer::MousePoint() const {
    const float x = float(EM_ASM_DOUBLE({ return (Module.epidemicMouse || [ 0, 0 ])[0]; })),
                y = float(EM_ASM_DOUBLE({ return (Module.epidemicMouse || [ 0, 0 ])[1]; }));
    return {(x - offsetX_) / scale_, (y - offsetY_) / scale_};

}

void Renderer::Capture(const std::filesystem::path &path) {
    const auto text = path.generic_string();
    EM_ASM(
        {
            const b = atob(Module.canvas.toDataURL('image/png').split(',')[1]);
            FS.writeFile(UTF8ToString($0), Uint8Array.from(b, c = > c.charCodeAt(0)));
        },
        text.c_str());
}

} // namespace epidemic::runtime::presentation
