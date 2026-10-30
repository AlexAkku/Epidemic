#pragma once
#include <windows.h>
#include <array>
#include <dwmapi.h>

namespace epidemic::game::editor {
// Windows-host presentation only. No authoring or runtime state lives here.
class EditorStyle final {
  public:
    COLORREF background=RGB(17,24,32),panel=RGB(27,37,48),input=RGB(17,27,37);
    COLORREF text=RGB(230,237,245),muted=RGB(157,175,193),border=RGB(68,85,104);
    COLORREF accent=RGB(22,101,168),selection=RGB(38,63,88),success=RGB(118,217,173),danger=RGB(255,150,150);
    EditorStyle() { Refresh(); }
    ~EditorStyle() { Clear(); }
    EditorStyle(const EditorStyle &)=delete;
    EditorStyle &operator=(const EditorStyle &)=delete;
    void Refresh() {
        HIGHCONTRASTW contrast{}; contrast.cbSize=sizeof(contrast);
        high_contrast_=SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0) && (contrast.dwFlags&HCF_HIGHCONTRASTON)!=0;
        if(high_contrast_) {
            background=GetSysColor(COLOR_BTNFACE); panel=input=GetSysColor(COLOR_WINDOW);
            text=GetSysColor(COLOR_WINDOWTEXT); muted=GetSysColor(COLOR_BTNTEXT); border=GetSysColor(COLOR_WINDOWTEXT);
            accent=GetSysColor(COLOR_HIGHLIGHT); selection=GetSysColor(COLOR_HIGHLIGHT); success=danger=text;
        } else {
            background=RGB(17,24,32); panel=RGB(27,37,48); input=RGB(17,27,37);
            text=RGB(230,237,245); muted=RGB(157,175,193); border=RGB(68,85,104);
            accent=RGB(22,101,168); selection=RGB(38,63,88); success=RGB(118,217,173); danger=RGB(255,150,150);
        }
        Clear(); brushes_={CreateSolidBrush(background),CreateSolidBrush(panel),CreateSolidBrush(input)};
    }
    void ApplyFrame(HWND window) const {
        const BOOL dark=!high_contrast_;
        (void)DwmSetWindowAttribute(window,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));
        const COLORREF caption=high_contrast_?DWMWA_COLOR_DEFAULT:panel;
        const COLORREF caption_text=high_contrast_?DWMWA_COLOR_DEFAULT:text;
        (void)DwmSetWindowAttribute(window,DWMWA_CAPTION_COLOR,&caption,sizeof(caption));
        (void)DwmSetWindowAttribute(window,DWMWA_TEXT_COLOR,&caption_text,sizeof(caption_text));
    }
    [[nodiscard]] HBRUSH Background() const { return brushes_[0]?brushes_[0]:GetSysColorBrush(COLOR_BTNFACE); }
    [[nodiscard]] HBRUSH Panel() const { return brushes_[1]?brushes_[1]:GetSysColorBrush(COLOR_WINDOW); }
    [[nodiscard]] HBRUSH Input() const { return brushes_[2]?brushes_[2]:GetSysColorBrush(COLOR_WINDOW); }
    [[nodiscard]] bool HighContrast() const { return high_contrast_; }
  private:
    void Clear() { for(auto &brush:brushes_) if(brush){DeleteObject(brush);brush=nullptr;} }
    std::array<HBRUSH,3> brushes_{};
    bool high_contrast_=false;
};
} // namespace epidemic::game::editor
