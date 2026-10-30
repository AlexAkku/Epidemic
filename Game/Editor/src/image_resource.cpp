#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Game/Editor/project_path.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Serialization/document_codec.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#endif
namespace epidemic::game::editor {
namespace {
constexpr std::string_view kRecipe="png/wic/v1;windows-pbgra8;srgb;nearest;clamp;no-mip;no-trim";
const auto kImageType=foundation::StringId::FromString("epidemic.editor.cooked_image");
constexpr runtime::SchemaVersion kVersion{1,0,0};
struct Failure : std::exception { foundation::Error error; explicit Failure(foundation::Error value):error(std::move(value)){} };
foundation::Error Error(std::string_view code) { return foundation::Error::Create(code,"Image pipeline rejected candidate"); }
void Require(bool pass,std::string_view code){if(!pass)throw Failure{Error(code)};}
template<class T> T Take(foundation::Result<T> result){if(!result)throw Failure{result.GetError()};return std::move(result).Value();}
void Take(const foundation::Result<void> &result){if(!result)throw Failure{result.GetError()};}
bool Key(std::string_view key){return key.size()==64 && key.find_first_not_of("0123456789abcdef")==std::string_view::npos;}
std::filesystem::path Cooked(const std::filesystem::path &root,std::string_view key){
    Require(Key(key),"editor.image_key");
    return Take(ResolveProjectPath(root,".epidemic/Cache/images/"+std::string(key)+".image"));
}
struct SourceDigest {std::string key;std::uint64_t hash=14695981039346656037ULL;};
SourceDigest Fingerprint(const std::filesystem::path &path,const std::atomic_bool *cancel=nullptr){
#ifdef _WIN32
    struct Hash {
        BCRYPT_ALG_HANDLE algorithm=nullptr; BCRYPT_HASH_HANDLE hash=nullptr; std::vector<unsigned char> object;
        ~Hash(){if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}
    } state;
    Require(BCryptOpenAlgorithmProvider(&state.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"editor.image_digest");
    DWORD size=0,written=0;
    Require(BCryptGetProperty(state.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&written,0)>=0,"editor.image_digest");
    state.object.resize(size);
    Require(BCryptCreateHash(state.algorithm,&state.hash,state.object.data(),size,nullptr,0,0)>=0,"editor.image_digest");
    // The hash provider only reads these bytes; its legacy API lacks const.
    auto recipe=std::string(kRecipe);
    Require(BCryptHashData(state.hash,reinterpret_cast<PUCHAR>(recipe.data()),static_cast<ULONG>(recipe.size()),0)>=0,"editor.image_digest");
    std::ifstream input(path,std::ios::binary);Require(input.good(),"editor.image_source");
    SourceDigest result;
    std::array<unsigned char,65536> bytes{};std::uint64_t total=0;
    while(input){input.read(reinterpret_cast<char *>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
        Require(!cancel || !cancel->load(std::memory_order_relaxed),"editor.image_cancelled");
        total+=static_cast<std::uint64_t>(input.gcount());Require(total<=128ULL*1024*1024,"editor.png_source_limit");
        for(std::streamsize i=0;i<input.gcount();++i){result.hash^=bytes[static_cast<std::size_t>(i)];result.hash*=1099511628211ULL;}
        Require(BCryptHashData(state.hash,bytes.data(),static_cast<ULONG>(input.gcount()),0)>=0,"editor.image_digest");}
    Require(input.eof() && !input.bad(),"editor.image_source");
    std::array<unsigned char,32> digest{};Require(BCryptFinishHash(state.hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0,"editor.image_digest");
    constexpr char hex[]="0123456789abcdef";std::string key;key.reserve(64);
    for(const auto byte:digest){key.push_back(hex[byte/16]);key.push_back(hex[byte%16]);}result.key=std::move(key);return result;
#else
    (void)path;(void)cancel;throw Failure{Error("editor.png_windows_only")};
#endif
}
}
foundation::Result<runtime::AssetId> CookProjectPng(ProjectFolder &project,std::string_view relative,const std::atomic_bool *cancel){
    using R=foundation::Result<runtime::AssetId>;
    try{
        auto prepared=Take(PrepareCookedPng(project.Root(),relative,cancel));
        for(const auto &record:project.Sources())if(record.relative_path=="Content/"+std::string(relative)){
            const auto id=record.id;Take(project.PublishCooked(id,prepared));return R::Success(id);
        }
        return project.IndexSource(relative,{foundation::StringId::FromString("epidemic.texture.png")},prepared.Key());
    }catch(const Failure &failure){return R::Failure(failure.error);}
    catch(const std::exception &){return R::Failure(Error("editor.image_failure"));}
}
runtime::ResourceType ImageResourceLoader::GetResourceType() const{return {kImageType};}
foundation::Result<runtime::ResourceId> ImageResourceLoader::Register(std::string key){
    using R=foundation::Result<runtime::ResourceId>;
    if(!Key(key))return R::Failure(Error("editor.image_key"));
    const auto id=runtime::ResourceId::FromString("epidemic.editor.image/"+key);
    if(const auto entry=keys_.find(id.Raw());entry!=keys_.end()){
        if(entry->second!=key)return R::Failure(Error("editor.image_key_collision"));return R::Success(id);}
    keys_.emplace(id.Raw(),std::move(key));return R::Success(id);
}
void ImageResourceLoader::CollectUnused(runtime::IResourceManager &manager){
    for(auto entry=keys_.begin();entry!=keys_.end();){
        const auto evicted=manager.Evict(runtime::ResourceId{foundation::StringId{entry->first}});
        if(evicted || evicted.GetError().HasCode("resource.not_found"))entry=keys_.erase(entry);
        else ++entry; // In-use leases, including pending requests, retain their key.
    }
}
foundation::Result<void> ImageResourceLoader::StagePrepared(runtime::ResourceLoadArtifact artifact){
    const auto image=std::dynamic_pointer_cast<const ImagePayload>(artifact.payload);
    if(!keys_.contains(artifact.resource_id.Raw()) || artifact.type!=GetResourceType() || !artifact.dependencies.empty() ||
       !image || image->image.width==0 || image->image.height==0 || image->image.width>4096 || image->image.height>4096 ||
       image->image.pixels.size()!=std::size_t{image->image.width}*image->image.height*4)
        return foundation::Result<void>::Failure(Error("editor.image_prepared"));
    prepared_=std::move(artifact);return foundation::Result<void>::Success();
}
foundation::Result<runtime::ResourceLoadArtifact> ImageResourceLoader::Load(runtime::ResourceRequest request){
    using R=foundation::Result<runtime::ResourceLoadArtifact>;
    try{
        Require(request.type==GetResourceType(),"editor.image_type");
        if(prepared_ && prepared_->resource_id==request.resource_id){
            auto ready=std::move(*prepared_);prepared_.reset();return R::Success(std::move(ready));
        }
        const auto entry=keys_.find(request.resource_id.Raw());
        Require(entry!=keys_.end(),"editor.image_unknown");
        const auto bytes=Take(platform::ReadFilePackage(foundation::Path{Cooked(root_,entry->second)},runtime::kMaxDocumentBytes));
        const auto document=Take(runtime::DecodeDocument(bytes));
        Require(document.GetTypeId()==kImageType && document.GetSchemaVersion()==kVersion,"editor.image_version");
        auto services=Take(runtime::CreateSerializationServices());auto reader=Take(services.archives->CreateReader(document));
        const auto width=Take(reader->ReadUInt64("width")),height=Take(reader->ReadUInt64("height"));
        Require(width>0 && height>0 && width<=4096 && height<=4096,"editor.image_dimensions");
        Require(Take(reader->ReadString("recipe"))==kRecipe && Take(reader->ReadString("key"))==entry->second,"editor.image_recipe");
        auto pixels=Take(reader->ReadBytes("pixels"));Require(pixels.size()==width*height*4,"editor.image_stride");
        auto payload=std::make_shared<ImagePayload>();payload->image={static_cast<std::uint32_t>(width),static_cast<std::uint32_t>(height),std::move(pixels)};
        return R::Success({request.resource_id,request.type,std::move(payload),{}});
    }catch(const Failure &failure){return R::Failure(failure.error);}
    catch(const std::exception &){return R::Failure(Error("editor.image_load"));}
}
foundation::Result<PreparedCookedPng> PrepareCookedPng(
    const std::filesystem::path &root,std::string_view relative,const std::atomic_bool *cancel,std::string_view previous_key){
    using R=foundation::Result<PreparedCookedPng>;
    try {
        const auto check=[&]{Require(!cancel || !cancel->load(std::memory_order_relaxed),"editor.image_cancelled");};
        check();
        const auto path=Take(ResolveProjectPath(root,"Content/"+std::string(relative)));
        PreparedCookedPng prepared;
#ifdef _WIN32
        const auto handle=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        Require(handle!=INVALID_HANDLE_VALUE,"editor.image_source_busy");
        prepared.source_read_=std::shared_ptr<void>(handle,[](void *value){CloseHandle(value);});
#endif
        const auto digest=Fingerprint(path,cancel);
        prepared.key_=digest.key;prepared.source_hash_=digest.hash;prepared.root_=root;prepared.relative_=relative;
        const auto &key=prepared.key_;
        check();
        // Periodic audit/startup reconciliation needs hashing, not repeat decoding.
        if(key==previous_key && std::filesystem::is_regular_file(Cooked(root,key)))return R::Success(std::move(prepared));
        auto image=Take(DecodePng(path));
        check();
        Require(image.pixels.size()<runtime::kMaxDocumentBytes-4096,"editor.image_cooked_limit");
        // The source lease denies writes/delete, so hash and decoder see one file.
        auto services=Take(runtime::CreateSerializationServices());auto writer=services.archives->CreateWriter();
        Require(writer!=nullptr,"editor.image_codec");
        Take(writer->WriteUInt64("width",image.width));Take(writer->WriteUInt64("height",image.height));
        Take(writer->WriteString("recipe",kRecipe));Take(writer->WriteString("key",key));
        Take(writer->WriteBytes("pixels",image.pixels));
        const auto bytes=Take(runtime::EncodeDocument(Take(writer->Finalize(kImageType,kVersion))));
        const auto destination=Cooked(root,key);
        std::error_code ec;std::filesystem::create_directories(destination.parent_path(),ec);
        Require(!ec,"editor.image_cache");
        check();
        Take(platform::CommitFilePackage(foundation::Path{destination},bytes,runtime::kMaxDocumentBytes));
        check();
        prepared.rebuilt_=true;
        return R::Success(std::move(prepared));
    }catch(const Failure &failure){return R::Failure(failure.error);}
    catch(const std::exception &){return R::Failure(Error("editor.image_prepare"));}
}
foundation::Result<PreparedCookedPng> PrepareImportedPng(const std::filesystem::path &root,
    const std::filesystem::path &external,const std::atomic_bool *cancel){
    using R=foundation::Result<PreparedCookedPng>;
    try{
#ifdef _WIN32
        const auto check=[&]{Require(!cancel || !cancel->load(std::memory_order_relaxed),"editor.image_cancelled");};check();
        const auto input_handle=CreateFileW(external.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        Require(input_handle!=INVALID_HANDLE_VALUE,"editor.image_source_busy");
        auto input_lease=std::shared_ptr<void>(input_handle,[](void *handle){CloseHandle(handle);});
        std::array<unsigned char,16> random{};
        Require(BCryptGenRandom(nullptr,random.data(),static_cast<ULONG>(random.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0,"editor.image_stage_id");
        constexpr char digits[]="0123456789abcdef";std::string relative=".imports/";
        for(const auto byte:random){relative+=digits[byte/16];relative+=digits[byte%16];}relative+=".png";
        const auto stage=Take(ResolveProjectPath(root,"Content/"+relative));std::error_code error;
        std::filesystem::create_directories(stage.parent_path(),error);Require(!error,"editor.image_stage");
        (void)Take(ResolveProjectPath(root,"Content/"+relative));
        Require(!std::filesystem::exists(stage),"editor.image_stage_collision");
        auto cleanup=std::shared_ptr<const std::filesystem::path>(new std::filesystem::path(stage),[](const std::filesystem::path *path){
            std::error_code ignored;std::filesystem::remove(*path,ignored);delete path;
        });
        {
            std::ifstream input(external,std::ios::binary);std::ofstream output(stage,std::ios::binary);
            Require(input.good() && output.good(),"editor.image_copy");
            std::array<char,65536> bytes{};std::uint64_t copied=0;
            while(input){input.read(bytes.data(),static_cast<std::streamsize>(bytes.size()));check();
                copied+=static_cast<std::uint64_t>(input.gcount());Require(copied<=128ULL*1024*1024,"editor.png_source_limit");
                output.write(bytes.data(),input.gcount());Require(output.good(),"editor.image_copy");
            }
            Require(input.eof() && !input.bad(),"editor.image_copy");output.flush();Require(output.good(),"editor.image_copy");
        }
        input_lease.reset();
        auto prepared=Take(PrepareCookedPng(root,relative,cancel));prepared.staging_cleanup_=std::move(cleanup);
        return R::Success(std::move(prepared));
#else
        (void)root;(void)external;(void)cancel;return R::Failure(Error("editor.png_windows_only"));
#endif
    }catch(const Failure &failure){return R::Failure(failure.error);}
    catch(const std::exception &){return R::Failure(Error("editor.image_copy"));}
}
DecodedPng MakeImageThumbnail(const DecodedPng &image){
    if(!image.width || !image.height || image.width>4096 || image.height>4096 || image.pixels.size()!=static_cast<std::size_t>(image.width)*image.height*4)return {};
    const auto divisor=std::max(1U,(std::max(image.width,image.height)+127)/128);
    DecodedPng out;out.width=std::max(1U,image.width/divisor);out.height=std::max(1U,image.height/divisor);
    out.pixels.resize(static_cast<std::size_t>(out.width)*out.height*4);
    for(std::uint32_t y=0;y<out.height;++y)for(std::uint32_t x=0;x<out.width;++x){
        const auto source=(static_cast<std::size_t>(y)*divisor*image.width+static_cast<std::size_t>(x)*divisor)*4;
        const auto dest=(static_cast<std::size_t>(y)*out.width+x)*4;
        std::copy_n(image.pixels.begin()+static_cast<std::ptrdiff_t>(source),4,out.pixels.begin()+static_cast<std::ptrdiff_t>(dest));}
    return out;
}
} // namespace epidemic::game::editor
