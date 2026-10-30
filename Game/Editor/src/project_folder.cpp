#include "Epidemic/Game/Editor/project_folder.h"
#include "Epidemic/Game/Editor/asset_commands.h"
#include "Epidemic/Game/Editor/content_dependencies.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Game/Editor/png_import.h"
#include "Epidemic/Game/Editor/project_path.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include "Epidemic/Runtime/Serialization/document_codec.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "project_archive.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace epidemic::game::editor {
namespace {
using Bytes = std::vector<std::byte>;
using VoidResult = foundation::Result<void>;
using foundation::Result;
struct Failure : std::exception {
  foundation::Error error;
  explicit Failure(foundation::Error value) : error(std::move(value)) {}
};
foundation::Error Error(std::string_view code, std::string_view why) {
  return foundation::Error::Create(code, why);
}
void Require(bool value, std::string_view code, std::string_view why) {
  if (!value)
    throw Failure{Error(code, why)};
}
template <class T> T Take(Result<T> result) {
  if (!result)
    throw Failure{result.GetError()};
  return std::move(result).Value();
}
void Take(const VoidResult &result) {
  if (!result)
    throw Failure{result.GetError()};
}
constexpr std::uint64_t kMaxSources = 16384;
const auto kCommitType =
    foundation::StringId::FromString("epidemic.editor.folder_commit");
const auto kSourcesType =
    foundation::StringId::FromString("epidemic.editor.source_index");
constexpr runtime::SchemaVersion kVersion{1, 0, 0};
constexpr runtime::SchemaVersion kSourcesVersion{1, 1, 0};
constexpr runtime::SchemaVersion kSpriteSourcesVersion{1, 2, 0};
constexpr runtime::SchemaVersion kSheetSourcesVersion{1, 3, 0};
constexpr runtime::SchemaVersion kLifecycleSourcesVersion{1, 4, 0};

struct Commit {
  std::uint64_t generation = 0, project = 0, entry_scene = 0,
                authoring_hash = 0, assets_hash = 0;
  std::string title, authoring, assets;
};
struct SourceIndex {
  std::uint64_t next = 1;
  std::vector<ProjectSourceAsset> records;
  std::vector<PrefabSpriteBinding> sprites;
  std::vector<SpriteSheetDefinition> sheets;
};
std::uint64_t Hash(std::span<const std::byte> data) {
  std::uint64_t h = 14695981039346656037ULL;
  for (auto b : data) {
    h ^= static_cast<std::uint8_t>(b);
    h *= 1099511628211ULL;
  }
  return h;
}
std::string RevisionName(std::uint64_t value) {
  // Fixed-width decimal sorts lexically; no locale/path dependence.
  std::string s = std::to_string(value);
  return std::string(s.size() < 16 ? 16 - s.size() : 0, '0') + s;
}
std::string RevisionDir(std::uint64_t g) {
  return "ProjectData/Revisions/" + RevisionName(g);
}
std::string AuthoringPath(std::uint64_t g) {
  return RevisionDir(g) + "/Authoring.epidemic";
}
std::string SourcesPath(std::uint64_t g) {
  return RevisionDir(g) + "/Assets.manifest";
}
std::string CommitPath(std::uint64_t g) {
  return "ProjectData/Commits/" + RevisionName(g) + ".commit";
}
void CheckTitle(std::string_view title) {
  Require(
      !title.empty() && title.size() <= 256 &&
          std::none_of(title.begin(), title.end(),
                       [](unsigned char ch) { return ch == 0 || ch < 32; }),
      "editor.folder_title",
      "Project title must be nonempty, <=256 bytes and contain no controls");
}
Bytes EncodeCommit(const Commit &c) {
  auto services = Take(runtime::CreateSerializationServices());
  auto w = services.archives->CreateWriter();
  Require(w != nullptr, "editor.folder_codec", "No archive writer");
  Take(w->WriteUInt64("generation", c.generation));
  Take(w->WriteUInt64("project", c.project));
  Take(w->WriteUInt64("entry_scene", c.entry_scene));
  Take(w->WriteString("title", c.title));
  Take(w->WriteString("authoring", c.authoring));
  Take(w->WriteUInt64("authoring_hash", c.authoring_hash));
  Take(w->WriteString("assets", c.assets));
  Take(w->WriteUInt64("assets_hash", c.assets_hash));
  return Take(
      runtime::EncodeDocument(Take(w->Finalize(kCommitType, kVersion))));
}
Commit DecodeCommit(std::span<const std::byte> data) {
  auto doc = Take(runtime::DecodeDocument(data));
  Require(doc.GetTypeId() == kCommitType, "editor.folder_commit",
          "Wrong commit document type");
  Require(doc.GetSchemaVersion() == kVersion, "editor.folder_version",
          "Unsupported project descriptor version");
  auto services = Take(runtime::CreateSerializationServices());
  auto r = Take(services.archives->CreateReader(doc));
  Commit c;
  c.generation = Take(r->ReadUInt64("generation"));
  c.project = Take(r->ReadUInt64("project"));
  c.entry_scene = Take(r->ReadUInt64("entry_scene"));
  c.title = Take(r->ReadString("title"));
  c.authoring = Take(r->ReadString("authoring"));
  c.authoring_hash = Take(r->ReadUInt64("authoring_hash"));
  c.assets = Take(r->ReadString("assets"));
  c.assets_hash = Take(r->ReadUInt64("assets_hash"));
  CheckTitle(c.title);
  Require(c.generation > 0 && c.project > 0 && c.entry_scene > 0 &&
              c.authoring == AuthoringPath(c.generation) &&
              c.assets == SourcesPath(c.generation),
          "editor.folder_commit", "Commit references are inconsistent");
  return c;
}
Bytes EncodeSources(const SourceIndex &index) {
  Require(index.next > 0 && index.records.size() <= kMaxSources,
          "editor.folder_sources", "Invalid source index size");
  auto services = Take(runtime::CreateSerializationServices());
  auto w = services.archives->CreateWriter();
  Require(w != nullptr, "editor.folder_codec", "No archive writer");
  Take(w->WriteUInt64("next", index.next));
  Take(w->BeginArray("sources", index.records.size()));
  for (std::size_t i = 0; i < index.records.size(); ++i) {
    const auto &a = index.records[i];
    Take(w->BeginArrayElement(i));
    Take(w->WriteUInt64("id", a.id.Raw()));
    Take(w->WriteUInt64("type", a.type.value.Raw()));
    Take(w->WriteString("path", a.relative_path));
    Take(w->WriteUInt64("hash", a.source_hash));
    Take(w->WriteUInt64("generation", a.generation));
    Take(w->WriteString("cooked", a.cooked_key));
    Take(w->WriteString("original", a.original_path));
    Take(w->EndArrayElement());
  }
  Take(w->EndArray());
  Require(index.sprites.size() <= kMaxSources, "editor.sprite_limit",
          "Too many sprite bindings");
  Take(w->BeginArray("sprites", index.sprites.size()));
  for (std::size_t i = 0; i < index.sprites.size(); ++i) {
    const auto &s = index.sprites[i];
    Take(w->BeginArrayElement(i));
    Take(w->WriteUInt64("prefab", s.prefab.Raw()));
    Take(w->WriteUInt64("texture", s.texture.Raw()));
    Take(w->WriteUInt64("region", s.region_id));
    Take(w->WriteDouble("pivot_x", s.pivot_x));
    Take(w->WriteDouble("pivot_y", s.pivot_y));
    Take(w->WriteDouble("ppu", s.pixels_per_unit));
    Take(w->EndArrayElement());
  }
  Take(w->EndArray());
  Require(index.sheets.size() <= kMaxSources, "editor.sheet_limit",
          "Too many sheet records");
  Take(w->BeginArray("sheets", index.sheets.size()));
  for (std::size_t i = 0; i < index.sheets.size(); ++i) {
    const auto &sheet = index.sheets[i];
    Take(w->BeginArrayElement(i));
    Take(w->WriteUInt64("texture", sheet.texture.Raw()));
    const auto metadata = sheets::Encode(sheet.texture.Raw(), sheet.regions);
    Require(static_cast<bool>(metadata), "editor.sheet_metadata",
            "Cannot encode sheet");
    Take(w->WriteString("regions", metadata.value));
    Take(w->EndArrayElement());
  }
  Take(w->EndArray());
  return Take(runtime::EncodeDocument(
      Take(w->Finalize(kSourcesType, kLifecycleSourcesVersion))));
}
SourceIndex DecodeSources(std::span<const std::byte> data) {
  auto doc = Take(runtime::DecodeDocument(data));
  Require(doc.GetTypeId() == kSourcesType, "editor.folder_sources",
          "Wrong source index type");
  Require(doc.GetSchemaVersion() == kVersion ||
              doc.GetSchemaVersion() == kSourcesVersion ||
              (doc.GetSchemaVersion() == kSpriteSourcesVersion ||
               (doc.GetSchemaVersion() == kSheetSourcesVersion ||
                doc.GetSchemaVersion() == kLifecycleSourcesVersion)),
          "editor.folder_version", "Unsupported source index version");
  auto services = Take(runtime::CreateSerializationServices());
  auto r = Take(services.archives->CreateReader(doc));
  SourceIndex index;
  index.next = Take(r->ReadUInt64("next"));
  const auto count = Take(r->BeginArray("sources"));
  Require(index.next > 0 && count <= kMaxSources, "editor.folder_sources",
          "Source index bounds exceeded");
  std::set<std::uint64_t> ids;
  std::set<std::string> paths;
  for (std::size_t i = 0; i < count; ++i) {
    Take(r->BeginArrayElement(i));
    ProjectSourceAsset a;
    a.id = runtime::AssetId{foundation::StringId{Take(r->ReadUInt64("id"))}};
    a.type =
        runtime::AssetType{foundation::StringId{Take(r->ReadUInt64("type"))}};
    a.relative_path = Take(r->ReadString("path"));
    a.source_hash = Take(r->ReadUInt64("hash"));
    a.generation = Take(r->ReadUInt64("generation"));
    if (doc.GetSchemaVersion() == kSourcesVersion ||
        doc.GetSchemaVersion() == kSpriteSourcesVersion ||
        (doc.GetSchemaVersion() == kSheetSourcesVersion ||
         doc.GetSchemaVersion() == kLifecycleSourcesVersion))
      a.cooked_key = Take(r->ReadString("cooked"));
    if (doc.GetSchemaVersion() == kLifecycleSourcesVersion)
      a.original_path = Take(r->ReadString("original"));
    Require(a.original_path.empty() ||
                (a.original_path.starts_with("Content/") &&
                 a.original_path.size() <= runtime::kMaxAssetPathBytes &&
                 a.relative_path.starts_with("Content/.Trash/")),
            "editor.folder_trash", "Invalid staged delete metadata");
    Require(a.cooked_key.empty() ||
                (a.cooked_key.size() == 64 &&
                 a.cooked_key.find_first_not_of("0123456789abcdef") ==
                     std::string::npos),
            "editor.folder_sources", "Invalid cooked key");
    Take(r->EndArrayElement());
    Require(a.id.IsValid() && a.type.IsValid() && a.generation > 0 &&
                a.generation <= std::numeric_limits<std::uint32_t>::max() &&
                a.relative_path.starts_with("Content/") &&
                a.relative_path.size() <= runtime::kMaxAssetPathBytes,
            "editor.folder_sources", "Invalid source asset metadata");
    auto folded = a.relative_path;
    std::transform(
        folded.begin(), folded.end(), folded.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    Require(ids.insert(a.id.Raw()).second && paths.insert(folded).second,
            "editor.folder_sources",
            "Duplicate source identity or case-insensitive path");
    index.records.push_back(std::move(a));
  }
  Take(r->EndArray());
  if (doc.GetSchemaVersion() == kSpriteSourcesVersion ||
      (doc.GetSchemaVersion() == kSheetSourcesVersion ||
       doc.GetSchemaVersion() == kLifecycleSourcesVersion)) {
    const auto sprites = Take(r->BeginArray("sprites"));
    Require(sprites <= kMaxSources, "editor.sprite_limit",
            "Too many sprite bindings");
    std::set<std::uint64_t> prefabs;
    for (std::size_t i = 0; i < sprites; ++i) {
      Take(r->BeginArrayElement(i));
      PrefabSpriteBinding s;
      s.prefab =
          runtime::AssetId{foundation::StringId{Take(r->ReadUInt64("prefab"))}};
      s.texture = runtime::AssetId{
          foundation::StringId{Take(r->ReadUInt64("texture"))}};
      if ((doc.GetSchemaVersion() == kSheetSourcesVersion ||
           doc.GetSchemaVersion() == kLifecycleSourcesVersion))
        s.region_id = Take(r->ReadUInt64("region"));
      s.pivot_x = static_cast<float>(Take(r->ReadDouble("pivot_x")));
      s.pivot_y = static_cast<float>(Take(r->ReadDouble("pivot_y")));
      s.pixels_per_unit = static_cast<float>(Take(r->ReadDouble("ppu")));
      Take(r->EndArrayElement());
      Require(s.prefab.IsValid() && s.texture.IsValid() &&
                  std::isfinite(s.pivot_x) && std::isfinite(s.pivot_y) &&
                  s.pivot_x >= 0 && s.pivot_x <= 1 && s.pivot_y >= 0 &&
                  s.pivot_y <= 1 && std::isfinite(s.pixels_per_unit) &&
                  s.pixels_per_unit > 0 && s.pixels_per_unit <= 4096 &&
                  prefabs.insert(s.prefab.Raw()).second &&
                  ids.contains(s.texture.Raw()),
              "editor.sprite_binding", "Invalid or duplicate sprite binding");
      index.sprites.push_back(s);
    }
    Take(r->EndArray());
  }
  if ((doc.GetSchemaVersion() == kSheetSourcesVersion ||
       doc.GetSchemaVersion() == kLifecycleSourcesVersion)) {
    const auto sheets_count = Take(r->BeginArray("sheets"));
    Require(sheets_count <= kMaxSources, "editor.sheet_limit",
            "Too many spritesheets");
    std::set<std::uint64_t> sheet_ids;
    for (std::size_t i = 0; i < sheets_count; ++i) {
      Take(r->BeginArrayElement(i));
      const auto raw = Take(r->ReadUInt64("texture"));
      const auto encoded = Take(r->ReadString("regions"));
      Take(r->EndArrayElement());
      Require(raw && ids.contains(raw) && sheet_ids.insert(raw).second,
              "editor.sheet_identity", "Sheet has missing or duplicate parent");
      const auto decoded = sheets::Decode(raw, encoded);
      Require(static_cast<bool>(decoded), "editor.sheet_metadata",
              "Malformed region table");
      index.sheets.push_back(
          {runtime::AssetId{foundation::StringId{raw}}, decoded.value});
    }
    Take(r->EndArray());
  }
  return index;
}
Result<Bytes> Read(const std::filesystem::path &p) {
  return platform::ReadFilePackage(foundation::Path{p},
                                   runtime::kMaxDocumentBytes);
}
VoidResult Publish(const std::filesystem::path &p,
                   std::span<const std::byte> bytes) {
  return platform::CommitFilePackage(foundation::Path{p}, bytes,
                                     runtime::kMaxDocumentBytes);
}
void EnsureDirectories(const std::filesystem::path &root) {
  for (const auto *path : {"Content", "Scripts", "ProjectData/Revisions",
                           "ProjectData/Commits", ".epidemic"}) {
    std::error_code ec;
    std::filesystem::create_directories(root / path, ec);
    Require(!ec, "editor.folder_io", "Cannot create project directory");
  }
}
std::filesystem::path RootPath(const std::filesystem::path &root) {
  std::error_code ec;
  auto absolute = std::filesystem::canonical(root, ec);
  Require(!ec && std::filesystem::is_directory(absolute), "editor.folder_root",
          "Project root does not exist");
  return absolute;
}
std::filesystem::path SafeAt(const std::filesystem::path &root,
                             std::string_view rel) {
  return Take(ResolveProjectPath(root, rel));
}
Bytes EncodeMoves(
    std::uint64_t base,
    const std::vector<std::pair<std::string, std::string>> &moves) {
  auto services = Take(runtime::CreateSerializationServices());
  auto writer = services.archives->CreateWriter();
  Require(writer != nullptr, "editor.folder_codec", "No move journal writer");
  Take(writer->WriteUInt64("base", base));
  Take(writer->BeginArray("moves", moves.size()));
  for (std::size_t i = 0; i < moves.size(); ++i) {
    Take(writer->BeginArrayElement(i));
    Take(writer->WriteString("before", moves[i].first));
    Take(writer->WriteString("after", moves[i].second));
    Take(writer->EndArrayElement());
  }
  Take(writer->EndArray());
  return Take(runtime::EncodeDocument(Take(writer->Finalize(
      foundation::StringId::FromString("epidemic.editor.move_journal"),
      kVersion))));
}
void RecoverMoves(const std::filesystem::path &root, std::uint64_t generation) {
  const auto journal = SafeAt(root, ".epidemic/source-moves.journal");
  std::error_code ec;
  if (!std::filesystem::exists(journal, ec)) {
    Require(!ec, "editor.folder_journal", "Cannot inspect move journal");
    return;
  }
  const auto document = Take(runtime::DecodeDocument(Take(Read(journal))));
  Require(document.GetTypeId() == foundation::StringId::FromString(
                                      "epidemic.editor.move_journal") &&
              document.GetSchemaVersion() == kVersion,
          "editor.folder_journal", "Unsupported move journal");
  auto services = Take(runtime::CreateSerializationServices());
  auto reader = Take(services.archives->CreateReader(document));
  const auto base = Take(reader->ReadUInt64("base"));
  const auto count = Take(reader->BeginArray("moves"));
  Require(count <= 128 && base <= generation, "editor.folder_journal",
          "Move journal bounds/base mismatch");
  std::vector<std::pair<std::string, std::string>> moves;
  for (std::size_t i = 0; i < count; ++i) {
    Take(reader->BeginArrayElement(i));
    auto before = Take(reader->ReadString("before")),
         after = Take(reader->ReadString("after"));
    Require(before.starts_with("Content/") && after.starts_with("Content/"),
            "editor.folder_journal", "Move journal path outside Content");
    (void)SafeAt(root, before);
    (void)SafeAt(root, after);
    moves.emplace_back(std::move(before), std::move(after));
    Take(reader->EndArrayElement());
  }
  Take(reader->EndArray());
  if (base == generation)
    while (!moves.empty()) {
      const auto before = SafeAt(root, moves.back().first),
                 after = SafeAt(root, moves.back().second);
      const bool original = std::filesystem::exists(before, ec);
      Require(!ec, "editor.folder_journal", "Cannot inspect original source");
      const bool moved = std::filesystem::exists(after, ec);
      Require(!ec, "editor.folder_journal", "Cannot inspect moved source");
      Require(original != moved, "editor.folder_journal",
              "Ambiguous move recovery; preserve files for manual repair");
      if (moved) {
        std::filesystem::rename(after, before, ec);
        Require(!ec, "editor.folder_journal",
                "Cannot restore uncommitted source move");
      }
      moves.pop_back();
      Take(Publish(journal, EncodeMoves(base, moves)));
    }
  std::filesystem::remove(journal, ec);
  Require(!ec, "editor.folder_journal", "Cannot retire recovered move journal");
}
std::uint64_t HashFileStream(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  Require(in.good(), "editor.folder_source", "Cannot read source file");
  std::uint64_t h = 14695981039346656037ULL;
  std::array<char, 65536> bytes{};
  while (in) {
    in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    for (std::streamsize i = 0; i < in.gcount(); ++i) {
      h ^= static_cast<unsigned char>(bytes[static_cast<std::size_t>(i)]);
      h *= 1099511628211ULL;
    }
  }
  Require(in.eof() && !in.bad(), "editor.folder_source",
          "Source file read failed");
  return h;
}
struct Restored {
  Commit commit;
  SourceIndex index;
  std::unique_ptr<EditorProject> editor;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named
// filesystem/source and mount/path roles intentionally share their public value
// types.
Restored RestoreCommit(const std::filesystem::path &root,
                       const std::filesystem::path &file,
                       const ProjectSchema &schema) {
  const auto commit_bytes = Take(Read(file));
  const auto c = DecodeCommit(commit_bytes);
  Require(file.filename() ==
              std::filesystem::path{RevisionName(c.generation) + ".commit"},
          "editor.folder_commit", "Commit filename mismatch");
  const auto authoring_file = SafeAt(root, c.authoring),
             source_file = SafeAt(root, c.assets);
  const auto authoring = Take(Read(authoring_file));
  const auto sources = Take(Read(source_file));
  Require(Hash(authoring) == c.authoring_hash && Hash(sources) == c.assets_hash,
          "editor.folder_integrity", "Commit file hashes do not match");
  auto index = DecodeSources(sources);
  // Fail on malformed index/path without interpreting arbitrary metadata as
  // host paths.
  for (const auto &a : index.records)
    (void)SafeAt(root, a.relative_path);
  auto loaded = EditorProject::Open(authoring_file, schema);
  // A valid committed byte image that cannot be interpreted by the current
  // game schema must not silently downgrade to an older snapshot.
  Require(static_cast<bool>(loaded), "editor.folder_schema",
          "Committed authoring cannot open with current schema");
  auto editor = std::move(loaded).Value();
  Require(editor->Snapshot()->definition.id.Raw() == c.project &&
              editor->Snapshot()->definition.entry_scene.Raw() == c.entry_scene,
          "editor.folder_integrity",
          "Authoring project identity/entry do not match commit");
  return {c, std::move(index), std::move(editor)};
}
std::vector<std::filesystem::path> Commits(const std::filesystem::path &root) {
  std::vector<std::filesystem::path> result;
  std::error_code ec;
  const auto dir = root / "ProjectData/Commits";
  if (!std::filesystem::is_directory(dir, ec) || ec)
    return result;
  for (const auto &item : std::filesystem::directory_iterator(dir)) {
    const auto name = item.path().filename().string();
    if (name.size() != 23 || !name.ends_with(".commit") ||
        !std::all_of(name.begin(), name.begin() + 16,
                     [](char c) { return c >= '0' && c <= '9'; }))
      continue;
    const auto status = item.symlink_status(ec);
    if (ec || !std::filesystem::is_regular_file(status))
      continue;
    result.push_back(item.path());
  }
  std::sort(result.begin(), result.end(), std::greater<>{});
  return result;
}
bool IsSubpath(const std::filesystem::path &root,
               const std::filesystem::path &candidate) {
  auto a = root.begin(), b = candidate.begin();
  for (; a != root.end(); ++a, ++b) {
    if (b == candidate.end())
      return false;
#ifdef _WIN32
    if (CompareStringOrdinal(a->c_str(), -1, b->c_str(), -1, TRUE) !=
        CSTR_EQUAL)
      return false;
#else
    if (*a != *b)
      return false;
#endif
  }
  return true;
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named
// filesystem/source and mount/path roles intentionally share their public value
// types.
void CopyTree(const std::filesystem::path &src,
              const std::filesystem::path &dest, bool skip_imports = false) {
  std::error_code ec;
  if (!std::filesystem::exists(src, ec))
    return;
  Require(!ec && std::filesystem::is_directory(src, ec), "editor.folder_clone",
          "Source mount is not a directory");
  for (auto iterator = std::filesystem::recursive_directory_iterator(src);
       iterator != std::filesystem::recursive_directory_iterator{};
       ++iterator) {
    const auto &entry = *iterator;
    if (skip_imports && iterator.depth() == 0) {
      auto name = entry.path().filename().generic_string();
      std::transform(
          name.begin(), name.end(), name.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      if (name == ".imports") {
        iterator.disable_recursion_pending();
        continue;
      }
    }
    const auto status = entry.symlink_status(ec);
    Require(!ec && !std::filesystem::is_symlink(status), "editor.folder_clone",
            "Symlink in portable source tree");

#ifdef _WIN32
    const auto attributes = GetFileAttributesW(entry.path().c_str());
    Require(attributes != INVALID_FILE_ATTRIBUTES &&
                (attributes &
                 static_cast<DWORD>(FILE_ATTRIBUTE_REPARSE_POINT)) == 0,
            "editor.folder_clone", "Reparse point in portable source tree");
#endif
    const auto relative = entry.path().lexically_relative(src);
    Require(!relative.empty(), "editor.folder_clone", "Invalid source path");
    const auto output = dest / relative;
    if (std::filesystem::is_directory(status)) {
      std::filesystem::create_directories(output, ec);
    } else if (std::filesystem::is_regular_file(status)) {
      std::filesystem::create_directories(output.parent_path(), ec);
      if (!ec)
        std::filesystem::copy_file(entry.path(), output,
                                   std::filesystem::copy_options::none, ec);
    } else
      throw Failure{
          Error("editor.folder_clone", "Unsupported source entry kind")};
    Require(!ec, "editor.folder_clone", "Cannot clone source file");
  }
}
} // namespace

struct ProjectFolder::WriterLock {
#ifdef _WIN32
  HANDLE handle = INVALID_HANDLE_VALUE;
  ~WriterLock() {
    if (handle != INVALID_HANDLE_VALUE)
      CloseHandle(handle);
  }
#else
  int fd = -1;
  ~WriterLock() {
    if (fd >= 0) {
      (void)flock(fd, LOCK_UN);
      (void)close(fd);
    }
  }
#endif
};
ProjectFolder::ProjectFolder(std::filesystem::path root, std::string title,
                             const ProjectSchema &schema)
    : root_(std::move(root)), title_(std::move(title)), schema_(schema) {}
ProjectFolder::~ProjectFolder() = default;

VoidResult ProjectFolder::AcquireLock() {
  lock_ = std::make_unique<WriterLock>();
  const auto filename = root_ / ".epidemic/workspace.lock";
#ifdef _WIN32
  lock_->handle =
      CreateFileW(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (lock_->handle == INVALID_HANDLE_VALUE) {
    lock_.reset();
    return VoidResult::Failure(
        Error("editor.folder_locked", "Folder is locked by another process"));
  }
#else
  lock_->fd = open(filename.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
  if (lock_->fd < 0 || flock(lock_->fd, LOCK_EX | LOCK_NB) != 0) {
    lock_.reset();
    return VoidResult::Failure(
        Error("editor.folder_locked", "Folder is locked by another process"));
  }
#endif
  return VoidResult::Success();
}
Result<std::unique_ptr<ProjectFolder>> ProjectFolder::CreateWithIds(
    const std::filesystem::path &root, std::string title,
    ProjectDefinition definition, const ProjectSchema &schema,
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> &retired) {
  using R = Result<std::unique_ptr<ProjectFolder>>;
  try {
    CheckTitle(title);
    std::error_code ec;
    Require(!std::filesystem::exists(root, ec) && !ec, "editor.folder_exists",
            "Destination must not exist");
    std::filesystem::create_directories(root, ec);
    Require(!ec, "editor.folder_io", "Cannot create destination");
    auto canonical = RootPath(root);
    EnsureDirectories(canonical);
    auto folder = std::unique_ptr<ProjectFolder>(
        new ProjectFolder(canonical, std::move(title), schema));
    Take(folder->AcquireLock());
    folder->editor_ = Take(EditorProject::Create(
        SafeAt(canonical, AuthoringPath(1)), std::move(definition), schema));
    for (const auto &id : retired)
      folder->editor_->used_ids_.insert(id);
    Require(folder->editor_->used_ids_.size() <= 65536, "editor.identity_limit",
            "Retired identity budget exceeded");
    Take(folder->Save());
    return R::Success(std::move(folder));
  } catch (const Failure &failure) {
    return R::Failure(failure.error);
  } catch (const std::exception &) {
    return R::Failure(
        Error("editor.folder_io", "Create project folder failed"));
  }
}
Result<std::unique_ptr<ProjectFolder>>
ProjectFolder::Create(const std::filesystem::path &root, std::string title,
                      ProjectDefinition definition,
                      const ProjectSchema &schema) {
  return CreateWithIds(root, std::move(title), std::move(definition), schema,
                       {});
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named
// filesystem/source and mount/path roles intentionally share their public value
// types.
Result<std::unique_ptr<ProjectFolder>>
ProjectFolder::ImportLegacy(const std::filesystem::path &root,
                            const std::filesystem::path &legacy,
                            std::string title, const ProjectSchema &schema) {
  using R = Result<std::unique_ptr<ProjectFolder>>;
  try {
    const auto bytes = Take(detail::ReadFile(legacy));
    auto archive = Take(detail::ReadProject(bytes, schema));
    // Validate original identity, including all retired IDs, before creating
    // a destination. The source file is always read-only.
    (void)Take(EditorProject::Open(legacy, schema));
    return CreateWithIds(root, std::move(title), std::move(archive.definition),
                         schema, archive.used_ids);
  } catch (const Failure &failure) {
    return R::Failure(failure.error);
  } catch (const std::exception &) {
    return R::Failure(
        Error("editor.folder_legacy", "Legacy project import failed"));
  }
}
Result<std::unique_ptr<ProjectFolder>>
ProjectFolder::Open(const std::filesystem::path &root,
                    const ProjectSchema &schema) {
  using R = Result<std::unique_ptr<ProjectFolder>>;
  try {
    auto canonical = RootPath(root);
    Require(
        std::filesystem::is_directory(canonical / ".epidemic") &&
            std::filesystem::is_directory(canonical / "ProjectData/Commits"),
        "editor.folder_missing", "Not a project folder");
    auto folder = std::unique_ptr<ProjectFolder>(
        new ProjectFolder(canonical, "", schema));
    Take(folder->AcquireLock());
    const auto commits = Commits(canonical);
    Require(!commits.empty(), "editor.folder_missing",
            "No committed project descriptor");
    // Do not turn a newer unknown schema into a silent downgrade.
    const auto newest = Read(commits.front());
    if (newest) {
      auto doc = runtime::DecodeDocument(newest.Value());
      if (doc && doc.Value().GetTypeId() == kCommitType &&
          doc.Value().GetSchemaVersion() != kVersion)
        throw Failure{Error("editor.folder_version",
                            "Unsupported newer project version")};
    }
    std::optional<Restored> restored;
    for (const auto &commit : commits) {
      try {
        restored = RestoreCommit(canonical, commit, schema);
        break;
      } catch (const Failure &failure) {
        if (failure.error.HasCode("editor.folder_version") ||
            failure.error.HasCode("editor.folder_schema"))
          throw;
        // Last-good fallback for a broken committed generation.
      }
    }
    if (!restored)
      throw Failure{Error("editor.folder_integrity",
                          "No intact committed project revision")};
    EnsureDirectories(canonical);
    folder->title_ = restored->commit.title;
    RecoverMoves(canonical, restored->commit.generation);
    folder->generation_ = restored->commit.generation;
    folder->editor_ = std::move(restored->editor);
    folder->assets_ = std::move(restored->index.records);
    folder->visuals_ = std::make_shared<VisualState>(VisualState{
        std::move(restored->index.sprites), std::move(restored->index.sheets)});
    folder->saved_visuals_ = folder->visuals_;
    folder->saved_assets_ = folder->assets_;
    folder->next_asset_number_ = restored->index.next;
    return R::Success(std::move(folder));
  } catch (const Failure &failure) {
    return R::Failure(failure.error);
  } catch (const std::exception &) {
    return R::Failure(Error("editor.folder_io", "Open project folder failed"));
  }
}
VoidResult ProjectFolder::Save() {
  try {
    Require(editor_ && lock_ &&
                generation_ < std::numeric_limits<std::uint64_t>::max(),
            "editor.folder_state",
            "Missing authoring model, lock, or generation overflow");
    // The process lock prevents concurrent editor writers; a manual external
    // edit to a committed marker is detected on reopening, not hot-reloaded.
    const auto next = generation_ + 1;
    SourceIndex index{next_asset_number_, assets_, visuals_->sprites,
                      visuals_->sheets};
    // Source availability is diagnostic, not authoring validity. Preserve
    // missing/failed source references and their last-good cooked key.
    for (const auto &source : index.records)
      (void)SafeAt(root_, source.relative_path);
    auto archive = detail::ProjectArchive{
        editor_->current_->definition,
        {editor_->used_ids_.begin(), editor_->used_ids_.end()}};
    const auto authored = Take(detail::WriteProject(archive, schema_));
    const auto sources = EncodeSources(index);
    const auto authoring_path = AuthoringPath(next),
               sources_path = SourcesPath(next);
    const auto folder_path = SafeAt(root_, RevisionDir(next));
    std::error_code ec;
    std::filesystem::create_directories(folder_path, ec);
    Require(!ec, "editor.folder_io", "Cannot prepare revision directory");
    Take(Publish(SafeAt(root_, authoring_path), authored));
    Take(Publish(SafeAt(root_, sources_path), sources));
    // Read back staged bytes before writing the commit marker.
    const auto checked_authoring = Take(Read(SafeAt(root_, authoring_path)));
    const auto checked_sources = Take(Read(SafeAt(root_, sources_path)));
    Require(Hash(checked_authoring) == Hash(authored) &&
                Hash(checked_sources) == Hash(sources),
            "editor.folder_integrity", "Staged revision failed verification");
    Commit commit{next,
                  editor_->current_->definition.id.Raw(),
                  editor_->current_->definition.entry_scene.Raw(),
                  Hash(authored),
                  Hash(sources),
                  title_,
                  authoring_path,
                  sources_path};
    const auto bytes = EncodeCommit(commit);
    decltype(editor_->saved_documents_) saved_documents;
    for (const auto &doc : editor_->current_->definition.documents)
      saved_documents.emplace(doc.id.Raw(), doc.revision);
    const auto journal_path = SafeAt(root_, ".epidemic/source-moves.journal");
    const auto descriptor_path = root_ / "Project.epidemicproj";
    // The commit marker is the LAST authoritative write. An interrupted
    // multi-file save leaves only uncommitted orphan revision files.
    Take(Publish(SafeAt(root_, CommitPath(next)), bytes));
    generation_ = next;
    editor_->saved_state_ = editor_->current_->state_id;
    editor_->saved_documents_.swap(saved_documents);
    saved_assets_ = std::move(index.records);
    saved_visuals_ = visuals_;
    asset_dirty_ = false;
    sprite_dirty_ = false;
    pending_moves_.clear();
    {
      std::error_code ignored;
      std::filesystem::remove(journal_path, ignored);
    }
    // Convenience descriptor, not recovery authority. Failure here does not
    // undo an already durable commit and does not turn Save into a false FAIL.
    try {
      (void)Publish(descriptor_path, bytes);
    } catch (const std::exception &) {
      return VoidResult::Success(); /* Commit remains durable. */
    }
    return VoidResult::Success();
  } catch (const Failure &failure) {
    return VoidResult::Failure(failure.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.folder_io", "Project commit failed"));
  }
}
std::vector<ProjectFolder::EditStep>
ProjectFolder::PrepareEdit(EditStep step) const {
  auto result = edits_;
  result.resize(edit_cursor_);
  result.push_back(std::move(step));
  const auto bytes = [](const std::shared_ptr<const VisualState> &state) {
    if (!state)
      return std::size_t{0};
    auto total = state->sprites.size() * sizeof(PrefabSpriteBinding) +
                 state->sheets.size() * sizeof(SpriteSheetDefinition);
    for (const auto &sheet : state->sheets) {
      total += sheet.regions.size() * sizeof(sheets::Region);
      for (const auto &region : sheet.regions)
        total += region.name.size();
    }
    return total;
  };
  auto total = std::size_t{0};
  for (const auto &entry : result)
    total += bytes(entry.before) + bytes(entry.after);
  while (result.size() > 1 &&
         (result.size() > 64 || total > std::size_t{32} * 1024 * 1024)) {
    total -= bytes(result.front().before) + bytes(result.front().after);
    result.erase(result.begin());
  }
  return result;
}
void ProjectFolder::RefreshAssetDirty() noexcept {
  // Records keep insertion order. Undone imports retain new tombstones solely
  // for identity reservation; they do not change the logical save point.
  std::size_t saved = 0;
  asset_dirty_ = false;
  for (const auto &value : assets_) {
    if (saved < saved_assets_.size() && saved_assets_[saved].id == value.id) {
      if (value != saved_assets_[saved++]) {
        asset_dirty_ = true;
        return;
      }
    } else if (!value.IsTrashed()) {
      asset_dirty_ = true;
      return;
    }
  }
  asset_dirty_ = saved != saved_assets_.size();
}
void ProjectFolder::RefreshVisualDirty() noexcept {
  sprite_dirty_ = visuals_ != saved_visuals_ && *visuals_ != *saved_visuals_;
}
Result<EditReceipt> ProjectFolder::Execute(const EditRequest &request) {
  try {
    if (request.expected_revision != editor_->Revision())
      return editor_->Execute(request);
    for (const auto &command : request.commands)
      if (const auto *created = std::get_if<CreateDocument>(&command))
        if (std::any_of(assets_.begin(), assets_.end(),
                        [&](const auto &source) {
                          return source.id == created->document.id;
                        }))
          return Result<EditReceipt>::Failure(
              Error("editor.folder_identity",
                    "Document ID is reserved by a source asset"));
    auto history = PrepareEdit({});
    const auto revision = editor_->Revision();
    auto edited = editor_->Execute(request);
    if (edited && revision != editor_->Revision()) {
      edits_.swap(history);
      edit_cursor_ = edits_.size();
    }
    return edited;
  } catch (const std::exception &) {
    return Result<EditReceipt>::Failure(Error(
        "editor.folder_history", "Cannot prepare document command history"));
  }
}
VoidResult ProjectFolder::RestoreEdit(const EditStep &step, bool forward) {
  if (step.kind == EditStep::Kind::Lifecycle) {
    const auto applied =
        ApplySourceLifecycle(forward ? step.after_source : step.before_source);
    if (!applied)
      return applied;
    const auto &visual = forward ? step.after : step.before;
    if (visual) {
      visuals_ = visual;
      RefreshVisualDirty();
    }
    return applied;
  }
  if (step.kind == EditStep::Kind::Document)
    return forward ? editor_->Redo() : editor_->Undo();
  if (step.kind == EditStep::Kind::Move) {
    const auto moved = ApplySourceMove(
        step.move.id, (forward ? step.move.after : step.move.before).substr(8));
    if (moved)
      ++asset_revision_;
    return moved;
  }
  visuals_ = forward ? step.after : step.before;
  RefreshVisualDirty();
  ++asset_revision_;
  return VoidResult::Success();
}
VoidResult ProjectFolder::Undo() {
  if (!CanUndo())
    return VoidResult::Failure(
        Error("editor.undo_empty", "No project command to undo"));
  const auto restored = RestoreEdit(edits_[edit_cursor_ - 1], false);
  if (restored)
    --edit_cursor_;
  return restored;
}
VoidResult ProjectFolder::Redo() {
  if (!CanRedo())
    return VoidResult::Failure(
        Error("editor.redo_empty", "No project command to redo"));
  const auto restored = RestoreEdit(edits_[edit_cursor_], true);
  if (restored)
    ++edit_cursor_;
  return restored;
}
Result<std::unique_ptr<PlaySession>>
ProjectFolder::Play(PlayOptions options) const {
  using R = Result<std::unique_ptr<PlaySession>>;
  try {
    auto session = Take(editor_->Play(std::move(options)));
    std::vector<PlaySprite> sprites;
    const auto snapshot = Snapshot();
    const auto *entry = snapshot->Find(snapshot->definition.entry_scene);
    const auto *scene =
        entry
            ? std::get_if<composition::SceneCompositionDefinition>(&entry->data)
            : nullptr;
    if (!scene)
      return R::Failure(Error("editor.play_scene", "Entry Scene missing"));
    for (const auto &placement : scene->placements) {
      const auto binding = std::find_if(
          visuals_->sprites.begin(), visuals_->sprites.end(),
          [&](const auto &value) { return value.prefab == placement.prefab; });
      if (binding == visuals_->sprites.end())
        continue;
      const auto source =
          std::find_if(assets_.begin(), assets_.end(), [&](const auto &value) {
            return value.id == binding->texture;
          });
      Require(source != assets_.end() && !source->IsTrashed() &&
                  !source->cooked_key.empty(),
              "editor.play_sprite_source", "Used Sprite source is unavailable");
      PlaySprite sprite;
      sprite.placement = placement.node;
      sprite.texture = source->id;
      sprite.generation = source->generation;
      sprite.cooked_key = source->cooked_key;
      sprite.region_id = binding->region_id;
      sprite.pivot_x = binding->pivot_x;
      sprite.pivot_y = binding->pivot_y;
      sprite.pixels_per_unit = binding->pixels_per_unit;
      if (sprite.region_id) {
        const auto sheet = std::find_if(
            visuals_->sheets.begin(), visuals_->sheets.end(),
            [&](const auto &value) { return value.texture == source->id; });
        Require(sheet != visuals_->sheets.end(), "editor.play_sprite_region",
                "Used sheet is unavailable");
        const auto region = std::find_if(
            sheet->regions.begin(), sheet->regions.end(),
            [&](const auto &value) { return value.id == sprite.region_id; });
        Require(region != sheet->regions.end() && !region->missing,
                "editor.play_sprite_region", "Used region is unavailable");
        sprite.x = region->x;
        sprite.y = region->y;
        sprite.width = region->width;
        sprite.height = region->height;
      }
      sprites.push_back(std::move(sprite));
    }
    Take(session->InstallSprites(root_, std::move(sprites)));
    return R::Success(std::move(session));
  } catch (const Failure &failure) {
    return R::Failure(failure.error);
  } catch (const std::exception &) {
    return R::Failure(
        Error("editor.play_sprite", "Cannot initialize Sprite resources"));
  }
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named
// filesystem/source and mount/path roles intentionally share their public value
// types.
Result<std::filesystem::path>
ProjectFolder::ResolveMount(std::string_view mount,
                            std::string_view relative) const {
  if (mount != "Content" && mount != "Scripts" && mount != "ProjectData" &&
      mount != "Cache")
    return Result<std::filesystem::path>::Failure(
        Error("editor.folder_mount", "Unknown project mount"));
  const auto prefix = mount == "Cache" ? ".epidemic/Cache" : std::string(mount);
  return ResolveProjectPath(root_, prefix + "/" + std::string(relative));
}
Result<runtime::AssetId> ProjectFolder::IndexSource(std::string_view path,
                                                    runtime::AssetType type,
                                                    std::string cooked_key) {
  using R = Result<runtime::AssetId>;
  try {
    Require(!IsPrivateContentPath(path), "editor.import_destination",
            "Reserved source directory");
    Require(type.IsValid() && !path.empty() &&
                path.size() <= runtime::kMaxAssetPathBytes - 8 &&
                next_asset_number_ <
                    std::numeric_limits<std::uint64_t>::max() &&
                assets_.size() < kMaxSources,
            "editor.folder_sources", "Invalid source registration");
    Require(cooked_key.empty() ||
                (cooked_key.size() == 64 &&
                 cooked_key.find_first_not_of("0123456789abcdef") ==
                     std::string::npos),
            "editor.folder_cooked", "Invalid cooked key");
    const auto qualified = "Content/" + std::string(path);
    const auto source = SafeAt(root_, qualified);
    std::error_code ec;
    Require(std::filesystem::is_regular_file(source, ec) && !ec,
            "editor.folder_source", "Source file missing");
    auto folded = qualified;
    std::transform(
        folded.begin(), folded.end(), folded.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto &a : assets_) {
      auto old = a.relative_path;
      std::transform(old.begin(), old.end(), old.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
      });
      Require(old != folded, "editor.folder_sources",
              "Source path already indexed");
    }
    runtime::AssetId id{};
    do {
      Require(next_asset_number_ < std::numeric_limits<std::uint64_t>::max(),
              "editor.folder_sources", "Source identity counter exhausted");
      const auto key =
          "epidemic.project.asset/" +
          std::to_string(editor_->Snapshot()->definition.id.Raw()) + "/" +
          std::to_string(next_asset_number_++);
      id = runtime::AssetId{foundation::StringId::FromString(key)};
    } while (!id.IsValid() || editor_->used_ids_.contains({id.Raw(), 0}) ||
             editor_->Snapshot()->assets->Contains(id) ||
             std::any_of(assets_.begin(), assets_.end(),
                         [&](const auto &a) { return a.id == id; }));
    assets_.push_back({id, type, qualified, HashFileStream(source), 1,
                       std::move(cooked_key)});
    asset_dirty_ = true;
    ++asset_revision_;
    return R::Success(id);
  } catch (const Failure &f) {
    return R::Failure(f.error);
  } catch (const std::exception &) {
    return R::Failure(Error("editor.folder_source", "Cannot index source"));
  }
}
VoidResult ProjectFolder::ReindexSource(runtime::AssetId id) {
  try {
    const auto it = std::find_if(assets_.begin(), assets_.end(),
                                 [&](const auto &a) { return a.id == id; });
    Require(it != assets_.end() &&
                it->generation < std::numeric_limits<std::uint32_t>::max(),
            "editor.folder_sources", "Unknown source or generation overflow");
    const auto source = SafeAt(root_, it->relative_path);
    const auto next_hash = HashFileStream(source);
    if (next_hash != it->source_hash) {
      it->source_hash = next_hash;
      ++it->generation;
      asset_dirty_ = true;
    }
    return VoidResult::Success();
  } catch (const Failure &f) {
    return VoidResult::Failure(f.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.folder_source", "Source refresh failed"));
  }
}
VoidResult ProjectFolder::PublishCooked(runtime::AssetId id, std::string key) {
  try {
    Require(key.size() == 64 &&
                key.find_first_not_of("0123456789abcdef") == std::string::npos,
            "editor.folder_cooked", "Invalid cooked digest");
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == id; });
    Require(found != assets_.end() &&
                found->generation < std::numeric_limits<std::uint32_t>::max(),
            "editor.folder_cooked", "Unknown source or generation overflow");
    const auto hash = HashFileStream(SafeAt(root_, found->relative_path));
    if (found->cooked_key != key || found->source_hash != hash) {
      found->cooked_key.swap(key);
      found->source_hash = hash;
      ++found->generation;
      asset_dirty_ = true;
      ++asset_revision_;
    }
    return VoidResult::Success();
  } catch (const Failure &f) {
    return VoidResult::Failure(f.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.folder_cooked", "Cannot publish cooked candidate"));
  }
}
VoidResult ProjectFolder::PublishCooked(runtime::AssetId id,
                                        const PreparedCookedPng &candidate) {
  try {
    Require(candidate.source_read_ && candidate.root_ == root_ &&
                candidate.key_.size() == 64 &&
                candidate.key_.find_first_not_of("0123456789abcdef") ==
                    std::string::npos,
            "editor.folder_cooked", "Invalid prepared source lease");
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == id; });
    Require(found != assets_.end() && !found->IsTrashed() &&
                found->relative_path == "Content/" + candidate.relative_ &&
                found->type ==
                    runtime::AssetType{foundation::StringId::FromString(
                        "epidemic.texture.png")} &&
                found->generation < std::numeric_limits<std::uint32_t>::max(),
            "editor.folder_cooked", "Prepared source identity changed");
    if (found->cooked_key != candidate.key_ ||
        found->source_hash != candidate.source_hash_) {
      auto key =
          candidate.key_; // Allocate before changing any published field.
      found->cooked_key.swap(key);
      found->source_hash = candidate.source_hash_;
      ++found->generation;
      asset_dirty_ = true;
      ++asset_revision_;
    }
    return VoidResult::Success();
  } catch (const Failure &f) {
    return VoidResult::Failure(f.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.folder_cooked", "Cannot publish prepared candidate"));
  }
}
VoidResult ProjectFolder::ApplySourceMove(runtime::AssetId id,
                                          std::string_view new_content_path) {
  try {
    Require(!new_content_path.empty() &&
                new_content_path.size() < runtime::kMaxAssetPathBytes - 8,
            "editor.folder_path", "Invalid destination path");
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == id; });
    Require(found != assets_.end(), "editor.folder_sources",
            "Unknown asset ID");
    auto relative = "Content/" + std::string(new_content_path);
    const auto old_path = SafeAt(root_, found->relative_path);
    const auto new_path = SafeAt(root_, relative);
    Require(old_path != new_path, "editor.folder_path",
            "Destination matches source");
    auto folded = relative;
    std::transform(
        folded.begin(), folded.end(), folded.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto &asset : assets_)
      if (asset.id != id) {
        auto other = asset.relative_path;
        std::transform(
            other.begin(), other.end(), other.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        Require(other != folded, "editor.folder_path",
                "Destination conflicts with another asset");
      }
    std::error_code ec;
    Require(std::filesystem::is_regular_file(old_path, ec) && !ec,
            "editor.folder_source", "Missing source file");
    Require(!std::filesystem::exists(new_path, ec) && !ec, "editor.folder_path",
            "Destination exists");
    Require(std::filesystem::is_directory(new_path.parent_path(), ec) && !ec,
            "editor.folder_path", "Destination directory does not exist");
    Require(pending_moves_.size() < 128, "editor.folder_journal_limit",
            "Save before more source moves");
    auto pending = pending_moves_;
    pending.emplace_back(found->relative_path, relative);
    Take(Publish(SafeAt(root_, ".epidemic/source-moves.journal"),
                 EncodeMoves(generation_, pending)));
    pending_moves_.swap(pending);
    std::filesystem::rename(old_path, new_path, ec);
    Require(!ec, "editor.folder_io", "Cannot move source file");
    found->relative_path.swap(relative);
    RefreshAssetDirty();
    return VoidResult::Success();
  } catch (const Failure &f) {
    return VoidResult::Failure(f.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(Error("editor.folder_io", "Move source failed"));
  }
}
VoidResult ProjectFolder::MoveSource(runtime::AssetId id,
                                     std::string_view relative) {
  try {
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == id; });
    Require(found != assets_.end() && !found->IsTrashed(),
            "editor.folder_move_history", "Unknown active asset");
    Require(!IsPrivateContentPath(relative), "editor.import_destination",
            "Reserved source directory");
    EditStep step;
    step.kind = EditStep::Kind::Move;
    step.move = {id, found->relative_path, "Content/" + std::string(relative)};
    auto history = PrepareEdit(std::move(step));
    const auto moved = ApplySourceMove(id, relative);
    if (!moved)
      return moved;
    edits_.swap(history);
    edit_cursor_ = edits_.size();
    ++asset_revision_;
    return VoidResult::Success();
  } catch (const Failure &failure) {
    return VoidResult::Failure(failure.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.folder_move_history", "Cannot prepare move history"));
  }
}
VoidResult ProjectFolder::UndoAssetMove() {
  if (!CanUndo() || edits_[edit_cursor_ - 1].kind != EditStep::Kind::Move)
    return VoidResult::Failure(Error("editor.folder_move_history",
                                     "The last command is not a source move"));
  return Undo();
}
VoidResult ProjectFolder::RedoAssetMove() {
  if (!CanRedo() || edits_[edit_cursor_].kind != EditStep::Kind::Move)
    return VoidResult::Failure(Error("editor.folder_move_history",
                                     "The next command is not a source move"));
  return Redo();
}
VoidResult
ProjectFolder::ApplySourceLifecycle(const ProjectSourceAsset &target) {
  try {
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == target.id; });
    Require(found != assets_.end(), "editor.asset_unknown",
            "Source asset not found");
    auto original = target.original_path;
    auto checked = assets_;
    const auto index = static_cast<std::size_t>(found - assets_.begin());
    checked[index].relative_path = target.relative_path;
    checked[index].original_path = target.original_path;
    (void)DecodeSources(EncodeSources(
        {next_asset_number_, checked, visuals_->sprites, visuals_->sheets}));
    const auto destination = SafeAt(root_, target.relative_path);
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    Require(!error, "editor.asset_restore",
            "Cannot prepare source destination");
    (void)SafeAt(root_,
                 target.relative_path); // Re-check newly created ancestors.
    if (found->relative_path != target.relative_path) {
      const auto moved =
          ApplySourceMove(target.id, target.relative_path.substr(8));
      if (!moved)
        return moved;
    }
    found->original_path.swap(original);
    RefreshAssetDirty();
    ++asset_revision_;
    return VoidResult::Success();
  } catch (const Failure &failure) {
    return VoidResult::Failure(failure.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.asset_restore", "Cannot restore source disposition"));
  }
}
Result<runtime::AssetId> ProjectFolder::CommitImportedPng(
    PreparedCookedPng &&candidate, std::string_view destination,
    runtime::AssetId duplicate_of, std::uint64_t expected_duplicate_revision) {
  using R = Result<runtime::AssetId>;
  try {
    Require(candidate.root_ == root_ && candidate.staging_cleanup_ &&
                candidate.source_read_.use_count() == 1 &&
                candidate.relative_.starts_with(".imports/"),
            "editor.import_proof",
            "Import requires exclusively owned staged source");
    Require(!destination.empty() && !IsPrivateContentPath(destination),
            "editor.import_destination", "Invalid destination");
    const auto target = SafeAt(root_, "Content/" + std::string(destination));
    std::error_code error;
    Require(!std::filesystem::exists(target, error) && !error &&
                std::filesystem::is_directory(target.parent_path(), error) &&
                !error,
            "editor.import_destination",
            "Destination exists or parent missing");
    auto counter = next_asset_number_;
    runtime::AssetId id;
    do {
      Require(counter < std::numeric_limits<std::uint64_t>::max(),
              "editor.folder_sources", "Identity counter exhausted");
      id = runtime::AssetId::FromString(
          "epidemic.project.asset/" +
          std::to_string(Snapshot()->definition.id.Raw()) + "/" +
          std::to_string(counter++));
    } while (!id.IsValid() || editor_->used_ids_.contains({id.Raw(), 0}) ||
             Snapshot()->assets->Contains(id) ||
             std::any_of(assets_.begin(), assets_.end(),
                         [&](const auto &source) { return source.id == id; }));
    ProjectSourceAsset record{
        id,
        {foundation::StringId::FromString("epidemic.texture.png")},
        "Content/" + std::string(destination),
        candidate.source_hash_,
        1,
        candidate.key_,
        {}};
    auto records = assets_;
    records.push_back(record);
    auto visual = std::make_shared<VisualState>(*visuals_);
    if (duplicate_of.IsValid()) {
      Require(expected_duplicate_revision == asset_revision_,
              "editor.duplicate_stale",
              "Asset metadata changed during duplicate; retry");
      const auto source =
          std::find_if(assets_.begin(), assets_.end(), [&](const auto &asset) {
            return asset.id == duplicate_of;
          });
      Require(source != assets_.end() && !source->IsTrashed() &&
                  source->type == record.type,
              "editor.duplicate_source", "Duplicate source no longer exists");
      Require(source->source_hash == candidate.source_hash_,
              "editor.duplicate_stale",
              "Duplicate source fingerprint changed; reimport and retry");
      const auto sheet = std::find_if(
          visuals_->sheets.begin(), visuals_->sheets.end(),
          [&](const auto &value) { return value.texture == duplicate_of; });
      if (sheet != visuals_->sheets.end())
        visual->sheets.push_back({id, sheet->regions});
    }
    (void)DecodeSources(
        EncodeSources({counter, records, visual->sprites, visual->sheets}));
    EditStep step;
    step.kind = EditStep::Kind::Lifecycle;
    step.after_source = record;
    step.before_source = record;
    step.before_source.original_path = record.relative_path;
    step.before_source.relative_path = "Content/.Trash/" +
                                       std::to_string(id.Raw()) + "/" +
                                       target.filename().generic_string();
    step.before = visuals_;
    step.after = visual;
    auto history = PrepareEdit(std::move(step));
    const auto stage = SafeAt(root_, "Content/" + candidate.relative_);
    candidate.source_read_.reset(); // Finished worker; allow the atomic rename.
    std::filesystem::rename(stage, target, error);
    Require(!error, "editor.import_commit",
            "Cannot publish staged source file");
    assets_.swap(records);
    next_asset_number_ = counter;
    visuals_ = std::move(visual);
    edits_.swap(history);
    edit_cursor_ = edits_.size();
    asset_dirty_ = true;
    RefreshVisualDirty();
    ++asset_revision_;
    return R::Success(id);
  } catch (const Failure &failure) {
    return R::Failure(failure.error);
  } catch (const std::exception &) {
    return R::Failure(Error("editor.import_commit", "Cannot publish import"));
  }
}
VoidResult ProjectFolder::StageDeleteSource(runtime::AssetId id,
                                            bool allow_broken_references) {
  try {
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == id; });
    Require(found != assets_.end() && !found->IsTrashed(),
            "editor.asset_unknown", "Active source not found");
    Require(allow_broken_references ||
                BuildContentDependencyIndex(*this).Incoming(id).empty(),
            "editor.asset_referenced",
            "Source has incoming references; replace or explicitly keep broken "
            "references");
    EditStep step;
    step.kind = EditStep::Kind::Lifecycle;
    step.before_source = *found;
    step.after_source = *found;
    step.after_source.original_path = found->relative_path;
    step.after_source.relative_path =
        "Content/.Trash/" + std::to_string(id.Raw()) + "/" +
        std::filesystem::path(found->relative_path).filename().generic_string();
    auto history = PrepareEdit(step);
    const auto removed = ApplySourceLifecycle(step.after_source);
    if (!removed)
      return removed;
    edits_.swap(history);
    edit_cursor_ = edits_.size();
    return VoidResult::Success();
  } catch (const Failure &failure) {
    return VoidResult::Failure(failure.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.asset_delete", "Cannot stage source deletion"));
  }
}
VoidResult ProjectFolder::RestoreSource(runtime::AssetId id) {
  try {
    const auto found =
        std::find_if(assets_.begin(), assets_.end(),
                     [&](const auto &asset) { return asset.id == id; });
    Require(found != assets_.end() && found->IsTrashed(),
            "editor.asset_unknown", "Trashed source not found");
    EditStep step;
    step.kind = EditStep::Kind::Lifecycle;
    step.before_source = *found;
    step.after_source = *found;
    step.after_source.relative_path = found->original_path;
    step.after_source.original_path.clear();
    auto history = PrepareEdit(step);
    const auto restored = ApplySourceLifecycle(step.after_source);
    if (!restored)
      return restored;
    edits_.swap(history);
    edit_cursor_ = edits_.size();
    return VoidResult::Success();
  } catch (const Failure &failure) {
    return VoidResult::Failure(failure.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.asset_restore", "Cannot restore staged source"));
  }
}
VoidResult ProjectFolder::SetPrefabSprite(runtime::AssetId prefab,
                                          runtime::AssetId texture,
                                          float pivot_x, float pivot_y,
                                          float pixels_per_unit) {
  return SetPrefabSpriteRegion(prefab, texture, 0, pivot_x, pivot_y,
                               pixels_per_unit);
}
VoidResult ProjectFolder::SetPrefabSpriteRegion(runtime::AssetId prefab,
                                                runtime::AssetId texture,
                                                std::uint64_t region_id,
                                                float pivot_x, float pivot_y,
                                                float pixels_per_unit) {
  return Execute(
      AssetEditRequest{asset_revision_,
                       {AssetSetSprite{{prefab, texture, region_id, pivot_x,
                                        pivot_y, pixels_per_unit}}}});
}
VoidResult ProjectFolder::SetSpriteSheet(runtime::AssetId texture,
                                         std::vector<sheets::Region> regions,
                                         std::uint32_t width,
                                         std::uint32_t height) {
  return Execute(AssetEditRequest{
      asset_revision_,
      {AssetSetSheet{{texture, std::move(regions)}, width, height}}});
}
VoidResult ProjectFolder::Execute(const AssetEditRequest &request) {
  try {
    Require(request.expected_revision == asset_revision_,
            "editor.asset_revision", "Asset view is stale");
    Require(!request.commands.empty() && request.commands.size() <= 128,
            "editor.asset_commands", "Invalid asset command count");
    auto candidate = std::make_shared<VisualState>(*visuals_);
    for (const auto &command : request.commands) {
      if (const auto *set = std::get_if<AssetSetSprite>(&command)) {
        const auto &binding = set->binding;
        Require(binding.prefab.IsValid() && std::isfinite(binding.pivot_x) &&
                    std::isfinite(binding.pivot_y) && binding.pivot_x >= 0 &&
                    binding.pivot_x <= 1 && binding.pivot_y >= 0 &&
                    binding.pivot_y <= 1 &&
                    std::isfinite(binding.pixels_per_unit) &&
                    binding.pixels_per_unit > 0 &&
                    binding.pixels_per_unit <= 4096,
                "editor.sprite_binding", "Invalid Sprite parameters");
        const auto *document = Snapshot()->Find(binding.prefab);
        Require(document && document->Kind() == DocumentKind::Prefab,
                "editor.sprite_prefab", "Prefab not found");
        if (binding.texture.IsValid()) {
          const auto source = std::find_if(
              assets_.begin(), assets_.end(),
              [&](const auto &asset) { return asset.id == binding.texture; });
          Require(source != assets_.end() && !source->IsTrashed() &&
                      source->type ==
                          runtime::AssetType{foundation::StringId::FromString(
                              "epidemic.texture.png")} &&
                      !source->cooked_key.empty(),
                  "editor.sprite_texture", "Sprite requires cooked PNG");
          if (binding.region_id) {
            const auto sheet =
                std::find_if(candidate->sheets.begin(), candidate->sheets.end(),
                             [&](const auto &value) {
                               return value.texture == binding.texture;
                             });
            Require(sheet != candidate->sheets.end() &&
                        std::any_of(sheet->regions.begin(),
                                    sheet->regions.end(),
                                    [&](const auto &region) {
                                      return region.id == binding.region_id &&
                                             !region.missing;
                                    }),
                    "editor.sprite_region", "Region missing or retired");
          }
        }
        std::erase_if(candidate->sprites, [&](const auto &value) {
          return value.prefab == binding.prefab;
        });
        if (binding.texture.IsValid())
          candidate->sprites.push_back(binding);
      } else {
        const auto &sheet_command = std::get<AssetSetSheet>(command);
        auto regions = sheet_command.sheet.regions;
        const auto source = std::find_if(
            assets_.begin(), assets_.end(), [&](const auto &asset) {
              return asset.id == sheet_command.sheet.texture;
            });
        Require(source != assets_.end() && !source->IsTrashed() &&
                    source->type ==
                        runtime::AssetType{foundation::StringId::FromString(
                            "epidemic.texture.png")},
                "editor.sheet_parent", "Sheet requires indexed PNG");
        const auto png = Take(DecodePng(SafeAt(root_, source->relative_path)));
        Require(png.width == sheet_command.width &&
                    png.height == sheet_command.height,
                "editor.sheet_source", "Sheet dimensions differ from source");
        const auto validated = sheets::Validate(sheet_command.width,
                                                sheet_command.height, regions);
        Require(static_cast<bool>(validated), "editor.sheet_regions",
                validated.error);
        const auto old =
            std::find_if(candidate->sheets.begin(), candidate->sheets.end(),
                         [&](const auto &value) {
                           return value.texture == sheet_command.sheet.texture;
                         });
        if (old != candidate->sheets.end()) {
          auto reconciled = sheets::Reconcile(old->regions, regions);
          Require(static_cast<bool>(reconciled), "editor.sheet_reconcile",
                  reconciled.error);
          old->regions = std::move(reconciled.value);
        } else
          candidate->sheets.push_back(
              {sheet_command.sheet.texture, std::move(regions)});
      }
    }
    std::sort(candidate->sprites.begin(), candidate->sprites.end(),
              [](const auto &a, const auto &b) {
                return a.prefab.Raw() < b.prefab.Raw();
              });
    std::sort(candidate->sheets.begin(), candidate->sheets.end(),
              [](const auto &a, const auto &b) {
                return a.texture.Raw() < b.texture.Raw();
              });
    auto canonical = DecodeSources(EncodeSources(
        {next_asset_number_, assets_, candidate->sprites, candidate->sheets}));
    candidate->sprites = std::move(canonical.sprites);
    candidate->sheets = std::move(canonical.sheets);
    if (*candidate == *visuals_)
      return VoidResult::Success();
    EditStep step;
    step.kind = EditStep::Kind::Visual;
    step.before = visuals_;
    step.after = candidate;
    auto history = PrepareEdit(std::move(step));
    visuals_ = std::move(candidate);
    edits_.swap(history);
    edit_cursor_ = edits_.size();
    ++asset_revision_;
    RefreshVisualDirty();
    return VoidResult::Success();
  } catch (const Failure &failure) {
    return VoidResult::Failure(failure.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.asset_command_failed", "Asset transaction rejected"));
  }
}
Result<std::shared_ptr<const runtime::IAssetCatalog>>
ProjectFolder::BuildSourceCatalog() const {
  using R = Result<std::shared_ptr<const runtime::IAssetCatalog>>;
  auto catalog = runtime::CreateAssetServices();
  if (!catalog)
    return R::Failure(catalog.GetError());
  for (const auto &a : assets_) {
    if (a.IsTrashed())
      continue;
    runtime::AssetMetadata metadata;
    metadata.id = a.id;
    metadata.type = a.type;
    metadata.location = {runtime::AssetLocationKind::FilePath, a.relative_path};
    metadata.state = runtime::AssetState::Indexed;
    metadata.content_hash = a.source_hash;
    metadata.version = static_cast<std::uint32_t>(a.generation);
    auto accepted = catalog.Value().writer->RegisterAsset(metadata);
    if (!accepted)
      return R::Failure(accepted.GetError());
  }
  auto sealed = catalog.Value().writer->Seal();
  if (!sealed)
    return R::Failure(sealed.GetError());
  return R::Success(catalog.Value().catalog);
}
VoidResult
ProjectFolder::CloneTo(const std::filesystem::path &destination) const {
  try {
    Require(!IsDirty(), "editor.folder_dirty", "Save before cloning a project");
    std::error_code ec;
    const auto target_candidate =
        std::filesystem::weakly_canonical(destination, ec);
    Require(!ec && !IsSubpath(root_, target_candidate), "editor.folder_clone",
            "Clone destination may not be inside the source folder");
    Require(!std::filesystem::exists(destination, ec) && !ec,
            "editor.folder_exists", "Clone destination must not exist");
    std::filesystem::create_directories(destination, ec);
    Require(!ec, "editor.folder_clone", "Cannot create clone directory");
    const auto target = RootPath(destination);
    EnsureDirectories(target);
    const auto marker = SafeAt(root_, CommitPath(generation_));
    auto commit = RestoreCommit(root_, marker, schema_);
    const auto copy = [&](std::string_view p) {
      const auto src = SafeAt(root_, p);
      const auto dst = SafeAt(target, p);
      std::filesystem::create_directories(dst.parent_path(), ec);
      Require(!ec, "editor.folder_clone", "Cannot prepare clone path");
      std::filesystem::copy_file(src, dst, std::filesystem::copy_options::none,
                                 ec);
      Require(!ec, "editor.folder_clone", "Cannot copy committed project file");
    };
    copy(AuthoringPath(generation_));
    copy(SourcesPath(generation_));
    CopyTree(SafeAt(root_, "Content"), SafeAt(target, "Content"), true);
    CopyTree(SafeAt(root_, "Scripts"), SafeAt(target, "Scripts"));
    CopyTree(SafeAt(root_, ".epidemic/Cache/images"),
             SafeAt(target, ".epidemic/Cache/images"));
    // Last-good cooked data remains portable even if source is missing/corrupt.
    for (const auto &source : commit.index.records)
      (void)SafeAt(target, source.relative_path);
    // A clone interrupted before this point is not a committed project.
    const auto bytes = Take(Read(marker));
    const auto descriptor_path = target / "Project.epidemicproj";
    Take(Publish(target / CommitPath(generation_), bytes));
    try {
      (void)Publish(descriptor_path, bytes);
    } catch (const std::exception &) {
      return VoidResult::Success(); /* Clone marker is already durable. */
    }
    return VoidResult::Success();
  } catch (const Failure &f) {
    return VoidResult::Failure(f.error);
  } catch (const std::exception &) {
    return VoidResult::Failure(
        Error("editor.folder_clone", "Cannot clone project folder"));
  }
}
} // namespace epidemic::game::editor
