#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows_scene_view.h"
#include "renderer.h"
#include <d2d1_1helper.h>
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace epidemic::game::editor {
namespace {
std::wstring Wide(std::string_view text) {
    const auto size=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring result(static_cast<std::size_t>(size),L'\0');
    MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),result.data(),size); return result;
}
}
NativeSceneView::NativeSceneView(HWND window,std::function<void(runtime::SceneNodeId)> select,std::function<void(std::string)> error)
    : window_(window), select_(std::move(select)), error_(std::move(error)) {
    if (!SetWindowSubclass(window_,&Procedure,2,reinterpret_cast<DWORD_PTR>(this))) throw std::runtime_error("Cannot initialize Scene View");
}
NativeSceneView::~NativeSceneView() {
    if (GetCapture()==window_) ReleaseCapture();
    renderer_.reset(); RemoveWindowSubclass(window_,&Procedure,2);
}
void NativeSceneView::Sync(std::shared_ptr<const ProjectSnapshot> snapshot,runtime::AssetId scene,runtime::SceneNodeId selected,UINT dpi,bool reset) {
    const bool changed=reset || snapshot_!=snapshot || view_.Document()!=scene;
    const bool redraw=changed || selected_!=selected || dpi_!=dpi;
    dpi_=dpi; selected_=selected; Resize();
    if (changed) {
        if (reset) view_.Clear();
        snapshot_=std::move(snapshot);
        if (snapshot_ && scene.IsValid()) {
            const auto updated=view_.Update(*snapshot_,scene);
            if (!updated) { failure_=updated.GetError().code+": "+updated.GetError().message; error_(failure_); }
            else failure_.clear();
        } else { view_.Clear(); snapshot_.reset(); }
    }
    if (redraw) InvalidateRect(window_,nullptr,FALSE);
}
void NativeSceneView::Resize() {
    RECT rect{}; GetClientRect(window_,&rect);
    view_.Resize(static_cast<double>(rect.right)*96/dpi_,static_cast<double>(rect.bottom)*96/dpi_);
}
ViewPoint NativeSceneView::Point(LPARAM position) const { return {static_cast<double>(GET_X_LPARAM(position))*96/dpi_,static_cast<double>(GET_Y_LPARAM(position))*96/dpi_}; }
void NativeSceneView::Frame(bool all) { view_.Fit(all?runtime::SceneNodeId{}:selected_); InvalidateRect(window_,nullptr,FALSE); }
void NativeSceneView::Capture(const std::filesystem::path &file) {
    Paint();
    if (!renderer_ || !failure_.empty()) throw std::runtime_error("Scene View cannot capture a failed surface");
    renderer_->EnableCaptures(true);
    try { Paint(); renderer_->Capture(file); }
    catch (...) { renderer_->EnableCaptures(false); throw; }
    renderer_->EnableCaptures(false);
}
void NativeSceneView::Paint() {
    if (!failure_.empty()) return;
    RECT client{}; GetClientRect(window_,&client);
    if (client.right<=0 || client.bottom<=0) return;
    Resize();
    if (!renderer_) renderer_=std::make_unique<runtime::presentation::Renderer>(window_,std::span<const runtime::presentation::SpriteEntry>{},0);
    renderer_->Resize();
    const auto physical=renderer_->ClientExtent(), logical=renderer_->LogicalExtent();
    const float base=physical.x/logical.x, dpi=static_cast<float>(dpi_)/96;
    const float offset=(physical.x-std::min(1280.f,physical.x)*base)*.5f;
    const float offset_y=(physical.y-720.f*base)*.5f;
    renderer_->Begin();
    // Cancel Presentation's game letterboxing, then draw in editor DIPs.
    renderer_->PushTransform(D2D1::Matrix3x2F::Scale(dpi/base,dpi/base)*D2D1::Matrix3x2F::Translation(-offset/base,-offset_y/base));
    const auto extent=view_.Extent(); const auto w=static_cast<float>(extent.x), h=static_cast<float>(extent.y);
    renderer_->Rect(0,0,w,h,0x17222e);
    const auto world=view_.ToWorld({0,0});
    const double step=std::pow(10.,std::floor(std::log10(80/view_.Scale()))), pixel=step*view_.Scale();
    const auto origin=view_.ToScreen({0,0});
    // Modulo in screen space avoids loops proportional to world coordinates.
    const auto grid=[&](bool vertical) {
        const double length=vertical?extent.y:extent.x, start=std::fmod(vertical?origin.x:origin.y,pixel);
        const int count=static_cast<int>(std::min(2048.,std::ceil(length/pixel)+2));
        for (int index=0;index<count;++index) {
            const double value=start+index*pixel;
            if (value<0 || value>length) continue;
            const auto v=static_cast<float>(value);
            renderer_->Line(vertical?v:0,vertical?0:v,vertical?v:w,vertical?h:v,0x344351);
        }
    };
    grid(true); grid(false);
    if (origin.x>=0 && origin.x<=extent.x) renderer_->Line(static_cast<float>(origin.x),0,static_cast<float>(origin.x),h,0x78bca5,2);
    if (origin.y>=0 && origin.y<=extent.y) renderer_->Line(0,static_cast<float>(origin.y),w,static_cast<float>(origin.y),0xe2a387,2);
    for (const auto &item:view_.Items()) {
        const auto b=view_.ScreenBounds(item);
        if (b.right<0 || b.bottom<0 || b.left>extent.x || b.top>extent.y) continue;
        const auto x=static_cast<float>(std::clamp(b.left,0.,extent.x)), y=static_cast<float>(std::clamp(b.top,0.,extent.y));
        const auto right=static_cast<float>(std::clamp(b.right,0.,extent.x)), bottom=static_cast<float>(std::clamp(b.bottom,0.,extent.y));
        const unsigned color=item.id==selected_?0xffd078:0x9ac7e5;
        renderer_->Rect(x,y,right-x,bottom-y,color,item.id==selected_?.24f:.10f);
        renderer_->Line(x,y,right,y,color,2); renderer_->Line(right,y,right,bottom,color,2);
        renderer_->Line(right,bottom,x,bottom,color,2); renderer_->Line(x,bottom,x,y,color,2);
        const auto identity=item.label.rfind(" [");
        const auto label_x=std::clamp(x+4,8.f,std::max(8.f,w-148));
        const auto label_y=std::max(26.f,y-42);
        renderer_->Text(Wide(item.label.substr(0,identity)),label_x,label_y,13,color,140);
        if (identity!=std::string::npos) renderer_->Text(Wide(item.label.substr(identity+1)),label_x,label_y+18,13,color,140);
    }
    char coordinates[128];
    std::snprintf(coordinates,sizeof(coordinates),"XY authoring | grid %.3g | X %.4g  Y %.4g",step,world.x,world.y);
    renderer_->Rect(0,0,w,24,0x17222e,.95f); renderer_->Text(Wide(coordinates),8,4,12,0xe2eaf0,w-16);
    renderer_->Rect(0,h-24,w,24,0x17222e,.95f); renderer_->Text(L"MMB / empty drag pan | wheel zoom | F / Home",8,h-21,12,0xe2eaf0,w-16);
    renderer_->PopTransform(); renderer_->End();
}
LRESULT CALLBACK NativeSceneView::Procedure(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) noexcept {
    auto *self=reinterpret_cast<NativeSceneView *>(data);
    try { return self->Message(msg,wp,lp); }
    catch (const std::exception &error) {
        try { self->failure_=error.what(); self->error_("Scene View: "+self->failure_); InvalidateRect(hwnd,nullptr,FALSE); }
        catch (...) { OutputDebugStringA("Scene View: failed to publish rendering diagnostic\n"); }
        return 0;
    }
}
LRESULT NativeSceneView::Message(UINT msg,WPARAM wp,LPARAM lp) {
    if (msg==WM_ERASEBKGND) return 1;
    if (msg==WM_PAINT) {
        PAINTSTRUCT paint{}; const auto dc=BeginPaint(window_,&paint);
        // Always balance BeginPaint, including renderer/device failures.
        try {
            if (!failure_.empty()) { RECT rect{}; GetClientRect(window_,&rect); FillRect(dc,&rect,GetSysColorBrush(COLOR_WINDOW)); const auto text=Wide(failure_); DrawTextW(dc,text.c_str(),-1,&rect,DT_WORDBREAK); }
            else Paint();
        } catch (...) { EndPaint(window_,&paint); throw; }
        EndPaint(window_,&paint); return 0;
    }
    if (msg==WM_SIZE) { Resize(); InvalidateRect(window_,nullptr,FALSE); return 0; }
    if (msg==WM_LBUTTONDOWN) {
        SetFocus(window_); const auto hit=view_.Pick(Point(lp)); select_(hit);
        if (!hit.IsValid()) { panning_=true; last_=Point(lp); SetCapture(window_); }
        return 0;
    }
    if (msg==WM_MBUTTONDOWN) { SetFocus(window_); panning_=true; last_=Point(lp); SetCapture(window_); return 0; }
    if (msg==WM_MBUTTONUP || msg==WM_LBUTTONUP || msg==WM_CANCELMODE) { panning_=false; if (GetCapture()==window_) ReleaseCapture(); return 0; }
    if (msg==WM_CAPTURECHANGED) { panning_=false; return 0; }
    if (msg==WM_MOUSEMOVE && panning_) { const auto next=Point(lp); view_.Pan({next.x-last_.x,next.y-last_.y}); last_=next; InvalidateRect(window_,nullptr,FALSE); return 0; }
    if (msg==WM_MOUSEWHEEL) {
        POINT point{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)}; ScreenToClient(window_,&point);
        view_.Zoom({static_cast<double>(point.x)*96/dpi_,static_cast<double>(point.y)*96/dpi_},static_cast<double>(GET_WHEEL_DELTA_WPARAM(wp))/WHEEL_DELTA);
        InvalidateRect(window_,nullptr,FALSE); return 0;
    }
    if (msg==WM_KEYDOWN && (wp=='F' || wp==VK_HOME)) { Frame(wp==VK_HOME); return 0; }
    return DefSubclassProc(window_,msg,wp,lp);
}
} // namespace epidemic::game::editor
