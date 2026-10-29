#include "Epidemic/Game/Editor/project_folder.h"
#include "Epidemic/Game/Editor/project_path.h"
#include "project_archive.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Serialization/document_codec.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <exception>
#include <span>
#include <fstream>
#include <functional>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <system_error>
#include <utility>
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
foundation::Error Error(std::string_view code, std::string_view why) { return foundation::Error::Create(code,why); }
void Require(bool value, std::string_view code, std::string_view why) {
    if (!value) throw Failure{Error(code,why)};
}
template<class T> T Take(Result<T> result) {
    if (!result) throw Failure{result.GetError()};
    return std::move(result).Value();
}
void Take(const VoidResult &result) { if (!result) throw Failure{result.GetError()}; }
constexpr std::uint64_t kMaxSources = 16384;
const auto kCommitType=foundation::StringId::FromString("epidemic.editor.folder_commit");
const auto kSourcesType=foundation::StringId::FromString("epidemic.editor.source_index");
constexpr runtime::SchemaVersion kVersion{1,0,0};

struct Commit {
    std::uint64_t generation=0, project=0, entry_scene=0, authoring_hash=0, assets_hash=0;
    std::string title, authoring, assets;
};
struct SourceIndex {
    std::uint64_t next=1;
    std::vector<ProjectSourceAsset> records;
};
std::uint64_t Hash(std::span<const std::byte> data) {
    std::uint64_t h=14695981039346656037ULL;
    for (auto b:data) { h^=static_cast<std::uint8_t>(b); h*=1099511628211ULL; }
    return h;
}
std::string RevisionName(std::uint64_t value) {
    // Fixed-width decimal sorts lexically; no locale/path dependence.
    std::string s=std::to_string(value);
    return std::string(s.size()<16?16-s.size():0,'0')+s;
}
std::string RevisionDir(std::uint64_t g) { return "ProjectData/Revisions/"+RevisionName(g); }
std::string AuthoringPath(std::uint64_t g) { return RevisionDir(g)+"/Authoring.epidemic"; }
std::string SourcesPath(std::uint64_t g) { return RevisionDir(g)+"/Assets.manifest"; }
std::string CommitPath(std::uint64_t g) { return "ProjectData/Commits/"+RevisionName(g)+".commit"; }
void CheckTitle(std::string_view title) {
    Require(!title.empty() && title.size()<=256 && std::none_of(title.begin(),title.end(),[](unsigned char ch){return ch==0 || ch<32;}),
        "editor.folder_title","Project title must be nonempty, <=256 bytes and contain no controls");
}
Bytes EncodeCommit(const Commit &c) {
    auto services=Take(runtime::CreateSerializationServices());
    auto w=services.archives->CreateWriter(); Require(w!=nullptr,"editor.folder_codec","No archive writer");
    Take(w->WriteUInt64("generation",c.generation)); Take(w->WriteUInt64("project",c.project));
    Take(w->WriteUInt64("entry_scene",c.entry_scene)); Take(w->WriteString("title",c.title));
    Take(w->WriteString("authoring",c.authoring)); Take(w->WriteUInt64("authoring_hash",c.authoring_hash));
    Take(w->WriteString("assets",c.assets)); Take(w->WriteUInt64("assets_hash",c.assets_hash));
    return Take(runtime::EncodeDocument(Take(w->Finalize(kCommitType,kVersion))));
}
Commit DecodeCommit(std::span<const std::byte> data) {
    auto doc=Take(runtime::DecodeDocument(data));
    Require(doc.GetTypeId()==kCommitType,"editor.folder_commit","Wrong commit document type");
    Require(doc.GetSchemaVersion()==kVersion,"editor.folder_version","Unsupported project descriptor version");
    auto services=Take(runtime::CreateSerializationServices());auto r=Take(services.archives->CreateReader(doc));
    Commit c;
    c.generation=Take(r->ReadUInt64("generation")); c.project=Take(r->ReadUInt64("project"));
    c.entry_scene=Take(r->ReadUInt64("entry_scene")); c.title=Take(r->ReadString("title"));
    c.authoring=Take(r->ReadString("authoring")); c.authoring_hash=Take(r->ReadUInt64("authoring_hash"));
    c.assets=Take(r->ReadString("assets")); c.assets_hash=Take(r->ReadUInt64("assets_hash"));
    CheckTitle(c.title);
    Require(c.generation>0 && c.project>0 && c.entry_scene>0 && c.authoring==AuthoringPath(c.generation) && c.assets==SourcesPath(c.generation),
        "editor.folder_commit","Commit references are inconsistent");
    return c;
}
Bytes EncodeSources(const SourceIndex &index) {
    Require(index.next>0 && index.records.size()<=kMaxSources,"editor.folder_sources","Invalid source index size");
    auto services=Take(runtime::CreateSerializationServices());auto w=services.archives->CreateWriter();
    Require(w!=nullptr,"editor.folder_codec","No archive writer");
    Take(w->WriteUInt64("next",index.next));
    Take(w->BeginArray("sources",index.records.size()));
    for(std::size_t i=0;i<index.records.size();++i) {
        const auto &a=index.records[i]; Take(w->BeginArrayElement(i));
        Take(w->WriteUInt64("id",a.id.Raw())); Take(w->WriteUInt64("type",a.type.value.Raw()));
        Take(w->WriteString("path",a.relative_path));Take(w->WriteUInt64("hash",a.source_hash));
        Take(w->WriteUInt64("generation",a.generation)); Take(w->EndArrayElement());
    }
    Take(w->EndArray());
    return Take(runtime::EncodeDocument(Take(w->Finalize(kSourcesType,kVersion))));
}
SourceIndex DecodeSources(std::span<const std::byte> data) {
    auto doc=Take(runtime::DecodeDocument(data));
    Require(doc.GetTypeId()==kSourcesType,"editor.folder_sources","Wrong source index type");
    Require(doc.GetSchemaVersion()==kVersion,"editor.folder_version","Unsupported source index version");
    auto services=Take(runtime::CreateSerializationServices()); auto r=Take(services.archives->CreateReader(doc));
    SourceIndex index; index.next=Take(r->ReadUInt64("next"));
    const auto count=Take(r->BeginArray("sources"));
    Require(index.next>0 && count<=kMaxSources,"editor.folder_sources","Source index bounds exceeded");
    std::set<std::uint64_t> ids; std::set<std::string> paths;
    for(std::size_t i=0;i<count;++i) {
        Take(r->BeginArrayElement(i));
        ProjectSourceAsset a;
        a.id=runtime::AssetId{foundation::StringId{Take(r->ReadUInt64("id"))}};
        a.type=runtime::AssetType{foundation::StringId{Take(r->ReadUInt64("type"))}};
        a.relative_path=Take(r->ReadString("path")); a.source_hash=Take(r->ReadUInt64("hash"));
        a.generation=Take(r->ReadUInt64("generation"));Take(r->EndArrayElement());
        Require(a.id.IsValid() && a.type.IsValid() && a.generation>0 &&
                a.generation<=std::numeric_limits<std::uint32_t>::max() &&
                a.relative_path.starts_with("Content/") && a.relative_path.size()<=runtime::kMaxAssetPathBytes,
                "editor.folder_sources","Invalid source asset metadata");
        auto folded=a.relative_path;
        std::transform(folded.begin(),folded.end(),folded.begin(),[](unsigned char ch){return static_cast<char>(std::tolower(ch));});
        Require(ids.insert(a.id.Raw()).second && paths.insert(folded).second,
                "editor.folder_sources","Duplicate source identity or case-insensitive path");
        index.records.push_back(std::move(a));
    }
    Take(r->EndArray());
    return index;
}
Result<Bytes> Read(const std::filesystem::path &p) {
    return platform::ReadFilePackage(foundation::Path{p},runtime::kMaxDocumentBytes);
}
VoidResult Publish(const std::filesystem::path &p,std::span<const std::byte> bytes) {
    return platform::CommitFilePackage(foundation::Path{p},bytes,runtime::kMaxDocumentBytes);
}
void EnsureDirectories(const std::filesystem::path &root) {
    for(const auto *path:{"Content","Scripts","ProjectData/Revisions","ProjectData/Commits",".epidemic"}) {
        std::error_code ec;
        std::filesystem::create_directories(root/path,ec);
        Require(!ec,"editor.folder_io","Cannot create project directory");
    }
}
std::filesystem::path RootPath(const std::filesystem::path &root) {
    std::error_code ec;
    auto absolute=std::filesystem::canonical(root,ec);
    Require(!ec && std::filesystem::is_directory(absolute),"editor.folder_root","Project root does not exist");
    return absolute;
}
std::filesystem::path SafeAt(const std::filesystem::path &root,std::string_view rel) {
    return Take(ResolveProjectPath(root,rel));
}
std::uint64_t HashFileStream(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);
    Require(in.good(),"editor.folder_source","Cannot read source file");
    std::uint64_t h=14695981039346656037ULL;
    std::array<char,65536> bytes{};
    while(in) { in.read(bytes.data(),static_cast<std::streamsize>(bytes.size()));
        for(std::streamsize i=0;i<in.gcount();++i){h^=static_cast<unsigned char>(bytes[static_cast<std::size_t>(i)]);h*=1099511628211ULL;}
    }
    Require(in.eof() && !in.bad(),"editor.folder_source","Source file read failed");
    return h;
}
void CheckSourcesExist(const std::filesystem::path &root,const SourceIndex &index) {
    for(const auto &a:index.records) {
        const auto file=SafeAt(root,a.relative_path);std::error_code ec;
        Require(std::filesystem::is_regular_file(file,ec) && !ec,"editor.folder_source","Indexed source file is missing");
        Require(HashFileStream(file)==a.source_hash,"editor.folder_source","Indexed source hash changed; reindex before commit");
    }
}
struct Restored {
    Commit commit;
    SourceIndex index;
    std::unique_ptr<EditorProject> editor;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named filesystem/source and mount/path roles intentionally share their public value types.
Restored RestoreCommit(const std::filesystem::path &root,const std::filesystem::path &file,const ProjectSchema &schema) {
    const auto commit_bytes=Take(Read(file));const auto c=DecodeCommit(commit_bytes);
    Require(file.filename()==std::filesystem::path{RevisionName(c.generation)+".commit"},"editor.folder_commit","Commit filename mismatch");
    const auto authoring_file=SafeAt(root,c.authoring),source_file=SafeAt(root,c.assets);
    const auto authoring=Take(Read(authoring_file));const auto sources=Take(Read(source_file));
    Require(Hash(authoring)==c.authoring_hash && Hash(sources)==c.assets_hash,"editor.folder_integrity","Commit file hashes do not match");
    auto index=DecodeSources(sources);
    // Fail on malformed index/path without interpreting arbitrary metadata as host paths.
    for(const auto &a:index.records) (void)SafeAt(root,a.relative_path);
    auto loaded=EditorProject::Open(authoring_file,schema);
    // A valid committed byte image that cannot be interpreted by the current
    // game schema must not silently downgrade to an older snapshot.
    Require(static_cast<bool>(loaded),"editor.folder_schema","Committed authoring cannot open with current schema");
    auto editor=std::move(loaded).Value();
    Require(editor->Snapshot()->definition.id.Raw()==c.project &&
            editor->Snapshot()->definition.entry_scene.Raw()==c.entry_scene,
            "editor.folder_integrity","Authoring project identity/entry do not match commit");
    return {c,std::move(index),std::move(editor)};
}
std::vector<std::filesystem::path> Commits(const std::filesystem::path &root) {
    std::vector<std::filesystem::path> result;
    std::error_code ec;
    const auto dir=root/"ProjectData/Commits";
    if(!std::filesystem::is_directory(dir,ec) || ec)return result;
    for(const auto &item:std::filesystem::directory_iterator(dir)) {
        const auto name=item.path().filename().string();
        if(name.size()!=23 || !name.ends_with(".commit") ||
           !std::all_of(name.begin(),name.begin()+16,[](char c){return c>='0' && c<='9';}))continue;
        const auto status=item.symlink_status(ec);
        if(ec || !std::filesystem::is_regular_file(status))continue;
        result.push_back(item.path());
    }
    std::sort(result.begin(),result.end(),std::greater<>{});
    return result;
}
bool IsSubpath(const std::filesystem::path &root,const std::filesystem::path &candidate) {
    auto a=root.begin(), b=candidate.begin();
    for(;a!=root.end();++a,++b) if(b==candidate.end() || *a!=*b)return false;
    return true;
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named filesystem/source and mount/path roles intentionally share their public value types.
void CopyTree(const std::filesystem::path &src,const std::filesystem::path &dest) {
    std::error_code ec;
    if(!std::filesystem::exists(src,ec))return;
    Require(!ec && std::filesystem::is_directory(src,ec),"editor.folder_clone","Source mount is not a directory");
    for(const auto &entry:std::filesystem::recursive_directory_iterator(src)) {
        const auto status=entry.symlink_status(ec);
        Require(!ec && !std::filesystem::is_symlink(status),"editor.folder_clone","Symlink in portable source tree");
        const auto relative=entry.path().lexically_relative(src);
        Require(!relative.empty(),"editor.folder_clone","Invalid source path");
        const auto output=dest/relative;
        if(std::filesystem::is_directory(status)) {
            std::filesystem::create_directories(output,ec);
        } else if(std::filesystem::is_regular_file(status)) {
            std::filesystem::create_directories(output.parent_path(),ec);
            if(!ec)std::filesystem::copy_file(entry.path(),output,std::filesystem::copy_options::none,ec);
        } else throw Failure{Error("editor.folder_clone","Unsupported source entry kind")};
        Require(!ec,"editor.folder_clone","Cannot clone source file");
    }
}
} // namespace

struct ProjectFolder::WriterLock {
#ifdef _WIN32
    HANDLE handle=INVALID_HANDLE_VALUE;
    ~WriterLock() { if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle); }
#else
    int fd=-1;
    ~WriterLock() { if(fd>=0){ (void)flock(fd,LOCK_UN);(void)close(fd); } }
#endif
};
ProjectFolder::ProjectFolder(std::filesystem::path root,std::string title,const ProjectSchema &schema)
    : root_(std::move(root)),title_(std::move(title)),schema_(schema) {}
ProjectFolder::~ProjectFolder()=default;

VoidResult ProjectFolder::AcquireLock() {
    lock_=std::make_unique<WriterLock>();
    const auto filename=root_/".epidemic/workspace.lock";
#ifdef _WIN32
    lock_->handle=CreateFileW(filename.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(lock_->handle==INVALID_HANDLE_VALUE) {lock_.reset();return VoidResult::Failure(Error("editor.folder_locked","Folder is locked by another process"));}
#else
    lock_->fd=open(filename.c_str(),O_CREAT|O_RDWR|O_CLOEXEC,0600);
    if(lock_->fd<0 || flock(lock_->fd,LOCK_EX|LOCK_NB)!=0){lock_.reset();return VoidResult::Failure(Error("editor.folder_locked","Folder is locked by another process"));}
#endif
    return VoidResult::Success();
}
Result<std::unique_ptr<ProjectFolder>> ProjectFolder::CreateWithIds(const std::filesystem::path &root,std::string title,
    ProjectDefinition definition,const ProjectSchema &schema,
    const std::vector<std::pair<std::uint64_t,std::uint64_t>> &retired) {
    using R=Result<std::unique_ptr<ProjectFolder>>;
    try {
        CheckTitle(title);
        std::error_code ec;
        Require(!std::filesystem::exists(root,ec) && !ec,"editor.folder_exists","Destination must not exist");
        std::filesystem::create_directories(root,ec);Require(!ec,"editor.folder_io","Cannot create destination");
        auto canonical=RootPath(root);EnsureDirectories(canonical);
        auto folder=std::unique_ptr<ProjectFolder>(new ProjectFolder(canonical,std::move(title),schema));
        Take(folder->AcquireLock());
        folder->editor_=Take(EditorProject::Create(SafeAt(canonical,AuthoringPath(1)),std::move(definition),schema));
        for(const auto &id:retired) folder->editor_->used_ids_.insert(id);
        Require(folder->editor_->used_ids_.size()<=65536,"editor.identity_limit","Retired identity budget exceeded");
        Take(folder->Save());
        return R::Success(std::move(folder));
    } catch(const Failure &failure) {return R::Failure(failure.error);}
    catch(const std::exception &) {return R::Failure(Error("editor.folder_io","Create project folder failed"));}
}
Result<std::unique_ptr<ProjectFolder>> ProjectFolder::Create(const std::filesystem::path &root,std::string title,
    ProjectDefinition definition,const ProjectSchema &schema) {
    return CreateWithIds(root,std::move(title),std::move(definition),schema,{});
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named filesystem/source and mount/path roles intentionally share their public value types.
Result<std::unique_ptr<ProjectFolder>> ProjectFolder::ImportLegacy(const std::filesystem::path &root,
    const std::filesystem::path &legacy,std::string title,const ProjectSchema &schema) {
    using R=Result<std::unique_ptr<ProjectFolder>>;
    try {
        const auto bytes=Take(detail::ReadFile(legacy));
        auto archive=Take(detail::ReadProject(bytes,schema));
        // Validate original identity, including all retired IDs, before creating
        // a destination. The source file is always read-only.
        (void)Take(EditorProject::Open(legacy,schema));
        return CreateWithIds(root,std::move(title),std::move(archive.definition),schema,archive.used_ids);
    } catch(const Failure &failure){return R::Failure(failure.error);}
    catch(const std::exception &){return R::Failure(Error("editor.folder_legacy","Legacy project import failed"));}
}
Result<std::unique_ptr<ProjectFolder>> ProjectFolder::Open(const std::filesystem::path &root,const ProjectSchema &schema) {
    using R=Result<std::unique_ptr<ProjectFolder>>;
    try {
        auto canonical=RootPath(root);
        const auto commits=Commits(canonical);
        Require(!commits.empty(),"editor.folder_missing","No committed project descriptor");
        // Do not turn a newer unknown schema into a silent downgrade.
        const auto newest=Read(commits.front());
        if(newest) {auto doc=runtime::DecodeDocument(newest.Value());
            if(doc && doc.Value().GetTypeId()==kCommitType && doc.Value().GetSchemaVersion()!=kVersion)
                throw Failure{Error("editor.folder_version","Unsupported newer project version")};}
        std::optional<Restored> restored;
        for(const auto &commit:commits) {
            try {restored=RestoreCommit(canonical,commit,schema);break;}
            catch(const Failure &failure) {
                if(failure.error.HasCode("editor.folder_version") || failure.error.HasCode("editor.folder_schema"))throw;
                // Last-good fallback for a broken committed generation.
            }
        }
        if(!restored) throw Failure{Error("editor.folder_integrity","No intact committed project revision")};
        EnsureDirectories(canonical);
        auto folder=std::unique_ptr<ProjectFolder>(new ProjectFolder(canonical,restored->commit.title,schema));
        Take(folder->AcquireLock());
        folder->generation_=restored->commit.generation;
        folder->editor_=std::move(restored->editor);
        folder->assets_=std::move(restored->index.records);
        folder->next_asset_number_=restored->index.next;
        return R::Success(std::move(folder));
    } catch(const Failure &failure) {return R::Failure(failure.error);}
    catch(const std::exception &) {return R::Failure(Error("editor.folder_io","Open project folder failed"));}
}
VoidResult ProjectFolder::Save() {
    try {
        Require(editor_ && lock_ && generation_<std::numeric_limits<std::uint64_t>::max(),
            "editor.folder_state","Missing authoring model, lock, or generation overflow");
        // The process lock prevents concurrent editor writers; a manual external
        // edit to a committed marker is detected on reopening, not hot-reloaded.
        const auto next=generation_+1;
        SourceIndex index{next_asset_number_,assets_};
        CheckSourcesExist(root_,index);
        auto archive=detail::ProjectArchive{editor_->current_->definition,{editor_->used_ids_.begin(),editor_->used_ids_.end()}};
        const auto authored=Take(detail::WriteProject(archive,schema_));
        const auto sources=EncodeSources(index);
        const auto authoring_path=AuthoringPath(next), sources_path=SourcesPath(next);
        const auto folder_path=SafeAt(root_,RevisionDir(next));
        std::error_code ec;std::filesystem::create_directories(folder_path,ec);
        Require(!ec,"editor.folder_io","Cannot prepare revision directory");
        Take(Publish(SafeAt(root_,authoring_path),authored));
        Take(Publish(SafeAt(root_,sources_path),sources));
        // Read back staged bytes before writing the commit marker.
        const auto checked_authoring=Take(Read(SafeAt(root_,authoring_path)));
        const auto checked_sources=Take(Read(SafeAt(root_,sources_path)));
        Require(Hash(checked_authoring)==Hash(authored) && Hash(checked_sources)==Hash(sources),
            "editor.folder_integrity","Staged revision failed verification");
        Commit commit{next,editor_->current_->definition.id.Raw(),editor_->current_->definition.entry_scene.Raw(),
            Hash(authored),Hash(sources),title_,authoring_path,sources_path};
        const auto bytes=EncodeCommit(commit);
        // The commit marker is the LAST authoritative write. An interrupted
        // multi-file save leaves only uncommitted orphan revision files.
        Take(Publish(SafeAt(root_,CommitPath(next)),bytes));
        generation_=next;
        editor_->saved_state_=editor_->current_->state_id;
        editor_->saved_documents_.clear();
        for(const auto &doc:editor_->current_->definition.documents)
            editor_->saved_documents_.emplace(doc.id.Raw(),doc.revision);
        asset_dirty_=false;
        // Convenience descriptor, not recovery authority. Failure here does not
        // undo an already durable commit and does not turn Save into a false FAIL.
        (void)Publish(root_/"Project.epidemicproj",bytes);
        return VoidResult::Success();
    } catch(const Failure &failure) {return VoidResult::Failure(failure.error);}
    catch(const std::exception &) {return VoidResult::Failure(Error("editor.folder_io","Project commit failed"));}
}
Result<EditReceipt> ProjectFolder::Execute(const EditRequest &request) {return editor_->Execute(request);}
VoidResult ProjectFolder::Undo(){return editor_->Undo();}
VoidResult ProjectFolder::Redo(){return editor_->Redo();}
Result<std::unique_ptr<PlaySession>> ProjectFolder::Play(PlayOptions options) const {return editor_->Play(std::move(options));}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- Named filesystem/source and mount/path roles intentionally share their public value types.
Result<std::filesystem::path> ProjectFolder::ResolveMount(std::string_view mount,std::string_view relative) const {
    if(mount!="Content" && mount!="Scripts" && mount!="ProjectData" && mount!="Cache")
        return Result<std::filesystem::path>::Failure(Error("editor.folder_mount","Unknown project mount"));
    const auto prefix=mount=="Cache"?".epidemic/Cache":std::string(mount);
    return ResolveProjectPath(root_,prefix+"/"+std::string(relative));
}
Result<runtime::AssetId> ProjectFolder::IndexSource(std::string_view path,runtime::AssetType type) {
    using R=Result<runtime::AssetId>;
    try {
        Require(type.IsValid() && !path.empty() && path.size()<runtime::kMaxAssetPathBytes && next_asset_number_<std::numeric_limits<std::uint64_t>::max() && assets_.size()<kMaxSources,
            "editor.folder_sources","Invalid source registration");
        const auto qualified="Content/"+std::string(path);
        const auto source=SafeAt(root_,qualified);
        std::error_code ec;
        Require(std::filesystem::is_regular_file(source,ec) && !ec,"editor.folder_source","Source file missing");
        auto folded=qualified;
        std::transform(folded.begin(),folded.end(),folded.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        for(const auto &a:assets_) {auto old=a.relative_path;
            std::transform(old.begin(),old.end(),old.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
            Require(old!=folded,"editor.folder_sources","Source path already indexed");
        }
        runtime::AssetId id{};
        do {
            const auto key="epidemic.project.asset/"+std::to_string(editor_->Snapshot()->definition.id.Raw())+"/"+std::to_string(next_asset_number_++);
            id=runtime::AssetId{foundation::StringId::FromString(key)};
        } while(!id.IsValid() || editor_->Snapshot()->assets->Contains(id) ||
                std::any_of(assets_.begin(),assets_.end(),[&](const auto &a){return a.id==id;}));
        assets_.push_back({id,type,qualified,HashFileStream(source),1});asset_dirty_=true;
        return R::Success(id);
    } catch(const Failure &f) {return R::Failure(f.error);}
    catch(const std::exception &) {return R::Failure(Error("editor.folder_source","Cannot index source"));}
}
VoidResult ProjectFolder::ReindexSource(runtime::AssetId id) {
    try {
        const auto it=std::find_if(assets_.begin(),assets_.end(),[&](const auto &a){return a.id==id;});
        Require(it!=assets_.end() && it->generation<std::numeric_limits<std::uint32_t>::max(),
            "editor.folder_sources","Unknown source or generation overflow");
        const auto source=SafeAt(root_,it->relative_path);
        const auto next_hash=HashFileStream(source);
        if(next_hash!=it->source_hash){it->source_hash=next_hash;++it->generation;asset_dirty_=true;}
        return VoidResult::Success();
    } catch(const Failure &f) {return VoidResult::Failure(f.error);}
    catch(const std::exception &) {return VoidResult::Failure(Error("editor.folder_source","Source refresh failed"));}
}
VoidResult ProjectFolder::MoveSource(runtime::AssetId id, std::string_view new_content_path) {
    try {
        Require(!new_content_path.empty() && new_content_path.size()<runtime::kMaxAssetPathBytes-8,
            "editor.folder_path", "Invalid destination path");
        const auto found=std::find_if(assets_.begin(),assets_.end(),[&](const auto &asset){return asset.id==id;});
        Require(found!=assets_.end(),"editor.folder_sources","Unknown asset ID");
        auto relative="Content/"+std::string(new_content_path);
        const auto old_path=SafeAt(root_,found->relative_path);
        const auto new_path=SafeAt(root_,relative);
        Require(old_path!=new_path,"editor.folder_path","Destination matches source");
        auto folded=relative;
        std::transform(folded.begin(),folded.end(),folded.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        for(const auto &asset:assets_) if(asset.id!=id) {
            auto other=asset.relative_path;
            std::transform(other.begin(),other.end(),other.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
            Require(other!=folded,"editor.folder_path","Destination conflicts with another asset");
        }
        std::error_code ec;
        Require(std::filesystem::is_regular_file(old_path,ec) && !ec,"editor.folder_source","Missing source file");
        Require(!std::filesystem::exists(new_path,ec) && !ec,"editor.folder_path","Destination exists");
        Require(std::filesystem::is_directory(new_path.parent_path(),ec) && !ec,"editor.folder_path","Destination directory does not exist");
        std::filesystem::rename(old_path,new_path,ec);
        Require(!ec,"editor.folder_io","Cannot move source file");
        found->relative_path.swap(relative);asset_dirty_=true;
        return VoidResult::Success();
    } catch(const Failure &f) {return VoidResult::Failure(f.error);}
    catch(const std::exception &) {return VoidResult::Failure(Error("editor.folder_io","Move source failed"));}
}
Result<std::shared_ptr<const runtime::IAssetCatalog>> ProjectFolder::BuildSourceCatalog() const {
    using R=Result<std::shared_ptr<const runtime::IAssetCatalog>>;
    auto catalog=runtime::CreateAssetServices();if(!catalog)return R::Failure(catalog.GetError());
    for(const auto &a:assets_) {
        runtime::AssetMetadata metadata;
        metadata.id=a.id;metadata.type=a.type;
        metadata.location={runtime::AssetLocationKind::FilePath,a.relative_path};
        metadata.state=runtime::AssetState::Indexed;
        metadata.content_hash=a.source_hash;metadata.version=static_cast<std::uint32_t>(a.generation);
        auto accepted=catalog.Value().writer->RegisterAsset(metadata);if(!accepted)return R::Failure(accepted.GetError());
    }
    auto sealed=catalog.Value().writer->Seal();if(!sealed)return R::Failure(sealed.GetError());
    return R::Success(catalog.Value().catalog);
}
VoidResult ProjectFolder::CloneTo(const std::filesystem::path &destination) const {
    try {
        Require(!IsDirty(),"editor.folder_dirty","Save before cloning a project");
        std::error_code ec;
        const auto target_candidate=std::filesystem::weakly_canonical(destination,ec);
        Require(!ec && !IsSubpath(root_,target_candidate),"editor.folder_clone","Clone destination may not be inside the source folder");
        Require(!std::filesystem::exists(destination,ec) && !ec,"editor.folder_exists","Clone destination must not exist");
        std::filesystem::create_directories(destination,ec);Require(!ec,"editor.folder_clone","Cannot create clone directory");
        const auto target=RootPath(destination);EnsureDirectories(target);
        const auto marker=SafeAt(root_,CommitPath(generation_));
        auto commit=RestoreCommit(root_,marker,schema_);
        const auto copy=[&](std::string_view p){const auto src=SafeAt(root_,p);const auto dst=SafeAt(target,p);
            std::filesystem::create_directories(dst.parent_path(),ec);Require(!ec,"editor.folder_clone","Cannot prepare clone path");
            std::filesystem::copy_file(src,dst,std::filesystem::copy_options::none,ec);
            Require(!ec,"editor.folder_clone","Cannot copy committed project file");};
        copy(AuthoringPath(generation_));copy(SourcesPath(generation_));
        CopyTree(root_/"Content",target/"Content");CopyTree(root_/"Scripts",target/"Scripts");
        // Source files and hashes must be valid before publishing the marker.
        CheckSourcesExist(target,commit.index);
        // A clone interrupted before this point is not a committed project.
        const auto bytes=Take(Read(marker));
        Take(Publish(target/CommitPath(generation_),bytes));
        (void)Publish(target/"Project.epidemicproj",bytes);
        return VoidResult::Success();
    } catch(const Failure &f){return VoidResult::Failure(f.error);}
    catch(const std::exception &) {return VoidResult::Failure(Error("editor.folder_clone","Cannot clone project folder"));}
}
} // namespace epidemic::game::editor
