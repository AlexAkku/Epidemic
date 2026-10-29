#pragma once
// P04.0 test-only spike: MOCKPNG is not a decoder or a production project format.
#include "Epidemic/Runtime/Assets/asset_services.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Serialization/document_codec.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include "Epidemic/Game/Editor/project_path.h"
#include <filesystem>
#include <span>
#include <string>

namespace p04_spike {
using namespace epidemic;
using namespace composition_tests;
const auto manifest_type=foundation::StringId::FromString("test.p04.manifest");
const auto asset_id=runtime::AssetId::FromString("p04.spike.stable.asset");
inline std::filesystem::path Root() { return std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY "/p04-spike"}; }
struct Manifest { std::uint64_t id=0, generation=0; std::string source, cooked; };
inline void Commit(const std::filesystem::path &path,std::string_view value) {
    Take(platform::CommitFilePackage(foundation::Path{path},std::as_bytes(std::span{value.data(),value.size()}),4096));
}
inline std::string Read(const std::filesystem::path &path) {
    const auto bytes=Take(platform::ReadFilePackage(foundation::Path{path},4096));
    if(bytes.empty()) return {};
    return std::string(reinterpret_cast<const char *>(bytes.data()),bytes.size());
}
inline void Save(const Manifest &m) {
    auto s=Take(runtime::CreateSerializationServices()); auto w=s.archives->CreateWriter();
    Check(w!=nullptr,"manifest writer");
    Take(w->WriteUInt64("id",m.id)); Take(w->WriteUInt64("generation",m.generation));
    Take(w->WriteString("source",m.source)); Take(w->WriteString("cooked",m.cooked));
    auto bytes=Take(runtime::EncodeDocument(Take(w->Finalize(manifest_type,{1,0,0}))));
    Take(platform::CommitFilePackage(foundation::Path{Root()/"manifest.spike"},bytes,4096));
}
inline Manifest LoadAt(const std::filesystem::path &directory) {
    auto bytes=Take(platform::ReadFilePackage(foundation::Path{directory/"manifest.spike"},4096));
    auto doc=Take(runtime::DecodeDocument(bytes));
    Check(doc.GetTypeId()==manifest_type && doc.GetSchemaVersion()==runtime::SchemaVersion{1,0,0},"manifest schema");
    auto s=Take(runtime::CreateSerializationServices()); auto r=Take(s.archives->CreateReader(doc));
    return {Take(r->ReadUInt64("id")),Take(r->ReadUInt64("generation")),Take(r->ReadString("source")),Take(r->ReadString("cooked"))};
}
inline Manifest Load() { return LoadAt(Root()); }
inline void CheckCatalog(const Manifest &m) {
    auto a=Take(runtime::CreateAssetServices()); runtime::AssetMetadata meta;
    meta.id=runtime::AssetId{foundation::StringId{m.id}}; meta.type={foundation::StringId::FromString("test.mock.png")};
    meta.location={runtime::AssetLocationKind::FilePath,m.source}; meta.state=runtime::AssetState::Indexed; meta.version=1;
    Take(a.writer->RegisterAsset(meta)); Take(a.writer->Seal());
    Check(a.catalog->IsSealed() && a.catalog->FindById(asset_id).has_value(),"sealed catalog lookup");
    meta.id=runtime::AssetId::FromString("p04.spike.other.asset");
    Check(!a.writer->RegisterAsset(meta),"sealed catalog refuses new registration");
}
inline bool Reimport(Manifest &m,std::string_view candidate) {
    if(!candidate.starts_with("MOCKPNG:")) return false;
    const auto next=m.generation+1; const auto cooked="Cache/mock-"+std::to_string(next);
    Commit(Root()/cooked,candidate); auto updated=m; updated.generation=next; updated.cooked=cooked;
    Save(updated); m=std::move(updated); return true;
}
inline void Write() {
    std::filesystem::create_directories(Root()/"Content"); std::filesystem::create_directories(Root()/"Cache");
    for(const auto path:{"../outside","/absolute","C:/drive","Content/../escape","Content//empty","NUL.txt","Content/file:stream","Content/trailing.","Content/"})
        Check(!game::editor::ResolveProjectPath(Root(),path),"unsafe project path rejected");
    const auto checked=Take(game::editor::ResolveProjectPath(Root(),"Content/mock.png"));
    Check(checked==std::filesystem::canonical(Root())/"Content/mock.png","portable relative path resolves inside root");
    Manifest m{asset_id.Raw(),0,"Content/mock.png",""}; Commit(Root()/m.source,"MOCKPNG:first");
    Check(Reimport(m,Read(Root()/m.source)),"first mock import"); CheckCatalog(m);
}
inline void ReadAndReimport() {
    auto m=Load(); Check(m.id==asset_id.Raw() && m.generation==1,"cross-process identity and generation");
    CheckCatalog(m); const auto old=Read(Root()/m.cooked);
    Commit(Root()/m.source,"bad candidate"); Check(!Reimport(m,Read(Root()/m.source)),"invalid candidate rejected");
    auto unchanged=Load(); Check(unchanged.generation==1 && Read(Root()/unchanged.cooked)==old,"last-good preserved");
    Commit(Root()/m.source,"MOCKPNG:second"); Check(Reimport(m,Read(Root()/m.source)),"second mock import");
    const auto updated=Load(); Check(updated.id==asset_id.Raw() && updated.generation==2 && Read(Root()/updated.cooked)=="MOCKPNG:second","stable ID and new generation");
    Check(Read(Root()/unchanged.cooked)==old,"prior cooked version retained"); CheckCatalog(updated);
    // A prepared but unpublished generation must leave the committed manifest usable.
    Commit(Root()/"Cache/mock-orphan","MOCKPNG:orphan");
    const auto after_interruption=Load();
    Check(after_interruption.id==updated.id && after_interruption.generation==updated.generation &&
        Read(Root()/after_interruption.cooked)=="MOCKPNG:second","unpublished candidate leaves manifest unchanged");
    // Copy only the committed manifest and its relative source/cooked files.
    const auto relocated=Root()/"relocated";
    std::filesystem::create_directories(relocated/"Content");
    std::filesystem::create_directories(relocated/"Cache");
    std::filesystem::copy_file(Root()/"manifest.spike",relocated/"manifest.spike",std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(Root()/updated.source,relocated/updated.source,std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(Root()/updated.cooked,relocated/updated.cooked,std::filesystem::copy_options::overwrite_existing);
    const auto copied=LoadAt(relocated);
    Check(copied.id==asset_id.Raw() && copied.generation==2 && Read(relocated/copied.source)=="MOCKPNG:second" &&
        Read(relocated/copied.cooked)=="MOCKPNG:second","relocated mock project preserves identity and relative files");
}
} // namespace p04_spike
