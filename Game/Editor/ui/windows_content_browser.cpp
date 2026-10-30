#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "windows_content_browser.h"
#include "Epidemic/Game/Editor/content_browser.h"
#include "Epidemic/Game/Editor/content_dependencies.h"
#include "Epidemic/Game/Editor/content_hot_reload.h"
#include "Epidemic/Game/Editor/content_thumbnail_cache.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Runtime/Resources/resource_services.h"
#include "renderer.h"
#include "windows_editor_style.h"
#include <algorithm>
#include <array>
#include <commctrl.h>
#include <commdlg.h>
#include <d2d1_1helper.h>
#include <future>
#include <stdexcept>
#include <thread>
namespace epidemic::game::editor {
namespace {
std::wstring Wide(std::string_view text) {
  if (text.empty())
    return {};
  const auto size = MultiByteToWideChar(
      CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      result.data(), size);
  return result;
}
std::string Utf8(std::wstring_view text) {
  if (text.empty())
    return {};
  const auto size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                        static_cast<int>(text.size()), nullptr,
                                        0, nullptr, nullptr);
  std::string result(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      result.data(), size, nullptr, nullptr);
  return result;
}
std::string Text(HWND handle) {
  const auto size = GetWindowTextLengthW(handle);
  std::wstring text(static_cast<std::size_t>(size) + 1, L'\0');
  GetWindowTextW(handle, text.data(), size + 1);
  text.resize(static_cast<std::size_t>(size));
  return Utf8(text);
}
foundation::Result<runtime::ResourceLoadArtifact>
PrepareBrowserPreview(std::filesystem::path root, std::string key) {
  ImageResourceLoader worker{std::move(root)};
  const auto id = worker.Register(std::move(key));
  if (!id)
    return foundation::Result<runtime::ResourceLoadArtifact>::Failure(
        id.GetError());
  return worker.Load({id.Value(), worker.GetResourceType()});
}
struct Browser final {
  enum : int {
    Search = 200,
    List,
    Path,
    Import,
    Reimport,
    Rename,
    Save,
    Preview,
    Info,
    Close,
    UndoMove,
    RedoMove,
    AssignPrefab,
    Duplicate,
    Delete,
    Restore,
    CancelJobs,
    References,
    ShowTrash
  };
  EditorWorkspace &workspace;
  bool smoke = false;
  HWND window = nullptr;
  EditorStyle style;
  BrowserListing listing;
  runtime::AssetId selection{};
  std::uint64_t content_seen = 0;
  bool show_trash = false;
  ThumbnailCache thumbnails{std::size_t{16} * 1024 * 1024, 128};
  std::unique_ptr<runtime::presentation::Renderer> renderer;
  runtime::presentation::RendererTexturePtr texture;
  // Loader precedes services: borrowed registry loader outlives the manager.
  std::unique_ptr<ImageResourceLoader> loader;
  runtime::ResourceServices resources;
  std::optional<runtime::ResourceLease> lease;
  std::uint32_t image_width = 0, image_height = 0;
  std::string desired_key, working_key;
  std::future<foundation::Result<runtime::ResourceLoadArtifact>> preview_job;
  explicit Browser(EditorWorkspace &value, bool test)
      : workspace(value), smoke(test) {}
  HWND Control(int id) const { return GetDlgItem(window, id); }
  void Label(int id, std::string_view text) const {
    SetWindowTextW(Control(id), Wide(text).c_str());
  }
  void Error(std::string_view text) const { Label(Info, text); }
  void Result(const foundation::Result<void> &result) {
    if (!result)
      Error(result.GetError().code + ": " + result.GetError().message);
    else {
      Refresh();
      Error("Ready. Save commits authoring and asset metadata together.");
    }
  }
  void NativeError(const char *message) const noexcept {
    try {
      Error(message);
    } catch (...) {
      SetWindowTextW(Control(Info),
                     L"Cannot display Content Browser diagnostic");
    }
  }
  void Add(int id, const wchar_t *kind, const wchar_t *text, DWORD flags = 0) {
    if (lstrcmpiW(kind, L"BUTTON") == 0)
      flags |= BS_OWNERDRAW;
    const auto child = CreateWindowExW(
        0, kind, text, WS_CHILD | WS_VISIBLE | flags, 0, 0, 10, 10, window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    if (!child)
      throw std::runtime_error("Cannot create Content Browser control");
    SendMessageW(child, WM_SETFONT,
                 reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                 TRUE);
  }
  bool DrawButton(const DRAWITEMSTRUCT &item) const {
    if (item.CtlType != ODT_BUTTON)
      return false;
    FillRect(item.hDC, &item.rcItem,
             (item.itemState & static_cast<UINT>(ODS_SELECTED)) != 0
                 ? style.Input()
                 : style.Panel());
    FrameRect(item.hDC, &item.rcItem, style.Input());
    std::array<wchar_t, 128> text{};
    GetWindowTextW(item.hwndItem, text.data(), static_cast<int>(text.size()));
    const auto font = SendMessageW(item.hwndItem, WM_GETFONT, 0, 0);
    const auto previous =
        SelectObject(item.hDC, reinterpret_cast<HGDIOBJ>(font));
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC,
                 IsWindowEnabled(item.hwndItem) ? style.text : style.muted);
    auto rect = item.rcItem;
    DrawTextW(item.hDC, text.data(), -1, &rect,
              DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    if ((item.itemState & static_cast<UINT>(ODS_FOCUS)) != 0) {
      InflateRect(&rect, -3, -3);
      DrawFocusRect(item.hDC, &rect);
    }
    if (previous)
      SelectObject(item.hDC, previous);
    return true;
  }
  void Initialize() {
    style.ApplyFrame(window);
    Add(Search, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    SendMessageW(Control(Search), EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(L"Search Content paths"));
    Add(List, L"LISTBOX", L"",
        WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY |
            LBS_NOINTEGRALHEIGHT);
    Add(Path, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    SendMessageW(Control(Path), EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(
                     L"Destination relative to Content (e.g. Enemy.png)"));
    Add(Import, L"BUTTON", L"Import PNG...", WS_TABSTOP);
    Add(Reimport, L"BUTTON", L"Reimport", WS_TABSTOP);
    Add(Rename, L"BUTTON", L"Rename / Move", WS_TABSTOP);
    Add(UndoMove, L"BUTTON", L"Undo", WS_TABSTOP);
    Add(RedoMove, L"BUTTON", L"Redo", WS_TABSTOP);
    Add(Save, L"BUTTON", L"Save project", WS_TABSTOP);
    Add(Close, L"BUTTON", L"Close", WS_TABSTOP);
    Add(AssignPrefab, L"BUTTON", L"Assign Sprite", WS_TABSTOP);
    Add(Duplicate, L"BUTTON", L"Duplicate", WS_TABSTOP);
    Add(Delete, L"BUTTON", L"Delete", WS_TABSTOP);
    Add(Restore, L"BUTTON", L"Restore", WS_TABSTOP);
    Add(CancelJobs, L"BUTTON", L"Cancel jobs", WS_TABSTOP);
    Add(References, L"BUTTON", L"References", WS_TABSTOP);
    Add(ShowTrash, L"BUTTON", L"Show trash", WS_TABSTOP);
    Add(Preview, L"STATIC", L"", WS_BORDER);
    Add(Info, L"EDIT", L"", ES_READONLY | ES_MULTILINE | WS_VSCROLL);
    loader = std::make_unique<ImageResourceLoader>(workspace.Folder()->Root());
    auto created =
        runtime::CreateResourceServices({std::size_t{64} * 1024 * 1024});
    if (!created)
      throw std::runtime_error("Cannot create image Resources");
    resources = std::move(created).Value();
    const auto registered = resources.loaders->RegisterLoader(*loader);
    if (!registered)
      throw std::runtime_error("Cannot register image loader");
    const auto frozen = resources.loaders->Freeze();
    if (!frozen)
      throw std::runtime_error("Cannot freeze image loaders");
    if (!SetWindowSubclass(Control(Preview), &PreviewProc, 1,
                           reinterpret_cast<DWORD_PTR>(this)))
      throw std::runtime_error("Cannot initialize image preview");
    Layout();
    Refresh();
    Error("Select a PNG to preview or assign to selected Prefab.");
    SetTimer(window, 20, 300, nullptr);
  }
  void Layout() {
    RECT bounds{};
    GetClientRect(window, &bounds);
    const auto dpi = GetDpiForWindow(window);
    const auto scale = [&](int value) {
      return MulDiv(value, static_cast<int>(dpi), 96);
    };
    const int margin = scale(12), row = scale(28), gap = scale(8),
              width = std::max(1L, bounds.right - 2 * margin);
    const int left = std::max(scale(260), width * 55 / 100),
              right = std::max(1, width - left - gap), top = margin + row + gap;
    const auto move = [&](int id, int x, int y, int w, int h) {
      MoveWindow(Control(id), x, y, std::max(1, w), std::max(1, h), TRUE);
    };
    move(Search, margin, margin, width, row);
    move(List, margin, top, left, bounds.bottom - top - scale(142));
    move(Preview, margin + left + gap, top, right,
         std::max(row, static_cast<int>(bounds.bottom) - top - scale(240)));
    move(Info, margin + left + gap,
         std::max(top + row, static_cast<int>(bounds.bottom) - scale(230)),
         right, scale(104));
    move(Path, margin, bounds.bottom - scale(122), width, row);
    const int secondary = std::max(scale(70), (width - 6 * gap) / 7);
    int secondary_x = margin;
    for (int id : {AssignPrefab, Duplicate, Delete, Restore, CancelJobs,
                   References, ShowTrash}) {
      move(id, secondary_x, bounds.bottom - scale(155), secondary, row);
      secondary_x += secondary + gap;
    }
    const int button = std::max(scale(70), (width - 6 * gap) / 7),
              y = bounds.bottom - scale(80);
    int x = margin;
    for (int id : {Import, Reimport, Rename, UndoMove, RedoMove, Save, Close}) {
      move(id, x, y, button, row);
      x += button + gap;
    }
    if (renderer) {
      RECT preview{};
      GetClientRect(Control(Preview), &preview);
      renderer->SetClientExtent(
          static_cast<unsigned>(std::max(1L, preview.right)),
          static_cast<unsigned>(std::max(1L, preview.bottom)));
      renderer->Resize();
    }
    InvalidateRect(Control(Preview), nullptr, FALSE);
  }
  void Refresh() {
    BrowserFilter filter;
    filter.search = Text(Control(Search));
    filter.show_trashed = show_trash;
    listing = BrowseContent(*workspace.Folder(), filter);
    SendMessageW(Control(List), LB_RESETCONTENT, 0, 0);
    for (std::size_t i = 0; i < listing.items.size(); ++i) {
      const auto &item = listing.items[i];
      const auto text =
          Wide(std::string(item.trashed   ? "[Trash] "
                           : item.present ? ""
                                          : "[Missing] ") +
               item.path + "  [" + std::to_string(item.id.Raw()) + "]");
      SendMessageW(Control(List), LB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(text.c_str()));
      if (item.id == selection)
        SendMessageW(Control(List), LB_SETCURSEL, i, 0);
    }
    SetWindowTextW(window, workspace.IsDirty() ? L"Content Browser *"
                                               : L"Content Browser");
  }
  void Select() {
    const auto index = SendMessageW(Control(List), LB_GETCURSEL, 0, 0);
    if (index == LB_ERR ||
        static_cast<std::size_t>(index) >= listing.items.size())
      return;
    const auto &item = listing.items[static_cast<std::size_t>(index)];
    selection = item.id;
    Label(Path,
          item.path.starts_with("Content/") ? item.path.substr(8) : item.path);
    for (const auto &source : workspace.Folder()->Sources())
      if (source.id == selection) {
        Error("AssetId: " + std::to_string(source.id.Raw()) +
              " | generation: " + std::to_string(source.generation) + "\r\n" +
              source.relative_path + "\r\n" +
              (source.cooked_key.empty() ? "No cooked image"
                                         : source.cooked_key));
        if (source.cooked_key.empty() || source.IsTrashed()) {
          desired_key.clear();
          ClearPreview();
          return;
        }
        desired_key = source.cooked_key;
        PumpPreview();
        return;
      }
  }
  void PumpPreview() {
    if (preview_job.valid()) {
      if (preview_job.wait_for(std::chrono::milliseconds{0}) !=
          std::future_status::ready)
        return;
      auto result = preview_job.get();
      if (working_key == desired_key) {
        if (!result) {
          Error(result.GetError().code + ": last-good preview retained");
          desired_key.clear();
          return;
        }
        PublishPreview(working_key, std::move(result).Value());
        desired_key.clear();
        return;
      }
    }
    if (desired_key.empty())
      return;
    working_key = desired_key;
    const auto root = workspace.Folder()->Root();
    const auto key = working_key;
    preview_job =
        std::async(std::launch::async, PrepareBrowserPreview, root, key);
  }
  void PublishPreview(const std::string &key,
                      runtime::ResourceLoadArtifact artifact) {
    const auto id = loader->Register(key);
    if (!id) {
      Error(id.GetError().code);
      return;
    }
    const auto staged = loader->StagePrepared(std::move(artifact));
    if (!staged) {
      Error(staged.GetError().code);
      return;
    }
    const auto candidate = resources.manager->RequestLease(
        {id.Value(), loader->GetResourceType()});
    if (!candidate) {
      loader->CollectUnused(*resources.manager);
      Error(candidate.GetError().code);
      return;
    }
    const auto loaded = resources.manager->ProcessPendingLoads();
    const auto payload = std::dynamic_pointer_cast<const ImagePayload>(
        resources.manager->GetPayload(candidate.Value().resource));
    if (!loaded || !payload) {
      (void)resources.manager->Release(candidate.Value());
      loader->CollectUnused(*resources.manager);
      Error("Image publication failed; previous preview retained.");
      return;
    }
    try {
      if (!renderer) {
        RECT bounds{};
        GetClientRect(Control(Preview), &bounds);
        renderer = std::make_unique<runtime::presentation::Renderer>(
            Control(Preview),
            std::span<const runtime::presentation::SpriteEntry>{}, 0);
        renderer->SetClientExtent(
            static_cast<unsigned>(std::max(1L, bounds.right)),
            static_cast<unsigned>(std::max(1L, bounds.bottom)));
        renderer->Resize();
      }
      DecodedPng thumbnail;
      if (auto cached = thumbnails.Get(key))
        thumbnail = {cached->width, cached->height, std::move(cached->bgra)};
      else {
        thumbnail = MakeImageThumbnail(payload->image);
        (void)thumbnails.Put(
            key, {thumbnail.width, thumbnail.height, thumbnail.pixels});
      }
      auto candidate_texture = renderer->CreateTexture(
          thumbnail.width, thumbnail.height, thumbnail.pixels);
      ClearPreview();
      lease = candidate.Value();
      texture = std::move(candidate_texture);
      image_width = thumbnail.width;
      image_height = thumbnail.height;
      InvalidateRect(Control(Preview), nullptr, FALSE);
    } catch (...) {
      (void)resources.manager->Release(candidate.Value());
      loader->CollectUnused(*resources.manager);
      throw;
    }
  }
  void Smoke() {
    bool found = false;
    for (std::size_t i = 0; i < listing.items.size(); ++i)
      if (listing.items[i].path.ends_with(".png")) {
        SendMessageW(Control(List), LB_SETCURSEL, i, 0);
        Select();
        for (int attempt = 0;
             attempt < 600 && (!desired_key.empty() || preview_job.valid());
             ++attempt) {
          PumpPreview();
          std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        found = texture && lease.has_value();
        break;
      }
    if (!found)
      throw std::runtime_error(
          "Content smoke: no resident PNG/texture preview");
    Paint();
    Layout();
    Paint();
    ClearPreview();
    if (resources.manager->GetMemoryStatistics().resident_bytes != 0 ||
        resources.manager->GetMemoryStatistics().invariant_failure_count != 0)
      throw std::runtime_error(
          "Content smoke: leaked resident image after release");
    Select();
    for (int attempt = 0;
         attempt < 600 && (!desired_key.empty() || preview_job.valid());
         ++attempt) {
      PumpPreview();
      std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    Paint();
    if (!texture || !lease)
      throw std::runtime_error("Content smoke: repeated preview lost resource");
    EndDialog(window, 0);
  }
  void ClearPreview() {
    texture.reset();
    if (lease) {
      (void)resources.manager->Release(*lease);
      lease.reset();
    }
    if (resources.manager)
      loader->CollectUnused(*resources.manager);
    if (const auto preview = Control(Preview))
      InvalidateRect(preview, nullptr, FALSE);
  }
  void Paint() {
    PAINTSTRUCT paint{};
    BeginPaint(Control(Preview), &paint);
    EndPaint(Control(Preview), &paint);
    if (!renderer)
      return;
    const auto extent = renderer->ClientExtent();
    const float base =
        std::max(.001f, std::min(extent.x / 1280.f, extent.y / 720.f));
    const float offset = (extent.x - 1280.f * base) * .5f,
                offset_y = (extent.y - 720.f * base) * .5f;
    renderer->Begin();
    renderer->PushTransform(
        D2D1::Matrix3x2F::Scale(1 / base, 1 / base) *
        D2D1::Matrix3x2F::Translation(-offset / base, -offset_y / base));
    renderer->Rect(0, 0, extent.x, extent.y, 0x17222e);
    if (texture && image_width && image_height) {
      const float scale =
          std::min((extent.x - 16) / static_cast<float>(image_width),
                   (extent.y - 16) / static_cast<float>(image_height));
      const auto w = static_cast<float>(image_width) * scale,
                 h = static_cast<float>(image_height) * scale;
      renderer->Texture(texture, (extent.x - w) * .5f, (extent.y - h) * .5f, w,
                        h);
    }
    renderer->PopTransform();
    renderer->End();
  }
  void Action(int id) {
    if (id == Close) {
      EndDialog(window, 0);
      return;
    }
    if (id == AssignPrefab) {
      const auto project = workspace.Project();
      if (!project || !selection.IsValid()) {
        Error("Select a PNG asset first");
        return;
      }
      const auto chosen = workspace.Selection();
      auto prefab = chosen.document;
      if (chosen.kind == UiElement::SceneNode) {
        const auto *scene_doc = project->Snapshot()->Find(chosen.document);
        const auto *scene =
            scene_doc ? std::get_if<composition::SceneCompositionDefinition>(
                            &scene_doc->data)
                      : nullptr;
        if (scene)
          for (const auto &placement : scene->placements)
            if (placement.node.Raw() == chosen.element) {
              prefab = placement.prefab;
              break;
            }
      }
      Result(workspace.AssignPrefabSprite(prefab, selection));
      return;
    }
    if (id == UndoMove) {
      Result(workspace.Undo());
      return;
    }
    if (id == RedoMove) {
      Result(workspace.Redo());
      return;
    }
    if (id == Save) {
      Result(workspace.Save());
      return;
    }
    if (id == Rename) {
      Result(workspace.MoveAsset(selection, Text(Control(Path))));
      return;
    }
    if (id == CancelJobs) {
      workspace.CancelAssetJobs();
      Error("Cancellation requested; already published assets are retained.");
      return;
    }
    if (id == ShowTrash) {
      show_trash = !show_trash;
      Refresh();
      return;
    }
    if (id == Restore) {
      Result(workspace.RestoreAsset(selection));
      return;
    }
    if (id == Delete) {
      auto result = workspace.DeleteAsset(selection);
      if (!result && result.GetError().code == "editor.asset_referenced") {
        if (MessageBoxW(window,
                        L"This asset has incoming references. Move to trash "
                        L"and keep broken references?",
                        L"Delete asset", MB_YESNO | MB_ICONWARNING) != IDYES)
          return;
        result = workspace.DeleteAsset(selection, true);
      }
      Result(result);
      return;
    }
    if (id == Duplicate) {
      const auto result =
          workspace.QueueDuplicate(selection, Text(Control(Path)));
      if (!result)
        Error(result.GetError().code + ": " + result.GetError().message);
      else
        Error("Duplicate queued. Enter a new Content path before duplicating.");
      return;
    }
    if (id == References) {
      const auto index = BuildContentDependencyIndex(*workspace.Folder());
      const auto incoming = index.Incoming(selection);
      std::string text =
          "Incoming references: " + std::to_string(incoming.size());
      for (const auto &ref : incoming)
        text += "\r\n" + std::to_string(ref.dependent.Raw());
      Error(text);
      return;
    }
    if (id == Reimport) {
      const auto result = workspace.ReimportPng(selection);
      if (!result)
        Result(result);
      else
        Error("Reimport queued. Last-good preview remains until publication.");
      return;
    }
    if (id == Import) {
      std::array<wchar_t, 32768> path{};
      OPENFILENAMEW dialog{};
      dialog.lStructSize = sizeof(dialog);
      dialog.hwndOwner = window;
      dialog.lpstrFilter = L"PNG image\0*.png\0\0";
      dialog.lpstrFile = path.data();
      dialog.nMaxFile = static_cast<DWORD>(path.size());
      dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                     OFN_NOCHANGEDIR;
      if (!GetOpenFileNameW(&dialog)) {
        if (CommDlgExtendedError())
          Error("File dialog failed");
        return;
      }
      const std::filesystem::path external{path.data()};
      auto relative = Text(Control(Path));
      if (relative.empty())
        relative = Utf8(external.filename().wstring());
      const auto result = workspace.QueueImportPng(external, relative);
      if (!result)
        Error(result.GetError().code + ": " + result.GetError().message);
      else
        Error("Import queued. Source copy and PNG preparation run off the UI "
              "thread.");
    }
  }
  static LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message, WPARAM wp,
                                      LPARAM lp, UINT_PTR,
                                      DWORD_PTR data) noexcept {
    auto &self = *reinterpret_cast<Browser *>(data);
    try {
      if (message == WM_PAINT) {
        self.Paint();
        return 0;
      }
      if (message == WM_ERASEBKGND)
        return 1;
    } catch (const std::exception &error) {
      self.texture.reset();
      self.renderer.reset();
      self.NativeError(error.what());
    }
    return DefSubclassProc(hwnd, message, wp, lp);
  }
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Required Win32
  // callback ABI and parameter ordering.
  static INT_PTR CALLBACK Procedure(HWND hwnd, UINT message, WPARAM wp,
                                    LPARAM lp) noexcept {
    if (message == WM_INITDIALOG) {
      SetWindowLongPtrW(hwnd, DWLP_USER, lp);
      reinterpret_cast<Browser *>(lp)->window = hwnd;
    }
    auto *self =
        reinterpret_cast<Browser *>(GetWindowLongPtrW(hwnd, DWLP_USER));
    if (!self)
      return FALSE;
    try {
      if (message == WM_INITDIALOG) {
        self->Initialize();
        if (self->smoke)
          self->Smoke();
        return TRUE;
      }
      if (message == WM_DRAWITEM && lp &&
          self->DrawButton(*reinterpret_cast<const DRAWITEMSTRUCT *>(lp)))
        return TRUE;
      if (message == WM_CLOSE) {
        EndDialog(hwnd, 0);
        return TRUE;
      }
      if (message == WM_TIMER && wp == 20) {
        self->PumpPreview();
        (void)self->workspace.PollContent();
        const auto events =
            self->workspace.ContentEventsSince(self->content_seen);
        self->content_seen = self->workspace.ContentRevision();
        if (!events.empty()) {
          self->Refresh();
          for (const auto &event : events)
            if (event.id == self->selection &&
                event.kind == HotAssetEventKind::Updated)
              self->Select();
          for (const auto &event : events)
            if (event.kind == HotAssetEventKind::Failed ||
                event.kind == HotAssetEventKind::Missing)
              self->Error(event.diagnostic);
        }
        return TRUE;
      }
      if (message == WM_SIZE) {
        self->Layout();
        return TRUE;
      }
      if (message == WM_GETMINMAXINFO) {
        auto &info = *reinterpret_cast<MINMAXINFO *>(lp);
        const auto dpi = GetDpiForWindow(hwnd);
        info.ptMinTrackSize = {MulDiv(720, static_cast<int>(dpi), 96),
                               MulDiv(500, static_cast<int>(dpi), 96)};
        return TRUE;
      }
      if (message == WM_COMMAND) {
        const auto id = LOWORD(wp);
        if (id == IDCANCEL) {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        if (id == Search && HIWORD(wp) == EN_CHANGE) {
          self->Refresh();
          return TRUE;
        }
        if (id == List && HIWORD(wp) == LBN_SELCHANGE) {
          self->Select();
          return TRUE;
        }
        if (HIWORD(wp) == BN_CLICKED) {
          self->Action(id);
          return TRUE;
        }
      }
      if (message == WM_CTLCOLORDLG)
        return reinterpret_cast<INT_PTR>(self->style.Background());
      if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT ||
          message == WM_CTLCOLORLISTBOX) {
        auto dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, self->style.text);
        SetBkColor(dc, self->style.input);
        return reinterpret_cast<INT_PTR>(self->style.Input());
      }
      if (message == WM_DESTROY) {
        KillTimer(hwnd, 20);
        self->ClearPreview();
        self->renderer.reset();
        return TRUE;
      }
    } catch (const std::exception &error) {
      self->NativeError(error.what());
      if (message == WM_INITDIALOG)
        EndDialog(hwnd, -1);
    } catch (...) {
      SetWindowTextW(self->Control(Info), L"Content Browser operation failed");
      if (message == WM_INITDIALOG)
        EndDialog(hwnd, -1);
    }
    return FALSE;
  }
};
} // namespace
void RunContentBrowser(HWND owner, EditorWorkspace &workspace, bool smoke) {
  if (!workspace.Folder())
    throw std::runtime_error(
        "Create or open a project folder first (File menu).");
  struct Template {
    DLGTEMPLATE dialog;
    WORD menu = 0, kind = 0, title = 0;
  } layout{};
  layout.dialog.style =
      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_MODALFRAME;
  layout.dialog.cx = 560;
  layout.dialog.cy = 360;
  Browser browser{workspace, smoke};
  const auto result = DialogBoxIndirectParamW(
      GetModuleHandleW(nullptr), &layout.dialog, owner, &Browser::Procedure,
      reinterpret_cast<LPARAM>(&browser));
  if (result == -1)
    throw std::runtime_error("Cannot open Content Browser");
}
void ShowContentBrowser(HWND owner, EditorWorkspace &workspace) {
  RunContentBrowser(owner, workspace, false);
}
void SmokeContentBrowser(HWND owner, EditorWorkspace &workspace) {
  RunContentBrowser(owner, workspace, true);
}
} // namespace epidemic::game::editor
