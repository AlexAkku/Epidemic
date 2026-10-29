#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <psapi.h>
// Win32's convenience macro collides with the existing IWindowSystem API.
#undef CreateWindow
#include "Epidemic/Platform/windows_platform_runtime.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "Epidemic/Game/Editor/editor_inspector.h"
#include "windows_scene_view.h"
#include "windows_content_browser.h"
#include "windows_editor_style.h"
#include "../example/demo_game.h"
#include "../example/editor_demo.h"
#include "../example/editor_stress.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <limits>
#include <set>
#include <unordered_map>
#include <charconv>
#include <tuple>
#include <source_location>
#include <numbers>

namespace {
using namespace epidemic;
using namespace epidemic::runtime;
namespace ed = game::editor;
namespace ex = ed::example;
namespace comp = gameplay::composition;
namespace gc = game::composition;
template<class T> using Result = foundation::Result<T>;
std::wstring Wide(std::string_view text) {
    if (text.empty()) return {};
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!size) throw std::runtime_error("Invalid UTF-8");
    std::wstring out(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), size); return out;
}
std::string Utf8(std::wstring_view text) {
    if (text.empty()) return {};
    const auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("Invalid Unicode");
    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr); return out;
}
std::string Text(HWND control) {
    std::wstring text(static_cast<std::size_t>(GetWindowTextLengthW(control)) + 1, L'\0');
    text.resize(static_cast<std::size_t>(GetWindowTextW(control, text.data(), static_cast<int>(text.size())))); return Utf8(text);
}
void SetText(HWND control, std::string_view text) { const auto wide = Wide(text); SetWindowTextW(control, wide.c_str()); }
void Item(HWND list, std::string_view text, UINT message = LB_ADDSTRING) { const auto wide = Wide(text); SendMessageW(list, message, 0, reinterpret_cast<LPARAM>(wide.c_str())); }
std::string Number(std::uint64_t id) { return std::to_string(id); }
// Full, lossless base-36 identity: shorter than decimal, without prefix collisions.
std::string LabelId(std::uint64_t id) { char text[32]; const auto end=std::to_chars(std::begin(text),std::end(text),id,36).ptr; return {text,end}; }
auto SelectionKey(const ed::UiSelection &value) { return std::tuple{value.document.Raw(),value.kind,value.element,value.component.Raw(),value.part.Raw()}; }
std::string FormatNodeLabel(const graph::NodeDefinition &node) { return node.name+" ["+LabelId(node.id.Raw())+"]"; }
std::string FormatPortLabel(const graph::NodeDefinition &node, const graph::PortDefinition &port) {
    return node.name+" / "+port.name+" ["+LabelId(port.id.Raw())+"]";
}
std::string PortLabel(const graph::GraphDefinition &definition, graph::ElementId id) {
    for (const auto &node:definition.nodes) for (const auto &port:node.ports)
        if (port.id==id) return FormatPortLabel(node,port);
    return Number(id.Raw());
}
const comp::PrefabNode *Find(const comp::PrefabNode &root, std::uint64_t id) {
    if (root.id.Raw() == id) return &root;
    for (const auto &child : root.children) if (const auto *node = Find(child, id)) return node;
    return nullptr;
}
enum Control : int { New = 100, Open, Save, Undo, Redo, Play, Stop, SendEvent, Payload,
    Tree, Fields, Value, Apply, Revert, Add, Remove, NewScene, NewPrefab, NewGraph,
    Palette, Source, Destination, Connect, AddComponent, ReplaceComponent, AttachBehavior, Place, Prefabs,
    Status, Log, Hint, DocumentsLabel, PropertiesLabel, ToolsLabel, EventLabel, NodeLabel, FromLabel, ToLabel, PrefabLabel,
    ValueLabel, BehaviorLabel, BehaviorGraph, FollowPrefab, Context, ToolsHint, ProjectLabel, HistoryLabel, PreviewLabel,
    ParentLabel, SceneParents, Reparent, Viewport, ViewLabel, FrameSelection, FitScene,
    ToolMode, CoordinateSpace, SnapLabel, SnapValue, SetSnap, BooleanValue, ComponentLabel, ComponentTypes, OpenReference, RefreshInspector,
    OverrideMode, ResetOverride, ExposeParameter, DetachBinding, NewDemo, Brand, InspectorTabs, WorkspaceHint, LogLabel, NewProjectFolder, CloneProjectFolder, ContentBrowser };
struct Field {
    std::string name, value;
    std::function<Result<ed::Command>(std::string_view)> command;
    std::optional<comp::PropertyKind> kind{};
    std::string detail{};
    AssetId reference{};
    bool missing=false;
    std::optional<ed::Command> reset;
    std::function<Result<std::vector<ed::Command>>(comp::PropertyId)> binding;
    bool bound=false;
};
std::string FieldText(const Field &field) { return field.name+" = "+field.value+(field.command?"":"  [read only]"); }
class Desktop final {
  public:
    Desktop(platform::WindowPtr window, ex::Schema &schema)
        : window_(std::move(window)), schema_(schema), workspace_(schema.View(), [&schema] { return ex::Options(schema); }),
          hwnd_(window_->GetNativeHandle().As<HWND>()) {
        INITCOMMONCONTROLSEX common{sizeof(common), ICC_TREEVIEW_CLASSES|ICC_TAB_CLASSES};
        if (!InitCommonControlsEx(&common) || !SetWindowSubclass(hwnd_, &ParentProc, 1, reinterpret_cast<DWORD_PTR>(this)))
            throw std::runtime_error("Cannot initialize Editor window");
        try {
            Controls();
            scene_view_=std::make_unique<ed::NativeSceneView>(ControlHandle(Viewport),workspace_,[this](SceneNodeId node) { return PickScene(node); },
                [this](std::string error) { local_error_=std::move(error); Diagnostics(); },[this] { Refresh(); });
            Fonts(GetDpiForWindow(hwnd_)); Layout(); Refresh();
        }
        catch (...) { if (font_) DeleteObject(font_); if (heading_font_) DeleteObject(heading_font_); if(meta_font_)DeleteObject(meta_font_); RemoveWindowSubclass(hwnd_, &ParentProc, 1); throw; }
    }
    ~Desktop() {
        KillTimer(hwnd_, 1);
        scene_view_.reset();
        for (int id=New;id<=LogLabel;++id) if (const auto child=ControlHandle(id)) { SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE); RemoveWindowSubclass(child,&ChildProc,1); }
        if (font_) DeleteObject(font_);
        if (heading_font_) DeleteObject(heading_font_);
        if(meta_font_)DeleteObject(meta_font_);
        RemoveWindowSubclass(hwnd_, &ParentProc, 1);
    }
    bool Closed() const noexcept { return closed_; }
    void Load(const std::filesystem::path &file) { (void)workspace_.Open(file); Refresh(true); }
    void ContentSmoke(const std::filesystem::path &file) { ex::Take(workspace_.Open(file)); ed::SmokeContentBrowser(hwnd_,workspace_); SendMessageW(hwnd_,WM_CLOSE,0,0); }
    void Smoke(const std::filesystem::path &file, platform::IPlatformRuntime &events);
    void DemoSmoke(const std::filesystem::path &file,platform::IPlatformRuntime &events);
    void StressSmoke(const std::filesystem::path &file,platform::WindowsPlatformRuntime &events,std::size_t count);
  private:
    HWND ControlHandle(int id) const { return GetDlgItem(hwnd_, id); }
    HWND Make(int id, const wchar_t *kind, const wchar_t *label, DWORD style = 0) {
        if (lstrcmpiW(kind,L"BUTTON")==0) style|=BS_OWNERDRAW;
        if (lstrcmpiW(kind,L"COMBOBOX")==0) style|=CBS_OWNERDRAWFIXED|CBS_HASSTRINGS;
        const auto control = CreateWindowExW(0, kind, label, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10,
            hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        if (!control) throw std::runtime_error("Cannot create Editor control");
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        if (!SetWindowSubclass(control, &ChildProc, 1, reinterpret_cast<DWORD_PTR>(this))) throw std::runtime_error("Cannot initialize keyboard navigation");
        return control;
    }
    void Controls();
    void Fonts(UINT dpi);
    void Layout();
    void PaintChrome(HDC dc);
    void PaintPicker(HWND control,HDC dc);
    bool DrawControl(const DRAWITEMSTRUCT &item);
    void CaptureUi(const std::filesystem::path &file);
    void Refresh(bool new_project=false);
    void Inspector();
    void Diagnostics();
    void SyncScene(bool reset=false);
    bool PickScene(SceneNodeId node);
    int Shortcut(HWND source, WPARAM key) const noexcept;
    void SelectField(int index);
    bool Draft() const { return field_ >= 0 && static_cast<std::size_t>(field_) < fields_.size() && fields_[static_cast<std::size_t>(field_)].command && Text(ControlHandle(Value)) != fields_[static_cast<std::size_t>(field_)].value; }
    bool ApplyDraft(bool explicit_apply=false);
    bool SaveProject();
    Result<void> CreateDemoCopy(const std::filesystem::path &file);
    bool CanReplace();
    std::optional<std::filesystem::path> Choose(bool save, bool folder=false);
    void Action(int id);
    bool Execute(std::vector<ed::Command> commands, const std::optional<ed::UiSelection> &select_after={}) {
        const auto result = workspace_.Execute(std::move(commands), view_revision_);
        if (result) { if (select_after) (void)workspace_.Select(*select_after); Refresh(); } else Diagnostics(); return static_cast<bool>(result);
    }
    foundation::StringId Fresh() { return ex::Id(nonce_ + "/" + Number(++next_)); }
    HTREEITEM TreeItem(HTREEITEM parent, std::string_view label, const ed::UiSelection &selection);
    void PrefabTree(HTREEITEM parent, AssetId doc, const comp::PrefabNode &node);
    void SceneTree(HTREEITEM parent, const ed::ProjectSnapshot &snapshot, const ed::EditorDocument &doc);
    void AddField(Field field) {
        const auto text=FieldText(field);
        Item(ControlHandle(Fields),text);
        fields_.push_back(std::move(field));
    }
    void NewDocument(ed::DocumentKind kind);
    static LRESULT CALLBACK ParentProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) noexcept;
    static LRESULT CALLBACK ChildProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) noexcept;
    LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
    platform::WindowPtr window_;
    ex::Schema &schema_;
    ed::EditorWorkspace workspace_;
    std::unique_ptr<ed::NativeSceneView> scene_view_;
    HWND hwnd_;
    HFONT font_=nullptr, heading_font_=nullptr,meta_font_=nullptr;
    ed::EditorStyle style_;
    UINT dpi_=96;
    std::vector<ed::UiSelection> selections_;
    std::vector<HTREEITEM> tree_items_;
    std::vector<Field> fields_;
    std::vector<graph::ElementId> outputs_, inputs_;
    std::vector<AssetId> prefabs_, graphs_;
    std::vector<comp::PrefabComponent> component_recipes_;
    std::vector<SceneNodeId> parents_;
    std::uint64_t view_revision_ = 0, next_ = 0;
    std::string nonce_ = Number(static_cast<std::uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    std::string local_error_, log_text_;
    int field_ = -1;
    bool refreshing_ = false, closed_ = false, timer_running_ = false, scene_document_=false, edit_overrides_=false;
    bool inspector_actions_=false;
};
void Desktop::Controls() {
    const auto menu=CreateMenu();
    if(!menu)throw std::runtime_error("Cannot create Editor menu");
    const auto group=[&](const wchar_t *label,std::initializer_list<std::pair<int,const wchar_t *>> entries) {
        const auto popup=CreatePopupMenu();
        if(!popup){DestroyMenu(menu);throw std::runtime_error("Cannot create Editor submenu");}
        for(const auto &[id,title]:entries) AppendMenuW(popup,MF_STRING,static_cast<UINT_PTR>(id),title);
        AppendMenuW(menu,MF_POPUP,reinterpret_cast<UINT_PTR>(popup),label);
    };
    group(L"File",{{New,L"Example\tCtrl+N"},{NewDemo,L"New demo"},{Open,L"Open...\tCtrl+O"},{Save,L"Save\tCtrl+S"},{NewProjectFolder,L"New project folder..."},{CloneProjectFolder,L"Clone project folder..."}});
    group(L"Edit",{{Undo,L"Undo\tCtrl+Z"},{Redo,L"Redo\tCtrl+Y"}});
    group(L"View",{{FrameSelection,L"Frame selection"},{FitScene,L"Fit scene"},{RefreshInspector,L"Refresh values"},{ContentBrowser,L"Content Browser..."}});
    group(L"Run",{{Play,L"Play\tF5"},{Stop,L"Stop\tShift+F5"},{SendEvent,L"Send hit"}});
    group(L"Help",{{10000,L"About"}}); group(L"Terminal",{});
    EnableMenuItem(menu,5,MF_BYPOSITION|MF_GRAYED);
    if(!SetMenu(hwnd_,menu)){DestroyMenu(menu);throw std::runtime_error("Cannot attach Editor menu");}
    Make(Brand,L"STATIC",L"",SS_NOPREFIX); ShowWindow(ControlHandle(Brand),SW_HIDE);
    for (const auto &[id, label] : std::array<std::pair<int, const wchar_t *>, 8>{{
        {New,L"Example"}, {Open,L"Open..."}, {Save,L"Save"}, {Undo,L"Undo"}, {Redo,L"Redo"},
        {Play,L"Play (F5)"}, {Stop,L"Stop"}, {SendEvent,L"Send hit"}}}) Make(id,L"BUTTON",label,WS_TABSTOP);
    Make(EventLabel,L"STATIC",L"Hit / wait ticks"); Make(Payload,L"EDIT",L"10",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    Make(NewDemo,L"BUTTON",L"New demo",WS_TABSTOP);
    Make(ProjectLabel,L"STATIC",L""); ShowWindow(ControlHandle(ProjectLabel),SW_HIDE); Make(HistoryLabel,L"STATIC",L"Edit history"); Make(PreviewLabel,L"STATIC",L"Play preview");
    Make(Context,L"STATIC",L"Open a project or create an example.",SS_NOPREFIX);
    Make(DocumentsLabel,L"STATIC",L"Documents / hierarchy");
    Make(Tree,WC_TREEVIEWW,L"",WS_TABSTOP | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS);
    Make(PropertiesLabel,L"STATIC",L"Inspector");
    Make(Fields,L"LISTBOX",L"",WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);
    Make(InspectorTabs,WC_TABCONTROLW,L"",WS_TABSTOP);
    for(auto *label:{L"Properties",L"Actions"}) { TCITEMW item{}; item.mask=TCIF_TEXT; item.pszText=const_cast<wchar_t *>(label); TabCtrl_InsertItem(ControlHandle(InspectorTabs),TabCtrl_GetItemCount(ControlHandle(InspectorTabs)),&item); }
    Make(WorkspaceHint,L"STATIC",L"",SS_NOPREFIX);
    Make(LogLabel,L"STATIC",L"",SS_NOPREFIX); ShowWindow(ControlHandle(LogLabel),SW_HIDE);
    Make(ValueLabel,L"STATIC",L"Value");
    Make(Value,L"EDIT",L"",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    Make(Apply,L"BUTTON",L"Apply",WS_TABSTOP); Make(Revert,L"BUTTON",L"Revert",WS_TABSTOP);
    Make(Hint,L"EDIT",L"Choose a property. Apply publishes authoring; Revert discards the draft.",WS_TABSTOP | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY);
    Make(NewScene,L"BUTTON",L"+ Scene",WS_TABSTOP); Make(NewPrefab,L"BUTTON",L"+ Prefab",WS_TABSTOP); Make(NewGraph,L"BUTTON",L"+ Graph",WS_TABSTOP);
    Make(Add,L"BUTTON",L"Add item",WS_TABSTOP); Make(Remove,L"BUTTON",L"Delete item",WS_TABSTOP);
    Make(ToolsLabel,L"STATIC",L"Document actions"); Make(NodeLabel,L"STATIC",L"Graph node template");
    Make(Palette,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    for (const auto *name : {"Event entry", "Native damage", "Wait"}) Item(ControlHandle(Palette),name,CB_ADDSTRING);
    SendMessageW(ControlHandle(Palette),CB_SETCURSEL,0,0);
    Make(FromLabel,L"STATIC",L"Output port");
    Make(Source,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(ToLabel,L"STATIC",L"Input port");
    Make(Destination,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(Connect,L"BUTTON",L"Connect ports",WS_TABSTOP);
    Make(ComponentLabel,L"STATIC",L"Registered component template");
    Make(ComponentTypes,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    for (auto recipe:ex::ComponentRecipes(schema_)) if (const auto *info=schema_.components.Find(recipe.type)) {
        Item(ControlHandle(ComponentTypes),info->name+" ["+LabelId(recipe.type.Raw())+"]",CB_ADDSTRING); component_recipes_.push_back(std::move(recipe));
    }
    SendMessageW(ControlHandle(ComponentTypes),CB_SETCURSEL,0,0);
    Make(AddComponent,L"BUTTON",L"Add component",WS_TABSTOP); Make(ReplaceComponent,L"BUTTON",L"Replace component",WS_TABSTOP);
    Make(BooleanValue,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST);
    for (const auto *value:{"false","true"}) Item(ControlHandle(BooleanValue),value,CB_ADDSTRING);
    ShowWindow(ControlHandle(BooleanValue),SW_HIDE);
    Make(OpenReference,L"BUTTON",L"Open reference",WS_TABSTOP); Make(RefreshInspector,L"BUTTON",L"Refresh values",WS_TABSTOP);
    Make(OverrideMode,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST);
    for (const auto *mode:{"View Definition","Edit Instance Override"}) Item(ControlHandle(OverrideMode),mode,CB_ADDSTRING);
    SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,0,0);
    Make(ResetOverride,L"BUTTON",L"Reset selected override",WS_TABSTOP);
    Make(ExposeParameter,L"BUTTON",L"Expose parameter",WS_TABSTOP); Make(DetachBinding,L"BUTTON",L"Detach binding",WS_TABSTOP);
    Make(BehaviorLabel,L"STATIC",L"Graph asset for hit behavior");
    Make(BehaviorGraph,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(AttachBehavior,L"BUTTON",L"Assign hit GraphBehavior",WS_TABSTOP);
    Make(PrefabLabel,L"STATIC",L"Prefab asset for Scene placement");
    Make(Prefabs,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(Place,L"BUTTON",L"Assign Prefab",WS_TABSTOP);
    Make(FollowPrefab,L"BUTTON",L"Edit Prefab Definition",WS_TABSTOP);
    Make(ParentLabel,L"STATIC",L"Spatial parent (keep local)");
    Make(SceneParents,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(Reparent,L"BUTTON",L"Set parent (keep local)",WS_TABSTOP);
    Make(Viewport,L"STATIC",L"Scene View",WS_TABSTOP | WS_CLIPSIBLINGS | SS_NOTIFY);
    Make(ViewLabel,L"STATIC",L"Scene View / XY authoring");
    Make(FrameSelection,L"BUTTON",L"Frame selection",WS_TABSTOP); Make(FitScene,L"BUTTON",L"Fit scene",WS_TABSTOP);
    Make(ToolMode,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST);
    for (const auto *name:{"Move","Rotate Z","Scale XY"}) Item(ControlHandle(ToolMode),name,CB_ADDSTRING);
    Make(CoordinateSpace,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST);
    for (const auto *name:{"World XY","Local XY"}) Item(ControlHandle(CoordinateSpace),name,CB_ADDSTRING);
    for (int id:{ToolMode,CoordinateSpace}) SendMessageW(ControlHandle(id),CB_SETCURSEL,0,0);
    Make(SnapLabel,L"STATIC",L"Move snap in Scene units (0 = off)");
    Make(SnapValue,L"EDIT",L"0",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL); Make(SetSnap,L"BUTTON",L"Set snap",WS_TABSTOP);
    Make(ToolsHint,L"STATIC",L"",SS_NOPREFIX);
    Make(Status,L"STATIC",L"");
    Make(Log,L"EDIT",L"",WS_TABSTOP | ES_READONLY | ES_AUTOHSCROLL);
}
void Desktop::Fonts(UINT dpi) {
    NONCLIENTMETRICSW metrics{}; metrics.cbSize=sizeof(metrics);
    if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0,dpi)) throw std::runtime_error("Cannot query UI font");
    metrics.lfMessageFont.lfHeight=-MulDiv(11,static_cast<int>(dpi),72);
    const auto normal=CreateFontIndirectW(&metrics.lfMessageFont);
    metrics.lfMessageFont.lfWeight=FW_SEMIBOLD;
    metrics.lfMessageFont.lfHeight=-MulDiv(12,static_cast<int>(dpi),72);
    const auto heading=CreateFontIndirectW(&metrics.lfMessageFont);
    metrics.lfMessageFont.lfWeight=FW_NORMAL; metrics.lfMessageFont.lfHeight=-MulDiv(9,static_cast<int>(dpi),72);
    const auto meta=CreateFontIndirectW(&metrics.lfMessageFont);
    if (!normal || !heading || !meta) { if (normal) DeleteObject(normal); if (heading) DeleteObject(heading); if(meta)DeleteObject(meta); throw std::runtime_error("Cannot create UI font"); }
    for (int id=New;id<=LogLabel;++id) {
        const bool title=id==Brand || id==DocumentsLabel || id==PropertiesLabel || id==ToolsLabel || id==ViewLabel || id==LogLabel;
        const bool metadata=id==Hint || id==ToolsHint || id==WorkspaceHint || id==ProjectLabel || id==HistoryLabel || id==PreviewLabel || id==EventLabel;
        SendMessageW(ControlHandle(id),WM_SETFONT,reinterpret_cast<WPARAM>(title?heading:metadata?meta:normal),TRUE);
    }
    if (font_) DeleteObject(font_);
    if (heading_font_) DeleteObject(heading_font_);
    if(meta_font_)DeleteObject(meta_font_);
    font_=normal; heading_font_=heading; meta_font_=meta; dpi_=dpi;
    style_.ApplyFrame(hwnd_);
    for(int id:{Palette,Source,Destination,ComponentTypes,BooleanValue,OverrideMode,BehaviorGraph,Prefabs,SceneParents,ToolMode,CoordinateSpace}) {
        SendMessageW(ControlHandle(id),CB_SETITEMHEIGHT,0,MulDiv(26,static_cast<int>(dpi),96));
        SendMessageW(ControlHandle(id),CB_SETITEMHEIGHT,static_cast<WPARAM>(-1),MulDiv(22,static_cast<int>(dpi),96));
    }
    TreeView_SetItemHeight(ControlHandle(Tree),MulDiv(25,static_cast<int>(dpi),96));
    TreeView_SetBkColor(ControlHandle(Tree),style_.input); TreeView_SetTextColor(ControlHandle(Tree),style_.text);
    SendMessageW(ControlHandle(Fields),LB_SETITEMHEIGHT,0,MulDiv(36,static_cast<int>(dpi),96));
    SendMessageW(ControlHandle(Fields),LB_SETHORIZONTALEXTENT,0,0);
}
void Desktop::Layout() {
    RECT client{}; GetClientRect(hwnd_,&client);
    const auto px=[&](int value){return MulDiv(value,static_cast<int>(dpi_),96);};
    const int width=MulDiv(client.right,96,static_cast<int>(dpi_)),height=MulDiv(client.bottom,96,static_cast<int>(dpi_));
    if(width<=0 || height<=0)return;
    const auto move=[&](int id,int x,int y,int w,int h){MoveWindow(ControlHandle(id),px(x),px(std::max(0,y-30)),px(std::max(1,w)),px(std::max(1,h)),TRUE);};
    const int left=std::clamp(width/5,196,248),right_width=std::clamp(width*28/100,300,356);
    const int center=left+22,right=width-right_width-12,middle=right-center-10,bottom=height-8;
    move(Brand,16,7,340,24); move(ProjectLabel,16,34,340,17); move(HistoryLabel,374,34,160,17);
    move(PreviewLabel,width-410,34,246,17); move(EventLabel,width-144,34,120,17);
    move(New,16,56,88,30); move(Open,110,56,70,30); move(Save,186,56,64,30); move(NewDemo,256,56,94,30);
    move(Undo,374,56,70,30); move(Redo,450,56,70,30);
    move(Play,width-410,56,82,30); move(Stop,width-322,56,68,30); move(SendEvent,width-248,56,96,30); move(Payload,width-144,56,72,30);
    move(Context,16,102,width-32,42);
    move(DocumentsLabel,12,154,left,24); move(Tree,12,184,left,bottom-264);
    move(Add,12,bottom-70,left/2-3,30); move(Remove,18+left/2,bottom-70,left/2-3,30);
    move(NewScene,12,bottom-34,left/3-4,30); move(NewPrefab,16+left/3,bottom-34,left/3-4,30); move(NewGraph,20+2*left/3,bottom-34,left/3-8,30);
    move(PropertiesLabel,right,154,right_width-110,24); move(RefreshInspector,right+right_width-106,152,106,28);
    move(InspectorTabs,right,184,right_width,28); move(OverrideMode,right,220,right_width,150);
    const int fields_top=scene_document_?254:184;
    move(Fields,right,fields_top,right_width,bottom-fields_top-188);
    move(ValueLabel,right,bottom-180,right_width,22); move(Value,right,bottom-154,right_width,30);
    move(BooleanValue,right,bottom-154,right_width,130);
    move(Apply,right,bottom-116,64,30); move(Revert,right+70,bottom-116,64,30); move(OpenReference,right+140,bottom-116,right_width-140,30);
    move(Hint,right,bottom-78,right_width,74);
    move(ViewLabel,center,154,middle,24); move(ToolsLabel,center,154,middle,24);
    move(ToolMode,center,184,110,160); move(CoordinateSpace,center+118,184,110,130);
    move(FrameSelection,center+236,184,110,30); move(FitScene,center+354,184,std::min(78,middle-354),30);
    move(SnapLabel,center,224,82,22); move(SnapValue,center+88,220,70,30); move(SetSnap,center+166,220,90,30);
    move(Viewport,center,262,middle,bottom-292); move(WorkspaceHint,center,bottom-26,middle,24);
    // Graph and Prefab use the center as a document workspace; Scene actions
    // occupy the Inspector's Actions tab. Every operation retains its control ID.
    const int action_x=scene_document_?right:center,action_w=scene_document_?right_width:middle;
    move(NodeLabel,center,192,middle,22); move(Palette,center,220,middle,240);
    move(FromLabel,center,266,middle,22); move(Source,center,294,middle,280);
    move(ToLabel,center,340,middle,22); move(Destination,center,368,middle,280); move(Connect,center,414,170,32);
    move(ComponentLabel,center,192,middle,22); move(ComponentTypes,center,220,middle,240);
    move(AddComponent,center,264,middle/2-4,32); move(ReplaceComponent,center+middle/2+4,264,middle/2-4,32);
    move(BehaviorLabel,center,320,middle,22); move(BehaviorGraph,center,348,middle,240); move(AttachBehavior,center,392,middle,32);
    move(ExposeParameter,center,442,middle/2-4,32); move(DetachBinding,center+middle/2+4,442,middle/2-4,32);
    move(PrefabLabel,action_x,226,action_w,22); move(Prefabs,action_x,254,action_w,240);
    move(Place,action_x,296,action_w,32); move(FollowPrefab,action_x,334,action_w,32);
    move(ParentLabel,action_x,390,action_w,22); move(SceneParents,action_x,418,action_w,240); move(Reparent,action_x,460,action_w,32);
    move(ResetOverride,action_x,510,action_w,32);
    move(ToolsHint,action_x,scene_document_?552:492,action_w,std::max(32,bottom-(scene_document_?552:492)-4));
    for(int id:{Palette,Source,Destination,BehaviorGraph,Prefabs,SceneParents,ComponentTypes,OverrideMode})
        SendMessageW(ControlHandle(id),CB_SETDROPPEDWIDTH,static_cast<WPARAM>(px(std::max(action_w,650))),0);
    move(Status,16,height,340,22); move(LogLabel,16,height,1,1); move(Log,360,height,width-372,22);
    InvalidateRect(hwnd_,nullptr,FALSE);
}

void Desktop::PaintChrome(HDC dc) {
    RECT client{}; GetClientRect(hwnd_,&client); FillRect(dc,&client,style_.Background());
    const auto px=[&](int v){return MulDiv(v,static_cast<int>(dpi_),96);};
    const int width=MulDiv(client.right,96,static_cast<int>(dpi_)),height=MulDiv(client.bottom,96,static_cast<int>(dpi_));
    if(width<=0 || height<=0)return;
    const int left=std::clamp(width/5,196,248),right_width=std::clamp(width*28/100,300,356);
    const int center=left+22,right=width-right_width-12,bottom=height-38;
    const auto panel=[&](int x,int y,int w,int h){RECT r{px(x),px(y),px(x+w),px(y+h)}; FillRect(dc,&r,style_.Panel());};
    panel(0,0,width,64); panel(0,68,width,48); panel(8,120,left+8,bottom-116);
    panel(center-4,120,right-center+4,bottom-116); panel(right-4,120,right_width+8,bottom-116);
    panel(8,height-34,width-16,30);
    const auto pen=CreatePen(PS_SOLID,1,style_.border); const auto previous=SelectObject(dc,pen);
    for(const auto y:{64,116,height-38}){MoveToEx(dc,0,px(y),nullptr);LineTo(dc,client.right,px(y));}
    SelectObject(dc,previous); DeleteObject(pen);
}
void Desktop::PaintPicker(HWND control,HDC dc) {
    RECT bounds{};GetClientRect(control,&bounds);FillRect(dc,&bounds,style_.Input());
    SetBkMode(dc,TRANSPARENT);const auto previous=SelectObject(dc,font_);
    const auto frame=[&](RECT rect,COLORREF color){const auto brush=CreateSolidBrush(color);FrameRect(dc,&rect,brush);DeleteObject(brush);};
    const int pad=MulDiv(8,static_cast<int>(dpi_),96);
    if(control==ControlHandle(InspectorTabs)) {
        FillRect(dc,&bounds,style_.Panel());
        for(int index=0;index<TabCtrl_GetItemCount(control);++index) {
            RECT rect{};TabCtrl_GetItemRect(control,index,&rect);
            const bool selected=TabCtrl_GetCurSel(control)==index;
            const auto brush=CreateSolidBrush(selected?style_.selection:style_.panel);FillRect(dc,&rect,brush);DeleteObject(brush);
            frame(rect,selected?style_.accent:style_.border);
            std::array<wchar_t,64> text{};TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=text.data();item.cchTextMax=static_cast<int>(text.size());TabCtrl_GetItem(control,index,&item);
            SetTextColor(dc,selected && style_.HighContrast()?GetSysColor(COLOR_HIGHLIGHTTEXT):style_.text);
            DrawTextW(dc,text.data(),-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            if(GetFocus()==control && selected){InflateRect(&rect,-3,-3);DrawFocusRect(dc,&rect);}
        }
    } else {
        const bool enabled=IsWindowEnabled(control)!=FALSE;
        frame(bounds,GetFocus()==control?style_.accent:style_.border);
        RECT text=bounds;text.left+=pad;text.right-=pad*3;
        SetTextColor(dc,enabled?style_.text:style_.muted);
        const auto label=Wide(Text(control));DrawTextW(dc,label.c_str(),static_cast<int>(label.size()),&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
        const int x=bounds.right-pad*2,y=(bounds.bottom-bounds.top)/2;
        const auto pen=CreatePen(PS_SOLID,1,enabled?style_.text:style_.muted);const auto old=SelectObject(dc,pen);
        MoveToEx(dc,x-3,y-2,nullptr);LineTo(dc,x,y+1);LineTo(dc,x+4,y-3);SelectObject(dc,old);DeleteObject(pen);
        if(GetFocus()==control){InflateRect(&text,-2,-2);DrawFocusRect(dc,&text);}
    }
    SelectObject(dc,previous);
}

bool Desktop::DrawControl(const DRAWITEMSTRUCT &item) {
    if(item.CtlType==ODT_COMBOBOX) {
        const bool selected=(item.itemState&ODS_SELECTED)!=0;
        const auto brush=CreateSolidBrush(selected?style_.selection:style_.input);FillRect(item.hDC,&item.rcItem,brush);DeleteObject(brush);
        if(item.itemID==static_cast<UINT>(-1))return true;
        const auto length=SendMessageW(item.hwndItem,CB_GETLBTEXTLEN,item.itemID,0);if(length<0)return true;
        std::wstring label(static_cast<std::size_t>(length)+1,L'\0');SendMessageW(item.hwndItem,CB_GETLBTEXT,item.itemID,reinterpret_cast<LPARAM>(label.data()));
        auto rect=item.rcItem;rect.left+=MulDiv(8,static_cast<int>(dpi_),96);rect.right-=MulDiv(8,static_cast<int>(dpi_),96);
        SetBkMode(item.hDC,TRANSPARENT);SetTextColor(item.hDC,selected && style_.HighContrast()?GetSysColor(COLOR_HIGHLIGHTTEXT):style_.text);
        const auto old=SelectObject(item.hDC,font_);DrawTextW(item.hDC,label.data(),static_cast<int>(length),&rect,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
        if((item.itemState&ODS_FOCUS)!=0)DrawFocusRect(item.hDC,&rect);SelectObject(item.hDC,old);return true;
    }
    if(item.CtlType==ODT_BUTTON) {
        const bool disabled=(item.itemState&ODS_DISABLED)!=0,pressed=(item.itemState&ODS_SELECTED)!=0;
        const bool primary=item.CtlID==Play || item.CtlID==Apply;
        const COLORREF fill=primary && !disabled?style_.accent:pressed?style_.selection:style_.input;
        const auto brush=CreateSolidBrush(fill); FillRect(item.hDC,&item.rcItem,brush); DeleteObject(brush);
        const auto border=CreateSolidBrush(primary && !disabled?style_.accent:style_.border); FrameRect(item.hDC,&item.rcItem,border); DeleteObject(border);
        SetBkMode(item.hDC,TRANSPARENT); SetTextColor(item.hDC,disabled?GetSysColor(COLOR_GRAYTEXT):primary?GetSysColor(COLOR_HIGHLIGHTTEXT):style_.text);
        const auto previous=SelectObject(item.hDC,font_); auto rect=item.rcItem;
        if(pressed)OffsetRect(&rect,1,1);
        const auto text=Wide(Text(item.hwndItem)); DrawTextW(item.hDC,text.c_str(),static_cast<int>(text.size()),&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
        if((item.itemState&ODS_FOCUS)!=0){InflateRect(&rect,-4,-4);DrawFocusRect(item.hDC,&rect);}
        SelectObject(item.hDC,previous); return true;
    }
    if(item.CtlType!=ODT_LISTBOX || item.CtlID!=Fields)return false;
    const bool selected=(item.itemState&ODS_SELECTED)!=0;
    const auto brush=CreateSolidBrush(selected?style_.selection:style_.input); FillRect(item.hDC,&item.rcItem,brush); DeleteObject(brush);
    if(item.itemID>=fields_.size())return true;
    const auto &field=fields_[item.itemID]; const auto slash=field.name.rfind(" / ");
    const auto label=Wide(slash==std::string::npos?field.name:field.name.substr(slash+3));
    std::string detail=field.detail.empty()?(field.command?"Editable":"Read only"):field.detail;
    if(const auto end=detail.find(" / ",detail.find(" / ")==std::string::npos?0:detail.find(" / ")+3);end!=std::string::npos)detail.resize(end);
    if(!field.command && !field.detail.empty())detail="Read only | "+detail;
    if(slash!=std::string::npos) {
        auto owner=field.name.substr(0,slash); const auto start=owner.rfind(" / "); if(start!=std::string::npos)owner=owner.substr(start+3);
        const auto dot=owner.rfind('.'); if(dot!=std::string::npos)owner=owner.substr(dot+1);
        detail=owner+" | "+detail;
    }
    SetBkMode(item.hDC,TRANSPARENT);
    const int pad=MulDiv(8,static_cast<int>(dpi_),96),line=MulDiv(20,static_cast<int>(dpi_),96);
    RECT name=item.rcItem; name.left+=pad;name.right=item.rcItem.left+(item.rcItem.right-item.rcItem.left)*58/100;name.bottom=name.top+line;
    const auto previous=SelectObject(item.hDC,font_);
    SetTextColor(item.hDC,selected && style_.HighContrast()?GetSysColor(COLOR_HIGHLIGHTTEXT):style_.text);
    DrawTextW(item.hDC,label.c_str(),static_cast<int>(label.size()),&name,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    RECT value=name;value.left=name.right+pad;value.right=item.rcItem.right-pad;
    const auto text=Wide(field.value); DrawTextW(item.hDC,text.c_str(),static_cast<int>(text.size()),&value,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    RECT caption=item.rcItem;caption.left+=pad;caption.right-=pad;caption.top+=line;
    SelectObject(item.hDC,meta_font_);SetTextColor(item.hDC,selected && style_.HighContrast()?GetSysColor(COLOR_HIGHLIGHTTEXT):style_.muted);
    const auto meta=Wide(detail); DrawTextW(item.hDC,meta.c_str(),static_cast<int>(meta.size()),&caption,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);
    if((item.itemState&ODS_FOCUS)!=0){auto focus=item.rcItem;InflateRect(&focus,-1,-1);DrawFocusRect(item.hDC,&focus);}
    SelectObject(item.hDC,previous); return true;
}

HTREEITEM Desktop::TreeItem(HTREEITEM parent, std::string_view label, const ed::UiSelection &selection) {
    auto text = Wide(label); selections_.push_back(selection);
    TVINSERTSTRUCTW item{}; item.hParent=parent; item.hInsertAfter=TVI_LAST;
    item.item.mask=TVIF_TEXT | TVIF_PARAM; item.item.pszText=text.data(); item.item.lParam=static_cast<LPARAM>(selections_.size()-1);
    const auto result = TreeView_InsertItem(ControlHandle(Tree),&item);
    tree_items_.push_back(result);
    return result;
}
void Desktop::PrefabTree(HTREEITEM parent, AssetId doc, const comp::PrefabNode &node) {
    const auto *document=workspace_.Project()->Snapshot()->Find(doc);
    const auto label=document && std::get<comp::PrefabDefinition>(document->data).root.id==node.id?document->name:std::string{"Entity"};
    const auto item = TreeItem(parent,label+" ["+LabelId(node.id.Raw())+"]",{doc,ed::UiElement::PrefabNode,node.id.Raw()});
    for (const auto &component:node.components) {
        const auto *info=schema_.components.Find(component.type);
        TreeItem(item,(info?info->name:"Component")+(component.part.IsValid()?" / part "+LabelId(component.part.Raw()):""),
            {doc,ed::UiElement::PrefabComponent,node.id.Raw(),component.type,component.part});
    }
    for (const auto &child : node.children) PrefabTree(item,doc,child);
}
void Desktop::SceneTree(HTREEITEM parent,const ed::ProjectSnapshot &snapshot,const ed::EditorDocument &doc) {
    const auto &scene=std::get<gc::SceneCompositionDefinition>(doc.data);
    std::unordered_map<SceneNodeId,std::vector<const SceneNodeSnapshot *>> children;
    std::unordered_map<SceneNodeId,AssetId> placements;
    for (const auto &node:scene.scene.nodes) children[node.parent].push_back(&node);
    for (const auto &placement:scene.placements) placements.emplace(placement.node,placement.prefab);
    struct Pending { HTREEITEM parent=nullptr; const SceneNodeSnapshot *node=nullptr; };
    std::vector<Pending> pending;
    const auto append=[&](HTREEITEM item,SceneNodeId id) { const auto found=children.find(id); if (found!=children.end()) for (auto i=found->second.rbegin();i!=found->second.rend();++i) pending.push_back({item,*i}); };
    append(parent,{});
    while (!pending.empty()) {
        const auto current=pending.back(); pending.pop_back();
        std::string name="Scene node";
        if (const auto placement=placements.find(current.node->id);placement!=placements.end()) if (const auto *prefab=snapshot.Find(placement->second)) name=prefab->name;
        const auto item=TreeItem(current.parent,name+" ["+LabelId(current.node->id.Raw())+"]",{doc.id,ed::UiElement::SceneNode,current.node->id.Raw()});
        append(item,current.node->id);
    }
}
void Desktop::Refresh(bool new_project) {
    if (new_project) { edit_overrides_=false; inspector_actions_=false; TabCtrl_SetCurSel(ControlHandle(InspectorTabs),0); SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,0,0); }
    std::set<decltype(SelectionKey(ed::UiSelection{}))> expanded;
    // Common-controls can return additional state bits despite the requested mask.
    for (std::size_t i=0;i<tree_items_.size();++i) if ((TreeView_GetItemState(ControlHandle(Tree),tree_items_[i],TVIS_EXPANDED)&static_cast<UINT>(TVIS_EXPANDED))!=0) expanded.insert(SelectionKey(selections_[i]));
    const bool initial=new_project || tree_items_.empty();
    struct RefreshGuard {
        bool &active; HWND tree;
        RefreshGuard(bool &flag,HWND handle):active(flag),tree(handle) {}
        ~RefreshGuard() {
            if (!active) return;
            active=false; SendMessageW(tree,WM_SETREDRAW,TRUE,0); InvalidateRect(tree,nullptr,TRUE);
        }
    } guard{refreshing_,ControlHandle(Tree)};
    refreshing_=true; field_=-1; fields_.clear(); selections_.clear(); tree_items_.clear(); outputs_.clear(); inputs_.clear(); prefabs_.clear(); graphs_.clear();
    SendMessageW(ControlHandle(Tree),WM_SETREDRAW,FALSE,0);
    TreeView_DeleteAllItems(ControlHandle(Tree));
    for (int id : {Source,Destination,Prefabs,BehaviorGraph}) SendMessageW(ControlHandle(id),CB_RESETCONTENT,0,0);
    if (const auto *project=workspace_.Project()) {
        view_revision_=project->Revision(); const auto snapshot=project->Snapshot();
        for (const auto &doc : snapshot->definition.documents) {
            const std::array<const char *,3> kinds{"Scene","Prefab","Graph"};
            const auto root=TreeItem(TVI_ROOT,std::string{kinds[static_cast<std::size_t>(doc.Kind())]}+" / "+doc.name+(project->IsDirty(doc.id)?" *":""),{doc.id});
            if (const auto *scene=std::get_if<gc::SceneCompositionDefinition>(&doc.data)) {
                (void)scene; SceneTree(root,*snapshot,doc);
            } else if (const auto *prefab=std::get_if<comp::PrefabDefinition>(&doc.data)) {
                PrefabTree(root,doc.id,prefab->root); prefabs_.push_back(doc.id); Item(ControlHandle(Prefabs),doc.name,CB_ADDSTRING);
            } else if (const auto *graph=std::get_if<graph::GraphDefinition>(&doc.data)) {
                graphs_.push_back(doc.id); Item(ControlHandle(BehaviorGraph),doc.name,CB_ADDSTRING);
                std::unordered_map<graph::ElementId,std::string> port_labels;
                for (const auto &node:graph->nodes) {
                    TreeItem(root,FormatNodeLabel(node),{doc.id,ed::UiElement::GraphNode,node.id.Raw()});
                    for (const auto &port:node.ports) port_labels.emplace(port.id,FormatPortLabel(node,port));
                }
                for (const auto &edge:graph->connections) TreeItem(root,"Link ["+LabelId(edge.id.Raw())+"]: "+port_labels.at(edge.output_port)+" -> "+port_labels.at(edge.input_port),{doc.id,ed::UiElement::Connection,edge.id.Raw()});
            }
        }
        SetText(hwnd_,"Epidemic Editor / "+(workspace_.Folder()?workspace_.Folder()->Title():Utf8(project->File().filename().wstring()))+(workspace_.IsDirty()?" *":""));
    } else { SetText(hwnd_,"Epidemic Editor"); view_revision_=0; }
    // Restoring an expanded descendant can open its ancestors. Restore parents
    // last, including explicit collapsed states, before revealing the selection.
    for (std::size_t i=tree_items_.size();i>0;--i) TreeView_Expand(ControlHandle(Tree),tree_items_[i-1],
        initial || expanded.contains(SelectionKey(selections_[i-1]))?TVE_EXPAND:TVE_COLLAPSE);
    for (std::size_t i=0;i<tree_items_.size();++i) if (selections_[i]==workspace_.Selection()) { TreeView_SelectItem(ControlHandle(Tree),tree_items_[i]); TreeView_EnsureVisible(ControlHandle(Tree),tree_items_[i]); break; }
    SendMessageW(ControlHandle(Tree),WM_SETREDRAW,TRUE,0); InvalidateRect(ControlHandle(Tree),nullptr,TRUE);
    SendMessageW(ControlHandle(Prefabs),CB_SETCURSEL,0,0);
    SendMessageW(ControlHandle(BehaviorGraph),CB_SETCURSEL,0,0);
    Inspector(); SyncScene(new_project); refreshing_=false; Diagnostics();
}
void Desktop::SyncScene(bool reset) {
    const auto *project=workspace_.Project(); const auto snapshot=project?project->Snapshot():nullptr;
    const auto selection=workspace_.Selection(); const auto *doc=snapshot?snapshot->Find(selection.document):nullptr;
    const bool scene=doc && std::holds_alternative<gc::SceneCompositionDefinition>(doc->data);
    if (scene_document_!=scene) { scene_document_=scene; Layout(); }
    for (int id:{Viewport,ViewLabel,FrameSelection,FitScene,ToolMode,CoordinateSpace,SnapLabel,SnapValue,SetSnap}) ShowWindow(ControlHandle(id),scene?SW_SHOW:SW_HIDE);
    ShowWindow(ControlHandle(Hint),scene?SW_HIDE:SW_SHOW);
    scene_view_->Sync(snapshot,scene?selection.document:AssetId{},selection.kind==ed::UiElement::SceneNode?SceneNodeId{selection.element}:SceneNodeId{},dpi_,reset);
    EnableWindow(ControlHandle(FrameSelection),scene && selection.kind==ed::UiElement::SceneNode);
}
bool Desktop::PickScene(SceneNodeId node) {
    if (Draft()) { local_error_="Apply or Revert the edited field before changing selection."; Diagnostics(); return false; }
    const ed::UiSelection key{workspace_.Selection().document,node.IsValid()?ed::UiElement::SceneNode:ed::UiElement::Document,node.Raw()};
    for (std::size_t i=0;i<selections_.size();++i) if (selections_[i]==key) {
        TreeView_SelectItem(ControlHandle(Tree),tree_items_[i]); TreeView_EnsureVisible(ControlHandle(Tree),tree_items_[i]); return workspace_.Selection()==key;
    }
    return false;
}
void Desktop::Inspector() {
    fields_.clear(); field_=-1; SendMessageW(ControlHandle(Fields),LB_RESETCONTENT,0,0); SendMessageW(ControlHandle(Fields),LB_SETHORIZONTALEXTENT,0,0); SetText(ControlHandle(Value),"");
    parents_.clear(); SendMessageW(ControlHandle(SceneParents),CB_RESETCONTENT,0,0);
    ShowWindow(ControlHandle(BooleanValue),SW_HIDE); ShowWindow(ControlHandle(Value),SW_SHOW); EnableWindow(ControlHandle(OpenReference),FALSE);
    const auto *project=workspace_.Project();
    const auto snapshot=project?project->Snapshot():nullptr;
    const auto selection=workspace_.Selection(); const auto *doc=snapshot?snapshot->Find(selection.document):nullptr;
    if (!doc) { EnableWindow(ControlHandle(Value),FALSE); SetText(ControlHandle(ValueLabel),"Value"); SetText(ControlHandle(Hint),"Select a document or object."); return; }
    const auto asset=doc->id;
    AddField({"Document",doc->name,[asset](std::string_view text) { return Result<ed::Command>::Success(ed::RenameDocument{asset,std::string{text}}); }});
    AddField({"Asset ID",Number(asset.value.Raw()),{}});
    if (const auto *prefab=std::get_if<comp::PrefabDefinition>(&doc->data)) {
        if (const auto *node=Find(prefab->root,selection.element)) {
            AddField({"Entity ID",Number(node->id.Raw()),{}});
            if (selection.kind==ed::UiElement::PrefabComponent) { AddField({"Component type ID",Number(selection.component.Raw()),{}}); AddField({"Part ID",Number(selection.part.Raw()),{}}); }
        }
    } else if (const auto *scene=std::get_if<gc::SceneCompositionDefinition>(&doc->data)) {
        for (const auto &node:scene->scene.nodes) if (node.id.Raw()==selection.element && selection.kind==ed::UiElement::SceneNode) {
            AddField({"Placement ID",Number(node.id.Raw()),{}});
            AddField({"Parent ID",Number(node.parent.Raw()),{}});
            const auto transform=node.local_transform;
            const auto add_transform=[&](std::string name,double value,ed::UiTransformField target) {
                AddField({std::move(name),ed::EditorWorkspace::FormatValue(value),[asset,id=node.id,transform,target](std::string_view text) {
                    return ed::EditorWorkspace::TransformCommand(asset,id,transform,target,text);
                }});
            };
            const std::array<float,3> position{transform.position.x,transform.position.y,transform.position.z}, scale{transform.scale.x,transform.scale.y,transform.scale.z};
            const std::array<ed::UiTransformField,3> positions{ed::UiTransformField::PositionX,ed::UiTransformField::PositionY,ed::UiTransformField::PositionZ}, scales{ed::UiTransformField::ScaleX,ed::UiTransformField::ScaleY,ed::UiTransformField::ScaleZ};
            for (std::size_t axis=0;axis<3;++axis) add_transform(std::string{"Local position "}+"XYZ"[axis],position[axis],positions[axis]);
            for (std::size_t axis=0;axis<3;++axis) add_transform(std::string{"Local scale "}+"XYZ"[axis],scale[axis],scales[axis]);
            if (transform.rotation.x==0 && transform.rotation.y==0) add_transform("Local rotation Z (degrees)",std::remainder(2*std::atan2(static_cast<double>(transform.rotation.z),static_cast<double>(transform.rotation.w))*180/std::numbers::pi,360.0),ed::UiTransformField::RotationZDegrees);
            else {
                const std::array<float,4> quaternion{transform.rotation.x,transform.rotation.y,transform.rotation.z,transform.rotation.w};
                for (std::size_t axis=0;axis<4;++axis) AddField({std::string{"Local quaternion "}+"XYZW"[axis],ed::EditorWorkspace::FormatValue(static_cast<double>(quaternion[axis])),{}});
            }
            parents_.push_back({}); Item(ControlHandle(SceneParents),"Scene root (0)",CB_ADDSTRING);
            std::unordered_map<SceneNodeId,AssetId> placements; for (const auto &placement:scene->placements) placements.emplace(placement.node,placement.prefab);
            for (const auto &parent:scene->scene.nodes) if (parent.id!=node.id) {
                std::string name="Scene node";
                if (const auto placed=placements.find(parent.id);placed!=placements.end()) if (const auto *target=snapshot->Find(placed->second)) name=target->name;
                parents_.push_back(parent.id); Item(ControlHandle(SceneParents),name+" ["+LabelId(parent.id.Raw())+"]",CB_ADDSTRING);
            }
            const auto parent=std::find(parents_.begin(),parents_.end(),node.parent);
            if (parent!=parents_.end()) SendMessageW(ControlHandle(SceneParents),CB_SETCURSEL,static_cast<WPARAM>(parent-parents_.begin()),0);
            for (const auto &placement:scene->placements) if (placement.node==node.id) { Field reference{"Prefab asset",Number(placement.prefab.value.Raw()),{}}; reference.reference=placement.prefab; AddField(std::move(reference)); }
        }
    } else if (const auto *definition=std::get_if<graph::GraphDefinition>(&doc->data)) {
        for (const auto &node:definition->nodes) for (const auto &port:node.ports) {
            const std::array<const char *,3> kinds{"data","control","event"};
            const auto name=FormatPortLabel(node,port)+" ["+kinds[static_cast<std::size_t>(port.kind)]+"]";
            if (port.direction==graph::PortDirection::Output) { outputs_.push_back(port.id); Item(ControlHandle(Source),name,CB_ADDSTRING); }
            else { inputs_.push_back(port.id); Item(ControlHandle(Destination),name,CB_ADDSTRING); }
            if (node.id.Raw()==selection.element && selection.kind==ed::UiElement::GraphNode) AddField({name,Number(port.id.Raw()),{}});
        }
        for (const auto &node:definition->nodes) if (node.id.Raw()==selection.element && selection.kind==ed::UiElement::GraphNode) AddField({"Node ID",Number(node.id.Raw()),{}});
        for (const auto &edge:definition->connections) if (edge.id.Raw()==selection.element && selection.kind==ed::UiElement::Connection) {
            AddField({"Output ID",Number(edge.output_port.Raw()),{}}); AddField({"Input ID",Number(edge.input_port.Raw()),{}});
            AddField({"Output",PortLabel(*definition,edge.output_port),{}}); AddField({"Input",PortLabel(*definition,edge.input_port),{}});
        }
        SendMessageW(ControlHandle(Source),CB_SETCURSEL,0,0); SendMessageW(ControlHandle(Destination),CB_SETCURSEL,0,0);
    }
    for (const auto &entry:ed::InspectComponents(*snapshot,selection,schema_.components,workspace_.Session(),edit_overrides_)) {
        const std::array<const char *,4> kinds{"i64","double","bool","string"};
        Field field{entry.name,ed::InspectorText(entry),{},entry.kind,entry.source,entry.reference};
        field.missing=entry.editable && !entry.value;
        if (entry.kind) field.detail=std::string{kinds[static_cast<std::size_t>(*entry.kind)]}+" / "+field.detail;
        if (!entry.readonly_reason.empty()) field.detail+=" / "+entry.readonly_reason;
        if (entry.binding.IsValid()) field.detail+=" / bound parameter ["+Number(entry.binding.Raw())+"]";
        if (entry.definition_value) {
            ed::InspectorEntry display; display.value=entry.definition_value; const auto text=ed::InspectorText(display);
            const bool valid=text.empty() || MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);
            field.detail+=" / definition default: "+(valid?text:"<invalid UTF-8>");
        } else if (entry.target==ed::InspectorTarget::ComponentOverride) field.detail+=" / definition default unavailable";
        ed::InspectorEntry key; key.owner=entry.owner; key.property=entry.property; key.kind=entry.kind; key.editable=entry.editable;
        key.target=entry.target; key.placement=entry.placement; key.binding=entry.binding; key.overridden=entry.overridden;
        if (entry.editable && entry.kind) field.command=[key](std::string_view text) { return ed::InspectorCommand(key,text); };
        if (auto reset=ed::InspectorResetCommand(key);reset) field.reset=std::move(reset).Value();
        if (entry.target==ed::InspectorTarget::Property && entry.owner.kind==ed::UiElement::PrefabComponent && entry.value) {
            field.bound=entry.binding.IsValid();
            field.binding=[key,value=*entry.value](comp::PropertyId parameter) mutable {
                key.value=value; return ed::InspectorBinding(key,parameter);
            };
        }
        if (!field.value.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,field.value.data(),static_cast<int>(field.value.size()),nullptr,0)) {
            field.value="<invalid UTF-8>"; field.command={}; field.binding={}; field.detail+=" / unsupported text encoding";
        }
        AddField(std::move(field));
    }
    if (!fields_.empty()) SelectField(0);
}
void Desktop::SelectField(int index) {
    field_=index; SendMessageW(ControlHandle(Fields),LB_SETCURSEL,static_cast<WPARAM>(index),0);
    const bool valid=index>=0 && static_cast<std::size_t>(index)<fields_.size();
    EnableWindow(ControlHandle(Value),valid && static_cast<bool>(fields_[static_cast<std::size_t>(index)].command));
    SetText(ControlHandle(Value),valid?fields_[static_cast<std::size_t>(index)].value:"");
    const auto *field=valid?&fields_[static_cast<std::size_t>(index)]:nullptr;
    const bool boolean=field && field->command && field->kind==comp::PropertyKind::Boolean;
    ShowWindow(ControlHandle(BooleanValue),boolean?SW_SHOW:SW_HIDE); ShowWindow(ControlHandle(Value),boolean?SW_HIDE:SW_SHOW);
    const WPARAM boolean_index=field && field->value=="true"?1:field && field->value=="false"?0:static_cast<WPARAM>(-1);
    SendMessageW(ControlHandle(BooleanValue),CB_SETCURSEL,boolean_index,0);
    EnableWindow(ControlHandle(OpenReference),field && field->reference.IsValid() && workspace_.Project()->Snapshot()->Find(field->reference));
    const std::array<const char *,4> kinds{"i64","double","bool","string"};
    SetText(ControlHandle(ValueLabel),field && field->kind?std::string{"Value ("}+kinds[static_cast<std::size_t>(*field->kind)]+(field->command?")":", read only)"):"Value");
    SetText(ControlHandle(Hint),field?field->name+"\r\n"+(field->detail.empty()?(field->command?"Editable authoring value. Apply / Revert the draft.":"Read-only identity or reference."):field->detail):"Choose a property. Apply publishes authoring; Revert discards the field draft.");
}
bool Desktop::ApplyDraft(bool explicit_apply) {
    const bool missing=field_>=0 && static_cast<std::size_t>(field_)<fields_.size() && fields_[static_cast<std::size_t>(field_)].missing;
    if (!Draft() && !(explicit_apply && missing)) return true;
    const auto &field=fields_[static_cast<std::size_t>(field_)]; if (!field.command) return false;
    auto command=field.command(Text(ControlHandle(Value)));
    const int input=field.kind==comp::PropertyKind::Boolean?BooleanValue:Value;
    if (!command) { local_error_=command.GetError().code+": "+command.GetError().message; Diagnostics(); SetFocus(ControlHandle(input)); return false; }
    const int selected=field_; if (!Execute({std::move(command).Value()})) { SetFocus(ControlHandle(input)); return false; }
    SelectField(selected); local_error_.clear(); Diagnostics(); return true;
}
bool Desktop::SaveProject() { if (!ApplyDraft()) return false; const auto saved=workspace_.Save(); if (saved) { local_error_.clear(); Refresh(); } else Diagnostics(); return static_cast<bool>(saved); }
Result<void> Desktop::CreateDemoCopy(const std::filesystem::path &file) {
    auto definition=ex::DemoDefinition(file,schema_.View());
    if (!definition) return Result<void>::Failure(definition.GetError());
    auto created=workspace_.New(file,std::move(definition).Value(),true); if (!created) return created;
    auto saved=workspace_.Save(); if (saved) local_error_.clear(); Refresh(true); return saved;
}
bool Desktop::CanReplace() {
    if (!Draft() && !workspace_.IsDirty()) return true;
    const auto answer=MessageBoxW(hwnd_,L"Save changes before leaving this project?",L"Unsaved authoring data",MB_YESNOCANCEL | MB_ICONQUESTION);
    return answer==IDNO || (answer==IDYES && SaveProject());
}
std::optional<std::filesystem::path> Desktop::Choose(bool save, bool folder) {
    std::array<wchar_t,32768> file{};
    OPENFILENAMEW dialog{}; dialog.lStructSize=sizeof(dialog); dialog.hwndOwner=hwnd_;
    dialog.lpstrFilter=folder?L"New project folder name\0*.epidemicproj\0\0":L"Epidemic project\0*.epidemic;*.epidemicproj\0Legacy authoring\0*.epidemic\0Project folder descriptor\0*.epidemicproj\0\0"; dialog.lpstrFile=file.data(); dialog.nMaxFile=static_cast<DWORD>(file.size());
    dialog.lpstrDefExt=folder?L"epidemicproj":L"epidemic"; dialog.Flags=OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    if (save?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog)) return std::filesystem::path{file.data()};
    if (CommDlgExtendedError()) { local_error_="Cannot open project file dialog"; Diagnostics(); } return std::nullopt;
}
void Desktop::NewDocument(ed::DocumentKind kind) {
    const AssetId id{Fresh()}; ed::DocumentData data;
    if (kind==ed::DocumentKind::Scene) { gc::SceneCompositionDefinition scene; scene.asset_id=id; data=scene; }
    else if (kind==ed::DocumentKind::Prefab) { auto root=ex::Enemy(schema_,Fresh()); root.behaviors.clear(); data=comp::PrefabDefinition{id.value,{},root}; }
    else data=graph::GraphDefinition{id,"Graph",{}, {}};
    const std::array<const char *,3> names{"Scene","Prefab","Graph"};
    (void)Execute({ed::CreateDocument{{id,names[static_cast<std::size_t>(kind)],0,std::move(data)}}},ed::UiSelection{id});
}
void Desktop::Action(int id) {
    scene_view_->Cancel();
    if (id==RefreshInspector) {
        if (Draft()) { local_error_="Apply or Revert the field before refreshing values."; Diagnostics(); return; }
        Inspector(); Diagnostics(); return;
    }
    if (id==OverrideMode) {
        if (Draft()) {
            // The mode change has not rebuilt the fields yet; retain the
            // previous mode until the pending field is explicitly resolved.
            SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,edit_overrides_?1:0,0);
            local_error_="Apply or Revert the field before changing edit mode."; Diagnostics(); return;
        }
        edit_overrides_=SendMessageW(ControlHandle(OverrideMode),CB_GETCURSEL,0,0)==1;
        Inspector(); Diagnostics(); return;
    }
    if (id==ResetOverride || id==ExposeParameter || id==DetachBinding) {
        if (Draft()) { local_error_="Apply or Revert the field before changing its override/binding."; Diagnostics(); return; }
        if (field_<0 || static_cast<std::size_t>(field_)>=fields_.size()) return;
        auto &field=fields_[static_cast<std::size_t>(field_)];
        if (id==ResetOverride) { if (field.reset) (void)Execute({*field.reset}); return; }
        if (!field.binding) return;
        auto commands=field.binding(id==ExposeParameter?Fresh():comp::PropertyId{});
        if (!commands) { local_error_=commands.GetError().message; Diagnostics(); return; }
        (void)Execute(std::move(commands).Value()); return;
    }
    if (id==OpenReference) {
        if (Draft()) { local_error_="Apply or Revert the field before opening a reference."; Diagnostics(); return; }
        if (field_<0 || static_cast<std::size_t>(field_)>=fields_.size()) return;
        const auto reference=fields_[static_cast<std::size_t>(field_)].reference;
        if (reference.IsValid() && workspace_.Select({reference})) Refresh(); return;
    }
    if (id==ToolMode || id==CoordinateSpace || id==SetSnap) {
        const auto index=SendMessageW(ControlHandle(ToolMode),CB_GETCURSEL,0,0);
        if (index<0 || index>2) return;
        const auto tool=static_cast<ed::GizmoTool>(index);
        if (tool!=ed::GizmoTool::Move) SendMessageW(ControlHandle(CoordinateSpace),CB_SETCURSEL,1,0);
        EnableWindow(ControlHandle(CoordinateSpace),tool==ed::GizmoTool::Move);
        const auto space=SendMessageW(ControlHandle(CoordinateSpace),CB_GETCURSEL,0,0)==0?ed::GizmoSpace::World:ed::GizmoSpace::Local;
        double step=scene_view_->SnapStep();
        if (id==SetSnap) {
            const auto parsed=ed::EditorWorkspace::ParseValue(comp::PropertyKind::Real,Text(ControlHandle(SnapValue)));
            if (!parsed || std::get<double>(parsed.Value())<0) { local_error_="Move snap must be a finite nonnegative number; 0 disables snap."; Diagnostics(); return; }
            step=std::get<double>(parsed.Value());
        }
        const auto configured=scene_view_->Configure(tool,space,step);
        local_error_=configured?"":configured.GetError().message; Diagnostics(); return;
    }
    if (id==FrameSelection || id==FitScene) { scene_view_->Frame(id==FitScene); return; }
    if (id==Revert) { SelectField(field_); local_error_.clear(); Diagnostics(); return; }
    if (id==Apply) { (void)ApplyDraft(true); return; }
    if (id==NewDemo) {
        if (!CanReplace()) return;
        const auto file=Choose(true); if (!file) return;
        const auto result=CreateDemoCopy(*file);
        if (!result) { local_error_=result.GetError().code+": "+result.GetError().message; Diagnostics(); }
        return;
    }
    if (id==ContentBrowser) { if(!ApplyDraft())return; ed::ShowContentBrowser(hwnd_,workspace_); Refresh(); return; }
    if (id==NewProjectFolder || id==CloneProjectFolder) {
        if (id==NewProjectFolder && !CanReplace()) return;
        const auto file=Choose(true,true); if(!file)return;
        const auto root=file->parent_path()/file->stem();
        auto definition=ex::Definition(schema_);definition.id=Fresh();
        const auto result=id==NewProjectFolder?
            workspace_.NewFolder(root,Utf8(file->stem().wstring()),std::move(definition),true):workspace_.CloneFolder(root);
        if(result){local_error_.clear();Refresh(true);}else Diagnostics();return;
    }
    if (id==New || id==Open) {
        if (!CanReplace()) return;
        const auto file=Choose(id==New); if (!file) return;
        const auto result=id==New?workspace_.New(*file,ex::Definition(schema_),true):workspace_.Open(*file,true);
        if (result) { local_error_.clear(); Refresh(true); } else Diagnostics(); return;
    }
    if (id==Save) { (void)SaveProject(); return; }
    if (id==Reparent) {
        if (!ApplyDraft()) return;
        const auto selection=workspace_.Selection(); const auto index=SendMessageW(ControlHandle(SceneParents),CB_GETCURSEL,0,0);
        if (selection.kind==ed::UiElement::SceneNode && index>=0 && static_cast<std::size_t>(index)<parents_.size())
            (void)Execute({ed::SetSceneParent{selection.document,SceneNodeId{selection.element},parents_[static_cast<std::size_t>(index)]}});
        return;
    }
    if (id==FollowPrefab) {
        if (!ApplyDraft()) return;
        if (const auto target=workspace_.PrefabDefinitionSelection();target && workspace_.Select(*target)) { local_error_.clear(); Refresh(); }
        else { local_error_="Select a Scene placement with an available Prefab definition."; Diagnostics(); }
        return;
    }
    if (id==Stop) { (void)workspace_.Stop(); if (!Draft()) Inspector(); Diagnostics(); return; }
    if (!ApplyDraft()) return;
    local_error_.clear();
    if (id==Undo) { (void)workspace_.Undo(); Refresh(); return; }
    if (id==Redo) { (void)workspace_.Redo(); Refresh(); return; }
    if (id==Play) { if (workspace_.Play()) Inspector(); Diagnostics(); return; }
    if (id==SendEvent) {
        const auto selection=workspace_.Selection();
        if (selection.kind!=ed::UiElement::SceneNode) { local_error_="Select a Scene placement before sending hit."; Diagnostics(); return; }
        const auto payload=ed::EditorWorkspace::ParseValue(comp::PropertyKind::Integer,Text(ControlHandle(Payload)));
        if (!payload) local_error_=payload.GetError().code;
        else (void)workspace_.Send(SceneNodeId{selection.element},{ex::Id("hit"),std::get<std::int64_t>(payload.Value())});
        Diagnostics(); return;
    }
    const auto *project=workspace_.Project(); if (!project) return;
    if (id==NewScene || id==NewPrefab || id==NewGraph) { NewDocument(static_cast<ed::DocumentKind>(id-NewScene)); return; }
    const auto selection=workspace_.Selection(); const auto snapshot=project->Snapshot(); const auto *doc=snapshot->Find(selection.document); if (!doc) return;
    if (id==Add) {
        if (doc->Kind()==ed::DocumentKind::Scene) { SceneNodeSnapshot node; node.id=SceneNodeId{Fresh().Raw()}; (void)Execute({ed::AddSceneNode{doc->id,node}},ed::UiSelection{doc->id,ed::UiElement::SceneNode,node.id.Raw()}); }
        else if (doc->Kind()==ed::DocumentKind::Prefab) {
            const auto &prefab=std::get<comp::PrefabDefinition>(doc->data);
            auto child=ex::Enemy(schema_,Fresh()); child.behaviors.clear();
            const ed::UiSelection created{doc->id,ed::UiElement::PrefabNode,child.id.Raw()};
            (void)Execute({ed::AddEntity{doc->id,selection.kind==ed::UiElement::PrefabNode?comp::PrefabNodeId{selection.element}:prefab.root.id,std::move(child)}},created);
        } else {
            const auto palette=SendMessageW(ControlHandle(Palette),CB_GETCURSEL,0,0);
            if (palette<0 || palette>2) return;
            auto node=ex::Graph().nodes[static_cast<std::size_t>(palette)]; node.id=Fresh();
            for (auto &port:node.ports) port.id=Fresh();
            const ed::UiSelection created{doc->id,ed::UiElement::GraphNode,node.id.Raw()};
            (void)Execute({ed::AddGraphNode{doc->id,std::move(node)}},created);
        }
    } else if (id==Remove) {
        switch (selection.kind) {
        case ed::UiElement::Document: (void)Execute({ed::DeleteDocument{doc->id}}); break;
        case ed::UiElement::SceneNode: (void)Execute({ed::DeleteSceneNode{doc->id,SceneNodeId{selection.element}}}); break;
        case ed::UiElement::PrefabNode: (void)Execute({ed::DeleteEntity{doc->id,comp::PrefabNodeId{selection.element}}}); break;
        case ed::UiElement::GraphNode: (void)Execute({ed::DeleteGraphNode{doc->id,graph::ElementId{selection.element}}}); break;
        case ed::UiElement::Connection: (void)Execute({ed::DisconnectPorts{doc->id,graph::ElementId{selection.element}}}); break;
        case ed::UiElement::PrefabComponent: (void)Execute({ed::DeleteComponent{doc->id,comp::PrefabNodeId{selection.element},selection.component,selection.part}}); break;
        }
    } else if (id==Connect && doc->Kind()==ed::DocumentKind::Graph) {
        const auto out=SendMessageW(ControlHandle(Source),CB_GETCURSEL,0,0), in=SendMessageW(ControlHandle(Destination),CB_GETCURSEL,0,0);
        if (out>=0 && in>=0 && static_cast<std::size_t>(out)<outputs_.size() && static_cast<std::size_t>(in)<inputs_.size()) (void)Execute({ed::ConnectPorts{doc->id,{Fresh(),outputs_[static_cast<std::size_t>(out)],inputs_[static_cast<std::size_t>(in)]}}});
    } else if (id==AddComponent || id==ReplaceComponent) {
        const auto index=SendMessageW(ControlHandle(ComponentTypes),CB_GETCURSEL,0,0);
        if (index<0 || static_cast<std::size_t>(index)>=component_recipes_.size()) return;
        const auto &recipe=component_recipes_[static_cast<std::size_t>(index)];
        auto commands=ed::ComponentCommands(*snapshot,selection,recipe,id==ReplaceComponent);
        if (!commands) { local_error_=commands.GetError().message; Diagnostics(); return; }
        const auto part=id==ReplaceComponent?selection.part:recipe.part;
        (void)Execute(std::move(commands).Value(),ed::UiSelection{doc->id,ed::UiElement::PrefabComponent,selection.element,recipe.type,part});
    } else if (id==AttachBehavior && (selection.kind==ed::UiElement::PrefabNode || selection.kind==ed::UiElement::PrefabComponent)) {
        const auto index=SendMessageW(ControlHandle(BehaviorGraph),CB_GETCURSEL,0,0);
        if (index<0 || static_cast<std::size_t>(index)>=graphs_.size()) return;
        const auto *graph_doc=snapshot->Find(graphs_[static_cast<std::size_t>(index)]);
        if (!graph_doc) return;
        const auto &graph=std::get<graph::GraphDefinition>(graph_doc->data);
        graph::ElementId event{};
        for (const auto &node:graph.nodes) for (const auto &port:node.ports) if (port.kind==graph::PortKind::Event && port.direction==graph::PortDirection::Input && !event.Raw()) event=port.id;
        if (!event.Raw()) { local_error_="Graph has no event input. Add an Event entry first."; Diagnostics(); return; }
        const auto *node=Find(std::get<comp::PrefabDefinition>(doc->data).root,selection.element); if (!node) return;
        const auto *behaviors=&node->behaviors;
        if (selection.kind==ed::UiElement::PrefabComponent) {
            const auto component=std::find_if(node->components.begin(),node->components.end(),[&](const auto &item){return item.type==selection.component && item.part==selection.part;});
            if (component==node->components.end()) return;
            behaviors=&component->behaviors;
        }
        auto behavior_id=Fresh();
        for (const auto &behavior:*behaviors) if (std::any_of(behavior.events.begin(),behavior.events.end(),[](const auto &route){return route.event==ex::Id("hit");})) { behavior_id=behavior.id; break; }
        const bool component=selection.kind==ed::UiElement::PrefabComponent;
        (void)Execute({ed::SetBehavior{doc->id,node->id,component?selection.component:gameplay::TypeId{},component?selection.part:gameplay::entities::EntityPartId{},{behavior_id,graph_doc->id.value,{{ex::Id("hit"),event}}, {}}}});
    } else if (id==Place && selection.kind==ed::UiElement::SceneNode) {
        const auto prefab=SendMessageW(ControlHandle(Prefabs),CB_GETCURSEL,0,0);
        if (prefab>=0 && static_cast<std::size_t>(prefab)<prefabs_.size()) {
            gc::ScenePrefabPlacement placement{SceneNodeId{selection.element},prefabs_[static_cast<std::size_t>(prefab)],{}};
            for (const auto &old:std::get<gc::SceneCompositionDefinition>(doc->data).placements) if (old.node==placement.node && old.prefab==placement.prefab) placement.overrides=old.overrides;
            (void)Execute({ed::PlacePrefab{doc->id,std::move(placement)}});
        }
    }
}
void Desktop::Diagnostics() {
    const auto *project=workspace_.Project(); const auto *session=workspace_.Session(); bool playing=session && session->IsPlaying();
    if (playing && !timer_running_) {
        if (SetTimer(hwnd_,1,33,nullptr)) timer_running_=true;
        else { (void)workspace_.Stop(); playing=false; local_error_="Cannot start Play timer; session stopped."; }
    } else if (!playing && timer_running_) { KillTimer(hwnd_,1); timer_running_=false; }
    SetText(ControlHandle(Status),std::string{playing?"PLAYING SNAPSHOT | ":"AUTHORING | "}+(project?(workspace_.IsDirty()?"Unsaved changes":"Saved"):"Open a project or create an example")+(Draft()?" | Unapplied field":""));
    EnableWindow(ControlHandle(Save),project!=nullptr); EnableWindow(ControlHandle(Undo),project && project->CanUndo()); EnableWindow(ControlHandle(Redo),project && project->CanRedo());
    EnableWindow(ControlHandle(Play),project && !playing); EnableWindow(ControlHandle(Stop),playing); EnableWindow(ControlHandle(SendEvent),playing);
    const auto selection=workspace_.Selection(); const auto snapshot=project?project->Snapshot():nullptr; const auto *doc=snapshot?snapshot->Find(selection.document):nullptr;
    const bool graph=doc && std::holds_alternative<graph::GraphDefinition>(doc->data), prefab=selection.kind==ed::UiElement::PrefabNode;
    const bool prefab_doc=doc && std::holds_alternative<comp::PrefabDefinition>(doc->data), scene_doc=doc && std::holds_alternative<gc::SceneCompositionDefinition>(doc->data);
    const auto visible=[&](int id,bool show) { const auto control=ControlHandle(id); if (((static_cast<DWORD_PTR>(GetWindowLongPtrW(control,GWL_STYLE))&static_cast<DWORD_PTR>(WS_VISIBLE))!=0)!=show) ShowWindow(control,show?SW_SHOW:SW_HIDE); };
    for (int id:{NodeLabel,Palette,FromLabel,Source,ToLabel,Destination,Connect}) visible(id,graph);
    for (int id:{AddComponent,ReplaceComponent,ComponentLabel,ComponentTypes,BehaviorLabel,BehaviorGraph,AttachBehavior,ExposeParameter,DetachBinding}) visible(id,prefab_doc);
    const bool actions=scene_doc && inspector_actions_;
    for (int id:{PrefabLabel,Prefabs,Place,FollowPrefab,ParentLabel,SceneParents,Reparent,ResetOverride}) visible(id,actions);
    visible(OverrideMode,scene_doc && !actions); visible(InspectorTabs,scene_doc);
    for(int id:{Fields,ValueLabel,Apply,Revert,OpenReference,Hint}) visible(id,!actions);
    const bool boolean=field_>=0 && static_cast<std::size_t>(field_)<fields_.size() && fields_[static_cast<std::size_t>(field_)].command && fields_[static_cast<std::size_t>(field_)].kind==comp::PropertyKind::Boolean;
    visible(Value,!actions && !boolean); visible(BooleanValue,!actions && boolean);
    visible(ToolsLabel,!scene_doc); visible(ToolsHint,!scene_doc || actions);
    SetText(ControlHandle(ToolsLabel),graph?"Graph actions":prefab_doc?"Prefab definition":scene_doc?"Scene placement":"Document actions");
    SetText(ControlHandle(Add),graph?"+ Node":prefab_doc?"+ Entity":"+ Object");
    SetText(ControlHandle(Remove),"Delete");
    SetText(ControlHandle(PropertiesLabel),actions?"Scene actions":"Inspector");
    SetText(ControlHandle(HistoryLabel),"HISTORY"); SetText(ControlHandle(PreviewLabel),playing?"PLAYING SNAPSHOT":"PLAY CONTROLS");
    SetText(ControlHandle(WorkspaceHint),scene_doc?"XY authoring  |  Pan: middle drag  |  Zoom: wheel":prefab_doc?"Ownership tree  |  Parameters and components share this definition":"Typed ports  |  Node and connection lists are in Hierarchy");
    const std::string tool_hint=!doc?"Open a project or create an example.":graph?"Select ports of compatible kind, direction and type. Select a node or link to delete it.":prefab_doc?"Edit Definition. Expose a property as a parameter; edit its default row or detach its binding. Tree nesting is ownership, not spatial parenting; no Prefab geometry preview is available.":"Reset removes only the selected override layer. Runtime rows remain read-only.";
    if (Text(ControlHandle(ToolsHint))!=tool_hint) SetText(ControlHandle(ToolsHint),tool_hint);
    std::string context="Open a project or create an example.";
    if (doc && project) {
        context=std::string{graph?"Graph":prefab_doc?"Prefab":"Scene"}+" / "+doc->name+(project->IsDirty(doc->id)?" *":"");
        for (std::size_t i=0;i<selections_.size();++i) if (selections_[i]==selection) {
            std::array<wchar_t,512> label{}; TVITEMW item{}; item.mask=TVIF_TEXT; item.hItem=tree_items_[i]; item.pszText=label.data(); item.cchTextMax=static_cast<int>(label.size());
            TreeView_GetItem(ControlHandle(Tree),&item); context+="  |  Selected: "+Utf8(label.data()); break;
        }
        context+=playing?"\nEditing authoring data; Play uses its existing snapshot.":scene_doc?
            (edit_overrides_?"\nEdit Instance Override: changes belong only to this placement.":"\nView Definition: use Edit Prefab Definition or choose Edit Instance Override."):
            prefab_doc?"\nEdit Definition: shared by all placements without an overriding value.":"\nAuthoring data. Full stable IDs are available in properties.";
    }
    if (Text(ControlHandle(Context))!=context) SetText(ControlHandle(Context),context);
    for (int id:{NewScene,NewPrefab,NewGraph,Add,Remove}) EnableWindow(ControlHandle(id),project!=nullptr);
    EnableWindow(ControlHandle(Add),doc && selection.kind!=ed::UiElement::PrefabComponent);
    for (int id:{Palette,Source,Destination,Connect}) EnableWindow(ControlHandle(id),graph);
    EnableWindow(ControlHandle(AddComponent),prefab); EnableWindow(ControlHandle(ReplaceComponent),selection.kind==ed::UiElement::PrefabComponent);
    for (int id:{AttachBehavior,BehaviorGraph}) EnableWindow(ControlHandle(id),prefab || selection.kind==ed::UiElement::PrefabComponent);
    EnableWindow(ControlHandle(Place),selection.kind==ed::UiElement::SceneNode); EnableWindow(ControlHandle(Prefabs),selection.kind==ed::UiElement::SceneNode);
    EnableWindow(ControlHandle(FollowPrefab),workspace_.PrefabDefinitionSelection().has_value());
    for (int id:{SceneParents,Reparent}) EnableWindow(ControlHandle(id),selection.kind==ed::UiElement::SceneNode);
    const bool missing_field=field_>=0 && static_cast<std::size_t>(field_)<fields_.size() && fields_[static_cast<std::size_t>(field_)].missing;
    const auto *selected_field=field_>=0 && static_cast<std::size_t>(field_)<fields_.size()?&fields_[static_cast<std::size_t>(field_)]:nullptr;
    EnableWindow(ControlHandle(ResetOverride),selected_field && selected_field->reset.has_value());
    EnableWindow(ControlHandle(ExposeParameter),selected_field && selected_field->binding && !selected_field->bound);
    EnableWindow(ControlHandle(DetachBinding),selected_field && selected_field->binding && selected_field->bound);
    EnableWindow(ControlHandle(Apply),Draft() || missing_field); EnableWindow(ControlHandle(Revert),Draft());
    std::string log=local_error_;
    if (workspace_.LastError()) { const auto &error=*workspace_.LastError(); log+="\r\nLast command failed: "+error.code+": "+error.message; }
    const std::array<const char *,5> states{"Completed","Budget exhausted","Failed","Waiting","Cancelled"};
    for (const auto &activation:workspace_.Activations()) {
        const auto report=activation->Report(); log+="\r\nActivation "+Number(activation->Id())+" / "+(report?states[static_cast<std::size_t>(report->status)]:"Queued");
        if (report) log+=" / last step instructions "+Number(report->instructions)+" / native calls "+Number(report->native_calls);
        if (report && report->diagnostic) { const auto &d=*report->diagnostic; log+=" / "+d.error.code+" / asset "+Number(d.asset.value.Raw())+" / node "+Number(d.node.Raw())+" / instruction "+Number(d.instruction)+" / "+d.error.message; }
    }
    if (log.empty()) log="Ready";
    log="Diagnostics: "+log;
    for(std::size_t at=0;(at=log.find("\r\n",at))!=std::string::npos;) log.replace(at,2," | ");
    if (log!=log_text_) { log_text_=log; SetText(ControlHandle(Log),log); }
    const auto menu=GetMenu(hwnd_); if(menu) for(int id:{New,NewDemo,Open,Save,Undo,Redo,FrameSelection,FitScene,RefreshInspector,Play,Stop,SendEvent}) EnableMenuItem(menu,static_cast<UINT>(id),MF_BYCOMMAND|(IsWindowEnabled(ControlHandle(id))?MF_ENABLED:MF_GRAYED));
}
int Desktop::Shortcut(HWND source, WPARAM key) const noexcept {
    if (GetKeyState(VK_CONTROL)<0) {
        if (key=='S') return Save;
        if (key=='O') return Open;
        if (key=='N') return New;
        if (source!=ControlHandle(Value) && source!=ControlHandle(Payload) && source!=ControlHandle(SnapValue)) {
            if (key=='Z') return Undo;
            if (key=='Y') return Redo;
        }
    } else if (key==VK_F5) return GetKeyState(VK_SHIFT)<0?Stop:Play;
    else if (source==ControlHandle(Value) || source==ControlHandle(BooleanValue)) {
        if (key==VK_RETURN) return Apply;
        if (key==VK_ESCAPE) return Revert;
    }
    else if (source==ControlHandle(SnapValue) && key==VK_RETURN) return SetSnap;
    return 0;
}
LRESULT CALLBACK Desktop::ParentProc(HWND,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) noexcept {
    auto &self=*reinterpret_cast<Desktop *>(data);
    try { return self.Message(msg,wp,lp); }
    catch (const std::exception &error) {
        // Reporting an exception must not allocate again (e.g. bad_alloc).
        std::array<wchar_t,512> detail{};
        if (!MultiByteToWideChar(CP_UTF8,0,error.what(),-1,detail.data(),static_cast<int>(detail.size())))
            lstrcpynW(detail.data(),L"Editor operation failed; diagnostic unavailable.",static_cast<int>(detail.size()));
        SetWindowTextW(self.ControlHandle(Log),detail.data()); return 0;
    }
    catch (...) { SetWindowTextW(self.ControlHandle(Log),L"Unexpected Editor error"); return 0; }
}
LRESULT CALLBACK Desktop::ChildProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) noexcept {
    auto &self=*reinterpret_cast<Desktop *>(data);
    std::array<wchar_t,32> kind{};GetClassNameW(hwnd,kind.data(),static_cast<int>(kind.size()));
    const bool picker=hwnd==self.ControlHandle(InspectorTabs) || lstrcmpiW(kind.data(),L"COMBOBOX")==0;
    if(picker && msg==WM_ERASEBKGND){RECT rect{};GetClientRect(hwnd,&rect);FillRect(reinterpret_cast<HDC>(wp),&rect,self.style_.Input());return TRUE;}
    if(picker && (msg==WM_PAINT || msg==WM_PRINT || msg==WM_PRINTCLIENT)) {
        PAINTSTRUCT paint{};const auto dc=msg==WM_PAINT?BeginPaint(hwnd,&paint):reinterpret_cast<HDC>(wp);
        try{self.PaintPicker(hwnd,dc);}catch(...){SetWindowTextW(self.ControlHandle(Log),L"Cannot render Editor control.");}
        if(msg==WM_PAINT)EndPaint(hwnd,&paint);return 0;
    }
    if (msg==WM_KEYDOWN) {
        const int action=self.Shortcut(hwnd,wp);
        if (wp==VK_TAB) { const auto next=GetNextDlgTabItem(self.hwnd_,hwnd,GetKeyState(VK_SHIFT)<0); if (next) SetFocus(next); return 0; }
        if (action) { SendMessageW(self.hwnd_,WM_COMMAND,static_cast<WPARAM>(action),0); return 0; }
    }
    return DefSubclassProc(hwnd,msg,wp,lp);
}
LRESULT Desktop::Message(UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_NOTIFY && !lp)return 0;
    if(msg==WM_MEASUREITEM && lp) { auto &item=*reinterpret_cast<MEASUREITEMSTRUCT *>(lp);if(item.CtlType==ODT_COMBOBOX){item.itemHeight=static_cast<UINT>(MulDiv(26,static_cast<int>(dpi_),96));return TRUE;} }
    if(msg==WM_ERASEBKGND) { PaintChrome(reinterpret_cast<HDC>(wp)); return 1; }
    if(msg==WM_PAINT) { PAINTSTRUCT paint{}; const auto dc=BeginPaint(hwnd_,&paint); PaintChrome(dc); EndPaint(hwnd_,&paint); return 0; }
    if(msg==WM_CTLCOLORSTATIC || msg==WM_CTLCOLOREDIT || msg==WM_CTLCOLORLISTBOX) {
        const auto dc=reinterpret_cast<HDC>(wp); const int id=GetDlgCtrlID(reinterpret_cast<HWND>(lp));
        const bool input=msg!=WM_CTLCOLORSTATIC || id==Log || id==Hint;
        COLORREF color=style_.text;
        if(id==Hint || id==ToolsHint || id==WorkspaceHint || id==ProjectLabel || id==HistoryLabel || id==EventLabel)color=style_.muted;
        if(id==Status)color=workspace_.LastError() || !local_error_.empty()?style_.danger:style_.text;
        if(id==PreviewLabel && workspace_.Session() && workspace_.Session()->IsPlaying())color=style_.success;
        SetTextColor(dc,color); SetBkColor(dc,input?style_.input:style_.panel); return reinterpret_cast<LRESULT>(input?style_.Input():style_.Panel());
    }
    if(msg==WM_DRAWITEM && lp) { if(DrawControl(*reinterpret_cast<const DRAWITEMSTRUCT *>(lp)))return TRUE; }
    if(msg==WM_SYSCOLORCHANGE || msg==WM_THEMECHANGED || (msg==WM_SETTINGCHANGE && wp==SPI_SETHIGHCONTRAST)) { style_.Refresh(); style_.ApplyFrame(hwnd_); TreeView_SetBkColor(ControlHandle(Tree),style_.input); TreeView_SetTextColor(ControlHandle(Tree),style_.text); InvalidateRect(hwnd_,nullptr,TRUE); }
    // Platform publishes raw system keys for games; a desktop host also needs
    // their standard Windows behavior (Alt+F4, Alt+Space).
    if (msg==WM_SYSKEYDOWN || msg==WM_SYSKEYUP) { (void)DefSubclassProc(hwnd_,msg,wp,lp); return DefWindowProcW(hwnd_,msg,wp,lp); }
    if (msg==WM_KEYDOWN) {
        if (wp==VK_TAB) { const auto next=GetNextDlgTabItem(hwnd_,nullptr,GetKeyState(VK_SHIFT)<0); if (next) SetFocus(next); return 0; }
        if (const int action=Shortcut(hwnd_,wp)) { Action(action); return 0; }
    }
    if (msg==WM_DPICHANGED) {
        const auto result=DefSubclassProc(hwnd_,msg,wp,lp);
        Fonts(HIWORD(wp));
        if (const auto *suggested=reinterpret_cast<const RECT *>(lp)) SetWindowPos(hwnd_,nullptr,suggested->left,suggested->top,suggested->right-suggested->left,suggested->bottom-suggested->top,SWP_NOZORDER | SWP_NOACTIVATE);
        Layout(); SyncScene(); return result;
    }
    if (msg==WM_SIZE) { Layout(); return DefSubclassProc(hwnd_,msg,wp,lp); }
    if (msg==WM_GETMINMAXINFO) { const auto dpi=static_cast<int>(dpi_); auto *limits=reinterpret_cast<MINMAXINFO *>(lp); limits->ptMinTrackSize={MulDiv(1000,dpi,96),MulDiv(800,dpi,96)}; return 0; }
    if (msg==WM_CLOSE) { scene_view_->Cancel(); if (CanReplace()) { const auto result=workspace_.Stop(); if (result) { KillTimer(hwnd_,1); timer_running_=false; closed_=true; } else Diagnostics(); } return 0; }
    if (msg==WM_TIMER && wp==1) { if (timer_running_) { (void)workspace_.Tick(GameDuration{1}); Diagnostics(); } return 0; }
    if (msg==WM_COMMAND && !refreshing_) {
        const auto id=LOWORD(wp), notification=HIWORD(wp);
        if(id==10000 && !lp){MessageBoxW(hwnd_,L"Epidemic Editor - development build",L"About",MB_OK | MB_ICONINFORMATION);return 0;}
        if (id==Fields && notification==LBN_SELCHANGE) {
            const auto index=static_cast<int>(SendMessageW(ControlHandle(Fields),LB_GETCURSEL,0,0));
            if (!Draft()) SelectField(index);
            else { SendMessageW(ControlHandle(Fields),LB_SETCURSEL,static_cast<WPARAM>(field_),0); local_error_="Apply or Revert the edited field before changing selection."; }
            Diagnostics();
        } else if (id==BooleanValue && notification==CBN_SELCHANGE) {
            const auto index=SendMessageW(ControlHandle(BooleanValue),CB_GETCURSEL,0,0);
            if (index==0 || index==1) SetText(ControlHandle(Value),index==1?"true":"false");
            Diagnostics();
        } else if (id==Value && notification==EN_CHANGE) Diagnostics();
        else if (notification==BN_CLICKED || ((id==ToolMode || id==CoordinateSpace || id==OverrideMode) && notification==CBN_SELCHANGE)) Action(id);
        return 0;
    }
    if(msg==WM_NOTIFY && lp) {
        const auto *header=reinterpret_cast<NMHDR *>(lp);
        if(header->idFrom==Tree && header->code==NM_CUSTOMDRAW) {
            auto &draw=*reinterpret_cast<NMTVCUSTOMDRAW *>(lp);
            if(draw.nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW;
            if(draw.nmcd.dwDrawStage==CDDS_ITEMPREPAINT) {
                const bool selected=reinterpret_cast<HTREEITEM>(draw.nmcd.dwItemSpec)==TreeView_GetSelection(ControlHandle(Tree));
                draw.clrText=selected && style_.HighContrast()?GetSysColor(COLOR_HIGHLIGHTTEXT):style_.text;
                draw.clrTextBk=selected?style_.selection:style_.input;
                // Only the paint copy changes; native selection/lifetime stays intact.
                draw.nmcd.uItemState&=~static_cast<UINT>(CDIS_SELECTED);
                return CDRF_DODEFAULT;
            }
        }
    }
    if (msg==WM_NOTIFY && !refreshing_) {
        const auto *notice=reinterpret_cast<NMHDR *>(lp);
        if(notice->idFrom==InspectorTabs && notice->code==TCN_SELCHANGING && Draft()) { local_error_="Apply or Revert the field before changing Inspector tabs."; Diagnostics(); return TRUE; }
        if(notice->idFrom==InspectorTabs && notice->code==TCN_SELCHANGE) { inspector_actions_=TabCtrl_GetCurSel(ControlHandle(InspectorTabs))==1; Layout(); Diagnostics(); return 0; }
        if (notice->idFrom==Tree && notice->code==TVN_SELCHANGINGW && Draft()) { local_error_="Apply or Revert the edited field before changing selection."; Diagnostics(); return TRUE; }
        if (notice->idFrom==Tree && notice->code==TVN_SELCHANGEDW) {
            const auto *change=reinterpret_cast<NMTREEVIEWW *>(lp); const auto index=static_cast<std::size_t>(change->itemNew.lParam);
            if (index<selections_.size() && workspace_.Select(selections_[index])) {
                refreshing_=true; outputs_.clear(); inputs_.clear(); SendMessageW(ControlHandle(Source),CB_RESETCONTENT,0,0); SendMessageW(ControlHandle(Destination),CB_RESETCONTENT,0,0); Inspector(); SyncScene(); refreshing_=false; Diagnostics();
            }
            return 0;
        }
    }
    return DefSubclassProc(hwnd_,msg,wp,lp);
}
void Desktop::Smoke(const std::filesystem::path &file, platform::IPlatformRuntime &events) {
    const auto check=[](bool value,const std::source_location at=std::source_location::current()) { if (!value) throw std::runtime_error("Native Editor smoke invariant at line "+Number(at.line())); };
    const auto menu=GetMenu(hwnd_); check(menu && GetMenuItemCount(menu)==6);
    check((GetMenuState(menu,5,MF_BYPOSITION)&MF_GRAYED)!=0);
    check(GetMenuItemCount(GetSubMenu(menu,0))==6 && GetMenuItemCount(GetSubMenu(menu,1))==2);
    check(GetMenuState(GetSubMenu(menu,0),NewProjectFolder,MF_BYCOMMAND)!=static_cast<UINT>(-1));
    check(GetMenuState(GetSubMenu(menu,0),CloneProjectFolder,MF_BYCOMMAND)!=static_cast<UINT>(-1));
    check(GetMenuState(GetSubMenu(menu,2),ContentBrowser,MF_BYCOMMAND)!=static_cast<UINT>(-1));
    check(GetMenuItemID(GetSubMenu(menu,0),2)==Open && GetMenuItemID(GetSubMenu(menu,1),0)==Undo);
    check((static_cast<DWORD_PTR>(GetWindowLongPtrW(ControlHandle(Log),GWL_STYLE))&(static_cast<DWORD_PTR>(WS_BORDER)|static_cast<DWORD_PTR>(ES_MULTILINE)))==0);
    auto empty=ex::Definition(schema_); empty.documents.clear(); empty.entry_scene={};
    ex::Take(workspace_.New(file,std::move(empty))); Refresh(true);
    SendMessageW(hwnd_,WM_COMMAND,NewScene,0);
    check(workspace_.Project()->Snapshot()->definition.documents.size()==1 && workspace_.Selection().kind==ed::UiElement::Document);
    ex::Take(workspace_.New(file,ex::Definition(schema_),true)); Refresh(true);
    // Drive the real native command handlers and typed property control.
    (void)workspace_.Select({AssetId::FromString("example.prefab"),ed::UiElement::PrefabNode,ex::Id("enemy").Raw()}); Refresh();
    int points=-1; for (std::size_t i=0;i<fields_.size();++i) if (fields_[i].name.ends_with(" / points")) points=static_cast<int>(i);
    check(points>=0); SelectField(points); SetText(ControlHandle(Value),"87");
    SendMessageW(hwnd_,WM_COMMAND,Apply,0); check(!Draft() && workspace_.Project()->IsDirty());
    const auto edited=workspace_.Project()->Snapshot();
    SendMessageW(hwnd_,WM_COMMAND,Undo,0); check(workspace_.Project()->Snapshot()!=edited);
    SendMessageW(hwnd_,WM_COMMAND,Redo,0); check(workspace_.Project()->Snapshot()==edited);
    SelectField(points); SetText(ControlHandle(Value),"-1"); SendMessageW(hwnd_,WM_COMMAND,Apply,0);
    check(Draft() && workspace_.Project()->Snapshot()==edited); SendMessageW(hwnd_,WM_COMMAND,Revert,0);
    SendMessageW(hwnd_,WM_COMMAND,Save,0); check(!workspace_.Project()->IsDirty());
    const auto choose=[&](const ed::UiSelection &target) {
        const auto found=std::find(selections_.begin(),selections_.end(),target); check(found!=selections_.end());
        TreeView_SelectItem(ControlHandle(Tree),tree_items_[static_cast<std::size_t>(found-selections_.begin())]); check(workspace_.Selection()==target);
    };
    const auto prefab_id=AssetId::FromString("example.prefab");
    // M5: generic palette and schema fields, driven through the native handlers.
    const ed::UiSelection entity{prefab_id,ed::UiElement::PrefabNode,ex::Id("enemy").Raw()};
    const ed::UiSelection tuning{prefab_id,ed::UiElement::PrefabComponent,entity.element,schema_.tuning,{}};
    const ed::UiSelection movement{prefab_id,ed::UiElement::PrefabComponent,entity.element,schema_.movement,{}};
    choose(movement); SendMessageW(ControlHandle(ComponentTypes),CB_SETCURSEL,2,0);
    const auto before_replace=workspace_.Project()->Snapshot(); Action(ReplaceComponent);
    check(workspace_.Selection()==tuning && workspace_.Project()->Snapshot()!=before_replace);
    Action(Undo); check(workspace_.Project()->Snapshot()==before_replace);
    choose(movement); SendMessageW(ControlHandle(ComponentTypes),CB_SETCURSEL,0,0); Action(ReplaceComponent);
    check(workspace_.Project()->Snapshot()==before_replace); // Health slot is occupied.
    choose(entity); SendMessageW(ControlHandle(ComponentTypes),CB_SETCURSEL,2,0); Action(AddComponent);
    check(workspace_.Selection()==tuning);
    const auto field_named=[&](std::string_view suffix) {
        const auto found=std::find_if(fields_.begin(),fields_.end(),[&](const auto &field){return field.name.ends_with(suffix);});
        check(found!=fields_.end()); SelectField(static_cast<int>(found-fields_.begin()));
    };
    const auto enter=[&](std::string_view suffix,std::string_view text) { field_named(suffix); SetText(ControlHandle(Value),text); Action(Apply); check(!Draft()); };
    enter(" / count","12"); enter(" / weight","2.75"); enter(" / title","  Unicode: ÐŸÑ€Ð¸Ð²ÐµÑ‚  ");
    field_named(" / active"); check(!IsWindowVisible(ControlHandle(Value)));
    SendMessageW(ControlHandle(BooleanValue),CB_SETCURSEL,0,0);
    SendMessageW(hwnd_,WM_COMMAND,MAKEWPARAM(BooleanValue,CBN_SELCHANGE),0);
    check(Draft()); Action(Apply); check(!Draft()); field_named(" / active"); check(fields_[static_cast<std::size_t>(field_)].value=="false");
    const auto tuned=workspace_.Project()->Snapshot(); const auto tuned_revision=workspace_.Project()->Revision();
    field_named(" / weight"); SetText(ControlHandle(Value),"nan"); Action(Apply);
    check(Draft() && workspace_.Project()->Snapshot()==tuned); Action(Revert);
    field_named(" / count"); SetText(ControlHandle(Value),"-1"); Action(Apply);
    check(Draft() && workspace_.Project()->Revision()==tuned_revision); Action(Revert);
    Action(Remove); check(workspace_.Selection()==entity); Action(Undo); check(workspace_.Project()->Snapshot()==tuned);
    choose(tuning); Action(AttachBehavior);
    const auto reference=std::find_if(fields_.begin(),fields_.end(),[](const auto &field){return field.reference.IsValid();});
    check(reference!=fields_.end()); SelectField(static_cast<int>(reference-fields_.begin()));
    Action(OpenReference); check(workspace_.Selection().document==AssetId::FromString("example.graph"));
    Action(Undo); check(workspace_.Project()->Snapshot()==tuned); choose(entity);
    Action(AddComponent); check(workspace_.Project()->Snapshot()==tuned && !local_error_.empty()); // Duplicate slot cannot overwrite.
    Action(Save); check(!workspace_.Project()->IsDirty() && local_error_.empty());
    // Fixture loaded without an authored string: explicit empty Apply must publish,
    // while Save/selection must not manufacture a default value.
    auto incomplete=component_recipes_[2]; incomplete.properties.pop_back();
    ex::Take(workspace_.Execute({ed::SetComponent{prefab_id,ex::Id("enemy"),incomplete}},workspace_.Project()->Revision()));
    Refresh(); choose(tuning); field_named(" / title");
    check(fields_[static_cast<std::size_t>(field_)].missing && !Draft());
    const auto absent=workspace_.Project()->Snapshot(); Action(Save); check(workspace_.Project()->Snapshot()==absent);
    field_named(" / title"); check(IsWindowEnabled(ControlHandle(Apply))); Action(Apply);
    check(workspace_.Project()->Snapshot()!=absent && !fields_[static_cast<std::size_t>(field_)].missing);
    Action(Undo); check(workspace_.Project()->Snapshot()==absent);
    Action(Undo); check(workspace_.Project()->Snapshot()==tuned); Action(Save);
    // M6: expose/default/binding and explicit placement override through UI.
    choose(tuning); field_named(" / count"); Action(ExposeParameter);
    field_named(" / count"); check(!fields_[static_cast<std::size_t>(field_)].command && fields_[static_cast<std::size_t>(field_)].bound);
    Action(DetachBinding); field_named(" / count"); check(static_cast<bool>(fields_[static_cast<std::size_t>(field_)].command)); Action(Undo);
    const auto parameter_field=[&] {
        const auto found=std::find_if(fields_.begin(),fields_.end(),[](const auto &field){return field.name.starts_with("Parameter [");});
        check(found!=fields_.end()); SelectField(static_cast<int>(found-fields_.begin()));
    };
    parameter_field(); SetText(ControlHandle(Value),"30"); Action(Apply);
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42});
    SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,1,0); Action(OverrideMode);
    enter(" / count","99"); field_named(" / count"); check(fields_[static_cast<std::size_t>(field_)].reset.has_value());
    const auto overridden=workspace_.Project()->Snapshot(); Action(ResetOverride); field_named(" / count");
    check(fields_[static_cast<std::size_t>(field_)].value=="30" && !fields_[static_cast<std::size_t>(field_)].reset);
    Action(Undo); check(workspace_.Project()->Snapshot()==overridden); Action(Undo); // Remove only that property override.
    field_named(" / count"); SetText(ControlHandle(Value),"44");
    SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,0,0); Action(OverrideMode);
    check(Draft() && SendMessageW(ControlHandle(OverrideMode),CB_GETCURSEL,0,0)==1); Action(Revert);
    SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,0,0); Action(OverrideMode);
    field_named(" / count"); check(!fields_[static_cast<std::size_t>(field_)].command);
    Action(Undo); Action(Undo); check(workspace_.Project()->Snapshot()==tuned); Action(Save);
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42});
    check(IsWindowVisible(ControlHandle(Viewport))==IsWindowVisible(hwnd_));
    POINT native_hit{20,20}; ClientToScreen(ControlHandle(Viewport),&native_hit);
    check(SendMessageW(ControlHandle(Viewport),WM_NCHITTEST,0,MAKELPARAM(native_hit.x,native_hit.y))==HTCLIENT);
    SendMessageW(hwnd_,WM_COMMAND,FitScene,0);
    const auto before_camera=workspace_.Project()->Snapshot();
    const auto pick=scene_view_->Projection().ToScreen({0,0});
    SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0); check(local_error_.empty());
    SendMessageW(ControlHandle(Viewport),WM_LBUTTONDOWN,0,MAKELPARAM(static_cast<int>(pick.x*dpi_/96),static_cast<int>(pick.y*dpi_/96)));
    check(workspace_.Selection().element==42 && workspace_.Project()->Snapshot()==before_camera);
    // Raster regression: Presentation letterboxes tall child surfaces as well
    // as wide ones. A pixel inside the picked box must contain its selected fill.
    struct ComScope {
        HRESULT status=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        ~ComScope() { if (SUCCEEDED(status)) CoUninitialize(); }
    } com;
    check(SUCCEEDED(com.status) || com.status==RPC_E_CHANGED_MODE);
    const auto capture_dpi=dpi_;
    for (const auto dpi:{96u,144u}) {
        Fonts(dpi); SyncScene();
        SetWindowPos(ControlHandle(Viewport),nullptr,0,0,MulDiv(480,static_cast<int>(dpi),96),MulDiv(1000,static_cast<int>(dpi),96),SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        scene_view_->Frame(true);
        const auto capture=file.parent_path()/("scene-view-"+Number(dpi)+".png"); scene_view_->Capture(capture);
        const auto bounds=scene_view_->Projection().ScreenBounds(scene_view_->Projection().Items().front());
        const auto probe=scene_view_->Projection().ToScreen({0,0});
        check(scene_view_->Projection().Pick({probe.x+20,bounds.top+20})==SceneNodeId{42});
        Microsoft::WRL::ComPtr<IWICImagingFactory> imaging;
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&imaging))));
        check(SUCCEEDED(imaging->CreateDecoderFromFilename(capture.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)));
        check(SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(imaging->CreateFormatConverter(&converter)));
        check(SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)));
        const WICRect pixel_region{static_cast<int>((probe.x+20)*dpi/96),static_cast<int>((bounds.top+20)*dpi/96),1,1};
        std::array<BYTE,4> pixel{};
        check(SUCCEEDED(converter->CopyPixels(&pixel_region,4,4,pixel.data())));
        check(pixel[0]>60 && pixel[0]>pixel[2]);
    }
    Fonts(capture_dpi); Layout(); SyncScene();
    // Drive owned native handlers only; no external input/desktop automation.
    const auto original_scene=workspace_.Project()->Snapshot();
    const auto original_revision=workspace_.Project()->Revision();
    const auto gesture_point=[&](ed::ViewPoint point) { return MAKELPARAM(static_cast<int>(std::lround(point.x*dpi_/96)),static_cast<int>(std::lround(point.y*dpi_/96))); };
    const auto gizmo=[&] { return ed::MakeGizmo(scene_view_->Projection(),scene_view_->Projection().Items().front().world,ed::GizmoSpace::Local); };
    const auto mouse=[&](UINT message,ed::ViewPoint point) { SendMessageW(ControlHandle(Viewport),message,0,gesture_point(point)); };
    const auto undo_gesture=[&] { SendMessageW(hwnd_,WM_COMMAND,Undo,0); check(workspace_.Project()->Snapshot()==original_scene); };
    const auto mode=[&](int index) { SendMessageW(ControlHandle(ToolMode),CB_SETCURSEL,static_cast<WPARAM>(index),0); SendMessageW(hwnd_,WM_COMMAND,MAKEWPARAM(ToolMode,CBN_SELCHANGE),0); };
    scene_view_->Frame(true); auto handles=gizmo(); mouse(WM_LBUTTONDOWN,handles.x);
    for (int i=1;i<=5;++i) mouse(WM_MOUSEMOVE,{handles.x.x+i*8,handles.x.y+25});
    check(workspace_.Project()->Snapshot()==original_scene && workspace_.Project()->Revision()==original_revision);
    mouse(WM_LBUTTONUP,{handles.x.x+40,handles.x.y+25});
    check(workspace_.Project()->Revision()==original_revision+1 && workspace_.Project()->IsDirty());
    const auto native_moved=workspace_.Project()->Snapshot(); undo_gesture();
    SendMessageW(hwnd_,WM_COMMAND,Redo,0); check(workspace_.Project()->Snapshot()==native_moved); undo_gesture();
    for (const auto message:{WM_KEYDOWN,WM_CAPTURECHANGED,WM_RBUTTONDOWN,WM_CANCELMODE,WM_KILLFOCUS,WM_SIZE}) {
        handles=gizmo(); mouse(WM_LBUTTONDOWN,handles.x); mouse(WM_MOUSEMOVE,{handles.x.x+40,handles.x.y});
        SendMessageW(ControlHandle(Viewport),static_cast<UINT>(message),message==WM_KEYDOWN?VK_ESCAPE:0,0);
        check(workspace_.Project()->Snapshot()==original_scene && !workspace_.Project()->IsDirty() && GetCapture()!=ControlHandle(Viewport));
        check(scene_view_->Projection().Items().front().world.position==Vec3{});
    }
    mode(1); check(!IsWindowEnabled(ControlHandle(CoordinateSpace))); handles=gizmo();
    mouse(WM_LBUTTONDOWN,handles.x); mouse(WM_LBUTTONUP,handles.y);
    check(workspace_.Project()->Snapshot()!=original_scene); undo_gesture();
    mode(2); handles=gizmo(); mouse(WM_LBUTTONDOWN,handles.x); mouse(WM_LBUTTONUP,{handles.x.x+52,handles.x.y});
    check(scene_view_->Projection().Items().front().world.scale.x==2); undo_gesture();
    handles=gizmo(); mouse(WM_LBUTTONDOWN,handles.pivot); mouse(WM_LBUTTONUP,{handles.pivot.x+26,handles.pivot.y-26});
    check(scene_view_->Projection().Items().front().world.scale.x==1.5f && scene_view_->Projection().Items().front().world.scale.y==1.5f); undo_gesture();
    handles=gizmo(); mouse(WM_LBUTTONDOWN,handles.x); mouse(WM_LBUTTONUP,handles.pivot);
    check(workspace_.Project()->Snapshot()==original_scene && !workspace_.Project()->IsDirty());
    const auto arrow_revision=workspace_.Project()->Revision();
    SendMessageW(ControlHandle(Viewport),WM_KEYDOWN,VK_RIGHT,1); SendMessageW(ControlHandle(Viewport),WM_KEYDOWN,VK_RIGHT,1);
    check(workspace_.Project()->Snapshot()==original_scene); SendMessageW(ControlHandle(Viewport),WM_KEYUP,VK_RIGHT,0);
    check(workspace_.Project()->Revision()==arrow_revision+1); undo_gesture();
    mode(0); SendMessageW(ControlHandle(CoordinateSpace),CB_SETCURSEL,0,0); Action(CoordinateSpace);
    SetText(ControlHandle(SnapValue),"nan"); Action(SetSnap); check(scene_view_->SnapStep()==0 && workspace_.Project()->Snapshot()==original_scene);
    SetText(ControlHandle(SnapValue),"1"); Action(SetSnap); check(scene_view_->SnapStep()==1);
    handles=gizmo(); mouse(WM_LBUTTONDOWN,handles.pivot); mouse(WM_LBUTTONUP,{handles.pivot.x+2.3*scene_view_->Projection().Scale(),handles.pivot.y});
    check(scene_view_->Projection().Items().front().world.position.x==2); undo_gesture();
    SetText(ControlHandle(SnapValue),"0"); Action(SetSnap);
    check(IsWindowEnabled(ControlHandle(FollowPrefab))); SendMessageW(hwnd_,WM_COMMAND,FollowPrefab,0);
    check(workspace_.Selection()==ed::UiSelection{prefab_id,ed::UiElement::PrefabNode,ex::Id("enemy").Raw()});
    const ed::UiSelection component{prefab_id,ed::UiElement::PrefabComponent,ex::Id("enemy").Raw(),schema_.health,{}};
    choose(component);
    check(std::any_of(fields_.begin(),fields_.end(),[](const auto &field){return field.name.ends_with(" / points");}));
    check(std::none_of(fields_.begin(),fields_.end(),[](const auto &field){return field.name.ends_with(" / speed");}));
    SendMessageW(hwnd_,WM_COMMAND,Remove,0); check(workspace_.Selection().kind==ed::UiElement::PrefabNode);
    SendMessageW(hwnd_,WM_COMMAND,Undo,0); choose(component);
    // Preserve a collapsed, unrelated document while refreshing the selected component.
    const auto scene_index=std::find(selections_.begin(),selections_.end(),ed::UiSelection{AssetId::FromString("example.scene")})-selections_.begin();
    const auto collapsed=[&] {
        TVITEMW item{}; item.mask=TVIF_STATE; item.stateMask=TVIS_EXPANDED; item.hItem=tree_items_[static_cast<std::size_t>(scene_index)];
        check(TreeView_GetItem(ControlHandle(Tree),&item)); return (item.state&static_cast<UINT>(TVIS_EXPANDED))==0;
    };
    TreeView_Expand(ControlHandle(Tree),tree_items_[static_cast<std::size_t>(scene_index)],TVE_COLLAPSE);
    check(collapsed());
    Refresh(); check(workspace_.Selection()==component);
    check(collapsed());
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42});
    const auto edit_transform=[&](std::string_view name,std::string_view value) {
        const auto found=std::find_if(fields_.begin(),fields_.end(),[&](const auto &field){return field.name==name;}); check(found!=fields_.end());
        SelectField(static_cast<int>(found-fields_.begin())); SetText(ControlHandle(Value),value); SendMessageW(hwnd_,WM_COMMAND,Apply,0);
    };
    edit_transform("Local position X","-12.5"); edit_transform("Local scale X","2"); edit_transform("Local rotation Z (degrees)","90");
    const auto transformed=workspace_.Project()->Snapshot();
    edit_transform("Local scale X","0"); check(Draft() && workspace_.Project()->Snapshot()==transformed); SendMessageW(hwnd_,WM_COMMAND,Revert,0);
    SendMessageW(hwnd_,WM_COMMAND,Add,0); const auto group_id=workspace_.Selection().element;
    check(workspace_.Selection().kind==ed::UiElement::SceneNode && group_id!=42);
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42});
    SendMessageW(ControlHandle(SceneParents),CB_SETCURSEL,1,0); SendMessageW(hwnd_,WM_COMMAND,Reparent,0);
    const auto parented=workspace_.Project()->Snapshot();
    const auto &scene=std::get<gc::SceneCompositionDefinition>(parented->Find(AssetId::FromString("example.scene"))->data);
    check(scene.scene.nodes.front().parent.Raw()==group_id && scene.scene.nodes.front().local_transform.position.x==-12.5f);
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,group_id});
    SendMessageW(ControlHandle(SceneParents),CB_SETCURSEL,1,0); SendMessageW(hwnd_,WM_COMMAND,Reparent,0); check(workspace_.Project()->Snapshot()==parented);
    // Graph buttons create, validate and undo connections through the same Command API.
    SendMessageW(hwnd_,WM_COMMAND,NewGraph,0); const auto draft_graph=workspace_.Selection().document;
    SendMessageW(ControlHandle(Palette),CB_SETCURSEL,0,0); SendMessageW(hwnd_,WM_COMMAND,Add,0);
    SendMessageW(ControlHandle(Palette),CB_SETCURSEL,1,0); SendMessageW(hwnd_,WM_COMMAND,Add,0);
    const auto before_invalid=workspace_.Project()->Snapshot();
    check(std::get<graph::GraphDefinition>(before_invalid->Find(draft_graph)->data).nodes.size()==2);
    SendMessageW(ControlHandle(Source),CB_SETCURSEL,1,0); SendMessageW(ControlHandle(Destination),CB_SETCURSEL,1,0);
    SendMessageW(hwnd_,WM_COMMAND,Connect,0); check(workspace_.Project()->Snapshot()==before_invalid);
    SendMessageW(ControlHandle(Source),CB_SETCURSEL,0,0); SendMessageW(hwnd_,WM_COMMAND,Connect,0);
    const auto connected=workspace_.Project()->Snapshot();
    const auto &edges=std::get<graph::GraphDefinition>(connected->Find(draft_graph)->data).connections; check(edges.size()==1);
    const auto edge=edges.front().id;
    (void)workspace_.Select({draft_graph,ed::UiElement::Connection,edge.Raw()}); Refresh(); SendMessageW(hwnd_,WM_COMMAND,Remove,0);
    check(std::get<graph::GraphDefinition>(workspace_.Project()->Snapshot()->Find(draft_graph)->data).connections.empty());
    SendMessageW(hwnd_,WM_COMMAND,Undo,0); check(workspace_.Project()->Snapshot()==connected);
    SendMessageW(hwnd_,WM_COMMAND,Redo,0);
    (void)workspace_.Select({draft_graph}); Refresh();
    SendMessageW(hwnd_,WM_COMMAND,Add,0); // A second Native damage must be distinguishable.
    std::set<std::wstring> port_labels;
    bool unique_labels=true;
    for (LRESULT index=0;index<SendMessageW(ControlHandle(Source),CB_GETCOUNT,0,0);++index) {
        const auto length=SendMessageW(ControlHandle(Source),CB_GETLBTEXTLEN,static_cast<WPARAM>(index),0);
        check(length>=0); std::wstring label(static_cast<std::size_t>(length)+1,L'\0');
        SendMessageW(ControlHandle(Source),CB_GETLBTEXT,static_cast<WPARAM>(index),reinterpret_cast<LPARAM>(label.data()));
        label.resize(static_cast<std::size_t>(length));
        unique_labels=port_labels.insert(std::move(label)).second && unique_labels;
    }
    SendMessageW(hwnd_,WM_COMMAND,Save,0); check(!workspace_.Project()->IsDirty());
    (void)workspace_.Select({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42}); Refresh();
    SendMessageW(hwnd_,WM_KEYDOWN,VK_F5,0);
    const bool root_shortcut=workspace_.Session() && workspace_.Session()->IsPlaying();
    if (!root_shortcut) SendMessageW(hwnd_,WM_COMMAND,Play,0);
    check(workspace_.Session() && workspace_.Session()->IsPlaying());
    SetText(ControlHandle(Payload),"1"); SendMessageW(hwnd_,WM_COMMAND,SendEvent,0); ex::Take(workspace_.Tick({}));
    Action(RefreshInspector);
    check(std::any_of(fields_.begin(),fields_.end(),[](const auto &field){return field.detail.find("runtime snapshot")!=std::string::npos && !field.command;}));
    check(workspace_.Activations().size()==1); const auto activation=workspace_.Activations().front(); check(!activation->IsFinished());
    SendMessageW(hwnd_,WM_COMMAND,Stop,0); check(activation->IsFinished());
    ex::Take(workspace_.Open(file)); Refresh(); check(!workspace_.Project()->IsDirty());
    (void)workspace_.Select({AssetId::FromString("example.prefab"),ed::UiElement::PrefabNode,ex::Id("enemy").Raw()}); Refresh();
    check(std::any_of(fields_.begin(),fields_.end(),[](const auto &field){return field.name.ends_with(" / points") && field.value=="87";}));
    choose(tuning); field_named(" / title"); check(fields_[static_cast<std::size_t>(field_)].value=="  Unicode: ÐŸÑ€Ð¸Ð²ÐµÑ‚  ");
    field_named(" / count"); check(fields_[static_cast<std::size_t>(field_)].value=="12");
    field_named(" / active"); check(fields_[static_cast<std::size_t>(field_)].value=="false");
    // Exercise the supported minimum client layout without touching any other window.
    const auto actual_dpi=GetDpiForWindow(hwnd_);
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42});
    for (const auto dpi:{96u,144u}) {
        Fonts(dpi);
        SetWindowPos(hwnd_,nullptr,0,0,MulDiv(1000,static_cast<int>(dpi),96),MulDiv(800,static_cast<int>(dpi),96),SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); Layout();
        RECT client{}; GetClientRect(hwnd_,&client);
        SyncScene();
        for (int id:{Tree,Fields,Value,BooleanValue,ComponentTypes,OpenReference,RefreshInspector,OverrideMode,ResetOverride,ExposeParameter,DetachBinding,NewDemo,FollowPrefab,Connect,Place,ToolsHint,Log,Viewport,FrameSelection,FitScene}) {
            RECT bounds{}; GetWindowRect(ControlHandle(id),&bounds);
            POINT first{bounds.left,bounds.top},last{bounds.right,bounds.bottom}; ScreenToClient(hwnd_,&first); ScreenToClient(hwnd_,&last);
            check(first.x>=0 && first.y>=0 && last.x<=client.right && last.y<=client.bottom && last.y>first.y);
        }
        LOGFONTW font{}; check(GetObjectW(font_,sizeof(font),&font)!=0 && font.lfHeight==-MulDiv(11,static_cast<int>(dpi),72));
        CaptureUi(file.parent_path()/(dpi==96?"ui-scene-min.png":"ui-scene-min-150.png"));
        SendMessageW(hwnd_,WM_COMMAND,FitScene,0); SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0); check(local_error_.empty());
        const auto screen=scene_view_->Projection().ToScreen({-12.5,0});
        check(scene_view_->Projection().Pick(screen)==SceneNodeId{42});
        const auto native_point=MAKELPARAM(static_cast<int>(screen.x*dpi/96),static_cast<int>(screen.y*dpi/96));
        SendMessageW(ControlHandle(Viewport),WM_LBUTTONDOWN,0,native_point);
        check(workspace_.Selection().element==42);
        const auto stable=workspace_.Project()->Snapshot();
        SendMessageW(ControlHandle(Viewport),WM_MBUTTONDOWN,MK_MBUTTON,MAKELPARAM(60,60));
        SendMessageW(ControlHandle(Viewport),WM_MOUSEMOVE,MK_MBUTTON,MAKELPARAM(90,75));
        SendMessageW(ControlHandle(Viewport),WM_MBUTTONUP,0,MAKELPARAM(90,75));
        check(workspace_.Project()->Snapshot()==stable && GetCapture()!=ControlHandle(Viewport));
        scene_view_->Frame(true);
        const auto &projected=scene_view_->Projection().Items().front();
        check(projected.id==SceneNodeId{42}); const auto position=projected.world.position;
        const auto dpi_frame=ed::MakeGizmo(scene_view_->Projection(),projected.world,ed::GizmoSpace::World);
        const auto dpi_revision=workspace_.Project()->Revision();
        mouse(WM_LBUTTONDOWN,dpi_frame.x); mouse(WM_LBUTTONUP,{dpi_frame.x.x+26,dpi_frame.x.y});
        check(workspace_.Project()->Revision()==dpi_revision+1);
        check(std::abs(scene_view_->Projection().Items().front().world.position.x-position.x-26/scene_view_->Projection().Scale())<.001);
        Action(Undo); check(workspace_.Project()->Snapshot()==stable);
    }
    Fonts(actual_dpi); Layout();
    SendMessageW(hwnd_,WM_SYSKEYDOWN,VK_F4,static_cast<LPARAM>(1ULL<<29u));
    // DefWindowProc may post the system-close command. Exercise the same
    // Platform event pump as the real application before checking its outcome.
    events.PumpEvents();
    if (!unique_labels) std::fprintf(stderr,"REGRESSION: repeated nodes produce ambiguous port labels\n");
    if (!root_shortcut) std::fprintf(stderr,"REGRESSION: F5 ignored when the host window owns focus\n");
    if (!closed_) std::fprintf(stderr,"REGRESSION: Alt+F4 does not close the host window\n");
    check(unique_labels && root_shortcut && closed_);
}
void Desktop::CaptureUi(const std::filesystem::path &file) {
    // Render only this owned, hidden window. No desktop pixels or input APIs.
    const auto require=[](bool value){if(!value)throw std::runtime_error("Cannot capture owned Editor UI");};
    struct ComScope { HRESULT status=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED); ~ComScope(){if(SUCCEEDED(status))CoUninitialize();} } com;
    require(SUCCEEDED(com.status) || com.status==RPC_E_CHANGED_MODE);
    RECT client{}; GetClientRect(hwnd_,&client); const int width=client.right,height=client.bottom;
    require(width>0 && height>0);
    struct Raster {
        HWND window=nullptr; HDC screen=nullptr,dc=nullptr; HBITMAP bitmap=nullptr; HGDIOBJ previous=nullptr; void *pixels=nullptr;
        ~Raster(){if(previous)SelectObject(dc,previous);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);if(screen)ReleaseDC(window,screen);}
    } raster;
    raster.window=hwnd_; raster.screen=GetDC(hwnd_); raster.dc=CreateCompatibleDC(raster.screen);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    raster.bitmap=CreateDIBSection(raster.screen,&info,DIB_RGB_COLORS,&raster.pixels,nullptr,0);
    require(raster.dc && raster.bitmap && raster.pixels); raster.previous=SelectObject(raster.dc,raster.bitmap);
    PaintChrome(raster.dc);
    for(auto child=GetWindow(hwnd_,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)) {
        if((static_cast<DWORD_PTR>(GetWindowLongPtrW(child,GWL_STYLE))&static_cast<DWORD_PTR>(WS_VISIBLE))==0 || child==ControlHandle(Viewport))continue;
        RECT bounds{};GetWindowRect(child,&bounds);MapWindowPoints(nullptr,hwnd_,reinterpret_cast<POINT *>(&bounds),2);
        const auto saved=SaveDC(raster.dc);SetViewportOrgEx(raster.dc,bounds.left,bounds.top,nullptr);
        IntersectClipRect(raster.dc,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
        SendMessageW(child,WM_PRINT,reinterpret_cast<WPARAM>(raster.dc),PRF_NONCLIENT|PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
        RestoreDC(raster.dc,saved);
    }
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    require(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))));
    if(scene_document_) {
        const auto viewport=file.parent_path()/(file.stem().wstring()+L"-viewport.png"); scene_view_->Capture(viewport);
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder; Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame; Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
        require(SUCCEEDED(factory->CreateDecoderFromFilename(viewport.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder)));
        require(SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)));
        require(SUCCEEDED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)));
        RECT rect{}; GetWindowRect(ControlHandle(Viewport),&rect); MapWindowPoints(nullptr,hwnd_,reinterpret_cast<POINT *>(&rect),2);
        require(rect.left>=0 && rect.top>=0 && rect.right<=width && rect.bottom<=height);
        UINT w=0,h=0; require(SUCCEEDED(frame->GetSize(&w,&h)) && w==static_cast<UINT>(rect.right-rect.left) && h==static_cast<UINT>(rect.bottom-rect.top));
        auto *data=static_cast<BYTE *>(raster.pixels)+(static_cast<std::size_t>(rect.top)*static_cast<std::size_t>(width)+static_cast<std::size_t>(rect.left))*4;
        require(SUCCEEDED(converter->CopyPixels(nullptr,static_cast<UINT>(width*4),static_cast<UINT>((height-rect.top)*width*4-rect.left*4),data)));
    }
    auto *pixels=static_cast<BYTE *>(raster.pixels); for(int i=0;i<width*height;++i)pixels[i*4+3]=255;
    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder; Microsoft::WRL::ComPtr<IWICStream> stream; Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> output;
    require(SUCCEEDED(factory->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(file.c_str(),GENERIC_WRITE)));
    require(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder)) && SUCCEEDED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)));
    require(SUCCEEDED(encoder->CreateNewFrame(&output,nullptr)) && SUCCEEDED(output->Initialize(nullptr)) && SUCCEEDED(output->SetSize(static_cast<UINT>(width),static_cast<UINT>(height))));
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA; require(SUCCEEDED(output->SetPixelFormat(&format)));
    require(SUCCEEDED(output->WritePixels(static_cast<UINT>(height),static_cast<UINT>(width*4),static_cast<UINT>(width*height*4),pixels)));
    require(SUCCEEDED(output->Commit()) && SUCCEEDED(encoder->Commit()));
}

void Desktop::DemoSmoke(const std::filesystem::path &file,platform::IPlatformRuntime &events) {
    const auto check=[](bool value,const std::source_location at=std::source_location::current()) { if (!value) throw std::runtime_error("EditorDemo native invariant at line "+Number(at.line())); };
    check(!CreateDemoCopy(EPIDEMIC_EDITOR_DEMO_PROJECT)); ex::Take(CreateDemoCopy(file));
    const auto scene=AssetId::FromString("example.scene");
    const auto choose=[&](std::uint64_t node) {
        const ed::UiSelection selection{scene,node?ed::UiElement::SceneNode:ed::UiElement::Document,node};
        const auto found=std::find(selections_.begin(),selections_.end(),selection); check(found!=selections_.end());
        TreeView_SelectItem(ControlHandle(Tree),tree_items_[static_cast<std::size_t>(found-selections_.begin())]); check(workspace_.Selection()==selection);
    };
    const auto field=[&](std::string_view suffix) {
        const auto found=std::find_if(fields_.begin(),fields_.end(),[&](const auto &item){return item.name.ends_with(suffix);});
        check(found!=fields_.end()); SelectField(static_cast<int>(found-fields_.begin()));
    };
    const auto edit=[&](std::string_view suffix,std::string_view text){field(suffix); SetText(ControlHandle(Value),text); Action(Apply); check(!Draft());};
    choose(0); check(scene_view_->Projection().Items().size()==3); Action(FitScene);
    check(SendMessageW(hwnd_,WM_NOTIFY,0,0)==0);
    if(!style_.HighContrast())check(GetRValue(style_.background)<64 && GetRValue(style_.input)<64 && GetRValue(style_.text)>200);
    check((static_cast<DWORD_PTR>(GetWindowLongPtrW(ControlHandle(ToolMode),GWL_STYLE))&static_cast<DWORD_PTR>(CBS_HASSTRINGS))!=0);
    choose(42); field("Local position X"); CaptureUi(file.parent_path()/"ui-scene.png");
    const auto unchanged=workspace_.Project()->Snapshot(); const auto epoch=workspace_.Project()->Revision();
    SetText(ControlHandle(Value),"-7"); check(Draft());
    NMHDR tab{ControlHandle(InspectorTabs),InspectorTabs,TCN_SELCHANGING};
    check(SendMessageW(hwnd_,WM_NOTIFY,InspectorTabs,reinterpret_cast<LPARAM>(&tab))==TRUE && !inspector_actions_);
    check(workspace_.Project()->Snapshot()==unchanged && workspace_.Project()->Revision()==epoch); Action(Revert);
    check(SendMessageW(hwnd_,WM_NOTIFY,InspectorTabs,reinterpret_cast<LPARAM>(&tab))==FALSE);
    TabCtrl_SetCurSel(ControlHandle(InspectorTabs),1);tab.code=TCN_SELCHANGE;SendMessageW(hwnd_,WM_NOTIFY,InspectorTabs,reinterpret_cast<LPARAM>(&tab));
    const auto shown=[&](int id){return (static_cast<DWORD_PTR>(GetWindowLongPtrW(ControlHandle(id),GWL_STYLE))&static_cast<DWORD_PTR>(WS_VISIBLE))!=0;};
    check(inspector_actions_ && shown(Place) && !shown(Fields) && !shown(Value)); CaptureUi(file.parent_path()/"ui-actions.png");
    TabCtrl_SetCurSel(ControlHandle(InspectorTabs),0);SendMessageW(hwnd_,WM_NOTIFY,InspectorTabs,reinterpret_cast<LPARAM>(&tab));
    check(!inspector_actions_ && !shown(Place) && shown(Fields) && shown(Value) && !Draft());
    check(workspace_.Project()->Snapshot()==unchanged && workspace_.Project()->Revision()==epoch);
    check(Text(ControlHandle(Hint)).find("Local position X")!=std::string::npos);
    check((static_cast<DWORD_PTR>(GetWindowLongPtrW(ControlHandle(Fields),GWL_STYLE))&static_cast<DWORD_PTR>(WS_HSCROLL))==0);
    (void)workspace_.Select({AssetId::FromString("example.prefab"),ed::UiElement::PrefabNode,ex::Id("enemy").Raw()}); Refresh(); CaptureUi(file.parent_path()/"ui-prefab.png");
    (void)workspace_.Select({AssetId::FromString("example.graph")}); Refresh(); CaptureUi(file.parent_path()/"ui-graph.png"); choose(0);
    const auto baseline=workspace_.Project()->Snapshot();
    const auto mouse=[&](UINT message,ed::ViewPoint point) { SendMessageW(ControlHandle(Viewport),message,0,MAKELPARAM(static_cast<int>(std::lround(point.x*dpi_/96)),static_cast<int>(std::lround(point.y*dpi_/96)))); };
    const auto center=scene_view_->Projection().ToScreen({-6,0}); mouse(WM_LBUTTONDOWN,center); mouse(WM_LBUTTONUP,center);
    check(workspace_.Selection().element==42 && workspace_.Project()->Snapshot()==baseline); choose(43); choose(42);
    SetText(ControlHandle(SnapValue),"1"); Action(SetSnap); scene_view_->Frame(true);
    const auto gizmo=ed::MakeGizmo(scene_view_->Projection(),scene_view_->Projection().Items().front().world,ed::GizmoSpace::World);
    mouse(WM_LBUTTONDOWN,gizmo.x); mouse(WM_LBUTTONUP,{gizmo.x.x+scene_view_->Projection().Scale(),gizmo.x.y});
    field("Local position X"); check(fields_[static_cast<std::size_t>(field_)].value=="-5");
    const auto moved=workspace_.Project()->Snapshot(); Action(Undo); check(workspace_.Project()->Snapshot()==baseline); Action(Redo); check(workspace_.Project()->Snapshot()==moved);
    SendMessageW(ControlHandle(OverrideMode),CB_SETCURSEL,1,0); Action(OverrideMode);
    edit(" / points","110"); edit(" / label","Edited Scout");
    const auto edited=workspace_.Project()->Snapshot(); const auto revision=workspace_.Project()->Revision();
    field(" / points"); SetText(ControlHandle(Value),"bad"); Action(Apply);
    check(Draft() && workspace_.Project()->Snapshot()==edited && workspace_.Project()->Revision()==revision); Action(Revert);
    choose(44); Action(Remove); Action(Undo); check(workspace_.Project()->Snapshot()==edited); choose(42);
    choose(0); edit("Document","Arena edited"); Action(Save); check(!workspace_.Project()->IsDirty()); choose(42);
    const auto authored=workspace_.Project()->Snapshot(); Action(Play); check(workspace_.Session() && workspace_.Session()->IsPlaying());
    SetText(ControlHandle(Payload),"1"); Action(SendEvent); ex::Take(workspace_.Tick({}));
    check(workspace_.Activations().size()==1); const auto activation=workspace_.Activations().back(); check(!activation->IsFinished());
    ex::Take(workspace_.Tick(GameDuration{1})); check(activation->IsFinished()); Action(RefreshInspector);
    check(std::any_of(fields_.begin(),fields_.end(),[](const auto &row){return row.name.ends_with(" / points") && row.value=="108" && row.detail.find("runtime snapshot")!=std::string::npos && !row.command;}));
    SetText(ControlHandle(Payload),"999"); Action(SendEvent); ex::Take(workspace_.Tick({})); Action(RefreshInspector);
    check(Text(ControlHandle(Log)).find("example.damage_range")!=std::string::npos);
    Action(Stop); check(workspace_.Project()->Snapshot()==authored && !workspace_.Project()->IsDirty());
    ex::Take(workspace_.Open(file)); Refresh(true); choose(42); field("Local position X"); check(fields_[static_cast<std::size_t>(field_)].value=="-5");
    SendMessageW(hwnd_,WM_CLOSE,0,0); events.PumpEvents(); check(closed_);
}
void Desktop::StressSmoke(const std::filesystem::path &file,platform::WindowsPlatformRuntime &events,std::size_t count) {
    const auto check=[](bool value,const std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Native stress invariant at line "+Number(at.line()));};
    const auto measure=[&]<class F>(const char *name,F &&action,int repeats=1){
        const auto start=std::chrono::steady_clock::now(); for(int i=0;i<repeats;++i)action();
        const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/repeats;
        std::printf("M8 placements=%zu metric=%s value=%.6f\n",count,name,ms);
    };
    struct Usage { double cpu_ms=0; SIZE_T private_bytes=0,working_set=0; DWORD handles=0,gdi=0,user=0; };
    const auto usage=[&]{
        PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb=sizeof(memory); FILETIME created{},exited{},kernel{},user{}; DWORD handles=0;
        check(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),sizeof(memory))!=FALSE);
        check(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user)!=FALSE && GetProcessHandleCount(GetCurrentProcess(),&handles)!=FALSE);
        const auto ticks=[](const FILETIME &time){return (static_cast<std::uint64_t>(time.dwHighDateTime)<<32u)|time.dwLowDateTime;};
        return Usage{static_cast<double>(ticks(kernel)+ticks(user))/10000.,memory.PrivateUsage,memory.WorkingSetSize,handles,GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS)};
    };
    const auto memory=[&](const char *phase){const auto current=usage(); std::printf("M8 placements=%zu phase=%s private_bytes=%zu working_set=%zu handles=%lu gdi=%lu user=%lu\n",count,phase,current.private_bytes,current.working_set,current.handles,current.gdi,current.user);};
    const auto idle=[&](const char *phase){
        check(!timer_running_); SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0); const auto before=usage(); const auto start=std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now()-start<std::chrono::milliseconds{500}){
            events.PumpEvents(); (void)events.DrainEvents();
            const auto remaining=std::chrono::milliseconds{500}-std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start);
            if(remaining.count()>0)MsgWaitForMultipleObjectsEx(0,nullptr,static_cast<DWORD>(remaining.count()),QS_ALLINPUT,MWMO_INPUTAVAILABLE);
        }
        const auto after=usage(); const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::printf("M8 placements=%zu phase=%s idle_cpu_pct_one_core=%.6f repaint_pending=%d\n",count,phase,(after.cpu_ms-before.cpu_ms)/elapsed*100,GetUpdateRect(ControlHandle(Viewport),nullptr,FALSE)!=FALSE); memory(phase);
    };
    std::filesystem::create_directories(file.parent_path());
    const auto scene=AssetId::FromString("example.scene");
    ex::Take(workspace_.New(file,ex::Take(ex::StressDefinition(file,schema_.View(),count)),true)); ex::Take(workspace_.Save());
    const auto healthy=workspace_.Project()->Snapshot();
    ex::Take(workspace_.Execute({ed::RenameDocument{scene,std::string{"Bad UTF-8 "}+static_cast<char>(0xff)}},workspace_.Project()->Revision()));
    SendMessageW(hwnd_,WM_COMMAND,Save,0); // Save succeeds; the presentation refresh rejects unsupported text.
    check(!refreshing_);
    check(Text(ControlHandle(Log)).find("Invalid UTF-8")!=std::string::npos);
    SendMessageW(hwnd_,WM_COMMAND,Undo,0); check(workspace_.Project()->Snapshot()==healthy);
    Action(Save); check(!workspace_.Project()->IsDirty());
    measure("hierarchy_refresh_ms",[&]{Refresh();},3);
    const auto choose=[&](std::uint64_t node){
        const ed::UiSelection selection{scene,node?ed::UiElement::SceneNode:ed::UiElement::Document,node};
        const auto found=std::find(selections_.begin(),selections_.end(),selection); check(found!=selections_.end());
        TreeView_SelectItem(ControlHandle(Tree),tree_items_[static_cast<std::size_t>(found-selections_.begin())]); check(workspace_.Selection()==selection);
    };
    const auto field=[&](std::string_view suffix){const auto found=std::find_if(fields_.begin(),fields_.end(),[&](const auto &row){return row.name.ends_with(suffix);}); check(found!=fields_.end()); SelectField(static_cast<int>(found-fields_.begin()));};
    choose(42); Action(FitScene); check(scene_view_->Projection().Items().size()==count);
    SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0); check(local_error_.empty()); idle("before_cycles");
    measure("render_frame_ms",[&]{SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0);},10);
    std::uint64_t picked=0; measure("native_pick_ms",[&]{picked=scene_view_->Projection().Pick(scene_view_->Projection().ToScreen({-6,0})).Raw();},1000); check(picked!=0);
    const auto baseline=workspace_.Project()->Snapshot(); const auto epoch=workspace_.Project()->Revision();
    const auto mouse=[&](UINT message,ed::ViewPoint point){SendMessageW(ControlHandle(Viewport),message,0,MAKELPARAM(static_cast<int>(std::lround(point.x*dpi_/96)),static_cast<int>(std::lround(point.y*dpi_/96))));};
    const auto &projection=scene_view_->Projection(); auto gizmo=ed::MakeGizmo(projection,projection.Items().front().world,ed::GizmoSpace::World);
    mouse(WM_LBUTTONDOWN,gizmo.x); int moves=0;
    measure("native_drag_preview_ms",[&]{++moves; mouse(WM_MOUSEMOVE,{gizmo.x.x+moves*.5,gizmo.x.y});},60);
    check(workspace_.Project()->Snapshot()==baseline && workspace_.Project()->Revision()==epoch);
    measure("native_drag_commit_ms",[&]{mouse(WM_LBUTTONUP,{gizmo.x.x+30,gizmo.x.y});});
    check(workspace_.Project()->Revision()==epoch+1); measure("native_undo_ms",[&]{Action(Undo);}); check(workspace_.Project()->Snapshot()==baseline);
    measure("native_redo_ms",[&]{Action(Redo);}); Action(Undo); check(!workspace_.Project()->IsDirty());
    for(const auto message:{WM_CANCELMODE,WM_CAPTURECHANGED,WM_KILLFOCUS,WM_RBUTTONDOWN}){
        gizmo=ed::MakeGizmo(scene_view_->Projection(),scene_view_->Projection().Items().front().world,ed::GizmoSpace::World);
        mouse(WM_LBUTTONDOWN,gizmo.x); mouse(WM_MOUSEMOVE,{gizmo.x.x+30,gizmo.x.y}); SendMessageW(ControlHandle(Viewport),static_cast<UINT>(message),0,0);
        check(workspace_.Project()->Snapshot()==baseline && GetCapture()!=ControlHandle(Viewport));
    }
    choose(0); const auto long_name=std::string(220,'W')+" / Arena ÐŸÑ€Ð¸Ð¼ÐµÑ€"; field("Document");
    SetText(ControlHandle(Value),long_name); Action(Apply); check(!Draft()); field("Document"); check(Text(ControlHandle(Value))==long_name);
    const auto authored=workspace_.Project()->Snapshot(); const auto author_epoch=workspace_.Project()->Revision();
    SendMessageW(ControlHandle(Value),EM_SETSEL,0,-1); SendMessageW(ControlHandle(Value),EM_REPLACESEL,TRUE,reinterpret_cast<LPARAM>(L"unapplied edit"));
    check(Draft()); SendMessageW(ControlHandle(Value),WM_UNDO,0,0); check(!Draft() && workspace_.Project()->Snapshot()==authored && workspace_.Project()->Revision()==author_epoch);
    const auto actual_dpi=GetDpiForWindow(hwnd_);
    for(const auto dpi:{96u,144u}){
        Fonts(dpi); SetWindowPos(hwnd_,nullptr,0,0,MulDiv(1000,static_cast<int>(dpi),96),MulDiv(800,static_cast<int>(dpi),96),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); Layout(); SyncScene();
        SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0); check(local_error_.empty());
        HWND next=GetNextDlgTabItem(hwnd_,nullptr,FALSE); check(next!=nullptr && IsChild(hwnd_,next));
    }
    SetWindowPos(hwnd_,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    Fonts(actual_dpi); SetWindowPos(hwnd_,nullptr,0,0,1280,850,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE); Layout(); SyncScene();
    SendMessageW(ControlHandle(Viewport),WM_PAINT,0,0); check(local_error_.empty());
    measure("native_save_ms",[&]{Action(Save);}); check(!workspace_.Project()->IsDirty());
    const int cycles=count==200?10:3;
    for(int i=0;i<cycles;++i){
        measure("native_open_ms",[&]{ex::Take(workspace_.Open(file)); Refresh(true);}); choose(42);
        measure("native_play_ms",[&]{Action(Play);}); check(timer_running_ && workspace_.Session()->IsPlaying());
        SetText(ControlHandle(Payload),"1"); Action(SendEvent); ex::Take(workspace_.Tick({})); const auto waiting=workspace_.Activations().back(); check(!waiting->IsFinished());
        measure("native_stop_ms",[&]{Action(Stop);}); const auto report=waiting->Report(); check(waiting->IsFinished() && report && report->status==graph::ExecutionStatus::Cancelled && !timer_running_);
        check(!workspace_.Project()->IsDirty()); memory("cycle");
    }
    idle("after_cycles");
    Action(Play); SetText(ControlHandle(Payload),"1"); Action(SendEvent); ex::Take(workspace_.Tick({})); const auto waiting=workspace_.Activations().back();
    SendMessageW(hwnd_,WM_CLOSE,0,0); events.PumpEvents(); const auto report=waiting->Report();
    check(closed_ && waiting->IsFinished() && report && report->status==graph::ExecutionStatus::Cancelled && !timer_running_ && !workspace_.Project()->IsDirty());
    std::puts("PASS: native bounded load, idle/lifecycle resources, render/pick/drag, long text, DPI/layout, text Undo and close during Play");
}
} // namespace
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    try {
        int argc=0;
        const auto release=[](wchar_t **pointer) { LocalFree(static_cast<HLOCAL>(pointer)); };
        const std::unique_ptr<wchar_t *,decltype(release)> parsed{CommandLineToArgvW(GetCommandLineW(),&argc),release};
        auto **argv=parsed.get();
        if (!argv) return 1;
        std::vector<std::wstring> args; for (int i=1;i<argc;++i) args.emplace_back(argv[i]);
        const bool content_smoke=args.size()==2 && args[0]==L"--content-smoke";
        const bool stress=args.size()==2 && (args[0]==L"--stress-smoke" || args[0]==L"--stress-diagnostic");
        const bool demo_smoke=args.size()==2 && args[0]==L"--demo-smoke";
        const bool smoke=content_smoke || stress || demo_smoke || (args.size()==2 && args[0]==L"--ui-smoke");
        if ((!args.empty() && !smoke && args.size()!=1) || (!args.empty() && args[0].starts_with(L"--") && !smoke)) throw std::runtime_error("usage: EpidemicEditorDesktop [project.epidemic] | --ui-smoke|--demo-smoke|--stress-smoke|--stress-diagnostic <project.epidemic>");
        platform::WindowsPlatformRuntime platform; ex::Schema schema;
        auto window=ex::Take(platform.CreateWindow({"Epidemic Editor",1280,850,!smoke}));
        {
            Desktop editor{window,schema};
            if (content_smoke) editor.ContentSmoke(std::filesystem::path{args[1]});
            else if (stress) editor.StressSmoke(std::filesystem::path{args[1]},platform,args[0]==L"--stress-diagnostic"?1000:200);
            else if (demo_smoke) editor.DemoSmoke(std::filesystem::path{args[1]},platform);
            else if (smoke) editor.Smoke(std::filesystem::path{args[1]},platform);
            else {
                if (!args.empty()) editor.Load(std::filesystem::path{args[0]});
                const auto active=[&] { return !editor.Closed() && !window->IsCloseRequested() && !platform.IsExitRequested(); };
                while (active()) {
                    platform.PumpEvents(); (void)platform.DrainEvents();
                    if (active()) MsgWaitForMultipleObjectsEx(0,nullptr,INFINITE,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
                }
            }
        }
        window->Close(); platform.Shutdown(); return 0;
    } catch (const std::exception &error) {
        // CI smoke failures use a deterministic nonzero exit, never a modal dialog.
        const std::wstring_view command{GetCommandLineW()};
        if (command.find(L"--ui-smoke")==std::wstring_view::npos && command.find(L"--demo-smoke")==std::wstring_view::npos && command.find(L"--stress-smoke")==std::wstring_view::npos && command.find(L"--stress-diagnostic")==std::wstring_view::npos) MessageBoxW(nullptr,Wide(error.what()).c_str(),L"Epidemic Editor",MB_OK | MB_ICONERROR);
        std::fprintf(stderr,"%s\n",error.what()); OutputDebugStringA(error.what()); return 1;
    }
}
