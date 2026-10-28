#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <wincodec.h>
#include <wrl/client.h>
// Win32's convenience macro collides with the existing IWindowSystem API.
#undef CreateWindow
#include "Epidemic/Platform/windows_platform_runtime.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "windows_scene_view.h"
#include "../example/demo_game.h"
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
    Palette, Source, Destination, Connect, AddHealth, AddMovement, AttachBehavior, Place, Prefabs,
    Status, Log, Hint, DocumentsLabel, PropertiesLabel, ToolsLabel, EventLabel, NodeLabel, FromLabel, ToLabel, PrefabLabel,
    ValueLabel, BehaviorLabel, BehaviorGraph, FollowPrefab, Context, ToolsHint, ProjectLabel, HistoryLabel, PreviewLabel,
    ParentLabel, SceneParents, Reparent, Viewport, ViewLabel, FrameSelection, FitScene,
    ToolMode, CoordinateSpace, SnapLabel, SnapValue, SetSnap };
struct Field {
    std::string name, value;
    std::function<Result<ed::Command>(std::string_view)> command;
};
class Desktop final {
  public:
    Desktop(platform::WindowPtr window, ex::Schema &schema)
        : window_(std::move(window)), schema_(schema), workspace_(schema.View(), [&schema] { return ex::Options(schema); }),
          hwnd_(window_->GetNativeHandle().As<HWND>()) {
        INITCOMMONCONTROLSEX common{sizeof(common), ICC_TREEVIEW_CLASSES};
        if (!InitCommonControlsEx(&common) || !SetWindowSubclass(hwnd_, &ParentProc, 1, reinterpret_cast<DWORD_PTR>(this)))
            throw std::runtime_error("Cannot initialize Editor window");
        try {
            Controls();
            scene_view_=std::make_unique<ed::NativeSceneView>(ControlHandle(Viewport),workspace_,[this](SceneNodeId node) { return PickScene(node); },
                [this](std::string error) { local_error_=std::move(error); Diagnostics(); },[this] { Refresh(); });
            Fonts(GetDpiForWindow(hwnd_)); Layout(); Refresh();
        }
        catch (...) { if (font_) DeleteObject(font_); if (heading_font_) DeleteObject(heading_font_); RemoveWindowSubclass(hwnd_, &ParentProc, 1); throw; }
    }
    ~Desktop() {
        KillTimer(hwnd_, 1);
        scene_view_.reset();
        for (int id=New;id<=SetSnap;++id) if (const auto child=ControlHandle(id)) { SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE); RemoveWindowSubclass(child,&ChildProc,1); }
        if (font_) DeleteObject(font_);
        if (heading_font_) DeleteObject(heading_font_);
        RemoveWindowSubclass(hwnd_, &ParentProc, 1);
    }
    bool Closed() const noexcept { return closed_; }
    void Load(const std::filesystem::path &file) { (void)workspace_.Open(file); Refresh(true); }
    void Smoke(const std::filesystem::path &file, platform::IPlatformRuntime &events);
  private:
    HWND ControlHandle(int id) const { return GetDlgItem(hwnd_, id); }
    HWND Make(int id, const wchar_t *kind, const wchar_t *label, DWORD style = 0) {
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
    void Refresh(bool new_project=false);
    void Inspector();
    void Diagnostics();
    void SyncScene(bool reset=false);
    bool PickScene(SceneNodeId node);
    int Shortcut(HWND source, WPARAM key) const noexcept;
    void SelectField(int index);
    bool Draft() const { return field_ >= 0 && static_cast<std::size_t>(field_) < fields_.size() && Text(ControlHandle(Value)) != fields_[static_cast<std::size_t>(field_)].value; }
    bool ApplyDraft();
    bool SaveProject();
    bool CanReplace();
    std::optional<std::filesystem::path> Choose(bool save);
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
        const auto text=field.name+" = "+field.value+(field.command?"":"  [read only]");
        Item(ControlHandle(Fields),text);
        MeasureField(text);
        fields_.push_back(std::move(field));
    }
    void MeasureField(std::string_view text) {
        const auto wide=Wide(text); const auto dc=GetDC(ControlHandle(Fields));
        if (dc) {
            const auto old=SelectObject(dc,font_); SIZE extent{};
            if (GetTextExtentPoint32W(dc,wide.data(),static_cast<int>(wide.size()),&extent)) {
                const auto width=SendMessageW(ControlHandle(Fields),LB_GETHORIZONTALEXTENT,0,0);
                SendMessageW(ControlHandle(Fields),LB_SETHORIZONTALEXTENT,static_cast<WPARAM>(std::max(width,static_cast<LRESULT>(extent.cx+16))),0);
            }
            SelectObject(dc,old); ReleaseDC(ControlHandle(Fields),dc);
        }
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
    HFONT font_=nullptr, heading_font_=nullptr;
    UINT dpi_=96;
    std::vector<ed::UiSelection> selections_;
    std::vector<HTREEITEM> tree_items_;
    std::vector<Field> fields_;
    std::vector<graph::ElementId> outputs_, inputs_;
    std::vector<AssetId> prefabs_, graphs_;
    std::vector<SceneNodeId> parents_;
    std::uint64_t view_revision_ = 0, next_ = 0;
    std::string nonce_ = Number(static_cast<std::uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    std::string local_error_, log_text_;
    int field_ = -1;
    bool refreshing_ = false, closed_ = false, timer_running_ = false, scene_document_=false;
};
void Desktop::Controls() {
    for (const auto &[id, label] : std::array<std::pair<int, const wchar_t *>, 8>{{
        {New,L"New example"}, {Open,L"Open..."}, {Save,L"Save"}, {Undo,L"Undo"}, {Redo,L"Redo"},
        {Play,L"Play (F5)"}, {Stop,L"Stop"}, {SendEvent,L"Send hit"}}}) Make(id,L"BUTTON",label,WS_TABSTOP);
    Make(EventLabel,L"STATIC",L"Payload / wait ticks"); Make(Payload,L"EDIT",L"10",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    Make(ProjectLabel,L"STATIC",L"Project"); Make(HistoryLabel,L"STATIC",L"Edit history"); Make(PreviewLabel,L"STATIC",L"Play preview");
    Make(Context,L"STATIC",L"Open a project or create an example.",SS_NOPREFIX);
    Make(DocumentsLabel,L"STATIC",L"Documents / hierarchy");
    Make(Tree,WC_TREEVIEWW,L"",WS_TABSTOP | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS);
    Make(PropertiesLabel,L"STATIC",L"Authoring properties");
    Make(Fields,L"LISTBOX",L"",WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT);
    Make(ValueLabel,L"STATIC",L"Value");
    Make(Value,L"EDIT",L"",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    Make(Apply,L"BUTTON",L"Apply",WS_TABSTOP); Make(Revert,L"BUTTON",L"Revert field",WS_TABSTOP);
    Make(Hint,L"STATIC",L"Choose a property. Enter a typed value, then Apply.\nPlay runs an isolated snapshot. Save writes authoring data.");
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
    Make(AddHealth,L"BUTTON",L"Add Health",WS_TABSTOP); Make(AddMovement,L"BUTTON",L"Add Movement",WS_TABSTOP);
    Make(BehaviorLabel,L"STATIC",L"Graph asset for hit behavior");
    Make(BehaviorGraph,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(AttachBehavior,L"BUTTON",L"Assign hit GraphBehavior",WS_TABSTOP);
    Make(PrefabLabel,L"STATIC",L"Prefab asset for Scene placement");
    Make(Prefabs,L"COMBOBOX",L"",WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    Make(Place,L"BUTTON",L"Assign Prefab",WS_TABSTOP);
    Make(FollowPrefab,L"BUTTON",L"Open Prefab definition",WS_TABSTOP);
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
    Make(Log,L"EDIT",L"",WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL);
}
void Desktop::Fonts(UINT dpi) {
    NONCLIENTMETRICSW metrics{}; metrics.cbSize=sizeof(metrics);
    if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0,dpi)) throw std::runtime_error("Cannot query UI font");
    metrics.lfMessageFont.lfHeight=-MulDiv(10,static_cast<int>(dpi),72);
    const auto normal=CreateFontIndirectW(&metrics.lfMessageFont);
    metrics.lfMessageFont.lfWeight=FW_SEMIBOLD;
    const auto heading=CreateFontIndirectW(&metrics.lfMessageFont);
    if (!normal || !heading) { if (normal) DeleteObject(normal); if (heading) DeleteObject(heading); throw std::runtime_error("Cannot create UI font"); }
    for (int id=New;id<=SetSnap;++id) {
        const bool title=id==DocumentsLabel || id==PropertiesLabel || id==ToolsLabel || id==ProjectLabel || id==HistoryLabel || id==PreviewLabel || id==ViewLabel;
        SendMessageW(ControlHandle(id),WM_SETFONT,reinterpret_cast<WPARAM>(title?heading:normal),TRUE);
    }
    if (font_) DeleteObject(font_);
    if (heading_font_) DeleteObject(heading_font_);
    font_=normal; heading_font_=heading; dpi_=dpi;
    SendMessageW(ControlHandle(Fields),LB_SETHORIZONTALEXTENT,0,0);
    for (const auto &field:fields_) MeasureField(field.name+" = "+field.value+(field.command?"":"  [read only]"));
}
void Desktop::Layout() {
    RECT rect{}; GetClientRect(hwnd_, &rect);
    const int dpi = static_cast<int>(dpi_);
    const auto px = [dpi](int value) { return MulDiv(value, dpi, 96); };
    const int width = MulDiv(rect.right,96,dpi), height = MulDiv(rect.bottom,96,dpi);
    const auto move = [&](int id,int x,int y,int w,int h) { MoveWindow(ControlHandle(id),px(x),px(y),px(w),px(h),TRUE); };
    move(ProjectLabel,12,8,290,20); move(HistoryLabel,326,8,164,20); move(PreviewLabel,512,8,280,20);
    move(New,12,32,116,30); move(Open,134,32,80,30); move(Save,220,32,80,30);
    move(Undo,326,32,76,30); move(Redo,408,32,76,30);
    move(Play,512,32,90,30); move(Stop,608,32,70,30); move(SendEvent,684,32,90,30);
    move(EventLabel,792,8,width-804,20); move(Payload,792,32,100,30);
    move(Context,12,76,width-24,42);
    const int left=(width-48)*28/100, mid=(width-left-48)/2, right=left+mid+36;
    const int bottom=height-164;
    move(DocumentsLabel,12,124,left,22); move(PropertiesLabel,left+24,124,mid,22); move(ToolsLabel,right,124,width-right-12,22);
    move(Tree,12,152,left,bottom-198);
    move(NewScene,12,bottom-34,left/3-4,30); move(NewPrefab,16+left/3,bottom-34,left/3-4,30); move(NewGraph,20+2*left/3,bottom-34,left/3-8,30);
    move(Fields,left+24,152,mid,bottom-328); move(ValueLabel,left+24,bottom-166,mid,22); move(Value,left+24,bottom-140,mid,28);
    move(Apply,left+24,bottom-104,80,30); move(Revert,left+110,bottom-104,110,30);
    move(Hint,left+24,bottom-64,mid,64);
    if (scene_document_) {
        const int view_height=std::max(140,(bottom-152)*46/100);
        move(ViewLabel,left+24,124,mid,22);
        move(ToolMode,left+24,152,130,160); move(CoordinateSpace,left+160,152,mid-136,130);
        move(FrameSelection,left+24,188,140,28); move(FitScene,left+170,188,90,28);
        move(Viewport,left+24,224,mid,view_height-36);
        move(PropertiesLabel,left+24,200+view_height,mid,22);
        move(Fields,left+24,228+view_height,mid,std::max(36,bottom-336-view_height));
        move(ValueLabel,left+24,bottom-104,mid,22); move(Value,left+24,bottom-80,mid,28);
        move(Apply,left+24,bottom-44,80,30); move(Revert,left+110,bottom-44,110,30);
    }
    const int rw=width-right-10;
    for (int id:{Palette,Source,Destination,BehaviorGraph,Prefabs,SceneParents}) SendMessageW(ControlHandle(id),CB_SETDROPPEDWIDTH,static_cast<WPARAM>(px(std::max(rw,650))),0);
    move(Add,right,152,rw/2-3,30); move(Remove,right+rw/2+3,152,rw/2-3,30);
    move(NodeLabel,right,198,rw,22); move(Palette,right,224,rw,240);
    move(FromLabel,right,270,rw,22); move(Source,right,296,rw,280);
    move(ToLabel,right,340,rw,22); move(Destination,right,366,rw,280); move(Connect,right,408,rw,30);
    move(AddHealth,right,198,rw/2-3,30); move(AddMovement,right+rw/2+3,198,rw/2-3,30);
    move(BehaviorLabel,right,250,rw,22); move(BehaviorGraph,right,276,rw,240); move(AttachBehavior,right,316,rw,30);
    move(PrefabLabel,right,198,rw,22); move(Prefabs,right,224,rw,240); move(Place,right,264,rw,30); move(FollowPrefab,right,310,rw,30);
    move(ParentLabel,right,366,rw,22); move(SceneParents,right,392,rw,240); move(Reparent,right,430,rw,30);
    move(ToolsHint,right,466,rw,std::max(70,bottom-466));
    if (scene_document_) {
        move(SnapLabel,right,466,rw,22); move(SnapValue,right,492,90,28); move(SetSnap,right+96,492,96,28);
        move(ToolsHint,right,532,rw,std::max(30,bottom-532));
    }
    move(Status,12,bottom+8,width-24,24); move(Log,12,bottom+38,width-24,116);
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
    std::set<decltype(SelectionKey(ed::UiSelection{}))> expanded;
    // Common-controls can return additional state bits despite the requested mask.
    for (std::size_t i=0;i<tree_items_.size();++i) if ((TreeView_GetItemState(ControlHandle(Tree),tree_items_[i],TVIS_EXPANDED)&static_cast<UINT>(TVIS_EXPANDED))!=0) expanded.insert(SelectionKey(selections_[i]));
    const bool initial=new_project || tree_items_.empty();
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
        SetText(hwnd_,"Epidemic Editor / "+Utf8(project->File().filename().wstring())+(project->IsDirty()?" *":""));
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
    const auto *project=workspace_.Project();
    const auto snapshot=project?project->Snapshot():nullptr;
    const auto selection=workspace_.Selection(); const auto *doc=snapshot?snapshot->Find(selection.document):nullptr;
    if (!doc) { EnableWindow(ControlHandle(Value),FALSE); return; }
    const auto asset=doc->id;
    AddField({"Document",doc->name,[asset](std::string_view text) { return Result<ed::Command>::Success(ed::RenameDocument{asset,std::string{text}}); }});
    AddField({"Asset ID",Number(asset.value.Raw()),{}});
    if (const auto *prefab=std::get_if<comp::PrefabDefinition>(&doc->data)) {
        if (const auto *node=Find(prefab->root,selection.element)) {
            AddField({"Entity ID",Number(node->id.Raw()),{}});
            for (const auto &component:node->components) {
                if (selection.kind==ed::UiElement::PrefabComponent && (component.type!=selection.component || component.part!=selection.part)) continue;
                const auto *info=schema_.components.Find(component.type); if (!info) continue;
                if (selection.kind==ed::UiElement::PrefabComponent) { AddField({"Component type ID",Number(component.type.Raw()),{}}); AddField({"Part ID",Number(component.part.Raw()),{}}); }
                for (const auto &property:component.properties) {
                    const auto metadata=std::find_if(info->properties.begin(),info->properties.end(),[&](const auto &p){return p.id==property.property;});
                    if (metadata==info->properties.end()) continue;
                    Field field{info->name+" / "+metadata->name,ed::EditorWorkspace::FormatValue(property.value),{}};
                    if (property.parameter.Raw()==0) field.command=[asset,node_id=node->id,type=component.type,part=component.part,id=property.property,kind=metadata->kind](std::string_view text) {
                        auto value=ed::EditorWorkspace::ParseValue(kind,text);
                        if (!value) return Result<ed::Command>::Failure(value.GetError());
                        return Result<ed::Command>::Success(ed::SetProperty{asset,node_id,type,part,{id,std::move(value).Value(),{}}});
                    };
                    AddField(std::move(field));
                }
            }
            for (const auto &behavior:node->behaviors) AddField({"GraphBehavior",Number(behavior.graph.Raw()),{}});
        }
        for (const auto &parameter:prefab->parameters) AddField({"Parameter "+Number(parameter.id.Raw()),ed::EditorWorkspace::FormatValue(parameter.value),{}});
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
            for (const auto &placement:scene->placements) if (placement.node==node.id) AddField({"Prefab asset",Number(placement.prefab.value.Raw()),{}});
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
    if (!fields_.empty()) SelectField(0);
}
void Desktop::SelectField(int index) {
    field_=index; SendMessageW(ControlHandle(Fields),LB_SETCURSEL,static_cast<WPARAM>(index),0);
    const bool valid=index>=0 && static_cast<std::size_t>(index)<fields_.size();
    EnableWindow(ControlHandle(Value),valid && static_cast<bool>(fields_[static_cast<std::size_t>(index)].command));
    SetText(ControlHandle(Value),valid?fields_[static_cast<std::size_t>(index)].value:"");
}
bool Desktop::ApplyDraft() {
    if (!Draft()) return true;
    const auto &field=fields_[static_cast<std::size_t>(field_)]; if (!field.command) return false;
    auto command=field.command(Text(ControlHandle(Value)));
    if (!command) { local_error_=command.GetError().code+": "+command.GetError().message; Diagnostics(); SetFocus(ControlHandle(Value)); return false; }
    const int selected=field_; if (!Execute({std::move(command).Value()})) { SetFocus(ControlHandle(Value)); return false; }
    SelectField(selected); local_error_.clear(); Diagnostics(); return true;
}
bool Desktop::SaveProject() { if (!ApplyDraft()) return false; const auto saved=workspace_.Save(); if (saved) Refresh(); else Diagnostics(); return static_cast<bool>(saved); }
bool Desktop::CanReplace() {
    if (!Draft() && (!workspace_.Project() || !workspace_.Project()->IsDirty())) return true;
    const auto answer=MessageBoxW(hwnd_,L"Save changes before leaving this project?",L"Unsaved authoring data",MB_YESNOCANCEL | MB_ICONQUESTION);
    return answer==IDNO || (answer==IDYES && SaveProject());
}
std::optional<std::filesystem::path> Desktop::Choose(bool save) {
    std::array<wchar_t,32768> file{};
    OPENFILENAMEW dialog{}; dialog.lStructSize=sizeof(dialog); dialog.hwndOwner=hwnd_;
    dialog.lpstrFilter=L"Epidemic project (*.epidemic)\0*.epidemic\0\0"; dialog.lpstrFile=file.data(); dialog.nMaxFile=static_cast<DWORD>(file.size());
    dialog.lpstrDefExt=L"epidemic"; dialog.Flags=OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
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
    if (id==Apply) { (void)ApplyDraft(); return; }
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
    if (id==Stop) { (void)workspace_.Stop(); Diagnostics(); return; }
    if (!ApplyDraft()) return;
    local_error_.clear();
    if (id==Undo) { (void)workspace_.Undo(); Refresh(); return; }
    if (id==Redo) { (void)workspace_.Redo(); Refresh(); return; }
    if (id==Play) { (void)workspace_.Play(); Diagnostics(); return; }
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
    } else if ((id==AddHealth || id==AddMovement) && selection.kind==ed::UiElement::PrefabNode) {
        const auto *node=Find(std::get<comp::PrefabDefinition>(doc->data).root,selection.element); if (!node) return;
        const auto component=ex::Enemy(schema_).components[id==AddHealth?0:1];
        const auto exists=std::any_of(node->components.begin(),node->components.end(),[&](const auto &item){return item.type==component.type;});
        if (exists) { local_error_="Component already exists; edit its properties."; Diagnostics(); }
        else (void)Execute({ed::SetComponent{doc->id,node->id,component}});
    } else if (id==AttachBehavior && selection.kind==ed::UiElement::PrefabNode) {
        const auto index=SendMessageW(ControlHandle(BehaviorGraph),CB_GETCURSEL,0,0);
        if (index<0 || static_cast<std::size_t>(index)>=graphs_.size()) return;
        const auto *graph_doc=snapshot->Find(graphs_[static_cast<std::size_t>(index)]);
        if (!graph_doc) return;
        const auto &graph=std::get<graph::GraphDefinition>(graph_doc->data);
        graph::ElementId event{};
        for (const auto &node:graph.nodes) for (const auto &port:node.ports) if (port.kind==graph::PortKind::Event && port.direction==graph::PortDirection::Input && !event.Raw()) event=port.id;
        if (!event.Raw()) { local_error_="Graph has no event input. Add an Event entry first."; Diagnostics(); return; }
        const auto *node=Find(std::get<comp::PrefabDefinition>(doc->data).root,selection.element); if (!node) return;
        auto behavior_id=Fresh();
        for (const auto &behavior:node->behaviors) if (std::any_of(behavior.events.begin(),behavior.events.end(),[](const auto &route){return route.event==ex::Id("hit");})) { behavior_id=behavior.id; break; }
        (void)Execute({ed::SetBehavior{doc->id,node->id,{},{},{behavior_id,graph_doc->id.value,{{ex::Id("hit"),event}}, {}}}});
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
    SetText(ControlHandle(Status),std::string{playing?"PLAYING SNAPSHOT | ":"AUTHORING | "}+(project?(project->IsDirty()?"Unsaved changes":"Saved"):"Open a project or create an example")+(Draft()?" | Unapplied field":""));
    EnableWindow(ControlHandle(Save),project!=nullptr); EnableWindow(ControlHandle(Undo),project && project->CanUndo()); EnableWindow(ControlHandle(Redo),project && project->CanRedo());
    EnableWindow(ControlHandle(Play),project && !playing); EnableWindow(ControlHandle(Stop),playing); EnableWindow(ControlHandle(SendEvent),playing);
    const auto selection=workspace_.Selection(); const auto snapshot=project?project->Snapshot():nullptr; const auto *doc=snapshot?snapshot->Find(selection.document):nullptr;
    const bool graph=doc && std::holds_alternative<graph::GraphDefinition>(doc->data), prefab=selection.kind==ed::UiElement::PrefabNode;
    const bool prefab_doc=doc && std::holds_alternative<comp::PrefabDefinition>(doc->data), scene_doc=doc && std::holds_alternative<gc::SceneCompositionDefinition>(doc->data);
    const auto visible=[&](int id,bool show) { const auto control=ControlHandle(id); if (((static_cast<DWORD_PTR>(GetWindowLongPtrW(control,GWL_STYLE))&static_cast<DWORD_PTR>(WS_VISIBLE))!=0)!=show) ShowWindow(control,show?SW_SHOW:SW_HIDE); };
    for (int id:{NodeLabel,Palette,FromLabel,Source,ToLabel,Destination,Connect}) visible(id,graph);
    for (int id:{AddHealth,AddMovement,BehaviorLabel,BehaviorGraph,AttachBehavior}) visible(id,prefab_doc);
    for (int id:{PrefabLabel,Prefabs,Place,FollowPrefab,ParentLabel,SceneParents,Reparent}) visible(id,scene_doc);
    SetText(ControlHandle(ToolsLabel),graph?"Graph actions":prefab_doc?"Prefab definition":scene_doc?"Scene placement":"Document actions");
    SetText(ControlHandle(Add),graph?"Add node":prefab_doc?"Add Entity":"Add placement");
    SetText(ControlHandle(Remove),selection.kind==ed::UiElement::PrefabComponent?"Delete component":"Delete selection");
    const std::string tool_hint=!doc?"Open a project or create an example.":graph?"Select ports of compatible kind, direction and type. Select a node or link to delete it.":prefab_doc?"Definition data. Child entities express ownership, not spatial parenting. Select an Entity to assign components or behavior.":"Drag axes or Move body. Esc/RMB cancels. Arrows move World XY; Shift x10. Rotate/Scale use Local XY hierarchy. Boxes are placeholders.";
    if (Text(ControlHandle(ToolsHint))!=tool_hint) SetText(ControlHandle(ToolsHint),tool_hint);
    std::string context="Open a project or create an example.";
    if (doc && project) {
        context=std::string{graph?"Graph":prefab_doc?"Prefab":"Scene"}+" / "+doc->name+(project->IsDirty(doc->id)?" *":"");
        for (std::size_t i=0;i<selections_.size();++i) if (selections_[i]==selection) {
            std::array<wchar_t,512> label{}; TVITEMW item{}; item.mask=TVIF_TEXT; item.hItem=tree_items_[i]; item.pszText=label.data(); item.cchTextMax=static_cast<int>(label.size());
            TreeView_GetItem(ControlHandle(Tree),&item); context+="  |  Selected: "+Utf8(label.data()); break;
        }
        context+=playing?"\nEditing authoring data; Play uses its existing snapshot.":"\nAuthoring data. Full stable IDs are available in properties.";
    }
    if (Text(ControlHandle(Context))!=context) SetText(ControlHandle(Context),context);
    for (int id:{NewScene,NewPrefab,NewGraph,Add,Remove}) EnableWindow(ControlHandle(id),project!=nullptr);
    EnableWindow(ControlHandle(Add),doc && selection.kind!=ed::UiElement::PrefabComponent);
    for (int id:{Palette,Source,Destination,Connect}) EnableWindow(ControlHandle(id),graph);
    for (int id:{AddHealth,AddMovement,AttachBehavior,BehaviorGraph}) EnableWindow(ControlHandle(id),prefab);
    EnableWindow(ControlHandle(Place),selection.kind==ed::UiElement::SceneNode); EnableWindow(ControlHandle(Prefabs),selection.kind==ed::UiElement::SceneNode);
    EnableWindow(ControlHandle(FollowPrefab),workspace_.PrefabDefinitionSelection().has_value());
    for (int id:{SceneParents,Reparent}) EnableWindow(ControlHandle(id),selection.kind==ed::UiElement::SceneNode);
    EnableWindow(ControlHandle(Apply),Draft()); EnableWindow(ControlHandle(Revert),Draft());
    std::string log=local_error_;
    if (workspace_.LastError()) { const auto &error=*workspace_.LastError(); log+="\r\nLast command failed: "+error.code+": "+error.message; }
    const std::array<const char *,5> states{"Completed","Budget exhausted","Failed","Waiting","Cancelled"};
    for (const auto &activation:workspace_.Activations()) {
        const auto report=activation->Report(); log+="\r\nActivation "+Number(activation->Id())+" / "+(report?states[static_cast<std::size_t>(report->status)]:"Queued");
        if (report) log+=" / last step instructions "+Number(report->instructions)+" / native calls "+Number(report->native_calls);
        if (report && report->diagnostic) { const auto &d=*report->diagnostic; log+=" / "+d.error.code+" / asset "+Number(d.asset.value.Raw())+" / node "+Number(d.node.Raw())+" / instruction "+Number(d.instruction)+" / "+d.error.message; }
    }
    if (log.empty()) log="Ready. New example includes Scene, Enemy Prefab (Health / Movement) and Event -> Damage -> Wait -> Damage.\r\nSelect an Enemy placement in Scene, Play, then Send hit. Runtime changes are discarded by Stop.";
    if (log!=log_text_) { log_text_=log; SetText(ControlHandle(Log),log); }
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
    else if (source==ControlHandle(Value)) {
        if (key==VK_RETURN) return Apply;
        if (key==VK_ESCAPE) return Revert;
    }
    else if (source==ControlHandle(SnapValue) && key==VK_RETURN) return SetSnap;
    return 0;
}
LRESULT CALLBACK Desktop::ParentProc(HWND,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) noexcept {
    auto &self=*reinterpret_cast<Desktop *>(data);
    try { return self.Message(msg,wp,lp); }
    catch (const std::exception &) { SetWindowTextW(self.ControlHandle(Log),L"Editor operation failed; authoring commands remain atomic."); return 0; }
    catch (...) { SetWindowTextW(self.ControlHandle(Log),L"Unexpected Editor error"); return 0; }
}
LRESULT CALLBACK Desktop::ChildProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) noexcept {
    auto &self=*reinterpret_cast<Desktop *>(data);
    if (msg==WM_KEYDOWN) {
        const int action=self.Shortcut(hwnd,wp);
        if (wp==VK_TAB) { const auto next=GetNextDlgTabItem(self.hwnd_,hwnd,GetKeyState(VK_SHIFT)<0); if (next) SetFocus(next); return 0; }
        if (action) { SendMessageW(self.hwnd_,WM_COMMAND,static_cast<WPARAM>(action),0); return 0; }
    }
    return DefSubclassProc(hwnd,msg,wp,lp);
}
LRESULT Desktop::Message(UINT msg,WPARAM wp,LPARAM lp) {
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
        if (id==Fields && notification==LBN_SELCHANGE) {
            const auto index=static_cast<int>(SendMessageW(ControlHandle(Fields),LB_GETCURSEL,0,0));
            if (!Draft()) SelectField(index);
            else { SendMessageW(ControlHandle(Fields),LB_SETCURSEL,static_cast<WPARAM>(field_),0); local_error_="Apply or Revert the edited field before changing selection."; }
            Diagnostics();
        } else if (id==Value && notification==EN_CHANGE) Diagnostics();
        else if (notification==BN_CLICKED || ((id==ToolMode || id==CoordinateSpace) && notification==CBN_SELCHANGE)) Action(id);
        return 0;
    }
    if (msg==WM_NOTIFY && !refreshing_) {
        const auto *notice=reinterpret_cast<NMHDR *>(lp);
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
    check(workspace_.Activations().size()==1); const auto activation=workspace_.Activations().front(); check(!activation->IsFinished());
    SendMessageW(hwnd_,WM_COMMAND,Stop,0); check(activation->IsFinished());
    ex::Take(workspace_.Open(file)); Refresh(); check(!workspace_.Project()->IsDirty());
    (void)workspace_.Select({AssetId::FromString("example.prefab"),ed::UiElement::PrefabNode,ex::Id("enemy").Raw()}); Refresh();
    check(std::any_of(fields_.begin(),fields_.end(),[](const auto &field){return field.name.ends_with(" / points") && field.value=="87";}));
    // Exercise the supported minimum client layout without touching any other window.
    const auto actual_dpi=GetDpiForWindow(hwnd_);
    choose({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42});
    for (const auto dpi:{96u,144u}) {
        Fonts(dpi);
        SetWindowPos(hwnd_,nullptr,0,0,MulDiv(1000,static_cast<int>(dpi),96),MulDiv(800,static_cast<int>(dpi),96),SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); Layout();
        RECT client{}; GetClientRect(hwnd_,&client);
        SyncScene();
        for (int id:{Tree,Fields,Value,FollowPrefab,Connect,Place,ToolsHint,Log,Viewport,FrameSelection,FitScene}) {
            RECT bounds{}; GetWindowRect(ControlHandle(id),&bounds);
            POINT first{bounds.left,bounds.top},last{bounds.right,bounds.bottom}; ScreenToClient(hwnd_,&first); ScreenToClient(hwnd_,&last);
            check(first.x>=0 && first.y>=0 && last.x<=client.right && last.y<=client.bottom && last.y>first.y);
        }
        LOGFONTW font{}; check(GetObjectW(font_,sizeof(font),&font)!=0 && font.lfHeight==-MulDiv(10,static_cast<int>(dpi),72));
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
} // namespace
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    try {
        int argc=0;
        const auto release=[](wchar_t **pointer) { LocalFree(static_cast<HLOCAL>(pointer)); };
        const std::unique_ptr<wchar_t *,decltype(release)> parsed{CommandLineToArgvW(GetCommandLineW(),&argc),release};
        auto **argv=parsed.get();
        if (!argv) return 1;
        std::vector<std::wstring> args; for (int i=1;i<argc;++i) args.emplace_back(argv[i]);
        const bool smoke=args.size()==2 && args[0]==L"--ui-smoke";
        if ((!args.empty() && !smoke && args.size()!=1) || (!args.empty() && args[0].starts_with(L"--") && !smoke)) throw std::runtime_error("usage: EpidemicEditorDesktop [project.epidemic] | --ui-smoke <project.epidemic>");
        platform::WindowsPlatformRuntime platform; ex::Schema schema;
        auto window=ex::Take(platform.CreateWindow({"Epidemic Editor",1280,850,!smoke}));
        {
            Desktop editor{window,schema};
            if (smoke) editor.Smoke(std::filesystem::path{args[1]},platform);
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
        if (std::wstring_view{GetCommandLineW()}.find(L"--ui-smoke")==std::wstring_view::npos) MessageBoxW(nullptr,Wide(error.what()).c_str(),L"Epidemic Editor",MB_OK | MB_ICONERROR);
        std::fprintf(stderr,"%s\n",error.what()); OutputDebugStringA(error.what()); return 1;
    }
}
