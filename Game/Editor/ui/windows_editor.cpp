#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
// Win32's convenience macro collides with the existing IWindowSystem API.
#undef CreateWindow
#include "Epidemic/Platform/windows_platform_runtime.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "../example/demo_game.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <limits>

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
const comp::PrefabNode *Find(const comp::PrefabNode &root, std::uint64_t id) {
    if (root.id.Raw() == id) return &root;
    for (const auto &child : root.children) if (const auto *node = Find(child, id)) return node;
    return nullptr;
}
enum Control : int { New = 100, Open, Save, Undo, Redo, Play, Stop, SendEvent, Payload,
    Tree, Fields, Value, Apply, Revert, Add, Remove, NewScene, NewPrefab, NewGraph,
    Palette, Source, Destination, Connect, AddHealth, AddMovement, AttachBehavior, Place, Prefabs,
    Status, Log, Hint, DocumentsLabel, PropertiesLabel, ToolsLabel, EventLabel, NodeLabel, FromLabel, ToLabel, PrefabLabel,
    ValueLabel, BehaviorLabel, BehaviorGraph };
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
        try { Controls(); Layout(); Refresh(); SetTimer(hwnd_, 1, 33, nullptr); }
        catch (...) { RemoveWindowSubclass(hwnd_, &ParentProc, 1); throw; }
    }
    ~Desktop() {
        KillTimer(hwnd_, 1);
        for (int id=New;id<=BehaviorGraph;++id) if (const auto child=ControlHandle(id)) RemoveWindowSubclass(child,&ChildProc,1);
        RemoveWindowSubclass(hwnd_, &ParentProc, 1);
    }
    bool Closed() const noexcept { return closed_; }
    void Load(const std::filesystem::path &file) { (void)workspace_.Open(file); Refresh(); }
    void Smoke(const std::filesystem::path &file);
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
    void Layout();
    void Refresh();
    void Inspector();
    void Diagnostics();
    void SelectField(int index);
    bool Draft() const { return field_ >= 0 && static_cast<std::size_t>(field_) < fields_.size() && Text(ControlHandle(Value)) != fields_[static_cast<std::size_t>(field_)].value; }
    bool ApplyDraft();
    bool SaveProject();
    bool CanReplace();
    std::optional<std::filesystem::path> Choose(bool save);
    void Action(int id);
    bool Execute(std::vector<ed::Command> commands) {
        const auto result = workspace_.Execute(std::move(commands), view_revision_);
        if (result) Refresh(); else Diagnostics(); return static_cast<bool>(result);
    }
    foundation::StringId Fresh() { return ex::Id(nonce_ + "/" + Number(++next_)); }
    HTREEITEM TreeItem(HTREEITEM parent, std::string label, ed::UiSelection selection);
    void PrefabTree(HTREEITEM parent, AssetId doc, const comp::PrefabNode &node);
    void AddField(Field field) {
        const auto text=field.name+" = "+field.value+(field.command?"":"  [read only]");
        Item(ControlHandle(Fields),text);
        const auto wide=Wide(text); const auto dc=GetDC(ControlHandle(Fields));
        if (dc) {
            const auto old=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT)); SIZE extent{};
            if (GetTextExtentPoint32W(dc,wide.data(),static_cast<int>(wide.size()),&extent)) {
                const auto width=SendMessageW(ControlHandle(Fields),LB_GETHORIZONTALEXTENT,0,0);
                SendMessageW(ControlHandle(Fields),LB_SETHORIZONTALEXTENT,static_cast<WPARAM>(std::max(width,static_cast<LRESULT>(extent.cx+16))),0);
            }
            SelectObject(dc,old); ReleaseDC(ControlHandle(Fields),dc);
        }
        fields_.push_back(std::move(field));
    }
    void NewDocument(ed::DocumentKind kind);
    static LRESULT CALLBACK ParentProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) noexcept;
    static LRESULT CALLBACK ChildProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) noexcept;
    LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
    platform::WindowPtr window_;
    ex::Schema &schema_;
    ed::EditorWorkspace workspace_;
    HWND hwnd_;
    std::vector<ed::UiSelection> selections_;
    std::vector<Field> fields_;
    std::vector<graph::ElementId> outputs_, inputs_;
    std::vector<AssetId> prefabs_, graphs_;
    std::uint64_t view_revision_ = 0, next_ = 0;
    std::string nonce_ = Number(static_cast<std::uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    std::string local_error_, log_text_;
    int field_ = -1;
    bool refreshing_ = false, closed_ = false;
};
void Desktop::Controls() {
    for (const auto &[id, label] : std::array<std::pair<int, const wchar_t *>, 8>{{
        {New,L"New example"}, {Open,L"Open..."}, {Save,L"Save"}, {Undo,L"Undo"}, {Redo,L"Redo"},
        {Play,L"Play (F5)"}, {Stop,L"Stop"}, {SendEvent,L"Send hit"}}}) Make(id,L"BUTTON",label,WS_TABSTOP);
    Make(EventLabel,L"STATIC",L"Payload / wait ticks"); Make(Payload,L"EDIT",L"10",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    Make(DocumentsLabel,L"STATIC",L"DOCUMENTS / CONTENT");
    Make(Tree,WC_TREEVIEWW,L"",WS_TABSTOP | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS);
    Make(PropertiesLabel,L"STATIC",L"AUTHORING PROPERTIES");
    Make(Fields,L"LISTBOX",L"",WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT);
    Make(ValueLabel,L"STATIC",L"Value");
    Make(Value,L"EDIT",L"",WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    Make(Apply,L"BUTTON",L"Apply",WS_TABSTOP); Make(Revert,L"BUTTON",L"Revert field",WS_TABSTOP);
    Make(Hint,L"STATIC",L"Choose a property. Enter a typed value, then Apply.\nPlay runs an isolated snapshot. Save writes authoring data.");
    Make(NewScene,L"BUTTON",L"+ Scene",WS_TABSTOP); Make(NewPrefab,L"BUTTON",L"+ Prefab",WS_TABSTOP); Make(NewGraph,L"BUTTON",L"+ Graph",WS_TABSTOP);
    Make(Add,L"BUTTON",L"Add item",WS_TABSTOP); Make(Remove,L"BUTTON",L"Delete item",WS_TABSTOP);
    Make(ToolsLabel,L"STATIC",L"COMPOSITION / GRAPH"); Make(NodeLabel,L"STATIC",L"Graph node template");
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
    Make(Status,L"STATIC",L"");
    Make(Log,L"EDIT",L"",WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL);
}
void Desktop::Layout() {
    RECT rect{}; GetClientRect(hwnd_, &rect);
    const int dpi = static_cast<int>(GetDpiForWindow(hwnd_));
    const auto px = [dpi](int value) { return MulDiv(value, dpi, 96); };
    const int width = MulDiv(rect.right,96,dpi), height = MulDiv(rect.bottom,96,dpi);
    const auto move = [&](int id,int x,int y,int w,int h) { MoveWindow(ControlHandle(id),px(x),px(y),px(w),px(h),TRUE); };
    int x = 10;
    for (int id = New; id <= SendEvent; ++id) { const int w = id == New ? 108 : (id == Open || id == Play || id == SendEvent ? 90 : 64); move(id,x,10,w,28); x += w+6; }
    move(EventLabel,10,47,140,20); move(Payload,155,44,90,25);
    const int left = width / 4, mid = (width-left-40)/2, right = left+mid+30;
    const int bottom = height-190;
    move(DocumentsLabel,10,82,left,20); move(PropertiesLabel,left+20,82,mid,20); move(ToolsLabel,right,82,width-right-10,20);
    move(Tree,10,106,left,bottom-150);
    move(NewScene,10,bottom-34,left/3-4,26); move(NewPrefab,14+left/3,bottom-34,left/3-4,26); move(NewGraph,18+2*left/3,bottom-34,left/3-8,26);
    move(Fields,left+20,106,mid,bottom-246); move(ValueLabel,left+20,bottom-134,mid,20); move(Value,left+20,bottom-106,mid,26);
    move(Apply,left+20,bottom-74,80,28); move(Revert,left+106,bottom-74,110,28);
    move(Hint,left+20,bottom-38,mid,48);
    const int rw=width-right-10;
    move(NodeLabel,right,106,rw,20); move(Palette,right,128,rw,240);
    move(Add,right,162,rw/2-3,28); move(Remove,right+rw/2+3,162,rw/2-3,28);
    move(FromLabel,right,202,rw,20); move(Source,right,224,rw,280);
    move(ToLabel,right,256,rw,20); move(Destination,right,278,rw,280); move(Connect,right,310,rw,28);
    move(AddHealth,right,350,rw/2-3,28); move(AddMovement,right+rw/2+3,350,rw/2-3,28);
    move(BehaviorLabel,right,384,rw,20); move(BehaviorGraph,right,406,rw,240); move(AttachBehavior,right,438,rw,28);
    move(PrefabLabel,right,482,rw,20); move(Prefabs,right,504,rw,240); move(Place,right,536,rw,28);
    move(Status,10,bottom+20,width-20,24); move(Log,10,bottom+48,width-20,132);
}
HTREEITEM Desktop::TreeItem(HTREEITEM parent, std::string label, ed::UiSelection selection) {
    auto text = Wide(label); selections_.push_back(selection);
    TVINSERTSTRUCTW item{}; item.hParent=parent; item.hInsertAfter=TVI_LAST;
    item.item.mask=TVIF_TEXT | TVIF_PARAM; item.item.pszText=text.data(); item.item.lParam=static_cast<LPARAM>(selections_.size()-1);
    const auto result = TreeView_InsertItem(ControlHandle(Tree),&item);
    if (selection == workspace_.Selection()) TreeView_SelectItem(ControlHandle(Tree),result);
    return result;
}
void Desktop::PrefabTree(HTREEITEM parent, AssetId doc, const comp::PrefabNode &node) {
    const auto item = TreeItem(parent,"Entity " + Number(node.id.Raw()),{doc,ed::UiElement::PrefabNode,node.id.Raw()});
    for (const auto &child : node.children) PrefabTree(item,doc,child);
    TreeView_Expand(ControlHandle(Tree),item,TVE_EXPAND);
}
void Desktop::Refresh() {
    refreshing_=true; field_=-1; fields_.clear(); selections_.clear(); outputs_.clear(); inputs_.clear(); prefabs_.clear(); graphs_.clear();
    TreeView_DeleteAllItems(ControlHandle(Tree));
    for (int id : {Source,Destination,Prefabs,BehaviorGraph}) SendMessageW(ControlHandle(id),CB_RESETCONTENT,0,0);
    if (const auto *project=workspace_.Project()) {
        view_revision_=project->Revision(); const auto snapshot=project->Snapshot();
        for (const auto &doc : snapshot->definition.documents) {
            const std::array<const char *,3> kinds{"Scene","Prefab","Graph"};
            const auto root=TreeItem(TVI_ROOT,std::string{kinds[static_cast<std::size_t>(doc.Kind())]}+" / "+doc.name+(project->IsDirty(doc.id)?" *":""),{doc.id});
            if (const auto *scene=std::get_if<gc::SceneCompositionDefinition>(&doc.data)) {
                for (const auto &node:scene->scene.nodes) TreeItem(root,"Placement "+Number(node.id.Raw())+(node.parent.Raw()?" / parent "+Number(node.parent.Raw()):""),{doc.id,ed::UiElement::SceneNode,node.id.Raw()});
            } else if (const auto *prefab=std::get_if<comp::PrefabDefinition>(&doc.data)) {
                PrefabTree(root,doc.id,prefab->root); prefabs_.push_back(doc.id); Item(ControlHandle(Prefabs),doc.name,CB_ADDSTRING);
            } else if (const auto *graph=std::get_if<graph::GraphDefinition>(&doc.data)) {
                graphs_.push_back(doc.id); Item(ControlHandle(BehaviorGraph),doc.name,CB_ADDSTRING);
                for (const auto &node:graph->nodes) TreeItem(root,node.name,{doc.id,ed::UiElement::GraphNode,node.id.Raw()});
                for (const auto &edge:graph->connections) TreeItem(root,"Link "+Number(edge.id.Raw()),{doc.id,ed::UiElement::Connection,edge.id.Raw()});
            }
            TreeView_Expand(ControlHandle(Tree),root,TVE_EXPAND);
        }
        SetText(hwnd_,"Epidemic Editor / "+Utf8(project->File().filename().wstring())+(project->IsDirty()?" *":""));
    } else { SetText(hwnd_,"Epidemic Editor"); view_revision_=0; }
    SendMessageW(ControlHandle(Prefabs),CB_SETCURSEL,0,0);
    SendMessageW(ControlHandle(BehaviorGraph),CB_SETCURSEL,0,0);
    Inspector(); refreshing_=false; Diagnostics();
}
void Desktop::Inspector() {
    fields_.clear(); field_=-1; SendMessageW(ControlHandle(Fields),LB_RESETCONTENT,0,0); SendMessageW(ControlHandle(Fields),LB_SETHORIZONTALEXTENT,0,0); SetText(ControlHandle(Value),"");
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
                const auto *info=schema_.components.Find(component.type); if (!info) continue;
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
            AddField({"Parent ID",Number(node.parent.Raw()),{}});
            const std::array<float,3> position{node.local_transform.position.x,node.local_transform.position.y,node.local_transform.position.z};
            for (std::size_t axis=0;axis<3;++axis) AddField({std::string{"Position "}+"XYZ"[axis],ed::EditorWorkspace::FormatValue(static_cast<double>(position[axis])),[asset,node,axis](std::string_view text) {
                const auto value=ed::EditorWorkspace::ParseValue(comp::PropertyKind::Real,text);
                if (!value) return Result<ed::Command>::Failure(value.GetError());
                const auto real=std::get<double>(value.Value());
                if (std::abs(real)>std::numeric_limits<float>::max()) return Result<ed::Command>::Failure(foundation::Error::Create("editor.ui.position_range","Position exceeds float range"));
                auto transform=node.local_transform;
                if (axis==0) transform.position.x=static_cast<float>(real);
                else if (axis==1) transform.position.y=static_cast<float>(real); else transform.position.z=static_cast<float>(real);
                return Result<ed::Command>::Success(ed::SetSceneTransform{asset,node.id,transform});
            }});
            for (const auto &placement:scene->placements) if (placement.node==node.id) AddField({"Prefab asset",Number(placement.prefab.value.Raw()),{}});
        }
    } else if (const auto *definition=std::get_if<graph::GraphDefinition>(&doc->data)) {
        for (const auto &node:definition->nodes) for (const auto &port:node.ports) {
            const std::array<const char *,3> kinds{"data","control","event"};
            const auto name=node.name+" / "+port.name+" ["+kinds[static_cast<std::size_t>(port.kind)]+"]";
            if (port.direction==graph::PortDirection::Output) { outputs_.push_back(port.id); Item(ControlHandle(Source),name,CB_ADDSTRING); }
            else { inputs_.push_back(port.id); Item(ControlHandle(Destination),name,CB_ADDSTRING); }
            if (node.id.Raw()==selection.element && selection.kind==ed::UiElement::GraphNode) AddField({name,Number(port.id.Raw()),{}});
        }
        for (const auto &edge:definition->connections) if (edge.id.Raw()==selection.element && selection.kind==ed::UiElement::Connection) {
            AddField({"Output ID",Number(edge.output_port.Raw()),{}}); AddField({"Input ID",Number(edge.input_port.Raw()),{}});
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
    const int selected=field_; if (!Execute({std::move(command).Value()})) return false;
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
    if (Execute({ed::CreateDocument{{id,names[static_cast<std::size_t>(kind)],0,std::move(data)}}})) { (void)workspace_.Select({id}); Refresh(); }
}
void Desktop::Action(int id) {
    if (id==Revert) { SelectField(field_); local_error_.clear(); Diagnostics(); return; }
    if (id==Apply) { (void)ApplyDraft(); return; }
    if (id==New || id==Open) {
        if (!CanReplace()) return;
        const auto file=Choose(id==New); if (!file) return;
        const auto result=id==New?workspace_.New(*file,ex::Definition(schema_),true):workspace_.Open(*file,true);
        if (result) { local_error_.clear(); Refresh(); } else Diagnostics(); return;
    }
    if (id==Save) { (void)SaveProject(); return; }
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
    const auto selection=workspace_.Selection(); const auto snapshot=project->Snapshot(); const auto *doc=snapshot->Find(selection.document); if (!doc) return;
    if (id==NewScene || id==NewPrefab || id==NewGraph) { NewDocument(static_cast<ed::DocumentKind>(id-NewScene)); return; }
    if (id==Add) {
        if (doc->Kind()==ed::DocumentKind::Scene) { SceneNodeSnapshot node; node.id=SceneNodeId{Fresh().Raw()}; (void)Execute({ed::AddSceneNode{doc->id,node}}); }
        else if (doc->Kind()==ed::DocumentKind::Prefab) {
            const auto &prefab=std::get<comp::PrefabDefinition>(doc->data);
            auto child=ex::Enemy(schema_,Fresh()); child.behaviors.clear();
            (void)Execute({ed::AddEntity{doc->id,selection.kind==ed::UiElement::PrefabNode?comp::PrefabNodeId{selection.element}:prefab.root.id,std::move(child)}});
        } else {
            const auto palette=SendMessageW(ControlHandle(Palette),CB_GETCURSEL,0,0);
            if (palette<0 || palette>2) return;
            auto node=ex::Graph().nodes[static_cast<std::size_t>(palette)]; node.id=Fresh();
            for (auto &port:node.ports) port.id=Fresh(); (void)Execute({ed::AddGraphNode{doc->id,std::move(node)}});
        }
    } else if (id==Remove) {
        switch (selection.kind) {
        case ed::UiElement::Document: (void)Execute({ed::DeleteDocument{doc->id}}); break;
        case ed::UiElement::SceneNode: (void)Execute({ed::DeleteSceneNode{doc->id,SceneNodeId{selection.element}}}); break;
        case ed::UiElement::PrefabNode: (void)Execute({ed::DeleteEntity{doc->id,comp::PrefabNodeId{selection.element}}}); break;
        case ed::UiElement::GraphNode: (void)Execute({ed::DeleteGraphNode{doc->id,graph::ElementId{selection.element}}}); break;
        case ed::UiElement::Connection: (void)Execute({ed::DisconnectPorts{doc->id,graph::ElementId{selection.element}}}); break;
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
    const auto *project=workspace_.Project(); const auto *session=workspace_.Session(); const bool playing=session && session->IsPlaying();
    SetText(ControlHandle(Status),std::string{playing?"PLAYING SNAPSHOT | ":"AUTHORING | "}+(project?(project->IsDirty()?"Unsaved changes":"Saved"):"Open a project or create an example")+(Draft()?" | Unapplied field":""));
    EnableWindow(ControlHandle(Save),project!=nullptr); EnableWindow(ControlHandle(Undo),project && project->CanUndo()); EnableWindow(ControlHandle(Redo),project && project->CanRedo());
    EnableWindow(ControlHandle(Play),project && !playing); EnableWindow(ControlHandle(Stop),playing); EnableWindow(ControlHandle(SendEvent),playing);
    const auto selection=workspace_.Selection(); const auto snapshot=project?project->Snapshot():nullptr; const auto *doc=snapshot?snapshot->Find(selection.document):nullptr;
    const bool graph=doc && doc->Kind()==ed::DocumentKind::Graph, prefab=selection.kind==ed::UiElement::PrefabNode;
    for (int id:{NewScene,NewPrefab,NewGraph,Add,Remove}) EnableWindow(ControlHandle(id),project!=nullptr);
    for (int id:{Palette,Source,Destination,Connect}) EnableWindow(ControlHandle(id),graph);
    for (int id:{AddHealth,AddMovement,AttachBehavior,BehaviorGraph}) EnableWindow(ControlHandle(id),prefab);
    EnableWindow(ControlHandle(Place),selection.kind==ed::UiElement::SceneNode); EnableWindow(ControlHandle(Prefabs),selection.kind==ed::UiElement::SceneNode);
    EnableWindow(ControlHandle(Apply),Draft()); EnableWindow(ControlHandle(Revert),Draft());
    std::string log=local_error_;
    if (workspace_.LastError()) { const auto &error=*workspace_.LastError(); log+="\r\n"+error.code+": "+error.message; }
    const std::array<const char *,5> states{"Completed","Budget exhausted","Failed","Waiting","Cancelled"};
    for (const auto &activation:workspace_.Activations()) {
        const auto report=activation->Report(); log+="\r\nActivation "+Number(activation->Id())+" / "+(report?states[static_cast<std::size_t>(report->status)]:"Queued");
        if (report) log+=" / instructions "+Number(report->instructions)+" / native calls "+Number(report->native_calls);
        if (report && report->diagnostic) { const auto &d=*report->diagnostic; log+=" / "+d.error.code+" / asset "+Number(d.asset.value.Raw())+" / node "+Number(d.node.Raw())+" / instruction "+Number(d.instruction)+" / "+d.error.message; }
    }
    if (log.empty()) log="Ready. New example includes Scene, Enemy Prefab (Health / Movement) and Event -> Damage -> Wait -> Damage.\r\nSelect Placement 42, Play, then Send hit. Runtime changes are discarded by Stop.";
    if (log!=log_text_) { log_text_=log; SetText(ControlHandle(Log),log); }
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
        int action=0;
        if (GetKeyState(VK_CONTROL)<0) {
            if (wp=='S') action=Save; else if (wp=='O') action=Open; else if (wp=='N') action=New;
            else if (wp=='Z' && hwnd!=self.ControlHandle(Value)) action=Undo;
            else if (wp=='Y' && hwnd!=self.ControlHandle(Value)) action=Redo;
        } else if (wp==VK_F5) action=GetKeyState(VK_SHIFT)<0?Stop:Play;
        else if (wp==VK_RETURN && hwnd==self.ControlHandle(Value)) action=Apply;
        else if (wp==VK_ESCAPE && hwnd==self.ControlHandle(Value)) action=Revert;
        else if (wp==VK_TAB) { const auto next=GetNextDlgTabItem(self.hwnd_,hwnd,GetKeyState(VK_SHIFT)<0); if (next) SetFocus(next); return 0; }
        if (action) { SendMessageW(self.hwnd_,WM_COMMAND,static_cast<WPARAM>(action),0); return 0; }
    }
    return DefSubclassProc(hwnd,msg,wp,lp);
}
LRESULT Desktop::Message(UINT msg,WPARAM wp,LPARAM lp) {
    if (msg==WM_SIZE || msg==WM_DPICHANGED) { Layout(); return DefSubclassProc(hwnd_,msg,wp,lp); }
    if (msg==WM_GETMINMAXINFO) { const auto dpi=static_cast<int>(GetDpiForWindow(hwnd_)); auto *limits=reinterpret_cast<MINMAXINFO *>(lp); limits->ptMinTrackSize={MulDiv(1000,dpi,96),MulDiv(800,dpi,96)}; return 0; }
    if (msg==WM_CLOSE) { if (CanReplace()) { const auto result=workspace_.Stop(); if (result) closed_=true; else Diagnostics(); } return 0; }
    if (msg==WM_TIMER) { (void)workspace_.Tick(GameDuration{1}); Diagnostics(); return 0; }
    if (msg==WM_COMMAND && !refreshing_) {
        const auto id=LOWORD(wp), notification=HIWORD(wp);
        if (id==Fields && notification==LBN_SELCHANGE) {
            const auto index=static_cast<int>(SendMessageW(ControlHandle(Fields),LB_GETCURSEL,0,0));
            if (!Draft()) SelectField(index);
            else { SendMessageW(ControlHandle(Fields),LB_SETCURSEL,static_cast<WPARAM>(field_),0); local_error_="Apply or Revert the edited field before changing selection."; }
            Diagnostics();
        } else if (id==Value && notification==EN_CHANGE) Diagnostics();
        else if (notification==BN_CLICKED) Action(id);
        return 0;
    }
    if (msg==WM_NOTIFY && !refreshing_) {
        const auto *notice=reinterpret_cast<NMHDR *>(lp);
        if (notice->idFrom==Tree && notice->code==TVN_SELCHANGINGW && Draft()) { local_error_="Apply or Revert the edited field before changing selection."; Diagnostics(); return TRUE; }
        if (notice->idFrom==Tree && notice->code==TVN_SELCHANGEDW) {
            const auto *change=reinterpret_cast<NMTREEVIEWW *>(lp); const auto index=static_cast<std::size_t>(change->itemNew.lParam);
            if (index<selections_.size() && workspace_.Select(selections_[index])) {
                refreshing_=true; outputs_.clear(); inputs_.clear(); SendMessageW(ControlHandle(Source),CB_RESETCONTENT,0,0); SendMessageW(ControlHandle(Destination),CB_RESETCONTENT,0,0); Inspector(); refreshing_=false; Diagnostics();
            }
            return 0;
        }
    }
    return DefSubclassProc(hwnd_,msg,wp,lp);
}
void Desktop::Smoke(const std::filesystem::path &file) {
    const auto check=[](bool value) { if (!value) throw std::runtime_error("Native Editor smoke invariant"); };
    ex::Take(workspace_.New(file,ex::Definition(schema_))); Refresh();
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
    SendMessageW(hwnd_,WM_COMMAND,Save,0); check(!workspace_.Project()->IsDirty());
    (void)workspace_.Select({AssetId::FromString("example.scene"),ed::UiElement::SceneNode,42}); Refresh();
    SendMessageW(hwnd_,WM_COMMAND,Play,0); check(workspace_.Session() && workspace_.Session()->IsPlaying());
    SetText(ControlHandle(Payload),"1"); SendMessageW(hwnd_,WM_COMMAND,SendEvent,0); ex::Take(workspace_.Tick({}));
    check(workspace_.Activations().size()==1); const auto activation=workspace_.Activations().front(); check(!activation->IsFinished());
    SendMessageW(hwnd_,WM_COMMAND,Stop,0); check(activation->IsFinished());
    ex::Take(workspace_.Open(file)); Refresh(); check(!workspace_.Project()->IsDirty());
    (void)workspace_.Select({AssetId::FromString("example.prefab"),ed::UiElement::PrefabNode,ex::Id("enemy").Raw()}); Refresh();
    check(std::any_of(fields_.begin(),fields_.end(),[](const auto &field){return field.name.ends_with(" / points") && field.value=="87";}));
    // Exercise the supported minimum client layout without touching any other window.
    SetWindowPos(hwnd_,nullptr,0,0,1000,800,SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    RECT client{},tools{}; GetClientRect(hwnd_,&client); GetWindowRect(ControlHandle(Place),&tools);
    POINT corner{tools.right,tools.bottom}; ScreenToClient(hwnd_,&corner);
    check(corner.x<=client.right && corner.y<client.bottom-140);
    SendMessageW(hwnd_,WM_CLOSE,0,0); check(closed_);
}
} // namespace
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    try {
        int argc=0; auto **argv=CommandLineToArgvW(GetCommandLineW(),&argc);
        if (!argv) return 1;
        std::vector<std::wstring> args; for (int i=1;i<argc;++i) args.emplace_back(argv[i]); LocalFree(argv);
        const bool smoke=args.size()==2 && args[0]==L"--ui-smoke";
        if ((!args.empty() && !smoke && args.size()!=1) || (!args.empty() && args[0].starts_with(L"--") && !smoke)) throw std::runtime_error("usage: EpidemicEditorDesktop [project.epidemic] | --ui-smoke <project.epidemic>");
        platform::WindowsPlatformRuntime platform; ex::Schema schema;
        auto window=ex::Take(platform.CreateWindow({"Epidemic Editor",1280,850,!smoke}));
        {
            Desktop editor{window,schema};
            if (smoke) editor.Smoke(std::filesystem::path{args[1]});
            else {
                if (!args.empty()) editor.Load(std::filesystem::path{args[0]});
                while (!editor.Closed() && !window->IsCloseRequested() && !platform.IsExitRequested()) { platform.PumpEvents(); (void)platform.DrainEvents(); MsgWaitForMultipleObjectsEx(0,nullptr,33,QS_ALLINPUT,MWMO_INPUTAVAILABLE); }
            }
        }
        window->Close(); platform.Shutdown(); return 0;
    } catch (const std::exception &error) {
        // CI smoke failures use a deterministic nonzero exit, never a modal dialog.
        if (std::wstring_view{GetCommandLineW()}.find(L"--ui-smoke")==std::wstring_view::npos) MessageBoxW(nullptr,Wide(error.what()).c_str(),L"Epidemic Editor",MB_OK | MB_ICONERROR);
        std::fprintf(stderr,"%s\n",error.what()); OutputDebugStringA(error.what()); return 1;
    }
}
